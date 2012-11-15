/**
 * This file contains the functions needed to handle simple firewall
 * test (client part).
 *
 * Jakub S³awiñski 2006-07-15
 * jeremian@poczta.fm
 */

#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "test_sfw.h"
#include "logging.h"
#include "protocol.h"
#include "network.h"
#include "utils.h"

static int c2s_result = SFW_NOTTESTED;
static int s2c_result = SFW_NOTTESTED;
static int testTime;
static int sfwsockfd;
static I2Addr sfwcli_addr = NULL;


/**
 * Print the appropriate message when the SIGALRM is caught.
 * @param signo signal number (shuld be SIGALRM)
 */
void catch_alrm(int signo) {
  if (signo == SIGALRM) {
    log_println(1, "SIGALRM was caught");
    return;
  }
  log_println(0, "Unknown (%d) signal was caught", signo);
}

/**
 * Perform the client part of the opposite Simple firewall test in a separate thread.
 * This thread listens for messages from the server for a given time period,
 * and checks to see if the server has sent a message that is valid
 * and sufficient to determine if the S->C direction has a fire-wall. It updates
 * the value of the S->C firewall test result indicator based on the outcome.
 *
 * @param vptr void pointer reference to argument passed to this routine
 * @return void pointer NULL
 */

void*
test_osfw_clt(void* vptr) {
  char buff[BUFFSIZE + 1];
  int sockfd;
  fd_set fds;
  struct timeval sel_tv;
  int msgLen, msgType;
  struct sockaddr_storage srv_addr;
  socklen_t srvlen;

  // protocol logging
  enum PROCESS_TYPE_INT proctypeenum = PROCESS_TYPE;
  enum PROCESS_STATUS_INT procstatusenum = UNKNOWN;
  enum TEST_ID testids = SFW;

  // listen for a message from server for a preset time, testTime
  FD_ZERO(&fds);
  FD_SET(sfwsockfd, &fds);
  sel_tv.tv_sec = testTime;
  sel_tv.tv_usec = 0;
  switch (select(sfwsockfd+1, &fds, NULL, NULL, &sel_tv)) {
    case -1: // If Socket error, status of firewall is unknown
      log_println(0, "Simple firewall test: select exited with error");
      I2AddrFree(sfwcli_addr);
      return NULL;
    case 0: // timeout, indicates that firewall is a possibility
      log_println(1, "Simple firewall test: CLT: no connection for %d seconds", testTime);
      s2c_result = SFW_POSSIBLE;
      I2AddrFree(sfwcli_addr);
      return NULL;
  }

  // Get message sent by server. If message reception is erroneous, firewall status
  // .. cannot be determined
  srvlen = sizeof(srv_addr);
  sockfd = accept(sfwsockfd, (struct sockaddr *) &srv_addr, &srvlen);

  // log protocol for connection accept
  if (sockfd > 0) {
    procstatusenum = PROCESS_STARTED;
    proctypeenum = CONNECT_TYPE;
    protolog_procstatus(getpid(), testids , proctypeenum,
                        procstatusenum, sockfd );
  }

  msgLen = sizeof(buff);
  if (recv_msg(sockfd, &msgType, buff, &msgLen)) {
    log_println(0, "Simple firewall test: unrecognized message");
    s2c_result = SFW_UNKNOWN;
    close(sockfd);
    I2AddrFree(sfwcli_addr);
    return NULL;
  }

  // The server is expected to send a TEST_MSG type of packet. Any other message
  // .. type is unexpected at this point and leaves firewall status indeterminate.
  if (check_msg_type(SFW_TEST_LOG, TEST_MSG, msgType, buff, msgLen)) {
    s2c_result = SFW_UNKNOWN;
    close(sockfd);
    I2AddrFree(sfwcli_addr);
    return NULL;
  }

  // The server is expected to send a 20 char message that
  // says "Simple firewall test" . Every other message string
  // indicates an unknown firewall status

  if (msgLen != SFW_TEST_DEFAULT_LEN) {
    log_println(0, "Simple firewall test: Improper message");
    s2c_result = SFW_UNKNOWN;
    close(sockfd);
    I2AddrFree(sfwcli_addr);
    return NULL;
  }
  buff[msgLen] = 0;
  if (strcmp(buff, SFW_PREDEFINED_TEST_MSG) != 0) {
    log_println(0, "Simple firewall test: Improper message");
    s2c_result = SFW_UNKNOWN;
    close(sockfd);
    I2AddrFree(sfwcli_addr);
    return NULL;
  }

  // If none of the above conditions were met, then, the server
  // message has been received correctly, and there seems to be no
  // firewall. Perform clean-up activities.

  s2c_result = SFW_NOFIREWALL;
  close(sockfd);
  I2AddrFree(sfwcli_addr);

  return NULL;
}

