/**
 * This file contains methods to perform the middlebox test.
 * The middlebox test is a brief throughput test from the Server to the Client
 * with a limited CWND to check for a duplex mismatch condition.
 * This test also uses a pre-defined MSS value to check if any intermediate node
 * is modifying connection settings.
 *
 *  Created on: Oct 18, 2011
 *      Author: kkumar
 */

#include  <syslog.h>
#include <pthread.h>
#include <sys/times.h>
#include <assert.h>

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

/**
 * Perform the Middlebox test.
 *
 * @param ctlsockfd Client control socket descriptor
 * @param agent Web100 agent used to track the connection
 * @param options  Test options
 * @param s2c_throughput_mid In-out parameter for S2C throughput results (evaluated by the MID TEST),
 * @param conn_options Connection options
 * @return 0 - success,
 *          >0 - error code.
 *          Error codes:
 * 				-1 - Listener socket creation failed
 *				-3 - web100 connection data not obtained
 *				-100 - timeout while waiting for client to connect to serverÕs ephemeral port
 *				-errno- Other specific socket error numbers
 *				-101 - Retries exceeded while waiting for client to connect
 *				-102 - Retries exceeded while waiting for data from connected client
 *			Other used return codes:
 *				1 - Message reception errors/inconsistencies
 *				2 - Unexpected message type received/no message received due to timeout
 *				3 Ð Received message is invalid
 *
 */

