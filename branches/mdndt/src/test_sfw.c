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
static int s2c_result = SFW_NOTTESTED;

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
 * Function name: test_osfw_srv
 * Description: Performs the server part of the opposite Simple
 *              firewall test.
 * Arguments: ctlsockfd - the client control socket descriptor
 */

void
test_osfw_srv(int ctlsockfd, web100_agent* agent, int testTime)
{
  char buff[BUFFSIZE+1];
  I2Addr sfwcli_addr = NULL;
  web100_var* var;
  web100_connection* cn;
  web100_group* group;
  int msgLen, msgType;
  int sfwport, sfwsock;
  char* hostname;
  struct sigaction new, old;

  cn = web100_connection_from_socket(agent, ctlsockfd);
  if (cn) {
    web100_agent_find_var_and_group(agent, "RemAddress", &group, &var);
    web100_raw_read(var, cn, buff);
    hostname = web100_value_to_text(web100_get_var_type(var), buff);

    msgLen = sizeof(buff);
    if (recv_msg(ctlsockfd, &msgType, &buff, &msgLen)) {
      log_println(0, "Protocol error!");
      send_msg(ctlsockfd, TEST_FINALIZE, "", 0);
      log_println(1, " <-------------------------->");
      return;
    }
    if (check_msg_type("Simple firewall test", TEST_MSG, msgType)) {
      send_msg(ctlsockfd, TEST_FINALIZE, "", 0);
      log_println(1, " <-------------------------->");
      return;
    }
    if (msgLen <= 0) {
      log_println(0, "Improper message");
      send_msg(ctlsockfd, TEST_FINALIZE, "", 0);
      log_println(1, " <-------------------------->");
      return;
    }
    buff[msgLen] = 0;
    if (check_int(buff, &sfwport)) {
      log_println(0, "Invalid port number");
      send_msg(ctlsockfd, TEST_FINALIZE, "", 0);
      log_println(1, " <-------------------------->");
      return;
    }
   
    if ((sfwcli_addr = I2AddrByNode(NULL, hostname)) == NULL) {
      log_println(0, "Unable to resolve server address");
      send_msg(ctlsockfd, TEST_FINALIZE, "", 0);
      log_println(1, " <-------------------------->");
      return;
    }
    I2AddrSetPort(sfwcli_addr, sfwport);
    
    /* ignore the alrm signal */
    memset(&new, 0, sizeof(new));
    new.sa_handler = catch_alrm;
    sigaction(SIGALRM, &new, &old);
    alarm(testTime + 1);
    if (CreateConnectSocket(&sfwsock, NULL, sfwcli_addr, 0) == 0) {
      send_msg(sfwsock, TEST_MSG, "Simple firewall test", 20);
    }
    alarm(0);
    sigaction(SIGPIPE, &old, NULL);
  }
  else {
    log_println(0, "Simple firewall test: Cannot find connection");
  }
    
  send_msg(ctlsockfd, TEST_FINALIZE, "", 0);
  log_println(1, " <-------------------------->");
}

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
test_sfw_srv(int ctlsockfd, web100_agent* agent, TestOptions* options, int conn_options)
{
  char buff[BUFFSIZE+1];
  I2Addr sfwsrv_addr = NULL;
  int sfwsockfd, sfwsockport, sockfd;
  struct sockaddr_storage cli_addr;
  socklen_t clilen;
  fd_set fds;
  struct timeval sel_tv;
  int msgLen, msgType;
  web100_var* var;
  web100_connection* cn;
  web100_group* group;
  int testTime = 30;
  int maxRTT, maxRTO;
  
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
    
    cn = web100_connection_from_socket(agent, ctlsockfd);
    if (cn) {
      web100_agent_find_var_and_group(agent, "MaxRTT", &group, &var);
      web100_raw_read(var, cn, buff);
      maxRTT = atoi(web100_value_to_text(web100_get_var_type(var), buff));
      web100_agent_find_var_and_group(agent, "MaxRTO", &group, &var);
      web100_raw_read(var, cn, buff);
      maxRTO = atoi(web100_value_to_text(web100_get_var_type(var), buff));
      if (maxRTT > maxRTO)
        maxRTO = maxRTT;
      if (((4.0 * ((double) maxRTO) / 1000.0) + 1) < 30.0)
        testTime = (4.0 * ((double) maxRTO) / 1000.0) + 1;
    }
    log_println(1, "  -- time: %d", testTime);
    
    sprintf(buff, "%d %d", sfwsockport, testTime);
    send_msg(ctlsockfd, TEST_PREPARE, buff, strlen(buff));
    
    FD_ZERO(&fds);
    FD_SET(sfwsockfd, &fds);
    sel_tv.tv_sec = testTime;
    sel_tv.tv_usec = 0;
    switch (select(sfwsockfd+1, &fds, NULL, NULL, &sel_tv)) {
      case -1:
        log_println(0, "Simple firewall test: select exited with error");
        sprintf(buff, "%d", SFW_UNKNOWN);
        send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
        I2AddrFree(sfwsrv_addr);
        test_osfw_srv(ctlsockfd, agent, testTime);
        return 1;
      case 0:
        log_println(0, "Simple firewall test: no connection for %d seconds", testTime);
        sprintf(buff, "%d", SFW_POSSIBLE);
        send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
        I2AddrFree(sfwsrv_addr);
        test_osfw_srv(ctlsockfd, agent, testTime);
        return 2;
    }
    clilen = sizeof(cli_addr);
    sockfd = accept(sfwsockfd, (struct sockaddr *) &cli_addr, &clilen);

    if (recv_msg(sockfd, &msgType, &buff, &msgLen)) {
      log_println(0, "Simple firewall test: unrecognized message");
      sprintf(buff, "%d", SFW_UNKNOWN);
      send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
      close(sockfd);
      I2AddrFree(sfwsrv_addr);
      test_osfw_srv(ctlsockfd, agent, testTime);
      return 1;
    }
    if (check_msg_type("Simple firewall test", TEST_MSG, msgType)) {
      sprintf(buff, "%d", SFW_UNKNOWN);
      send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
      close(sockfd);
      I2AddrFree(sfwsrv_addr);
      test_osfw_srv(ctlsockfd, agent, testTime);
      return 1;
    }
    if (msgLen != 20) {
      log_println(0, "Simple firewall test: Improper message");
      sprintf(buff, "%d", SFW_UNKNOWN);
      send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
      close(sockfd);
      I2AddrFree(sfwsrv_addr);
      test_osfw_srv(ctlsockfd, agent, testTime);
      return 1;
    }
    buff[msgLen] = 0;
    if (strcmp(buff, "Simple firewall test") != 0) {
      log_println(0, "Simple firewall test: Improper message");
      sprintf(buff, "%d", SFW_UNKNOWN);
      send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
      close(sockfd);
      I2AddrFree(sfwsrv_addr);
      test_osfw_srv(ctlsockfd, agent, testTime);
      return 1;
    }
    
    sprintf(buff, "%d", SFW_NOFIREWALL);
    send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
    close(sockfd);
    I2AddrFree(sfwsrv_addr);
    test_osfw_srv(ctlsockfd, agent, testTime);
  }
  return 0;
}

