/**
 * This file contains the functions needed to read the web100
 * variables.  It links into the main web100srv program.
 *
 * Richard Carlson 3/10/04
 * rcarlson@interent2.edu
 */

#include <ctype.h>
#include <time.h>
#include <assert.h>

#include "logging.h"
#include "network.h"
#include "protocol.h"
#include "strlutils.h"
#include "utils.h"
#include "web100srv.h"

struct tcp_name {
  char* web100_name;
  char* web10g_name;
};

/* Must match in-order with tcp_vars in web100srv.h struct */
// TODO: more robust order matching with included file and preprocessor macros
static struct tcp_name tcp_names[] = {
/* {"WEB100", "WEB10G" } / tcp_vars name / */
  {"Timeouts", "Timeouts"}, /* Timeouts */
  {"SumRTT", "SumRTT"}, /* SumRTT */
  {"CountRTT", "CountRTT"}, /* CountRTT */
  {"PktsRetrans", "SegsRetrans"}, /* PktsRetrans */
  {"FastRetran", "FastRetran"}, /* FastRetran */
  {"DataPktsOut", "DataSegsOut"}, /* DataPktsOut */
  {"AckPktsOut", NULL}, /* AckPktsOut - not included in web10g */
  {"CurMSS", "CurMSS"}, /* CurrentMSS */
  {"DupAcksIn", "DupAcksIn"}, /* DupAcksIn */
  /* NOTE: in the server to client throughput test all packets received from client are ack's 
   * So SegsIn == AckPktsIn. I don't see a replacement in web10g maybe (SegsIn - DataSegsIn)
   */
  {"AckPktsIn", "SegsIn"}, /* AckPktsIn - not included in web10g */
  {"MaxRwinRcvd", "MaxRwinRcvd"}, /* MaxRwinRcvd */
  {"X_Sndbuf", NULL}, /* Sndbuf - Not in Web10g pull from socket */
  {"CurCwnd", "CurCwnd"}, /* CurrentCwnd */
  {"SndLimTimeRwin", "SndLimTimeRwin"}, /* SndLimTimeRwin */
  {"SndLimTimeCwnd", "SndLimTimeCwnd"}, /* SndLimTimeCwnd */
  {"SndLimTimeSender", "SndLimTimeSnd"}, /* SndLimTimeSender */
  {"DataBytesOut", "DataOctetsOut"}, /* DataBytesOut */
  {"SndLimTransRwin", "SndLimTransRwin"}, /* SndLimTransRwin */
  {"SndLimTransCwnd", "SndLimTransCwnd"}, /* SndLimTransCwnd */
  {"SndLimTransSender", "SndLimTransSnd"}, /* SndLimTransSender */
  {"MaxSsthresh", "MaxSsthresh"}, /* MaxSsthresh */
  {"CurRTO", "CurRTO"}, /* CurrentRTO */
  {"CurRwinRcvd", "CurRwinRcvd"}, /* CurrentRwinRcvd */
  {"MaxCwnd", NULL}, /* MaxCwnd split into MaxSsCwnd and MaxCaCwnd web10g */
  {"CongestionSignals", "CongSignals"}, /* CongestionSignals */
  {"PktsOut", "SegsOut"}, /* PktsOut */
  {"MinRTT", "MinRTT"}, /* MinRTT */
  {"RcvWinScale", "WinScaleRcvd"}, /* RcvWinScale */
  {"SndWinScale", "WinScaleSent"}, /* SndWinScale */
  {"CongAvoid", "CongAvoid"}, /* CongAvoid */
  {"CongestionOverCount", "CongOverCount"}, /* CongestionOverCount */
  {"MaxRTT", "MaxRTT"}, /* MaxRTT */
  {"OtherReductions", "OtherReductions"}, /* OtherReductions */
  {"CurTimeoutCount", "CurTimeoutCount"}, /* CurTimeoutCount */
  {"AbruptTimeouts", "AbruptTimeouts"}, /* AbruptTimeouts */
  {"SendStall", "SendStall"}, /* SendStall */
  {"SlowStart", "SlowStart"}, /* SlowStart */
  {"SubsequentTimeouts", "SubsequentTimeouts"}, /* SubsequentTimeouts */
  {"ThruBytesAcked", "ThruOctetsAcked"}, /* ThruBytesAcked */
  { NULL, "MaxSsCwnd" }, /* MaxSsCwnd */
  { NULL, "MaxCaCwnd" } /* MaxCaCwnd */
};

/**
 * set up the necessary structures for monitoring connections at the
 * beginning
 * @param *VarFileName pointer to file name of Web100 variables
 * @return integer indicating number of web100 variables read
 * 		or indicating failure of initialization
 */
int tcp_stat_init(char *VarFileName) {
#if USE_WEB100
  FILE * fp;
  char line[256], trimmedline[256];
  int count_vars = 0;

  assert(VarFileName);

  // unable to open web100 variable file
  if ((fp = fopen(VarFileName, "r")) == NULL) {
    err_sys("Required Web100 variables file missing, use -f option to specify "
            "file location\n");
    exit(5);
  }

  while (fgets(line, 255, fp) != NULL) {  // read file linewise
    if ((line[0] == '#') || (line[0] == '\n'))  // if newline or comment, ignore
      continue;
    // copy web100 variable into our array and increment count of read variables

    // remove unwanted chars (right now, trailing/preceding chars from wb100
    // var names)
    trim(line, strlen(line), trimmedline, sizeof(trimmedline));
    strlcpy(web_vars[count_vars].name, trimmedline,
            sizeof(web_vars[count_vars].name));
    count_vars++;
  }
  fclose(fp);
  log_println(1, "web100_init() read %d variables from file", count_vars);

  return (count_vars);
#elif USE_TCPE
  return TOTAL_INDEX_MAX;
#endif
}

/**
 * Get a string representation of an ip address.
 *
 * @param addr A sockaddr structure which contains the address
 * @param buf A buffer to fill with the ip address as a string
 * @param len The length of buf.
 */
static void addr2a(struct sockaddr_storage* addr, char * buf, int len) {
  if (((struct sockaddr*)addr)->sa_family == AF_INET) {
    inet_ntop(AF_INET, &(((struct sockaddr_in*)addr)->sin_addr), buf, len);
  }
#ifdef AF_INET6
  else if (((struct sockaddr*)addr)->sa_family == AF_INET6) {
    inet_ntop(AF_INET6, &(((struct sockaddr_in6*)addr)->sin6_addr), buf, len);
  }
#endif
}

