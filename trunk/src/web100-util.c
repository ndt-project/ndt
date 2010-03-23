/*
 * This file contains the functions needed to read the web100
 * variables.  It links into the main web100srv program.
 *
 * Richard Carlson 3/10/04
 * rcarlson@interent2.edu
 */

#include <ctype.h>
#include <time.h>
#include <assert.h>

#include "web100srv.h"
#include "network.h"
#include "logging.h"
#include "utils.h"
#include "protocol.h"

/*
 * set up the necessary structures for monitoring connections at the
 * beginning
 */
int
web100_init(char *VarFileName)
{

  FILE *fp;
  char line[256];
  int count_vars = 0;

  assert(VarFileName);

  if((fp = fopen(VarFileName, "r")) == NULL) {
    err_sys("Required Web100 variables file missing, use -f option to specify file location\n");
    exit(5);
  }

  while (fgets(line, 255, fp) != NULL) {
    if ((line[0] == '#') || (line[0] == '\n'))
      continue;
    strncpy(web_vars[count_vars].name, line, (strlen(line)-1));
    count_vars++;
  }
  fclose(fp);
  log_println(1, "web100_init() read %d variables from file", count_vars);

  return(count_vars);
}

void
web100_middlebox(int sock, web100_agent* agent, web100_connection* cn, char *results)
{

  web100_var* var;
  web100_group* group;
  web100_snapshot* snap;
  char buff[8192], line[256];
  char* sndbuff;
  int i, j, k, octets=0;
  int SndMax=0, SndUna=0;
  fd_set wfd;
  struct timeval sel_tv;
  int ret;
  char tmpstr[200];
  size_t tmpstrlen = sizeof(tmpstr);
  I2Addr addr = NULL;
  web100_var *LimCwnd;
  u_int32_t limcwnd_val;

  static char vars[][255] = {
    "CurMSS",
    "WinScaleSent",
    "WinScaleRecv",
  };

  assert(results);

  log_println(4, "Looking for Web100 data on socketid %d", sock);
  
  addr = I2AddrByLocalSockFD(get_errhandle(), sock, False);
  memset(tmpstr, 0, 200);
  I2AddrNodeName(addr, tmpstr, &tmpstrlen);
  sprintf(line, "%s;", tmpstr);
  memset(tmpstr, 0, 200);
  I2AddrServName(addr, tmpstr, &tmpstrlen);
  log_print(3, "Server: %s%s ",  line, tmpstr);
  memcpy(meta.server_ip, line, strlen(line));
  meta.server_ip[(strlen(line)-1)] = 0;
  strncat(results, line, strlen(line));
  I2AddrFree(addr);
  tmpstrlen = sizeof(tmpstr);
  addr = I2AddrBySockFD(get_errhandle(), sock, False);
  memset(tmpstr, 0, 200);
  I2AddrNodeName(addr, tmpstr, &tmpstrlen);
  sprintf(line, "%s;", tmpstr);
  I2AddrServName(addr, tmpstr, &tmpstrlen);
  log_print(3, "Client: %s%s ",  line, tmpstr);
  strncat(results, line, strlen(line));
  I2AddrFree(addr);

  for (i=0; i<3; i++) {
    web100_agent_find_var_and_group(agent, vars[i], &group, &var);
    web100_raw_read(var, cn, buff);
    if (strcmp(vars[i], "CurMSS") == 0)
      octets = atoi(web100_value_to_text(web100_get_var_type(var), buff));
    sprintf(line, "%s;", web100_value_to_text(web100_get_var_type(var), buff));
    if (strcmp(line, "4294967295;") == 0)
      sprintf(line, "%d;", -1);
    strcat(results, line);
    log_print(3, "%s",  line);
  }
  log_println(3, "");
  log_println(0, "Sending %d Byte packets over the network", octets);

  /* The initial check has been completed, now stream data to the remote client
   * for 5 seconds with very limited buffer space.  The idea is to see if there
   * is a difference between this test and the later s2c speed test.  If so, then
   * it may be due to a duplex mismatch condition.
   * RAC 2/28/06
   */

  web100_agent_find_var_and_group(agent, "LimCwnd", &group, &LimCwnd);
  limcwnd_val = 2 * octets;
  web100_raw_write(LimCwnd, cn, &limcwnd_val);
  log_println(5, "Setting Cwnd Limit to %d octets", limcwnd_val);

  sndbuff = malloc(octets);
  if (sndbuff == NULL) {
    log_println(0, "Failed to allocate memory --> switching to 8192 Byte packets");
    sndbuff = buff;
    octets = 8192;
  }
  if (getuid() == 0) {
    system("echo 1 > /proc/sys/net/ipv4/route/flush");
  }
  k=0;
  for (j=0; j<octets; j++) {
    while (!isprint(k & 0x7f))
      k++;
    sndbuff[j] = (k++ & 0x7f);
  }

  group = web100_group_find(agent, "read");
  snap = web100_snapshot_alloc(group, cn);

  FD_ZERO(&wfd);
  FD_SET(sock, &wfd);
  sel_tv.tv_sec = 5;
  sel_tv.tv_usec = 0;
  while ((ret = select(sock+1, NULL, &wfd, NULL, &sel_tv)) > 0) {
    if ((ret == -1) && (errno == EINTR))  /* a signal arrived, ignore it */
	continue;

    web100_snap(snap);
    web100_agent_find_var_and_group(agent, "SndNxt", &group, &var);
    web100_snap_read(var, snap, line);
    SndMax = atoi(web100_value_to_text(web100_get_var_type(var), line));
    web100_agent_find_var_and_group(agent, "SndUna", &group, &var);
    web100_snap_read(var, snap, line);
    SndUna = atoi(web100_value_to_text(web100_get_var_type(var), line));
    if ((octets<<4) < (SndMax - SndUna - 1)) {
      continue;
    }

    k = write(sock, sndbuff, octets);
    if ((k == -1) && (errno == EINTR))
      continue;
    if (k < 0 )
      break;
  }
  log_println(5, "Finished with web100_middlebox() routine snap-0x%x, sndbuff=%x0x", snap, sndbuff);
  web100_snapshot_free(snap);
  /* free(sndbuff); */
}
 
