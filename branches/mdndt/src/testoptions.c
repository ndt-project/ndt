/*
 * This file contains the functions needed to handle various tests.
 *
 * Jakub S³awiñski 2006-06-24
 * jeremian@poczta.fm
 */

#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "testoptions.h"
#include "network.h"
#include "mrange.h"
#include "logging.h"
#include "utils.h"
#include "protocol.h"

int mon_pipe1[2], mon_pipe2[2];

/*
 * Function name: initialize_tests
 * Description: Initializes the tests for the client.
 * Arguments: ctlsockfd - the client control socket descriptor
 *            options - the test options
 *            conn_options - the connection options
 * Returns: 0 - success,
 *          >0 - error code.
 */

int
initialize_tests(int ctlsockfd, TestOptions* options, int conn_options)
{
  unsigned char useropt;
  int msgType;
  int msgLen = 1;

  assert(ctlsockfd != -1);
  assert(options);

  /* read the test suite request */
  if (recv_msg(ctlsockfd, &msgType, &useropt, &msgLen)) {
    return 1;
  }
  if ((msgType != MSG_LOGIN) || (msgLen != 1)) {
    return 2;
  }
  if (!(useropt & (TEST_MID | TEST_C2S | TEST_S2C | TEST_SFW))) {
    return 3;
  }
  if (useropt & TEST_MID) {
    options->midopt = TOPT_ENABLED;
  }
  if (useropt & TEST_C2S) {
    options->c2sopt = TOPT_ENABLED;
  }
  if (useropt & TEST_S2C) {
    options->s2copt = TOPT_ENABLED;
  }
  if (useropt & TEST_SFW) {
    options->sfwopt = TOPT_ENABLED;
  }
  return 0;
}

/*
 * Function name: test_mid
 * Description: Performs the Middlebox test.
 * Arguments: ctlsockfd - the client control socket descriptor
 *            agent - the Web100 agent used to track the connection
 *            options - the test options
 *            s2c2spd - the S2C throughput results (evaluated by the MID TEST)
 *            conn_options - the connection options
 * Returns: 0 - success,
 *          >0 - error code.
 */

