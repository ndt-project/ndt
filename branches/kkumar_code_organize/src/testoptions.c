/*
 * This file contains the functions needed to handle various tests.
 *
 * Jakub Sï¿½awiï¿½ski 2006-06-24
 * jeremian@poczta.fm
 */

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#include "testoptions.h"
#include "network.h"
#include "mrange.h"
#include "logging.h"
#include "utils.h"
#include "protocol.h"
#include "I2util/util.h"
#include "runningtest.h"

int mon_pipe1[2];
int mon_pipe2[2]; // used to store PIDs of pipes created for snap data in S2c test
//static int currentTest = TEST_NONE;

typedef struct snapArgs {
  web100_snapshot* snap;
  web100_log* log;
  int delay;
} SnapArgs;

typedef struct workerArgs {
    SnapArgs* snapArgs;
    web100_agent* agent;
    CwndPeaks* peaks;
    int writeSnap;
} WorkerArgs;

static int workerLoop = 0;
static pthread_mutex_t mainmutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t maincond = PTHREAD_COND_INITIALIZER;
static int slowStart = 1;
static int prevCWNDval = -1;
static int decreasing = 0;


/**
 * Count the CWND peaks from a snapshot and record the minimal and maximum one.
 * @param agent Web100 agent used to track the connection
 * @param peaks Structure containing CWND peaks information
 * @param snap Web100 snapshot structure
 */

