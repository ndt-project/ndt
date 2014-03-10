/**
 * This file contains the functions needed to handle various tests.
 *
 * Jakub S�awi�ski 2006-06-24
 * jeremian@poczta.fm
 */

#include <assert.h>
#include <string.h>
// #include <ctype.h>
#include <pthread.h>

#include "testoptions.h"
#include "network.h"
#include "mrange.h"
#include "logging.h"
#include "utils.h"
#include "protocol.h"
#include "I2util/util.h"
#include "runningtest.h"
#include "strlutils.h"
#include "web100srv.h"


// Worker thread characteristics used to record snaplog and Cwnd peaks
typedef struct workerArgs {
  SnapArgs* snapArgs;  // snapArgs struct pointer
  tcp_stat_agent* agent;  // tcp_stat agent pointer
  CwndPeaks* peaks;  // data indicating Cwnd values
  int writeSnap;  // enable writing snaplog
} WorkerArgs;

int workerLoop = 0;
pthread_mutex_t mainmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t maincond = PTHREAD_COND_INITIALIZER;
static int slowStart = 1;
static int prevCWNDval = -1;
static int decreasing = 0;

/**
 * Count the CWND peaks from a snapshot and record the minimal and maximum one.
 * Also record the number of transitions between increasing or decreasing
 * trends of the values.
 * @param agent Web100 agent used to track the connection
 * @param peaks Structure containing CWND peaks information
 * @param snap Web100 snapshot structure
 */
void findCwndPeaks(tcp_stat_agent* agent, CwndPeaks* peaks,
                   tcp_stat_snap* snap) {
  int CurCwnd;
  char buf[128];

  tcp_stats_snap_read_var(agent, snap, "CurCwnd", buf, sizeof(buf));

  CurCwnd = atoi(buf);

  if (slowStart) {
    if (CurCwnd < prevCWNDval) {
      slowStart = 0;
      peaks->max = prevCWNDval;
      peaks->amount = 1;
      decreasing = 1;
    }
  } else {
    // current congestion window < previous value, so, decreasing
    if (CurCwnd < prevCWNDval) {
      // update values based on actual values
      if (prevCWNDval > peaks->max) {
        peaks->max = prevCWNDval;
      }
      if (!decreasing) {
        peaks->amount += 1;
      }
      decreasing = 1;
      // current congestion window size > previous value,
    } else if (CurCwnd > prevCWNDval) {
      // not decreasing.
      if ((peaks->min == -1) || (prevCWNDval < peaks->min)) {
        peaks->min = prevCWNDval;
      }
      decreasing = 0;
    }
  }
  prevCWNDval = CurCwnd;
}

/**
 * Print the appropriate message when the SIGALRM is caught.
 * @param signo The signal number (shuld be SIGALRM)
 */

void catch_s2c_alrm(int signo) {
  if (signo == SIGALRM) {
    log_println(1, "SIGALRM was caught");
    return;
  }
  log_println(0, "Unknown (%d) signal was caught", signo);
}

/**
 * Write the snap logs with fixed time intervals in a separate
 *              thread, locking and releasing resources as necessary.
 * @param arg pointer to the snapshot structure
 * @return void pointer null
 */

void*
snapWorker(void* arg) {
  /* WARNING void* arg (workerArgs) is on the stack of the function below and
   * doesn't exist forever. */
  WorkerArgs *workerArgs = (WorkerArgs*) arg;
  SnapArgs *snapArgs = workerArgs->snapArgs;
  tcp_stat_agent* agent = workerArgs->agent;
  CwndPeaks* peaks = workerArgs->peaks;
  int writeSnap = workerArgs->writeSnap;

  // snap log written into every "delay" milliseconds
  double delay = ((double) snapArgs->delay) / 1000.0;

  while (1) {
    pthread_mutex_lock(&mainmutex);
    if (workerLoop) {
      pthread_mutex_unlock(&mainmutex);
      pthread_cond_broadcast(&maincond);
      break;
    }
    pthread_mutex_unlock(&mainmutex);
    mysleep(0.01);
  }

  // Find Congestion window peaks from a web_100 snapshot, if enabled
  // Write snap log , if enabled, all in a synchronous manner.
  while (1) {
    pthread_mutex_lock(&mainmutex);
    if (!workerLoop) {
      pthread_mutex_unlock(&mainmutex);
      break;
    }
    tcp_stats_take_snapshot(agent, snapArgs->conn, snapArgs->snap);
    if (peaks) {
      findCwndPeaks(agent, peaks, snapArgs->snap);
    }
    if (writeSnap) {
      tcp_stats_write_snapshot(snapArgs->log, snapArgs->snap);
    }
    pthread_mutex_unlock(&mainmutex);
    mysleep(delay);
  }

  return NULL;
}

/**
 * Adds test id to the test suite
 * @param first first test indicator
 * @param buff test suite description
 * @param test_id id of the test
 */
