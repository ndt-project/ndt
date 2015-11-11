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

typedef struct s2cWriteWorkerArgs {
  int connectionId;
  Connection* connection;
  double stopTime;
  char* buff;
} S2CWriteWorkerArgs;

typedef struct s2cServerStream {
#if USE_WEB100
  /* experimental code to capture and log multiple copies of the
   * web100 variables using the web100_snap() & log() functions.
   */

  web100_group* tgroup;
  web100_group* rgroup;
#endif
  tcp_stat_connection conn;
  S2CWriteWorkerArgs writeWorkerArgs;
  pthread_t writeWorkerIds;
  SnapArgs snapArgs;
  pthread_t workerThreadId;
} S2CServerStream;

void* s2cWriteWorker(void* arg);

const char RESULTS_KEYS[] = "ThroughputValue UnsentDataAmount TotalSentByte";

/**
 * Use write or SSL_write() in their raw forms for maximum speed.
 * @param conn The Connection to use
 * @param buff The data to send
 * @param buff_len The length of the data
 * @return The number of bytes written or a negative failure code
 */
int raw_write(Connection *conn, char *buff, int buff_len) {
  if (conn->ssl != NULL) {
    return SSL_write(conn->ssl, buff, buff_len);
  } else {
    return write(conn->socket, buff, buff_len);
  }
}

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
 * @param ctl - the client control Connection
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
 * @param s2c_ThroughputSnapshots Variable used to set s2c throughput snapshots
 * @param extended indicates if extended s2c test should be performed
 *
 * @return 0 on success, error code otherwise.
 *     Error codes:
 *         -1     - Message reception errors/inconsistencies in client's final
 *                  message, or Listener socket creation failed or cannot write
 *                  message header information while attempting to send
 *                  TEST_PREPARE message
 *         -2     - Cannot write message data while attempting to send
 *                  TEST_PREPARE message, or Unexpected message type received
 *         -3     - Received message is invalid
 *         -4     - Unable to create worker threads
 *         -100   - timeout while waiting for client to connect to server's
 *                  ephemeral port
 *         -101   - Retries exceeded while waiting for client to connect
 *         -102   - Retries exceeded while waiting for data from connected client
 *         -errno - Other specific socket error numbers
 */
