/*
 * This file contains the functions needed to read the web100
 * variables.  It links into the main web100srv program.
 *
 * Richard Carlson 3/10/04
 * rcarlson@interent2.edu
 *
 */

/* local include file contains needed structures */
#include "web100srv.h"

/*set up the necessary structures for monitoring connections at the
    beginning
*/
int web100_init(char *VarFileName, int debug)  {

  int i, j;
  FILE *fp;
  char line[256];
  int count_vars = 0;

  if((fp = fopen(VarFileName, "r")) == NULL) {
    err_sys("Required Web100 variables file missing, use -f option to specify file location\n");
    exit(5);
  }

  /* while((fscanf(fp, "%s", web_vars[count_vars].name))!= EOF) { */
  while (fgets(line, 255, fp) != NULL) {
    if ((line[0] == '#') || (line[0] == '\n'))
      continue;
    strncpy(web_vars[count_vars].name, line, (strlen(line)-1));
    count_vars++;
  }
  fclose(fp);
  if (debug > 0)
      fprintf(stderr, "web100_init() read %d variables from file\n", count_vars);

  return(count_vars);
}

web100_connection* local_find_connection(int sock, web100_agent* agent, int debug) {

  web100_connection* cn;
  struct web100_connection_spec spec;
  struct sockaddr_in localaddr, peeraddr;
  int addrlen=sizeof(localaddr), lport, rport;
  char buf[32], line[256];

  /* printf("web100_middlebox(): getting socket info\n"); */
  getsockname(sock, (struct sockaddr *)&localaddr, &addrlen);
  getpeername(sock, (struct sockaddr *)&peeraddr, &addrlen);
  spec.src_addr = localaddr.sin_addr.s_addr;
  spec.dst_addr = peeraddr.sin_addr.s_addr;
  spec.src_port = ntohs(localaddr.sin_port);
  spec.dst_port = ntohs(peeraddr.sin_port);
  if (debug > 1) {
    fprintf(stderr, "socket info src=%s:%d, ", inet_ntoa(localaddr.sin_addr), spec.src_port);
    fprintf(stderr, "dst=%s:%d\n", inet_ntoa(peeraddr.sin_addr), spec.dst_port);
  }

  if ((cn = web100_connection_find(agent, &spec)) == NULL) {
    if (debug > 1) {
      fprintf(stderr, "Failed to find connection for src=%s:%d, ", inet_ntoa(localaddr.sin_addr),
        ntohs(localaddr.sin_port));
      fprintf(stderr, "dst=%s:%d\n", inet_ntoa(peeraddr.sin_addr), ntohs(peeraddr.sin_port));
    }
    return(NULL);
  }

  return(cn);
}

void web100_middlebox(int sock, web100_agent* agent, int debug) {

  web100_var* var;
  web100_connection* cn;
  web100_group* group;
  char buf[32], line[256];
  int i;
  static char vars[][255] = {
        "LocalAddress",
        "RemAddress",
        "CurMSS",
        "WinScaleSent",
        "WinScaleRecv",
  };

  cn = local_find_connection(sock, agent, debug);

  for (i=0; i<5; i++) {
    web100_agent_find_var_and_group(agent, vars[i], &group, &var);
    web100_raw_read(var, cn, buf);
    sprintf(line, "%s;", web100_value_to_text(web100_get_var_type(var), buf));
    if (strcmp(line, "4294967295;") == 0)
      sprintf(line, "%d;", -1);
    write(sock, line, strlen(line));
    if (debug > 2) 
      fprintf(stderr, "%s",  line);
  }

  if (debug > 2)
    fprintf(stderr, "\n");

}
 
