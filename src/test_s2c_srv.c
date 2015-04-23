/**
 * File contains tests to perform the S2C Throughput test.
 * This throughput test tests the achievable network bandwidth from
 * the Server to the Client by performing a 10 seconds
 * memory-to-memory data transfer.
 *
 *  Created : Oct 17, 2011
 *      Author: kkumar@internet2.edu
 */
#include  <syslog.h>
#include <pthread.h>
#include <sys/times.h>
#include <ctype.h>
#include "tests_srv.h"
#include "strlutils.h"
#include "ndtptestconstants.h"
#include "utils.h"
#include "testoptions.h"
#include "runningtest.h"
#include "logging.h"
#include "protocol.h"
#include "network.h"
#include "mrange.h"
#include "jsonutils.h"
#include "websocket.h"

extern pthread_mutex_t mainmutex;
extern pthread_cond_t maincond;

const char RESULTS_KEYS[] = "ThroughputValue UnsentDataAmount TotalSentByte";

/**
 * Perform the S2C Throughput test. This throughput test tests the achievable
 * network bandwidth from the Server to the Client by performing a 10 seconds
 * memory-to-memory data transfer.
 *
 * The Server also collects web100 data variables, that are sent to the Client
 * at the end of the test session.
 *
 * Protocol messages exchanged between the Client and Server
 * are sent using the same connection and message format as the NDTP-Control protocol.
 * The throughput packets are sent on the new connection, though, and do not
 * follow the NDTP-Control protocol message format.
 *
 * @param ctlsockfd - the client control socket descriptor
 * @param agent - the Web100 agent used to track the connection
 * @param testOptions - the test options
 * @param conn_options - the connection options
 * @param testOptions Test options
 * @param s2cspd In-out parameter to store S2C throughput value
 * @param set_buff enable setting TCP send/recv buffer size to be used (seems unused in file)
 * @param window value of TCP send/rcv buffer size intended to be used.
 * @param autotune autotuning option. Deprecated.
 * @param device string devine name inout parameter
 * @param options Test Option variables
 * @param spds[][] speed check array
 * @param spd_index  index used for speed check array
 * @param count_vars count of web100 variables
 * @param peaks Cwnd peaks structure pointer
 *
 * @return 0 - success,
 *         >0 - error code.
 *     Error codes:
 * 			 	-1	   - Message reception errors/inconsistencies in client�s final message, or Listener socket creation failed or cannot write message header information while attempting to send
 *        		 TEST_PREPARE message
 *				-2 	   - Cannot write message data while attempting to send
 *           		 TEST_PREPARE message, or Unexpected message type received
 *           	-3 	   -  Received message is invalid
 *          	-100   - timeout while waiting for client to connect to server�s ephemeral port
 *				-101   - Retries exceeded while waiting for client to connect
 *				-102   - Retries exceeded while waiting for data from connected client
 *  			-errno - Other specific socket error numbers
 */