int test_s2c(Connection *ctl, tcp_stat_agent *agent, TestOptions *testOptions,
             int conn_options, double *s2cspd, int set_buff, int window,
             int autotune, char* device, Options *options, char spds[4][256],
             int *spd_index, int count_vars, CwndPeaks *peaks, SSL_CTX *ctx,
             struct throughputSnapshot **s2c_ThroughputSnapshots, int extended) {
#if USE_WEB100
  web100_snapshot* tsnap[MAX_STREAMS];
  web100_snapshot* rsnap[MAX_STREAMS];
  web100_var* var;
#elif USE_WEB10G
  estats_val_data* snap[MAX_STREAMS];
#endif
  /* Just a holder for web10g */
  tcp_stat_group* group = NULL;
  /* Pipe that handles returning packet pair timing */
  int mon_pipe[2];
  Connection xmitsfd[MAX_STREAMS];
  int ret;  // ctrl protocol read/write return status
  int j, k, n;
  int streamsNum = 1;
  int stream, attempts;
  pid_t s2c_childpid = 0;  // s2c_childpid

  char tmpstr[256];  // string array used for temp storage of many char*
  struct sockaddr_storage cli_addr[MAX_STREAMS];
  struct throughputSnapshot *lastThroughputSnapshot;

  socklen_t clilen;
  double bytes_written;  // bytes written in the throughput test
  double tx_duration;  // total time for which data was txed
  double tmptime;  // temporary time store
  double testDuration = 10; // default test duration
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
  char *jsonMsgValue, *tempStr;

  int nextseqtosend = 0, lastunackedseq = 0;
  int drainingqueuecount = 0, bufctlrnewdata = 0;

  S2CServerStream streams[MAX_STREAMS];

  // variables used for protocol validation logs
  enum TEST_STATUS_INT teststatuses = TEST_NOT_STARTED;
  enum TEST_ID testids = extended ? S2C_EXT : S2C;
  char snaplogsuffix[256] = "s2c_snaplog";

  for (i = 0; i < MAX_STREAMS; i++) {
    streams[i].snapArgs.snap = NULL;
#if USE_WEB100
    tsnap[i] = NULL;
    rsnap[i] = NULL;
    streams[i].snapArgs.log = NULL;
#endif
    streams[i].snapArgs.delay = options->snapDelay;
  }
  wait_sig = 0;

  log_println(1, "test client version: %s", testOptions->client_version);

  // Determine port to be used. Compute based on options set earlier
  // by reading from config file, or use default port2 (3003)
  if ((!extended && testOptions->s2copt) || (extended && testOptions->s2cextopt)) {
    if (extended)
      setCurrentTest(TEST_S2C_EXT);
    else
      setCurrentTest(TEST_S2C);
    log_println(1, " <-- %d - S2C throughput test -->",
                testOptions->child0);

    // protocol logs
    teststatuses = TEST_STARTED;
    protolog_status(testOptions->child0, testids, teststatuses, ctl->socket);

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
        log_println(1, "WARNING: ephemeral port number was bound");
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
      send_json_message_any(ctl, MSG_ERROR, buff, strlen(buff),
                            testOptions->connection_flags, JSON_SINGLE_VALUE);
      return -1;
    }

    // get socket FD and the ephemeral port number that client will connect to
    // run tests
    testOptions->s2csockfd = I2AddrFD(s2csrv_addr);
    testOptions->s2csockport = I2AddrPort(s2csrv_addr);
    log_println(1, "  -- s2c %d port: %d", testOptions->child0, testOptions->s2csockport);
    if (extended) {
      log_println(1, "  -- s2c ext -- duration = %d", options->s2c_duration);
      log_println(1, "  -- s2c ext -- throughput snapshots: enabled = %s, delay = %d, offset = %d",
                          options->s2c_throughputsnaps ? "true" : "false", options->s2c_snapsdelay, options->s2c_snapsoffset);
      log_println(1, "  -- s2c ext -- number of streams: %d", options->s2c_streamsnum);
    }
    pair.port1 = -1;
    pair.port2 = testOptions->s2csockport;

    // Data received from speed-chk. Send TEST_PREPARE "GO" signal with port
    // number
    snprintf(buff, sizeof(buff), "%d", testOptions->s2csockport);
    if (extended) {
      snprintf(buff, sizeof(buff), "%d %d %d %d %d %d", testOptions->s2csockport,
               options->s2c_duration, options->s2c_throughputsnaps,
               options->s2c_snapsdelay, options->s2c_snapsoffset, options->s2c_streamsnum);
      lastThroughputSnapshot = NULL;
    }
    j = send_json_message_any(ctl, TEST_PREPARE, buff, strlen(buff),
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
    if (extended) {
      streamsNum = options->s2c_streamsnum;
      testDuration = options->s2c_duration / 1000.0;
    }

    stream = 0;
    for (attempts = 0;
         attempts < RETRY_COUNT * streamsNum && stream < streamsNum;
         attempts++) {
      ret = select((testOptions->s2csockfd) + 1, &rfd, NULL, NULL,
                   &sel_tv);
      if ((ret == -1) && (errno == EINTR))
        continue;
      if (ret == 0)
        return SOCKET_CONNECT_TIMEOUT;  // timeout
      if (ret < 0)
        return -errno;  // other socket errors. exit

      // If a valid connection request is received, client has connected.
      // Proceed.
      // Note the new connection - xmitsfd - used in the throughput test
      xmitsfd[stream].socket = accept(testOptions->s2csockfd, (struct sockaddr *) &cli_addr[stream], &clilen);
      if (xmitsfd[stream].socket > 0) {
        log_println(6, "accept(%d/%d) for %d completed", stream, streamsNum, testOptions->child0);
        if (testOptions->connection_flags & TLS_SUPPORT) {
          errno = setup_SSL_connection(&xmitsfd[stream], ctx);
          if (errno != 0) return -errno;
        }
        if (testOptions->connection_flags & WEBSOCKET_SUPPORT) {
          // To preserve user privacy, make sure that the HTTP header
          // processing is done prior to the start of packet capture, as many
          // browsers have headers that uniquely identify a single user.
          if (initialize_websocket_connection(&xmitsfd[stream], 0, "s2c") != 0) {
            close_connection(&xmitsfd[stream]);
          }
        }
        stream++;
      } else {
        // socket interrupted, wait some more
        if ((xmitsfd[stream].socket == -1) && (errno == EINTR)) {
          log_println(
              6,
              "Child %d interrupted while waiting for accept() to complete",
              testOptions->child0);
          continue;
        }
        log_println(
            6,
            "-------     S2C connection setup for %d returned because (%d)",
            testOptions->child0, errno);
        if (xmitsfd[stream].socket < 0)   // other socket errors, quit
          return -errno;
      }
    }
    // If we didn't make enough streams then we can't run the test.
    if (stream != streamsNum) {
      // Loop exited without creating all the necessary streams
      log_println(
            6,
            "s2c child %d, unable to open connection, return from test",
            testOptions->child0);
      return RETRY_EXCEEDED_WAITING_CONNECT;  // retry exceeded. exit
    }
    protolog_procstatus(testOptions->child0, testids, CONNECT_TYPE,
                        PROCESS_STARTED, xmitsfd[0].socket);
    src_addr = I2AddrByLocalSockFD(get_errhandle(), xmitsfd[0].socket, 0);
    for (i = 0; i < streamsNum; ++i) {
      streams[i].conn = tcp_stat_connection_from_socket(agent, xmitsfd[i].socket);
    }

    // set up packet capture. The data collected is used for bottleneck link
    // calculations
    if (xmitsfd[0].socket > 0) {
      log_println(6, "S2C child %d, ready to fork()",
                  testOptions->child0);
      if (getuid() == 0) {
        if (pipe(mon_pipe) != 0) {
          log_println(0, "S2C test error: can't create pipe.");
        } else {
          if ((s2c_childpid = fork()) == 0) {
            close(testOptions->s2csockfd);
            for (i = 0; i < streamsNum; i++) {
              close(xmitsfd[i].socket);
            }
            log_println(
                5,
                "S2C test Child thinks pipe() returned fd0=%d, fd1=%d",
                mon_pipe[0], mon_pipe[1]);
            log_println(2, "S2C test calling init_pkttrace() with pd=%p",
                        &cli_addr[0]);
            init_pkttrace(src_addr, cli_addr, streamsNum,
                          clilen, mon_pipe, device, &pair, "s2c",
                          options->s2c_duration / 1000.0);
            log_println(6,
                        "S2C test ended, why is timer still running?");
            /* Close the pipe */
            close(mon_pipe[0]);
            close(mon_pipe[1]);
            exit(0); /* Packet trace finished, terminate gracefully */
          } else if (s2c_childpid < 0) {
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
      setCwndlimit(streams[0].conn, group, agent, options);
      /* End of test code */

      // create directory to write web100 snaplog trace
      create_client_logdir((struct sockaddr *) &cli_addr[0], clilen,
                           options->s2c_logname[0], sizeof(options->s2c_logname[0]),
                           snaplogsuffix, sizeof(snaplogsuffix));

      for (i = 1; i < streamsNum; i++) {
        tempStr = strrchr(options->s2c_logname[0], '.');
        snprintf(options->s2c_logname[i], strlen(options->s2c_logname[0]) - strlen(tempStr) + 1, "%s", options->s2c_logname[0]);
        snprintf(&options->s2c_logname[i][strlen(options->s2c_logname[i])], sizeof(options->s2c_logname[i])-strlen(options->s2c_logname[i]),
                 "_%d.s2c_snaplog", i);
      }

      /* Kludge way of nuking Linux route cache.  This should be done
       * using the sysctl interface.
       */
      if (getuid() == 0) {
        // system("/sbin/sysctl -w net.ipv4.route.flush=1");
        system("echo 1 > /proc/sys/net/ipv4/route/flush");
      }
      for (i = 0; i < streamsNum; ++i) {
#if USE_WEB100
        streams[i].rgroup = web100_group_find(agent, "read");
        rsnap[i] = web100_snapshot_alloc(streams[i].rgroup, streams[i].conn);
        streams[i].tgroup = web100_group_find(agent, "tune");
        tsnap[i] = web100_snapshot_alloc(streams[i].tgroup, streams[i].conn);
#elif USE_WEB10G
        estats_val_data_new(&snap[i]);
#endif
      }

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
      if (send_json_message_any(ctl, TEST_START, "", 0, testOptions->connection_flags,
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
      if (options->snapshots && options->snaplog) {
        for (i = 0; i < streamsNum; i++) {
          tempStr = strrchr(options->s2c_logname[i], '/');
          memcpy(meta.s2c_snaplog[i], tempStr+1, strlen(tempStr));
        }
      }
      // get web100 snapshot and also log it based on options
      /*start_snap_worker(&snapArgs, agent, options->snaplog, &workerLoop,
        &workerThreadId, meta.s2c_snaplog, options->s2c_logname,
        conn, group);*///new file changes
      if (options->snapshots) {
        for (i = 0; i < streamsNum; ++i) {
            start_snap_worker(&streams[i].snapArgs, agent, peaks, options->snaplog,
                              &streams[i].workerThreadId, options->s2c_logname[i],
                              streams[i].conn, group);
        }
      }
      /* alarm(20); */
      tmptime = secs();  // current time
      tx_duration = tmptime + testDuration;  // set timeout to test duration s in future

      for (i = 0; i < streamsNum; ++i) {
        streams[i].writeWorkerArgs.connectionId = i + 1;
        streams[i].writeWorkerArgs.connection = &xmitsfd[i];
        streams[i].writeWorkerArgs.stopTime = tx_duration;
        streams[i].writeWorkerArgs.buff = buff;
      }


      log_println(6, "S2C child %d beginning test", testOptions->child0);

      if (streamsNum == 1) {
        while (secs() < tx_duration) {
          // Increment total attempts at sending-> buffer control
          bufctrlattempts++;
          if (options->avoidSndBlockUp) {  // Do not block send buffers
            pthread_mutex_lock(&mainmutex);

            // get details of next sequence # to be sent and fetch value from snap file
#if USE_WEB100
            web100_agent_find_var_and_group(agent, "SndNxt", &group, &var);
            web100_snap_read(var, streams[0].snapArgs.snap, tmpstr);
            nextseqtosend = atoi(web100_value_to_text(web100_get_var_type(var), tmpstr));
            // get oldest un-acked sequence number
            web100_agent_find_var_and_group(agent, "SndUna", &group, &var);
            web100_snap_read(var, streams[0].snapArgs.snap, tmpstr);
            lastunackedseq = atoi(web100_value_to_text(web100_get_var_type(var), tmpstr));
#elif USE_WEB10G
            struct estats_val value;
            web10g_find_val(streams[0].snapArgs.snap, "SndNxt", &value);
            nextseqtosend = value.uv32;
            web10g_find_val(streams[0].snapArgs.snap, "SndUna", &value);
            lastunackedseq = value.uv32;
#endif
            pthread_mutex_unlock(&mainmutex);

            // Temporarily stop sending data if you sense that the buffer is overwhelmed
            // This is calculated by checking if (8192 * 4) < ((Next Sequence Number To Be Sent) - (Oldest Unacknowledged Sequence Number) - 1)
            if (is_buffer_clogged(nextseqtosend, lastunackedseq)) {
              // Increment draining queue value
              drainingqueuecount++;
              continue;
            }
          }

          n = raw_write(&xmitsfd[0], buff, RECLTH);
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
      }
      else {
        for (i = 0; i < streamsNum; ++i) {
          if (pthread_create(&streams[i].writeWorkerIds, NULL, s2cWriteWorker, (void*) &streams[i].writeWorkerArgs)) {
            log_println(0, "Cannot create write worker thread for throughput download test!");
            streams[i].writeWorkerIds = 0;
            return -4;
          }
        }

        for (i = 0; i < streamsNum; ++i) {
          pthread_join(streams[i].writeWorkerIds, NULL);
        }
      }

      /* alarm(10); */
      sigaction(SIGALRM, &old, NULL);
      sndqueue = sndq_len(xmitsfd[0].socket);

      // finalize the midbox test ; disabling socket used for throughput test
      log_println(6, "S2C child %d finished test", testOptions->child0);
      for (i = 0; i < streamsNum; i++) {
        shutdown_connection(&xmitsfd[i]);
      }

      // get actual time duration during which data was transmitted
      tx_duration = secs() - tmptime;

      // Throughput in kbps =
      // (no of bits sent * 8) / (1000 * time data was sent)
      x2cspd = (8.e-3 * bytes_written) / tx_duration;

      // Release semaphore, and close snaplog file.  finalize other data
      if (options->snapshots) {
        for (i = 0; i < streamsNum; i++) {
          stop_snap_worker(&streams[i].workerThreadId, options->snaplog, &streams[i].snapArgs);
        }
      }

      // send the x2cspd to the client
      memset(buff, 0, sizeof(buff));

      // Send throughput, unsent byte count, total sent byte count to client
      snprintf(buff, sizeof(buff), "%0.0f %d %0.0f", x2cspd, sndqueue,
               bytes_written);
      if (testOptions->connection_flags & JSON_SUPPORT) {
        if (send_json_msg_any(ctl, TEST_MSG, buff, strlen(buff), testOptions->connection_flags,
                              JSON_MULTIPLE_VALUES, RESULTS_KEYS, " ", buff, " ") < 0)
            log_println(6,
                "S2C test - failed to send test message to pid=%d",
                s2c_childpid);
      }
      else {
        if (send_json_message_any(ctl, TEST_MSG, buff, strlen(buff),
                                  testOptions->connection_flags, JSON_SINGLE_VALUE) < 0)
          log_println(6,
              "S2C test - failed to send test message to pid=%d",
              s2c_childpid);
      }

      for (i = 0; i < streamsNum; ++i) {
#if USE_WEB100
        web100_snap(rsnap[i]);
        web100_snap(tsnap[i]);
#elif USE_WEB10G
        estats_read_vars(snap[i], streams[i].conn, agent);
#endif
      }

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
    ret = tcp_stat_get_data(tsnap, xmitsfd, streamsNum, ctl, agent, count_vars, testOptions);
     for (i = 0; i < streamsNum; ++i) {
      web100_snapshot_free(tsnap[i]);
    }
    // send tuning-related web100 data collected to client
    ret = tcp_stat_get_data(rsnap, xmitsfd, streamsNum, ctl, agent, count_vars, testOptions);
    for (i = 0; i < streamsNum; ++i) {
      web100_snapshot_free(rsnap[i]);
    }
#elif USE_WEB10G
    ret = tcp_stat_get_data(snap, xmitsfd, streamsNum, ctl, agent, count_vars, testOptions);
    for (i = 0; i < streamsNum; ++i) {
      estats_val_data_free(&snap[i]);
    }
#endif

    // If sending web100 variables above failed, indicate to client
    if (ret < 0) {
      log_println(6, "S2C - No web100 data received for pid=%d",
                  s2c_childpid);
      snprintf(buff, sizeof(buff), "No Data Collected: 000000");
      send_json_message_any(ctl, TEST_MSG, buff, strlen(buff), testOptions->connection_flags,
                            JSON_SINGLE_VALUE);
    }

    // Wait for message from client. Client sends its calculated throughput
    // value
    log_println(6, "S2CSPD reception starts");
    msgLen = sizeof(buff);
    if (recv_any_msg(ctl, &msgType, buff, &msgLen, testOptions->connection_flags)) {
      log_println(0, "Protocol error!");
      snprintf(
          buff,
          sizeof(buff),
          "Server (S2C throughput test): Invalid S2C throughput received");
      send_json_message_any(ctl, MSG_ERROR, buff, strlen(buff),
                        testOptions->connection_flags, JSON_SINGLE_VALUE);
      return -1;
    }
    if (check_msg_type("S2C throughput test", TEST_MSG, msgType, buff,
                       msgLen)) {
      snprintf(
          buff,
          sizeof(buff),
          "Server (S2C throughput test): Invalid S2C throughput received");
      send_json_message_any(ctl, MSG_ERROR, buff, strlen(buff),
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
      send_json_message_any(ctl, MSG_ERROR, buff, strlen(buff),
                            testOptions->connection_flags, JSON_SINGLE_VALUE);
      return -3;
    }
    *s2cspd = atoi(buff);  // save Throughput value as seen by client
    if (extended && options->s2c_throughputsnaps) {
      char* strtokptr = strtok(buff, " ");
      while ((strtokptr = strtok(NULL, " ")) != NULL) {
        if (lastThroughputSnapshot != NULL) {
          lastThroughputSnapshot->next = (struct throughputSnapshot*) malloc(sizeof(struct throughputSnapshot));
          lastThroughputSnapshot = lastThroughputSnapshot->next;
        }
        else {
          *s2c_ThroughputSnapshots = lastThroughputSnapshot = (struct throughputSnapshot*) malloc(sizeof(struct throughputSnapshot));
        }
        lastThroughputSnapshot->next = NULL;
        lastThroughputSnapshot->time = atof(strtokptr);
        strtokptr = strtok(NULL, " ");
        lastThroughputSnapshot->throughput = atof(strtokptr);
      }
    }
    log_println(6, "S2CSPD from client %f", *s2cspd);
    // Final activities of ending tests.
    if (send_json_message_any(ctl, TEST_FINALIZE, "", 0,
                              testOptions->connection_flags, JSON_SINGLE_VALUE) < 0)
      log_println(6,
                  "S2C test - failed to send finalize message to pid=%d",
                  s2c_childpid);

    if (getuid() == 0) {
      stop_packet_trace(mon_pipe);
    }

    // log end of test (generic and protocol logs)
    log_println(1, " <------------ %d ------------->", testOptions->child0);
    // log protocol validation logs
    teststatuses = TEST_ENDED;
    protolog_status(testOptions->child0, testids, teststatuses, ctl->socket);

    setCurrentTest(TEST_NONE);
  }
  return 0;
}

void* s2cWriteWorker(void* arg) {
  S2CWriteWorkerArgs *workerArgs = (S2CWriteWorkerArgs*) arg;
  int connectionId = workerArgs->connectionId;
  Connection* conn = workerArgs->connection;
  double stopTime = workerArgs->stopTime;
  char* threadBuff = workerArgs->buff;
  double threadBytes = 0;
  int threadPackets = 0, n;
  double threadTime = secs();


  while (secs() < stopTime) {
    // attempt to write random data into the client socket
    n = raw_write(conn, threadBuff, RECLTH); // TODO avoid snd block
    // socket interrupted, continue attempting to write
    if ((n == -1) && (errno == EINTR))
      continue;
    if (n < 0)
      break;  // all data written. Exit
    threadPackets++;
    threadBytes += n;
  }

  log_println(6, " ---S->C thread %d (sc %d): speed=%0.0f, bytes=%0.0f, pkts=%d, lth=%d, time=%0.0f", connectionId, conn->socket,
                 ((BITS_8_FLOAT * threadBytes) / KILO) / (secs() - threadTime), threadBytes, threadPackets, RECLTH, secs() - threadTime);

  return NULL;
}