void
web100_get_data_recv(int sock, web100_agent* agent, web100_connection* cn, int count_vars)
{
  int i, ok;
  web100_var* var;
  char buf[32], line[256], *ctime();
  web100_group* group;
  FILE *fp;
  time_t tt;

  tt=time(0);
  fp=fopen(get_logfile(),"a");
  if (fp)
    fprintf(fp,"%15.15s;", ctime(&tt)+4);
  web100_agent_find_var_and_group(agent, "RemAddress", &group, &var);
  web100_raw_read(var, cn, buf);
  sprintf(line, "%s;", web100_value_to_text(web100_get_var_type(var), buf));
  if (fp)
    fprintf(fp,"%s",  line);

  ok = 1;
  for(i=0; i<count_vars; i++) {
    if ((web100_agent_find_var_and_group(agent, web_vars[i].name, &group, &var)) != WEB100_ERR_SUCCESS) {
      log_println(1, "Variable %d (%s) not found in KIS", i, web_vars[i].name);
      ok = 0;
      continue;
    }

    if (cn == NULL) {
      fprintf(stderr, "Web100_get_data_recv() failed, return to testing routine\n");
      return;
    }

    if ((web100_raw_read(var, cn, buf)) != WEB100_ERR_SUCCESS) {
      if (get_debuglvl() > 9)
        web100_perror("web100_raw_read()");
      continue;
    }
    if (ok == 1) {
      sprintf(web_vars[i].value, "%s", web100_value_to_text(web100_get_var_type(var), buf));
      if (fp)
        fprintf(fp, "%d;", (int32_t)atoi(web_vars[i].value));
      log_println(9, "%s: %d", web_vars[i].name, atoi(web_vars[i].value));
    }
    ok = 1;
  }
  if (fp) {
    fprintf(fp, "\n");
    fclose(fp);
  }

}

