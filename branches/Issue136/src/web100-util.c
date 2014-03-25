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
// TODO: is there better way to do this?? preprocessor macros or something
static struct tcp_name tcp_names[] = {
/* {"WEB100", "WEB10G" } / tcp_vars name / */
  {"Timeouts", "Timeouts"}, /* Timeouts */
  {"SumRTT", "SumRTT"}, /* SumRTT */
  {"CountRTT", "CountRTT"}, /* CountRTT */
  {"PktsRetrans", "SegsRetrans"}, /* PktsRetrans */
  {"FastRetran", "FastRetran"}, /* FastRetran */
  {"DataPktsOut", "DataSegsOut"}, /* DataPktsOut */
  {"AckPktsOut", "AckSegsOut"}, /* AckSegsOut (like AckSegsIn) =
                                                SegsOut - DataSegsOut */
  {"CurMSS", "CurMSS"}, /* CurrentMSS */
  {"DupAcksIn", "DupAcksIn"}, /* DupAcksIn */
  /* NOTE: in the server to client throughput test all packets received from client are ack's 
   * So SegsIn == AckPktsIn. Replacement in web10g is AckPktsIn == (SegsIn - DataSegsIn)
   * 
   * So web100 kernel code looked like this
   * 
	vars->PktsIn++;
	if (skb->len == th->doff*4) {
		vars->AckPktsIn++;
		if (TCP_SKB_CB(skb)->ack_seq == tp->snd_una)
			vars->DupAcksIn++;
	} else {
		vars->DataPktsIn++;
		vars->DataBytesIn += skb->len - th->doff*4;
	}
   * Web10G kernel code looks like this
   *
	vars->SegsIn++;
	if (skb->len == th->doff * 4) {
		if (TCP_SKB_CB(skb)->ack_seq == tp->snd_una)
			vars->DupAcksIn++;
	} else {
		vars->DataSegsIn++;
		vars->DataOctetsIn += skb->len - th->doff * 4;
	}
   */
  {"AckPktsIn", "AckSegsIn"}, /* AckPktsIn - not included in web10g */
  {"MaxRwinRcvd", "MaxRwinRcvd"}, /* MaxRwinRcvd */
  {"X_Sndbuf", NULL}, /* Sndbuf - Not in Web10g pull from socket */
  {"CurCwnd", "CurCwnd"}, /* CurrentCwnd */
  {"SndLimTimeRwin", "SndLimTimeRwin"}, /* SndLimTimeRwin */
  {"SndLimTimeCwnd", "SndLimTimeCwnd"}, /* SndLimTimeCwnd */
  {"SndLimTimeSender", "SndLimTimeSnd"}, /* SndLimTimeSender */
  {"DataBytesOut", "HCDataOctetsOut"}, /* DataBytesOut HC for 10Gig */
  {"SndLimTransRwin", "SndLimTransRwin"}, /* SndLimTransRwin */
  {"SndLimTransCwnd", "SndLimTransCwnd"}, /* SndLimTransCwnd */
  {"SndLimTransSender", "SndLimTransSnd"}, /* SndLimTransSender */
  {"MaxSsthresh", "MaxSsthresh"}, /* MaxSsthresh */
  {"CurRTO", "CurRTO"}, /* CurrentRTO */
  {"CurRwinRcvd", "CurRwinRcvd"}, /* CurrentRwinRcvd */
  {"MaxCwnd", "MaxCwnd"}, /* MaxCwnd not in web10g but is
            MAX(MaxSsCwnd, MaxCaCwnd) special case in web10g_find_val */
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
#elif USE_WEB10G
  return TOTAL_INDEX_MAX;
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
 * 			 currentMSS, WinScaleSent, WinScaleRecv, SumRTT, CountRTT and MaxRwinRcvd values
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
#elif USE_WEB10G
  struct estats_val value;
  estats_val_data* data = NULL;
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
  static char vars[][255] = {"WinScaleSent", "WinScaleRcvd", "SumRTT", "CountRTT", "MaxRwinRcvd" };

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

  // get current MSS value
#if USE_WEB100
    // read web100_group and web100_var of CurMSS into group and var
    web100_agent_find_var_and_group(agent, "CurMSS", &group, &var);
    // read variable value from web100 connection
    web100_raw_read(var, cn, buff);

    // get current MSS in textual format and append to results