/**
 * Get a string representation of an port number.
 *
 * @param addr A sockaddr structure which contains the port number
 * @param buf A buffer to fill with the port number as a string
 * @param len The length of buf.
 */
static void port2a(struct sockaddr_storage* addr, char* buf, int len) {
  if (((struct sockaddr*)addr)->sa_family == AF_INET) {
    snprintf(buf, len, "%hu", ntohs(((struct sockaddr_in*)addr)->sin_port));
  }
#ifdef AF_INET6
  else if (((struct sockaddr*)addr)->sa_family == AF_INET6) {
    snprintf(buf, len, "%hu", ntohs(((struct sockaddr_in6*)addr)->sin6_port));
  }
#endif
}

/**
 * Performs part of the middlebox test.
 * The server sets the maximum value of the
 * congestion window, and starts a 5 second long
 *  throughput test using the newly created connection.
 *
 *  The NDT server then sends packets contiguously.
 *    These are written using a buffer of the size of the current MSS or
 *    8192 bytes.
 *
 * @param sock integer socket file descriptor
 * @param agent pointer to a web100_agent
 * @param cn pointer to the web100_connection
 * @param results pointer to string containing Server address , client address
 * 			 currentMSS, WinScaleSent and WinScaleRecv values
 *
 *
 */
void tcp_stat_middlebox(int sock, tcp_stat_agent* agent, tcp_stat_connection cn,
                        char *results, size_t results_strlen) {
#if USE_WEB100
  web100_var* var;
  web100_group* group;
  web100_snapshot* snap;
  web100_var* LimCwnd;
#elif USE_TCPE
  struct tcpe_val value;
  tcpe_data* data = NULL;
#endif

  char buff[8192], line[256];
  char* sndbuff;
  int i, j, k, currentMSSval = 0;
  int SndMax = 0, SndUna = 0;
  fd_set wfd;
  struct timeval sel_tv;
  int ret;
  char tmpstr[200];
  size_t tmpstrlen = sizeof(tmpstr);
  u_int32_t limcwnd_val;
  struct sockaddr_storage saddr;
  socklen_t saddr_size;

  // middlebox test results
  static char vars[][255] = { "CurMSS", "WinScaleSent", "WinScaleRecv", };

  assert(results);

  log_println(4, "Looking for Web100 data on socketid %d", sock);

  // get Server address and add to "results"

  // get socket IP address
  saddr_size = sizeof(saddr);
  if (getsockname(sock, (struct sockaddr *) &saddr, &saddr_size) == -1) {
    /* Make it clear something failed but continue test */
    log_println(0, "Middlebox - getsockname() failed: %s", strerror(errno));
    snprintf(line, sizeof(line), "address_error;");
    snprintf(tmpstr, sizeof(tmpstr), "0");
  } else {
    // copy address into tmpstr String
    memset(tmpstr, 0, 200);
    addr2a(&saddr, tmpstr , tmpstrlen);
    // now copy tmpstr containing address into "line"
    snprintf(line, sizeof(line), "%s;", tmpstr);
    memset(tmpstr, 0, 200);
    // service name into tmpstr
    tmpstrlen = sizeof(tmpstr);
    port2a(&saddr, tmpstr, tmpstrlen);
  }
  log_print(3, "Server: %s%s ", line, tmpstr);
  // copy servers address into the meta test struct
  memcpy(meta.server_ip, line, strlen(line));
  // terminate the IP address string
  meta.server_ip[(strlen(line) - 1)] = 0;
  // Add this address to results
  strlcat(results, line, results_strlen);

  // Now perform the above set of functions for client address/service name
  // and copy into results
  tmpstrlen = sizeof(tmpstr);
  saddr_size = sizeof(saddr);
  if (getpeername(sock, (struct sockaddr *) &saddr, &saddr_size) == -1) {
    /* Make it clear something failed but continue test */
    log_println(0, "Middlebox - getpeername() failed: %s", strerror(errno));
    snprintf(line, sizeof(line), "address_error;");
    snprintf(tmpstr, sizeof(tmpstr), "0");
  } else {
    // copy address into tmpstr String
    memset(tmpstr, 0, 200);
    addr2a(&saddr, tmpstr , tmpstrlen);
    snprintf(line, sizeof(line), "%s;", tmpstr);
    tmpstrlen = sizeof(tmpstr);
    port2a(&saddr, tmpstr, tmpstrlen);
  }
  log_print(3, "Client: %s%s ", line, tmpstr);
  strlcat(results, line, results_strlen);

  // get web100 values for the middlebox test result group
  for (i = 0; i < sizeof(vars) / sizeof(vars[0]); i++) {
#if USE_WEB100
    // read web100_group and web100_var of vars[i] into group and var
    web100_agent_find_var_and_group(agent, vars[i], &group, &var);
    // read variable value from web100 connection
    web100_raw_read(var, cn, buff);

    // get current MSS in textual format and append to results

    // get current MSS value and append to "results"
    if (strcmp(vars[i], "CurMSS") == 0)
      currentMSSval = atoi(
          web100_value_to_text(web100_get_var_type(var), buff));
    snprintf(line, sizeof(line), "%s;",
             web100_value_to_text(web100_get_var_type(var), buff));
#elif USE_TCPE
    web10g_get_val(agent, cn, vars[i], &value);
    if (strcmp(vars[i], "CurMSS") == 0)
      currentMSSval = value.uv32;
    snprintf(line, sizeof(line), "%u;", value.uv32);
#endif

    if (strcmp(line, "4294967295;") == 0)
      snprintf(line, sizeof(line), "%d;", -1);

    // strlcat(results, line, sizeof(results));
    strlcat(results, line, results_strlen);
    log_print(3, "%s", line);
  }
  log_println(3, "");
  log_println(0, "Sending %d Byte packets over the network, and data=%s",
              currentMSSval, line);

  /* The initial check has been completed, now stream data to the remote client
   * for 5 seconds with very limited buffer space.  The idea is to see if there
   * is a difference between this test and the later s2c speed test.  If so, then
   * it may be due to a duplex mismatch condition.
   * RAC 2/28/06
   */

  limcwnd_val = 2 * currentMSSval;

#if USE_WEB100
  // get web100_var and web100_group
  web100_agent_find_var_and_group(agent, "LimCwnd", &group, &LimCwnd);

  // set TCP CWND web100 variable to twice the current MSS Value
  web100_raw_write(LimCwnd, cn, &limcwnd_val);
#elif USE_TCPE
  tcpe_write_var("LimCwnd", (uint32_t)limcwnd_val, cn, agent);
#endif

  log_println(5, "Setting Cwnd Limit to %d octets", limcwnd_val);

  // try to allocate memory of the size of current MSS Value
  sndbuff = malloc(currentMSSval);
  if (sndbuff == NULL) {  // not possible, use MIDDLEBOX_PREDEFINED_MSS bytes
    log_println(0,
                "Failed to allocate memory --> switching to 8192 Byte packets");
    sndbuff = buff;
    currentMSSval = MIDDLEBOX_PREDEFINED_MSS;
  }
  if (getuid() == 0) {
    system("echo 1 > /proc/sys/net/ipv4/route/flush");
  }

  // fill send buffer with random printable data

  k = 0;
  for (j = 0; j < currentMSSval; j++) {
    while (!isprint(k & 0x7f))  // Is character printable?
      k++;
    sndbuff[j] = (k++ & 0x7f);
  }

#if USE_WEB100
  // get web100 group with name "read"
  group = web100_group_find(agent, "read");
  snap = web100_snapshot_alloc(group, cn);
#elif USE_TCPE
  tcpe_data_new(&data);
#endif

  FD_ZERO(&wfd);
  FD_SET(sock, &wfd);
  sel_tv.tv_sec = 5;  // 5 seconds maximum for select call below to complete
  sel_tv.tv_usec = 0;  // 5s + 0ms
  while ((ret = select(sock + 1, NULL, &wfd, NULL, &sel_tv)) > 0) {
    if ((ret == -1) && (errno == EINTR)) /* a signal arrived, ignore it */
      continue;

#if USE_WEB100
    web100_snap(snap);

    // get next sequence # to be sent
    web100_agent_find_var_and_group(agent, "SndNxt", &group, &var);
    web100_snap_read(var, snap, line);
    SndMax = atoi(web100_value_to_text(web100_get_var_type(var), line));
    // get oldest un-acked sequence number
    web100_agent_find_var_and_group(agent, "SndUna", &group, &var);
    web100_snap_read(var, snap, line);
    SndUna = atoi(web100_value_to_text(web100_get_var_type(var), line));
#elif USE_TCPE
    tcpe_read_vars(data, cn, agent);
    web10g_find_val(data, "SndNxt", &value);
    SndMax = value.uv32;
    web10g_find_val(data, "SndUna", &value);
    SndUna = value.uv32;
#endif

    // stop sending data if (buf size * 16) <
    // [ (Next Sequence # To Be Sent) - (Oldest Unacknowledged Sequence #) - 1 ]
    if ((currentMSSval << 4) < (SndMax - SndUna - 1)) {
      continue;
    }

    // write currentMSSval number of bytes from send-buffer data into socket
    k = write(sock, sndbuff, currentMSSval);
    if ((k == -1) && (errno == EINTR))  // error writing into socket due
                                        // to interruption, try again
      continue;
    if (k < 0)  // general error writing to socket. quit
      break;
  }

#if USE_WEB100
  log_println(5, "Finished with web100_middlebox() routine snap-0x%x, "
              "sndbuff=%x0x", snap, sndbuff);
  web100_snapshot_free(snap);
#elif USE_TCPE
  tcpe_data_free(&data);
  log_println(5, "Finished with web10g_middlebox() routine, "
              "sndbuff=%x0x", sndbuff);
#endif
  /* free(sndbuff); */
}

