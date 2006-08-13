/*
 * This file contains the functions needed to handle Middlebox
 * test (client part).
 *
 * Jakub S³awiñski 2006-08-02
 * jeremian@poczta.fm
 */

#include "clt_tests.h"
#include "logging.h"
#include "network.h"
#include "protocol.h"
#include "utils.h"

int
test_mid_clt(int ctlSocket, char tests, char* host, int conn_options, int buf_size, char* tmpstr2)
{
  char buff[BUFFSIZE+1];
  int msgLen, msgType;
  int midport = 3003;
  I2Addr sec_addr = NULL;
  int ret, one=1, i, inlth;
  int largewin;
  int set_size;
  int in2Socket;
  socklen_t optlen;
  double t, spdin;
  uint32_t bytes;
  struct timeval sel_tv;
  fd_set rfd;

  if (tests & TEST_MID) {
    log_println(1, " <-- Middlebox test -->");
    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, &buff, &msgLen)) {
      log_println(0, "Protocol error!");
      exit(1);
    }
    if (check_msg_type("Middlebox test", TEST_PREPARE, msgType)) {
      exit(2);
    }
    if (msgLen <= 0) {
      log_println(0, "Improper message");
      exit(3);
    }
    buff[msgLen] = 0;
    if (check_int(buff, &midport)) {
      log_println(0, "Invalid port number");
      exit(4);
    }
    log_println(1, "  -- port: %d", midport);
    if ((sec_addr = I2AddrByNode(get_errhandle(), host)) == NULL) {
      perror("Unable to resolve server address\n");
      exit(-3);
    }
    I2AddrSetPort(sec_addr, midport);

    if (get_debuglvl() > 4) {
      char tmpbuff[200];
      size_t tmpBufLen = 199;
      memset(tmpbuff, 0, 200);
      I2AddrNodeName(sec_addr, tmpbuff, &tmpBufLen);
      log_println(5, "connecting to %s:%d", tmpbuff, I2AddrPort(sec_addr));
    }

    if ((ret = CreateConnectSocket(&in2Socket, NULL, sec_addr, conn_options))) {
      perror("Connect() for middlebox failed");
      exit(-10);
    }

    largewin = 128*1024;
    optlen = sizeof(set_size);
    setsockopt(in2Socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    getsockopt(in2Socket, SOL_SOCKET, SO_SNDBUF, &set_size, &optlen);
    log_print(5, "\nSend buffer set to %d, ", set_size);
    getsockopt(in2Socket, SOL_SOCKET, SO_RCVBUF, &set_size, &optlen);
    log_println(5, "Receive buffer set to %d", set_size);
    if (buf_size > 0) {
      setsockopt(in2Socket, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size));
      setsockopt(in2Socket, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(buf_size));
      getsockopt(in2Socket, SOL_SOCKET, SO_SNDBUF, &set_size, &optlen);
      log_print(5, "Changed buffer sizes: Send buffer set to %d, ", set_size);
      getsockopt(in2Socket, SOL_SOCKET, SO_RCVBUF, &set_size, &optlen);
      log_println(5, "Receive buffer set to %d", set_size);
    }

    printf("Checking for Middleboxes . . . . . . . . . . . . . . . . . .  ");
    fflush(stdout);
    tmpstr2[0] = '\0';
    i = 0;
    bytes = 0;
    t = secs() + 5.0;
    sel_tv.tv_sec = 6;
    sel_tv.tv_usec = 5;
    FD_ZERO(&rfd);
    FD_SET(in2Socket, &rfd);
    for (;;) {
      if (secs() > t)
        break;
      ret = select(in2Socket+1, &rfd, NULL, NULL, &sel_tv);
      if (ret > 0) {
        inlth = read(in2Socket, buff, sizeof(buff));
        if (inlth == 0)
          break;
        bytes += inlth;
        continue;
      }
      if (ret < 0) {
        printf("nothing to read, exiting read loop\n");
        break;
      }
      if (ret == 0) {
        printf("timer expired, exiting read loop\n");
        break;
      }
    }
    shutdown(in2Socket, SHUT_RD);
    t =  secs() - t + 5.0;
    spdin = ((8.0 * bytes) / 1000) / t;

    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, &buff, &msgLen)) {
      log_println(0, "Protocol error!");
      exit(1);
    }
    if (check_msg_type("Middlebox test results", TEST_MSG, msgType)) {
      exit(2);
    }
    strncat(tmpstr2, buff, msgLen);

    memset(buff, 0, 128);
    sprintf(buff, "%0.0f", spdin);
    log_println(4, "CWND limited speed = %0.2f Kbps", spdin);
    send_msg(ctlSocket, TEST_MSG, buff, strlen(buff));
    printf("Done\n");

    I2AddrFree(sec_addr);

    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, &buff, &msgLen)) {
      log_println(0, "Protocol error!");
      exit(1);
    }
    if (check_msg_type("Middlebox test", TEST_FINALIZE, msgType)) {
      exit(2);
    }
    log_println(1, " <-------------------->");
  }
  return 0;
}