int
test_mid(int ctlsockfd, web100_agent* agent, TestOptions* options, int conn_options, double* s2c2spd)
{
  int maxseg=1456, largewin=16*1024*1024;
  int seg_size, win_size;
  int midfd;
  struct sockaddr_storage cli_addr;
  socklen_t optlen, clilen;
  char buff[BUFFSIZE+1];
  I2Addr midsrv_addr = NULL;
  char listenmidport[10];
  int msgType;
  int msgLen;
  
  assert(ctlsockfd != -1);
  assert(agent);
  assert(options);
  assert(s2c2spd);
  
  if (options->midopt) {
    log_println(1, " <-- Middlebox test -->");
    strcpy(listenmidport, PORT3);

    if (options->midsockport) {
      sprintf(listenmidport, "%d", options->midsockport);
    }
    else if (options->mainport) {
      sprintf(listenmidport, "%d", options->mainport + 2);
    }
    
    if (options->multiple) {
      strcpy(listenmidport, "0");
    }
    
    while (midsrv_addr == NULL) {
      midsrv_addr = CreateListenSocket(NULL,
          (options->multiple ? mrange_next(listenmidport) : listenmidport), conn_options);
      if (strcmp(listenmidport, "0") == 0) {
        log_println(0, "WARNING: ephemeral port number was bound");
        break;
      }
      if (options->multiple == 0) {
        break;
      }
    }
    if (midsrv_addr == NULL) {
      err_sys("server: CreateListenSocket failed");
    }
    options->midsockfd = I2AddrFD(midsrv_addr);
    options->midsockport = I2AddrPort(midsrv_addr);
    log_println(1, "  -- port: %d", options->midsockport);
    
    sprintf(buff, "%d", options->midsockport);
    send_msg(ctlsockfd, TEST_PREPARE, buff, strlen(buff));
    
    /* set mss to 1456 (strange value), and large snd/rcv buffers
     * should check to see if server supports window scale ?
     */
    setsockopt(options->midsockfd, SOL_TCP, TCP_MAXSEG, &maxseg, sizeof(maxseg));
    setsockopt(options->midsockfd, SOL_SOCKET, SO_SNDBUF, &largewin, sizeof(largewin));
    setsockopt(options->midsockfd, SOL_SOCKET, SO_RCVBUF, &largewin, sizeof(largewin));
    log_println(2, "Middlebox test, Port %d waiting for incoming connection (fd=%d)",
        options->midsockport, options->midsockfd);
    if (get_debuglvl() > 1) {
      optlen = sizeof(seg_size);
      getsockopt(options->midsockfd, SOL_TCP, TCP_MAXSEG, &seg_size, &optlen);
      getsockopt(options->midsockfd, SOL_SOCKET, SO_RCVBUF, &win_size, &optlen);
      log_println(2, "Set MSS to %d, Receiving Window size set to %dKB", seg_size, win_size);
      getsockopt(options->midsockfd, SOL_SOCKET, SO_SNDBUF, &win_size, &optlen);
      log_println(2, "Sending Window size set to %dKB", win_size);
    }

    /* Post a listen on port 3003.  Client will connect here to run the 
     * middlebox test.  At this time it really only checks the MSS value
     * and does NAT detection.  More analysis functions (window scale)
     * will be done in the future.
     */
    clilen = sizeof(cli_addr);
    midfd = accept(options->midsockfd, (struct sockaddr *) &cli_addr, &clilen);

    buff[0] = '\0';
    web100_middlebox(midfd, agent, buff);
    send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
    msgLen = sizeof(buff);
    if (recv_msg(ctlsockfd, &msgType, &buff, &msgLen)) {
      log_println(0, "Protocol error!");
      exit(1);
    }
    if (check_msg_type("Middlebox test", TEST_MSG, msgType)) {
      exit(2);
    }
    if (msgLen <= 0) {
      log_println(0, "Improper message");
      exit(3);
    }
    buff[msgLen] = 0;
    *s2c2spd = atof(buff);
    log_println(4, "CWND limited throughput = %0.0f kbps (%s)", *s2c2spd, buff);
    shutdown(midfd, SHUT_WR);
    close(midfd);
    close(options->midsockfd);
    send_msg(ctlsockfd, TEST_FINALIZE, "", 0);
    log_println(1, " <-------------------->");
  }
  return 0;
}

/*
 * Function name: test_c2s
 * Description: Performs the C2S Throughput test.
 * Arguments: ctlsockfd - the client control socket descriptor
 *            agent - the Web100 agent used to track the connection
 *            options - the test options
 *            conn_options - the connection options
 * Returns: 0 - success,
 *          >0 - error code.
 */