/**
 * Get receiver side Web100 stats and write them to the log file
 *
 * @param sock integer socket file descriptor
 * @param agent pointer to a tcp_stat_agent
 * @param cn pointer to a tcp_stat_connection
 * @param count_vars integer number of tcp_stat_vars to get value of
 *
 */
void tcp_stat_get_data_recv(int sock, tcp_stat_agent* agent,
                            tcp_stat_connection cn, int count_vars) {
#if USE_WEB100
  web100_var* var = NULL;
  web100_group* group = NULL;
#elif USE_TCPE
  tcpe_data* data = NULL;
#endif
  int i;
  char buf[32], line[256], *ctime();
  FILE * fp;
  time_t tt;

  // get current time
  tt = time(0);
  // open logfile in append mode and write time into it
  fp = fopen(get_logfile(), "a");
  if (fp)
    fprintf(fp, "%15.15s;", ctime(&tt) + 4);
  // get values for group, var of IP Address of the Remote host's side of
  // connection

#if USE_WEB100
  web100_agent_find_var_and_group(agent, "RemAddress", &group, &var);
  web100_raw_read(var, cn, buf);
  snprintf(line, sizeof(line), "%s;",
           web100_value_to_text(web100_get_var_type(var), buf));
#elif USE_TCPE
  web10g_get_remote_addr(agent, cn, buf, sizeof(buf));
  snprintf(line, sizeof(line), "%s;", buf);
#endif
  // write remote address to log file
  if (fp)
    fprintf(fp, "%s", line);

  // get values for other web100 variables and write to the log file

#if USE_WEB100
  int ok = 1;
  for (i = 0; i < count_vars; i++) {
    if ((web100_agent_find_var_and_group(agent, web_vars[i].name, &group,
                                         &var)) != WEB100_ERR_SUCCESS) {
      log_println(1, "Variable %d (%s) not found in KIS", i,
                  web_vars[i].name);
      ok = 0;
      continue;
    }

    if (cn == NULL) {
      fprintf(
          stderr,
          "Web100_get_data_recv() failed, return to testing routine\n");
      return;
    }

    if ((web100_raw_read(var, cn, buf)) != WEB100_ERR_SUCCESS) {
      if (get_debuglvl() > 9)
        web100_perror("web100_raw_read()");
      continue;
    }
    if (ok == 1) {
      snprintf(web_vars[i].value, sizeof(web_vars[i].value), "%s",
               web100_value_to_text(web100_get_var_type(var), buf));
      if (fp)
        fprintf(fp, "%d;", (int32_t) atoi(web_vars[i].value));
      log_println(9, "%s: %d", web_vars[i].name, atoi(web_vars[i].value));
    }
    ok = 1;
  }
#elif USE_TCPE
  tcpe_data_new(&data);
  tcpe_read_vars(data, cn, agent);

  // Loop through all the web10g variables and write to file/log_print them
  for (i = 0; i < ARRAYSIZE(data->val); i++) {
    if (data->val[i].mask) continue;

    switch (tcpe_var_array[i].type) {
      case TCPE_UNSIGNED64:
        if (fp)
          fprintf(fp, "%" PRIu64 ";", data->val[i].uv64);
        log_println(9, "%s: %" PRIu64,
                    tcpe_var_array[i].name, data->val[i].uv64);
        break;
      case TCPE_UNSIGNED32:
        if (fp)
          fprintf(fp, "%u;", data->val[i].uv32);
        log_println(9, "%s: %u", tcpe_var_array[i].name, data->val[i].uv32);
        break;
      case TCPE_SIGNED32:
        if (fp)
          fprintf(fp, "%d;", data->val[i].sv32);
        log_println(9, "%s: %d", tcpe_var_array[i].name, data->val[i].sv32);
        break;
      case TCPE_UNSIGNED16:
        if (fp)
          fprintf(fp, "%" PRIu16 ";", data->val[i].uv16);
        log_println(9, "%s: %" PRIu16,
                    tcpe_var_array[i].name, data->val[i].uv16);
        break;
      case TCPE_UNSIGNED8:
        if (fp)
          fprintf(fp, "%" PRIu8 ";", data->val[i].uv8);
        log_println(9, "%s: %" PRIu8, tcpe_var_array[i].name, data->val[i].uv8);
        break;
      default:
        break;
    }
  }
  tcpe_data_free(&data);
#endif

  // close file pointers after web100 variables have been fetched
  if (fp) {
    fprintf(fp, "\n");
    fclose(fp);
  }
}