int
web100_get_data(web100_snapshot* snap, int ctlsock, web100_agent* agent, int count_vars)
{

  int i;
  web100_var* var;
  char buf[32], line[256];
  web100_group* group;

  assert(snap);
  assert(agent);

  for(i=0; i<count_vars; i++) {
    if ((web100_agent_find_var_and_group(agent, web_vars[i].name, &group, &var)) != WEB100_ERR_SUCCESS) {
      log_println(9, "Variable %d (%s) not found in KIS", i, web_vars[i].name);
      continue;
    }

    if (snap == NULL) {
      fprintf(stderr, "Web100_get_data() failed, return to testing routine\n");
      log_println(6, "Web100_get_data() failed, return to testing routine\n");
      return(-1);
    }

    if ((web100_snap_read(var, snap, buf)) != WEB100_ERR_SUCCESS) {
      if (get_debuglvl() > 9) {
        log_print(9, "Variable %d (%s): ", i, web_vars[i].name);
        web100_perror("web100_snap_read()");
      }
      continue;
    }
    sprintf(web_vars[i].value, "%s", web100_value_to_text(web100_get_var_type(var), buf));
    sprintf(line, "%s: %d\n", web_vars[i].name, atoi(web_vars[i].value));
    send_msg(ctlsock, TEST_MSG, line, strlen(line));
    log_print(9, "%s", line);
  }
  log_println(6, "S2C test - Send web100 data to client pid=%d", getpid());
  return(0);

}

int
web100_rtt(int sock, web100_agent* agent, web100_connection* cn)
{
  web100_var* var;
  char buf[32];
  web100_group* group;
  double count, sum;

  if (cn == NULL)
    return(-10);

  if ((web100_agent_find_var_and_group(agent, "CountRTT", &group, &var)) != WEB100_ERR_SUCCESS)
    return(-24);
  if ((web100_raw_read(var, cn, buf)) != WEB100_ERR_SUCCESS) {
    return(-25);
  }
  count = atoi(web100_value_to_text(web100_get_var_type(var), buf));

  if ((web100_agent_find_var_and_group(agent, "SumRTT", &group, &var)) != WEB100_ERR_SUCCESS)
    return(-24);
  if ((web100_raw_read(var, cn, buf)) != WEB100_ERR_SUCCESS) {
    return(-25);
  }
  sum = atoi(web100_value_to_text(web100_get_var_type(var), buf));
  return(sum/count);
}