int test_s2c(int ctlsockfd, tcp_stat_agent* agent, TestOptions* testOptions,
             int conn_options, double* s2cspd, int set_buff, int window,
             int autotune, char* device, Options* options, char spds[4][256],
             int* spd_index, int count_vars, CwndPeaks* peaks) {
#if USE_WEB100
  /* experimental code to capture and log multiple copies of the
   * web100 variables using the web100_snap() & log() functions.
   */
  web100_snapshot* tsnap = NULL;
  web100_snapshot* rsnap = NULL;
  web100_group* tgroup;
  web100_group* rgroup;
  web100_var* var;
#elif USE_WEB10G
  estats_val_data* snap;
#endif
  tcp_stat_connection conn;
  /* Just a holder for web10g */
  tcp_stat_group* group = NULL;
  /* Pipe that handles returning packet pair timing */
  int mon_pipe[2];
  int ret;  // ctrl protocol read/write return status
  int j, k, n;
  int xmitsfd;  // transmit (i.e server) socket fd
  pid_t s2c_childpid = 0;  // s2c_childpid

  char tmpstr[256];  // string array used for temp storage of many char*
  struct sockaddr_storage cli_addr;

  socklen_t clilen;
  double bytes_written;  // bytes written in the throughput test
  double tx_duration;  // total time for which data was txed
  double tmptime;  // temporary time store
  double x2cspd;  // s->c test throughput
  struct timeval sel_tv;  // time
  fd_set rfd;  // receive file descriptor
  char buff[BUFFSIZE + 1];  // message payload buffer
  int bufctrlattempts = 0;  // number of buffer control attempts
  int i;  // temporary var used for iterators etc
  PortPair pair;  // socket ports
  I2Addr s2csrv_addr = NULL;
  I2Addr src_addr = NULL;
  char listens2cport[10];
  int msgType;
  int msgLen;
  int sndqueue;
  struct sigaction new, old;
  char* jsonMsgValue;

  pthread_t workerThreadId;
  int nextseqtosend = 0, lastunackedseq = 0;
  int drainingqueuecount = 0, bufctlrnewdata = 0;

  // variables used for protocol validation logs
  enum TEST_STATUS_INT teststatuses = TEST_NOT_STARTED;
  enum TEST_ID testids = S2C;
  enum PROCESS_STATUS_INT procstatusenum = UNKNOWN;
  enum PROCESS_TYPE_INT proctypeenum = CONNECT_TYPE;
  char snaplogsuffix[256] = "s2c_snaplog";

  SnapArgs snapArgs;
  snapArgs.snap = NULL;
#if USE_WEB100
  snapArgs.log = NULL;
#endif
  snapArgs.delay = options->snapDelay;
  wait_sig = 0;

  log_println(0, "test client version: %s", testOptions->client_version);

  // Determine port to be used. Compute based on options set earlier
  // by reading from config file, or use default port2 (3003)
  if (testOptions->s2copt) {
    setCurrentTest(TEST_S2C);
    log_println(1, " <-- %d - S2C throughput test -->",
                testOptions->child0);

    // protocol logs
    teststatuses = TEST_STARTED;
    protolog_status(testOptions->child0, testids, teststatuses, ctlsockfd);

    strlcpy(listens2cport, PORT4, sizeof(listens2cport));

    if (testOptions->s2csockport) {
      snprintf(listens2cport, sizeof(listens2cport), "%d",
               testOptions->s2csockport);
    } else if (testOptions->mainport) {
      snprintf(listens2cport, sizeof(listens2cport), "%d",
               testOptions->mainport + 2);
    }

    if (testOptions->multiple) {
      strlcpy(listens2cport, "0", sizeof(listens2cport));
    }

    // attempt to bind to a new port and obtain address structure with details
    // of listening port
    while (s2csrv_addr == NULL) {
      s2csrv_addr = CreateListenSocket(
          NULL,
          testOptions->multiple ?
              mrange_next(listens2cport, sizeof(listens2cport)) :
              listens2cport,
          conn_options, 0);
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
      log_println(
          0,
          "Server (S2C throughput test): CreateListenSocket failed: %s",
          strerror(errno));
      snprintf(
          buff,
          sizeof(buff),
          "Server (S2C throughput test): CreateListenSocket failed: %s",
          strerror(errno));
      send_json_message(ctlsockfd, MSG_ERROR, buff, strlen(buff),
                        testOptions->connection_flags, JSON_SINGLE_VALUE);
      return -1;
    }

    // get socket FD and the ephemeral port number that client will connect to
    // run tests
    testOptions->s2csockfd = I2AddrFD(s2csrv_addr);
    testOptions->s2csockport = I2AddrPort(s2csrv_addr);
    log_println(1, "  -- s2c %d port: %d", testOptions->child0,
                testOptions->s2csockport);
    pair.port1 = -1;
    pair.port2 = testOptions->s2csockport;

    // Data received from speed-chk. Send TEST_PREPARE "GO" signal with port
    // number
    snprintf(buff, sizeof(buff), "%d", testOptions->s2csockport);
    j = send_json_message(ctlsockfd, TEST_PREPARE, buff, strlen(buff),
                          testOptions->connection_flags, JSON_SINGLE_VALUE);
    if (j == -1) {
      log_println(6, "S2C %d Error!, Test start message not sent!",
                  testOptions->child0);
      return j;
    }
    if (j == -2) {  // could not write message data
      log_println(6, "S2C %d Error!, server port [%s] not sent!",
                  testOptions->child0, buff);
      return j;
    }

    // ok, await for connect on 3rd port
    // This is the second throughput test, with data streaming from
    // the server back to the client.  Again stream data for 10 seconds.

    log_println(1, "%d waiting for data on testOptions->s2csockfd",
                testOptions->child0);

    clilen = sizeof(cli_addr);
    FD_ZERO(&rfd);
    FD_SET(testOptions->s2csockfd, &rfd);
    sel_tv.tv_sec = 5;  // wait for 5 secs
    sel_tv.tv_usec = 0;
    for (j = 0; j < RETRY_COUNT; j++) {
      ret = select((testOptions->s2csockfd) + 1, &rfd, NULL, NULL,
                   &sel_tv);
      if ((ret == -1) && (errno == EINTR))
        continue;
      if (ret == 0)
        return SOCKET_CONNECT_TIMEOUT;  // timeout
      if (ret < 0)
        return -errno;  // other socket errors. exit
      if (j == 4)
        return RETRY_EXCEEDED_WAITING_CONNECT;  // retry exceeded. exit

      // If a valid connection request is received, client has connected.
      // Proceed.
      // Note the new socket fd - xmitfd - used in the throughput test
ximfd: xmitsfd = accept(testOptions->s2csockfd,
                        (struct sockaddr *) &cli_addr, &clilen);
       if (xmitsfd > 0) {
         log_println(6, "accept() for %d completed",
                     testOptions->child0);
         procstatusenum = PROCESS_STARTED;
         proctypeenum = CONNECT_TYPE;
         protolog_procstatus(testOptions->child0, testids, proctypeenum,
                             procstatusenum, xmitsfd);
         if (testOptions->connection_flags & WEBSOCKET_SUPPORT) {
	   // To preserve user privacy, make sure that the HTTP header
	   // processing is done prior to the start of packet capture, as many
	   // browsers have headers that uniquely identitfy a single user.
           if (initialize_websocket_connection(xmitsfd, 0, "s2c") != 0) {
             xmitsfd = 0;
           } 
         } 
         break;
       }
       // socket interrupted, wait some more
       if ((xmitsfd == -1) && (errno == EINTR)) {
         log_println(
             6,
             "Child %d interrupted while waiting for accept() to complete",
             testOptions->child0);
         goto ximfd;
       }
       log_println(
           6,
           "-------     S2C connection setup for %d returned because (%d)",
           testOptions->child0, errno);
       if (xmitsfd < 0)  // other socket errors, quit
         return -errno;
       if (++j == 4) {  // retry exceeded, quit
         log_println(
             6,
             "s2c child %d, unable to open connection, return from test",
             testOptions->child0);
         return -102;
       }
    }
    src_addr = I2AddrByLocalSockFD(get_errhandle(), xmitsfd, 0);
    conn = tcp_stat_connection_from_socket(agent, xmitsfd);

    // set up packet capture. The data collected is used for bottleneck link
    // calculations
    if (xmitsfd > 0) {
      log_println(6, "S2C child %d, ready to fork()",
                  testOptions->child0);
      if (getuid() == 0) {
        /*
           pipe(mon_pipe2);
           start_packet_trace(xmitsfd, testOptions->s2csockfd,
           &s2c_childpid, mon_pipe2, (struct sockaddr *) &cli_addr,
           clilen, device, &pair, "s2c", options->compress,
           meta.s2c_ndttrace);
           */
        if (pipe(mon_pipe) != 0) {
          log_println(0, "S2C test error: can't create pipe.");
        } else {
          if ((s2c_childpid = fork()) == 0) {
            /* close(ctlsockfd); */
            close(testOptions->s2csockfd);
            close(xmitsfd);
            log_println(
                5,
                "S2C test Child thinks pipe() returned fd0=%d, fd1=%d",
                mon_pipe[0], mon_pipe[1]);
            // log_println(2, "S2C test calling init_pkttrace() with pd=0x%x",
            //             (int) &cli_addr);
            init_pkttrace(src_addr, (struct sockaddr *) &cli_addr,
                          clilen, mon_pipe, device, &pair, "s2c",
                          options->compress);
            log_println(6,
                        "S2C test ended, why is timer still running?");
            /* Close the pipe */
            close(mon_pipe[0]);
            close(mon_pipe[1]);
            exit(0); /* Packet trace finished, terminate gracefully */
          } else if (s2c_childpid < 0){
            log_println(0, "S2C test error: can't create child process.");
          }
        }
        memset(tmpstr, 0, 256);
        for (i = 0; i < 5; i++) {  // read nettrace file name into "tmpstr"
          ret = read(mon_pipe[0], tmpstr, 128);
          // socket interrupted, try reading again
          if ((ret == -1) && (errno == EINTR))
            continue;
          break;
        }

        if (strlen(tmpstr) > 5)
          memcpy(meta.s2c_ndttrace, tmpstr, strlen(tmpstr));
        // name of nettrace file passed back from pcap child copied into meta
        // structure
      }

      /* experimental code, delete when finished */
      setCwndlimit(conn, group, agent, options);
      /* End of test code */

      // create directory to write web100 snaplog trace
      create_client_logdir((struct sockaddr *) &cli_addr, clilen,
                           options->s2c_logname, sizeof(options->s2c_logname),
                           snaplogsuffix, sizeof(snaplogsuffix));

      /* Kludge way of nuking Linux route cache.  This should be done
       * using the sysctl interface.
       */
      if (getuid() == 0) {
        // system("/sbin/sysctl -w net.ipv4.route.flush=1");
        if (system("echo 1 > /proc/sys/net/ipv4/route/flush") != 0)
          log_println(0, "C2S test error: error calling system function: \"echo 1 > /proc/sys/net/ipv4/route/flush\"");
      }
#if USE_WEB100
      rgroup = web100_group_find(agent, "read");
      rsnap = web100_snapshot_alloc(rgroup, conn);
      tgroup = web100_group_find(agent, "tune");
      tsnap = web100_snapshot_alloc(tgroup, conn);
#elif USE_WEB10G
      estats_val_data_new(&snap);
#endif

      // fill send buffer with random printable data for throughput test
      bytes_written = 0;
      k = 0;
      for (j = 0; j <= BUFFSIZE; j++) {
        while (!isprint(k & 0x7f))
          k++;
        buff[j] = (k++ & 0x7f);
      }
      if (testOptions->connection_flags & WEBSOCKET_SUPPORT) {
        // Make sure the data has a websocket header
        ((unsigned char*)buff)[0] = 0x82;  // One frame of binary data
	// Depending on BUFFSIZE, the websocket header will be 2, 4, or 10
	// bytes big.  This header is constructed to comply with RFC 6455.
        if (BUFFSIZE < 126) {
          buff[1] = (BUFFSIZE-2) & 0x7F;
        } else if (BUFFSIZE < 65536) {
          buff[1] = 126;
          ((unsigned char*)buff)[2] = ((BUFFSIZE - 4) >> 8) & 0xFF;
          ((unsigned char*)buff)[3] = (BUFFSIZE - 4) & 0xFF;
        } else {
          buff[1] = 127;
          ((unsigned char*)buff)[2] = (((long long)BUFFSIZE - 10) >> 56) & 0xFF;
          ((unsigned char*)buff)[3] = (((long long)BUFFSIZE - 10) >> 48) & 0xFF;
          ((unsigned char*)buff)[4] = (((long long)BUFFSIZE - 10) >> 40) & 0xFF;
          ((unsigned char*)buff)[5] = (((long long)BUFFSIZE - 10) >> 32) & 0xFF;
          ((unsigned char*)buff)[6] = (((long long)BUFFSIZE - 10) >> 24) & 0xFF;
          ((unsigned char*)buff)[7] = (((long long)BUFFSIZE - 10) >> 16) & 0xFF;
          ((unsigned char*)buff)[8] = (((long long)BUFFSIZE - 10) >> 8) & 0xFF;
          ((unsigned char*)buff)[9] = (BUFFSIZE - 10) & 0xFF;
        }
      }

      // Send message to client indicating TEST_START
      if (send_json_message(ctlsockfd, TEST_START, "", 0, testOptions->connection_flags,
                            JSON_SINGLE_VALUE) < 0)
        log_println(6,
                    "S2C test - Test-start message failed for pid=%d",
                    s2c_childpid);
      // ignore the alarm signal
      memset(&new, 0, sizeof(new));
      new.sa_handler = catch_s2c_alrm;
      sigaction(SIGALRM, &new, &old);

      // capture current values (i.e take snap shot) of web_100 variables
      // Write snap logs if option is enabled. update meta log to point to
      // this snaplog

      // If snaplog option is enabled, save snaplog details in meta file
      if (options->snaplog) {
        memcpy(meta.s2c_snaplog, snaplogsuffix, strlen(snaplogsuffix));
      }
      // get web100 snapshot and also log it based on options
      /*start_snap_worker(&snapArgs, agent, options->snaplog, &workerLoop,
        &workerThreadId, meta.s2c_snaplog, options->s2c_logname,
        conn, group);*///new file changes
      start_snap_worker(&snapArgs, agent, peaks, options->snaplog,
                        &workerThreadId, meta.s2c_snaplog, options->s2c_logname,
                        conn, group);

      /* alarm(20); */
      tmptime = secs();  // current time
      tx_duration = tmptime + 10.0;  // set timeout to 10 s in future

      log_println(6, "S2C child %d beginning test", testOptions->child0);

      while (secs() < tx_duration) {
        // Increment total attempts at sending-> buffer control
        bufctrlattempts++;
        if (options->avoidSndBlockUp) {  // Do not block send buffers
          pthread_mutex_lock(&mainmutex);

          // get details of next sequence # to be sent and fetch value from
          // snap file
#if USE_WEB100
          web100_agent_find_var_and_group(agent, "SndNxt", &group,
                                          &var);
          web100_snap_read(var, snapArgs.snap, tmpstr);
          nextseqtosend = atoi(
              web100_value_to_text(web100_get_var_type(var),
                                   tmpstr));
          // get oldest un-acked sequence number
          web100_agent_find_var_and_group(agent, "SndUna", &group,
                                          &var);
          web100_snap_read(var, snapArgs.snap, tmpstr);
          lastunackedseq = atoi(
              web100_value_to_text(web100_get_var_type(var),
                                   tmpstr));
#elif USE_WEB10G
          struct estats_val value;
          web10g_find_val(snapArgs.snap, "SndNxt", &value);
          nextseqtosend = value.uv32;
          web10g_find_val(snapArgs.snap, "SndUna", &value);
          lastunackedseq = value.uv32;
#endif
          pthread_mutex_unlock(&mainmutex);

          // Temporarily stop sending data if you sense that the buffer is
          // overwhelmed
          // This is calculated by checking if (8192 * 4) <
          //    ((Next Sequence Number To Be Sent) -
          //     (Oldest Unacknowledged Sequence Number) - 1)
          if (is_buffer_clogged(nextseqtosend, lastunackedseq)) {
            // Increment draining queue value
            drainingqueuecount++;
            continue;
          }
        }

        // attempt to write random data into the client socket
        n = write(xmitsfd, buff, RECLTH);
        // socket interrupted, continue attempting to write
        if ((n == -1) && (errno == EINTR))
          continue;
        if (n < 0)
          break;  // all data written. Exit
        bytes_written += n;

        if (options->avoidSndBlockUp) {
          bufctlrnewdata++;  // increment "sent data" queue
        }
      }  // Completed end of trying to transmit data for the goodput test
      /* alarm(10); */
      sigaction(SIGALRM, &old, NULL);
      sndqueue = sndq_len(xmitsfd);

      // finalize the midbox test ; disabling socket used for throughput test
      log_println(6, "S2C child %d finished test", testOptions->child0);
      shutdown(xmitsfd, SHUT_WR); /* end of write's */

      // get actual time duration during which data was transmitted
      tx_duration = secs() - tmptime;

      // Throughput in kbps =
      // (no of bits sent * 8) / (1000 * time data was sent)
      x2cspd = (8.e-3 * bytes_written) / tx_duration;

      // Release semaphore, and close snaplog file.  finalize other data
      stop_snap_worker(&workerThreadId, options->snaplog, &snapArgs);

      // send the x2cspd to the client
      memset(buff, 0, sizeof(buff));

      // Send throughput, unsent byte count, total sent byte count to client
      snprintf(buff, sizeof(buff), "%0.0f %d %0.0f", x2cspd, sndqueue,
               bytes_written);
      if (testOptions->connection_flags & JSON_SUPPORT) {
        if (send_json_msg(ctlsockfd, TEST_MSG, buff, strlen(buff), testOptions->connection_flags,
                          JSON_MULTIPLE_VALUES, RESULTS_KEYS, " ", buff, " ") < 0)
            log_println(6,
                "S2C test - failed to send test message to pid=%d",
                s2c_childpid);
      }
      else {
        if (send_json_message(ctlsockfd, TEST_MSG, buff, strlen(buff),
                              testOptions->connection_flags, JSON_SINGLE_VALUE) < 0)
          log_println(6,
              "S2C test - failed to send test message to pid=%d",
              s2c_childpid);
      }


#if USE_WEB100
      web100_snap(rsnap);
      web100_snap(tsnap);
#elif USE_WEB10G
      estats_read_vars(snap, conn, agent);
#endif

      log_println(1, "sent %d bytes to client in %0.2f seconds",
                  (int) bytes_written, tx_duration);
      log_println(
          1,
          "Buffer control counters Total = %d, new data = %d, "
          "Draining Queue = %d",
          bufctrlattempts, bufctlrnewdata, drainingqueuecount);

      /* Next send speed-chk a flag to retrieve the data it collected.
       * Skip this step if speed-chk isn't running.
       */

      if (getuid() == 0) {
        log_println(1, "Signal USR2(%d) sent to child [%d]", SIGUSR2,
                    s2c_childpid);
        testOptions->child2 = s2c_childpid;
        kill(s2c_childpid, SIGUSR2);
        FD_ZERO(&rfd);
        FD_SET(mon_pipe[0], &rfd);
        sel_tv.tv_sec = 1;
        sel_tv.tv_usec = 100000;
        i = 0;

        for (;;) {
          ret = select(mon_pipe[0] + 1, &rfd, NULL, NULL, &sel_tv);
          if ((ret == -1) && (errno == EINTR)) {
            log_println(
                6,
                "Interrupt received while waiting for s2c select90 to finish, "
                "continuing");
            continue;
          }
          if (((ret == -1) && (errno != EINTR)) || (ret == 0)) {
            log_println(
                4,
                "Failed to read pkt-pair data from S2C flow, retcode=%d, "
                "reason=%d, EINTR=%d",
                ret, errno, EINTR);
            snprintf(
                spds[(*spd_index)++],
                sizeof(spds[*spd_index]),
                " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0 -1");
            snprintf(
                spds[(*spd_index)++],
                sizeof(spds[*spd_index]),
                " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0 -1");
            break;
          }
          /* There is something to read, so get it from the pktpair child.  If an interrupt occurs,
           * just skip the read and go on
           * RAC 2/8/10
           */
          if (ret > 0) {
            if ((ret = read(mon_pipe[0], spds[*spd_index], 128))
                < 0)
              snprintf(
                  spds[*spd_index],
                  sizeof(spds[*spd_index]),
                  " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0 -1");
            log_println(1,
                        "%d bytes read '%s' from S2C monitor pipe", ret,
                        spds[*spd_index]);
            (*spd_index)++;
            if (i++ == 1)
              break;
            sel_tv.tv_sec = 1;
            sel_tv.tv_usec = 100000;
            continue;
          }
        }
      }

      log_println(1, "%6.0f kbps inbound pid-%d", x2cspd, s2c_childpid);
    }
    /* reset alarm() again, this 10 sec test should finish before this signal
     * is generated.  */
    /* alarm(30); */
    // Get web100 variables from snapshot taken earlier and send to client
    log_println(6, "S2C-Send web100 data vars to client pid=%d",
                s2c_childpid);

#if USE_WEB100
    // send web100 data to client
    ret = tcp_stat_get_data(tsnap, xmitsfd, ctlsockfd, agent, count_vars, testOptions);
    web100_snapshot_free(tsnap);
    // send tuning-related web100 data collected to client
    ret = tcp_stat_get_data(rsnap, xmitsfd, ctlsockfd, agent, count_vars, testOptions);
    web100_snapshot_free(rsnap);
#elif USE_WEB10G
    ret = tcp_stat_get_data(snap, xmitsfd, ctlsockfd, agent, count_vars, testOptions);
    estats_val_data_free(&snap);
#endif

    // If sending web100 variables above failed, indicate to client
    if (ret < 0) {
      log_println(6, "S2C - No web100 data received for pid=%d",
                  s2c_childpid);
      snprintf(buff, sizeof(buff), "No Data Collected: 000000");
      send_json_message(ctlsockfd, TEST_MSG, buff, strlen(buff), testOptions->connection_flags,
                        JSON_SINGLE_VALUE);
    }

    // Wait for message from client. Client sends its calculated throughput
    // value
    log_println(6, "S2CSPD reception starts");
    msgLen = sizeof(buff);
    if (recv_any_msg(ctlsockfd, &msgType, buff, &msgLen, testOptions->connection_flags)) {
      log_println(0, "Protocol error!");
      snprintf(
          buff,
          sizeof(buff),
          "Server (S2C throughput test): Invalid S2C throughput received");
      send_json_message(ctlsockfd, MSG_ERROR, buff, strlen(buff),
                        testOptions->connection_flags, JSON_SINGLE_VALUE);
      return -1;
    }
    if (check_msg_type("S2C throughput test", TEST_MSG, msgType, buff,
                       msgLen)) {
      snprintf(
          buff,
          sizeof(buff),
          "Server (S2C throughput test): Invalid S2C throughput received");
      send_json_message(ctlsockfd, MSG_ERROR, buff, strlen(buff),
                        testOptions->connection_flags, JSON_SINGLE_VALUE);
      return -2;
    }
    buff[msgLen] = 0;
    if (testOptions->connection_flags & JSON_SUPPORT) {
      jsonMsgValue = json_read_map_value(buff, DEFAULT_KEY);
      strlcpy(buff, jsonMsgValue, sizeof(buff));
      msgLen = strlen(buff);
      free(jsonMsgValue);
    }
    if (msgLen <= 0) {
      log_println(0, "Improper message");
      snprintf(
          buff,
          sizeof(buff),
          "Server (S2C throughput test): Invalid S2C throughput received");
      send_json_message(ctlsockfd, MSG_ERROR, buff, strlen(buff),
                        testOptions->connection_flags, JSON_SINGLE_VALUE);
      return -3;
    }
    *s2cspd = atoi(buff);  // save Throughput value as seen by client
    log_println(6, "S2CSPD from client %f", *s2cspd);
    // Final activities of ending tests. Close sockets, file descriptors,
    //    send finalise message to client
    close(xmitsfd);
    if (send_json_message(ctlsockfd, TEST_FINALIZE, "", 0,
                          testOptions->connection_flags, JSON_SINGLE_VALUE) < 0)
      log_println(6,
                  "S2C test - failed to send finalize message to pid=%d",
                  s2c_childpid);

    if (getuid() == 0) {
      stop_packet_trace(mon_pipe);
    }

    // log end of test (generic and protocol logs)
    log_println(0, " <------------ %d ------------->", testOptions->child0);
    // log protocol validation logs
    teststatuses = TEST_ENDED;
    protolog_status(testOptions->child0, testids, teststatuses, ctlsockfd);

    setCurrentTest(TEST_NONE);
  }
  return 0;
}