    // get current MSS value and append to "results"
      currentMSSval = atoi(
          web100_value_to_text(web100_get_var_type(var), buff));
    snprintf(line, sizeof(line), "%s;",
          web100_value_to_text(web100_get_var_type(var), buff));
#elif USE_WEB10G
    int type;
    type = web10g_get_val(agent, cn, "CurMSS", &value);
    if (type == -1) {
      log_println(0, "Middlebox: Failed to read the value of CurMSS");
      return;
    } else {
        currentMSSval = value.uv32;
      snprintf(line, sizeof(line), "%u;", value.uv32);
    }
#endif

    if (strcmp(line, "4294967295;") == 0)
      snprintf(line, sizeof(line), "%d;", -1);

    // strlcat(results, line, sizeof(results));
    strlcat(results, line, results_strlen);
    log_print(3, "%s", line);

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
#elif USE_WEB10G
  estats_write_var("LimCwnd", (uint32_t)limcwnd_val, cn, agent);
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
#elif USE_WEB10G
  estats_val_data_new(&data);
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
#elif USE_WEB10G
    estats_read_vars(data, cn, agent);
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

  // get web100 values for the middlebox test result group
  for (i = 0; i < sizeof(vars) / sizeof(vars[0]); i++) {
#if USE_WEB100
    // read web100_group and web100_var of vars[i] into group and var
    web100_agent_find_var_and_group(agent, vars[i], &group, &var);
    // read variable value from web100 connection
    web100_raw_read(var, cn, buff);

    snprintf(line, sizeof(line), "%s;",
          web100_value_to_text(web100_get_var_type(var), buff));
#elif USE_WEB10G
    int type;
    type = web10g_get_val(agent, cn, vars[i], &value);
    if (type == -1) {
      log_println(0, "Middlebox: Failed to read the value of %s", vars[i]);
      return;
    } else {
      snprintf(line, sizeof(line), "%u;", value.uv32);
    }
#endif

    if (strcmp(line, "4294967295;") == 0)
      snprintf(line, sizeof(line), "%d;", -1);

    // strlcat(results, line, sizeof(results));
    strlcat(results, line, results_strlen);
    log_print(3, "%s", line);
  }


#if USE_WEB100
  log_println(5, "Finished with web100_middlebox() routine snap-0x%x, "
              "sndbuff=%x0x", snap, sndbuff);
  web100_snapshot_free(snap);
#elif USE_WEB10G

  estats_val_data_free(&data);
  log_println(5, "Finished with web10g_middlebox() routine, "
              "sndbuff=%x0x", sndbuff);
#endif
  /* free(sndbuff); */
}

/**
 * Get receiver side Web100/Web10G stats and writes them to the log file
 *
 * @param sock integer socket file descriptor
 * @param agent pointer to a tcp_stat_agent
 * @param cn A tcp_stat_connection
 * @param count_vars integer number of tcp_stat_vars to get value of
 *
 */
void tcp_stat_get_data_recv(int sock, tcp_stat_agent* agent,
                            tcp_stat_connection cn, int count_vars) {
#if USE_WEB100
  web100_var* var = NULL;
  web100_group* group = NULL;
#elif USE_WEB10G
  estats_val_data* data = NULL;
  estats_error* err = NULL;
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
#elif USE_WEB10G
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
#elif USE_WEB10G
  estats_val_data_new(&data);
  estats_read_vars(data, cn, agent);

  // Loop through all the web10g variables and write to file/log_print them
  for (i = 0; i < data->length; i++) {
    if (data->val[i].masked) continue;
    char * str = NULL;

    if ((err = estats_val_as_string(&str, &data->val[i], estats_var_array[i].valtype)) != NULL) {
      log_println(0, "Error: tcp_stat_get_data_recv - estats_val_as_string");
      estats_error_print(stderr, err);
      estats_error_free(&err);
      continue;
    }
    if (fp)
      fprintf(fp, "%s;", str);
    log_println(9, "%s: %s", estats_var_array[i].name, estats_read_vars);
  }

  estats_val_data_free(&data);
#endif

  // close file pointers after web100 variables have been fetched
  if (fp) {
    fprintf(fp, "\n");
    fclose(fp);
  }
}

#if USE_WEB10G
/* Persistent storage needed. These are filled by tcp_stat_get_data 
 * and later read by tcp_stat_logvars and free()'d */
static estats_val_data* dataDumpSave = NULL;
static int X_SndBuf = -1;
static int X_RcvBuf = -1;
#endif



#if USE_WEB10G

/**
 * Print a web10g variable to a line using the new name and then write 
 * this line to the test socket. Used by tcp_stat_get_data().
 * i.e. 
 * <org_name>: <value>
 * 
 * @param old_name The name within web10g, or an additional name 
 *              supported by web10g_find_var
 * @param new_name The name to print this as (i.e. the web100 name)
 * @param snap A web10g snapshot
 * @param line A char* to write the line to
 * @param line_size Size of line in bytes
 * @param ctlsock The socket to write to
 *
 * If this fails nothing is sent on the cltsocket and the error will
 * be logged.
 * 
 */