/*
 * Function name: test_osfw_clt
 * Description: Performs the client part of the opposite Simple
 *              firewall test.
 * Arguments: ctlsockfd - the server control socket descriptor
 *            conn_options - the connection options
 */

void
test_osfw_clt(int ctlsockfd, int testTime, int conn_options)
{
  char buff[BUFFSIZE+1];
  I2Addr sfwcli_addr = NULL;
  int sfwsockfd, sfwsockport, sockfd;
  fd_set fds;
  struct timeval sel_tv;
  int msgLen, msgType;
  struct sockaddr_storage srv_addr;
  socklen_t srvlen;

  sfwcli_addr = CreateListenSocket(NULL, "0", conn_options);
  if (sfwcli_addr == NULL) {
    err_sys("client: CreateListenSocket failed");
  }
  sfwsockfd = I2AddrFD(sfwcli_addr);
  sfwsockport = I2AddrPort(sfwcli_addr);
  log_println(1, "  -- oport: %d", sfwsockport);

  sprintf(buff, "%d", sfwsockport);
  send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));

  FD_ZERO(&fds);
  FD_SET(sfwsockfd, &fds);
  sel_tv.tv_sec = testTime;
  sel_tv.tv_usec = 0;
  switch (select(sfwsockfd+1, &fds, NULL, NULL, &sel_tv)) {
    case -1:
      log_println(0, "Simple firewall test: select exited with error");
      I2AddrFree(sfwcli_addr);
      return;
    case 0:
      log_println(1, "Simple firewall test: no connection for %d seconds", testTime);
      s2c_result = SFW_POSSIBLE;
      I2AddrFree(sfwcli_addr);
      return;
  }
  srvlen = sizeof(srv_addr);
  sockfd = accept(sfwsockfd, (struct sockaddr *) &srv_addr, &srvlen);

  msgLen = sizeof(buff);
  if (recv_msg(sockfd, &msgType, &buff, &msgLen)) {
    log_println(0, "Simple firewall test: unrecognized message");
    s2c_result = SFW_UNKNOWN;
    close(sockfd);
    I2AddrFree(sfwcli_addr);
    return;
  }
  if (check_msg_type("Simple firewall test", TEST_MSG, msgType)) {
    s2c_result = SFW_UNKNOWN;
    close(sockfd);
    I2AddrFree(sfwcli_addr);
    return;
  }
  if (msgLen != 20) {
    log_println(0, "Simple firewall test: Improper message");
    s2c_result = SFW_UNKNOWN;
    close(sockfd);
    I2AddrFree(sfwcli_addr);
    return;
  }
  buff[msgLen] = 0;
  if (strcmp(buff, "Simple firewall test") != 0) {
    log_println(0, "Simple firewall test: Improper message");
    s2c_result = SFW_UNKNOWN;
    close(sockfd);
    I2AddrFree(sfwcli_addr);
    return;
  }

  s2c_result = SFW_NOFIREWALL;
  close(sockfd);
  I2AddrFree(sfwcli_addr);
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
  int testTime;
  I2Addr sfwsrv_addr = NULL;
  struct sigaction new, old;
  char* ptr;
  
  if (tests & TEST_SFW) {
    log_println(1, " <-- Simple firewall test -->");
    printf("checking for firewalls . . . . . . . . . . . . . . . . . . .  ");
    fflush(stdout);
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
    ptr = strtok(buff, " ");
    if (check_int(ptr, &sfwport)) {
      log_println(0, "Invalid port number");
      exit(4);
    }
    ptr = strtok(NULL, " ");
    if (check_int(ptr, &testTime)) {
      log_println(0, "Invalid waiting time");
      exit(4);
    }
    log_println(1, "  -- port: %d", sfwport);
    log_println(1, "  -- time: %d", testTime);
    if ((sfwsrv_addr = I2AddrByNode(NULL, host)) == NULL) {
      perror("Unable to resolve server address\n");
      exit(-3);
    }
    I2AddrSetPort(sfwsrv_addr, sfwport);

    /* ignore the alrm signal */
    memset(&new, 0, sizeof(new));
    new.sa_handler = catch_alrm;
    sigaction(SIGALRM, &new, &old);
    alarm(testTime + 1);
    if (CreateConnectSocket(&sfwsock, NULL, sfwsrv_addr, conn_options) == 0) {
      send_msg(sfwsock, TEST_MSG, "Simple firewall test", 20);
    }
    alarm(0);
    sigaction(SIGPIPE, &old, NULL);

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

    test_osfw_clt(ctlsockfd, testTime, conn_options);
    
    printf("Done\n");
    
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
    switch (s2c_result) {
      case SFW_NOFIREWALL:
        printf("Client is not behind a firewall.\n");
        break;
      case SFW_POSSIBLE:
        printf("Client is probably behind a firewall.\n");
        break;
      case SFW_UNKNOWN:
      case SFW_NOTTESTED:
        break;
    }
  }
  return 0;
}
