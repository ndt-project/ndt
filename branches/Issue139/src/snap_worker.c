/**
 * This file contains the functions used for the thread that takes periodic
 * snapshots of the TCP statistics.
 *
 * Aaron Brown 2014-03-10
 * aaron@internet2.edu
 */

#include <tgmath.h>
#include <pthread.h>

#include "logging.h"
#include "snap_worker.h"
#include "utils.h"

static void *snap_worker(void* arg);

SnapResults *alloc_snap_results(int max_snapshots, tcp_stat_agent *agent, tcp_stat_connection conn, tcp_stat_group *group) {
  SnapResults *snapResults;
  int i;

  snapResults = calloc(1, sizeof(SnapResults));
  if (!snapResults) {
    goto error_out;
  }

  snapResults->agent = agent;
  snapResults->conn  = conn;
  snapResults->group = group;

  snapResults->allocated = 0;
  snapResults->collected = 0;

  snapResults->snapshots = calloc(max_snapshots, sizeof(tcp_stat_snap *));
  if (!snapResults->snapshots) {
    goto error_out;
  }

  for(i = 0; i < max_snapshots; i++) {
    snapResults->snapshots[i] = tcp_stats_init_snapshot(agent, conn, group);
    if (!snapResults->snapshots[i]) {
      goto error_out;
    }

    snapResults->allocated++;
  }

  return snapResults;

error_out:
  free_snap_results(snapResults);
  return NULL;
}

void free_snap_results(SnapResults *results) {
  int i;

  for(i = 0; i < results->allocated; i++) {
    tcp_stats_free_snapshot(results->snapshots[i]);
  }

  free(results);
}

SnapWorker *alloc_snap_worker(tcp_stat_agent* agent, tcp_stat_group *group,
                       tcp_stat_connection conn, double delay, int max_snapshots) {
  SnapWorker *snapWorker = NULL;

  snapWorker = calloc(1, sizeof(SnapWorker));
  if (!snapWorker) {
    goto error_out;
  }

  snapWorker->delay = delay;

  if (pthread_mutex_init(&snapWorker->mutex, NULL) != 0) {
    goto error_out;
  }

  if (pthread_cond_init(&snapWorker->cond, NULL) != 0) {
    goto error_out;
  }

  snapWorker->results = alloc_snap_results(max_snapshots, agent, conn, group);
  if (!snapWorker->results) {
    goto error_out;
  }

  return snapWorker;

error_out:
  free_snap_worker(snapWorker);

  return NULL;
}

void free_snap_worker(SnapWorker *worker) {
  if (worker->results)
      free_snap_results(worker->results);

  pthread_mutex_destroy(&worker->mutex);
  pthread_cond_destroy(&worker->cond);

  free(worker);
}

/** Method to start snap worker thread that collects snap logs
 * @param snaparg object
 * @param tcp_stat_agent Agent
 * @param snaplogenabled Is snap logging enabled?
 * @param workerlooparg integer used to syncronize writing/reading from snaplog/tcp_stat snapshot
 * @param wrkrthreadidarg Thread Id of workera
 * @param metafilevariablename Which variable of the meta file gets assigned the snaplog name (unused now)
 * @param metafilename  value of metafile name
 * @param tcp_stat_connection connection pointer
 * @param tcp_stat_group group web100_group pointer
 */
SnapWorker *start_snap_worker(tcp_stat_agent* agent, tcp_stat_group *group,
                       tcp_stat_connection conn, int delay, int test_length) {
  SnapWorker *snapWorker = NULL;
  double converted_delay = ((double) delay) / 1000.0;

  // pad the maximum test length to be sure we have enough snapshots
  int max_number = ceil((test_length + 1) / converted_delay);

  snapWorker = alloc_snap_worker(agent, group, conn, converted_delay, max_number);
  if (!snapWorker) {
    goto error_out;
  }

  pthread_mutex_lock(&snapWorker->mutex);

  if (pthread_create(&snapWorker->pthread, NULL, snap_worker,
                     (void*) snapWorker)) {
    log_println(0, "Cannot create worker thread for writing snap log!");
    goto error_out;
  }

  // obtain web100 snap into "snaparg.snap"
  pthread_cond_wait(&snapWorker->cond, &snapWorker->mutex);
  pthread_mutex_unlock(&snapWorker->mutex);

  return snapWorker;

error_out:
  if (snapWorker)
    free_snap_worker(snapWorker);

  return NULL;
}

/**
 * Stop the snap worker process
 * @param workerThreadId Worker Thread's ID
 * @param snaplogenabled boolean indication whether snap logging is enabled
 * @param snapArgs_ptr  pointer to a snapArgs object
 * */
SnapResults *stop_snap_worker(SnapWorker *worker) {
  SnapResults *results;

  pthread_mutex_lock(&worker->mutex);
  worker->do_exit = 1;
  pthread_mutex_unlock(&worker->mutex);

  pthread_join(worker->pthread, NULL);

  results = worker->results;

  worker->results = NULL;

  free_snap_worker(worker);

  return results;
}

static void *snap_worker(void* arg) {
  SnapWorker  *workerArgs = (SnapWorker *) arg;
  SnapResults *results    = workerArgs->results;

  int i;

  pthread_mutex_lock(&workerArgs->mutex);
  pthread_cond_signal(&workerArgs->cond);

  i = 0;
  while (1) {
    if (workerArgs->do_exit) {
      pthread_mutex_unlock(&workerArgs->mutex);
      break;
    }

    if (i < results->allocated) {
      tcp_stats_take_snapshot(results->agent, results->conn, results->snapshots[i]);

      results->collected++;
      i++;
    }

    pthread_mutex_unlock(&workerArgs->mutex);
    mysleep(workerArgs->delay);
    pthread_mutex_lock(&workerArgs->mutex);
  }

  return NULL;
}