int test_mid(int ctlsockfd, web100_agent* agent, TestOptions* options,
             int conn_options, double* s2c_throughput_mid) {
  int maxseg = ETHERNET_MTU_SIZE;
  /* int maxseg=1456, largewin=16*1024*1024; */
  /* int seg_size, win_size; */
  int midsfd; // socket file-descriptor, used in mid-box throughput test from S->C
  int j; // temporary integer store
  int msgretvalue; // return value from socket read/writes
  struct sockaddr_storage cli_addr;
  /* socklen_t optlen, clilen; */
  socklen_t clilen;
  char buff[BUFFSIZE + 1]; // buf used for message payload
  I2Addr 	midsrv_addr = NULL; // server address
  char listenmidport[10]; // listener socket for middlebox tests
  int msgType;
  int msgLen;
  web100_connection* conn;
  char tmpstr[256]; // temporary string storage
  struct timeval sel_tv; // time
  fd_set rfd; // receiver file descriptor

  // variables used for protocol validation logging
  enum TEST_ID thistestId = NONE;
  enum TEST_STATUS_INT teststatusnow = TEST_NOT_STARTED;
  enum PROCESS_STATUS_INT procstatusenum = PROCESS_STARTED;
  enum PROCESS_TYPE_INT proctypeenum = CONNECT_TYPE;

  assert(ctlsockfd != -1);
  assert(agent);
  assert(options);
  assert(s2c_throughput_mid);

  if (options->midopt) { // middlebox tests need to be run.

    // Start with readying up (initializing)
    setCurrentTest(TEST_MID);
    log_println(1, " <-- %d - Middlebox test -->", options->child0);

    // protocol validation logs indicating start of tests
    thistestId = MIDDLEBOX;
    teststatusnow = TEST_STARTED;
    protolog_status(options->child0, thistestId, teststatusnow, ctlsockfd);

    // determine port to be used. Compute based on options set earlier
    // by reading from config file, or use default port3 (3003),
    //strcpy(listenmidport, PORT3);
    strlcpy(listenmidport, PORT3, sizeof(listenmidport));

    if (options->midsockport) {
      snprintf(listenmidport, sizeof(listenmidport), "%d", options->midsockport);
    } else if (options->mainport) {
      snprintf(listenmidport, sizeof(listenmidport), "%d", options->mainport + 2);
    }

    if (options->multiple) {
      //strcpy(listenmidport, "0");
      strlcpy(listenmidport, "0", sizeof(listenmidport));
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

      midsrv_addr =
          CreateListenSocket(
              NULL,
              (options->multiple ?
               mrange_next(listenmidport, sizeof(listenmidport)) : listenmidport), conn_options, 0)
          ;
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
      if (options->multiple == 0) { // simultaneous tests from multiple clients not allowed, quit now
        break;
      }
    }

    if (midsrv_addr == NULL) {
      log_println(0,
                  "Server (Middlebox test): CreateListenSocket failed: %s",
                  strerror(errno));
      snprintf(buff, sizeof(buff),
               "Server (Middlebox test): CreateListenSocket failed: %s",
               strerror(errno));
      send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
      return -1;
    }

    // get socket FD and the ephemeral port number that client will connect to
    options->midsockfd = I2AddrFD(midsrv_addr);
    options->midsockport = I2AddrPort(midsrv_addr);
    log_println(1, "  -- port: %d", options->midsockport);

    // send this port number to client
    snprintf(buff, sizeof(buff), "%d", options->midsockport);
    if ((msgretvalue = send_msg(ctlsockfd, TEST_PREPARE, buff, strlen(buff)))
        < 0)
      return msgretvalue;

    /* set mss to 1456 (strange value), and large snd/rcv buffers
     * should check to see if server supports window scale ?
     */
    setsockopt(options->midsockfd, SOL_TCP, TCP_MAXSEG, &maxseg,
               sizeof(maxseg));

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
    for (j = 0; j < RETRY_COUNT; j++) {
      msgretvalue = select((options->midsockfd) + 1, &rfd, NULL, NULL,
                           &sel_tv);
      if ((msgretvalue == -1) && (errno == EINTR)) // socket interruption. continue waiting for activity on socket
        continue;
      if (msgretvalue == 0) // timeout
        return SOCKET_CONNECT_TIMEOUT;
      if (msgretvalue < 0) // other socket errors, exit
        return -errno;
      if (j == 4) // retry exceeded. Quit
        return RETRY_EXCEEDED_WAITING_CONNECT;

midfd:
      // if a valid connection request is received, client has connected. Proceed.
      // Note the new socket fd used in the throughput test is this (midsfd)
      if ((midsfd = accept(options->midsockfd,
                           (struct sockaddr *) &cli_addr, &clilen)) > 0) {
        // log protocol validation indicating client connection
        procstatusenum = PROCESS_STARTED;
        proctypeenum = CONNECT_TYPE;
        protolog_procstatus(options->child0, thistestId, proctypeenum,
                            procstatusenum, midsfd);
        break;
      }

      if ((midsfd == -1) && (errno == EINTR)) // socket interrupted, wait some more
        goto midfd;

      snprintf(tmpstr,
               sizeof(tmpstr),
               "-------     middlebox connection setup returned because (%d)",
               errno);
      if (get_debuglvl() > 1)
        perror(tmpstr);
      if (midsfd < 0)
        return -errno;
      if (j == 4)
        return RETRY_EXCEEDED_WAITING_DATA;
    }

    // get meta test details copied into results
    memcpy(&meta.c_addr, &cli_addr, clilen);
    /* meta.c_addr = cli_addr; */
    meta.family = ((struct sockaddr *) &cli_addr)->sa_family;

    buff[0] = '\0';
    // get web100 connection data
    if ((conn = web100_connection_from_socket(agent, midsfd)) == NULL) {
      log_println(
          0,
          "!!!!!!!!!!!  test_mid() failed to get web100 connection data, rc=%d",
          errno);
      /* exit(-1); */
      return -3;
    }

    // Perform S->C throughput test. Obtained results in "buff"
    web100_middlebox(midsfd, agent, conn, buff, sizeof(buff));

    // Transmit results in the form of a TEST_MSG message
    send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));

    // Expect client to send throughput as calculated at its end

    msgLen = sizeof(buff);
    if (recv_msg(ctlsockfd, &msgType, buff, &msgLen)) { // message reception error
      log_println(0, "Protocol error!");
      snprintf(
          buff,
          sizeof(buff),
          "Server (Middlebox test): Invalid CWND limited throughput received");
      send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
      return 1;
    }
    if (check_msg_type("Middlebox test", TEST_MSG, msgType, buff, msgLen)) { // only TEST_MSG type valid
      snprintf(
          buff,
          sizeof(buff),
          "Server (Middlebox test): Invalid CWND limited throughput received");
      send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
      return 2;
    }
    if (msgLen <= 0) { // received message's length has to be a valid one
      log_println(0, "Improper message");
      snprintf(
          buff,
          sizeof(buff),
          "Server (Middlebox test): Invalid CWND limited throughput received");
      send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
      return 3;
    }

    // message payload from client == midbox S->c throughput
    buff[msgLen] = 0;
    *s2c_throughput_mid = atof(buff);
    log_println(4, "CWND limited throughput = %0.0f kbps (%s)",
                *s2c_throughput_mid, buff);

    // finalize the midbox test ; disabling socket used for throughput test and closing out both sockets
    shutdown(midsfd, SHUT_WR);
    close(midsfd);
    close(options->midsockfd);
    send_msg(ctlsockfd, TEST_FINALIZE, "", 0);
    log_println(1, " <--------- %d ----------->", options->child0);

    // log end of test into protocol doc, just to delimit.
    teststatusnow = TEST_ENDED;
    //protolog_status(1, options->child0, thistestId, teststatusnow);
    protolog_status(options->child0, thistestId, teststatusnow,ctlsockfd);

    setCurrentTest(TEST_NONE);
    /* I2AddrFree(midsrv_addr); */
  }
  /* I2AddrFree(midsrv_addr); */
  return 0;
}
