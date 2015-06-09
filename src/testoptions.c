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
#include "jsonutils.h"


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
#if USE_WEB100
  web100_group* group;
  web100_var* var;
  char tmpstr[256];
#elif USE_WEB10G
  struct estats_val value;
#endif

#if USE_WEB100
  web100_agent_find_var_and_group(agent, "CurCwnd", &group, &var);
  web100_snap_read(var, snap, tmpstr);
  CurCwnd = atoi(web100_value_to_text(web100_get_var_type(var), tmpstr));
#elif USE_WEB10G
  web10g_find_val(snap, "CurCwnd", &value);
  CurCwnd = value.uv32;
#endif

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
#if USE_WEB100
    web100_snap(snapArgs->snap);
    if (peaks) {
      findCwndPeaks(agent, peaks, snapArgs->snap);
    }
    if (writeSnap) {
      web100_log_write(snapArgs->log, snapArgs->snap);
    }
#elif USE_WEB10G
    estats_read_vars(snapArgs->snap, snapArgs->conn, agent);
    if (peaks) {
      findCwndPeaks(agent, peaks, snapArgs->snap);
    }
    if (writeSnap) {
      estats_record_write_data(snapArgs->log, snapArgs->snap);
    }
#endif
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
  unsigned char msgValue[CS_VERSION_LENGTH_MAX + 1] = {'\0', };
  unsigned char useropt = 0;
  int msgType;
  int msgLen = CS_VERSION_LENGTH_MAX + 1;
  int first = 1;
  char *invalid_test_suite = "Invalid test suite request.";
  char *client_timeout = "Client timeout.";
  char *invalid_test = "Invalid test request.";
  char *invalid_login_msg = "Invalid login message.";
  char* jsonMsgValue;

  // char remhostarr[256], protologlocalarr[256];
  // char *remhost_ptr = get_remotehost();

  assert(ctlsockfd != -1);
  assert(options);

  memset(options->client_version, 0, sizeof(options->client_version));

  // read the test suite request
  if (recv_msg(ctlsockfd, &msgType, msgValue, &msgLen)) {
    send_msg(ctlsockfd, MSG_ERROR, invalid_test_suite, 
      strlen(invalid_test_suite));
    return (-1);
  }
  if (msgLen == -1) {
    snprintf(buff, buff_strlen, "Client timeout");
    return (-4);
  }

  /*
   * Expecting a MSG_LOGIN or MSG_EXTENDED_LOGIN with 
   * payload byte indicating tests to be run and potentially
   * a US-ASCII string indicating the version number.
   * Three cases:
   * 1: MSG_LOGIN: Check that msgLen is 1
   * 2: MSG_EXTENDED_LOGIN: Check that msgLen is >= 1 and
   *    <= the maximum length of the client/server version 
   *    string (plus 1 to account for the initial useropt
   *    and then copy the client version into client_version.
   * 3: Neither
   * 
   * In case (1) or (2) we copy the 0th byte from the msgValue
   * into useropt so we'll do that in the fallthrough. 
   */
  if (msgType == MSG_LOGIN) { /* Case 1 */
    options->json_support = 0;
    if (msgLen != 1) {
      send_msg(ctlsockfd, MSG_ERROR, invalid_test, strlen(invalid_test));
      return (-2);
    }
  } else if (msgType == MSG_EXTENDED_LOGIN) { /* Case 2 */
    options->json_support = 1;
    jsonMsgValue = json_read_map_value(msgValue, DEFAULT_KEY);
    strlcpy(msgValue, jsonMsgValue, sizeof(msgValue));
    msgLen = strlen(jsonMsgValue);
    free(jsonMsgValue);
    if (msgLen >= 1 && msgLen <= (CS_VERSION_LENGTH_MAX + 1)) {
      memcpy(options->client_version, msgValue + 1, msgLen - 1);
      log_println(0, "Client version: %s-\n", options->client_version);
    } else {
      send_json_message(ctlsockfd, MSG_ERROR, invalid_test, strlen(invalid_test),
                        options->json_support, JSON_SINGLE_VALUE);
      return (-2);
    }
  } else { /* Case 3 */
    send_msg(ctlsockfd, MSG_ERROR,
             invalid_login_msg, 
             strlen(invalid_login_msg));
    return (-2);
  }
  useropt = msgValue[0];

  // client connect received correctly. Logging activity
  // log that client connected, and create log file
  log_println(0,
              "Client connect received from :IP %s to some server on socket %d",
              get_remotehost(), ctlsockfd);

  // set_protologfile(get_remotehost(), protologlocalarr);

  if (!(useropt
        & (TEST_MID | TEST_C2S | TEST_S2C | TEST_SFW | TEST_STATUS | TEST_EXT | TEST_META))) {
    // message received does not indicate a valid test!
    send_json_message(ctlsockfd, MSG_ERROR, invalid_test_suite,
                      strlen(invalid_test_suite), options->json_support, JSON_SINGLE_VALUE);
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
 * @param metafilename	value of metafile name
 * @param tcp_stat_connection connection pointer
 * @param tcp_stat_group group web100_group pointer
 */
void start_snap_worker(SnapArgs *snaparg, tcp_stat_agent* agentarg,
                       CwndPeaks* peaks, char snaplogenabled,
                       pthread_t *wrkrthreadidarg, char *metafilename,
                       tcp_stat_connection conn, tcp_stat_group* group) {
  FILE *fplocal;

  WorkerArgs workerArgs;
  workerArgs.snapArgs = snaparg;
  workerArgs.agent = agentarg;
  workerArgs.peaks = peaks;
  workerArgs.writeSnap = snaplogenabled;

#if USE_WEB100
  group = web100_group_find(agentarg, "read");
  snaparg->snap = web100_snapshot_alloc(group, conn);
#elif USE_WEB10G
  snaparg->conn = conn;
  estats_val_data_new(&snaparg->snap);
#endif

  if (snaplogenabled) {
    // memcpy(metafilevariablename, metafilename, strlen(metafilename));
    // The above could have been here, except for a caveat: metafile stores
    // just the file name, but full filename is needed to open the log file

    fplocal = fopen(get_logfile(), "a");

#if USE_WEB100
    snaparg->log = web100_log_open_write(metafilename, conn, group);
#elif USE_WEB10G
    estats_record_open(&snaparg->log, metafilename, "w");
#endif
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
#if USE_WEB100
  web100_snap(snaparg->snap);
  if (snaplogenabled) {
    web100_log_write(snaparg->log, snaparg->snap);
  }
#elif USE_WEB10G
  estats_read_vars(snaparg->snap, conn, agentarg);
  if (snaplogenabled) {
    estats_record_write_data(snaparg->log, snaparg->snap);
  }
#endif
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
#if USE_WEB100
  if (snaplogenabled) {
    web100_log_close_write(snapArgs_ptr->log);
  }
  web100_snapshot_free(snapArgs_ptr->snap);
#elif USE_WEB10G
  if (snaplogenabled) {
    estats_record_close(&snapArgs_ptr->log);
  }
  estats_val_data_free(&snapArgs_ptr->snap);
#endif
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
 * Set Cwnd limit
 * @param connarg tcp_stat_connection pointer
 * @param group_arg tcp_stat group pointer
 * @param agentarg tcp_stat agent pointer
 * */
void setCwndlimit(tcp_stat_connection connarg, tcp_stat_group* grouparg,
                  tcp_stat_agent* agentarg, Options* optionsarg) {
#if USE_WEB100
  web100_var *LimRwin, *yar;
#elif USE_WEB10G
  struct estats_val yar;
#endif

  u_int32_t limrwin_val;

  if (optionsarg->limit > 0) {
    log_print(1, "Setting Cwnd limit - ");

#if USE_WEB100
    if (connarg != NULL) {
      log_println(1,
                  "Got web100 connection pointer for recvsfd socket\n");
      char yuff[32];
      web100_agent_find_var_and_group(agentarg, "CurMSS", &grouparg,
                                      &yar);
      web100_raw_read(yar, connarg, yuff);
      log_println(1, "MSS = %s, multiplication factor = %d",
                  web100_value_to_text(web100_get_var_type(yar), yuff),
                  optionsarg->limit);
      limrwin_val = optionsarg->limit
          * (atoi(
              web100_value_to_text(web100_get_var_type(yar),
                                   yuff)));
      web100_agent_find_var_and_group(agentarg, "LimRwin", &grouparg,
                                      &LimRwin);
      log_print(1, "now write %d to limit the Receive window",
                limrwin_val);
      web100_raw_write(LimRwin, connarg, &limrwin_val);
#elif USE_WEB10G
    if (connarg != -1) {
      log_println(1,
                  "Got web10g connection for recvsfd socket\n");
      web10g_get_val(agentarg, connarg, "CurMSS", &yar);
      log_println(1, "MSS = %s, multiplication factor = %d",
                  yar.uv32, optionsarg->limit);
      limrwin_val = optionsarg->limit * yar.uv32;
      log_print(1, "now write %d to limit the Receive window", limrwin_val);
      estats_write_var("LimRwin", limrwin_val, connarg, agentarg);
#endif
      log_println(1, "  ---  Done");
    }
  }
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
