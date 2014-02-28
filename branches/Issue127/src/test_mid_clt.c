/**
 * This file contains the functions needed to handle Middlebox
 * test (client part).
 *
 * Jakub S³awiñski 2006-08-02
 * jeremian@poczta.fm
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "clt_tests.h"
#include "logging.h"
#include "network.h"
#include "protocol.h"
#include "utils.h"
#include "strlutils.h"

/**
 * Perform the client part of the middleBox testing. The middlebox test
 * is a 5.0 second throughput test from the Server to the Client to
 * check for duplex mismatch conditions. It determines if routers or
 * switches in the path may be making changes to some TCP parameters.
 * @param ctlSocket server control socket descriptor
 * @param tests set of tests to perform
 * @param host hostname of the server
 * @param conn_options connection options
 * @param buf_size TCP send/receive buffer size
 * @param testresult_str result obtained from server (containing server ip,
 * 						client ip, currentMSS, WinSCaleSent, WinScaleRcvd)
 * @return  integer
 *     => 0 on success
 *     < 0 if error
 *     Return codes used:
 *     0 = (the test has been finalized)
 *     > 0 if protocol interactions were not as expected:
 *     		1: Unable to receive protocol message successfully
 * 			2: Wrong message type received
 *			3: Protocol message received was of invalid length
 *			4: Protocol payload data received was invalid
 *			5: Protocol message received was invalid
 *     < 0 if generic error:
 *			-3: Unable to resolve server address
 *			-10: creating connection to server failed
 *
 */
int test_mid_clt(int ctlSocket, char tests, char* host, int conn_options,
                 int buf_size, char* testresult_str) {
  char buff[BUFFSIZE + 1];
  int msgLen, msgType;
  int midport = atoi(PORT3);
  I2Addr sec_addr = NULL;
  int retcode, inlth;
  int in2Socket;
  double t, spdin;
  uint32_t bytes;
  struct timeval sel_tv;
  fd_set rfd;

  enum TEST_STATUS_INT teststatuses = TEST_NOT_STARTED;
  enum TEST_ID testids = MIDDLEBOX;

  if (tests & TEST_MID) {  // middlebox test has to be performed
    log_println(1, " <-- Middlebox test -->");
    setCurrentTest(TEST_MID);
    // protocol logs
    teststatuses = TEST_STARTED;
    protolog_status(getpid(), testids, teststatuses, ctlSocket);


    //  Initially, expecting a TEST_PREPARE message. Any other message
    // ..type is unexpected at this stage.

    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed prepare message!");
      return 1;
    }
    if (check_msg_type(MIDBOX_TEST_LOG, TEST_PREPARE, msgType, buff, msgLen)) {
      return 2;
    }

    // The server is expected to send a message with a valid payload that
    // contains the port number that server wants client to bind to for this
    // test
    if (msgLen <= 0) {
      log_println(0, "Improper message");
      return 3;
    }
    buff[msgLen] = 0;
    if (check_int(buff, &midport)) {  // obtained message does not contain
                                      // integer port#
      log_println(0, "Invalid port number");
      return 4;
    }

    // get server address and set port
    log_println(1, "  -- port: %d", midport);
    if ((sec_addr = I2AddrByNode(get_errhandle(), host)) == NULL) {
      log_println(0, "Unable to resolve server address: %s", strerror(errno));
      return -3;
    }
    I2AddrSetPort(sec_addr, midport);

    // debug to check if correct port was set in addr struct
    if (get_debuglvl() > 4) {
      char tmpbuff[200];
      size_t tmpBufLen = 199;
      memset(tmpbuff, 0, 200);
      I2AddrNodeName(sec_addr, tmpbuff, &tmpBufLen);
      log_println(5, "connecting to %s:%d", tmpbuff, I2AddrPort(sec_addr));
    }

    // connect to server using port obtained above
    if ((retcode = CreateConnectSocket(&in2Socket, NULL, sec_addr,
                                       conn_options, buf_size))) {
      log_println(0, "Connect() for middlebox failed: %s", strerror(errno));
      return -10;
    }

    // start reading throughput test data from server using the connection
    // created above
    printf("Checking for Middleboxes . . . . . . . . . . . . . . . . . .  ");
    fflush(stdout);
    testresult_str[0] = '\0';
    bytes = 0;
    t = secs() + 5.0;  // set timer for 5 seconds, and read for 5 seconds
    sel_tv.tv_sec = 6;  // Time out the socket after 6.5 seconds
    sel_tv.tv_usec = 5;  // 500?
    FD_ZERO(&rfd);
    FD_SET(in2Socket, &rfd);
    for (;;) {
      if (secs() > t)
        break;
      retcode = select(in2Socket+1, &rfd, NULL, NULL, &sel_tv);
      if (retcode > 0) {
        inlth = read(in2Socket, buff, sizeof(buff));
        if (inlth == 0)
          break;
        bytes += inlth;
        continue;
      }
      if (retcode < 0) {
        printf("nothing to read, exiting read loop\n");
        break;
      }
      if (retcode == 0) {
        printf("timer expired, exiting read loop\n");
        break;
      }
    }
    // get actual time for which test was run
    t = secs() - t + 5.0;

    // calculate throughput in Kbps
    spdin = ((BITS_8_FLOAT * bytes) / KILO) / t;

    // Test is complete. Now, get results from server (includes CurrentMSS,
    // WinScaleSent, WinScaleRcvd..).
    // The results are sent from server in the form of a TEST_MSG object
    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed text message!");
      return 1;
    }
    if (check_msg_type(MIDBOX_TEST_LOG " results", TEST_MSG, msgType, buff,
                       msgLen)) {
      return 2;
    }

    strlcat(testresult_str, buff, MIDBOX_TEST_RES_SIZE);

    memset(buff, 0, sizeof(buff));
    // this should work since the throughput results from the server should
    //  ...fit well within BUFFSIZE
    snprintf(buff, sizeof(buff), "%0.0f", spdin);
    log_println(4, "CWND limited speed = %0.2f kbps", spdin);

    // client now sends throughput it calculated above to server, as a TEST_MSG
    send_msg(ctlSocket, TEST_MSG, buff, strlen(buff));
    printf("Done\n");

    I2AddrFree(sec_addr);

    // Expect an empty TEST_FINALIZE message from server
    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed finalize message!");
      return 1;
    }
    if (check_msg_type(MIDBOX_TEST_LOG, TEST_FINALIZE, msgType, buff, msgLen)) {
      return 2;
    }
    log_println(1, " <-------------------->");
    // log protocol test ending
    teststatuses = TEST_ENDED;
    protolog_status(getpid(), testids, teststatuses, ctlSocket);
    setCurrentTest(TEST_NONE);
  }
  return 0;
}
