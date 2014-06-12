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
#include "jsonutils.h"

#if USE_WEB100
#include <web100.h>
#endif
#if USE_WEB10G
#include <estats.h>
#endif

static struct web100_variables {
  int defined;
  char name[256];  // key
  char value[256];  // value
} web_vars[WEB100_VARS];

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
int tcp_stats_init(char *VarFileName) {
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
    web_vars[count_vars].defined = 1;
    count_vars++;
  }
  web_vars[count_vars].defined = 0;
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
 * @param results_keys pointer to string containing names of variables stored in results_values
 * @param results_values pointer to string containing Server address , client address
 * 			 currentMSS, WinScaleSent, WinScaleRecv, SumRTT, CountRTT and MaxRwinRcvd values
 *
 *
 */
void tcp_stat_middlebox(int sock, tcp_stat_agent* agent, tcp_stat_connection cn, char *results_keys,
						size_t results_keys_strlen, char *results_values, size_t results_strlen) {
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
  tcp_stat_group *group;
  tcp_stat_snap *snap;

  // middlebox test results
  static char vars[][255] = {"WinScaleSent", "WinScaleRcvd", "SumRTT", "CountRTT", "MaxRwinRcvd" };

  assert(results_keys);
  assert(results_values);

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
  strlcat(results_keys, SERVER_ADDRESS, results_keys_strlen);
  strlcat(results_keys, ";", results_keys_strlen);
  strlcat(results_values, line, results_strlen);

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
  strlcat(results_keys, CLIENT_ADDRESS, results_keys_strlen);
  strlcat(results_keys, ";", results_keys_strlen);
  strlcat(results_values, line, results_strlen);

  // get current MSS value
  tcp_stats_read_var(agent, cn, "CurMSS", buff, sizeof(buff));
  currentMSSval = atoi(buff);

  strlcat(results_keys, "CurMSS;", results_keys_strlen);
  strlcat(results_values, buff, results_strlen);

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
  tcp_stats_set_cwnd(agent, cn, limcwnd_val);

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

  group = tcp_stats_get_group(agent, "read");
  snap = tcp_stats_init_snapshot(agent, cn, group);

  FD_ZERO(&wfd);
  FD_SET(sock, &wfd);
  sel_tv.tv_sec = 5;  // 5 seconds maximum for select call below to complete
  sel_tv.tv_usec = 0;  // 5s + 0ms
  while ((ret = select(sock + 1, NULL, &wfd, NULL, &sel_tv)) > 0) {
    if ((ret == -1) && (errno == EINTR)) /* a signal arrived, ignore it */
      continue;

    tcp_stats_take_snapshot(agent, cn, snap);

    // get next sequence # to be sent
    tcp_stats_snap_read_var(agent, snap, "SndNxt", tmpstr, sizeof(tmpstr));
    SndMax = atoi(tmpstr);

    // get oldest un-acked sequence number
    tcp_stats_snap_read_var(agent, snap, "SndUna", tmpstr, sizeof(tmpstr));
    SndUna = atoi(tmpstr);

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
    if (tcp_stats_read_var(agent, cn, vars[i], buff, sizeof(buff)) != 0) {
      log_println(0, "Middlebox: Failed to read the value of %s", vars[i]);
      return;
    }

    snprintf(line, sizeof(line), "%s;", buff);

    if (strcmp(line, "4294967295;") == 0)
      snprintf(line, sizeof(line), "%d;", -1);

    // strlcat(results, line, sizeof(results));
    strlcat(results_keys, vars[i], results_keys_strlen);
    strlcat(results_keys, ";", results_keys_strlen);
    strlcat(results_values, line, results_strlen);
    log_print(3, "%s", line);
  }


  log_println(5, "Finished with middlebox() routine, "
                  "sndbuff=%x0x", sndbuff);
  tcp_stats_free_snapshot(snap);
  /* free(sndbuff); */
}

/**
 * Get receiver side Web100/Web10G stats and writes them to the log file
 *
 * @param sock integer socket file descriptor
 * @param agent pointer to a tcp_stat_agent
 * @param cn A tcp_stat_connection
 *
 */
