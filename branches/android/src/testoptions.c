/*
 * This file contains the functions needed to handle various tests.
 *
 * Jakub S³awiñski 2006-06-24
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

int mon_pipe1[2], mon_pipe2[2];
static int currentTest = TEST_NONE;

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
 * Function name: findCwndPeaks
 * Description: Count the CWND peaks and record the minimal and maximum one.
 * Arguments: agent - the Web100 agent used to track the connection
 *            peaks - the structure containing CWND peaks information
 *            snap - the web100 snapshot structure
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
      if (CurCwnd < prevCWNDval) {
          if (prevCWNDval > peaks->max) {
              peaks->max = prevCWNDval;
          }
          if (!decreasing) {
              peaks->amount += 1;
          }
          decreasing = 1;
      }
      else if (CurCwnd > prevCWNDval) {
          if ((peaks->min == -1) || (prevCWNDval < peaks->min)) {
              peaks->min = prevCWNDval;
          }
          decreasing = 0;
      }
  }
  prevCWNDval = CurCwnd;
}

/*
 * Function name: catch_s2c_alrm
 * Description: Prints the appropriate message when the SIGALRM is catched.
 * Arguments: signo - the signal number (shuld be SIGALRM)
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

/*
 * Function name: snapWorker
 * Description: Writes the snap logs with fixed time intervals in separate
 *              thread.
 * Arguments: arg - pointer to the snapshot structure
 * Returns: NULL
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

/*
 * Function name: initialize_tests
 * Description: Initializes the tests for the client.
 * Arguments: ctlsockfd - the client control socket descriptor
 *            options - the test options
 *            conn_options - the connection options
 * Returns: 0 - success,
 *          >0 - error code.
 */

int
/* initialize_tests(int ctlsockfd, TestOptions* options, int conn_options, char * buff) */
initialize_tests(int ctlsockfd, TestOptions* options, char * buff)
{
  unsigned char useropt=0;
  int msgType;
  int msgLen = 1;
  /* char buff[1024]; */
  int first = 1;
  char tmpbuff[16];

  assert(ctlsockfd != -1);
  assert(options);

  /* read the test suite request */
  if (recv_msg(ctlsockfd, &msgType, &useropt, &msgLen)) {
      sprintf(buff, "Invalid test suite request");
      send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
      return (-1);
  }
  if (msgLen == -1) {
      sprintf(buff, "Client timeout");
      return (-4);
  }
  if ((msgType != MSG_LOGIN) || (msgLen != 1)) {
      sprintf(buff, "Invalid test request");
      send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
      return (-2);
  }
  if (!(useropt & (TEST_MID | TEST_C2S | TEST_S2C | TEST_SFW | TEST_STATUS))) {
      sprintf(buff, "Invalid test suite request");
      send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
      return (-3);
  }
  if (useropt & TEST_MID) {
    /* options->midopt = TOPT_ENABLED; */
    if (first) {
      first = 0;
      sprintf(buff, "%ld", TEST_MID);
    }
  }
  if (useropt & TEST_SFW) {
    /* options->sfwopt = TOPT_ENABLED; */
    if (first) {
      first = 0;
      sprintf(buff, "%ld", TEST_SFW);
    }
    else {
      memset(tmpbuff, 0, 16);
      sprintf(tmpbuff, " %ld", TEST_SFW);
      strcat(buff, tmpbuff);
    }
  }
  if (useropt & TEST_C2S) {
    /* options->c2sopt = TOPT_ENABLED; */
    if (first) {
      first = 0;
      sprintf(buff, "%ld", TEST_C2S);
    }
    else {
      memset(tmpbuff, 0, 16);
      sprintf(tmpbuff, " %ld", TEST_C2S);
      strcat(buff, tmpbuff);
    }
  }
  if (useropt & TEST_S2C) {
    /* options->s2copt = TOPT_ENABLED; */
    if (first) {
      first = 0;
      sprintf(buff, "%ld", TEST_S2C);
    }
    else {
      memset(tmpbuff, 0, 16);
      sprintf(tmpbuff, " %ld", TEST_S2C);
      strcat(buff, tmpbuff);
    }
  }
  return useropt;
}

