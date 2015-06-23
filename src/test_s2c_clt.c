/**
 * This file contains the functions needed to handle S2C throughput
 * test (client part).
 *
 * Jakub S�awi�ski 2006-08-02
 * jeremian@poczta.fm
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "clt_tests.h"
#include "logging.h"
#include "network.h"
#include "network_clt.h"
#include "protocol.h"
#include "utils.h"
#include "strlutils.h"
#include "jsonutils.h"

int ssndqueue, sbytes;
double spdin, s2cspd;

#define THROUGHPUT_VALUE "ThroughputValue"
#define UNSENT_DATA_AMOUNT "UnsentDataAmount"
#define TOTALSENTBYTE "TotalSentByte"

/**
 * S2C throughput test to measure network bandwidth
 * from server to client.
 * @param ctlSocket 	Socket on which messages are received from the server
 * @param tests     	Character indicator for test to be run
 * @param host          Server name string
 * @param conn_options 	Options to use while connecting to server(for ex, IPV4)
 * @param buf_size  	TCP send/receive buffer size
 * @param result_srv		result obtained from server (containing values of web100 variables)
 * @param jsonSupport 	Indicates if messages should be sent using JSON format
 * @return integer > 0 if successful, < 0 in case of error
 * 		Return codes:
 * 		1: Error receiving protocol message
 *		2: Unexpected protocol message (type) received
 *		3: Improper message payload
 *		4: Incorrect message data received
 *		-3: Unable to resolve server address
 *		-15: Cannot connect to server
 */