void add_test_to_suite(int* first, char * buff, size_t buff_strlen,
                       int test_id) {
  char tmpbuff[16];
  if (*first) {
    *first = 0;
    snprintf(buff, buff_strlen, "%d", test_id);
  } else {
    memset(tmpbuff, 0, sizeof(tmpbuff));
    snprintf(tmpbuff, sizeof(tmpbuff), " %d", test_id);
    strlcat(buff, tmpbuff, buff_strlen);
  }
}

/**
 * Initialize the tests for the client.
 * @param ctlsockfd Client control socket descriptor
 * @param options   Test options
 * @param buff 		Connection options
 * @return integer 0 on success,
 *          >0 - error code.
 *          Error codes:
 *          -1 message reading in error
 *			-2 Invalid test request
 *			-3 Invalid test suite request
 *			-4 client timed out
 *
 */

int initialize_tests(int ctlsockfd, TestOptions* options, char * buff,
                     size_t buff_strlen) {
  unsigned char useropt = 0;
  int msgType;
  int msgLen = 1;
  int first = 1;
  char *invalid_test_suite = "Invalid test suite request.";
  char *client_timeout = "Client timeout.";
  char *invalid_test = "Invalid test request.";

  // char remhostarr[256], protologlocalarr[256];
  // char *remhost_ptr = get_remotehost();

  assert(ctlsockfd != -1);
  assert(options);

  // read the test suite request
  if (recv_msg(ctlsockfd, &msgType, &useropt, &msgLen)) {
    send_msg(ctlsockfd, MSG_ERROR, invalid_test_suite, 
      strlen(invalid_test_suite));
    return (-1);
  }
  if (msgLen == -1) {
    snprintf(buff, buff_strlen, "Client timeout");
    return (-4);
  }
  // Expecting a MSG_LOGIN with payload byte indicating tests to be run
  if ((msgType != MSG_LOGIN) || (msgLen != 1)) {
    send_msg(ctlsockfd, MSG_ERROR, invalid_test, strlen(invalid_test));
    return (-2);
  }
  // client connect received correctly. Logging activity
  // log that client connected, and create log file
  log_println(0,
              "Client connect received from :IP %s to some server on socket %d",
              get_remotehost(), ctlsockfd);

  // set_protologfile(get_remotehost(), protologlocalarr);

  if (!(useropt
        & (TEST_MID | TEST_C2S | TEST_S2C | TEST_SFW | TEST_STATUS
           | TEST_META))) {
    // message received does not indicate a valid test!
    send_msg(ctlsockfd, MSG_ERROR, invalid_test_suite, 
      strlen(invalid_test_suite));
    return (-3);
  }
  // construct test suite request based on user options received
  if (useropt & TEST_MID) {
    add_test_to_suite(&first, buff, buff_strlen, TEST_MID);
  }
  if (useropt & TEST_SFW) {
    add_test_to_suite(&first, buff, buff_strlen, TEST_SFW);
  }
  if (useropt & TEST_C2S) {
    add_test_to_suite(&first, buff, buff_strlen, TEST_C2S);
  }
  if (useropt & TEST_S2C) {
    add_test_to_suite(&first, buff, buff_strlen, TEST_S2C);
  }
  if (useropt & TEST_META) {
    add_test_to_suite(&first, buff, buff_strlen, TEST_META);
  }
  return useropt;
}

/** Method to start snap worker thread that collects snap logs
 * @param snaparg object
 * @param tcp_stat_agent Agent
 * @param snaplogenabled Is snap logging enabled?
 * @param workerlooparg integer used to syncronize writing/reading from snaplog/tcp_stat snapshot
 * @param wrkrthreadidarg Thread Id of workera
 * @param metafilevariablename Which variable of the meta file gets assigned the snaplog name (unused now)
 * @param metafilename	value of metafile name
 * @param tcp_stat_connection connection pointer
 * @param tcp_stat_group group web100_group pointer
 */
void start_snap_worker(SnapArgs *snaparg, tcp_stat_agent* agentarg,
                       CwndPeaks* peaks, char snaplogenabled,
                       pthread_t *wrkrthreadidarg, char *metafilevariablename,
                       char *metafilename, tcp_stat_connection conn,
                       tcp_stat_group* group) {
  FILE *fplocal;

  WorkerArgs workerArgs;
  workerArgs.snapArgs = snaparg;
  workerArgs.agent = agentarg;
  workerArgs.peaks = peaks;
  workerArgs.writeSnap = snaplogenabled;

  group = tcp_stats_get_group(agentarg, "read");

  snaparg->snap = tcp_stats_init_snapshot(agentarg, conn, group);

  if (snaplogenabled) {
    // memcpy(metafilevariablename, metafilename, strlen(metafilename));
    // The above could have been here, except for a caveat: metafile stores
    // just the file name, but full filename is needed to open the log file

    fplocal = fopen(get_logfile(), "a");

    snaparg->log = tcp_stats_open_log(metafilename, conn, group, "w");
    log_println( 0, "snaparg->log: %X", snaparg->log);

    if (fplocal == NULL) {
      log_println(
          0,
          "Unable to open log file '%s', continuing on without logging",
          get_logfile());
    } else {
      log_println(0, "Snaplog file: %s\n", metafilename);
      fprintf(fplocal, "Snaplog file: %s\n", metafilename);
      fclose(fplocal);
    }
  }

  if (pthread_create(wrkrthreadidarg, NULL, snapWorker,
                     (void*) &workerArgs)) {
    log_println(0, "Cannot create worker thread for writing snap log!");
    *wrkrthreadidarg = 0;
  }

  pthread_mutex_lock(&mainmutex);
  workerLoop= 1;
  // obtain web100 snap into "snaparg.snap"
  tcp_stats_take_snapshot(agentarg, conn, snaparg->snap);
  if (snaplogenabled) {
    tcp_stats_write_snapshot(snaparg->log, snaparg->snap);
  }
  pthread_cond_wait(&maincond, &mainmutex);
  pthread_mutex_unlock(&mainmutex);
}