#if USE_TCPE
// Persistent storage needed.
static tcpe_data* dataDumpSave;
static int X_SndBuf;
static int X_RcvBuf;
#endif

/**
 * Collect Web100 stats from a snapshot and transmit to a receiver.
 * The transmission is done using a TES_MSG type message and sent to
 * client reachable via the input parameter socket FD.
 *
 * @param snap pointer to a tcp_stat_snapshot taken earlier
 * @param ctlsock integer socket file descriptor indicating data recipient
 * @param agent pointer to a tcp_stat_agent
 * @param count_vars integer number of tcp_stat_variables to get value of
 *
 */
int tcp_stat_get_data(tcp_stat_snap* snap, int testsock, int ctlsock,
                      tcp_stat_agent* agent, int count_vars) {
  char line[256];
#if USE_WEB100
  int i;
  web100_var* var;
  web100_group* group;
  char buf[32];

  assert(snap);
  assert(agent);

  for (i = 0; i < count_vars; i++) {
    if ((web100_agent_find_var_and_group(agent, web_vars[i].name, &group,
                                         &var)) != WEB100_ERR_SUCCESS) {
      log_println(9, "Variable %d (%s) not found in KIS: ", i,
                  web_vars[i].name);
      continue;
    }

    // if no snapshot provided, no way to get values
    if (snap == NULL) {
      fprintf(stderr,
              "Web100_get_data() failed, return to testing routine\n");
      log_println(6,
                  "Web100_get_data() failed, return to testing routine\n");
      return (-1);
    }

    // handle an unsuccessful data retrieval
    if ((web100_snap_read(var, snap, buf)) != WEB100_ERR_SUCCESS) {
      if (get_debuglvl() > 9) {
        log_print(9, "Variable %d (%s): ", i, web_vars[i].name);
        web100_perror("web100_snap_read()");
      }
      continue;
    }

    // assign values and transmit message with all web100 variables to socket
    // receiver end
    snprintf(web_vars[i].value, sizeof(web_vars[i].value), "%s",
             web100_value_to_text(web100_get_var_type(var), buf));
    snprintf(line, sizeof(line), "%s: %d\n", web_vars[i].name,
             atoi(web_vars[i].value));
    send_msg(ctlsock, TEST_MSG, line, strlen(line));
    log_print(9, "%s", line);
  }
  log_println(6, "S2C test - Send web100 data to client pid=%d", getpid());
  return (0);
#elif USE_TCPE
  int j;
  unsigned int m;
  struct tcpe_val val;

  m = sizeof(X_RcvBuf);
  getsockopt(testsock, SOL_SOCKET, SO_RCVBUF, (void *)&X_RcvBuf, &m);
  m = sizeof(X_SndBuf);
  getsockopt(testsock, SOL_SOCKET, SO_SNDBUF, (void *)&X_SndBuf, &m);

  assert(snap);

  tcpe_data_new(&dataDumpSave);

  for (j = 0; j < ARRAYSIZE(snap->val); j++) {
    dataDumpSave->val[j].mask = snap->val[j].mask;
    dataDumpSave->val[j].uv64 = snap->val[j].uv64;
    if (snap->val[j].mask) continue;

    switch (tcpe_var_array[j].type) {
      case TCPE_UNSIGNED64:
        snprintf(line, sizeof(line), "%s: %" PRIu64 "\n",
                 tcpe_var_array[j].name, snap->val[j].uv64);
        break;
      case TCPE_UNSIGNED32:
        snprintf(line, sizeof(line), "%s: %u\n",
                 tcpe_var_array[j].name, snap->val[j].uv32);
        break;
      case TCPE_SIGNED32:
        snprintf(line, sizeof(line), "%s: %d\n",
                 tcpe_var_array[j].name, snap->val[j].sv32);
        break;
      case TCPE_UNSIGNED16:
        snprintf(line, sizeof(line), "%s: %" PRIu16 "\n",
                 tcpe_var_array[j].name, snap->val[j].uv16);
      case TCPE_UNSIGNED8:
        snprintf(line, sizeof(line), "%s: %" PRIu8 "\n",
                 tcpe_var_array[j].name, snap->val[j].uv8);
        break;
      default:
        break;
    }
    send_msg(ctlsock, TEST_MSG, line, strlen(line));
    log_print(9, "%s", line);
  }

  /* This is the list of changed variable names that the client tries to read.
   * Web100 -> Web10g
   * ECNEnabled -> ECN
   * NagleEnabled -> Nagle
   * SACKEnabled -> WillSendSACK & WillUseSACK
   * TimestampsEnabled -> TimeStamps
   * PktsRetrans -> SegsRetrans
   * X_Rcvbuf -> Not in web10g doesn't acutally use it so just leave it out
   * DataPktsOut -> DataSegsOut
   * AckPktsOut -> Depreciated
   * MaxCwnd -> MaxSsCwnd MaxCaCwnd
   * SndLimTimeSender -> SndLimTimeSnd
   * DataBytesOut -> DataOctetsOut
   * AckPktsIn -> Depreciated
   * SndLimTransSender -> SndLimTransSnd
   * PktsOut -> SegsOut
   * CongestionSignals -> CongSignals
   * RcvWinScale -> Same as WinScaleSent if WinScaleSent != -1
   */
  static const char* msg = "-~~~Web100_old_var_names~~~-: 1\n";
  send_msg(ctlsock, TEST_MSG, msg, strlen(msg));
  uint32_t temp;

  /* ECNEnabled -> ECN */
  val.uv64 = 0;
  web10g_find_val(snap, "ECN", &val);
  snprintf(line, sizeof(line), "ECNEnabled: %d\n", val.sv32);
  send_msg(ctlsock, TEST_MSG, line, strlen(line));

  /* NagleEnabled -> Nagle */
  val.uv64 = 0;
  web10g_find_val(snap, "Nagle", &val);
  snprintf(line, sizeof(line), "NagleEnabled: %d\n", val.sv32);
  send_msg(ctlsock, TEST_MSG, line, strlen(line));

  /* SACKEnabled -> WillUseSACK & WillSendSACK
   * keep this in line with web100 for now i.e. 0 == off 1 == on */
  val.uv64 = 0;
  web10g_find_val(snap, "WillUseSACK", &val);
  snprintf(line, sizeof(line), "SACKEnabled: %d\n", (val.sv32 == 1) ? 1 : 0);
  send_msg(ctlsock, TEST_MSG, line, strlen(line));

  /* TimestampsEnabled -> TimeStamps */
  val.uv64 = 0;
  web10g_find_val(snap, "TimeStamps", &val);
  snprintf(line, sizeof(line), "TimestampsEnabled: %d\n", val.sv32);
  send_msg(ctlsock, TEST_MSG, line, strlen(line));

  /* PktsRetrans -> SegsRetrans */
  val.uv64 = 0;
  web10g_find_val(snap, "SegsRetrans", &val);
  snprintf(line, sizeof(line), "PktsRetrans: %u\n", val.uv32);
  send_msg(ctlsock, TEST_MSG, line, strlen(line));

  /* DataPktsOut -> DataSegsOut */
  val.uv64 = 0;
  web10g_find_val(snap, "DataSegsOut", &val);
  snprintf(line, sizeof(line), "DataPktsOut: %u\n", val.uv32);
  send_msg(ctlsock, TEST_MSG, line, strlen(line));

  /* MaxCwnd -> MaxSsCwnd MaxCaCwnd */
  val.uv64 = 0;
  web10g_find_val(snap, "MaxSsCwnd", &val);
  temp = val.uv32;
  val.uv64 = 0;
  web10g_find_val(snap, "MaxCaCwnd", &val);
  temp = MAX(temp, val.uv32);
  snprintf(line, sizeof(line), "DataPktsOut: %u\n", temp);
  send_msg(ctlsock, TEST_MSG, line, strlen(line));

  /* SndLimTimeSender -> SndLimTimeSnd */
  val.uv64 = 0;
  web10g_find_val(snap, "SndLimTimeSnd", &val);
  snprintf(line, sizeof(line), "SndLimTimeSender: %u\n", val.uv32);
  send_msg(ctlsock, TEST_MSG, line, strlen(line));

  /* DataBytesOut -> DataOctetsOut */
  val.uv64 = 0;
  web10g_find_val(snap, "DataOctetsOut", &val);
  snprintf(line, sizeof(line), "DataBytesOut: %u\n", val.uv32);
  send_msg(ctlsock, TEST_MSG, line, strlen(line));

  /* SndLimTransSender -> SndLimTransSnd */
  val.uv64 = 0;
  web10g_find_val(snap, "SndLimTransSnd", &val);
  snprintf(line, sizeof(line), "SndLimTransSender: %u\n", val.uv32);
  send_msg(ctlsock, TEST_MSG, line, strlen(line));

  /* PktsOut -> SegsOut */
  val.uv64 = 0;
  web10g_find_val(snap, "SegsOut", &val);
  snprintf(line, sizeof(line), "PktsOut: %u\n", val.uv32);
  send_msg(ctlsock, TEST_MSG, line, strlen(line));

  /* CongestionSignals -> CongSignals */
  val.uv64 = 0;
  web10g_find_val(snap, "CongSignals", &val);
  snprintf(line, sizeof(line), "CongestionSignals: %u\n", val.uv32);
  send_msg(ctlsock, TEST_MSG, line, strlen(line));

  /* RcvWinScale -> Same as WinScaleSent if WinScaleSent != -1 */
  val.uv64 = 0;
  web10g_find_val(snap, "WinScaleSent", &val);
  if (val.sv32 == -1)
    snprintf(line, sizeof(line), "RcvWinScale: %u\n", 0);
  else
    snprintf(line, sizeof(line), "RcvWinScale: %d\n", val.sv32);
  send_msg(ctlsock, TEST_MSG, line, strlen(line));

  send_msg(ctlsock, TEST_MSG, msg, strlen(msg));

  log_println(6, "S2C test - Send web100 data to client pid=%d", getpid());
  return 0;
#endif
}