/*
 * Function name: test_mid
 * Description: Performs the Middlebox test.
 * Arguments: ctlsockfd - the client control socket descriptor
 *            agent - the Web100 agent used to track the connection
 *            options - the test options
 *            s2c2spd - the S2C throughput results (evaluated by the MID TEST)
 *            conn_options - the connection options
 * Returns: 0 - success,
 *          >0 - error code.
 */

int
test_mid(int ctlsockfd, web100_agent* agent, TestOptions* options, int conn_options, double* s2c2spd)
{
  int maxseg=1456;
  /* int maxseg=1456, largewin=16*1024*1024; */
  /* int seg_size, win_size; */
  int midsfd, j, ret;
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
  
  assert(ctlsockfd != -1);
  assert(agent);
  assert(options);
  assert(s2c2spd);
  
  if (options->midopt) {
    setCurrentTest(TEST_MID);
    log_println(1, " <-- %d - Middlebox test -->", options->child0);
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
        if (options->multiple == 0) {
            break;
        }
    }
    if (midsrv_addr == NULL) {
      log_println(0, "Server (Middlebox test): CreateListenSocket failed: %s", strerror(errno));
      sprintf(buff, "Server (Middlebox test): CreateListenSocket failed: %s", strerror(errno));
      send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
      return -1;
    }
    options->midsockfd = I2AddrFD(midsrv_addr);
    options->midsockport = I2AddrPort(midsrv_addr);
    log_println(1, "  -- port: %d", options->midsockport);
    
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
    FD_ZERO(&rfd);
    FD_SET(options->midsockfd, &rfd);
    sel_tv.tv_sec = 5;
    sel_tv.tv_usec = 0;
    for (j=0; j<5; j++) {
      ret = select((options->midsockfd)+1, &rfd, NULL, NULL, &sel_tv);
      if ((ret == -1) && (errno == EINTR))
	continue;
      if (ret == 0) 
	return -100;  /* timeout */ 
      if (ret < 0)
	return -errno; 
      if (j == 4)
	return -101;
midfd:
      if ((midsfd = accept(options->midsockfd, (struct sockaddr *) &cli_addr, &clilen)) > 0)
	break;

      if ((midsfd == -1) && (errno == EINTR))
	goto midfd;

      sprintf(tmpstr, "-------     middlebox connection setup returned because (%d)", errno);
      if (get_debuglvl() > 1)
        perror(tmpstr);
      if (midsfd < 0)
	return -errno;
      if (j == 4)
	return -102;
    } 
    memcpy(&meta.c_addr, &cli_addr, clilen);
    /* meta.c_addr = cli_addr; */
    meta.family = ((struct sockaddr *) &cli_addr)->sa_family;

    buff[0] = '\0';
    if ((conn = web100_connection_from_socket(agent, midsfd)) == NULL) {
        log_println(0, "!!!!!!!!!!!  test_mid() failed to get web100 connection data, rc=%d", errno);
        /* exit(-1); */
	return -3;
    }
    web100_middlebox(midsfd, agent, conn, buff);
    send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
    msgLen = sizeof(buff);
    if (recv_msg(ctlsockfd, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error!");
      sprintf(buff, "Server (Middlebox test): Invalid CWND limited throughput received");
      send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
      return 1;
    }
    if (check_msg_type("Middlebox test", TEST_MSG, msgType, buff, msgLen)) {
      sprintf(buff, "Server (Middlebox test): Invalid CWND limited throughput received");
      send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
      return 2;
    }
    if (msgLen <= 0) {
      log_println(0, "Improper message");
      sprintf(buff, "Server (Middlebox test): Invalid CWND limited throughput received");
      send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
      return 3;
    }
    buff[msgLen] = 0;
    *s2c2spd = atof(buff);
    log_println(4, "CWND limited throughput = %0.0f kbps (%s)", *s2c2spd, buff);

    shutdown(midsfd, SHUT_WR);
    close(midsfd);
    close(options->midsockfd);
    send_msg(ctlsockfd, TEST_FINALIZE, "", 0);
    log_println(1, " <--------- %d ----------->", options->child0);
    setCurrentTest(TEST_NONE);
  /* I2AddrFree(midsrv_addr); */
  }
  /* I2AddrFree(midsrv_addr); */
  return 0;
}