void
findCwndPeaks(web100_agent* agent, CwndPeaks* peaks, web100_snapshot* snap)
{
  web100_group* group;
  web100_var* var;
  int CurCwnd;
  char tmpstr[256];

  web100_agent_find_var_and_group(agent, "CurCwnd", &group, &var);
  web100_snap_read(var, snap, tmpstr);
  CurCwnd = atoi(web100_value_to_text(web100_get_var_type(var), tmpstr));

  if (slowStart) {
      if (CurCwnd < prevCWNDval) {
          slowStart = 0;
          peaks->max = prevCWNDval;
          peaks->amount = 1;
          decreasing = 1;
      }
  }
  else {
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
      }
      else if (CurCwnd > prevCWNDval) { // current congestion window size > previous value,
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

void
catch_s2c_alrm(int signo)
{
  if (signo == SIGALRM) {
    log_println(1, "SIGALRM was caught");
    return;
  }
  log_println(0, "Unknown (%d) signal was caught", signo);
}

/**
 * Write the snap logs with fixed time intervals in a separate
 *              thread, locking and releasing resources as necessary
 * @param arg pointer to the snapshot structure
 * @return void pointer null
 */

void*
snapWorker(void* arg)
{
    WorkerArgs *workerArgs = (WorkerArgs*) arg;
    SnapArgs *snapArgs = workerArgs->snapArgs;
    web100_agent* agent = workerArgs->agent;
    CwndPeaks* peaks = workerArgs->peaks;
    int writeSnap = workerArgs->writeSnap;

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

    while (1) {
        pthread_mutex_lock(&mainmutex);
        if (!workerLoop) {
            pthread_mutex_unlock(&mainmutex);
            break;
        }
        web100_snap(snapArgs->snap);
        if (peaks) {
            findCwndPeaks(agent, peaks, snapArgs->snap);
        }
        if (writeSnap) {
            web100_log_write(snapArgs->log, snapArgs->snap);
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
void
add_test_to_suite(int* first, char * buff, int test_id)
{
  char tmpbuff[16];
  if (*first) {
    *first = 0;
    sprintf(buff, "%d", test_id);
  }
  else {
    memset(tmpbuff, 0, 16);
    sprintf(tmpbuff, " %d", test_id);
    strcat(buff, tmpbuff);
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

int
initialize_tests(int ctlsockfd, TestOptions* options, char * buff)
{
  unsigned char useropt=0;
  int msgType;
  int msgLen = 1;
  int first = 1;
  char remhostarr[256], protologlocalarr[256];
  char *remhost_ptr = get_remotehost();


  assert(ctlsockfd != -1);
  assert(options);

  // read the test suite request
  if (recv_msg(ctlsockfd, &msgType, &useropt, &msgLen)) {
      sprintf(buff, "Invalid test suite request");
      send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
      return (-1);
  }
  if (msgLen == -1) {
      sprintf(buff, "Client timeout");
      return (-4);
  }
  // Expecting a MSG_LOGIN with payload byte indicating tests to be run
  if ((msgType != MSG_LOGIN) || (msgLen != 1)) {
      sprintf(buff, "Invalid test request");
      send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
      return (-2);
  }
  // client connect received correctly. Logging activity
  // log that client connected, and create log file
  log_println(0,
		  "Client connect received from :IP %s to some server on socket %d", get_remotehost(), ctlsockfd);

  set_protologfile(get_remotehost(), protologlocalarr);


  if (!(useropt & (TEST_MID | TEST_C2S | TEST_S2C | TEST_SFW | TEST_STATUS | TEST_META))) {
	  	  	  // message received does not indicate a valid test!
      sprintf(buff, "Invalid test suite request");
      send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
      return (-3);
  }
  // construct test suite request based on user options received
  if (useropt & TEST_MID) {
    add_test_to_suite(&first, buff, TEST_MID);
  }
  if (useropt & TEST_SFW) {
    add_test_to_suite(&first, buff, TEST_SFW);
  }
  if (useropt & TEST_C2S) {
    add_test_to_suite(&first, buff, TEST_C2S);
  }
  if (useropt & TEST_S2C) {
    add_test_to_suite(&first, buff, TEST_S2C);
  }
  if (useropt & TEST_META) {
    add_test_to_suite(&first, buff, TEST_META);
  }
  return useropt;
}

/**
 * Perform the Middlebox test.
 * @param ctlsockfd Client control socket descriptor
 * @param agent Web100 agent used to track the connection
 * @param options  Test options
 * @param s2c2spd In-out parameter for S2C throughput results (evaluated by the MID TEST),
 * @param conn_options Connection options
 * Returns: 0 - success,
 *          >0 - error code.
 *          Error codes:
 * 				-1 - Listener socket creation failed
 *				-3-web100 connection data not obtained
 *				-100 -timeout while waiting for client to connect to serverÕs ephemeral port
 *				-errono- Other specific socket error numbers
 *				-101 - Retries exceeded while waiting for client to connect
 *				-102 - Retries exceeded while waiting for data from connected client
 *				1 - Message reception errors/inconsistencies
 *				2 - Unexpected message type received/no message received due to timeout
 *				3 Ð Received message is invalid
 *
 */

int
test_mid(int ctlsockfd, web100_agent* agent, TestOptions* options, int conn_options, double* s2c2spd)
{
  int maxseg=1456;
  /* int maxseg=1456, largewin=16*1024*1024; */
  /* int seg_size, win_size; */
  int midsfd; // socket file-descriptor, used in throuput test from S->C
  int j, ret;
  struct sockaddr_storage cli_addr;
  /* socklen_t optlen, clilen; */
  socklen_t clilen;
  char buff[BUFFSIZE+1];
  I2Addr midsrv_addr = NULL;
  char listenmidport[10];
  int msgType;
  int msgLen;
  web100_connection* conn;
  char tmpstr[256];
  struct timeval sel_tv;
  fd_set rfd;

  // variables used for protocol validation logging
  enum TEST_ID thistestId = NONE;
  enum TEST_STATUS_INT teststatusnow = TEST_NOT_STARTED;
  enum PROCESS_STATUS_INT procstatusenum = PROCESS_STARTED;
  enum PROCESS_TYPE_INT proctypeenum = CONNECT_TYPE;

  assert(ctlsockfd != -1);
  assert(agent);
  assert(options);
  assert(s2c2spd);
  
  if (options->midopt) { // ready to run middlebox tests
    setCurrentTest(TEST_MID);
    log_println(1, " <-- %d - Middlebox test -->", options->child0);

    //protocol validation logs
    printf(" <--- %d - Middlebox test --->", options->child0);
    thistestId = MIDDLEBOX;
    teststatusnow = TEST_STARTED;
    //protolog_status(1, options->child0, thistestId, teststatusnow);
    protolog_status(options->child0, thistestId, teststatusnow);

    // determine port to be used. Compute based on options set earlier
    // by reading from config file, or use default port3 (3003),
    strcpy(listenmidport, PORT3);

    if (options->midsockport) {
      sprintf(listenmidport, "%d", options->midsockport);
    }
    else if (options->mainport) {
      sprintf(listenmidport, "%d", options->mainport + 2);
    }
    
    if (options->multiple) {
      strcpy(listenmidport, "0");
    }
    
/*  RAC debug  */
/*
    if (KillHung() == 0)
	log_println(5, "KillHung() returned 0, should have tried to kill off some LastAck process");
    else
	log_println(5, "KillHung(): returned non-0 response, nothing to kill or kill failed");
*/

    while (midsrv_addr == NULL) {

    	// attempt to bind to a new port and obtain address structure with details of listening port

        midsrv_addr = CreateListenSocket(NULL,
                (options->multiple ? mrange_next(listenmidport) : listenmidport), conn_options, 0);
        if (midsrv_addr == NULL) {
	  /*
            log_println(5, " Calling KillHung() because midsrv_address failed to bind");
            if (KillHung() == 0)
                continue;
	   */
        }
        if (strcmp(listenmidport, "0") == 0) {
            log_println(0, "WARNING: ephemeral port number was bound");
            break;
        }
        if (options->multiple == 0) { // todo
            break;
        }
    }
    if (midsrv_addr == NULL) {
      log_println(0, "Server (Middlebox test): CreateListenSocket failed: %s", strerror(errno));
      sprintf(buff, "Server (Middlebox test): CreateListenSocket failed: %s", strerror(errno));
      send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
      return -1;
    }

    // get socket FD and the ephemeral port number that client will connect to
    options->midsockfd = I2AddrFD(midsrv_addr);
    options->midsockport = I2AddrPort(midsrv_addr);
    log_println(1, "  -- port: %d", options->midsockport);
    
    // send this port number to client
    sprintf(buff, "%d", options->midsockport);
    if ((ret = send_msg(ctlsockfd, TEST_PREPARE, buff, strlen(buff))) < 0)
	return ret;
    
    /* set mss to 1456 (strange value), and large snd/rcv buffers
     * should check to see if server supports window scale ?
     */
    setsockopt(options->midsockfd, SOL_TCP, TCP_MAXSEG, &maxseg, sizeof(maxseg));
    /* setsockopt(options->midsockfd, SOL_SOCKET, SO_SNDBUF, &largewin, sizeof(largewin));
    setsockopt(options->midsockfd, SOL_SOCKET, SO_RCVBUF, &largewin, sizeof(largewin));
    log_println(2, "Middlebox test, Port %d waiting for incoming connection (fd=%d)",
        options->midsockport, options->midsockfd);
    if (get_debuglvl() > 1) {
      optlen = sizeof(seg_size);
      getsockopt(options->midsockfd, SOL_TCP, TCP_MAXSEG, &seg_size, &optlen);
      getsockopt(options->midsockfd, SOL_SOCKET, SO_RCVBUF, &win_size, &optlen);
      log_println(2, "Set MSS to %d, Receiving Window size set to %dKB", seg_size, win_size);
      getsockopt(options->midsockfd, SOL_SOCKET, SO_SNDBUF, &win_size, &optlen);
      log_println(2, "Sending Window size set to %dKB", win_size);
    }
*/

    /* Post a listen on port 3003.  Client will connect here to run the 
     * middlebox test.  At this time it really only checks the MSS value
     * and does NAT detection.  More analysis functions (window scale)
     * will be done in the future.
     */
    clilen = sizeof(cli_addr);

    // Wait on listening socket and read data once ready.
    // Choose a retry count of "5" to wait for activity on the socket
    FD_ZERO(&rfd);
    FD_SET(options->midsockfd, &rfd);
    sel_tv.tv_sec = 5;
    sel_tv.tv_usec = 0;
    for (j=0; j<5; j++) { // TODO 5 constant declaration
      ret = select((options->midsockfd)+1, &rfd, NULL, NULL, &sel_tv);
      if ((ret == -1) && (errno == EINTR)) // socket interruption. continue waiting for activity on socket
	continue;
      if (ret == 0)  // timeout
	return -100;
      if (ret < 0) // other socket errors, exit
	return -errno; 
      if (j == 4) // retry exceeded. Quit
	return -101;
midfd:

	  // if a valid connection request is received, client has connected. Proceed.
	  // Note the new socket fd used in the throughput test is this (midsfd)
      if ((midsfd = accept(options->midsockfd, (struct sockaddr *) &cli_addr, &clilen)) > 0) {
    	  // log protocol validation indicating client connection
    	  procstatusenum = PROCESS_STARTED;
    	  proctypeenum = CONNECT_TYPE;
    	  protolog_procstatus(options->child0, thistestId, proctypeenum, procstatusenum);
	break;
      }

      if ((midsfd == -1) && (errno == EINTR)) // socket interrupted, wait some more
	goto midfd;

      sprintf(tmpstr, "-------     middlebox connection setup returned because (%d)", errno);
      if (get_debuglvl() > 1)
        perror(tmpstr);
      if (midsfd < 0)
	return -errno;
      if (j == 4)
	return -102;
    } 

    // get meta test details copied into results
    memcpy(&meta.c_addr, &cli_addr, clilen);
    /* meta.c_addr = cli_addr; */
    meta.family = ((struct sockaddr *) &cli_addr)->sa_family;

    buff[0] = '\0';
    // get web100 connection data
    if ((conn = web100_connection_from_socket(agent, midsfd)) == NULL) {
        log_println(0, "!!!!!!!!!!!  test_mid() failed to get web100 connection data, rc=%d", errno);
        /* exit(-1); */
        return -3;
    }

    // Perform S->C throughput test. Obtained results in "buff"
    web100_middlebox(midsfd, agent, conn, buff);

    // Transmit results in the form of a TEST_MSG message
    send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));

    // Expect client to send throughput as calculated at its end

    msgLen = sizeof(buff);
    if (recv_msg(ctlsockfd, &msgType, buff, &msgLen)) {  // message reception error
      log_println(0, "Protocol error!");
      sprintf(buff, "Server (Middlebox test): Invalid CWND limited throughput received");
      send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
      return 1;
    }
    if (check_msg_type("Middlebox test", TEST_MSG, msgType, buff, msgLen)) { // only TEST_MSG type valid
      sprintf(buff, "Server (Middlebox test): Invalid CWND limited throughput received");
      send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
      return 2;
    }
    if (msgLen <= 0) { // received message's length has to be a valid one
      log_println(0, "Improper message");
      sprintf(buff, "Server (Middlebox test): Invalid CWND limited throughput received");
      send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
      return 3;
    }

    // message payload from client == midbox S->c throughput
    buff[msgLen] = 0;
    *s2c2spd = atof(buff); //todo s2c2spd could be named better
    log_println(4, "CWND limited throughput = %0.0f kbps (%s)", *s2c2spd, buff);

    // finalize the midbox test ; disabling socket used for throughput test and closing out both sockets
    shutdown(midsfd, SHUT_WR);
    close(midsfd);
    close(options->midsockfd);
    send_msg(ctlsockfd, TEST_FINALIZE, "", 0);
    log_println(1, " <--------- %d ----------->", options->child0);

    printf(" <--- %d - Middlebox test --->", options->child0);

    // log end of test into protocol doc, just to delimit.
    teststatusnow = TEST_ENDED;
    //protolog_status(1, options->child0, thistestId, teststatusnow);
    protolog_status( options->child0, thistestId, teststatusnow);

    setCurrentTest(TEST_NONE);
  /* I2AddrFree(midsrv_addr); */
  }
  /* I2AddrFree(midsrv_addr); */
  return 0;
}

/**
 * Perform the C2S Throughput test.
 * @param ctlsockfd Client control socket descriptor
 * @param agent Web100 agent used to track the connection
 * @param testOptions Test options
 * @param conn_options Connection options
 * @return 0 - success,
 *          >0 - error code
 *          Error codes:
 *          -1 - Listener socket creation failed -100 -timeout
 *              while waitin for client to connect to serverÕs ephemeral port
 *			-100 - timeout while waitinf for client to connect to serverÕs ephemeral port
 * 			-errno - Other specific socket error numbers
 *			-101 - Retries exceeded while waiting for client to connect
 *			-102 - Retries exceeded while waiting for data from connected client
 *
 */

int
test_c2s(int ctlsockfd, web100_agent* agent, TestOptions* testOptions, int conn_options, double* c2sspd,
    int set_buff, int window, int autotune, char* device, Options* options,
    int record_reverse, int count_vars, char spds[4][256], int* spd_index)
{
  /* int largewin=16*1024*1024; */
  int recvsfd;
  pid_t mon_pid1 = 0;
  int ret, n, i, j;
  /* int seg_size, win_size; */
  struct sockaddr_storage cli_addr;
  /* socklen_t optlen, clilen; */
  socklen_t clilen;
  char tmpstr[256];
  double t, bytes=0;
  struct timeval sel_tv;
  fd_set rfd;
  char buff[BUFFSIZE+1];
  PortPair pair;
  I2Addr c2ssrv_addr=NULL, src_addr=NULL;
  char listenc2sport[10];
  pthread_t workerThreadId;
  char namebuf[256], dir[128];
  size_t nameBufLen = 255; // TODO constant?
  char isoTime[64];
  DIR *dp;
  FILE *fp;

  web100_group* group;
  web100_connection* conn;

  SnapArgs snapArgs;
  snapArgs.snap = NULL;
  snapArgs.log = NULL;
  snapArgs.delay = options->snapDelay;
  wait_sig = 0;

  // Test ID and status descriptors
  enum TEST_ID testids = C2S;
  enum TEST_STATUS_INT teststatuses = TEST_NOT_STARTED;
  enum PROCESS_STATUS_INT procstatusenum = UNKNOWN;
  enum PROCESS_TYPE_INT proctypeenum = CONNECT_TYPE;


  if (testOptions->c2sopt) {
    setCurrentTest(TEST_C2S);
    log_println(1, " <-- %d - C2S throughput test -->", testOptions->child0);
    strcpy(listenc2sport, PORT2);
    
    //log protocol validation logs
    teststatuses = TEST_STARTED;
    //protolog_status(0,testOptions->child0, testids, teststatuses);
    protolog_status(testOptions->child0, testids, teststatuses);

    // Determine port to be used. Compute based on options set earlier
    // by reading from config file, or use default port2 (3002).
    if (testOptions->c2ssockport) {
      sprintf(listenc2sport, "%d", testOptions->c2ssockport);
    }
    else if (testOptions->mainport) {
      sprintf(listenc2sport, "%d", testOptions->mainport + 1);
    }
    
    if (testOptions->multiple) {
      strcpy(listenc2sport, "0");
    }
    
    // attempt to bind to a new port and obtain address structure with details of listening port
    while (c2ssrv_addr == NULL) {
      c2ssrv_addr = CreateListenSocket(NULL,
          (testOptions->multiple ? mrange_next(listenc2sport) : listenc2sport), conn_options, 0);
      if (strcmp(listenc2sport, "0") == 0) {
        log_println(0, "WARNING: ephemeral port number was bound");
        break;
      }
      if (testOptions->multiple == 0) {
        break;
      }
    }
    if (c2ssrv_addr == NULL) {
      log_println(0, "Server (C2S throughput test): CreateListenSocket failed: %s", strerror(errno));
      sprintf(buff, "Server (C2S throughput test): CreateListenSocket failed: %s", strerror(errno));
      send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
      return -1;
    }

    // get socket FD and the ephemeral port number that client will connect to run tests
    testOptions->c2ssockfd = I2AddrFD(c2ssrv_addr);
    testOptions->c2ssockport = I2AddrPort(c2ssrv_addr);
    log_println(1, "  -- port: %d", testOptions->c2ssockport);
    pair.port1 = testOptions->c2ssockport;
    pair.port2 = -1;

/*
    if (set_buff > 0) {
      setsockopt(testOptions->c2ssockfd, SOL_SOCKET, SO_SNDBUF, &window, sizeof(window));
      setsockopt(testOptions->c2ssockfd, SOL_SOCKET, SO_RCVBUF, &window, sizeof(window));
      if (get_debuglvl() > 1) {
        optlen = sizeof(seg_size);
        getsockopt(testOptions->c2ssockfd, SOL_TCP, TCP_MAXSEG, &seg_size, &optlen);
        getsockopt(testOptions->c2ssockfd, SOL_SOCKET, SO_RCVBUF, &win_size, &optlen);
        log_println(2, "Set MSS to %d, Receiving Window size set to %dKB", seg_size, win_size);
        getsockopt(testOptions->c2ssockfd, SOL_SOCKET, SO_SNDBUF, &win_size, &optlen);
        log_println(2, "Sending Window size set to %dKB", win_size);
      }
    }
*/
    /* if (autotune > 0) {
     *  setsockopt(testOptions->c2ssockfd, SOL_SOCKET, SO_SNDBUF, &largewin, sizeof(largewin));
     *  setsockopt(testOptions->c2ssockfd, SOL_SOCKET, SO_RCVBUF, &largewin, sizeof(largewin));
     * }
     */
    log_println(1, "listening for Inet connection on testOptions->c2ssockfd, fd=%d", testOptions->c2ssockfd);

    log_println(1, "Sending 'GO' signal, to tell client %d to head for the next test", testOptions->child0);
    sprintf(buff, "%d", testOptions->c2ssockport);

    // send TEST_PREPARE message with ephemeral port detail, indicating start of tests
    if ((ret = send_msg(ctlsockfd, TEST_PREPARE, buff, strlen(buff))) < 0)
	return ret;

    // Wait on listening socket and read data once ready.
    // Retry 5 times,  waiting for activity on the socket
    clilen = sizeof(cli_addr);
    log_println(6, "child %d - sent c2s prepare to client", testOptions->child0);
    FD_ZERO(&rfd);
    FD_SET(testOptions->c2ssockfd, &rfd);
    sel_tv.tv_sec = 5;
    sel_tv.tv_usec = 0;
    for (j=0; j<5; j++) { //todo 5 constant decl
      ret = select((testOptions->c2ssockfd)+1, &rfd, NULL, NULL, &sel_tv);
      if ((ret == -1) && (errno == EINTR)) // socket interrupted. continue waiting for activity on socket
    	  continue;
      if (ret == 0)   	// timeout
    	  return -100;
      if (ret < 0) 		// other socket errors. exit
    	  return -errno;
      if (j == 4) 		// retry exceeded. exit
    	  return -101;
recfd:

	  // If a valid connection request is received, client has connected. Proceed.
      // Note the new socket fd - recvsfd- used in the throughput test
      recvsfd = accept(testOptions->c2ssockfd, (struct sockaddr *) &cli_addr, &clilen);
      if (recvsfd > 0) {
	log_println(6, "accept() for %d completed", testOptions->child0);
		// log protocol validation indicating client accept
		procstatusenum = PROCESS_STARTED;
	    proctypeenum = CONNECT_TYPE;
	    protolog_procstatus(testOptions->child0,testids, proctypeenum, procstatusenum);
	break;
      }
      if ((recvsfd == -1) && (errno == EINTR)) { // socket interrupted, wait some more
	log_println(6, "Child %d interrupted while waiting for accept() to complete", 
		testOptions->child0);
	goto recfd;
      }
      log_println(6, "-------     C2S connection setup for %d returned because (%d)", 
		testOptions->child0, errno);
      if (recvsfd < 0) // other socket errors, 	quit
	return -errno; 
      if (j == 4) {    // retry exceeded, quit
	log_println(6, "c2s child %d, uable to open connection, return from test", testOptions->child0);
        return -102;
      }
    } 

    // Get address associated with the throughput test. Used for packet tracing
    log_println(6, "child %d - c2s ready for test with fd=%d", testOptions->child0, recvsfd);
    src_addr = I2AddrByLocalSockFD(get_errhandle(), recvsfd, 0);

    // Get web100 connection. Used to collect web100 variable statistics
    conn = web100_connection_from_socket(agent, recvsfd);

    // set up packet tracing. Collected data is used for bottleneck link calculations
    if (getuid() == 0) {
      pipe(mon_pipe1);
      if ((mon_pid1 = fork()) == 0) {
        /* close(ctlsockfd); */
        close(testOptions->c2ssockfd);
        close(recvsfd);
        log_println(5, "C2S test Child %d thinks pipe() returned fd0=%d, fd1=%d", 
		testOptions->child0,mon_pipe1[0], mon_pipe1[1]);
        /* log_println(2, "C2S test calling init_pkttrace() with pd=0x%x", (int) &cli_addr); */
        init_pkttrace(src_addr, (struct sockaddr *) &cli_addr, clilen, mon_pipe1,
		device, &pair, "c2s", options->compress);
        exit(0);    /* Packet trace finished, terminate gracefully */
      }

      // Get data collected from packet tracing into the C2S "ndttrace" file
      memset(tmpstr, 0, 256);
      for (i=0; i< 5; i++) {
          ret = read(mon_pipe1[0], tmpstr, 128);
	  if ((ret == -1) && (errno == EINTR))
	    continue;
	  break;
      }
      if (strlen(tmpstr) > 5)
        memcpy(meta.c2s_ndttrace, tmpstr, strlen(tmpstr));
      	  	  //name of nettrace file passed back from pcap child
    }

    log_println(5, "C2S test Parent thinks pipe() returned fd0=%d, fd1=%d", mon_pipe1[0], mon_pipe1[1]);

    /* Check, and if needed, set the web100 autotuning function on.  This improves
     * performance without requiring the entire system be re-configured.  Returned
     * value is true if set is done, so admin knows if system default should be changed
     *
     * 11/22/04, the sBufMode and rBufMode per connection autotuning variables are being
     * depricated, so this is another attempt at making this work.  If window scaling is
     * enabled, then scale the buffer size to the correct value.  This will also help
     * when reporting the buffer / RTT limit.
     */

    /* if (autotune > 0) 
     *  web100_setbuff(recvsfd, agent, conn, autotune);
     */

    /* ok, We are now read to start the throughput tests.  First
     * the client will stream data to the server for 10 seconds.
     * Data will be processed by the read loop below.
     */

    /* experimental code, delete when finished */
    {
        web100_var *LimRwin, *yar;
        u_int32_t limrwin_val;
        char yuff[32];

        if (options->limit > 0) {
            log_print(1, "Setting Cwnd limit - ");
            if (conn != NULL) {
                log_println(1, "Got web100 connection pointer for recvsfd socket\n");
                web100_agent_find_var_and_group(agent, "CurMSS", &group, &yar);
                web100_raw_read(yar, conn, yuff);
                log_println(1, "MSS = %s, multiplication factor = %d",
                        web100_value_to_text(web100_get_var_type(yar), yuff), options->limit);
                limrwin_val = options->limit * (atoi(web100_value_to_text(web100_get_var_type(yar), yuff)));
                web100_agent_find_var_and_group(agent, "LimRwin", &group, &LimRwin);
                log_print(1, "now write %d to limit the Receive window", limrwin_val);
                web100_raw_write(LimRwin, conn, &limrwin_val);
                log_println(1, "  ---  Done");
            }
        }
    }
    /* End of test code */

    // Create C->S log directories, and open the file for writing, if snaplog option is enabled
    {
        I2Addr sockAddr = I2AddrBySAddr(get_errhandle(), (struct sockaddr *) &cli_addr, clilen, 0, 0);
        memset(namebuf, 0, 256);
        I2AddrNodeName(sockAddr, namebuf, &nameBufLen);
	strncpy(options->c2s_logname, DataDirName, strlen(DataDirName));
    	if ((dp = opendir(options->c2s_logname)) == NULL && errno == ENOENT)
		mkdir(options->c2s_logname, 0755);
	closedir(dp);
    	get_YYYY(dir);
    	strncat(options->c2s_logname, dir, 4); 
    	if ((dp = opendir(options->c2s_logname)) == NULL && errno == ENOENT)
		mkdir(options->c2s_logname, 0755);
	closedir(dp);
    	strncat(options->c2s_logname, "/", 1);
    	get_MM(dir);
    	strncat(options->c2s_logname, dir, 2); 
    	if ((dp = opendir(options->c2s_logname)) == NULL && errno == ENOENT)
		mkdir(options->c2s_logname, 0755);
	closedir(dp);
    	strncat(options->c2s_logname, "/", 1);
    	get_DD(dir);
    	strncat(options->c2s_logname, dir, 2); 
    	if ((dp = opendir(options->c2s_logname)) == NULL && errno == ENOENT)
		mkdir(options->c2s_logname, 0755);
	closedir(dp);
    	strncat(options->c2s_logname, "/", 1);
    	sprintf(dir, "%s_%s:%d.c2s_snaplog", get_ISOtime(isoTime), namebuf, I2AddrPort(sockAddr));
	strncat(options->c2s_logname, dir, strlen(dir));
        group = web100_group_find(agent, "read");
        snapArgs.snap = web100_snapshot_alloc(group, conn);
  	I2AddrFree(sockAddr);
        if (options->snaplog) {
	    memcpy(meta.c2s_snaplog, dir, strlen(dir));
            fp = fopen(get_logfile(),"a");
            snapArgs.log = web100_log_open_write(options->c2s_logname, conn, group);
            if (fp == NULL) {
                log_println(0, "Unable to open log file '%s', continuing on without logging", get_logfile());
            }
            else {
                log_println(1, "c2s_snaplog file: %s\n", options->c2s_logname);
                fprintf(fp, "c2s_snaplog file: %s\n", options->c2s_logname);
                fclose(fp);
            }
        }
    } //end creating snaplog dirs and opening logfile

    sleep(2);

    // send empty TEST_START indicating start of the test
    send_msg(ctlsockfd, TEST_START, "", 0);
    /* alarm(30); */  /* reset alarm() again, this 10 sec test should finish before this signal
                 * is generated.  */

    // write into snaplog file, based on options. Lock/release web10 log file as necessary
    {
        WorkerArgs workerArgs;
        workerArgs.snapArgs = &snapArgs;
        workerArgs.agent = agent;
        workerArgs.peaks = NULL;
        workerArgs.writeSnap = options->snaplog;
        if (pthread_create(&workerThreadId, NULL, snapWorker, (void*) &workerArgs)) {
            log_println(0, "Cannot create worker thread for writing snap log!");
            workerThreadId = 0;
        }

        pthread_mutex_lock(&mainmutex);
        workerLoop = 1;
        web100_snap(snapArgs.snap);
        if (options->snaplog) {
            web100_log_write(snapArgs.log, snapArgs.snap);
        }
        pthread_cond_wait(&maincond, &mainmutex);
        pthread_mutex_unlock(&mainmutex);
    }

    // Wait on listening socket and read data once ready.
    t = secs();
    sel_tv.tv_sec = 11; // time out after 11 seconds
    sel_tv.tv_usec = 0;
    FD_ZERO(&rfd);
    FD_SET(recvsfd, &rfd);
    for (;;) {
      ret = select(recvsfd+1, &rfd, NULL, NULL, &sel_tv);
      if ((ret == -1) && (errno == EINTR)) {
    	  // socket interrupted. Continue waiting for activity on socket
    	  continue;
      }
      if (ret > 0) { // read from socket
        n = read(recvsfd, buff, sizeof(buff));
	if ((n == -1) && (errno == EINTR)) // read interrupted,continue waiting
	  continue;
        if (n == 0) // all data has been read
          break;
        bytes += n; // data byte count has to be increased
        continue;
      }
      break;
    }

    t = secs()-t;
    //  throughput in kilo bits per sec =
    // 			(Number of transmitted bytes * 8) / (time duration)*(1000)
    *c2sspd = (8.e-3 * bytes) / t;

    //calculated and assigned the value of the c->s value. hence release resources
    if (workerThreadId) {
        pthread_mutex_lock(&mainmutex);
        workerLoop = 0;
        pthread_mutex_unlock(&mainmutex);
        pthread_join(workerThreadId, NULL);
    }
    // close writing snaplog, if snaplog recording is enabled
    if (options->snaplog) {
        web100_log_close_write(snapArgs.log);
    }

    // send the server calculated value of C->S throughput as result to client
    sprintf(buff, "%6.0f kbps outbound for child %d", *c2sspd, testOptions->child0);
    log_println(1, "%s", buff);
    sprintf(buff, "%0.0f", *c2sspd);
    send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));


    // get receiver side Web100 stats and write them to the log file. close sockets

    if (record_reverse == 1)
      web100_get_data_recv(recvsfd, agent, conn, count_vars);
    /* shutdown(recvsfd, SHUT_RD); */
    close(recvsfd);
    close(testOptions->c2ssockfd);

    // Next, send speed-chk a flag to retrieve the data it collected.
    // Skip this step if speed-chk isn't running.

    if (getuid() == 0) {
      log_println(1, "Signal USR1(%d) sent to child [%d]", SIGUSR1, mon_pid1);
      testOptions->child1 = mon_pid1;
      kill(mon_pid1, SIGUSR1);
      FD_ZERO(&rfd);
      FD_SET(mon_pipe1[0], &rfd);
      sel_tv.tv_sec = 1;
      sel_tv.tv_usec = 100000;
      i = 0;

      for (;;) {
        ret = select(mon_pipe1[0]+1, &rfd, NULL, NULL, &sel_tv); 
        if ((ret == -1) && (errno == EINTR)) 
          continue;
	if (((ret == -1) && (errno != EINTR)) ||(ret == 0)) {
          log_println(4, "Failed to read pkt-pair data from C2S flow, retcode=%d, reason=%d", ret, errno);
          sprintf(spds[(*spd_index)++], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0 -1");
          sprintf(spds[(*spd_index)++], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0 -1");
	  break;
	}
	/* There is something to read, so get it from the pktpair child.  If an interrupt occurs, 
         * just skip the read and go on
         * RAC 2/8/10
         */
	if (ret > 0) {
          if ((ret = read(mon_pipe1[0], spds[*spd_index], 128)) < 0)
            sprintf(spds[*spd_index], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0 -1");
          log_println(1, "%d bytes read '%s' from C2S monitor pipe", ret, spds[*spd_index]);
          (*spd_index)++;
	    if (i++ == 1)
	      break;
	    sel_tv.tv_sec = 1;
	    sel_tv.tv_usec = 100000;
	    continue;
	  }
          /* if ((ret = read(mon_pipe1[0], spds[*spd_index], 128)) < 0)
           *  sprintf(spds[*spd_index], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0 -1");
           * log_println(1, "%d bytes read '%s' from C2S monitor pipe", ret, spds[*spd_index]);
           * (*spd_index)++;
	   * break;
           * } 
           */
      }
    }

    // An empty TEST_FINALIZE message is sent to conclude the test
    send_msg(ctlsockfd, TEST_FINALIZE, "", 0);

    //  Close opened resources for packet capture
    if (getuid() == 0) {
      for (i=0; i<5; i++) {
        ret = write(mon_pipe1[1], "c", 1);
	if (ret == 1)
	  break;
	if ((ret == -1) && (errno == EINTR))
	  continue;
      }
      close(mon_pipe1[0]);
      close(mon_pipe1[1]);
    }

    // log end of C->S test
    log_println(1, " <----------- %d -------------->", testOptions->child0);
    //protocol logs
    teststatuses = TEST_ENDED;
    //protolog_status(0,testOptions->child0, testids, teststatuses);
    protolog_status(testOptions->child0, testids, teststatuses);


    //set current test status and free address
    setCurrentTest(TEST_NONE);
  /* I2AddrFree(c2ssrv_addr); */
  I2AddrFree(src_addr);
  /* testOptions->child1 = mon_pid1; */
  }
  /* I2AddrFree(c2ssrv_addr); */
  /* I2AddrFree(src_addr); */
  /* testOptions->child1 = mon_pid1; */
  return 0; 
}

/**
 * Perform the S2C Throughput test.
 * Arguments: ctlsockfd - the client control socket descriptor
 *            agent - the Web100 agent used to track the connection
 *            testOptions - the test options
 *            conn_options - the connection options
 * Returns: 0 - success,
 *          >0 - error code.
 *          Error codes:
 * 			 	-1	   - Message reception errors/inconsistencies in clientÕs final message, or Listener socket creation failed or cannot write message header information while attempting to send
 *        		 TEST_PREPARE message
 *				-2 	   - Cannot write message data while attempting to send
 *           		 TEST_PREPARE message, or Unexpected message type received
 *           	-3 	   -  Received message is invalid
 *          	-100   - timeout while waiting for client to connect to serverÕs ephemeral port
 *				-101   - Retries exceeded while waiting for client to connect
 *				-102   - Retries exceeded while waiting for data from connected client
 *  			-errno - Other specific socket error numbers
 */

int
test_s2c(int ctlsockfd, web100_agent* agent, TestOptions* testOptions, int conn_options, double* s2cspd,
    int set_buff, int window, int autotune, char* device, Options* options, char spds[4][256],
    int* spd_index, int count_vars, CwndPeaks* peaks)
{
  /* int largewin=16*1024*1024; */
  int ret, j, k, n;
  int xmitsfd;
  pid_t mon_pid2 = 0;
  /* int seg_size, win_size; */
  char tmpstr[256];
  struct sockaddr_storage cli_addr;
  /* socklen_t optlen, clilen; */
  socklen_t  clilen;
  double bytes, s, t;
  double x2cspd;
  struct timeval sel_tv;
  fd_set rfd;
  char buff[BUFFSIZE+1];
  int c3=0, i; //todo what is c3?
  PortPair pair;
  I2Addr s2csrv_addr=NULL, src_addr=NULL;
  char listens2cport[10];
  int msgType;
  int msgLen;
  int sndqueue;
  struct sigaction new, old;
  char namebuf[256], dir[126];
  char isoTime[64];
  size_t nameBufLen = 255;
  DIR *dp;
  FILE *fp;
  
  /* experimental code to capture and log multiple copies of the
   * web100 variables using the web100_snap() & log() functions.
   */
  web100_snapshot* tsnap = NULL;
  web100_snapshot* rsnap = NULL;
  web100_group* group;
  web100_group* tgroup;
  web100_group* rgroup;
  web100_connection* conn;
  web100_var* var;
  pthread_t workerThreadId;
  int SndMax=0, SndUna=0;
  int c1=0, c2=0; // sent data attempt queue, Draining Queue. TODO name appr
  SnapArgs snapArgs;
  snapArgs.snap = NULL;
  snapArgs.log = NULL;
  snapArgs.delay = options->snapDelay;
  wait_sig = 0;
  
  // variables used for protocol validation logs
  enum TEST_STATUS_INT teststatuses = TEST_NOT_STARTED;
  enum TEST_ID testids = S2C;
  enum PROCESS_STATUS_INT procstatusenum = UNKNOWN;
  enum  PROCESS_TYPE_INT proctypeenum = CONNECT_TYPE;

  // Determine port to be used. Compute based on options set earlier
  // by reading from config file, or use default port2 (3003)
  if (testOptions->s2copt) {
    setCurrentTest(TEST_S2C);
    log_println(1, " <-- %d - S2C throughput test -->", testOptions->child0);

    //protocol logs
    teststatuses = TEST_STARTED;
    //protolog_status(0,testOptions->child0, testids, teststatuses);
    protolog_status(testOptions->child0, testids, teststatuses);

    strcpy(listens2cport, PORT4);
    
    if (testOptions->s2csockport) {
      sprintf(listens2cport, "%d", testOptions->s2csockport);
    }
    else if (testOptions->mainport) {
      sprintf(listens2cport, "%d", testOptions->mainport + 2);
    }
    
    if (testOptions->multiple) {
      strcpy(listens2cport, "0");
    }
    
    // attempt to bind to a new port and obtain address structure with details of listening port
    while (s2csrv_addr == NULL) {
      s2csrv_addr = CreateListenSocket(NULL,
          (testOptions->multiple ? mrange_next(listens2cport) : listens2cport), conn_options, 0);
      if (s2csrv_addr == NULL) {
	/*
        log_println(1, " Calling KillHung() because s2csrv_address failed to bind");
	if (KillHung() == 0)
	  continue;
	 */
      }
      if (strcmp(listens2cport, "0") == 0) {
        log_println(0, "WARNING: ephemeral port number was bound");
        break;
      }
      if (testOptions->multiple == 0) {
        break;
      }
    }
    if (s2csrv_addr == NULL) {
      log_println(0, "Server (S2C throughput test): CreateListenSocket failed: %s", strerror(errno));
      sprintf(buff, "Server (S2C throughput test): CreateListenSocket failed: %s", strerror(errno));
      send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
      return -1;
    }

    // get socket FD and the ephemeral port number that client will connect to run tests
    testOptions->s2csockfd = I2AddrFD(s2csrv_addr);
    testOptions->s2csockport = I2AddrPort(s2csrv_addr);
    log_println(1, "  -- s2c %d port: %d", testOptions->child0, testOptions->s2csockport);
    pair.port1 = -1;
    pair.port2 = testOptions->s2csockport;
    
/*
    if (set_buff > 0) {
      setsockopt(testOptions->s2csockfd, SOL_SOCKET, SO_SNDBUF, &window, sizeof(window));
      setsockopt(testOptions->s2csockfd, SOL_SOCKET, SO_RCVBUF, &window, sizeof(window));
      if (get_debuglvl() > 1) {
        optlen = sizeof(seg_size);
        getsockopt(testOptions->s2csockfd, SOL_TCP, TCP_MAXSEG, &seg_size, &optlen);
        getsockopt(testOptions->s2csockfd, SOL_SOCKET, SO_RCVBUF, &win_size, &optlen);
        log_println(2, "Set MSS to %d, Receiving Window size set to %dKB", seg_size, win_size);
        getsockopt(testOptions->s2csockfd, SOL_SOCKET, SO_SNDBUF, &win_size, &optlen);
        log_println(2, "Sending Window size set to %dKB", win_size);
      }
    }
*/
    /* if (autotune > 0) {
     *   setsockopt(testOptions->s2csockfd, SOL_SOCKET, SO_SNDBUF, &largewin, sizeof(largewin));
     *  setsockopt(testOptions->s2csockfd, SOL_SOCKET, SO_RCVBUF, &largewin, sizeof(largewin));
     * }
     */

    // Data received from speed-chk. Send TEST_PREPARE "GO" signal with port number
    sprintf(buff, "%d", testOptions->s2csockport);
    j = send_msg(ctlsockfd, TEST_PREPARE, buff, strlen(buff));
    if (j == -1) {
	log_println(6, "S2C %d Error!, Test start message not sent!", testOptions->child0);
	return j;
    }
    if (j == -2) { // could not write message data
	log_println(6, "S2C %d Error!, server port [%s] not sent!", testOptions->child0, buff);
	return j;
    }
    
    // ok, await for connect on 3rd port
    // This is the second throughput test, with data streaming from
    // the server back to the client.  Again stream data for 10 seconds.

    log_println(1, "%d waiting for data on testOptions->s2csockfd", testOptions->child0);

    clilen = sizeof(cli_addr);
    FD_ZERO(&rfd);
    FD_SET(testOptions->c2ssockfd, &rfd);
    sel_tv.tv_sec = 5;
    sel_tv.tv_usec = 0;
    for (j=0; j<5; j++) {
      ret = select((testOptions->s2csockfd)+1, &rfd, NULL, NULL, &sel_tv);
      if ((ret == -1) && (errno == EINTR))
	continue;
      if (ret == 0) 
	return -100;  // timeout
      if (ret < 0)
	return -errno;  // other socket errors. exit
      if (j == 4)
	return -101;	// retry exceeded. exit

      // If a valid connection request is received, client has connected. Proceed.
      // Note the new socket fd - xmitfd - used in the throughput test
ximfd:
      xmitsfd = accept(testOptions->s2csockfd, (struct sockaddr *) &cli_addr, &clilen);
      if (xmitsfd > 0) {
    	  log_println(6, "accept() for %d completed", testOptions->child0);
    	  //protocol logging
    	  procstatusenum = PROCESS_STARTED;
    	  proctypeenum = CONNECT_TYPE;
    	  protolog_procstatus(testOptions->child0, testids, proctypeenum, procstatusenum);
    	  break;
      }
      if ((xmitsfd == -1) && (errno == EINTR)) { // socket interrupted, wait some more
	log_println(6, "Child %d interrupted while waiting for accept() to complete", 
		testOptions->child0);
	goto ximfd;
      }
      log_println(6, "-------     S2C connection setup for %d returned because (%d)", 
		testOptions->child0, errno);
      if (xmitsfd < 0)	// other socket errors, quit
	return -errno; 
      if (++j == 4) {   // retry exceeded, quit
	log_println(6, "s2c child %d, unable to open connection, return from test", testOptions->child0);
        return -102;
      }
    } 
    src_addr = I2AddrByLocalSockFD(get_errhandle(), xmitsfd, 0);
    conn = web100_connection_from_socket(agent, xmitsfd); 

    // set up packet capture. The data collected is used for bottleneck link calculations
    if (xmitsfd > 0) {
      log_println(6, "S2C child %d, ready to fork()", testOptions->child0);
      if (getuid() == 0) {
        pipe(mon_pipe2);
        if ((mon_pid2 = fork()) == 0) {
          /* close(ctlsockfd); */
          close(testOptions->s2csockfd);
          close(xmitsfd);
          log_println(5, "S2C test Child thinks pipe() returned fd0=%d, fd1=%d", mon_pipe2[0], mon_pipe2[1]);
          /* log_println(2, "S2C test calling init_pkttrace() with pd=0x%x", (int) &cli_addr); */
          init_pkttrace(src_addr, (struct sockaddr *) &cli_addr, clilen, mon_pipe2, device, &pair, "s2c", options->compress);
	  log_println(6, "S2C test ended, why is timer still running?");
          exit(0);    /* Packet trace finished, terminate gracefully */
        }
	memset(tmpstr, 0, 256);
	for (i=0; i< 5; i++) { // read nettrace file name into "tmpstr"
          ret = read(mon_pipe2[0], tmpstr, 128);
	  if ((ret == -1) && (errno == EINTR)) // socket interrupted, try reading again
	    continue;
	  break;
	}
	  
	if (strlen(tmpstr) > 5)
	  memcpy(meta.s2c_ndttrace, tmpstr, strlen(tmpstr));
			// name of nettrace file passed back from pcap child copied into meta structure
      }

      /* Check, and if needed, set the web100 autotuning function on.  This improves
       * performance without requiring the entire system be re-configured.  Returned
       * value is true if set is done, so admin knows if system default should be changed
       *
       * 11/22/04, the sBufMode and rBufMode per connection autotuning variables are being
       * depricated, so this is another attempt at making this work.  If window scaling is
       * enabled, then scale the buffer size to the correct value.  This will also help
       * when reporting the buffer / RTT limit.
       */

    /*   if (autotune > 0) 
     *    web100_setbuff(xmitsfd, agent, conn, autotune);
     */

      /* experimental code, delete when finished */
      {
          web100_var *LimCwnd, *yar;
          u_int32_t limcwnd_val;
          char yuff[32];

          if (options->limit > 0) {
              log_println(1, "Setting Cwnd limit");
              web100_agent_find_var_and_group(agent, "CurMSS", &group, &yar);
              web100_raw_read(yar, conn, yuff);
              log_println(1, "MSS = %s, multiplication factor = %d",
                      web100_value_to_text(web100_get_var_type(yar), yuff), options->limit);
              web100_agent_find_var_and_group(agent, "LimCwnd", &group, &LimCwnd);
              limcwnd_val = options->limit * (atoi(web100_value_to_text(web100_get_var_type(yar), yuff)));
              log_print(1, "now write %d to limit the Congestion window", limcwnd_val);
              web100_raw_write(LimCwnd, conn, &limcwnd_val);
              log_println(1, "  ---  Done");
          }
      }
      /* End of test code */

      // create directory to write web100 snaplog trace
      {
        I2Addr sockAddr = I2AddrBySAddr(get_errhandle(), (struct sockaddr *) &cli_addr, clilen, 0, 0);
        memset(namebuf, 0, 256);
        I2AddrNodeName(sockAddr, namebuf, &nameBufLen);
	strncpy(options->s2c_logname, DataDirName, strlen(DataDirName));
    	if ((dp = opendir(options->s2c_logname)) == NULL && errno == ENOENT)
		mkdir(options->s2c_logname, 0755);
	closedir(dp);
    	get_YYYY(dir);
    	strncat(options->s2c_logname, dir, 4); 
    	if ((dp = opendir(options->s2c_logname)) == NULL && errno == ENOENT)
		mkdir(options->s2c_logname, 0755);
	closedir(dp);
    	strncat(options->s2c_logname, "/", 1);
    	get_MM(dir);
    	strncat(options->s2c_logname, dir, 2); 
    	if ((dp = opendir(options->s2c_logname)) == NULL && errno == ENOENT)
		mkdir(options->s2c_logname, 0755);
	closedir(dp);
    	strncat(options->s2c_logname, "/", 1);
    	get_DD(dir);
    	strncat(options->s2c_logname, dir, 2); 
    	if ((dp = opendir(options->s2c_logname)) == NULL && errno == ENOENT)
		mkdir(options->s2c_logname, 0755);
	closedir(dp);
    	strncat(options->s2c_logname, "/", 1);
    	sprintf(dir, "%s_%s:%d.s2c_snaplog", get_ISOtime(isoTime), namebuf, I2AddrPort(sockAddr));
	strncat(options->s2c_logname, dir, strlen(dir));
        group = web100_group_find(agent, "read");
        snapArgs.snap = web100_snapshot_alloc(group, conn);
  	I2AddrFree(sockAddr);

  		// If snaplog option is enabled, save snaplog trace
        if (options->snaplog) {
	    memcpy(meta.s2c_snaplog, dir, strlen(dir));
            fp = fopen(get_logfile(),"a");
            snapArgs.log = web100_log_open_write(options->s2c_logname, conn, group);
            if (fp == NULL) {
                log_println(0, "Unable to open log file '%s', continuing on without logging", get_logfile());
            }
            else {
                log_println(1, "snaplog file: %s\n", options->s2c_logname);
                fprintf(fp, "snaplog file: %s\n", options->s2c_logname);
                fclose(fp);
            }
        }
      }

      /* Kludge way of nuking Linux route cache.  This should be done
       * using the sysctl interface.
       */
      if (getuid() == 0) {
        /* system("/sbin/sysctl -w net.ipv4.route.flush=1"); */
        system("echo 1 > /proc/sys/net/ipv4/route/flush");
      }
      rgroup = web100_group_find(agent, "read");
      rsnap = web100_snapshot_alloc(rgroup, conn);
      tgroup = web100_group_find(agent, "tune");
      tsnap = web100_snapshot_alloc(tgroup, conn);

      // fill send buffer with random printable data for throughput test
      bytes = 0;
      k = 0;
      for (j=0; j<=BUFFSIZE; j++) {
        while (!isprint(k & 0x7f))
          k++;
        buff[j] = (k++ & 0x7f);
      }

      // Send message to client indicating TEST_START
      if (send_msg(ctlsockfd, TEST_START, "", 0) < 0)
	log_println(6, "S2C test - Test-start message failed for pid=%d", mon_pid2);
      /* ignore the alrm signal */
      memset(&new, 0, sizeof(new));
      new.sa_handler = catch_s2c_alrm;
      sigaction(SIGALRM, &new, &old);

      // capture current values (i.e take snap shot) of web_100 variables
      // Write into snaplog if option is enabled. Hold/release semaphores as needed.
      {
          WorkerArgs workerArgs;
          workerArgs.snapArgs = &snapArgs;
          workerArgs.agent = agent;
          workerArgs.peaks = peaks;
          workerArgs.writeSnap = options->snaplog;
          if (pthread_create(&workerThreadId, NULL, snapWorker, (void*) &workerArgs)) {
              log_println(0, "Cannot create worker thread for writing snap log!");
              workerThreadId = 0;
          }
          // acquire semaphore, and write snaplog
          pthread_mutex_lock(&mainmutex);
          workerLoop = 1;
          web100_snap(snapArgs.snap);
          if (options->snaplog) {
              web100_log_write(snapArgs.log, snapArgs.snap);
          }
          pthread_cond_wait(&maincond, &mainmutex);
          pthread_mutex_unlock(&mainmutex);
      }

      /* alarm(20); */
      t = secs();  // current time
      s = t + 10.0; // set timeout to 10 s in future

      log_println(6, "S2C child %d begining test", testOptions->child0);

      while(secs() < s) { 
        c3++; 							// Increment total attempts at sending-> buffer control
        if (options->avoidSndBlockUp) { // Do not block send buffers
            pthread_mutex_lock(&mainmutex);

            // get details of next sequence # to be sent and fetch value from snap file
            web100_agent_find_var_and_group(agent, "SndNxt", &group, &var);
            web100_snap_read(var, snapArgs.snap, tmpstr);
            SndMax = atoi(web100_value_to_text(web100_get_var_type(var), tmpstr));
            // get oldest un-acked sequence number
            web100_agent_find_var_and_group(agent, "SndUna", &group, &var);
            web100_snap_read(var, snapArgs.snap, tmpstr);
            SndUna = atoi(web100_value_to_text(web100_get_var_type(var), tmpstr));
            pthread_mutex_unlock(&mainmutex);

            // Temporarily stop sending data if you sense that the receiving end is overwhelmed
            // This is calculated by checking if (8192 * 4) <
            //        ((Next Sequence Number To Be Sent) - (Oldest Unacknowledged Sequence Number) - 1)
            // Increment draining queue value
            if ((RECLTH<<2) < (SndMax - SndUna - 1)) {
                c1++;
                continue;
            }
        }

        // attempt to write random data into the client socket
        n = write(xmitsfd, buff, RECLTH);
	if ((n == -1) && (errno == EINTR)) // socket interrupted, continue attempting to write
	  continue;
        if (n < 0)
          break;  // all data written. Exit
        bytes += n;

        if (options->avoidSndBlockUp) {
          c2++; // increment "sent data" queue
        }
      } // Completed end of trying to transmit data for the goodput test
      /* alarm(10); */
      sigaction(SIGALRM, &old, NULL);
      sndqueue = sndq_len(xmitsfd);

      // finalize the midbox test ; disabling socket used for throughput test
      log_println(6, "S2C child %d finished test", testOptions->child0);
      shutdown(xmitsfd, SHUT_WR);  /* end of write's */

      // get actual time duration during which data was transmitted
      s = secs() - t;

      // Throughput in kbps = (no of bits sent * 8) / (1000 * time data was sent)
      x2cspd = (8.e-3 * bytes) / s;

     // Release semaphore, and close snaplog file.  finalize other data
      if (workerThreadId) {
          pthread_mutex_lock(&mainmutex);
          workerLoop = 0;
          pthread_mutex_unlock(&mainmutex);
          pthread_join(workerThreadId, NULL);
      }
      if (options->snaplog) {
          web100_log_close_write(snapArgs.log);
      }
      web100_snapshot_free(snapArgs.snap);
      /* send the x2cspd to the client */
      memset(buff, 0, sizeof(buff));

      // Send throughput, unsent byte count, total sent byte count to client
      sprintf(buff, "%0.0f %d %0.0f", x2cspd, sndqueue, bytes);
      if (send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff)) < 0)
    	  log_println(6, "S2C test - failed to send test message to pid=%d", mon_pid2);

      web100_snap(rsnap);
      web100_snap(tsnap);

      log_println(1, "sent %d bytes to client in %0.2f seconds",(int) bytes, s);
      log_println(1, "Buffer control counters Total = %d, new data = %d, Draining Queue = %d", c3, c2, c1);

      /* Next send speed-chk a flag to retrieve the data it collected.
       * Skip this step if speed-chk isn't running.
       */

      if (getuid() == 0) {
        log_println(1, "Signal USR2(%d) sent to child [%d]", SIGUSR2, mon_pid2);
        testOptions->child2 = mon_pid2;
        kill(mon_pid2, SIGUSR2);
        FD_ZERO(&rfd);
        FD_SET(mon_pipe2[0], &rfd);
        sel_tv.tv_sec = 1;
        sel_tv.tv_usec = 100000;
	i = 0;

        for (;;) {
          ret = select(mon_pipe2[0]+1, &rfd, NULL, NULL, &sel_tv); 
          if ((ret == -1) && (errno == EINTR)) {
	    log_println(6, "Interrupt received while waiting for s2c select90 to finish, continuing");
            continue;
	  }
	  if (((ret == -1) && (errno != EINTR)) || (ret == 0)) {
            log_println(4, "Failed to read pkt-pair data from S2C flow, retcode=%d, reason=%d, EINTR=%d",
			  ret, errno, EINTR);
            sprintf(spds[(*spd_index)++], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0 -1");
            sprintf(spds[(*spd_index)++], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0 -1");
	    break;
	  }
	  /* There is something to read, so get it from the pktpair child.  If an interrupt occurs, 
           * just skip the read and go on
           * RAC 2/8/10
           */
	  if (ret > 0) {
            if ((ret = read(mon_pipe2[0], spds[*spd_index], 128)) < 0)
              sprintf(spds[*spd_index], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0 -1");
            log_println(1, "%d bytes read '%s' from S2C monitor pipe", ret, spds[*spd_index]);
            (*spd_index)++;
	    if (i++ == 1)
	      break;
	    sel_tv.tv_sec = 1;
	    sel_tv.tv_usec = 100000;
	    continue;
	  }
          /*   if ((ret = read(mon_pipe2[0], spds[*spd_index], 128)) < 0)
           *    sprintf(spds[*spd_index], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0 -1");
           *  log_println(1, "%d bytes read '%s' from S2C monitor pipe", ret, spds[*spd_index]);
           *  (*spd_index)++;
	   *  break;
           * } 
          */
        }
      }

      log_println(1, "%6.0f kbps inbound pid-%d", x2cspd, mon_pid2);
    }

    /* alarm(30); */  /* reset alarm() again, this 10 sec test should finish before this signal
                 * is generated.  */
    // Get web100 variables from snapshot taken earlier and send to client
    log_println(6, "S2C-Send web100 data vars to client pid=%d", mon_pid2);
    ret = web100_get_data(tsnap, ctlsockfd, agent, count_vars); //send web100 data to client
    web100_snapshot_free(tsnap);
    ret = web100_get_data(rsnap, ctlsockfd, agent, count_vars); //send tuning-related web100 data collected to client
    web100_snapshot_free(rsnap);

    // If sending web100 variables above failed, indicate to client
    if (ret < 0) {
	log_println(6, "S2C - No web100 data received for pid=%d", mon_pid2);
        sprintf(buff, "No Data Collected: 000000");
	send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
    }

    // Wait for message from client. Client sends its calculated throughput value
    msgLen = sizeof(buff);
    if (recv_msg(ctlsockfd, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error!");
      sprintf(buff, "Server (S2C throughput test): Invalid S2C throughput received");
      send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
      return -1;
    }
    if (check_msg_type("S2C throughput test", TEST_MSG, msgType, buff, msgLen)) {
      sprintf(buff, "Server (S2C throughput test): Invalid S2C throughput received");
      send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
      return -2;
    }
    if (msgLen <= 0) {
      log_println(0, "Improper message");
      sprintf(buff, "Server (S2C throughput test): Invalid S2C throughput received");
      send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
      return -3;
    }
    buff[msgLen] = 0;
    *s2cspd = atoi(buff); // save Throughput value as seen by client

    // Final activities of ending tests. Close sockets, file descriptors,
    //    send finalise message to client
    close(xmitsfd);
    if (send_msg(ctlsockfd, TEST_FINALIZE, "", 0) < 0)
	log_println(6, "S2C test - failed to send finalize message to pid=%d", mon_pid2);

    if (getuid() == 0) {
      for (i=0; i<5; i++) {
        ret = write(mon_pipe2[1], "c", 1);
	if (ret == 1)
	  break;
	if ((ret == -1) && (errno == EINTR))
	  continue;
      }
      close(mon_pipe2[0]);
      close(mon_pipe2[1]);
    }

    // log end of test (generic and protocol logs)
    log_println(1, " <------------ %d ------------->", testOptions->child0);
    //log protocol validation logs
    teststatuses = TEST_ENDED;
    //protolog_status(1,testOptions->child0, testids, teststatuses);
    protolog_status(testOptions->child0, testids, teststatuses);

    setCurrentTest(TEST_NONE);
    /* I2AddrFree(s2csrv_addr); */
    I2AddrFree(src_addr);
    /* testOptions->child2 = mon_pid2; */
  }
  /* I2AddrFree(s2csrv_addr); */
  /* I2AddrFree(src_addr); */
  /* testOptions->child2 = mon_pid2; */
  return 0;
}