int
web100_autotune(int sock, web100_agent* agent, web100_connection* cn)
{
  web100_var* var;
  char buf[32];
  web100_group* group;
  int i, j=0;

  if (cn == NULL)
    return(10);

  if ((web100_agent_find_var_and_group(agent, "X_SBufMode", &group, &var)) != WEB100_ERR_SUCCESS)
    return(22);
  if ((web100_raw_read(var, cn, buf)) != WEB100_ERR_SUCCESS) {
    log_println(4, "Web100_raw_read(X_SBufMode) failed with errorno=%d", errno);
    return(23);
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
  if ((web100_agent_find_var_and_group(agent, "X_RBufMode", &group, &var)) != WEB100_ERR_SUCCESS)
    return(22);
  if ((web100_raw_read(var, cn, buf)) != WEB100_ERR_SUCCESS) {
    log_println(4, "Web100_raw_read(X_RBufMode) failed with errorno=%d", errno);
    return(23);
  }
  i = atoi(web100_value_to_text(web100_get_var_type(var), buf));

  if (i == 0) 
    j |= 0x02;
  return(j);
}

int
web100_setbuff(int sock, web100_agent* agent, web100_connection* cn, int autotune)
{
  web100_var* var;
  char buf[32];
  web100_group* group;
  int buff;
  int sScale, rScale;

  if (cn == NULL)
    return(10);

  if ((web100_agent_find_var_and_group(agent, "SndWinScale", &group, &var)) != WEB100_ERR_SUCCESS)
    return(24);
  if ((web100_raw_read(var, cn, buf)) != WEB100_ERR_SUCCESS) {
    return(25);
  }
  sScale = atoi(web100_value_to_text(web100_get_var_type(var), buf));
  if (sScale > 15)
    sScale = 0;

  if ((web100_agent_find_var_and_group(agent, "RcvWinScale", &group, &var)) != WEB100_ERR_SUCCESS)
    return(34);
  if ((web100_raw_read(var, cn, buf)) != WEB100_ERR_SUCCESS) {
    return(35);
  }
  rScale = atoi(web100_value_to_text(web100_get_var_type(var), buf));
  if (rScale > 15)
    rScale = 0;

  if ((sScale > 0) && (rScale > 0)) {
    buff = (64 * 1024) << sScale;
    if (autotune & 0x01) {
      if ((web100_agent_find_var_and_group(agent, "LimCwnd", &group, &var)) != WEB100_ERR_SUCCESS)
        return(22);
      if ((web100_raw_write(var, cn, &buff)) != WEB100_ERR_SUCCESS) {
        log_println(4, "Web100_raw_write(LimCwnd) failed with errorno=%d", errno);
        return(23);
      }
    }
    buff = (64 * 1024) << rScale;
    if (autotune & 0x02) {
      if ((web100_agent_find_var_and_group(agent, "LimRwin", &group, &var)) != WEB100_ERR_SUCCESS)
        return(22);
      if ((web100_raw_write(var, cn, &buff)) != WEB100_ERR_SUCCESS) {
        log_println(4, "Web100_raw_write(LimCwnd) failed with errorno=%d", errno);
        return(23);
      }
    }
  }

  return(0);
}

int
web100_logvars(int *Timeouts, int *SumRTT, int *CountRTT, 
  int *PktsRetrans, int *FastRetran, int *DataPktsOut, int *AckPktsOut,
  int *CurrentMSS, int *DupAcksIn, int *AckPktsIn, int *MaxRwinRcvd,
  int *Sndbuf, int *CurrentCwnd, int *SndLimTimeRwin, int *SndLimTimeCwnd,
  int *SndLimTimeSender, int *DataBytesOut, int *SndLimTransRwin,
  int *SndLimTransCwnd, int *SndLimTransSender, int *MaxSsthresh,
  int *CurrentRTO, int *CurrentRwinRcvd, int *MaxCwnd, int *CongestionSignals,
  int *PktsOut, int *MinRTT, int count_vars, int *RcvWinScale, int *SndWinScale,
  int *CongAvoid, int *CongestionOverCount, int *MaxRTT, int *OtherReductions,
  int *CurTimeoutCount, int *AbruptTimeouts, int *SendStall, int *SlowStart,
  int *SubsequentTimeouts, int *ThruBytesAcked)
{
  int i;

  for (i=0; i<=count_vars; i++) {
    if (strcmp(web_vars[i].name, "Timeouts") == 0)
      *Timeouts = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "SumRTT") == 0)
      *SumRTT = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "CountRTT") == 0)
      *CountRTT = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "PktsRetrans") == 0)
      *PktsRetrans = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "FastRetran") == 0)
      *FastRetran = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "DataPktsOut") == 0)
      *DataPktsOut = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "AckPktsOut") == 0)
      *AckPktsOut = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "CurMSS") == 0)
      *CurrentMSS = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "DupAcksIn") == 0)
      *DupAcksIn = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "AckPktsIn") == 0)
      *AckPktsIn = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "MaxRwinRcvd") == 0)
      *MaxRwinRcvd = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "X_Sndbuf") == 0)
      *Sndbuf = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "CurCwnd") == 0)
      *CurrentCwnd = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "MaxCwnd") == 0)
      *MaxCwnd = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "SndLimTimeRwin") == 0)
      *SndLimTimeRwin = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "SndLimTimeCwnd") == 0)
      *SndLimTimeCwnd = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "SndLimTimeSender") == 0)
      *SndLimTimeSender = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "DataBytesOut") == 0)
      *DataBytesOut = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "SndLimTransRwin") == 0)
      *SndLimTransRwin = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "SndLimTransCwnd") == 0)
      *SndLimTransCwnd = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "SndLimTransSender") == 0)
      *SndLimTransSender = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "MaxSsthresh") == 0)
      *MaxSsthresh = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "CurRTO") == 0)
      *CurrentRTO = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "CurRwinRcvd") == 0)
      *CurrentRwinRcvd = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "CongestionSignals") == 0)
      *CongestionSignals = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "PktsOut") == 0)
      *PktsOut = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "MinRTT") == 0)
      *MinRTT = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "RcvWinScale") == 0)
      *RcvWinScale = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "SndWinScale") == 0)
      *SndWinScale = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "CongAvoid") == 0)
      *CongAvoid = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "CongestionOverCount") == 0)
      *CongestionOverCount = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "MaxRTT") == 0)
      *MaxRTT = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "OtherReductions") == 0)
      *OtherReductions = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "CurTimeoutCount") == 0)
      *CurTimeoutCount = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "AbruptTimeouts") == 0)
      *AbruptTimeouts = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "SendStall") == 0)
      *SendStall = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "SlowStart") == 0)
      *SlowStart = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "SubsequentTimeouts") == 0)
      *SubsequentTimeouts = atoi(web_vars[i].value);
    else if (strcmp(web_vars[i].name, "ThruBytesAcked") == 0)
      *ThruBytesAcked = atoi(web_vars[i].value);
  }

  return(0);
}


