/*
 * This file contains the functions needed to handle S2C throughput
 * test (client part).
 *
 * Jakub S³awiñski 2006-08-02
 * jeremian@poczta.fm
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "clt_tests.h"
#include "logging.h"
#include "network.h"
#include "protocol.h"
#include "utils.h"

int ssndqueue, sbytes;
double spdin, s2cspd;

int
test_s2c_clt(int ctlSocket, char tests, char* host, int conn_options, int buf_size, char* tmpstr)
{
  char buff[BUFFSIZE+1];
  int msgLen, msgType;
  int s2cport = 3003;
  I2Addr sec_addr = NULL;
  int inlth, ret, one=1, set_size;
  int inSocket;
  socklen_t optlen;
  uint32_t bytes;
  double t;
  struct timeval sel_tv;
  fd_set rfd;
  char* ptr;
  
  if (tests & TEST_S2C) {
    log_println(1, " <-- S2C throughput test -->");
    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed prepare message!");
      return 1;
    }
    if (check_msg_type("S2C throughput test", TEST_PREPARE, msgType, buff, msgLen)) {
      return 2;
    }
    if (msgLen <= 0) {
      log_println(0, "Improper message");
      return 3;
    }
    buff[msgLen] = 0;
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

    if ((sec_addr = I2AddrByNode(get_errhandle(), host)) == NULL) {
      log_println(0, "Unable to resolve server address: %s", strerror(errno));
      return -3;
    }
    I2AddrSetPort(sec_addr, s2cport);

    if ((ret = CreateConnectSocket(&inSocket, NULL, sec_addr, conn_options, buf_size))) {
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
    
    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed start message!");
      return 1;
    }
    if (check_msg_type("S2C throughput test", TEST_START, msgType, buff, msgLen)) {
      return 2;
    }

    printf("running 10s inbound test (server to client) . . . . . . ");
    fflush(stdout);

    bytes = 0;
    t = secs() + 15.0;
    sel_tv.tv_sec = 15;
    sel_tv.tv_usec = 5;
    FD_ZERO(&rfd);
    FD_SET(inSocket, &rfd);
    for (;;) {
      ret = select(inSocket+1, &rfd, NULL, NULL, &sel_tv);
      if (secs() > t) {
        log_println(5, "Receive test running long, break out of read loop");
        break;
      }
      if (ret > 0) {
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
    t = secs() - t + 15.0;
    spdin = ((8.0 * bytes) / 1000) / t;

    /* receive the s2cspd from the server */
    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed text message!");
      return 1;
    }
    if (check_msg_type("S2C throughput test", TEST_MSG, msgType, buff, msgLen)) {
      return 2;
    }
    if (msgLen <= 0) { 
      log_println(0, "Improper message");
      return 3;
    }
    buff[msgLen] = 0; 
    ptr = strtok(buff, " ");
    if (ptr == NULL) {
      log_println(0, "S2C: Improper message");
      return 4;
    }
    s2cspd = atoi(ptr);
    ptr = strtok(NULL, " ");
    if (ptr == NULL) {
      log_println(0, "S2C: Improper message");
      return 4;
    }
    ssndqueue = atoi(ptr);
    ptr = strtok(NULL, " ");
    if (ptr == NULL) {
      log_println(0, "S2C: Improper message");
      return 4;
    }
    sbytes = atoi(ptr);
    
    if (spdin < 1000)
      printf("%0.2f kb/s\n", spdin);
    else
      printf("%0.2f Mb/s\n", spdin/1000);

    I2AddrFree(sec_addr);

    sprintf(buff, "%0.0f", spdin);
    send_msg(ctlSocket, TEST_MSG, buff, strlen(buff));
    
    /* get web100 variables from server */
    tmpstr[0] = '\0';
    for (;;) {
      msgLen = sizeof(buff);
      if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
        log_println(0, "Protocol error - missed text/finalize message!");
        return 1;
      }
      if (msgType == TEST_FINALIZE) {
        break;
      }
      if (check_msg_type("S2C throughput test", TEST_MSG, msgType, buff, msgLen)) {
        return 2;
      }
      strncat(tmpstr, buff, msgLen);
      log_println(6, "tmpstr = '%s'", tmpstr);
    }
    log_println(1, " <------------------------->");
  }


  return 0;
}
