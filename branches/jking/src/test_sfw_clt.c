/*
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

/*
 * Function name: catch_alrm
 * Description: Prints the appropriate message when the SIGALRM is catched.
 * Arguments: signo - the signal number (shuld be SIGALRM)
 */

void
catch_alrm(int signo)
{
  if (signo == SIGALRM) {
    log_println(1, "SIGALRM was caught");
    return;
  }
  log_println(0, "Unknown (%d) signal was caught", signo);
}

/*
 * Function name: test_osfw_clt
 * Description: Performs the client part of the opposite Simple
 *              firewall test in the separate thread.
 */

void*
test_osfw_clt(void* vptr)
{
  char buff[BUFFSIZE+1];
  int sockfd;
  fd_set fds;
  struct timeval sel_tv;
  int msgLen, msgType;
  struct sockaddr_storage srv_addr;
  socklen_t srvlen;

  FD_ZERO(&fds);
  FD_SET(sfwsockfd, &fds);
  sel_tv.tv_sec = testTime;
  sel_tv.tv_usec = 0;
  switch (select(sfwsockfd+1, &fds, NULL, NULL, &sel_tv)) {
    case -1:
      log_println(0, "Simple firewall test: select exited with error");
      I2AddrFree(sfwcli_addr);
      return NULL;
    case 0:
      log_println(1, "Simple firewall test: no connection for %d seconds", testTime);
      s2c_result = SFW_POSSIBLE;
      I2AddrFree(sfwcli_addr);
      return NULL;
  }
  srvlen = sizeof(srv_addr);
  sockfd = accept(sfwsockfd, (struct sockaddr *) &srv_addr, &srvlen);

  msgLen = sizeof(buff);
  if (recv_msg(sockfd, &msgType, buff, &msgLen)) {
    log_println(0, "Simple firewall test: unrecognized message");
    s2c_result = SFW_UNKNOWN;
    close(sockfd);
    I2AddrFree(sfwcli_addr);
    return NULL;
  }
  if (check_msg_type("Simple firewall test", TEST_MSG, msgType, buff, msgLen)) {
    s2c_result = SFW_UNKNOWN;
    close(sockfd);
    I2AddrFree(sfwcli_addr);
    return NULL;
  }
  if (msgLen != 20) {
    log_println(0, "Simple firewall test: Improper message");
    s2c_result = SFW_UNKNOWN;
    close(sockfd);
    I2AddrFree(sfwcli_addr);
    return NULL;
  }
  buff[msgLen] = 0;
  if (strcmp(buff, "Simple firewall test") != 0) {
    log_println(0, "Simple firewall test: Improper message");
    s2c_result = SFW_UNKNOWN;
    close(sockfd);
    I2AddrFree(sfwcli_addr);
    return NULL;
  }

  s2c_result = SFW_NOFIREWALL;
  close(sockfd);
  I2AddrFree(sfwcli_addr);

  return NULL;
}

/*
 * Function name: test_sfw_clt
 * Description: Performs the client part of the Simple firewall test.
 * Arguments: ctlsockfd - the server control socket descriptor
 *            tests - the set of tests to perform
 *            host - the hostname of the server
 *            conn_options - the connection options
 * Returns: 0 - success (the test has been finalized).
 */

int
test_sfw_clt(int ctlsockfd, char tests, char* host, int conn_options)
{
  char buff[BUFFSIZE+1];
  int msgLen, msgType;
  int sfwport, sfwsock, sfwsockport;
  I2Addr sfwsrv_addr = NULL;
  struct sigaction new, old;
  char* ptr;
  pthread_t threadId;
  
  if (tests & TEST_SFW) {
    log_println(1, " <-- Simple firewall test -->");
    printf("checking for firewalls . . . . . . . . . . . . . . . . . . .  ");
    fflush(stdout);
    msgLen = sizeof(buff);
    if (recv_msg(ctlsockfd, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed prepare message!");
      return 1;
    }
    if (check_msg_type("Simple firewall test", TEST_PREPARE, msgType, buff, msgLen)) {
      return 2;
    }
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
    if (check_int(ptr, &sfwport)) {
      log_println(0, "Invalid port number");
      return 4;
    }
    ptr = strtok(NULL, " ");
    if (ptr == NULL) {
      log_println(0, "SFW: Improper message");
      return 5;
    }
    if (check_int(ptr, &testTime)) {
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

    sfwcli_addr = CreateListenSocket(NULL, "0", conn_options, 0);
    if (sfwcli_addr == NULL) {
      log_println(0, "Client (Simple firewall test): CreateListenSocket failed: %s", strerror(errno));
      return -1;
    }
    sfwsockfd = I2AddrFD(sfwcli_addr);
    sfwsockport = I2AddrPort(sfwcli_addr);
    log_println(1, "  -- oport: %d", sfwsockport);

    sprintf(buff, "%d", sfwsockport);
    send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
    
    if (recv_msg(ctlsockfd, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed text message!");
      return 1;
    }
    if (check_msg_type("Simple firewall test", TEST_START, msgType, buff, msgLen)) {
      return 2;
    }
    
    pthread_create(&threadId, NULL, &test_osfw_clt, NULL);

    /* ignore the alrm signal */
    memset(&new, 0, sizeof(new));
    new.sa_handler = catch_alrm;
    sigaction(SIGALRM, &new, &old);
    alarm(testTime + 1);
    if (CreateConnectSocket(&sfwsock, NULL, sfwsrv_addr, conn_options, 0) == 0) {
      send_msg(sfwsock, TEST_MSG, "Simple firewall test", 20);
    }
    alarm(0);
    sigaction(SIGALRM, &old, NULL);

    msgLen = sizeof(buff);
    if (recv_msg(ctlsockfd, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed start message!");
      return 1;
    }
    if (check_msg_type("Simple firewall test", TEST_MSG, msgType, buff, msgLen)) {
      return 2;
    }
    if (msgLen <= 0) {
      log_println(0, "Improper message");
      return 3;
    }
    buff[msgLen] = 0;
    if (check_int(buff, &c2s_result)) {
      log_println(0, "Invalid test result");
      return 4;
    }

    pthread_join(threadId, NULL);
    
    printf("Done\n");
    
    msgLen = sizeof(buff);
    if (recv_msg(ctlsockfd, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed finalize message!");
      return 1;
    }
    if (check_msg_type("Simple firewall test", TEST_FINALIZE, msgType, buff, msgLen)) {
      return 2;
    }
    log_println(1, " <-------------------------->");
  }
  return 0;
}

/*
 * Function name: results_sfw
 * Description: Prints the results of the Simple firewall test to the client.
 * Arguments: tests - the set of tests to perform
 *            host - the hostname of the server
 * Returns: 0 - success.
 */

int
results_sfw(char tests, char* host)
{
  if (tests & TEST_SFW) {
    switch (c2s_result) {
      case SFW_NOFIREWALL:
        printf("Server '%s' is not behind a firewall. [Connection to the ephemeral port was successful]\n", host);
        break;
      case SFW_POSSIBLE:
        printf("Server '%s' is probably behind a firewall. [Connection to the ephemeral port failed]\n", host);
        break;
      case SFW_UNKNOWN:
      case SFW_NOTTESTED:
        break;
    }
    switch (s2c_result) {
      case SFW_NOFIREWALL:
        printf("Client is not behind a firewall. [Connection to the ephemeral port was successful]\n");
        break;
      case SFW_POSSIBLE:
        printf("Client is probably behind a firewall. [Connection to the ephemeral port failed]\n");
        break;
      case SFW_UNKNOWN:
      case SFW_NOTTESTED:
        break;
    }
  }
  return 0;
}