#if USE_WEB100
/**
 * Calculate Web100 based Round-Trip Time (RTT) value.
 *
 * "SumRTT" = sum of all sampled round trip times.
 * "CountRTT" = count of samples in SumRTT.
 * Thus, sumRTT/CountRTT = RTT
 * @param ctlsock integer socket file descriptor indicating data recipient
 * @param agent pointer to a web100_agent
 * @param cn pointer to web100_connection
 * @return positive integral round trip time in milliseconds on success,
 * 		   negative integer error code if failure.
 * 		Error codes :
 * 				 -10 if web100-connection is null.
 * 				 -24 Cannot find CountRTT or SumRTT web100_variable's var/group.
 * 				 -25 cannot read the value of the countRTT or SumRTT web100_variable.
 *
 *
 */
int web100_rtt(int sock, web100_agent* agent, web100_connection* cn) {
  web100_var* var;
  char buf[32];
  web100_group* group;
  double count, sum;

  if (cn == NULL)
    return (-10);

  if ((web100_agent_find_var_and_group(agent, "CountRTT", &group, &var))
      != WEB100_ERR_SUCCESS)
    return (-24);
  if ((web100_raw_read(var, cn, buf)) != WEB100_ERR_SUCCESS) {
    return (-25);
  }
  count = atoi(web100_value_to_text(web100_get_var_type(var), buf));

  if ((web100_agent_find_var_and_group(agent, "SumRTT", &group, &var))
      != WEB100_ERR_SUCCESS)
    return (-24);
  if ((web100_raw_read(var, cn, buf)) != WEB100_ERR_SUCCESS) {
    return (-25);
  }
  sum = atoi(web100_value_to_text(web100_get_var_type(var), buf));
  return (sum / count);
}
#endif