/* Routine to read snaplog file and determine the number of times the
 * congestion window is reduced.
 */

int
CwndDecrease(web100_agent* agent, char* logname, int *dec_cnt, int *same_cnt, int *inc_cnt)
{

  web100_var* var;
  char buff[256];
  web100_snapshot* snap;
  int s1, s2, cnt, rt;
  web100_log* log;
  web100_group* group;
  web100_agent* agnt;

  if ((log = web100_log_open_read(logname)) == NULL)
    return(0);
  if ((snap = web100_snapshot_alloc_from_log(log)) == NULL)
    return(-1);
  if ((agnt = web100_get_log_agent(log)) == NULL)
    return(-1);
  if ((group = web100_get_log_group(log)) == NULL)
    return(-1);
  if (web100_agent_find_var_and_group(agnt, "CurCwnd", &group, &var) != WEB100_ERR_SUCCESS)
    return(-1);
  s2 = 0;
  cnt = 0;
  while (web100_snap_from_log(snap, log) == 0) {
    if (cnt++ == 0)
      continue;
    s1 = s2;
    rt = web100_snap_read(var, snap, buff);
    s2 = atoi(web100_value_to_text(web100_get_var_type(var), buff));
    if (cnt < 20) {
      log_println(7, "Reading snaplog 0x%x (%d), var = %s", (int) snap, cnt, (char*) var);
      log_println(7, "Checking for Cwnd decreases. rt=%d, s1=%d, s2=%d (%s), dec-cnt=%d",
          rt, s1, s2, web100_value_to_text(web100_get_var_type(var), buff), *dec_cnt);
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
  log_println(2, "-=-=-=- CWND window report: increases = %d, decreases = %d, no change = %d", *inc_cnt, *dec_cnt, *same_cnt);
  return(0);
}

unsigned short
csum(unsigned short *buff, int nwords)
{

/* generate a TCP/IP checksum for our packet */

  register int sum = 0;
  u_short answer = 0;
  register u_short *w = buff;
  register int nleft = nwords;

  while (nleft > 1) {
    sum += *w++;
    nleft -= 2;
  }

  if (nleft == 1) {
    *(u_char *) (&answer) = *(u_char *)w;
    sum += answer;
  }

  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  answer = ~sum;
  return(answer);
}

int
KillHung(void)

{
  web100_agent *agent;
  web100_group *group;
  web100_var *state, *var;
  web100_connection *conn;
  int cid, one=1, hung=0;
  int sd;
  char *pkt, buff[64];
  struct in_addr src, dst;
  struct iphdr *ip = NULL;
  struct tcphdr *tcp;
  struct sockaddr_in sin;
  struct pseudo_hdr *phdr;

  if ((agent = web100_attach(WEB100_AGENT_TYPE_LOCAL, NULL)) == NULL) {
    web100_perror("web100_attach");
    return(-1);
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
      phdr = (struct pseudo_hdr *) (pkt + sizeof(struct iphdr) - sizeof(struct pseudo_hdr));


  /* Build the TCP packet first, along with the pseudo header, the later
   * the IP header build process will overwrite the pseudo header fields
   */

      web100_agent_find_var_and_group(agent, "LocalAddress", &group, &var);
      web100_raw_read(var, conn, buff);
	log_println(3, "LocalAddress: '%s'", web100_value_to_text(web100_get_var_type(var), buff));
      dst.s_addr = inet_addr(web100_value_to_text(web100_get_var_type(var), buff)); 
      web100_agent_find_var_and_group(agent, "RemAddress", &group, &var);
      web100_raw_read(var, conn, buff);
      src.s_addr = inet_addr(web100_value_to_text(web100_get_var_type(var), buff)); 
	log_println(3, "RemAddress: '%s'", web100_value_to_text(web100_get_var_type(var), buff));

      phdr->protocol = IPPROTO_TCP;
      phdr->len = htons(sizeof(struct tcphdr)/4);
      phdr->s_addr = src.s_addr;
      phdr->d_addr = dst.s_addr;

      web100_agent_find_var_and_group(agent, "LocalPort", &group, &var);
      web100_raw_read(var, conn, buff);
      tcp->dest = htons(atoi(web100_value_to_text(web100_get_var_type(var), buff))); 
	log_println(3, "LocalPort: '%s'", web100_value_to_text(web100_get_var_type(var), buff));
      web100_agent_find_var_and_group(agent, "RemPort", &group, &var);
      web100_raw_read(var, conn, buff);
      tcp->source = htons(atoi(web100_value_to_text(web100_get_var_type(var), buff))); 
	log_println(3, "RemPort: '%s'", web100_value_to_text(web100_get_var_type(var), buff));
      sin.sin_port = tcp->dest;

      web100_agent_find_var_and_group(agent, "RcvNxt", &group, &var);
      web100_raw_read(var, conn, buff);
      tcp->seq = htonl(atoll(web100_value_to_text(web100_get_var_type(var), buff)) ); 
	log_println(3, "Seq No. (RcvNxt): '%s'", web100_value_to_text(web100_get_var_type(var), buff));
      web100_agent_find_var_and_group(agent, "SndUna", &group, &var);
      web100_raw_read(var, conn, buff);
      tcp->ack_seq = htonl(atoll(web100_value_to_text(web100_get_var_type(var), buff))); 
	log_println(3, "Ack No. (SndNxt): '%s'", web100_value_to_text(web100_get_var_type(var), buff));

      tcp->window = 0x7fff;
      tcp->res1 = 0;
      tcp->doff = sizeof(struct tcphdr)/4;
      /* tcp->syn = 1; */
      tcp->rst = 1;
      tcp->ack = 1;
      /* tcp->fin = 1; */
      tcp->urg_ptr = 0;
      tcp->check = csum((unsigned short *)phdr, sizeof(struct tcphdr) + sizeof(struct pseudo_hdr));

      bzero(pkt, sizeof(struct iphdr));
      ip->ihl = 5;
      ip->version = 4;
      ip->tos = 0;
      ip->tot_len = sizeof (struct iphdr) + sizeof (struct tcphdr);
      ip->id = htons(31890);
      ip->frag_off = htons(IP_DF);
      ip->ttl = IPDEFTTL;
      ip->protocol = IPPROTO_TCP;		/* TCP packet */
      ip->saddr = src.s_addr;
      ip->daddr = dst.s_addr;
      sin.sin_addr.s_addr = dst.s_addr;

      ip->check = csum((unsigned short *) pkt, sizeof(struct iphdr));

      if (setsockopt(sd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) 
	return(-1);

      sendto(sd, (char *)pkt, 40, 0, (struct sockaddr *) &sin, sizeof(sin));
	
      hung = 1;

    }
    conn = web100_connection_next(conn);
  }

  web100_detach(agent);
  /* free(pkt); */
  if (hung == 0)
    return(-1);
  return(0);
}
