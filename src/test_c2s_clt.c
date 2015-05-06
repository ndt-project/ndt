/**
 * This file contains the functions needed to handle C2S throughput
 * test (client part).
 *
 * Jakub S�awi�ski 2006-08-02
 * jeremian@poczta.fm
 */

#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "clt_tests.h"
#include "logging.h"
#include "network.h"
#include "network_clt.h"
#include "protocol.h"
#include "utils.h"
#include "jsonutils.h"

int pkts, lth;
int sndqueue;
double spdout, c2sspd;

/**
 * Client to server throughput test. This test performs 10 seconds
 * memory-to-memory data transfer to test achievable network bandwidth.
 *
 * @param ctlSocket Socket on which messages are received from the server
 * @param tests     Character indicator for test to be run
 * @param host		Server name string
 * @param conn_options Options to use while connecting to server(for ex, IPV4)
 * @param buf_size  TCP send/receive buffer size
 * @param jsonSupport Indicates if messages should be send using JSON format
 * @return integer > 0 if successful, < 0 in case of error
 * 		Return codes:
 * 		1: Error receiving protocol message
 *		2: Unexpected protocol message (type) received
 *		3: Improper message payload
 *		4: Incorrect message data received
 *		-3: Unable to resolve server address
 *		-11: Cannot connect to server
 *
 */
int test_c2s_clt(int ctlSocket, char tests, char* host, int conn_options,
                 int buf_size, int jsonSupport) {
  /* char buff[BUFFSIZE+1]; */
  char buff[64 * KILO_BITS];  // message payload.
  // note that there is a size variation between server and CLT - do not
  // know why this was changed from BUFFZISE, though
  // no specific problem is seen due to this
  char* jsonMsgValue; // temporary buffer for storing values from JSON message

  int msgLen, msgType;  // message related data
  int c2sport = atoi(PORT2);  // default C2S port
  I2Addr sec_addr = NULL;  // server address
  int retcode;  // return code
  int one = 1;  // socket option store
  int i, k;  // temporary iterator
  int outSocket;  // socket descriptor for the outgoing connection
  double t, stop_time;  // test-time indicators
  // variables used for protocol validation logs
  enum TEST_STATUS_INT teststatuses = TEST_NOT_STARTED;
  enum TEST_ID testids = C2S;


  if (tests & TEST_C2S) {  // C2S test has to be performed
    struct sigaction new, old;
    log_println(1, " <-- C2S throughput test -->");
    setCurrentTest(TEST_C2S);
    // protocol logs
    teststatuses = TEST_STARTED;
    protolog_status(getpid(), testids, teststatuses, ctlSocket);

    msgLen = sizeof(buff);

    // Initially, the server sends a TEST_PREPARE message. Any other message
    // type, or a message with payload length <0 is unexpected and
    // indicative of an error
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed prepare message!");
      return 1;
    }
    if (check_msg_type("C2S throughput test", TEST_PREPARE, msgType, buff,
                       msgLen)) {
      return 2;
    }
    buff[msgLen] = 0;
    if (jsonSupport) {
      jsonMsgValue = json_read_map_value(buff, DEFAULT_KEY);
      strlcpy(buff, jsonMsgValue, sizeof(buff));
      msgLen = strlen(buff);
      free(jsonMsgValue);
    }
    if (msgLen <= 0) {
      log_println(0, "Improper message");
      return 3;
    }

    // Server sends port number to bind to in the TEST_PREPARE. Check if this
    // message body (string) is a valid integral port number.
    if (check_int(buff, &c2sport)) {
      log_println(0, "Invalid port number");
      return 4;
    }
    log_println(1, "  -- port: %d", c2sport);

    // make struct of "address details" of the server using the host name
    if ((sec_addr = I2AddrByNode(get_errhandle(), host)) == NULL) {
      log_println(0, "Unable to resolve server address: %s", strerror(errno));
      return -3;
    }
    I2AddrSetPort(sec_addr, c2sport);  // set port value

    // connect to server  and set socket options
    if ((retcode = CreateConnectSocket(&outSocket, NULL, sec_addr, conn_options,
                                       buf_size))) {
      log_println(0, "Connect() for client to server failed", strerror(errno));
      return -11;
    }

    setsockopt(outSocket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    // Expect a TEST_START message from server now. Any other message
    // type is an error
    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed start message!");
      return 1;
    }
    if (check_msg_type("C2S throughput test", TEST_START, msgType, buff,
                       msgLen)) {
      return 2;
    }

    // Prepare for running a C->S throughput test
    printf("running 10s outbound test (client to server) . . . . . ");
    fflush(stdout);

    // ....Fill buffer upto NDTConstants.PREDEFNED_BUFFER_SIZE packets
    pkts = 0;
    k = 0;
    for (i = 0; i < (64*KILO_BITS); i++) {  // again buffer sizes differ.
      // Since the actual transmitted byte count is timed, it doesn't appear
      // that it is causing specific problems.
      while (!isprint(k & 0x7f))
        k++;
      buff[i] = (k++ % 0x7f);
    }

    // set the test time to 10 seconds
    t = secs();
    stop_time = t + 10;
    /* ignore the pipe signal */
    memset(&new, 0, sizeof(new));
    new.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &new, &old);

    // While the 10 s timer ticks, stream data to server. Record the byte count
    do {
      write(outSocket, buff, lth);
      pkts++;
    }while (secs() < stop_time);
    sigaction(SIGPIPE, &old, NULL);
    sndqueue = sndq_len(outSocket);  // get send-queue length

    // get actual duration for which data was sent to the server
    t = secs() - t;
    I2AddrFree(sec_addr);

    // Calculate C2S throughput in kbps
    spdout = ((BITS_8_FLOAT * pkts * lth) / KILO) / t;
    // log_println(6," ---C->S CLT speed=%0.0f, pkts= %d, lth=%d, time=%d",
    //             spdout, pkts, lth, t);


    // The client has stopped streaming data, and the server is now
    // expected to send a TEST_MSG message with the throughput it calculated.
    // So, its time now to receive this throughput (c2sspd).

    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed text message!");
      return 1;
    }
    if (check_msg_type("C2S throughput test", TEST_MSG, msgType, buff,
                       msgLen)) {
      // other message types at this juncture indicate error
      return 2;
    }
    if (msgLen <= 0) {  // message payload size cannot be negative! Error.
      log_println(0, "Improper message");
      return 3;
    }
    buff[msgLen] = 0;

    // get C->S test speed as calculated by server
    if (json_check_msg(buff) == 0) {
      jsonMsgValue = json_read_map_value(buff, DEFAULT_KEY);
      c2sspd = atoi(jsonMsgValue);
      free(jsonMsgValue);
    }
    else {
      c2sspd = atoi(buff);
    }

    // Print results in the most convenient units (kbps or Mbps)
    if (c2sspd < KILO)
      printf(" %0.2f kb/s\n", c2sspd);
    else
      printf(" %0.2f Mb/s\n", c2sspd/KILO);

    // Server should close test session with an empty TEST_FINALIZE message.
    // Any other type of message is an error.
    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed finalize message!");
      return 1;
    }
    if (check_msg_type("C2S throughput test", TEST_FINALIZE, msgType, buff,
                       msgLen)) {
      return 2;
    }
    log_println(1, " <------------------------->");
    // log protocol validation logs
    teststatuses = TEST_ENDED;
    protolog_status(getpid(), testids, teststatuses, ctlSocket);
    setCurrentTest(TEST_NONE);
  }
  return 0;
}
