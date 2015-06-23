/**
 * This file contains methods to perform the C2S tests.
 * This test intends to measure throughput from the client to the
 * server by performing a 10 seconds memory-to-memory data transfer.
 *
 *  Created : Oct 2011
 *      Author: kkumar@internet2.edu
 */

#include <syslog.h>
#include <pthread.h>
#include <sys/times.h>
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

/**
 * Perform the C2S Throughput test. This test intends to measure throughput
 * from the client to the server by performing a 10 seconds memory-to-memory data transfer.
 *
 * Protocol messages are exchanged between the Client and the Server using the same
 * connection and message format as the NDTP-Control protocol.Throughput packets are
 * sent on the new connection and do not follow the NDTP-Control protocol message format.
 *
 * When the Client stops streaming the test data (or the server test routine times out),
 * the Server sends the Client its calculated throughput value.
 *
 * @param ctl Client control Connection
 * @param agent Web100 agent used to track the connection
 * @param testOptions Test options
 * @param conn_options connection options
 * @param c2sspd In-out parameter to store C2S throughput value
 * @param set_buff enable setting TCP send/recv buffer size to be used (seems unused in file)
 * @param window value of TCP send/rcv buffer size intended to be used.
 * @param autotune autotuning option. Deprecated.
 * @param device string device name inout parameter
 * @param options Test Option variables
 * @param record_reverse integer indicating whether receiver-side statistics have to be logged
 * @param count_vars count of web100 variables
 * @param spds[] [] speed check array
 * @param spd_index  index used for speed check array
 * @param conn_options Connection options
 * @param ctx The SSL context (possibly NULL)
 * @return 0 on success, an error code otherwise
 *         Error codes:
 *           -1 - Listener socket creation failed
 *           -100 - Timeout while waiting for client to connect to server's ephemeral port
 *           -101 - Retries exceeded while waiting for client to connect
 *           -102 - Retries exceeded while waiting for data from connected client
 *           -errno - Other specific socket error numbers
 */