int test_s2c_clt(int ctlSocket, char tests, char* host, int conn_options,
                 int buf_size, char* result_srv, int jsonSupport) {
  char buff[BUFFSIZE + 1];
  int msgLen, msgType;
  int s2cport = atoi(PORT3);
  I2Addr sec_addr = NULL;
  int inlth, retcode, one = 1, set_size;
  int inSocket;
  socklen_t optlen;
  uint64_t bytes;
  double t;
  struct timeval sel_tv;
  fd_set rfd;
  char* ptr, *jsonMsgValue;

  // variables used for protocol validation logs
  enum TEST_STATUS_INT teststatuses = TEST_NOT_STARTED;
  enum TEST_ID testids = S2C;

  if (tests & TEST_S2C) {
    setCurrentTest(TEST_S2C);
    // protocol logs
    teststatuses = TEST_STARTED;
    protolog_status(getpid(), testids, teststatuses, ctlSocket);

    // First message expected from the server is a TEST_PREPARE. Any other
    // message type is unexpected at this point.
    log_println(1, " <-- S2C throughput test -->");
    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed prepare message!");
      return 1;
    }
    if (check_msg_type(S2C_TEST_LOG, TEST_PREPARE, msgType, buff, msgLen)) {
      return 2;
    }
    buff[msgLen] = 0;
    if (jsonSupport) {
      jsonMsgValue = json_read_map_value(buff, DEFAULT_KEY);
      strlcpy(buff, jsonMsgValue, sizeof(buff));
      msgLen = strlen(buff);
      free(jsonMsgValue);
    }
    // This TEST_PREPARE message is expected to have the port number as message
    // body. Check if this is a valid integral port.
    if (msgLen <= 0) {
      log_println(0, "Improper message");
      return 3;
    }
    if (check_int(buff, &s2cport)) {
      log_println(0, "Invalid port number");
      return 4;
    }
    log_println(1, "  -- port: %d", s2cport);

    /* Cygwin seems to want/need this extra getsockopt() function
     * call.  It certainly doesn't do anything, but the S2C test fails
     * at the connect() call if it's not there.  4/14/05 RAC
     */
    optlen = sizeof(set_size);
    getsockopt(ctlSocket, SOL_SOCKET, SO_SNDBUF, &set_size, &optlen);

    // get "address details" of the server using the host name
    if ((sec_addr = I2AddrByNode(get_errhandle(), host)) == NULL) {
      log_println(0, "Unable to resolve server address: %s", strerror(errno));
      return -3;
    }
    I2AddrSetPort(sec_addr, s2cport);  // set port to value obtained from server

    // Connect to the server; set socket options
    if ((retcode = CreateConnectSocket(&inSocket, NULL, sec_addr, conn_options,
                                       buf_size))) {
      log_println(0, "Connect() for Server to Client failed", strerror(errno));
      return -15;
    }

    setsockopt(inSocket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    /* Linux updates the sel_tv time values everytime select returns.  This
     * means that eventually the timer will reach 0 seconds and select will
     * exit with a timeout signal.  Other OS's don't do that so they need
     * another method for detecting a long-running process.  The check below
     * will cause the loop to terminate if select says there is something
     * to read and the loop has been active for over 14 seconds.  This usually
     * happens when there is a problem (duplex mismatch) and there is data
     * queued up on the server.
     */

    // Expect a TEST_START message from server now. Any other message
    // type is an error
    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed start message!");
      return 1;
    }
    if (check_msg_type(S2C_TEST_LOG, TEST_START, msgType, buff, msgLen)) {
      return 2;
    }

    // server performs the S->C throughput test now. Get data sent by server.

    printf("running 10s inbound test (server to client) . . . . . . ");
    fflush(stdout);

    // Set socket timeout to 15 seconds
    bytes = 0;
    t = secs() + 15.0;
    sel_tv.tv_sec = 15;
    sel_tv.tv_usec = 5;
    FD_ZERO(&rfd);
    FD_SET(inSocket, &rfd);
    // Read data sent by server as soon as it is available. Stop listening if
    // ...timeout has been exceeded.
    for (;;) {
      retcode = select(inSocket+1, &rfd, NULL, NULL, &sel_tv);
      if (secs() > t) {
        log_println(5, "Receive test running long, break out of read loop");
        break;
      }
      if (retcode > 0) {
        inlth = read(inSocket, buff, sizeof(buff));
        if (inlth == 0)
          break;
        bytes += inlth;
        continue;
      }
      if (get_debuglvl() > 5) {
        log_println(0, "s2c read loop exiting:", strerror(errno));
      }
      break;
    }

    // get actual time for which data was received, and calculate throughput
    // based on it.
    t = secs() - t + 15.0;
    spdin = ((BITS_8_FLOAT * bytes) / KILO) / t;  // kbps

    // log_println(0,"S->C: Received %d bytes in %0.2f secs: Spdin= %f", bytes,
    //             t, spdin);

    // Server sends calculated throughput value, unsent data amount in the
    // socket queue and overall number of sent bytes in a TEST_MSG
    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed text message!");
      return 1;
    }
    if (check_msg_type(S2C_TEST_LOG, TEST_MSG, msgType, buff, msgLen)) {
      return 2;  // no other message type expected
    }
    buff[msgLen] = 0;
    // Is message of valid length, and does it have all the data expected?
    if (msgLen <= 0) {
      log_println(0, "Improper message");
      return 3;
    }
    if (jsonSupport) {
      jsonMsgValue = json_read_map_value(buff, THROUGHPUT_VALUE);
      if (jsonMsgValue == NULL) {
        log_println(0, "S2C: Improper message");
        return 4;
      }
      s2cspd = atoi(jsonMsgValue);
      free(jsonMsgValue);

      jsonMsgValue = json_read_map_value(buff, UNSENT_DATA_AMOUNT);
      if (jsonMsgValue == NULL) {
        log_println(0, "S2C: Improper message");
        return 4;
      }
      ssndqueue = atoi(jsonMsgValue);
      free(jsonMsgValue);

      jsonMsgValue = json_read_map_value(buff, TOTALSENTBYTE);
      if (jsonMsgValue == NULL) {
        log_println(0, "S2C: Improper message");
        return 4;
      }
      sbytes = atoi(jsonMsgValue);
      free(jsonMsgValue);
    }
    else {
      ptr = strtok(buff, " ");
      if (ptr == NULL) {
        log_println(0, "S2C: Improper message");
        return 4;
      }
      s2cspd = atoi(ptr);  // get S->C throughput first
      ptr = strtok(NULL, " ");
      if (ptr == NULL) {
        log_println(0, "S2C: Improper message");
        return 4;
      }
      ssndqueue = atoi(ptr);  // get amount of unsent data in queue
      ptr = strtok(NULL, " ");
      if (ptr == NULL) {
        log_println(0, "S2C: Improper message");
        return 4;
      }
      sbytes = atoi(ptr);  // finally get total-sent-byte-count
    }

    // log_println(0,"S->C received throughput: %f",s2cspd);
    // log results in a convenient units format
    if (spdin < 1000)
      printf("%0.2f kb/s\n", spdin);
    else
      printf("%0.2f Mb/s\n", spdin/1000);

    I2AddrFree(sec_addr);

    // send TEST_MSG to server with the client-calculated throughput
    snprintf(buff, sizeof(buff), "%0.0f", spdin);
    send_json_message(ctlSocket, TEST_MSG, buff, strlen(buff),
                      jsonSupport, JSON_SINGLE_VALUE);

    // client now expected to receive web100 variables collected by server

    result_srv[0] = '\0';
    for (;;) {
      msgLen = sizeof(buff);
      memset(buff, 0, msgLen);  // reset buff and msgLen

      // get web100 variables
      if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
        log_println(0, "Protocol error - missed text/finalize message!");
        return 1;
      }

      // TEST_FINALIZE msg from server indicates end of web100 var transmission
      if (msgType == TEST_FINALIZE) {
        break;
      }

      // if neither TEST_FINALIZE, nor TEST_MSG, signal error!
      if (check_msg_type(S2C_TEST_LOG, TEST_MSG, msgType, buff, msgLen)) {
        return 2;
      }

      if (jsonSupport) {
        jsonMsgValue = json_read_map_value(buff, DEFAULT_KEY);
        strlcat(result_srv, jsonMsgValue, 2 * BUFFSIZE);
        free(jsonMsgValue);
      }
      else {
        // hardcoded size of array from main tests
        strlcat(result_srv, buff, 2 * BUFFSIZE);
      }

    }
    log_println(6, "result_srv = '%s', of len %d", result_srv, msgLen);
    log_println(1, " <------------------------->");

    // log protocol validation logs
    teststatuses = TEST_ENDED;
    protolog_status(getpid(), testids, teststatuses, ctlSocket);
    setCurrentTest(TEST_NONE);
  }

  return 0;
}
