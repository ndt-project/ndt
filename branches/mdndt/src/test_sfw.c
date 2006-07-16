/*
 * This file contains the functions needed to handle simple firewall
 * test.
 *
 * Jakub S³awiñski 2006-07-15
 * jeremian@poczta.fm
 */

#include <assert.h>

#include "test_sfw.h"
#include "logging.h"
#include "protocol.h"
#include "network.h"
#include "utils.h"

static int c2s_result = SFW_NOTTESTED;

/*
 * Function name: test_sfw_srv
 * Description: Performs the server part of the Simple firewall test.
 * Arguments: ctlsockfd - the client control socket descriptor
 *            options - the test options
 *            conn_options - the connection options
 * Returns: 0 - success (no firewalls on the path),
 *          1 - failure (protocol mismatch),
 *          2 - unknown (probably firwall on the path).
 */

int
test_sfw_srv(int ctlsockfd, TestOptions* options, int conn_options)
{
  char buff[BUFFSIZE+1];
  I2Addr sfwsrv_addr = NULL;
  int sfwsockfd, sfwsockport, sockfd;
  struct sockaddr_storage cli_addr;
  socklen_t clilen;
  fd_set fds;
  struct timeval sel_tv;
  int msgLen, msgType;
  
  assert(ctlsockfd != -1);
  assert(options);

  if (options->sfwopt) {
    log_println(1, " <-- Simple firewall test -->");
    
    sfwsrv_addr = CreateListenSocket(NULL, "0", conn_options);
    if (sfwsrv_addr == NULL) {
      err_sys("server: CreateListenSocket failed");
    }
    sfwsockfd = I2AddrFD(sfwsrv_addr);
    sfwsockport = I2AddrPort(sfwsrv_addr);
    log_println(1, "  -- port: %d", sfwsockport);
    
    sprintf(buff, "%d", sfwsockport);
    send_msg(ctlsockfd, TEST_PREPARE, buff, strlen(buff));
    
    FD_ZERO(&fds);
    FD_SET(sfwsockfd, &fds);
    sel_tv.tv_sec = 30;
    sel_tv.tv_usec = 0;
    switch (select(sfwsockfd+1, &fds, NULL, NULL, &sel_tv)) {
      case -1:
        log_println(0, "Simple firewall test: select exited with error");
        sprintf(buff, "%d", SFW_UNKNOWN);
        send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
        send_msg(ctlsockfd, TEST_FINALIZE, "", 0);
        I2AddrFree(sfwsrv_addr);
        log_println(1, " <-------------------------->");
        return 1;
      case 0:
        log_println(0, "Simple firewall test: no connection for 30 seconds");
        sprintf(buff, "%d", SFW_POSSIBLE);
        send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
        send_msg(ctlsockfd, TEST_FINALIZE, "", 0);
        I2AddrFree(sfwsrv_addr);
        log_println(1, " <-------------------------->");
        return 2;
    }
    clilen = sizeof(cli_addr);
    sockfd = accept(sfwsockfd, (struct sockaddr *) &cli_addr, &clilen);

    if (recv_msg(sockfd, &msgType, &buff, &msgLen)) {
      log_println(0, "Simple firewall test: unrecognized message");
      sprintf(buff, "%d", SFW_UNKNOWN);
      send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
      send_msg(ctlsockfd, TEST_FINALIZE, "", 0);
      close(sockfd);
      I2AddrFree(sfwsrv_addr);
      log_println(1, " <-------------------------->");
      return 1;
    }
    if (check_msg_type("Simple firewall test", TEST_MSG, msgType)) {
      sprintf(buff, "%d", SFW_UNKNOWN);
      send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
      send_msg(ctlsockfd, TEST_FINALIZE, "", 0);
      close(sockfd);
      I2AddrFree(sfwsrv_addr);
      log_println(1, " <-------------------------->");
      return 1;
    }
    if (msgLen != 20) {
      log_println(0, "Simple firewall test: Improper message");
      sprintf(buff, "%d", SFW_UNKNOWN);
      send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
      send_msg(ctlsockfd, TEST_FINALIZE, "", 0);
      close(sockfd);
      I2AddrFree(sfwsrv_addr);
      log_println(1, " <-------------------------->");
      return 1;
    }
    buff[msgLen] = 0;
    if (strcmp(buff, "Simple firewall test") != 0) {
      log_println(0, "Simple firewall test: Improper message");
      sprintf(buff, "%d", SFW_UNKNOWN);
      send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
      send_msg(ctlsockfd, TEST_FINALIZE, "", 0);
      close(sockfd);
      I2AddrFree(sfwsrv_addr);
      log_println(1, " <-------------------------->");
      return 1;
    }
    
    sprintf(buff, "%d", SFW_NOFIREWALL);
    send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
    send_msg(ctlsockfd, TEST_FINALIZE, "", 0);
    close(sockfd);
    I2AddrFree(sfwsrv_addr);
    log_println(1, " <-------------------------->");
  }
  return 0;
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
  int sfwport, sfwsock;
  I2Addr sfwsrv_addr = NULL;
  
  if (tests & TEST_SFW) {
    log_println(1, " <-- Simple firewall test -->");
    msgLen = sizeof(buff);
    if (recv_msg(ctlsockfd, &msgType, &buff, &msgLen)) {
      log_println(0, "Protocol error!");
      exit(1);
    }
    if (check_msg_type("Simple firewall test", TEST_PREPARE, msgType)) {
      exit(2);
    }
    if (msgLen <= 0) {
      log_println(0, "Improper message");
      exit(3);
    }
    buff[msgLen] = 0;
    if (check_int(buff, &sfwport)) {
      log_println(0, "Invalid port number");
      exit(4);
    }
    log_println(1, "  -- port: %d", sfwport);
    if ((sfwsrv_addr = I2AddrByNode(NULL, host)) == NULL) {
      perror("Unable to resolve server address\n");
      exit(-3);
    }
    I2AddrSetPort(sfwsrv_addr, sfwport);

    if (CreateConnectSocket(&sfwsock, NULL, sfwsrv_addr, conn_options) == 0) {
      send_msg(sfwsock, TEST_MSG, "Simple firewall test", 20);
    }

    msgLen = sizeof(buff);
    if (recv_msg(ctlsockfd, &msgType, &buff, &msgLen)) {
      log_println(0, "Protocol error!");
      exit(1);
    }
    if (check_msg_type("Simple firewall test", TEST_MSG, msgType)) {
      exit(2);
    }
    if (msgLen <= 0) {
      log_println(0, "Improper message");
      exit(3);
    }
    buff[msgLen] = 0;
    if (check_int(buff, &c2s_result)) {
      log_println(0, "Invalid test result");
      exit(4);
    }

    msgLen = sizeof(buff);
    if (recv_msg(ctlsockfd, &msgType, &buff, &msgLen)) {
      log_println(0, "Protocol error!");
      exit(1);
    }
    if (check_msg_type("Simple firewall test", TEST_FINALIZE, msgType)) {
      exit(2);
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
        printf("Server '%s' is not behind a firewall.\n", host);
        break;
      case SFW_POSSIBLE:
        printf("Server '%s' is probably behind a firewall.\n", host);
        break;
      case SFW_UNKNOWN:
      case SFW_NOTTESTED:
        break;
    }
  }
  return 0;
}