/*
 * Function name: test_c2s
 * Description: Performs the C2S Throughput test.
 * Arguments: ctlsockfd - the client control socket descriptor
 *            agent - the Web100 agent used to track the connection
 *            testOptions - the test options
 *            conn_options - the connection options
 * Returns: 0 - success,
 *          >0 - error code.
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
  size_t nameBufLen = 255;
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

  if (testOptions->c2sopt) {
    setCurrentTest(TEST_C2S);
    log_println(1, " <-- %d - C2S throughput test -->", testOptions->child0);
    strcpy(listenc2sport, PORT2);
    
    if (testOptions->c2ssockport) {
      sprintf(listenc2sport, "%d", testOptions->c2ssockport);
    }
    else if (testOptions->mainport) {
      sprintf(listenc2sport, "%d", testOptions->mainport + 1);
    }
    
    if (testOptions->multiple) {
      strcpy(listenc2sport, "0");
    }
    
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
    if ((ret = send_msg(ctlsockfd, TEST_PREPARE, buff, strlen(buff))) < 0)
	return ret;

    clilen = sizeof(cli_addr);
    log_println(6, "child %d - sent c2s prepare to client", testOptions->child0);
    FD_ZERO(&rfd);
    FD_SET(testOptions->c2ssockfd, &rfd);
    sel_tv.tv_sec = 5;
    sel_tv.tv_usec = 0;
    for (j=0; j<5; j++) {
      ret = select((testOptions->c2ssockfd)+1, &rfd, NULL, NULL, &sel_tv);
      if ((ret == -1) && (errno == EINTR))
	continue;
      if (ret == 0) 
	return -100;  /* timeout */ 
      if (ret < 0)
	return -errno; 
      if (j == 4)
	return -101;
