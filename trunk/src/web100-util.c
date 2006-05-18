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

web100_connection* local_find_connection(int sock, web100_agent* agent, int family, int debug) {

  	web100_connection* cn;
  	struct web100_connection_spec spec;
	struct web100_connection_spec_v6 spec6;
  	struct sockaddr_in localaddr, peeraddr;
  	struct sockaddr_in6 local6addr, peer6addr;
  	int addrlen=sizeof(localaddr), lport, rport, addr6len=sizeof(local6addr);
  	char buf[32], line[256];

	if (family == AF_INET) {
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
	}
	if (family == AF_INET6) {
	    getsockname(sock, (struct sockaddr*)&local6addr, &addr6len);
	    getpeername(sock, (struct sockaddr*)&peer6addr, &addr6len);
	    memcpy(&spec6.src_addr, &local6addr.sin6_addr, 16);
	    spec6.src_port = ntohs(local6addr.sin6_port);
	    memcpy(&spec6.dst_addr, &peer6addr.sin6_addr, 16);
	    spec6.dst_port = ntohs(peer6addr.sin6_port);
	    if ((cn = web100_connection_find_v6(agent, &spec6)) == NULL) {
		if (debug > 1) {
		    fprintf(stderr, "IPv6 connection failed! Src: addr=%x, port=%d\n",
				local6addr.sin6_addr.s6_addr, ntohs(local6addr.sin6_port));
		    fprintf(stderr, "IPv6 connection failed! Dst: addr=%x, port=%d\n",
				peer6addr.sin6_addr.s6_addr, ntohs(peer6addr.sin6_port));
		}
		return(NULL);
	    }
	}

  	return(cn);
}

void web100_middlebox(int sock, web100_agent* agent, char *results, int family, int debug) {

 	web100_var* var;
 	web100_connection* cn;
 	web100_group* group;
	web100_snapshot* snap;
 	char buff[8192], line[256];
 	int i, j, k, octets=0;
	int SndMax=0, SndUna=0;
	fd_set wfd;
	struct timeval sel_tv;
	int ret;

	double s, t, secs();
 	static char vars[][255] = {
            "LocalAddress",
            "RemAddress",
            "CurMSS",
            "WinScaleSent",
            "WinScaleRecv",
 	};

 	cn = local_find_connection(sock, agent, family, debug);
 	/* cn = web100_connection_from_socket(agent, sock); */
 	if (cn == NULL) {
	    fprintf(stderr, "!!!!!!!!!!!  web100_middlebox() failed to get web100 connection data, rc=%d\n", errno);
	    exit(-1);
 	}

 	for (i=0; i<5; i++) {
	    web100_agent_find_var_and_group(agent, vars[i], &group, &var);
	    web100_raw_read(var, cn, buff);
	    if (strcmp(vars[i], "CurMSS") == 0)
	        octets = atoi(web100_value_to_text(web100_get_var_type(var), buff));
	    sprintf(line, "%s;", web100_value_to_text(web100_get_var_type(var), buff));
	    if (strcmp(line, "4294967295;") == 0)
	        sprintf(line, "%d;", -1);
	    strcat(results, line);
	    /* write(sock, line, strlen(line)); */
	    if (debug > 2) 
	        fprintf(stderr, "%s",  line);
 	}

	 if (debug > 2)
	     fprintf(stderr, "\n");
	printf("Sending %d Byte packets over the network\n", octets);

/* The initial check has been completed, now stream data to the remote client
 * for 5 seconds with very limited buffer space.  The idea is to see if there
 * is a difference between this test and the later s2c speed test.  If so, then
 * it may be due to a duplex mismatch condition.
 * RAC 2/28/06
 */

        web100_var *LimCwnd;
        u_int32_t limcwnd_val;

	if (debug > 4)
            fprintf(stderr, "Setting Cwnd Limit");
        web100_agent_find_var_and_group(agent, "LimCwnd", &group, &LimCwnd);
        limcwnd_val = 2 * octets;
        web100_raw_write(LimCwnd, cn, &limcwnd_val);
	if (debug > 4)
	    fprintf(stderr, " to %d octets\n", limcwnd_val);

        if (getuid() == 0) {
            system("echo 1 > /proc/sys/net/ipv4/route/flush");
        }
	printf("Sending %d Byte packets over the network\n", octets);
	k=0;
        for (j=0; j<=octets; j++) {
            while (!isprint(k & 0x7f))
                k++;
            buff[j] = (k++ & 0x7f);
        }
	
	group = web100_group_find(agent, "read");
	snap = web100_snapshot_alloc(group, cn);

	/* s = secs() + 3.0;
         * while(secs() < s) {
	 */
	FD_ZERO(&wfd);
	FD_SET(sock, &wfd);
	sel_tv.tv_sec = 5;
	sel_tv.tv_usec = 0;
	while ((ret = select(sock+1, NULL, &wfd, NULL, &sel_tv)) > 0) {
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

            k = write(sock, buff, octets);
            if (k < 0 )
                break;
	}
	web100_snapshot_free(snap);
}
 