void web100_get_data_recv(int sock, web100_agent* agent, char *LogFileName, int count_vars, int debug) {

  int i, j, ok;
  web100_var* var;
  web100_connection* cn;
  char buf[32], line[256], *ctime();
  web100_group* group;
  FILE *fp;

  cn = local_find_connection(sock, agent, debug);

  /* printf("Web100_get_data_recv(): going to get %d variables\n", count_vars); */
  /* sprintf(line, "RemoteIPaddress: %s\n", inet_ntoa(peeraddr.sin_addr));
   * write(ctlsock, line, strlen(line));
   */
  i=time(0);
  fp=fopen(LogFileName,"a");
  fprintf(fp,"%15.15s;", ctime(&i)+4);
  web100_agent_find_var_and_group(agent, "RemAddress", &group, &var);
  web100_raw_read(var, cn, buf);
  sprintf(line, "%s;", web100_value_to_text(web100_get_var_type(var), buf));
  /* fprintf(fp, "%s;", inet_ntoa(peeraddr.sin_addr)); */
  fprintf(fp,"%s",  line);

  ok = 1;
  for(i=0; i<count_vars; i++) {
    /* printf("looking up var %d (%s)\n", i, web_vars[i].name); */
    if ((web100_agent_find_var_and_group(agent, web_vars[i].name, &group, &var)) != WEB100_ERR_SUCCESS) {
	if (debug > 0)
            fprintf(stderr, "Variable %d (%s) not found in KIS\n", i, web_vars[i].name);
        ok = 0;
        continue;
    }

    if (cn == NULL) {
	fprintf(stderr, "Web100_get_data_recv() failed, return to testing routine\n");
	return;
    }

    if ((web100_raw_read(var, cn, buf)) != WEB100_ERR_SUCCESS) {
        if (debug > 1)
	    web100_perror("web100_raw_read");
        continue;
    }
    if (ok == 1) {
      sprintf(web_vars[i].value, "%s", web100_value_to_text(web100_get_var_type(var), buf));
      /* sprintf(line, "%s: %d\n", web_vars[i].name, atoi(web_vars[i].value));
       * write(ctlsock, line, strlen(line));
       */
      fprintf(fp, "%d;", (int32_t)atoi(web_vars[i].value));
      if (debug > 5) {
        sprintf(line, "%s: %d\n", web_vars[i].name, atoi(web_vars[i].value));
        fprintf(stderr, "%s", line); 
      }
    }
    ok = 1;
  }
  fprintf(fp, "\n");
  fclose(fp);

}

int web100_get_data(int sock, int ctlsock, web100_agent* agent, int count_vars, int debug) {

  int i, j;
  u_int16_t k;
  web100_var* var;
  web100_connection* cn;
  char buf[32], line[256];
  web100_group* group;

  cn = local_find_connection(sock, agent, debug);

  /* printf("Web100_get_data(): going to get %d variables\n", count_vars);
   * web100_agent_find_var_and_group(agent, "RemAddress", &group, &var);
   * web100_raw_read(var, cn, buf);
   * sprintf(line, "RemoteIPaddress: %s\n", web100_value_to_text(web100_get_var_type(var), buf));
   * write(ctlsock, line, strlen(line));
   */

  for(i=0; i<count_vars; i++) {
    /* printf("looking up var %d (%s)\n", i, web_vars[i].name); */
    if ((web100_agent_find_var_and_group(agent, web_vars[i].name, &group, &var)) != WEB100_ERR_SUCCESS) {
	if (debug > 0)
            fprintf(stderr, "Variable %d (%s) not found in KIS\n", i, web_vars[i].name);
        continue;
    }

    if (cn == NULL) {
	fprintf(stderr, "Web100_get_data() failed, return to testing routine\n");
	return(-1);
    }

    if ((web100_raw_read(var, cn, buf)) != WEB100_ERR_SUCCESS) {
        if (debug > 1)
	    web100_perror("web100_raw_read");
        continue;
    }
        sprintf(web_vars[i].value, "%s", web100_value_to_text(web100_get_var_type(var), buf));
        sprintf(line, "%s: %d\n", web_vars[i].name, atoi(web_vars[i].value));
    if (ctlsock > 0) {
        write(ctlsock, line, strlen(line));
    }

    if (debug > 5)
      printf("%s", line);
  }
  return(0);

}

int web100_logvars(int *Timeouts, int *SumRTT, int *CountRTT, 
  int *PktsRetrans, int *FastRetran, int *DataPktsOut, int *AckPktsOut,
  int *CurrentMSS, int *DupAcksIn, int *AckPktsIn, int *MaxRwinRcvd,
  int *Sndbuf, int *CurrentCwnd, int *SndLimTimeRwin, int *SndLimTimeCwnd,
  int *SndLimTimeSender, int *DataBytesOut, int *SndLimTransRwin,
  int *SndLimTransCwnd, int *SndLimTransSender, int *MaxSsthresh,
  int *CurrentRTO, int *CurrentRwinRcvd, int *MaxCwnd, int *CongestionSignals,
  int *PktsOut, int *MinRTT, int count_vars, int *RcvWinScale) {

  int i;
    
  /* printf("web100_logvars(): lookking for %d variables\n", count_vars); */

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
  }

  return(0);
}