recfd:
      recvsfd = accept(testOptions->c2ssockfd, (struct sockaddr *) &cli_addr, &clilen);
      if (recvsfd > 0) {
	log_println(6, "accept() for %d completed", testOptions->child0);
	break;
      }
      if ((recvsfd == -1) && (errno == EINTR)) {
	log_println(6, "Child %d interrupted while waiting for accept() to complete", 
		testOptions->child0);
	goto recfd;
      }
      log_println(6, "-------     C2S connection setup for %d returned because (%d)", 
		testOptions->child0, errno);
      if (recvsfd < 0)
	return -errno; 
      if (j == 4) {
	log_println(6, "c2s child %d, uable to open connection, return from test", testOptions->child0);
        return -102;
      }
    } 
    log_println(6, "child %d - c2s ready for test with fd=%d", testOptions->child0, recvsfd);
    src_addr = I2AddrByLocalSockFD(get_errhandle(), recvsfd, 0);
    conn = web100_connection_from_socket(agent, recvsfd);

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
      memset(tmpstr, 0, 256);
      for (i=0; i< 5; i++) {
          ret = read(mon_pipe1[0], tmpstr, 128);
	  if ((ret == -1) && (errno == EINTR))
	    continue;
	  break;
      }
      if (strlen(tmpstr) > 5)
        memcpy(meta.c2s_ndttrace, tmpstr, strlen(tmpstr));  /* name of nettrace file passed back from pcap child */
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
    }

    sleep(2);
    send_msg(ctlsockfd, TEST_START, "", 0);
    /* alarm(30); */  /* reset alarm() again, this 10 sec test should finish before this signal
                 * is generated.  */

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

    t = secs();
    sel_tv.tv_sec = 11;
    sel_tv.tv_usec = 0;
    FD_ZERO(&rfd);
    FD_SET(recvsfd, &rfd);
    for (;;) {
      ret = select(recvsfd+1, &rfd, NULL, NULL, &sel_tv);
      if ((ret == -1) && (errno == EINTR)) {
	continue;
      }
      if (ret > 0) {
        n = read(recvsfd, buff, sizeof(buff));
	if ((n == -1) && (errno == EINTR))
	  continue;
        if (n == 0)
          break;
        bytes += n;
        continue;
      }
      break;
    }

    t = secs()-t;
    *c2sspd = (8.e-3 * bytes) / t;

    if (workerThreadId) {
        pthread_mutex_lock(&mainmutex);
        workerLoop = 0;
        pthread_mutex_unlock(&mainmutex);
        pthread_join(workerThreadId, NULL);
    }
    if (options->snaplog) {
        web100_log_close_write(snapArgs.log);
    }

    sprintf(buff, "%6.0f kbps outbound for child %d", *c2sspd, testOptions->child0);
    log_println(1, "%s", buff);
    /* send the c2sspd to the client */
    sprintf(buff, "%0.0f", *c2sspd);
    send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
    /* get receiver side Web100 stats and write them to the log file */
    if (record_reverse == 1)
      web100_get_data_recv(recvsfd, agent, conn, count_vars);
    /* shutdown(recvsfd, SHUT_RD); */
    close(recvsfd);
    close(testOptions->c2ssockfd);

    /* Next send speed-chk a flag to retrieve the data it collected.
     * Skip this step if speed-chk isn't running.
     */

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

    send_msg(ctlsockfd, TEST_FINALIZE, "", 0);

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

    log_println(1, " <----------- %d -------------->", testOptions->child0);
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

/*
 * Function name: test_s2c
 * Description: Performs the S2C Throughput test.
 * Arguments: ctlsockfd - the client control socket descriptor
 *            agent - the Web100 agent used to track the connection
 *            testOptions - the test options
 *            conn_options - the connection options
 * Returns: 0 - success,
 *          >0 - error code.
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
  int c3=0, i;
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
  int c1=0, c2=0;
  SnapArgs snapArgs;
  snapArgs.snap = NULL;
  snapArgs.log = NULL;
  snapArgs.delay = options->snapDelay;
  wait_sig = 0;
  
  if (testOptions->s2copt) {
    setCurrentTest(TEST_S2C);
    log_println(1, " <-- %d - S2C throughput test -->", testOptions->child0);
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

    /* Data received from speed-chk, tell applet to start next test */
    sprintf(buff, "%d", testOptions->s2csockport);
    j = send_msg(ctlsockfd, TEST_PREPARE, buff, strlen(buff));
    if (j == -1) {
	log_println(6, "S2C %d Error!, Test start message not sent!", testOptions->child0);
	return j;
    }
    if (j == -2) {
	log_println(6, "S2C %d Error!, server port [%s] not sent!", testOptions->child0, buff);
	return j;
    }
    
    /* ok, await for connect on 3rd port
     * This is the second throughput test, with data streaming from
     * the server back to the client.  Again stream data for 10 seconds.
     */
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
	return -100;  /* timeout */ 
      if (ret < 0)
	return -errno; 
      if (j == 4)
	return -101;