/**
 * Check if the "Auto Tune Send Buffer" and "Auto Tune Receive Buffer" options
 * are enabled and return status on each
 *
 * @param sock integer socket file descriptor indicating data recipient
 * @param agent pointer to a web100_agent
 * @param cn pointer to web100_connection
 * @return On successful fetch of required web100_varibles, integers:
 *  				0x01 if "Autotune send buffer" is not enabled
 *  				0x02 if "Autotune receive buffer" is not enabled
 *  				0x03 if both " Autotune send buffer" and "Autotune receive buffer" are not enabled.
 *  		If failure, error codes :
 *  				10 if web100-connection is null.
 * 					22 Cannot find X_SBufMode or X_RBufMode web100_variable's var/group.
 * 					23 cannot read the value of the X_SBufMode or X_RBufMode web100_variable.
 */

int tcp_stat_autotune(int sock, tcp_stat_agent* agent, tcp_stat_connection cn) {
#if USE_WEB100
  web100_var* var;
  char buf[32];
  web100_group* group;
  int i, j = 0;

  if (cn == NULL)
    return (10);

  if ((web100_agent_find_var_and_group(agent, "X_SBufMode", &group, &var))
      != WEB100_ERR_SUCCESS)
    return (22);
  if ((web100_raw_read(var, cn, buf)) != WEB100_ERR_SUCCESS) {
    log_println(4, "Web100_raw_read(X_SBufMode) failed with errorno=%d",
                errno);
    return (23);
  }
  i = atoi(web100_value_to_text(web100_get_var_type(var), buf));

  /* OK, the variable i now holds the value of the sbufmode autotune parm.  If it
   * is 0, autotuning is turned off, so we turn it on for this socket.
   */
  if (i == 0)
    j |= 0x01;

  /* OK, the variable i now holds the value of the rbufmode autotune parm.  If it
   * is 0, autotuning is turned off, so we turn it on for this socket.
   */
  if ((web100_agent_find_var_and_group(agent, "X_RBufMode", &group, &var))
      != WEB100_ERR_SUCCESS)
    return (22);
  if ((web100_raw_read(var, cn, buf)) != WEB100_ERR_SUCCESS) {
    log_println(4, "Web100_raw_read(X_RBufMode) failed with errorno=%d",
                errno);
    return (23);
  }
  i = atoi(web100_value_to_text(web100_get_var_type(var), buf));

  if (i == 0)
    j |= 0x02;
  return (j);
#elif USE_TCPE
  // Disabled in web10g.
  return 0x03;
#endif
}

#if USE_WEB100
/**
 * Check if the "Auto Tune Send Buffer" and "Auto Tune Receive Buffer" options
 * are enabled. If not, scale the Send window or receive window sizes based on the
 * scaling factors negotiated. Scaling factor sndWinScale is used to set used to set "LimCwnd"
 * (maximum size, in bytes, of the congestion window that may be used)
 * and RcvWindowScale is used to set "LimRwin"( maximum receive window size, in bytes, that may be advertised).
 *
 * This function seems unused currently.
 *
 * @param sock integer socket file descriptor indicating data recipient
 * @param agent pointer to a web100_agent
 * @param cn pointer to web100_connection
 * @return Integer, 0 on success
 *  	  If failure, error codes :
 *  				10 - web100-connection is null.
 *  				22 - Cannot find LimCwnd web100 variable's var/group data.
 *  				23 - Cannot find LimRwin web100 variable's var/group data
 * 					24 - cannot find SndWinScale web100 variable's var/group data.
 * 					25 - cannot read the value of the SndWinScale web100 variable.
 * 					34 - cannot find RcvWinScale web100 variable's var/group data.
 * 					35 - cannot read value of RcvWinScale web100 variable.
 *
 */
int tcp_stat_setbuff(int sock, tcp_stat_agent* agent, tcp_stat_connection cn,
                     int autotune) {
  web100_var* var;
  char buf[32];
  web100_group* group;
  int buff;
  int sScale, rScale;

  if (cn == NULL)
    return (10);

  // get Window scale used by the sender to decode snd.wnd
  if ((web100_agent_find_var_and_group(agent, "SndWinScale", &group, &var))
      != WEB100_ERR_SUCCESS)
    return (24);
  if ((web100_raw_read(var, cn, buf)) != WEB100_ERR_SUCCESS) {
    return (25);
  }
  sScale = atoi(web100_value_to_text(web100_get_var_type(var), buf));
  if (sScale > 15)  // define constant for 15
    sScale = 0;

  // get Window scale used by the sender to decode seg.wnd
  if ((web100_agent_find_var_and_group(agent, "RcvWinScale", &group, &var))
      != WEB100_ERR_SUCCESS)
    return (34);
  if ((web100_raw_read(var, cn, buf)) != WEB100_ERR_SUCCESS) {
    return (35);
  }
  rScale = atoi(web100_value_to_text(web100_get_var_type(var), buf));
  if (rScale > 15)
    rScale = 0;

  if ((sScale > 0) && (rScale > 0)) {
    // 64k( max TCP rcv window)
    buff = (MAX_TCP_WIN_BYTES * KILO_BITS) << sScale;
    if (autotune & 0x01) {  // autotune send buffer is not enabled
      if ((web100_agent_find_var_and_group(agent, "LimCwnd", &group, &var))
          != WEB100_ERR_SUCCESS)
        return (22);
      // attempt writing variable value, log failure
      if ((web100_raw_write(var, cn, &buff)) != WEB100_ERR_SUCCESS) {
        log_println(4,
                    "Web100_raw_write(LimCwnd) failed with errorno=%d",
                    errno);
        return (23);
      }
    }
    buff = (MAX_TCP_WIN_BYTES * KILO_BITS) << rScale;

    if (autotune & 0x02) {  // autotune receive buffer is not enabled
      if ((web100_agent_find_var_and_group(agent, "LimRwin", &group, &var))
          != WEB100_ERR_SUCCESS)
        return (22);
      if ((web100_raw_write(var, cn, &buff)) != WEB100_ERR_SUCCESS) {
        log_println(4,
                    "Web100_raw_write(LimRwin) failed with errorno=%d",
                    errno);
        return (23);
      }
    }
  }

  return (0);
}
#endif