void web100_get_data_recv(int sock, web100_agent* agent, char *LogFileName, int count_vars, int family, int debug) {

  int i, j, ok;
  web100_var* var;
  web100_connection* cn;
  char buf[32], line[256], *ctime();
  web100_group* group;
  FILE *fp;
  time_t tt;

  cn = local_find_connection(sock, agent, family, debug);
  /* cn = web100_connection_from_socket(agent, sock); */

  /* printf("Web100_get_data_recv(): going to get %d variables\n", count_vars); */
  /* sprintf(line, "RemoteIPaddress: %s\n", inet_ntoa(peeraddr.sin_addr));
   * write(ctlsock, line, strlen(line));
   */
  tt=time(0);
  fp=fopen(LogFileName,"a");
  fprintf(fp,"%15.15s;", ctime(&tt)+4);
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
	    web100_perror("web100_raw_read()");
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

/* int web100_get_data(int sock, int ctlsock, web100_agent* agent, int count_vars, int family, int debug) { */
int web100_get_data(web100_snapshot* snap, int ctlsock, web100_agent* agent, int count_vars, int family, int debug) {

  int i, j;
  u_int16_t k;
  web100_var* var;
  web100_connection* cn;
  char buf[32], line[256];
  web100_group* group;

  /* cn = local_find_connection(sock, agent, family, debug); */
  /* cn = web100_connection_from_socket(agent, sock); */

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

    /* if (cn == NULL) { */
    if (snap == NULL) {
	fprintf(stderr, "Web100_get_data() failed, return to testing routine\n");
	return(-1);
    }

    /* if ((web100_raw_read(var, cn, buf)) != WEB100_ERR_SUCCESS) { */
    if ((web100_snap_read(var, snap, buf)) != WEB100_ERR_SUCCESS) {
        if (debug > 4)
	    web100_perror("web100_snap_read()");
        continue;
    }
    sprintf(web_vars[i].value, "%s", web100_value_to_text(web100_get_var_type(var), buf));
    sprintf(line, "%s: %d\n", web_vars[i].name, atoi(web_vars[i].value));
    write(ctlsock, line, strlen(line));
    if (debug > 5)
      printf("%s", line);
  }
  return(0);

}

int web100_rtt(int sock, web100_agent* agent, int family, int debug) {

	web100_var* var;
	web100_connection* cn;
	char buf[32], line[64];
	web100_group* group;
	double count, sum;

	cn = local_find_connection(sock, agent, family, debug);
	/* cn = web100_connection_from_socket(agent, sock); */
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

int web100_autotune(int sock, web100_agent* agent, int family, int debug) {

	web100_var* var;
	web100_connection* cn;
	char buf[32], line[64];
	web100_group* group;
	int i, j=0;

	cn = local_find_connection(sock, agent, family, debug);
	/* cn = web100_connection_from_socket(agent, sock); */
	if (cn == NULL)
	    return(10);

	if ((web100_agent_find_var_and_group(agent, "X_SBufMode", &group, &var)) != WEB100_ERR_SUCCESS)
	    return(22);
	if ((web100_raw_read(var, cn, buf)) != WEB100_ERR_SUCCESS) {
	    if (debug > 3)
		fprintf(stderr, "Web100_raw_read(X_SBufMode) failed with errorno=%d\n", errno);
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
	    if (debug > 3)
		fprintf(stderr, "Web100_raw_read(X_RBufMode) failed with errorno=%d\n", errno);
	    return(23);
	}
	i = atoi(web100_value_to_text(web100_get_var_type(var), buf));

	if (i == 0) 
	    j |= 0x02;
	return(j);
}

int web100_setbuff(int sock, web100_agent* agent, int autotune, int family, int debug) {

	web100_var* var;
	web100_connection* cn;
	char buf[32];
	web100_group* group;
	int buff;
	int sScale, rScale;

	cn = local_find_connection(sock, agent, family, debug);
	/* cn = web100_connection_from_socket(agent, sock); */
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
	            if (debug > 3)
		        fprintf(stderr, "Web100_raw_write(LimCwnd) failed with errorno=%d\n", errno);
	            return(23);
	        }
	    }
	    buff = (64 * 1024) << rScale;
	    if (autotune & 0x02) {
	        if ((web100_agent_find_var_and_group(agent, "LimRwin", &group, &var)) != WEB100_ERR_SUCCESS)
	            return(22);
	        if ((web100_raw_write(var, cn, &buff)) != WEB100_ERR_SUCCESS) {
	            if (debug > 3)
		        fprintf(stderr, "Web100_raw_write(LimCwnd) failed with errorno=%d\n", errno);
	            return(23);
	        }
	    }
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
  int *PktsOut, int *MinRTT, int count_vars, int *RcvWinScale, int *SndWinScale,
  int *CongAvoid, int *CongestionOverCount, int *MaxRTT, int *OtherReductions,
  int *CurTimeoutCount, int *AbruptTimeouts, int *SendStall, int *SlowStart,
  int *SubsequentTimeouts, int *ThruBytesAcked) {

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

int CwndDecrease(web100_agent* agent, char* logname, int *dec_cnt,
		int *same_cnt, int *inc_cnt, int debug) {

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
	/* 
	ec_cnt = 0;
	inc_cnt = 0;
	same_cnt = 0;
	*/
	cnt = 0;
	while (web100_snap_from_log(snap, log) == 0) {
		if (cnt++ == 0)
			continue;
		s1 = s2;
		rt = web100_snap_read(var, snap, buff);
		s2 = atoi(web100_value_to_text(web100_get_var_type(var), buff));
		if ((debug > 6) && (cnt < 20)) {
			fprintf(stderr, "Reading snaplog 0x%x (%d), var = %s\n", snap, cnt, var);
			fprintf(stderr, "Checking for Cwnd decreases. rt=%d, s1=%d, s2=%d (%s), dec-cnt=%d\n", 
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
	if (debug > 1)
		fprintf(stderr, "-=-=-=- CWND window report: increases = %d, decreases = %d, no change = %d\n",
			*inc_cnt, *dec_cnt, *same_cnt);
	return(0);
}