/**
 * Perform the client part of the Simple firewall test.
 * @param ctlsockfd server control socket descriptor
 * @param tests set of tests to perform
 * @param host hostname of the server
 * @param conn_options the connection options
 * @return integer
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
 *     		-1: Listen socket creation failed
 *			-3: Unable to resolve server address
 *
 */
int test_sfw_clt(int ctlsockfd, char tests, char* host, int conn_options) {
  char buff[BUFFSIZE + 1];int msgLen, msgType;
  int sfwport, sfwsock, sfwsockport;
  I2Addr sfwsrv_addr = NULL;
  struct sigaction new, old;
  char* ptr;
  pthread_t threadId;

  // variables used for protocol validation logs
  enum TEST_STATUS_INT teststatuses = TEST_NOT_STARTED;
  enum TEST_ID testids = SFW;

  if (tests & TEST_SFW) { // SFW test is to be performed
    log_println(1, " <-- Simple firewall test -->");
    setCurrentTest(TEST_SFW);
    //protocol logs
    teststatuses = TEST_STARTED;
    protolog_status(getpid(), testids, teststatuses, ctlsockfd);


    printf("checking for firewalls . . . . . . . . . . . . . . . . . . .  ");
    fflush(stdout);
    msgLen = sizeof(buff);

    // The first message expected from the server is a TEST_PREPARE message.
    // Any other type of message is unexpected at this time.
    if (recv_msg(ctlsockfd, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed prepare message!");
      return 1;
    }
    if (check_msg_type(SFW_TEST_LOG, TEST_PREPARE, msgType, buff, msgLen)) {
      return 2;
    }

    // This message is expected to be of valid length, and have a message
    // .. payload containing the integral ephemeral port number and testTime
    // ... separated by a single space
    if (msgLen <= 0) {
      log_println(0, "Improper message");
      return 3;
    }
    buff[msgLen] = 0;
    ptr = strtok(buff, " ");
    if (ptr == NULL) {
      log_println(0, "SFW: Improper message");
      return 5;
    }
    if (check_int(ptr, &sfwport)) {  // get ephemeral port#
      log_println(0, "Invalid port number");
      return 4;
    }
    ptr = strtok(NULL, " ");
    if (ptr == NULL) {
      log_println(0, "SFW: Improper message");
      return 5;
    }
    if (check_int(ptr, &testTime)) { // get test time
      log_println(0, "Invalid waiting time");
      return 4;
    }
    log_println(1, "\n  -- port: %d", sfwport);
    log_println(1, "  -- time: %d", testTime);
    if ((sfwsrv_addr = I2AddrByNode(get_errhandle(), host)) == NULL) {
      log_println(0, "Unable to resolve server address: %s", strerror(errno));
      return -3;
    }
    I2AddrSetPort(sfwsrv_addr, sfwport);

    // create a listening socket, and if successful, send this socket number to server
    sfwcli_addr = CreateListenSocket(NULL, "0", conn_options, 0);
    if (sfwcli_addr == NULL) {
      log_println(0, "Client (Simple firewall test): CreateListenSocket failed: %s", strerror(errno));
      return -1;
    }
    sfwsockfd = I2AddrFD(sfwcli_addr);
    sfwsockport = I2AddrPort(sfwcli_addr);
    log_println(1, "  -- oport: %d", sfwsockport);

    // Send a TEST_MSG to server with the client's port number
    snprintf(buff, sizeof(buff), "%d", sfwsockport);
    send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));

    // The server responds to the TEST_MSG messages with a TEST_START message.
    // Any other type of message is an error here.
    if (recv_msg(ctlsockfd, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed start message!");
      return 1;
    }
    if (check_msg_type(SFW_TEST_LOG, TEST_START, msgType, buff, msgLen)) {
      return 2;
    }

    // Now listen for server sending out a test for the S->C direction , and
    // update test results
    pthread_create(&threadId, NULL, &test_osfw_clt, NULL);

    // ignore the alarm signal
    memset(&new, 0, sizeof(new));
    new.sa_handler = catch_alrm;
    sigaction(SIGALRM, &new, &old);
    alarm(testTime + 1);

    // Now, run Test from client for the C->S direction SFW test
    // ..trying to connect to ephemeral port number sent by server

    if (CreateConnectSocket(&sfwsock, NULL, sfwsrv_addr, conn_options, 0) == 0) {
      send_msg(sfwsock, TEST_MSG, SFW_PREDEFINED_TEST_MSG, 20);
    }
    alarm(0);
    sigaction(SIGALRM, &old, NULL);

    // Expect a TES_MSG from server with results. Other types of messages are
    // ..unexpected at this point.

    msgLen = sizeof(buff);
    if (recv_msg(ctlsockfd, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed text message!");
      return 1;
    }
    if (check_msg_type(SFW_TEST_LOG, TEST_MSG, msgType, buff, msgLen)) {
      return 2;
    }

    // The test results are sent as a numeric encoding the test results.

    if (msgLen <= 0) { // test results have valid length
      log_println(0, "Improper message");
      return 3;
    }
    buff[msgLen] = 0;
    if (check_int(buff, &c2s_result)) { // test result has to be a valid integer
      log_println(0, "Invalid test result");
      return 4;
    }

    // wait for the S->C listener thread to finish
    pthread_join(threadId, NULL);

    printf("Done\n");

    // The last message expected from the server is an empty TEST_FINALIZE message.
    msgLen = sizeof(buff);
    if (recv_msg(ctlsockfd, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed finalize message!");
      return 1;
    }
    if (check_msg_type(SFW_TEST_LOG, TEST_FINALIZE, msgType, buff, msgLen)) {
      return 2;
    }
    log_println(1, " <-------------------------->");
    // log protocol validation test ending
    teststatuses = TEST_ENDED;
    protolog_status(getpid(), testids, teststatuses,ctlsockfd);
    setCurrentTest(TEST_NONE);
  }
  return 0;
}

/**
 * Print the results of the Simple firewall test to the client.
 * @param tests set of tests to perform
 * @param host hostname of the server
 * @return 0
 */

int results_sfw(char tests, char* host) {
  if (tests & TEST_SFW) { // SFW test has been selected to be run
    // Print C->S direction's results
    switch (c2s_result) {
      case SFW_NOFIREWALL:
        printf(
            "Server '%s' is not behind a firewall."
            " [Connection to the ephemeral port was successful]\n",
            host);
        break;
      case SFW_POSSIBLE:
        printf(
            "Server '%s' is probably behind a firewall."
            " [Connection to the ephemeral port failed]\n",
            host);
        break;
      case SFW_UNKNOWN:
      case SFW_NOTTESTED:
        break;
    }
    // Print S->C directions results
    switch (s2c_result) {
      case SFW_NOFIREWALL:
        printf(
            "Client is not behind a firewall."
            " [Connection to the ephemeral port was successful]\n");
        break;
      case SFW_POSSIBLE:
        printf(
            "Client is probably behind a firewall."
            " [Connection to the ephemeral port failed]\n");
        break;
      case SFW_UNKNOWN:
      case SFW_NOTTESTED:
        break;
    }
  }
  return 0;
}