/**
 * @param sock integer socket file descriptor indicating data recipient
 * @param tcp_vars to local copies of tcp_stat variables
 * @return integer 0
 */
int tcp_stat_logvars(struct tcp_vars* vars, int count_vars) {
#if USE_WEB100
  int a, b;
  for (a = 0; a < sizeof(struct tcp_vars) / sizeof(tcp_stat_var); ++a) {
    const char* web100_name = tcp_names[a].web100_name;
    if (web100_name == NULL)
      continue;

    for (b = 0; b < count_vars; b++) {
      if (strcmp(web_vars[b].name, web100_name) == 0) {
        tcp_stat_var* var = ((tcp_stat_var*) vars) + a;
        *var = atoi(web_vars[b].value);
        log_println(5, "Found %s : %i", web100_name, *var);
        break;
      }
    }
    if (b == count_vars) {
      log_println(1, "WARNING: Failed to find Web100 var %s", web100_name);
    }
  }
#elif USE_TCPE
  int a, b;
  assert(dataDumpSave);
  for (a = 0; a < (sizeof(struct tcp_vars) / sizeof(tcp_stat_var)); ++a) {
    const char* web10g_name = tcp_names[a].web10g_name;
    if (web10g_name == NULL)
      continue;

    for (b = 0; b < ARRAYSIZE(dataDumpSave->val); ++b) {
      if (dataDumpSave->val[b].mask)
        continue;
      if (strcmp(tcpe_var_array[b].name, web10g_name) == 0) {
        tcp_stat_var* var = ((tcp_stat_var*) vars) + a;
        *var = dataDumpSave->val[b].uv32;
        log_println(5, "Found %s : %i", web10g_name, *var);
        break;
      }
    }
    if (b == ARRAYSIZE(dataDumpSave->val)) {
      log_println(1, "WARNING: Failed to find Web10g var %s", web10g_name);
    }
  }

  vars->AckPktsOut = 0;
  vars->Sndbuf = X_SndBuf;
  vars->MaxCwnd = MAX(vars->MaxSsCwnd, vars->MaxCaCwnd);

  tcpe_data_free(&dataDumpSave);
  dataDumpSave = NULL;
#endif
  return 0;
}

#if USE_WEB100
/**
 * Routine to read snaplog file and determine the number of times the
 * congestion window is reduced.
 *
 * @param agent pointer to a web100_agent
 * @param logname pointer to name of logfile
 * @param *dec_cnt pointer to integer indicating number of times decreased
 * @param *same_cnt pointer to integer indicating number of times kept same
 * @param *inc_cnt pointer to integer indicating number of times incremented
 * cn pointer to web100_connection
 * @return Integer, 0 on success, -1 on failure
 */

int CwndDecrease(web100_agent* agent, char* logname, u_int32_t *dec_cnt,
                 u_int32_t *same_cnt, u_int32_t *inc_cnt) {
  web100_var* var;
  char buff[256];
  web100_snapshot* snap;
  int s1, s2, cnt, rt;
  web100_log* log;
  web100_group* group;
  web100_agent* agnt;

  // open snaplog file to read values
  if ((log = web100_log_open_read(logname)) == NULL)
    return (0);
  if ((snap = web100_snapshot_alloc_from_log(log)) == NULL)
    return (-1);
  if ((agnt = web100_get_log_agent(log)) == NULL)
    return (-1);
  if ((group = web100_get_log_group(log)) == NULL)
    return (-1);

  // Find current values of the congestion window
  if (web100_agent_find_var_and_group(agnt, "CurCwnd", &group, &var)
      != WEB100_ERR_SUCCESS)
    return (-1);
  s2 = 0;
  cnt = 0;

  // get values and update counts
  while (web100_snap_from_log(snap, log) == 0) {
    if (cnt++ == 0)
      continue;
    s1 = s2;
    // Parse snapshot, returning variable values
    rt = web100_snap_read(var, snap, buff);
    s2 = atoi(web100_value_to_text(web100_get_var_type(var), buff));
    if (cnt < 20) {
      log_println(7, "Reading snaplog %p (%d), var = %s", snap, cnt,
                  (char*) var);
      log_println(
          7,
          "Checking for Cwnd decreases. rt=%d, s1=%d, s2=%d (%s), dec-cnt=%d",
          rt, s1, s2,
          web100_value_to_text(web100_get_var_type(var), buff),
          *dec_cnt);
    }
    if (s2 < s1)
      (*dec_cnt)++;
    if (s2 == s1)
      (*same_cnt)++;
    if (s2 > s1)
      (*inc_cnt)++;

    if (rt != WEB100_ERR_SUCCESS)
      break;
  }
  web100_snapshot_free(snap);
  web100_log_close_read(log);
  log_println(
      2,
      "-=-=-=- CWND window report: increases = %d, decreases = %d, "
      "no change = %d",
      *inc_cnt, *dec_cnt, *same_cnt);
  return (0);
}
#endif  // TODO: Implement in web10g when logging is doable.

#if USE_WEB100
/**
 * Generate TCP/IP checksum for our packet
 *
 * @param buff pointer to buffer
 * @param nwords integer length (in bytes) of the header
 * @return unsigned short checksum
 */