static void print_10gvar_renamed(const char * old_name,
      const char * new_name, const tcp_stat_snap* snap, char * line,
      int line_size, int ctlsock) {
  int type;
  struct estats_val val;
  estats_error* err;
  char * str;

  type = web10g_find_val(snap, old_name, &val);
  if (type == -1) {
    log_println(0, "In print_10gvar_renamed(), web10g_find_val()"
                    " failed to find %s", old_name);
  } else {
    if ((err = estats_val_as_string(&str, &val, type)) != NULL) {
      log_println(0, "In print_10gvar_renamed(), estats_val_as_string()"
                          " failed for %s", old_name);
      estats_error_print(stderr, err);
      estats_error_free(&err);
    } else {
      snprintf(line, line_size, "%s: %s\n", new_name, str);
      send_msg(ctlsock, TEST_MSG, line, strlen(line));
      free(str);
      str = NULL;
    }
  }
}

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
    /* Why do we atoi after getting as text anyway ?? */
    snprintf(line, sizeof(line), "%s: %d\n", web_vars[i].name,
             atoi(web_vars[i].value));
    send_msg(ctlsock, TEST_MSG, line, strlen(line));
    log_print(9, "%s", line);
  }
  log_println(6, "S2C test - Send web100 data to client pid=%d", getpid());
  return (0);