int test_c2s(Connection* ctl, tcp_stat_agent* agent, TestOptions* testOptions,
             int conn_options, double* c2sspd, int set_buff, int window,
             int autotune, char* device, Options* options, int record_reverse,
             int count_vars, char spds[4][256], int* spd_index, SSL_CTX* ctx) {
  tcp_stat_connection conn;
  tcp_stat_group* group = NULL;
  /* The pipe that will return packet pair results */
  int mon_pipe[2];
  int recvsfd;  // receiver socket file descriptor
  pid_t c2s_childpid = 0;  // child process pids
  int msgretvalue, tmpbytecount;  // used during the "read"/"write" process
  int i, j;  // used as loop iterators

  struct sockaddr_storage cli_addr;
  Connection c2s_conn = {0, NULL};

  socklen_t clilen;
  char tmpstr[256];  // string array used for all sorts of temp storage purposes
  double tmptime;  // time indicator
  double bytes_read = 0;  // number of bytes read during the throughput tests
  struct timeval sel_tv;  // time
  fd_set rfd;  // receive file descriptor
  char buff[BUFFSIZE + 1];  // message "payload" buffer
  PortPair pair;  // socket ports
  I2Addr c2ssrv_addr = NULL;  // c2s test's server address
  // I2Addr src_addr=NULL;  // c2s test source address
  char listenc2sport[10];  // listening port
  pthread_t workerThreadId;

  // snap related variables
  SnapArgs snapArgs;
  snapArgs.snap = NULL;
#if USE_WEB100
  snapArgs.log = NULL;
#endif
  snapArgs.delay = options->snapDelay;
  wait_sig = 0;

  // Test ID and status descriptors
  enum TEST_ID testids = C2S;
  enum TEST_STATUS_INT teststatuses = TEST_NOT_STARTED;
  char namesuffix[256] = "c2s_snaplog";

  if (testOptions->c2sopt) {
    setCurrentTest(TEST_C2S);
    log_println(1, " <-- %d - C2S throughput test -->",
                testOptions->child0);
    strlcpy(listenc2sport, PORT2, sizeof(listenc2sport));

    // log protocol validation logs
    teststatuses = TEST_STARTED;
    protolog_status(testOptions->child0, testids, teststatuses, ctl->socket);

    // Determine port to be used. Compute based on options set earlier
    // by reading from config file, or use default port2 (3002).
    if (testOptions->c2ssockport) {
      snprintf(listenc2sport, sizeof(listenc2sport), "%d",
               testOptions->c2ssockport);
    } else if (testOptions->mainport) {
      snprintf(listenc2sport, sizeof(listenc2sport), "%d",
               testOptions->mainport + 1);
    }

    if (testOptions->multiple) {
      strlcpy(listenc2sport, "0", sizeof(listenc2sport));
    }

    // attempt to bind to a new port and obtain address structure with details
    // of listening port
    while (c2ssrv_addr == NULL) {
      c2ssrv_addr = CreateListenSocket(
          NULL,
          (testOptions->multiple ?
           mrange_next(listenc2sport, sizeof(listenc2sport)) : listenc2sport),
          conn_options, 0);
      if (strcmp(listenc2sport, "0") == 0) {
        log_println(0, "WARNING: ephemeral port number was bound");
        break;
      }
      if (testOptions->multiple == 0) {
        break;
      }
    }
    if (c2ssrv_addr == NULL) {
      log_println(
          0,
          "Server (C2S throughput test): CreateListenSocket failed: %s",
          strerror(errno));
      snprintf(buff, sizeof(buff),
               "Server (C2S throughput test): CreateListenSocket failed: %s",
               strerror(errno));
      send_json_message_any(ctl, MSG_ERROR, buff, strlen(buff),
                            testOptions->connection_flags, JSON_SINGLE_VALUE);
      return -1;
    }

    // get socket FD and the ephemeral port number that client will connect to
    // run tests
    testOptions->c2ssockfd = I2AddrFD(c2ssrv_addr);
    testOptions->c2ssockport = I2AddrPort(c2ssrv_addr);
    log_println(1, "  -- port: %d", testOptions->c2ssockport);
    pair.port1 = testOptions->c2ssockport;
    pair.port2 = -1;

    log_println(
        1,
        "listening for Inet connection on testOptions->c2ssockfd, fd=%d",
        testOptions->c2ssockfd);

    log_println(
        1,
        "Sending 'GO' signal, to tell client %d to head for the next test",
        testOptions->child0);
    snprintf(buff, sizeof(buff), "%d", testOptions->c2ssockport);

    // send TEST_PREPARE message with ephemeral port detail, indicating start
    // of tests
    if ((msgretvalue = send_json_message_any(
             ctl, TEST_PREPARE, buff, strlen(buff),
             testOptions->connection_flags, JSON_SINGLE_VALUE)) < 0) {
      log_println(2, "Child %d could not send details about ephemeral port",
                  getpid());
      return msgretvalue;
    }

    // Wait on listening socket and read data once ready.
    // Retry 5 times,  waiting for activity on the socket
    clilen = sizeof(cli_addr);
    log_println(6, "child %d - sent c2s prepare to client",
                testOptions->child0);
    FD_ZERO(&rfd);
    FD_SET(testOptions->c2ssockfd, &rfd);
    sel_tv.tv_sec = 5;
    sel_tv.tv_usec = 0;

    for (j = 0; j < RETRY_COUNT; j++) {
      msgretvalue = select((testOptions->c2ssockfd) + 1, &rfd, NULL, NULL,
                           &sel_tv);
      // socket interrupted. continue waiting for activity on socket
      if ((msgretvalue == -1) && (errno == EINTR))
        continue;
      if (msgretvalue == 0)  // timeout
        return -SOCKET_CONNECT_TIMEOUT;
      if (msgretvalue < 0)  // other socket errors. exit
        return -errno;
      if (j == (RETRY_COUNT - 1))  // retry exceeded. exit
        return -RETRY_EXCEEDED_WAITING_CONNECT;
 recfd:

      // If a valid connection request is received, client has connected.
      // The new connection (c2s_conn) should be used in the throughput test.
      c2s_conn.socket = accept(testOptions->c2ssockfd,
                               (struct sockaddr *)&cli_addr, &clilen);
      if (c2s_conn.socket > 0) {
        log_println(6, "accept() for %d completed", testOptions->child0);

        // log protocol validation indicating client accept
        protolog_procstatus(testOptions->child0, testids, CONNECT_TYPE,
                            PROCESS_STARTED, c2s_conn.socket);
        if (testOptions->connection_flags & TLS_SUPPORT) {
          errno = setup_SSL_connection(&c2s_conn, ctx);
          if (errno != 0) return -errno;
        }
        // To preserve user privacy, make sure that the HTTP header
        // processing is done prior to the start of packet capture.
        if (testOptions->connection_flags & WEBSOCKET_SUPPORT) {
          if (initialize_websocket_connection(&c2s_conn, 0, "c2s") != 0) {
            log_println(2, "Child %d failed to init websocket", getpid());
            shutdown_connection(&c2s_conn);
            return -EIO;
          }
        }
        break;
      }
      // socket interrupted, wait some more
      if ((c2s_conn.socket == -1) && (errno == EINTR)) {
        log_println(
            6,
            "Child %d interrupted while waiting for accept() to complete",
            testOptions->child0);
        goto recfd;
      }
      log_println(
          6,
          "-------     C2S connection setup for %d returned because (%d)",
          testOptions->child0, errno);
      if (c2s_conn.socket < 0) {  // other socket errors, quit
        return -errno;
      }
      if (j == (RETRY_COUNT - 1)) {  // retry exceeded, quit
        log_println(
            6,
            "c2s child %d, uable to open connection, return from test",
            testOptions->child0);
        return -RETRY_EXCEEDED_WAITING_DATA;
      }
    }

    // Get address associated with the throughput test. Used for packet tracing
    log_println(6, "child %d - c2s ready for test with fd=%d",
                testOptions->child0, c2s_conn.socket);

    // commenting out below to move to init_pkttrace function
    I2Addr src_addr = I2AddrByLocalSockFD(get_errhandle(), c2s_conn.socket, 0);

    // Get tcp_stat connection. Used to collect tcp_stat variable statistics
    conn = tcp_stat_connection_from_socket(agent, c2s_conn.socket);

    // set up packet tracing. Collected data is used for bottleneck link
    // calculations
    if (getuid() == 0) {
      /*
         pipe(mon_pipe1);
         log_println(0, "%s test calling pkt_trace_start() with pd=%d for device %s, addr %s",
         currenttestdesc, clilen, device , src_addr);

         start_packet_trace(recvsfd, testOptions->c2ssockfd, &c2s_childpid,
         mon_pipe1, (struct sockaddr *) &cli_addr, clilen, device,
         &pair, currenttestdesc, options->compress, meta.c2s_ndttrace);
         */

      if (pipe(mon_pipe) != 0) {
        log_println(0, "C2S test error: can't create pipe.");
      } else {
        if ((c2s_childpid = fork()) == 0) {
          close(testOptions->c2ssockfd);
          close_connection(&c2s_conn);
          log_println(
              5,
              "C2S test Child %d thinks pipe() returned fd0=%d, fd1=%d",
              testOptions->child0, mon_pipe[0], mon_pipe[1]);
          log_println(2, "C2S test calling init_pkttrace() with pd=%p",
                      &cli_addr);
          init_pkttrace(src_addr, (struct sockaddr *) &cli_addr, clilen,
                        mon_pipe, device, &pair, "c2s", options->compress);
          log_println(1, "c2s is exiting gracefully");
          /* Close the pipe */
          close(mon_pipe[0]);
          close(mon_pipe[1]);
          exit(0); /* Packet trace finished, terminate gracefully */
        } else if (c2s_childpid < 0) {
          log_println(0, "C2S test error: can't create child process.");
        }
      }

      // Get data collected from packet tracing into the C2S "ndttrace" file
      memset(tmpstr, 0, 256);
      for (i = 0; i < 5; i++) {
        msgretvalue = read(mon_pipe[0], tmpstr, 128);
        if ((msgretvalue == -1) && (errno == EINTR))
          continue;
        break;
      }

      if (strlen(tmpstr) > 5)
        memcpy(meta.c2s_ndttrace, tmpstr, strlen(tmpstr));
      // name of nettrace file passed back from pcap child
      log_println(3, "--tracefile after packet_trace %s",
                  meta.c2s_ndttrace);
    }

    log_println(5, "C2S test Parent thinks pipe() returned fd0=%d, fd1=%d",
                mon_pipe[0], mon_pipe[1]);

    // experimental code, delete when finished
    setCwndlimit(conn, group, agent, options);

    // Create C->S snaplog directories, and perform some initialization based on
    // options
    create_client_logdir((struct sockaddr *) &cli_addr, clilen,
                         options->c2s_logname, sizeof(options->c2s_logname),
                         namesuffix, sizeof(namesuffix));
    // At 150k tests per day, this one sleep(2) wastes 83 hours of peoples'
    // lives every day.
    // TODO: solve the race conditions some other way.
    sleep(2);

    // send empty TEST_START indicating start of the test
    send_json_message_any(ctl, TEST_START, "", 0, testOptions->connection_flags,
                          JSON_SINGLE_VALUE);
    /* alarm(30); */  // reset alarm() again, this 10 sec test should finish
                      // before this signal is generated.

    // If snaplog recording is enabled, update meta file to indicate the same
    // and proceed to get snapshot and log it.
    // This block is needed here since the meta file stores names without the
    // full directory but fopen needs full path. Else, it could have gone into
    // the "start_snap_worker" method
    if (options->snaplog) {
      memcpy(meta.c2s_snaplog, namesuffix, strlen(namesuffix));
      /*start_snap_worker(&snapArgs, agent, options->snaplog, &workerLoop,
        &workerThreadId, meta.c2s_snaplog, options->c2s_logname,
        conn, group); */
    }
    start_snap_worker(&snapArgs, agent, NULL, options->snaplog, &workerThreadId,
                      meta.c2s_snaplog, options->c2s_logname, conn, group);
    // Wait on listening socket and read data once ready.
    tmptime = secs();
    sel_tv.tv_sec = 11;  // time out after 11 seconds
    sel_tv.tv_usec = 0;
    FD_ZERO(&rfd);
    FD_SET(c2s_conn.socket, &rfd);
    for (;;) {
      msgretvalue = select(c2s_conn.socket + 1, &rfd, NULL, NULL, &sel_tv);
      if ((msgretvalue == -1) && (errno == EINTR)) {
        // socket interrupted. Continue waiting for activity on socket
        continue;
      }
      if (msgretvalue > 0) {
	// Use read or SSL_read in their raw forms.  We want this to go as fast
	// as possible and we do not care about the contents of buff.
        if (c2s_conn.ssl != NULL) {
          tmpbytecount = SSL_read(c2s_conn.ssl, buff, sizeof(buff));
        } else {
          tmpbytecount = read(c2s_conn.socket, buff, sizeof(buff));
        }
        // read interrupted, continue waiting
        if ((tmpbytecount == -1) && (errno == EINTR))
          continue;
        if (tmpbytecount == 0)  // all data has been read
          break;
        bytes_read += tmpbytecount;  // data byte count has to be increased
        continue;
      }
      break;
    }

    tmptime = secs() - tmptime;
    //  throughput in kilo bits per sec =
    //  (transmitted_byte_count * 8) / (time_duration)*(1000)
    *c2sspd = (8.e-3 * bytes_read) / tmptime;

    // c->s throuput value calculated and assigned ! Release resources, conclude
    // snap writing.
    stop_snap_worker(&workerThreadId, options->snaplog, &snapArgs);

    // send the server calculated value of C->S throughput as result to client
    snprintf(buff, sizeof(buff), "%6.0f kbps outbound for child %d", *c2sspd,
             testOptions->child0);
    log_println(1, "%s", buff);
    snprintf(buff, sizeof(buff), "%0.0f", *c2sspd);
    send_json_message_any(ctl, TEST_MSG, buff, strlen(buff),
                          testOptions->connection_flags, JSON_SINGLE_VALUE);

    // get receiver side Web100 stats and write them to the log file. close
    // sockets
    if (record_reverse == 1)
      tcp_stat_get_data_recv(c2s_conn.socket, agent, conn, count_vars);

    shutdown_connection(&c2s_conn);
    close(testOptions->c2ssockfd);

    // Next, send speed-chk a flag to retrieve the data it collected.
    // Skip this step if speed-chk isn't running.

    if (getuid() == 0) {
      log_println(1, "Signal USR1(%d) sent to child [%d]", SIGUSR1,
                  c2s_childpid);
      testOptions->child1 = c2s_childpid;
      kill(c2s_childpid, SIGUSR1);
      FD_ZERO(&rfd);
      FD_SET(mon_pipe[0], &rfd);
      sel_tv.tv_sec = 1;
      sel_tv.tv_usec = 100000;
      i = 0;

      for (;;) {
        msgretvalue = select(mon_pipe[0] + 1, &rfd, NULL, NULL,
                             &sel_tv);
        if ((msgretvalue == -1) && (errno == EINTR))
          continue;
        if (((msgretvalue == -1) && (errno != EINTR))
            || (msgretvalue == 0)) {
          log_println(4, "Failed to read pkt-pair data from C2S flow, "
                      "retcode=%d, reason=%d", msgretvalue, errno);
          snprintf(spds[(*spd_index)++],
                   sizeof(spds[*spd_index]),
                   " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0 -1");
          snprintf(spds[(*spd_index)++],
                   sizeof(spds[*spd_index]),
                   " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0 -1");
          break;
        }
        /* There is something to read, so get it from the pktpair child.  If an interrupt occurs,
         * just skip the read and go on
         * RAC 2/8/10
         */
        if (msgretvalue > 0) {
          if ((msgretvalue = read(mon_pipe[0], spds[*spd_index],
                                  sizeof(spds[*spd_index]))) < 0) {
            snprintf(
                spds[*spd_index],
                sizeof(spds[*spd_index]),
                " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0 -1");
          }
          log_println(1, "%d bytes read '%s' from C2S monitor pipe",
                      msgretvalue, spds[*spd_index]);
          (*spd_index)++;
          if (i++ == 1)
            break;
          sel_tv.tv_sec = 1;
          sel_tv.tv_usec = 100000;
          continue;
        }
      }
    }

    // An empty TEST_FINALIZE message is sent to conclude the test
    send_json_message_any(ctl, TEST_FINALIZE, "", 0,
                          testOptions->connection_flags, JSON_SINGLE_VALUE);

    //  Close opened resources for packet capture
    if (getuid() == 0) {
      stop_packet_trace(mon_pipe);
    }

    // log end of C->S test
    log_println(1, " <----------- %d -------------->", testOptions->child0);
    // protocol logs
    teststatuses = TEST_ENDED;
    protolog_status(testOptions->child0, testids, teststatuses, ctl->socket);

    // set current test status and free address
    setCurrentTest(TEST_NONE);
  }

  return 0;
}