int
test_c2s(int ctlsockfd, web100_agent* agent, TestOptions* options, int conn_options, double* c2sspd,
    int set_buff, int window, int autotune, char* device, int limit,
    int record_reverse, int count_vars, char spds[4][256], int* spd_index)
{
  int largewin=16*1024*1024;
  int recvsfd;
  int mon_pid1 = 0;
  int ret, n;
  struct sockaddr_storage cli_addr;
  socklen_t clilen;
  char tmpstr[256];
  double t, bytes=0;
  struct timeval sel_tv;
  fd_set rfd;
  char buff[BUFFSIZE+1];
  PortPair pair;
  I2Addr c2ssrv_addr = NULL;
  char listenc2sport[10];

  web100_group* group;
  web100_connection* conn;

  if (options->c2sopt) {
    log_println(1, " <-- C2S throughput test -->");
    strcpy(listenc2sport, PORT2);
    
    if (options->c2ssockport) {
      sprintf(listenc2sport, "%d", options->c2ssockport);
    }
    else if (options->mainport) {
      sprintf(listenc2sport, "%d", options->mainport + 1);
    }
    
    if (options->multiple) {
      strcpy(listenc2sport, "0");
    }
    
    while (c2ssrv_addr == NULL) {
      c2ssrv_addr = CreateListenSocket(NULL,
          (options->multiple ? mrange_next(listenc2sport) : listenc2sport), conn_options);
      if (strcmp(listenc2sport, "0") == 0) {
        log_println(0, "WARNING: ephemeral port number was bound");
        break;
      }
      if (options->multiple == 0) {
        break;
      }
    }
    if (c2ssrv_addr == NULL) {
      err_sys("server: CreateListenSocket failed");
    }
    options->c2ssockfd = I2AddrFD(c2ssrv_addr);
    options->c2ssockport = I2AddrPort(c2ssrv_addr);
    log_println(1, "  -- port: %d", options->c2ssockport);
    pair.port1 = options->c2ssockport;
    pair.port2 = -1;
    
    if (set_buff > 0) {
      setsockopt(options->c2ssockfd, SOL_SOCKET, SO_SNDBUF, &window, sizeof(window));
      setsockopt(options->c2ssockfd, SOL_SOCKET, SO_RCVBUF, &window, sizeof(window));
    }
    if (autotune > 0) {
      setsockopt(options->c2ssockfd, SOL_SOCKET, SO_SNDBUF, &largewin, sizeof(largewin));
      setsockopt(options->c2ssockfd, SOL_SOCKET, SO_RCVBUF, &largewin, sizeof(largewin));
    }
    log_println(1, "listening for Inet connection on options->c2ssockfd, fd=%d", options->c2ssockfd);

    log_println(1, "Sending 'GO' signal, to tell client to head for the next test");
    sprintf(buff, "%d", options->c2ssockport);
    send_msg(ctlsockfd, TEST_PREPARE, buff, strlen(buff));

    clilen = sizeof(cli_addr);
    recvsfd = accept(options->c2ssockfd, (struct sockaddr *) &cli_addr, &clilen);

    if (getuid() == 0) {
      pipe(mon_pipe1);
      if ((mon_pid1 = fork()) == 0) {
        close(ctlsockfd);
        close(options->c2ssockfd);
        close(recvsfd);
        log_println(5, "C2S test Child thinks pipe() returned fd0=%d, fd1=%d", mon_pipe1[0], mon_pipe1[1]);
        log_println(2, "C2S test calling init_pkttrace() with pd=0x%x", (int) &cli_addr);
        init_pkttrace((struct sockaddr *) &cli_addr, clilen, mon_pipe1, device, &pair);
        exit(0);    /* Packet trace finished, terminate gracefully */
      }
      if (read(mon_pipe1[0], tmpstr, 128) <= 0) {
        log_println(0, "error & exit");
        exit(0);
      }
    }

    log_println(5, "C2S test Parent thinks pipe() returned fd0=%d, fd1=%d", mon_pipe1[0], mon_pipe1[1]);

    /* Check, and if needed, set the web100 autotuning function on.  This improves
     * performance without requiring the entire system be re-configured.  Returned
     * value is true if set is done, so admin knows if system default should be changed
     *
     * 11/22/04, the sBufMode and rBufMode per connection autotuning variables are being
     * depricated, so this is another attempt at making this work.  If window scaling is
     * enabled, then scale the buffer size to the correct value.  This will also help
     * when reporting the buffer / RTT limit.
     */

    if (autotune > 0) 
      web100_setbuff(recvsfd, agent, autotune);

    /* ok, We are now read to start the throughput tests.  First
     * the client will stream data to the server for 10 seconds.
     * Data will be processed by the read loop below.
     */

    /* experimental code, delete when finished */
    {
      web100_var *LimRwin, *yar;
      u_int32_t limrwin_val;
      char yuff[32];

      if (limit > 0) {
        log_print(0, "Setting Cwnd limit - ");
        if ((conn = web100_connection_from_socket(agent, recvsfd)) != NULL) {
          web100_agent_find_var_and_group(agent, "CurMSS", &group, &yar);
          web100_raw_read(yar, conn, yuff);
          log_println(0, "MSS = %s, multiplication factor = %d",
              web100_value_to_text(web100_get_var_type(yar), yuff), limit);
          limrwin_val = limit * (atoi(web100_value_to_text(web100_get_var_type(yar), yuff)));
          web100_agent_find_var_and_group(agent, "LimRwin", &group, &LimRwin);
          log_print(0, "now write %d to limit the Receive window", limrwin_val);
          web100_raw_write(LimRwin, conn, &limrwin_val);
          log_println(0, "  ---  Done");
        }
      }
    }
    /* End of test code */

    sleep(2);
    send_msg(ctlsockfd, TEST_START, "", 0);
    alarm(30);  /* reset alarm() again, this 10 sec test should finish before this signal
                 * is generated.  */


    t = secs();
    sel_tv.tv_sec = 11;
    sel_tv.tv_usec = 0;
    FD_ZERO(&rfd);
    FD_SET(recvsfd, &rfd);
    for (;;) {
      ret = select(recvsfd+1, &rfd, NULL, NULL, &sel_tv);
      if (ret > 0) {
        n = read(recvsfd, buff, sizeof(buff));
        if (n == 0)
          break;
        bytes += n;
        continue;
      }
      break;
    }

    t = secs()-t;
    *c2sspd = (8.e-3 * bytes) / t;
    sprintf(buff, "%6.0f Kbs outbound", *c2sspd);
    log_println(1, "%s", buff);
    /* send the c2sspd to the client */
    sprintf(buff, "%0.0f", *c2sspd);
    send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
    /* get receiver side Web100 stats and write them to the log file */
    if (record_reverse == 1)
      web100_get_data_recv(recvsfd, agent, count_vars);
    shutdown(recvsfd, SHUT_RD);
    close(recvsfd);
    close(options->c2ssockfd);

    /* Next send speed-chk a flag to retrieve the data it collected.
     * Skip this step if speed-chk isn't running.
     */

    if (getuid() == 0) {
      log_println(1, "Signal USR1(%d) sent to child [%d]", SIGUSR1, mon_pid1);
      kill(mon_pid1, SIGUSR1);
      FD_ZERO(&rfd);
      FD_SET(mon_pipe1[0], &rfd);
      sel_tv.tv_sec = 1;
      sel_tv.tv_usec = 100000;
read3:
      if ((select(32, &rfd, NULL, NULL, &sel_tv)) > 0) {
        if ((ret = read(mon_pipe1[0], spds[*spd_index], 128)) < 0)
          sprintf(spds[*spd_index], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0");
        log_println(1, "%d bytes read '%s' from monitor pipe", ret, spds[*spd_index]);
        (*spd_index)++;
        if ((ret = read(mon_pipe1[0], spds[*spd_index], 128)) < 0)
          sprintf(spds[*spd_index], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0");
        log_println(1, "%d bytes read '%s' from monitor pipe", ret, spds[*spd_index]);
        (*spd_index)++;
      } else {
        if (errno == EINTR)
          goto read3;
        log_println(4, "Failed to read pkt-pair data from C2S flow, reason = %d", errno);
        sprintf(spds[(*spd_index)++], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0");
        sprintf(spds[(*spd_index)++], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0");
      }
    } else {
      sprintf(spds[(*spd_index)++], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0");
      sprintf(spds[(*spd_index)++], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0");
    }

    send_msg(ctlsockfd, TEST_FINALIZE, "", 0);

    if (getuid() == 0) {
      write(mon_pipe1[1], "", 1);
      close(mon_pipe1[0]);
      close(mon_pipe1[1]);
    }
    /* we should wait for the SIGCHLD signal here */
    wait(NULL);
    
    log_println(1, " <------------------------->");
  }
  return 0;
}

/*
 * Function name: test_s2c
 * Description: Performs the S2C Throughput test.
 * Arguments: ctlsockfd - the client control socket descriptor
 *            agent - the Web100 agent used to track the connection
 *            options - the test options
 *            conn_options - the connection options
 * Returns: 0 - success,
 *          >0 - error code.
 */

int
test_s2c(int ctlsockfd, web100_agent* agent, TestOptions* options, int conn_options, double* s2cspd,
    int set_buff, int window, int autotune, char* device, int limit,
    int experimental, char* logname, char spds[4][256], int* spd_index, int count_vars)
{
  int largewin=16*1024*1024;
  int ret, j, k, n;
  int xmitsfd;
  int mon_pid2 = 0;
  char tmpstr[256];
  struct sockaddr_storage cli_addr;
  socklen_t clilen;
  double bytes, s, t;
  double x2cspd;
  struct timeval sel_tv;
  fd_set rfd;
  char buff[BUFFSIZE+1];
  int c3 = 0;
  PortPair pair;
  I2Addr s2csrv_addr = NULL;
  char listens2cport[10];
  int msgType;
  int msgLen;
  int sndqueue;
  
  /* experimental code to capture and log multiple copies of the
   * web100 variables using the web100_snap() & log() functions.
   */
  web100_snapshot* tsnap = NULL;
  web100_snapshot* rsnap = NULL;
  web100_group* group;
  web100_group* tgroup;
  web100_group* rgroup;
  web100_connection* conn;
  web100_connection* xconn;
  web100_snapshot* snap = NULL;
  web100_log* log = NULL;
  web100_var* var;
  int SndMax=0, SndUna=0;
  int c1=0, c2=0;
  
  if (options->s2copt) {
    log_println(1, " <-- S2C throughput test -->");
    strcpy(listens2cport, PORT4);
    
    if (options->s2csockport) {
      sprintf(listens2cport, "%d", options->s2csockport);
    }
    else if (options->mainport) {
      sprintf(listens2cport, "%d", options->mainport + 2);
    }
    
    if (options->multiple) {
      strcpy(listens2cport, "0");
    }
    
    while (s2csrv_addr == NULL) {
      s2csrv_addr = CreateListenSocket(NULL,
          (options->multiple ? mrange_next(listens2cport) : listens2cport), conn_options);
      if (strcmp(listens2cport, "0") == 0) {
        log_println(0, "WARNING: ephemeral port number was bound");
        break;
      }
      if (options->multiple == 0) {
        break;
      }
    }
    if (s2csrv_addr == NULL) {
      err_sys("server: CreateListenSocket failed");
    }
    options->s2csockfd = I2AddrFD(s2csrv_addr);
    options->s2csockport = I2AddrPort(s2csrv_addr);
    log_println(1, "  -- port: %d", options->s2csockport);
    pair.port1 = -1;
    pair.port2 = options->s2csockport;
    
    if (set_buff > 0) {
      setsockopt(options->s2csockfd, SOL_SOCKET, SO_SNDBUF, &window, sizeof(window));
      setsockopt(options->s2csockfd, SOL_SOCKET, SO_RCVBUF, &window, sizeof(window));
    }
    if (autotune > 0) {
      setsockopt(options->s2csockfd, SOL_SOCKET, SO_SNDBUF, &largewin, sizeof(largewin));
      setsockopt(options->s2csockfd, SOL_SOCKET, SO_RCVBUF, &largewin, sizeof(largewin));
    }
    log_println(1, "listening for Inet connection on options->s2csockfd, fd=%d", options->s2csockfd);

    /* Data received from speed-chk, tell applet to start next test */
    sprintf(buff, "%d", options->s2csockport);
    send_msg(ctlsockfd, TEST_PREPARE, buff, strlen(buff));
    
    /* ok, await for connect on 3rd port
     * This is the second throughput test, with data streaming from
     * the server back to the client.  Again stream data for 10 seconds.
     */
    log_println(1, "waiting for data on options->s2csockfd");

    j = 0;
    clilen = sizeof(cli_addr);
    for (;;) {
      if ((xmitsfd = accept(options->s2csockfd, (struct sockaddr *) &cli_addr, &clilen)) > 0)
        break;

      sprintf(tmpstr, "-------     S2C connection setup returned because (%d)", errno);
      if (get_debuglvl() > 1)
        perror(tmpstr);
      if (++j == 4)
        break;
    } 

    if (xmitsfd > 0) {
      if (getuid() == 0) {
        pipe(mon_pipe2);
        if ((mon_pid2 = fork()) == 0) {
          close(ctlsockfd);
          close(options->s2csockfd);
          close(xmitsfd);
          log_println(5, "S2C test Child thinks pipe() returned fd0=%d, fd1=%d", mon_pipe2[0], mon_pipe2[1]);
          log_println(2, "S2C test calling init_pkttrace() with pd=0x%x", (int) &cli_addr);
          init_pkttrace((struct sockaddr *) &cli_addr, clilen, mon_pipe2, device, &pair);
          exit(0);    /* Packet trace finished, terminate gracefully */
        }
        if (read(mon_pipe2[0], tmpstr, 128) <= 0) {
          log_println(0, "error & exit");
          exit(0);
        }
      }

      /* Check, and if needed, set the web100 autotuning function on.  This improves
       * performance without requiring the entire system be re-configured.  Returned
       * value is true if set is done, so admin knows if system default should be changed
       *
       * 11/22/04, the sBufMode and rBufMode per connection autotuning variables are being
       * depricated, so this is another attempt at making this work.  If window scaling is
       * enabled, then scale the buffer size to the correct value.  This will also help
       * when reporting the buffer / RTT limit.
       */

      if (autotune > 0) 
        web100_setbuff(xmitsfd, agent, autotune);

      /* experimental code, delete when finished */
      {
        web100_var *LimCwnd, *yar;
        u_int32_t limcwnd_val;
        char yuff[32];

        if (limit > 0) {
          log_println(0, "Setting Cwnd Limit");
          if ((conn = web100_connection_from_socket(agent, xmitsfd)) != NULL) {
            web100_agent_find_var_and_group(agent, "CurMSS", &group, &yar);
            web100_raw_read(yar, conn, yuff);
            web100_agent_find_var_and_group(agent, "LimCwnd", &group, &LimCwnd);
            limcwnd_val = limit * (atoi(web100_value_to_text(web100_get_var_type(yar), yuff)));
            web100_raw_write(LimCwnd, conn, &limcwnd_val);
          }
        }
      }
      /* End of test code */

      if (experimental > 0) {
        I2Addr sockAddr = I2AddrBySAddr(NULL, (struct sockaddr *) &cli_addr, clilen, 0, 0);
        char namebuf[200];
        size_t nameBufLen = 199;
        memset(namebuf, 0, 200);
        I2AddrNodeName(sockAddr, namebuf, &nameBufLen);
        sprintf(logname, "snaplog-%s.%d", namebuf, I2AddrPort(sockAddr));
        conn = web100_connection_from_socket(agent, xmitsfd);
        group = web100_group_find(agent, "read");
        snap = web100_snapshot_alloc(group, conn);
        if (experimental > 1)
          log = web100_log_open_write(logname, conn, group);
      }

      /* Kludge way of nuking Linux route cache.  This should be done
       * using the sysctl interface.
       */
      if (getuid() == 0) {
        /* system("/sbin/sysctl -w net.ipv4.route.flush=1"); */
        system("echo 1 > /proc/sys/net/ipv4/route/flush");
      }
      xconn = web100_connection_from_socket(agent, xmitsfd);
      rgroup = web100_group_find(agent, "read");
      rsnap = web100_snapshot_alloc(rgroup, xconn);
      tgroup = web100_group_find(agent, "tune");
      tsnap = web100_snapshot_alloc(tgroup, xconn);

      bytes = 0;
      k = 0;
      for (j=0; j<=BUFFSIZE; j++) {
        while (!isprint(k & 0x7f))
          k++;
        buff[j] = (k++ & 0x7f);
      }

      send_msg(ctlsockfd, TEST_START, "", 0);
      alarm(30);  /* reset alarm() again, this 10 sec test should finish before this signal
                   * is generated.  */
      t = secs();
      s = t + 10.0;

      while(secs() < s) { 
        c3++;
        if (experimental > 0) {
          web100_snap(snap);
          if (experimental > 1)
            web100_log_write(log, snap);
          web100_agent_find_var_and_group(agent, "SndNxt", &group, &var);
          web100_snap_read(var, snap, tmpstr);
          SndMax = atoi(web100_value_to_text(web100_get_var_type(var), tmpstr));
          web100_agent_find_var_and_group(agent, "SndUna", &group, &var);
          web100_snap_read(var, snap, tmpstr);
          SndUna = atoi(web100_value_to_text(web100_get_var_type(var), tmpstr));
        }
        if (experimental > 0) {
          if ((RECLTH<<2) < (SndMax - SndUna - 1)) {
            c1++;
            continue;
          }
        }

        n = write(xmitsfd, buff, RECLTH);
        if (n < 0)
          break;
        bytes += n;

        if (experimental > 0) {
          c2++;
        }
      }
      sndqueue = sndq_len(xmitsfd);

      shutdown(xmitsfd, SHUT_WR);  /* end of write's */
      s = secs() - t;
      x2cspd = (8.e-3 * bytes) / s;
      /* send the x2cspd to the client */
      sprintf(buff, "%0.0f %d %0.0f", x2cspd, sndqueue, bytes);
      send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
      if (experimental > 0) {
        web100_snapshot_free(snap);
        if (experimental > 1)
          web100_log_close_write(log);
      }

      web100_snap(rsnap);
      web100_snap(tsnap);

      log_println(1, "sent %d bytes to client in %0.2f seconds",(int) bytes, s);
      log_println(1, "Buffer control counters Total = %d, new data = %d, Draining Queue = %d", c3, c2, c1);
      /* Next send speed-chk a flag to retrieve the data it collected.
       * Skip this step if speed-chk isn't running.
       */

      if (getuid() == 0) {
        log_println(1, "Signal USR2(%d) sent to child [%d]", SIGUSR2, mon_pid2);
        kill(mon_pid2, SIGUSR2);
        FD_ZERO(&rfd);
        FD_SET(mon_pipe2[0], &rfd);
        sel_tv.tv_sec = 1;
        sel_tv.tv_usec = 100000;
read2:
        if ((ret = select(mon_pipe2[0]+1, &rfd, NULL, NULL, &sel_tv)) > 0) {
          if ((ret = read(mon_pipe2[0], spds[*spd_index], 128)) < 0)
            sprintf(spds[*spd_index], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0");
          log_println(1, "Read '%s' from monitor pipe", spds[*spd_index]);
          (*spd_index)++;
          if ((ret = read(mon_pipe2[0], spds[*spd_index], 128)) < 0)
            sprintf(spds[*spd_index], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0");
          log_println(1, "Read '%s' from monitor pipe", spds[*spd_index]);
          (*spd_index)++;
        } else {
          log_println(4, "Failed to read pkt-pair data from S2C flow, retcode=%d, reason=%d", ret, errno);
          if (errno == EINTR)
            goto read2;
          sprintf(spds[(*spd_index)++], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0");
          sprintf(spds[(*spd_index)++], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0");
        }
      } else {
        sprintf(spds[(*spd_index)++], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0");
        sprintf(spds[(*spd_index)++], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0");
      }

      log_println(1, "%6.0f Kbps inbound", x2cspd);
    }

    alarm(30);  /* reset alarm() again, this 10 sec test should finish before this signal
                 * is generated.  */
    ret = web100_get_data(tsnap, ctlsockfd, agent, count_vars);
    web100_snapshot_free(tsnap);
    ret = web100_get_data(rsnap, ctlsockfd, agent, count_vars);
    web100_snapshot_free(rsnap);

    msgLen = sizeof(buff);
    if (recv_msg(ctlsockfd, &msgType, &buff, &msgLen)) {
      log_println(0, "Protocol error!");
      exit(1);
    }
    if (check_msg_type("S2C throughput test", TEST_MSG, msgType)) {
      exit(2);
    }
    if (msgLen <= 0) {
      log_println(0, "Improper message");
      exit(3);
    }
    buff[msgLen] = 0;
    *s2cspd = atoi(buff);

    close(xmitsfd);
    send_msg(ctlsockfd, TEST_FINALIZE, "", 0);

    if (getuid() == 0) {
      write(mon_pipe2[1], "", 1);
      close(mon_pipe2[0]);
      close(mon_pipe2[1]);
    }
    /* we should wait for the SIGCHLD signal here */
    wait(NULL);

    log_println(1, " <------------------------->");
  }
  return 0;
}
