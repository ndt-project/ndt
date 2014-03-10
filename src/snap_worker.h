/**
 * This file contains the functions used for the thread that takes periodic
 * snapshots of the TCP statistics.
 *
 * Aaron Brown 2014-03-10
 * aaron@internet2.edu
 */

#ifndef SNAP_WORKER_H
#define SNAP_WORKER_H

#include <pthread.h>

#include "web100srv.h"

typedef struct SnapWorker {
  double delay;

  SnapResults *results;

  int do_exit;
  pthread_t pthread;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} SnapWorker;

SnapResults *alloc_snap_results(int max_snapshots, tcp_stat_agent *agent, tcp_stat_connection conn, tcp_stat_group *group);
void free_snap_results(SnapResults *results);
SnapWorker *alloc_snap_worker(tcp_stat_agent* agent, tcp_stat_group *group,
                       tcp_stat_connection conn, double delay, int max_snapshots);
void free_snap_worker(SnapWorker *worker);
SnapWorker *start_snap_worker(tcp_stat_agent* agent, tcp_stat_group *group,
                       tcp_stat_connection conn, int delay, int test_length);
SnapResults *stop_snap_worker(SnapWorker *worker);

#endif