unsigned short csum(unsigned short *buff, int nwords) {
  /* generate a TCP/IP checksum for our packet */

  register int sum = 0;
  u_short answer = 0;
  register u_short *w = buff;
  register int nleft = nwords;

  // make 16 bit words out of every couple of 8 bit words and
  // add up
  while (nleft > 1) {
    sum += *w++;
    nleft -= 2;
  }

  if (nleft == 1) {
    *(u_char *) (&answer) = *(u_char *) w;
    sum += answer;
  }

  // form 16 bit words, add and store
  sum = (sum >> 16) + (sum & 0xffff);
  // then add carries to the sume
  sum += (sum >> 16);

  // 1s complement of the above yields checksum
  answer = ~sum;
  return (answer);
}

/**
 * Try to close out connections in an unexpected or erroneous state.
 *
 * This function seems unused currently.
 *
 * @return integer, 0 if kill succeeded, and
 * 	  -1 if kill failed or there was nothing to kill
 */
int KillHung(void) {
  web100_agent *agent;
  web100_group *group;
  web100_var *state, *var;
  web100_connection *conn;
  int cid, one = 1, hung = 0;
  int sd;
  char *pkt, buff[64];
  struct in_addr src, dst;
  struct iphdr *ip = NULL;
  struct tcphdr *tcp;
  struct sockaddr_in sin;
  struct pseudo_hdr *phdr;

  if ((agent = web100_attach(WEB100_AGENT_TYPE_LOCAL, NULL)) == NULL) {
    web100_perror("web100_attach");
    return (-1);
  }

  group = web100_group_head(agent);
  conn = web100_connection_head(agent);

  while (conn) {
    cid = web100_get_connection_cid(conn);
    web100_agent_find_var_and_group(agent, "State", &group, &state);
    web100_raw_read(state, conn, buff);
    if (atoi(web100_value_to_text(web100_get_var_type(state), buff)) == 9) {
      /* Connection is in Last_Ack state, and probably stuck, so generate and
         send a FIN packet to the remote client.  This should force the connection
         to close
         */

      log_println(3, "Connection %d was found in LastAck state", cid);
      sin.sin_family = AF_INET;

      pkt = malloc(sizeof(struct iphdr) + sizeof(struct tcphdr) + 24);
      memset(pkt, 0, (24 + sizeof(struct iphdr) + sizeof(struct tcphdr)));
      sd = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
      ip = (struct iphdr *) pkt;
      tcp = (struct tcphdr *) (pkt + sizeof(struct iphdr));
      phdr = (struct pseudo_hdr *) (pkt + sizeof(struct iphdr)
                                    - sizeof(struct pseudo_hdr));

      /* Build the TCP packet first, along with the pseudo header, the later
       * the IP header build process will overwrite the pseudo header fields
       */

      web100_agent_find_var_and_group(agent, "LocalAddress", &group,
                                      &var);
      web100_raw_read(var, conn, buff);
      log_println(3, "LocalAddress: '%s'",
                  web100_value_to_text(web100_get_var_type(var), buff));
      dst.s_addr = inet_addr(
          web100_value_to_text(web100_get_var_type(var), buff));
      web100_agent_find_var_and_group(agent, "RemAddress", &group, &var);
      web100_raw_read(var, conn, buff);
      src.s_addr = inet_addr(
          web100_value_to_text(web100_get_var_type(var), buff));
      log_println(3, "RemAddress: '%s'",
                  web100_value_to_text(web100_get_var_type(var), buff));

      phdr->protocol = IPPROTO_TCP;
      phdr->len = htons(sizeof(struct tcphdr) / 4);
      phdr->s_addr = src.s_addr;
      phdr->d_addr = dst.s_addr;

      web100_agent_find_var_and_group(agent, "LocalPort", &group, &var);
      web100_raw_read(var, conn, buff);
      tcp->dest = htons(
          atoi(web100_value_to_text(web100_get_var_type(var), buff)));
      log_println(3, "LocalPort: '%s'",
                  web100_value_to_text(web100_get_var_type(var), buff));
      web100_agent_find_var_and_group(agent, "RemPort", &group, &var);
      web100_raw_read(var, conn, buff);
      tcp->source = htons(
          atoi(web100_value_to_text(web100_get_var_type(var), buff)));
      log_println(3, "RemPort: '%s'",
                  web100_value_to_text(web100_get_var_type(var), buff));
      sin.sin_port = tcp->dest;

      web100_agent_find_var_and_group(agent, "RcvNxt", &group, &var);
      web100_raw_read(var, conn, buff);
      tcp->seq = htonl(
          atoll(
              web100_value_to_text(web100_get_var_type(var),
                                   buff)));
      log_println(3, "Seq No. (RcvNxt): '%s'",
                  web100_value_to_text(web100_get_var_type(var), buff));
      web100_agent_find_var_and_group(agent, "SndUna", &group, &var);
      web100_raw_read(var, conn, buff);
      tcp->ack_seq = htonl(
          atoll(
              web100_value_to_text(web100_get_var_type(var),
                                   buff)));
      log_println(3, "Ack No. (SndNxt): '%s'",
                  web100_value_to_text(web100_get_var_type(var), buff));

      tcp->window = 0x7fff;
      tcp->res1 = 0;
      tcp->doff = sizeof(struct tcphdr) / 4;
      /* tcp->syn = 1; */
      tcp->rst = 1;
      tcp->ack = 1;
      /* tcp->fin = 1; */
      tcp->urg_ptr = 0;
      tcp->check = csum((unsigned short *) phdr,
                        sizeof(struct tcphdr) + sizeof(struct pseudo_hdr));

      bzero(pkt, sizeof(struct iphdr));
      ip->ihl = 5;
      ip->version = 4;
      ip->tos = 0;
      ip->tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr);
      ip->id = htons(31890);
      ip->frag_off = htons(IP_DF);
      ip->ttl = IPDEFTTL;
      ip->protocol = IPPROTO_TCP; /* TCP packet */
      ip->saddr = src.s_addr;
      ip->daddr = dst.s_addr;
      sin.sin_addr.s_addr = dst.s_addr;

      ip->check = csum((unsigned short *) pkt, sizeof(struct iphdr));

      if (setsockopt(sd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0)
        return (-1);

      sendto(sd, (char *) pkt, 40, 0, (struct sockaddr *) &sin,
             sizeof(sin));

      hung = 1;
    }
    // returns the next connection in the sequence
    conn = web100_connection_next(conn);
  }

  // close and free allocated agent
  web100_detach(agent);
  /* free(pkt); */
  if (hung == 0)
    return (-1);
  return (0);
}
#endif
