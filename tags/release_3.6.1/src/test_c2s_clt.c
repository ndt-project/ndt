/*
 * This file contains the functions needed to handle C2S throughput
 * test (client part).
 *
 * Jakub S³awiñski 2006-08-02
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
#include "protocol.h"
#include "utils.h"

int pkts, lth;
int sndqueue;
double spdout, c2sspd;

int
test_c2s_clt(int ctlSocket, char tests, char* host, int conn_options, int buf_size)
{
  /* char buff[BUFFSIZE+1]; */
  char buff[64*1024];
  int msgLen, msgType;
  int c2sport = 3002;
  I2Addr sec_addr = NULL;
  int ret, one=1, i, k;
  int outSocket;
  double t, stop_time;

  if (tests & TEST_C2S) {
    struct sigaction new, old;
    log_println(1, " <-- C2S throughput test -->");
    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed prepare message!");
      return 1;
    }
    if (check_msg_type("C2S throughput test", TEST_PREPARE, msgType, buff, msgLen)) {
      return 2;
    }
    if (msgLen <= 0) {
      log_println(0, "Improper message");
      return 3;
    }
    buff[msgLen] = 0;
    if (check_int(buff, &c2sport)) {
      log_println(0, "Invalid port number");
      return 4;
    }
    log_println(1, "  -- port: %d", c2sport);

    if ((sec_addr = I2AddrByNode(get_errhandle(), host)) == NULL) {
      log_println(0, "Unable to resolve server address: %s", strerror(errno));
      return -3;
    }
    I2AddrSetPort(sec_addr, c2sport);

    if ((ret = CreateConnectSocket(&outSocket, NULL, sec_addr, conn_options, buf_size))) {
      log_println(0, "Connect() for client to server failed", strerror(errno));
      return -11;
    }

    setsockopt(outSocket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed start message!");
      return 1;
    }
    if (check_msg_type("C2S throughput test", TEST_START, msgType, buff, msgLen)) {
      return 2;
    }

    printf("running 10s outbound test (client to server) . . . . . ");
    fflush(stdout);

    pkts = 0;
    k = 0;
    for (i=0; i<(64*1024); i++) {
      while (!isprint(k&0x7f))
        k++;
      buff[i] = (k++ % 0x7f);
    }
    t = secs();
    stop_time = t + 10;
    /* ignore the pipe signal */
    memset(&new, 0, sizeof(new));
    new.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &new, &old);
    do {
      write(outSocket, buff, lth);
      pkts++;
    } while (secs() < stop_time);
    sigaction(SIGPIPE, &old, NULL);
    sndqueue = sndq_len(outSocket);
    t = secs() - t;
    I2AddrFree(sec_addr);
    spdout = ((8.0 * pkts * lth) / 1000) / t;

    /* receive the c2sspd from the server */
    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed text message!");
      return 1;
    }
    if (check_msg_type("C2S throughput test", TEST_MSG, msgType, buff, msgLen)) {
      return 2;
    }
    if (msgLen <= 0) { 
      log_println(0, "Improper message");
      return 3;
    }
    buff[msgLen] = 0; 
    c2sspd = atoi(buff);

    if (c2sspd < 1000) 
      printf(" %0.2f kb/s\n", c2sspd);
    else
      printf(" %0.2f Mb/s\n", c2sspd/1000);
    
    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed finalize message!");
      return 1;
    }
    if (check_msg_type("C2S throughput test", TEST_FINALIZE, msgType, buff, msgLen)) {
      return 2;
    }
    log_println(1, " <------------------------->");
  }
  return 0;
}