void tcp_stat_get_data_recv(int sock, tcp_stat_agent* agent,
                            tcp_stat_connection cn) {
  socklen_t len;
  struct sockaddr_storage addr;
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

  len = sizeof(addr);
  getpeername(sock, (struct sockaddr*)&addr, &len);
  addr2a(&addr, buf, sizeof(buf));
  snprintf(line, sizeof(line), "%s;", buf);
  // write remote address to log file
  if (fp)
    fprintf(fp, "%s", line);

  // get values for other web100 variables and write to the log file

  for (i = 0; web_vars[i].defined; i++) {
    char buf[1024];
    if (tcp_stats_read_var(agent, cn, web_vars[i].name, buf, sizeof(buf)) != 0) {
      log_println(1, "Variable %d (%s) not found in KIS", i,
                  web_vars[i].name);
      continue;
    }

    snprintf(web_vars[i].value, sizeof(web_vars[i].value), "%s", buf);
    if (fp)
      fprintf(fp, "%d;", (int32_t) atoi(web_vars[i].value));
    log_println(9, "%s: %d", web_vars[i].name, atoi(web_vars[i].value));
  }

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
 * @param jsonSupport Indicates if messages should be sent using JSON format
 *
 * If this fails nothing is sent on the cltsocket and the error will
 * be logged.
 * 
 */
static void print_10gvar_renamed(const char * old_name,
      const char * new_name, const tcp_stat_snap* snap, char * line,
      int line_size, int ctlsock, int jsonSupport) {
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
      send_json_message(ctlsock, TEST_MSG, line, strlen(line), jsonSupport, JSON_SINGLE_VALUE);
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
 * @param jsonSupport indicates if messages should be sent usin JSON format
 *
 */
int tcp_stat_get_data(tcp_stat_snap* snap, int testsock, int ctlsock,
                      tcp_stat_agent* agent, int jsonSupport) {
  char line[256];
#if USE_WEB100
  int i;
  web100_var* var;
  web100_group* group;
  char buf[32];

  assert(snap);
  assert(agent);

  for (i = 0; i < web_vars[i].defined; i++) {
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
    send_json_message(ctlsock, TEST_MSG, line, strlen(line), jsonSupport, JSON_SINGLE_VALUE);
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
    send_json_message(ctlsock, TEST_MSG, line, strlen(line), jsonSupport, JSON_SINGLE_VALUE);
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
  send_json_message(ctlsock, TEST_MSG, frame_web100, strlen(frame_web100), jsonSupport, JSON_SINGLE_VALUE);

  /* ECNEnabled -> ECN */
  type = web10g_find_val(snap, "ECN", &val);
  if (type != ESTATS_SIGNED32) {
    log_println(0, "In tcp_stat_get_data(), web10g_find_val()"
                    " failed to find ECN bad type=%d", type);
  } else {
    snprintf(line, sizeof(line), "ECNEnabled: %"PRId32"\n", (val.sv32 == 1) ? 1 : 0);
    send_json_message(ctlsock, TEST_MSG, line, strlen(line), jsonSupport, JSON_SINGLE_VALUE);
  }

  /* NagleEnabled -> Nagle */
  type = web10g_find_val(snap, "Nagle", &val);
  if (type != ESTATS_SIGNED32) {
    log_println(0, "In tcp_stat_get_data(), web10g_find_val()"
                    " failed to find Nagle bad type=%d", type);
  } else {
    snprintf(line, sizeof(line), "NagleEnabled: %"PRId32"\n", (val.sv32 == 2) ? 1 : 0);
    send_json_message(ctlsock, TEST_MSG, line, strlen(line), jsonSupport, JSON_SINGLE_VALUE);
  }

  /* SACKEnabled -> WillUseSACK & WillSendSACK */
  type = web10g_find_val(snap, "WillUseSACK", &val);
  if (type == -1) {
    log_println(0, "In tcp_stat_get_data(), web10g_find_val()"
                    " failed to find WillUseSACK bad type=%d", type);
  } else {
  /* Yes this comes through as 3 from web100 */
    snprintf(line, sizeof(line), "SACKEnabled: %d\n", (val.sv32 == 1) ? 3 : 0);
    send_json_message(ctlsock, TEST_MSG, line, strlen(line), jsonSupport, JSON_SINGLE_VALUE);
  }

  /* TimestampsEnabled -> TimeStamps */
  type = web10g_find_val(snap, "TimeStamps", &val);
  if (type != ESTATS_SIGNED32) {
    log_println(0, "In tcp_stat_get_data(), web10g_find_val()"
                    " failed to find TimeStamps bad type=%d", type);
  } else {
    snprintf(line, sizeof(line), "TimestampsEnabled: %"PRId32"\n", (val.sv32 == 1) ? 1 : 0);
    send_json_message(ctlsock, TEST_MSG, line, strlen(line), jsonSupport, JSON_SINGLE_VALUE);
  }

  /* PktsRetrans -> SegsRetrans */
  print_10gvar_renamed("SegsRetrans", "PktsRetrans", snap, line,
                              sizeof(line), ctlsock, jsonSupport);

  /* DataPktsOut -> DataSegsOut */
  print_10gvar_renamed("DataSegsOut", "DataPktsOut", snap, line,
                              sizeof(line), ctlsock, jsonSupport);

  /* MaxCwnd -> MAX(MaxSsCwnd, MaxCaCwnd) */
  print_10gvar_renamed("MaxCwnd", "MaxCwnd", snap, line,
                              sizeof(line), ctlsock, jsonSupport);

  /* SndLimTimeSender -> SndLimTimeSnd */
  print_10gvar_renamed("SndLimTimeSnd", "SndLimTimeSender", snap, line,
                              sizeof(line), ctlsock, jsonSupport);

  /* DataBytesOut -> DataOctetsOut */
  print_10gvar_renamed("HCDataOctetsOut", "DataBytesOut", snap, line,
                              sizeof(line), ctlsock, jsonSupport);

  /* SndLimTransSender -> SndLimTransSnd */
  print_10gvar_renamed("SndLimTransSnd", "SndLimTransSender", snap, line,
                            sizeof(line), ctlsock, jsonSupport);

  /* PktsOut -> SegsOut */
  print_10gvar_renamed("SegsOut", "PktsOut", snap, line,
                            sizeof(line), ctlsock, jsonSupport);

  /* CongestionSignals -> CongSignals */
  print_10gvar_renamed("CongSignals", "CongestionSignals", snap, line,
                            sizeof(line), ctlsock, jsonSupport);

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
    send_json_message(ctlsock, TEST_MSG, line, strlen(line), jsonSupport, JSON_SINGLE_VALUE);
  }

  /* X_Rcvbuf & X_Sndbuf */
  snprintf(line, sizeof(line), "X_Rcvbuf: %d\n", X_RcvBuf);
  send_json_message(ctlsock, TEST_MSG, line, strlen(line), jsonSupport, JSON_SINGLE_VALUE);
  snprintf(line, sizeof(line), "X_Sndbuf: %d\n", X_SndBuf);
  send_json_message(ctlsock, TEST_MSG, line, strlen(line), jsonSupport, JSON_SINGLE_VALUE);

  send_json_message(ctlsock, TEST_MSG, frame_web100, strlen(frame_web100), jsonSupport, JSON_SINGLE_VALUE);

  log_println(6, "S2C test - Send web100 data to client pid=%d", getpid());
  return 0;
#endif
}

/**
 * Check if the "Auto Tune Send Buffer" and "Auto Tune Receive Buffer" options
 * are enabled and return status on each
 *
 * @param sock integer socket file descriptor indicating data recipient
 * @param agent pointer to a web100_agent
 * @return On successful fetch of required web100_variables, integers:
 *  				0x01 if "Autotune send buffer" is not enabled
 *  				0x02 if "Autotune receive buffer" is not enabled
 *  				0x03 if both " Autotune send buffer" and "Autotune receive buffer" are not enabled.
 *  		If failure, error codes :
 *  				10 if web100-connection is null.
 * 					22 Cannot find X_SBufMode or X_RBufMode web100_variable's var/group.
 * 					23 cannot read the value of the X_SBufMode or X_RBufMode web100_variable.
 */

int tcp_stats_autotune_enabled(tcp_stat_agent *agent, int sock) {
#if USE_WEB100
  web100_var* var;
  char buf[32];
  web100_group* group;
  web100_connection *cn;
  int i, j = 0;

  cn = tcp_stats_connection_from_socket(agent, sock);
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

/**
 * @param tcp_vars to local copies of tcp_stat variables
 * @return integer 0
 */
int tcp_stat_logvars(struct tcp_vars* vars) {
#if USE_WEB100
  int a, b;
  for (a = 0; a < sizeof(struct tcp_vars) / sizeof(tcp_stat_var); ++a) {
    const char* web100_name = tcp_names[a].web100_name;
    if (web100_name == NULL)
      continue;

    for (b = 0; web_vars[b].defined; b++) {
      if (strcmp(web_vars[b].name, web100_name) == 0) {
        tcp_stat_var* var = &((tcp_stat_var *)vars)[a];
        *var = atoi(web_vars[b].value);
        log_println(5, "Found %s : %i", web100_name, *var);
        break;
      }
    }
    if (web_vars[b].defined == 0) {
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

int CwndDecrease(SnapResults *results, u_int32_t *dec_cnt,
                 u_int32_t *same_cnt, u_int32_t *inc_cnt) {
  int i;
  int s1, s2, cnt;
  char buf[1024];

  s2 = 0;
  cnt = 0;

  for(i = 0; i < results->collected; i++) {
    tcp_stat_snap *snap = results->snapshots[i];
    if (cnt++ == 0) {
      continue;
    }
    s1 = s2;
    // Parse snapshot, returning variable values
    if (tcp_stats_snap_read_var(results->agent, snap, "CurCwnd", buf, sizeof(buf)) != 0) {
     continue;
    }

    s2 = atoi(buf);

    if (cnt < 20) {
      log_println(7, "Reading snaplog %p (%d), var = %s", snap, cnt, buf);
      log_println(
          7,
          "Checking for Cwnd decreases. s1=%d, s2=%d, dec-cnt=%d",
          s1, s2, *dec_cnt);
    }
    if (s2 < s1)
      (*dec_cnt)++;
    if (s2 == s1)
      (*same_cnt)++;
    if (s2 > s1)
      (*inc_cnt)++;
  }

  log_println(
      2,
      "-=-=-=- CWND window report: increases = %d, decreases = %d, "
      "no change = %d",
      *inc_cnt, *dec_cnt, *same_cnt);
  return (0);
}

int tcp_stats_read_var(tcp_stat_agent *agent, tcp_stat_connection conn, const char *var_name, char *buf, int bufsize) {
#if USE_WEB100
  web100_group *group;
  web100_var* var;
  char tmp[1024];
  char *tmpstr;

  // read web100_group and web100_var of vars[i] into group and var
  web100_agent_find_var_and_group(agent, var_name, &group, &var);
  // read variable value from web100 connection
  web100_raw_read(var, conn, tmp);
  tmpstr = web100_value_to_text(web100_get_var_type(var), tmp);
  strlcpy(buf, tmpstr, bufsize);
#elif USE_WEB10G
  struct estats_val value;
  int type;

  type = web10g_get_val(agent, conn, var_name, &value);
  if (type == -1) {
    return -1;
  }

  snprintf(buf, bufsize, "%u", value.uv32);
#endif

  return 0;
}

 int tcp_stats_snap_read_var(tcp_stat_agent *agent, tcp_stat_snap *snap, const char *var_name, char *buf, int bufsize) {
 #if USE_WEB100
   web100_group* group;
   web100_var* var;
   char tmpstr[256];

   web100_agent_find_var_and_group(agent, var_name, &group, &var);
   web100_snap_read(var, snap, tmpstr);
   snprintf(buf, bufsize, "%s", web100_value_to_text(web100_get_var_type(var), tmpstr));
 #elif USE_WEB10G
   struct estats_val value;

   web10g_find_val(snap, var_name, &value);
   snprintf(buf, bufsize, "%u", value.uv32);
 #endif
 }

tcp_stat_connection tcp_stats_connection_from_socket(tcp_stat_agent *agent, int sock) {
    tcp_stat_connection retval;

#if USE_WEB100
  retval = web100_connection_from_socket(agent, sock);
#elif USE_WEB10G
  retval = web10g_connection_from_socket(agent, sock);
  if (retval == -1)
    retval = NULL;
#endif
  return retval;
}

 void tcp_stats_set_cwnd(tcp_stat_agent *agent, tcp_stat_connection cn, uint32_t cwnd) {
 #if USE_WEB100
   web100_var* LimCwnd;
   web100_group* group;

   // get web100_var and web100_group
   web100_agent_find_var_and_group(agent, "LimCwnd", &group, &LimCwnd);

   // set TCP CWND web100 variable to twice the current MSS Value
   web100_raw_write(LimCwnd, cn, &cwnd);
 #elif USE_WEB10G
   estats_write_var("LimCwnd", (uint32_t)cwnd, cn, agent);
 #endif
 }

tcp_stat_agent *tcp_stats_init_agent() {
 tcp_stat_agent *agent;

#if USE_WEB100
  if ((agent = web100_attach(WEB100_AGENT_TYPE_LOCAL,
                             NULL)) == NULL) {
    web100_perror("web100_attach");
    return NULL;
  }
#elif USE_WEB10G
  if (estats_nl_client_init(&agent) != NULL) {
    log_println(0,
                  "Error: estats_client_init failed."
                  "Unable to use web10g.");
    return NULL;
  }
#endif
  return agent;
}

void tcp_stats_free_agent(tcp_stat_agent *agent) {
#if USE_WEB100
  web100_detach(agent);
#elif USE_WEB10G
  estats_nl_client_destroy(&agent);
#endif
  return;
}

tcp_stat_group *tcp_stats_get_group(tcp_stat_agent *agent, char *group_name) {
    tcp_stat_group *retval;

#if USE_WEB100
  retval = web100_group_find(agent, group_name);
#elif USE_WEB10G
  retval = NULL;
#endif

 return retval;
}

tcp_stat_snap *tcp_stats_init_snapshot(tcp_stat_agent *agent, tcp_stat_connection conn, tcp_stat_group *group) {
    tcp_stat_snap *retval;

#if USE_WEB100
  retval = web100_snapshot_alloc(group, conn);
#elif USE_WEB10G
  estats_val_data_new(&retval);
#endif

  return retval;
}

void tcp_stats_take_snapshot(tcp_stat_agent *agent, tcp_stat_connection conn, tcp_stat_snap *snap) {
#if USE_WEB100
  web100_snap(snap);
#elif USE_WEB10G
  estats_read_vars(snap, conn, agent);
#endif
  return;
}

void tcp_stats_free_snapshot(tcp_stat_snap *snap) {
#if USE_WEB100
  web100_snapshot_free(snap);
#elif USE_WEB10G
  estats_val_data_free(&snap);
#endif
}

tcp_stat_log *tcp_stats_open_log(char *filename, tcp_stat_connection conn, tcp_stat_group *group, char *mode) {
    tcp_stat_log *retval;

#if USE_WEB100
    if (strcmp(mode, "w") == 0) {
      retval = web100_log_open_write(filename, conn, group);
    }
    else if (strcmp(mode, "r") == 0) {
      retval = web100_log_open_read(filename);
    }
    else {
      retval = NULL;
    }
#elif USE_WEB10G
  estats_record_open(&retval, filename, mode);
#endif

  return retval;
}

int tcp_stats_read_snapshot(tcp_stat_snap **snap, tcp_stat_log *log) {
  int retval;

#if USE_WEB100
  if (*snap == NULL)
    if ((*snap = web100_snapshot_alloc_from_log(log)) == NULL)
      return (-1);

  retval = web100_snap_from_log(*snap, log);
#elif USE_WEB10G
  retval = estats_record_read_data(snap, log);
#endif

 return retval;
}

void tcp_stats_close_log(tcp_stat_log *log) {
#if USE_WEB100
  web100_log_close_write(log);
#else
  estats_record_close(log);
#endif
  return;
}

void tcp_stats_write_snapshot(tcp_stat_log *log, tcp_stat_snap *snap) {
#if USE_WEB100
  web100_log_write(log, snap);
#elif USE_WEB10G
  estats_record_write_data(log, snap);
#endif
 return;
}

void tcp_stats_set_cwnd_limit(tcp_stat_agent *agent, tcp_stat_connection conn, tcp_stat_group* group, uint32_t limit) {
    uint32_t limrwin_val;

#if USE_WEB100
  web100_var *LimRwin, *yar;
  char yuff[24];

  web100_agent_find_var_and_group(agent, "CurMSS", &group, &yar);
  web100_raw_read(yar, conn, yuff);
  limrwin_val = limit
       * (atoi(
           web100_value_to_text(web100_get_var_type(yar),
                                yuff)));
  web100_agent_find_var_and_group(agent, "LimRwin", &group,
                                   &LimRwin);
  web100_raw_write(LimRwin, conn, &limrwin_val);
#elif USE_WEB10G
  struct estats_val yar;
  web10g_get_val(agent, conn, "CurMSS", &yar);
  limrwin_val = limit * yar.uv32;
  estats_write_var("LimRwin", limrwin_val, conn, agent);
#endif
  return;
}