#elif USE_WEB10G
  int j;
  unsigned int xbuf_size;
  struct estats_val val;
  estats_error* err;

  xbuf_size = sizeof(X_RcvBuf);
  if (getsockopt(testsock,
        SOL_SOCKET, SO_RCVBUF, (void *)&X_RcvBuf, &xbuf_size) != 0) {
    log_println(0, "Error: failed to getsockopt() SO_RCVBUF");
  }
  xbuf_size = sizeof(X_SndBuf);
  if (getsockopt(testsock,
        SOL_SOCKET, SO_SNDBUF, (void *)&X_SndBuf, &xbuf_size) != 0) {
    log_println(0, "Error: failed to getsockopt() SO_RCVBUF");
  }

  assert(snap);

  /* Need to save this for later */
  estats_val_data_new(&dataDumpSave);
  memcpy(dataDumpSave, snap, sizeof(struct estats_val_data)
                          + (sizeof(struct estats_val) * snap->length));

  for (j = 0; j < snap->length; j++) {
    char *str;
    if (snap->val[j].masked) continue;

    if ((err = estats_val_as_string(&str, &snap->val[j],
                              estats_var_array[j].valtype)) != NULL) {
      log_println(0, "In tcp_stat_get_data() estats_val_as_string()"
                          " failed for %s", estats_var_array[j].name);
      estats_error_print(stderr, err);
      estats_error_free(&err);
      continue;
    }
    snprintf(line, sizeof(line), "%s: %s\n",
                 estats_var_array[j].name, str);
    send_msg(ctlsock, TEST_MSG, (const void *) line, strlen(line));
    log_print(9, "%s", line);
    free(str);
    str = NULL;
  }

  /* This is the list of changed variable names that the client tries to read.
   * Web100 -> Web10g
   * ECNEnabled -> ECN
   * NagleEnabled -> Nagle
   * SACKEnabled -> WillSendSACK & WillUseSACK
   * TimestampsEnabled -> TimeStamps
   * PktsRetrans -> SegsRetrans
   * X_Rcvbuf -> Not in web10g not used by client but send anyway
   * X_Sndbuf -> Not in web10g but could be interesting for the client
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
  static const char* frame_web100 = "-~~~Web100_old_var_names~~~-: 1\n";
  int type;
  char *str = NULL;
  send_msg(ctlsock, TEST_MSG, (const void *)frame_web100,
                                                strlen(frame_web100));

  /* ECNEnabled -> ECN */
  type = web10g_find_val(snap, "ECN", &val);
  if (type != ESTATS_SIGNED32) {
    log_println(0, "In tcp_stat_get_data(), web10g_find_val()"
                    " failed to find ECN bad type=%d", type);
  } else {
    snprintf(line, sizeof(line), "ECNEnabled: %"PRId32"\n", (val.sv32 == 1) ? 1 : 0);
    send_msg(ctlsock, TEST_MSG, line, strlen(line));
  }

  /* NagleEnabled -> Nagle */
  type = web10g_find_val(snap, "Nagle", &val);
  if (type != ESTATS_SIGNED32) {
    log_println(0, "In tcp_stat_get_data(), web10g_find_val()"
                    " failed to find Nagle bad type=%d", type);
  } else {
    snprintf(line, sizeof(line), "NagleEnabled: %"PRId32"\n", (val.sv32 == 2) ? 1 : 0);
    send_msg(ctlsock, TEST_MSG, line, strlen(line));
  }

  /* SACKEnabled -> WillUseSACK & WillSendSACK */
  type = web10g_find_val(snap, "WillUseSACK", &val);
  if (type == -1) {
    log_println(0, "In tcp_stat_get_data(), web10g_find_val()"
                    " failed to find WillUseSACK bad type=%d", type);
  } else {
  /* Yes this comes through as 3 from web100 */
    snprintf(line, sizeof(line), "SACKEnabled: %d\n", (val.sv32 == 1) ? 3 : 0);
    send_msg(ctlsock, TEST_MSG, line, strlen(line));
  }

  /* TimestampsEnabled -> TimeStamps */
  type = web10g_find_val(snap, "TimeStamps", &val);
  if (type != ESTATS_SIGNED32) {
    log_println(0, "In tcp_stat_get_data(), web10g_find_val()"
                    " failed to find TimeStamps bad type=%d", type);
  } else {
    snprintf(line, sizeof(line), "TimestampsEnabled: %"PRId32"\n", (val.sv32 == 1) ? 1 : 0);
    send_msg(ctlsock, TEST_MSG, line, strlen(line));
  }

  /* PktsRetrans -> SegsRetrans */
  print_10gvar_renamed("SegsRetrans", "PktsRetrans", snap, line,
                              sizeof(line), ctlsock);

  /* DataPktsOut -> DataSegsOut */
  print_10gvar_renamed("DataSegsOut", "DataPktsOut", snap, line,
                              sizeof(line), ctlsock);

  /* MaxCwnd -> MAX(MaxSsCwnd, MaxCaCwnd) */
  print_10gvar_renamed("MaxCwnd", "MaxCwnd", snap, line,
                              sizeof(line), ctlsock);

  /* SndLimTimeSender -> SndLimTimeSnd */
  print_10gvar_renamed("SndLimTimeSnd", "SndLimTimeSender", snap, line,
                              sizeof(line), ctlsock);

  /* DataBytesOut -> DataOctetsOut */
  print_10gvar_renamed("HCDataOctetsOut", "DataBytesOut", snap, line,
                              sizeof(line), ctlsock);

  /* SndLimTransSender -> SndLimTransSnd */
  print_10gvar_renamed("SndLimTransSnd", "SndLimTransSender", snap, line,
                            sizeof(line), ctlsock);

  /* PktsOut -> SegsOut */
  print_10gvar_renamed("SegsOut", "PktsOut", snap, line,
                            sizeof(line), ctlsock);

  /* CongestionSignals -> CongSignals */
  print_10gvar_renamed("CongSignals", "CongestionSignals", snap, line,
                            sizeof(line), ctlsock);

  /* RcvWinScale -> Same as WinScaleSent if WinScaleSent != -1 */
  type = web10g_find_val(snap, "WinScaleSent", &val);
  if (type == -1) {
    log_println(0, "In tcp_stat_get_data(), web10g_find_val()"
                    " failed to find WinScaleSent");
  } else {
    if (val.sv32 == -1)
      snprintf(line, sizeof(line), "RcvWinScale: %u\n", 0);
    else
      snprintf(line, sizeof(line), "RcvWinScale: %d\n", val.sv32);
    send_msg(ctlsock, TEST_MSG, (const void *) line, strlen(line));
  }

  /* X_Rcvbuf & X_Sndbuf */
  snprintf(line, sizeof(line), "X_Rcvbuf: %d\n", X_RcvBuf);
  send_msg(ctlsock, TEST_MSG, (const void *) line, strlen(line));
  snprintf(line, sizeof(line), "X_Sndbuf: %d\n", X_SndBuf);
  send_msg(ctlsock, TEST_MSG, (const void *) line, strlen(line));

  send_msg(ctlsock, TEST_MSG, (const void *)frame_web100,
                                                strlen(frame_web100));

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
 * @return On successful fetch of required web100_variables, integers:
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
#elif USE_WEB10G
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
  int PktsIn = -1;
  int DataPktsIn = -1;
  int has_AckPktsIn = 0;

    for (b = 0; b < count_vars; b++) {
      if (strcmp(web_vars[b].name, web100_name) == 0) {
        tcp_stat_var* var = &((tcp_stat_var *)vars)[a];
        *var = atoi(web_vars[b].value);
        log_println(5, "Found %s : %i", web100_name, *var);
        break;
      }
    }
    if (b == count_vars) {
      log_println(1, "WARNING: Failed to find Web100 var %s", web100_name);
    }
  }