ximfd:
      xmitsfd = accept(testOptions->s2csockfd, (struct sockaddr *) &cli_addr, &clilen);
      if (xmitsfd > 0) {
	log_println(6, "accept() for %d completed", testOptions->child0);
	break;
      }
      if ((xmitsfd == -1) && (errno == EINTR)) {
	log_println(6, "Child %d interrupted while waiting for accept() to complete", 
		testOptions->child0);
	goto ximfd;
      }
      log_println(6, "-------     S2C connection setup for %d returned because (%d)", 
		testOptions->child0, errno);
      if (xmitsfd < 0)
	return -errno; 
      if (++j == 4) {
	log_println(6, "s2c child %d, uable to open connection, return from test", testOptions->child0);
        return -102;
      }
    } 
    src_addr = I2AddrByLocalSockFD(get_errhandle(), xmitsfd, 0);
    conn = web100_connection_from_socket(agent, xmitsfd); 

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
	for (i=0; i< 5; i++) {
          ret = read(mon_pipe2[0], tmpstr, 128);
	  if ((ret == -1) && (errno == EINTR))
	    continue;
	  break;
	}
	  
	if (strlen(tmpstr) > 5)
	  memcpy(meta.s2c_ndttrace, tmpstr, strlen(tmpstr));  /* name of nettrace file passed back from pcap child */
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

      bytes = 0;
      k = 0;
      for (j=0; j<=BUFFSIZE; j++) {
        while (!isprint(k & 0x7f))
          k++;
        buff[j] = (k++ & 0x7f);
      }

      if (send_msg(ctlsockfd, TEST_START, "", 0) < 0)
	log_println(6, "S2C test - Test-start message failed for pid=%d", mon_pid2);
      /* ignore the alrm signal */
      memset(&new, 0, sizeof(new));
      new.sa_handler = catch_s2c_alrm;
      sigaction(SIGALRM, &new, &old);

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
      t = secs();
      s = t + 10.0;

      log_println(6, "S2C child %d begining test", testOptions->child0);
      while(secs() < s) { 
        c3++;
        if (options->avoidSndBlockUp) {
            pthread_mutex_lock(&mainmutex);
            web100_agent_find_var_and_group(agent, "SndNxt", &group, &var);
            web100_snap_read(var, snapArgs.snap, tmpstr);
            SndMax = atoi(web100_value_to_text(web100_get_var_type(var), tmpstr));
            web100_agent_find_var_and_group(agent, "SndUna", &group, &var);
            web100_snap_read(var, snapArgs.snap, tmpstr);
            SndUna = atoi(web100_value_to_text(web100_get_var_type(var), tmpstr));
            pthread_mutex_unlock(&mainmutex);
            if ((RECLTH<<2) < (SndMax - SndUna - 1)) {
                c1++;
                continue;
            }
        }

        n = write(xmitsfd, buff, RECLTH);
	if ((n == -1) && (errno == EINTR))
	  continue;
        if (n < 0)
          break;
        bytes += n;

        if (options->avoidSndBlockUp) {
          c2++;
        }
      }
      /* alarm(10); */
      sigaction(SIGALRM, &old, NULL);
      sndqueue = sndq_len(xmitsfd);

      log_println(6, "S2C child %d finished test", testOptions->child0);
      shutdown(xmitsfd, SHUT_WR);  /* end of write's */

      s = secs() - t;
      x2cspd = (8.e-3 * bytes) / s;
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
	    log_println(6, "Inerrupt received while waiting for s2c select90 to finish, continuing");
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
    log_println(6, "S2C-Send web100 data vars to client pid=%d", mon_pid2);
    ret = web100_get_data(tsnap, ctlsockfd, agent, count_vars);
    web100_snapshot_free(tsnap);
    ret = web100_get_data(rsnap, ctlsockfd, agent, count_vars);
    web100_snapshot_free(rsnap);
    if (ret < 0) {
	log_println(6, "S2C - No web100 data received for pid=%d", mon_pid2);
        sprintf(buff, "No Data Collected: 000000");
	send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
    }

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
    *s2cspd = atoi(buff);

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

    log_println(1, " <------------ %d ------------->", testOptions->child0);
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

/*
 * Function name: getCurrentTest
 * Description: Returns the id of the currently running test.
 * Returns: The id of the currently running test.
 */

int
getCurrentTest()
{
  return currentTest;
}

/*
 * Function name: setCurrentTest
 * Description: Sets the id of the currently running test.
 * Arguments: testId - the id of the currently running test
 */

void
setCurrentTest(int testId)
{
  currentTest = testId;
}