/**
 * Stop snapWorker
 * @param workerThreadId Worker Thread's ID
 * @param snaplogenabled boolean indication whether snap logging is enabled
 * @param snapArgs_ptr  pointer to a snapArgs object
 * */
void stop_snap_worker(pthread_t *workerThreadId, char snaplogenabled,
                      SnapArgs* snapArgs_ptr) {
  if (*workerThreadId) {
    pthread_mutex_lock(&mainmutex);
    workerLoop = 0;
    pthread_mutex_unlock(&mainmutex);
    pthread_join(*workerThreadId, NULL);
  }
  // close writing snaplog, if snaplog recording is enabled
  if (snaplogenabled) {
    tcp_stats_close_log(snapArgs_ptr->log);
  }
  tcp_stats_free_snapshot(snapArgs_ptr->snap);
}

/**
 * Start packet tracing for this client
 * @param socketfdarg socket file descriptor to initialize packet trace from
 * @param socketfdarg socket file descriptor to close
 * @param childpid pid resulting from fork()
 * @param imonarg int array of socket fd from pipe
 * @param cliaddrarg sock_addr used to determine client IP
 * @param clilenarg socket length
 * @param device device type
 * @param pairarg portpair struct instance
 * @param testindicator string indicating C2S/S2c test
 * @param iscompressionenabled  is compression enabled  (for log files)?
 * @param copylocationarg memory location to copy resulting string (from packet trace) into
 * */

void start_packet_trace(int socketfdarg, int socketfdarg2, pid_t *childpid,
                        int *imonarg, struct sockaddr *cliaddrarg,
                        socklen_t clilenarg, char* device, PortPair* pairarg,
                        const char* testindicatorarg, int iscompressionenabled,
                        char *copylocationarg) {
  char tmpstr[256];
  int i, readretval;

  I2Addr src_addr = I2AddrByLocalSockFD(get_errhandle(), socketfdarg, 0);

  if ((*childpid = fork()) == 0) {
    close(socketfdarg2);
    close(socketfdarg);
    log_println(0, "%s test Child thinks pipe() returned fd0=%d, fd1=%d",
                testindicatorarg, imonarg[0], imonarg[1]);

    init_pkttrace(src_addr, cliaddrarg, clilenarg, imonarg, device,
                  pairarg, testindicatorarg, iscompressionenabled);

    exit(0);  // Packet trace finished, terminate gracefully
  }
  memset(tmpstr, 0, 256);
  for (i = 0; i < 5; i++) {  // read nettrace file name into "tmpstr"
    readretval = read(imonarg[0], tmpstr, 128);
    if ((readretval == -1) && (errno == EINTR))
      // socket interrupted, try reading again
      continue;
    break;
  }

  // name of nettrace file passed back from pcap child copied into meta
  // structure
  if (strlen(tmpstr) > 5) {
    memcpy(copylocationarg, tmpstr, strlen(tmpstr));
    log_println(8, "**start pkt trace, memcopied dir name %s", tmpstr);
  }

  // free this address now.
  I2AddrFree(src_addr);
}

/**
 * Stop packet tracing activity.
 * @param monpipe_arr pointer to the monitor pipe file-descriptor array
 *
 * */
void stop_packet_trace(int *monpipe_arr) {
  int retval;
  int i;

  for (i = 0; i < RETRY_COUNT; i++) {
    // retval = write(mon_pipe1[1], "c", 1);
    retval = write(monpipe_arr[1], "c", 1);
    if (retval == 1)
      break;
    if ((retval == -1) && (errno == EINTR))
      continue;
  }
  close(monpipe_arr[0]);
  close(monpipe_arr[1]);
}

/**
 * Check if receiver is clogged and use decision to temporarily
 * stop sending packets.
 * @param nextseqtosend integer indicating the Next Sequence Number To Be Sent
 * @param lastunackedseq integer indicating the oldest un-acked sequence number
 * @return integer indicating whether buffer is clogged
 * */
int is_buffer_clogged(int nextseqtosend, int lastunackedseq) {
  int recclog = 0;
  if ((RECLTH << 2) < (nextseqtosend - lastunackedseq - 1)) {
    recclog = 1;
  }
  return recclog;
}