#elif USE_WEB10G
  int a;
  estats_val val;
  assert(dataDumpSave);

  for (a = 0; a < sizeof(struct tcp_vars) / sizeof(tcp_stat_var); ++a) {
    const char* web10g_name = tcp_names[a].web10g_name;
    int vartype;
    if (web10g_name == NULL)
      continue;

    /* Find each item in the list */
    if ((vartype = web10g_find_val(dataDumpSave, web10g_name, &val)) == -1) {
      log_println(1, "WARNING: Failed to find Web10g var %s", web10g_name);
    } else {
      tcp_stat_var* var = &((tcp_stat_var *)vars)[a];
      switch (vartype) {
        case ESTATS_UNSIGNED64:
          *var = (tcp_stat_var) val.uv64;
          break;
        case ESTATS_UNSIGNED32:
          *var = (tcp_stat_var) val.uv32;
          break;
        case ESTATS_SIGNED32:
          *var = (tcp_stat_var) val.sv32;
          break;
        case ESTATS_UNSIGNED16:
          *var = (tcp_stat_var) val.uv16;
          break;
        case ESTATS_UNSIGNED8:
          *var = (tcp_stat_var) val.uv8;
          break;
      }
      log_println(5, "Found %s : %i", web10g_name, *var);
    }
  }
  /* Our special case */
  vars->Sndbuf = X_SndBuf;

  estats_val_data_free(&dataDumpSave);
#endif
  return 0;
}


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

int CwndDecrease(char* logname, u_int32_t *dec_cnt,
                 u_int32_t *same_cnt, u_int32_t *inc_cnt) {
#if USE_WEB100
  web100_var* var;
  char buff[256];
  web100_snapshot* snap;
  int s1, s2, cnt, rt;
  web100_log* log;
  web100_group* group;
  web100_agent* agnt;
#elif USE_WEB10G
  estats_val var;
  char buff[256];
  estats_val_data* snap;
  int s1, s2, cnt, rt;
  estats_record* log;
#endif

#if USE_WEB100
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
#elif USE_WEB10G
  estats_record_open(&log, logname, "r");
#endif

  s2 = 0;
  cnt = 0;

  // get values and update counts
#if USE_WEB100
  while (web100_snap_from_log(snap, log) == 0) {
#elif USE_WEB10G
  while (estats_record_read_data(&snap, log) == NULL) {
#endif
    if (cnt++ == 0)
      continue;
    s1 = s2;
    // Parse snapshot, returning variable values
#if USE_WEB100
    rt = web100_snap_read(var, snap, buff);
    s2 = atoi(web100_value_to_text(web100_get_var_type(var), buff));
#elif USE_WEB10G
    rt = web10g_find_val(snap, "CurCwnd", &var);
    s2 = var.uv32;
#endif

    if (cnt < 20) {
#if USE_WEB100
      log_println(7, "Reading snaplog %p (%d), var = %s", snap, cnt,
                  (char*) var);
      log_println(
          7,
          "Checking for Cwnd decreases. rt=%d, s1=%d, s2=%d (%s), dec-cnt=%d",
          rt, s1, s2,
          web100_value_to_text(web100_get_var_type(var), buff),
          *dec_cnt);
#elif USE_WEB10G
      log_println(7, "Reading snaplog %p (%d), var = %"PRIu64, snap, cnt,
                     var.uv64);
      log_println(
          7,
          "Checking for Cwnd decreases. rt=%d, s1=%d, s2=%d, dec-cnt=%d",
          rt, s1, s2,
          *dec_cnt);
#endif
    }
    if (s2 < s1)
      (*dec_cnt)++;
    if (s2 == s1)
      (*same_cnt)++;
    if (s2 > s1)
      (*inc_cnt)++;

#if USE_WEB100
    if (rt != WEB100_ERR_SUCCESS)
      break;
#elif USE_WEB10G
    estats_val_data_free(&snap);
    if (rt != -1)
      break;
#endif
  }
#if USE_WEB100
  web100_snapshot_free(snap);
  web100_log_close_read(log);
#elif USE_WEB10G
  estats_record_close(&log);
#endif
  log_println(
      2,
      "-=-=-=- CWND window report: increases = %d, decreases = %d, "
      "no change = %d",
      *inc_cnt, *dec_cnt, *same_cnt);
  return (0);
}

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
