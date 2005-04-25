#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<sys/time.h>
#include	<netinet/in.h>
#include	<netinet/tcp.h>
#include	<arpa/inet.h>
#include	<unistd.h>
#include	<sys/errno.h>
#include	<string.h>
#include	<stdlib.h>
#include	<signal.h>
#include	<sys/timeb.h>
#include	<netdb.h>

extern int h_errno;

int Randomize, failed, cancopy;
int ECNEnabled, NagleEnabled, MSSSent, MSSRcvd;
int SACKEnabled, TimestampsEnabled, WinScaleRcvd, WinScaleSent;
int FastRetran, AckPktsOut, SmoothedRTT, CurrentCwnd, MaxCwnd;
int SndLimTimeRwin, SndLimTimeCwnd, SndLimTimeSender;
int SndLimTransRwin, SndLimTransCwnd, SndLimTransSender, MaxSsthresh;
int SumRTT, CountRTT, CurrentMSS, Timeouts, PktsRetrans;
int SACKsRcvd, DupAcksIn, MaxRwinRcvd, MaxRwinSent;
int DataPktsOut, Rcvbuf, Sndbuf, AckPktsIn, DataBytesOut;
int PktsOut, CongestionSignals, RcvWinScale;
int pkts, lth=8192, CurrentRTO;
int c2sData, c2sAck, s2cData, s2cAck;
int winssent, winsrecv, msglvl=0, debug=0;
double spdin, spdout;
double aspd;

int half_duplex, mylink, congestion, bad_cable, mismatch;
double loss, estimate, avgrtt, spd, waitsec, timesec, rttsec;
double order, rwintime, sendtime, cwndtime, rwin, swin, cwin;

    
void printVariables(char *tmpstr) {

	int i, j, k;
	char sysvar[128], sysval[128];

	k = strlen(tmpstr) - 1;
	/* printf("Web100 variable list (%s)\n", tmpstr); */
	for (i=0; tmpstr[i]!='d'; i++);
	i++;
	for (;;) {
	    for (j=0; tmpstr[i]!=' '; j++)
		sysvar[j] = tmpstr[i++];
	    sysvar[j] = '\0';
	    i++;
	    for (j=0; tmpstr[i]!='\n'; j++)
		sysval[j] = tmpstr[i++];
	    sysval[j] = '\0';
	    i++;
	    printf("%s %s\n", sysvar, sysval);
	    if (i >= k)
		return;
	}
	
}
void testResults(char *tmpstr) {

	int i=0;
	int Zero=0, bwdelay, minwin;
	char *sysvar, *sysval;
	char scaled[16];
	float j;

	sysvar = strtok(tmpstr, "d");
	sysvar = strtok(NULL, " ");
	sysval = strtok(NULL, "\n");
	i = atoi(sysval);
	save_int_values(sysvar, i);
	/* printf("Stored %d [%s] in %s\n", i, sysval, sysvar); */

        for(;;) {
	    sysvar = strtok(NULL, " ");
	    if (sysvar == NULL)
		break;
	    sysval = strtok(NULL, "\n");
	    if (index(sysval, '.') == NULL) {
	        i = atoi(sysval);
	    	save_int_values(sysvar, i);
		/* printf("Stored %d [%s] in %s\n", i, sysval, sysvar); */
	    }
	    else {
		j = atof(sysval);
		save_dbl_values(sysvar, &j);
		/* printf("Stored %0.2f (%s) in %s\n", j, sysval, sysvar); */
	    }
	/* printf("Read '%s' %s\n", sysvar, sysval); */
        }

 	if(CountRTT > 0) {

	  if (c2sData < 3) {
	    if (c2sData < 0) {
	      printf("Server unable to determine bottleneck link type.\n");
	    } else {
	      printf("Your host is connected to a ");
	      if (c2sData == 1) {
		printf("Dial-up Modem\n");
		mylink = .064;
	      } else {
		printf("Cable/DSL modem\n");
		mylink = 2;
	      }
	    }
	  } else {
	    printf("The slowest link in the end-to-end path is a ");
	    if (c2sData == 3) {
	      printf("10 Mbps Ethernet or WiFi 11b subnet\n");
	      mylink = 10;
	    } else if (c2sData == 4) {
	      printf("45 Mbps T3/DS3 or WiFi 11 a/g  subnet\n");
	      mylink = 45;
	    } else if (c2sData == 5) {
	      printf("100 Mbps ");
	      mylink = 100;
		if (half_duplex == 0) {
		  printf("Full duplex Fast Ethernet subnet\n");
		} else {
		  printf("Half duplex Fast Ethernet subnet\n");
		}
	    } else if (c2sData == 6) {
	      printf("a 622 Mbps OC-12 subnet\n");
	      mylink = 622;
	    } else if (c2sData == 7) {
	      printf("1.0 Gbps Gigabit Ethernet subnet\n");
	      mylink = 1000;
	    } else if (c2sData == 8) {
	      printf("2.4 Gbps OC-48 subnet\n");
	      mylink = 2400;
	    } else if (c2sData == 9) {
	      printf("10 Gbps 10 Gigabit Ethernet/OC-192 subnet\n");
	      mylink = 10000;
	    }
	  }

	  /* if (mismatch == 1) {
	   *  printf("Alarm: Duplex mismatch condition exists: ");
	   *    if (aspd < 1) {
	   * 	printf("Host set to Full and Switch set to Half duplex\n");
	   *      }
	   *     else {
	   * 	printf("Host set to Half and Switch set to Full duplex\n");
	   *      }
	   * }
           * if (mismatch == 2) {
           *  printf("Alarm: Duplex Mismatch condition on switch-to-switch uplink! ");
           *  printf("Contact your network administrator.\n");
           * }
	   */
	  switch(mismatch) {
		case 1 : printf("Warning: Old Duplex-Mismatch condition detected.\n");
			 break;

		case 2 : printf("Alarm: Duplex Mismatch condition detected. Switch=Full and Host=Half\n");
			 break;
		case 4 : printf("Alarm: Possible Duplex Mismatch condition detected. Switch=Full and Host=Half\n");
			 break;

		case 3 : printf("Alarm: Duplex Mismatch condition detected. Switch=Half and Host=Full\n");
			 break;
		case 5 : printf("Alarm: Possible Duplex Mismatch condition detected. Switch=Half and Host=Full\n");
			 break;
	  }


	  if (mismatch == 0) {
	      if (bad_cable == 1) {
	          printf("Alarm: Excessive errors, check network cable(s).\n");
	      }
	      if (congestion == 1) {
	          printf("Information: Other network traffic is congesting the link\n");
	      }
	      if ((rwin*2/rttsec) < mylink) {
		  j = (float)((mylink * avgrtt)*1000) / 8;
		  if ((int)j < MaxRwinRcvd) {
		      printf("Information: The receive buffer should be %0.0f ", j);
		      printf("Kbytes to maximize throughput\n");
		  }
	      }
	  }

	if (msglvl > 0) {
	    printf("\n\t------  Web100 Detailed Analysis  ------\n");

	    printf("\nWeb100 reports the Round trip time = %0.2f msec;", avgrtt);

            printf("the Packet size = %d Bytes; and \n", CurrentMSS);
            if (PktsRetrans > 0) {
                printf("There were %d packets retransmitted", PktsRetrans);
                printf(", %d duplicate acks received", DupAcksIn);
                printf(", and %d SACK blocks received\n", SACKsRcvd);
	        if (order > 0)
                    printf("Packets arrived out-of-order %0.2f%% of the time.\n", order*100);
                if (Timeouts > 0)
                    printf("The connection stalled %d times due to packet loss.\n", Timeouts);
	        if (waitsec > 0)
	            printf("The connection was idle %0.2f seconds (%0.2f%%) of the time.\n",
			 waitsec, (100*waitsec/timesec));
            } else if (DupAcksIn > 0) {
                printf("No packet loss - ");
	        if (order > 0)
                    printf("but packets arrived out-of-order %0.2f%% of the time.\n", order*100);
	        else
		    printf("\n");
            } else {
                printf("No packet loss was observed.\n");
            }

            if (rwintime > .015) {
                printf("This connection is receiver limited %0.2f%% of the time.\n", rwintime*100);
	        if ((2*(rwin/rttsec)) < mylink)
                    printf("  Increasing the current receive buffer (%0.2f KB) will improve performance\n",
			      (float) MaxRwinRcvd/1024);
            }
            if (sendtime > .015) {
                printf("This connection is sender limited %0.2f%% of the time.\n", sendtime*100);
	        if ((2*(swin/rttsec)) < mylink)
                    printf("  Increasing the current send buffer (%0.2f KB) will improve performance\n",
			      (float) Sndbuf/1024);
            }
            if (cwndtime > .005) {
                printf("This connection is network limited %0.2f%% of the time.\n", cwndtime*100);
                /* if (cwndtime > .15)
                 *    printf("  Contact your local network administrator to report a network problem\n");
                 * if (order > .15)
                 *    printf("  Contact your local network admin and report excessive packet reordering\n");
		 */
            }
	    if ((spd < 4) && (loss > .01)) {
                printf("Excessive packet loss is impacting your performance, check the ");
                printf("auto-negotiate function on your local PC and network switch\n");
            }
	    printf("\n    Web100 reports TCP negotiated the optional Performance Settings to: \n");
	    printf("RFC 2018 Selective Acknowledgment: ");
	    if(SACKEnabled == Zero)
	       printf("OFF\n");
	    else
	       printf("ON\n");
  
	    printf("RFC 896 Nagle Algorithm: ");
	    if(NagleEnabled == Zero)
	       printf("OFF\n");
	    else
	       printf("ON\n");

	    printf("RFC 3168 Explicit Congestion Notification: ");
	    if(ECNEnabled == Zero)
	       printf("OFF\n");
	    else
	       printf("ON\n");

	    printf("RFC 1323 Time Stamping: ");
	    if(TimestampsEnabled == 0)
	       printf("OFF\n");
	    else
	       printf("ON\n");
  
	    printf("RFC 1323 Window Scaling: ");
	    if((WinScaleRcvd == 0) || (WinScaleRcvd > 20))
	       printf("OFF\n");
	    else
	       printf("ON; Scaling Factors - Server=%d, Client=%d\n",
			WinScaleSent, WinScaleRcvd);

	    if ((RcvWinScale == 0)&& (Sndbuf > 65535))
		    Sndbuf = 65535;

            printf("The theoretical network limit is %0.2f Mbps\n", estimate);

            printf("The NDT server has a %0.0f KByte buffer which limits the throughput to %0.2f Mbps\n",
		    (float)Sndbuf/1024, (float)swin/rttsec);

            printf("Your PC/Workstation has a %0.0f KByte buffer which limits the throughput to %0.2f Mbps\n",
		    (float)MaxRwinRcvd/1024, (float)rwin/rttsec);

            printf("The network based flow control limits the throughput to %0.2f Mbps\n",
			    (float)cwin/rttsec);

	    printf("\nClient Data reports link is '%3d', Client Acks report link is '%3d'\n",
		c2sData, c2sAck);
	    printf("Server Data reports link is '%3d', Server Acks report link is '%3d'\n", 
		s2cData, s2cAck);
	  }
	}
}

/* this routine decodes the middlebox test results.  The data is returned
 * from the server is a specific order.  This routine pulls the string apart
 * and puts the values into the proper variable.  It then compares the values
 * to known values and writes out the specific results.
 *
 * server data is first
 * order is Server IP; Client IP; CurrentMSS; WinScaleRcvd; WinScaleSent;
 * Client then adds
 * Server IP; Client IP.
 */

void middleboxResults(char *tmpstr, uint32_t local_addr, uint32_t peer_addr) {

	char ssip[64], scip[64], *str;
	char csip[64], ccip[64];
	int mss;
	int k;

	str = strtok(tmpstr, ";");
	strcpy(ssip, str);
	str = strtok(NULL, ";");
	strcpy(scip, str);

	str = strtok(NULL, ";");
	mss = atoi(str);
	str = strtok(NULL, ";");
	winsrecv = atoi(str);
	str = strtok(NULL, ";");
	winssent = atoi(str);

	sprintf(ccip, "%u.%u.%u.%u", local_addr & 0xff,
		(local_addr >> 8) & 0xff, (local_addr >> 16) & 0xff, 
		(local_addr >> 24) & 0xff);
	sprintf(csip, "%u.%u.%u.%u", peer_addr  & 0xff,
		(peer_addr >> 8) & 0xff, (peer_addr >> 16) & 0xff, 
		(peer_addr >> 24) & 0xff);

	if (TimestampsEnabled == 1)
	    mss += 12;
	if (mss == 1456)
	     printf("Packet size is preserved End-to-End\n");
	else
	     printf("Information: Network Middlebox is modifying MSS variable (changed to %d)\n", mss);

	if (strcmp(ssip, csip) == 0)
	    printf("Server IP addresses are preserved End-to-End\n");
	else {
	    printf("Information: Network Address Translation (NAT) box is "); 
	    printf("modifying the Server's IP address\n");
	    printf("\tServer says [%s] but Client says [ %s]\n", ssip, csip);
	  }

	if (strcmp(scip, ccip) == 0)
	    printf("Client IP addresses are preserved End-to-End\n");
	else {
	    printf("Information: Network Address Translation (NAT) box is "); 
	    printf("modifying the Client's IP address\n");
	    printf("\tServer says [%s] but Client says [ %s]\n", scip, ccip);
	  }

}


save_int_values(char *sysvar, int sysval) {
/*  Values saved for interpretation:
 *	SumRTT 
 *	CountRTT
 *	CurrentMSS
 *	Timeouts
 *	PktsRetrans
 *	SACKsRcvd
 *	DupAcksIn
 *	MaxRwinRcvd
 *	MaxRwinSent
 *	Sndbuf
 *	Rcvbuf
 *	DataPktsOut
 *	SndLimTimeRwin
 *	SndLimTimeCwnd
 *	SndLimTimeSender
 */   
  
    if(strcmp("MSSSent:", sysvar) == 0) 
	MSSSent = sysval;
    else if(strcmp("MSSRcvd:", sysvar) == 0) 
	MSSRcvd = sysval;
    else if(strcmp("ECNEnabled:", sysvar) == 0) 
	ECNEnabled = sysval;
    else if(strcmp("NagleEnabled:", sysvar) == 0) 
	NagleEnabled = sysval;
    else if(strcmp("SACKEnabled:", sysvar) == 0) 
	SACKEnabled = sysval;
    else if(strcmp("TimestampsEnabled:", sysvar) == 0) 
	TimestampsEnabled = sysval;
    else if(strcmp("WinScaleRcvd:", sysvar) == 0) 
	WinScaleRcvd = sysval;
    else if(strcmp("WinScaleSent:", sysvar) == 0) 
	WinScaleSent = sysval;
    else if(strcmp("SumRTT:", sysvar) == 0) 
	SumRTT = sysval;
    else if(strcmp("CountRTT:", sysvar) == 0) 
	CountRTT = sysval;
    else if(strcmp("CurMSS:", sysvar) == 0)
	CurrentMSS = sysval;
    else if(strcmp("Timeouts:", sysvar) == 0) 
	Timeouts = sysval;
    else if(strcmp("PktsRetrans:", sysvar) == 0) 
	PktsRetrans = sysval;
    else if(strcmp("SACKsRcvd:", sysvar) == 0) 
	SACKsRcvd = sysval;
    else if(strcmp("DupAcksIn:", sysvar) == 0) 
	DupAcksIn = sysval;
    else if(strcmp("MaxRwinRcvd:", sysvar) == 0) 
	MaxRwinRcvd = sysval;
    else if(strcmp("MaxRwinSent:", sysvar) == 0) 
	MaxRwinSent = sysval;
    else if(strcmp("Sndbuf:", sysvar) == 0) 
	Sndbuf = sysval;
    else if(strcmp("X_Rcvbuf:", sysvar) == 0) 
	Rcvbuf = sysval;
    else if(strcmp("DataPktsOut:", sysvar) == 0) 
	DataPktsOut = sysval;
    else if(strcmp("FastRetran:", sysvar) == 0) 
	FastRetran = sysval;
    else if(strcmp("AckPktsOut:", sysvar) == 0) 
	AckPktsOut = sysval;
    else if(strcmp("SmoothedRTT:", sysvar) == 0) 
	SmoothedRTT = sysval;
    else if(strcmp("CurCwnd:", sysvar) == 0) 
	CurrentCwnd = sysval;
    else if(strcmp("MaxCwnd:", sysvar) == 0) 
	MaxCwnd = sysval;
    else if(strcmp("SndLimTimeRwin:", sysvar) == 0) 
	SndLimTimeRwin = sysval;
    else if(strcmp("SndLimTimeCwnd:", sysvar) == 0) 
	SndLimTimeCwnd = sysval;
    else if(strcmp("SndLimTimeSender:", sysvar) == 0) 
	SndLimTimeSender = sysval;
    else if(strcmp("DataBytesOut:", sysvar) == 0) 
	DataBytesOut = sysval;
    else if(strcmp("AckPktsIn:", sysvar) == 0) 
	AckPktsIn = sysval;
    else if(strcmp("SndLimTransRwin:", sysvar) == 0)
	SndLimTransRwin = sysval;
    else if(strcmp("SndLimTransCwnd:", sysvar) == 0)
	SndLimTransCwnd = sysval;
    else if(strcmp("SndLimTransSender:", sysvar) == 0)
	SndLimTransSender = sysval;
    else if(strcmp("MaxSsthresh:", sysvar) == 0)
	MaxSsthresh = sysval;
    else if(strcmp("CurRTO:", sysvar) == 0)
	CurrentRTO = sysval;
    else if(strcmp("c2sData:", sysvar) == 0)
	c2sData = sysval;
    else if(strcmp("c2sAck:", sysvar) == 0)
	c2sAck = sysval;
    else if(strcmp("s2cData:", sysvar) == 0)
	s2cData = sysval;
    else if(strcmp("s2cAck:", sysvar) == 0)
	s2cAck = sysval;
    else if(strcmp("PktsOut:", sysvar) == 0)
	PktsOut = sysval;
    else if(strcmp("mismatch:", sysvar) == 0)
	mismatch = sysval;
    else if(strcmp("bad_cable:", sysvar) == 0)
	bad_cable = sysval;
    else if(strcmp("congestion:", sysvar) == 0)
	congestion = sysval;
    else if(strcmp("half_duplex:", sysvar) == 0)
	half_duplex = sysval;
    else if(strcmp("CongestionSignals:", sysvar) == 0)
	CongestionSignals = sysval;
    else if(strcmp("RcvWinScale:", sysvar) == 0) {
	if (sysval > 15)
	    RcvWinScale = 0;
	else
	    RcvWinScale = sysval;
    }
}

save_dbl_values(char *sysvar, float *sysval) {

	if (strcmp(sysvar, "bw:") == 0)
	    estimate = *sysval;
	else if (strcmp(sysvar, "loss:") == 0)
	    loss = *sysval;
	else if (strcmp(sysvar, "avgrtt:") == 0)
	    avgrtt = *sysval;
	else if (strcmp(sysvar, "waitsec:") == 0)
	    waitsec = *sysval;
	else if (strcmp(sysvar, "timesec:") == 0)
	    timesec = *sysval;
	else if (strcmp(sysvar, "order:") == 0)
	    order = *sysval;
	else if (strcmp(sysvar, "rwintime:") == 0)
	    rwintime = *sysval;
	else if (strcmp(sysvar, "sendtime:") == 0)
	    sendtime = *sysval;
	else if (strcmp(sysvar, "cwndtime:") == 0)
	    cwndtime = *sysval;
	else if (strcmp(sysvar, "rttsec:") == 0)
	    rttsec = *sysval;
	else if (strcmp(sysvar, "rwin:") == 0)
	    rwin = *sysval;
	else if (strcmp(sysvar, "swin:") == 0)
	    swin = *sysval;
	else if (strcmp(sysvar, "cwin:") == 0)
	    cwin = *sysval;
	else if (strcmp(sysvar, "spd:") == 0)
	    spd = *sysval;
	else if (strcmp(sysvar, "aspd:") == 0)
	    aspd = *sysval;

}


int main(int argc, char *argv[])
    {

	int c;
	char tmpstr2[512], tmpstr[16384], varstr[16384];
        int ctlSocket, outSocket, inSocket, in2Socket;
	int ctlport = 3001, port3, port2, inlth;
        uint32_t bytes;
	double stop_time, wait2;
	int sbuf, rbuf;
	int ret, i, xwait, one=1;
	struct hostent *hp;
	struct sockaddr_in server, srv1, srv2, local;
	int largewin;
	char buff[8192], buff2[256];
	struct timeb *tp;
	time_t sec;
	char *host;
	int buf_size=0, set_size, k;
	uint32_t local_addr, peer_addr;
	struct timeval sel_tv;
	fd_set rfd;

	host = argv[argc-1];
	while ((c = getopt(argc, argv, "b:dhl")) != -1) {
	    switch (c) {
		case 'h':
			printf("Usage: %s {options} server\n", argv[0]);
			printf("\t-b # \tSet send/receive buffer to value\n");
			printf("\t-d \tIncrease debug level details\n");
			printf("\t-h \tPrint this help message\n");
			printf("\t-l \tIncrease message level details\n");
			exit(0);
		case 'b':
			buf_size = atoi(optarg);
			break;
		case 'd':
			debug++;
			break;
		case 'l':
			msglvl++;
			break;
		default:
			exit(0);
	    }
	}
	failed = 0;
	hp = (struct hostent *) gethostbyname(host);
	if (hp == NULL) {
	    printf("Error, NDT server name/address not supplied\n");
	    exit (-5);
	}

	printf("Testing network path for configuration and performance problems\n");
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	bcopy(hp->h_addr_list[0], &server.sin_addr.s_addr, hp->h_length);
	/* server.sin_addr.s_addr = (uint32_t) *hp->h_addr_list[0]; */
	server.sin_port = htons(ctlport);
	peer_addr = server.sin_addr.s_addr;

	/* printf("connecting to %s:%d\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port)); */
	ctlSocket = socket(PF_INET, SOCK_STREAM, 0);
	if ((ret = connect(ctlSocket, (struct sockaddr *) &server, sizeof(server))) < 0) {
	    perror("Connect() for control socket failed ");
	    exit(-4);
	}

	/* This is part of the server queuing process.  The server will now send
	 * a integer value over to the client before testing will begin.  If queuing
	 * is enabled, the server will send a positive value.  Zero indicated that
	 * testing can begin, and 9999 indicates that queuing is disabled and the 
	 * user should try again later.
	 */
		
	port3 = 0;
	for (;;) {
            inlth = read(ctlSocket, buff, 100);
	    if (debug > 2)
	    	printf("read %d octets '%s'\n", inlth, buff);
	    if (inlth <= 0)
		break;
	    if ((inlth > 6) && (buff[0] == '0')) {
		port3 = 1;
		break;
	    }

	    if ((strchr(buff, ' ') != NULL) && (inlth > 6)) {
                printf("Information: The server '%s' does not support this command line client\n",
			host);
                exit(0);
            }

	    xwait = atoi(buff);
	    /* fprintf(stderr, "wait flag received = %d\n", xwait); */
	    if (xwait == 0)	/* signal from ver 3.0.x NDT servers */
		break;
	    if (xwait == 9999) {
		fprintf(stderr, "Server Busy: Please wait 60 seconds for the current test to finish\n");
		exit(0);
	    }
	    
	    /* Each test should take less than 30 seconds, so tell them 45 sec * number of 
	     * tests in the queue.
	     */
	    xwait = (xwait * 45);
	    fprintf(stdout, "Another client is currently being served, your test will ");
	    fprintf(stdout,  "begin within %d seconds\n", xwait);
        }

	if (port3 == 0) {
	    inlth = read(ctlSocket, buff, 100);
	    if (debug > 2)
	        fprintf(stderr, "Test port numbers not received, read 2nd packet to get them\n");
            if (inlth == 0) {
                printf("Information: The server '%s' does not support this command line client\n",
			host);
                exit(0);
            }
	}

	port3 = atoi(strtok(buff, " "));
	port2 = atoi(strtok(NULL, " "));
	if (debug > 2)
	    fprintf(stderr, "Testing will begin using port %d and %d\n", port2, port3);

	/* now look for middleboxes (firewalls, NATs, and other boxes that
	 * muck with TCP's end-to-end priciples
	 *
	 */

	bzero(&srv1, sizeof(srv1));
	srv1.sin_family = AF_INET;
	bcopy(hp->h_addr_list[0], &srv1.sin_addr.s_addr, hp->h_length);
	srv1.sin_port = htons(port2);

	in2Socket = socket(PF_INET, SOCK_STREAM, 0);

	system("sleep 2");

	/* printf("connecting to %s:%d\n", inet_ntoa(srv1.sin_addr), ntohs(srv1.sin_port)); */
	largewin = 128*1024;
	k = sizeof(set_size);
	setsockopt(in2Socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	getsockopt(in2Socket, SOL_SOCKET, SO_SNDBUF, &set_size, &k);
	if (debug > 4)
	    printf("\nSend buffer set to %d, ", set_size);
	getsockopt(in2Socket, SOL_SOCKET, SO_RCVBUF, &set_size, &k);
	if (debug > 4)
	    printf("Receive buffer set to %d\n", set_size);
	if (buf_size > 0) {
	    setsockopt(in2Socket, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size));
	    setsockopt(in2Socket, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(buf_size));
	    getsockopt(in2Socket, SOL_SOCKET, SO_SNDBUF, &set_size, &k);
	    if (debug > 4)
	        printf("Changed buffer sizes: Send buffer set to %d, ", set_size);
	    getsockopt(in2Socket, SOL_SOCKET, SO_RCVBUF, &set_size, &k);
	    if (debug > 4)
	        printf("Receive buffer set to %d\n", set_size);
	}
	if ((ret = connect(in2Socket, (struct sockaddr *) &srv1, sizeof(srv1))) < 0) {
	    perror("Connect() for middlebox failed");
	    exit(-10);
	}

	printf("Checking for Middleboxes . . . . . . . . . . . . . . . . . .  ");
	fflush(stdout);
	tmpstr2[0] = '\0';
	i = 0;
	for (;;) {
	    inlth = read(in2Socket, buff, 512);
	    if (inlth <= 0) {
		/* printf("read %d buffers into tmpstr2\n", i); */
		break;
	    }
	    i++;
	    /* printf("%d characters read into buff = '%s'\n", inlth, buff); */
	    strncat(tmpstr2, buff, inlth);
	    /* printf("tmpstr = '%s'\n", tmpstr2); */
	}
	printf("Done\n");

        close(in2Socket);

	inlth = read(ctlSocket, buff, 100); 
	if (inlth <= 0) {  
	    fprintf(stderr, "read failed read 'Go' flag\n");
	    exit(-4);
	}
	

	bzero(&srv2, sizeof(srv2));
	srv2.sin_family = AF_INET;
	bcopy(hp->h_addr_list[0], &srv2.sin_addr.s_addr, hp->h_length);
	srv2.sin_port = htons(port3);

	printf("running 10s outbound test (client to server) . . . . . ");
	fflush(stdout);

	/* inlth = read(ctlSocket, buff, 100);  */
		
	outSocket = socket(PF_INET, SOCK_STREAM, 0);
	setsockopt(outSocket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	getsockopt(outSocket, SOL_SOCKET, SO_SNDBUF, &set_size, &k);
	if (debug > 8)
	    printf("\nSend buffer set to %d, ", set_size);
	getsockopt(outSocket, SOL_SOCKET, SO_RCVBUF, &set_size, &k);
	if (debug > 8)
	    printf("Receive buffer set to %d\n", set_size);
	if (buf_size > 0) {
	    setsockopt(outSocket, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size));
	    setsockopt(outSocket, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(buf_size));
	    getsockopt(outSocket, SOL_SOCKET, SO_SNDBUF, &set_size, &k);
	    if (debug > 4)
	        printf("Changed buffer sizes: Send buffer set to %d(%d), ", set_size, buf_size);
	    getsockopt(outSocket, SOL_SOCKET, SO_RCVBUF, &set_size, &k);
	    if (debug > 4)
	        printf("Receive buffer set to %d(%d)\n", set_size, buf_size);
	}
	if ((ret = connect(outSocket, (struct sockaddr *) &srv2, sizeof(srv2))) < 0) {
	    perror("Connect() for client to server failed");
	    exit(-11);
	}
	pkts = 0;
	k = 0;
	for (i=0; i<8192; i++) {
	    while (!isprint(k&0x7f))
		k++;
	    buff[i] = (k++ % 0x7f);
	}
	sec = time(0);
	stop_time = sec + 10; 
        do {
		write(outSocket, buff, lth);
		pkts++;
	} while (time(0) < stop_time);
	sec = time(0) - sec;
        close(outSocket);
	spdout = ((8.0 * pkts * lth) / 1000) / sec;

	inlth = read(ctlSocket, buff, 100); 
	if (spdout < 1000) 
	    printf(" %0.2f Kb/s\n", spdout);
	else
	    printf(" %0.2f Mb/s\n", spdout/1000);

	printf("running 10s inbound test (server to client) . . . . . . ");
	fflush(stdout);
	
	inlth = read(ctlSocket, buff, 100); 

	/* Cygwin seems to want/need this extra getsockopt() function
	 * call.  It certainly doesn't do anything, but the S2C test fails
	 * at the connect() call if it's not there.  4/14/05 RAC
	 */
	getsockopt(ctlSocket, SOL_SOCKET, SO_SNDBUF, &set_size, &k);

	inSocket = socket(PF_INET, SOCK_STREAM, 0);
	setsockopt(inSocket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	getsockopt(inSocket, SOL_SOCKET, SO_SNDBUF, &set_size, &k);
	if (debug > 4)
	    printf("\nSend buffer set to %d, ", set_size);
	getsockopt(inSocket, SOL_SOCKET, SO_RCVBUF, &set_size, &k);
	if (debug > 4)
	    printf("Receive buffer set to %d\n", set_size);
	if (buf_size > 0) {
	    setsockopt(inSocket, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size));
	    setsockopt(inSocket, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(buf_size));
	    getsockopt(inSocket, SOL_SOCKET, SO_SNDBUF, &set_size, &k);
	    if (debug > 4)
	        printf("Changed buffer sizes: Send buffer set to %d(%d), ", set_size, buf_size);
	    getsockopt(inSocket, SOL_SOCKET, SO_RCVBUF, &set_size, &k);
	    if (debug > 4)
	        printf("Receive buffer set to %d(%d)\n", set_size, buf_size);
	}
	if ((ret = connect(inSocket, (struct sockaddr *) &srv1, sizeof(srv1))) < 0) {
	    perror("Connect() for Server to Client failed");
	    exit(-15);
	}

	bytes = 0;
        sec = time(0);

        /* 
	 * while ((inlth = read(inSocket, buff, sizeof(buff))) > 0) {
         *         bytes += inlth;
         * }
	 *
	 * Linux updates the sel_tv time values everytime select returns.  This
	 * means that eventually the timer will reach 0 seconds and select will
	 * exit with a timeout signal.  Other OS's don't do that so they need
	 * another method for detecting a long-running process.  The check below
	 * will cause the loop to terminate if select says there is something
	 * to read and the loop has been active for over 14 seconds.  This usually
	 * happens when there is a problem (duplex mismatch) and there is data
	 * queued up on the server.
	 */

	
	sel_tv.tv_sec = 13;
	sel_tv.tv_usec = 500000;
	FD_ZERO(&rfd);
	FD_SET(inSocket, &rfd);
	for (;;) {
	    ret = select(inSocket+1, &rfd, NULL, NULL, &sel_tv);
	    if ((time(0)-sec) > 14) {
		if (debug > 4)
		    fprintf(stderr, "Receive test running long, break out of read loop\n");
		break;
	    }
	    if (ret > 0) {
		inlth = read(inSocket, buff, sizeof(buff));
		if (inlth == 0)
		    break;
		bytes += inlth;
		continue;
	    }
	    if (debug > 5)
		perror("s2c read loop exiting:");
	    break;
	}
        sec =  time(0) - sec;
	spdin = ((8.0 * bytes) / 1000) / sec;
	/* inlth = read(ctlSocket, buff, 512); */

	if (spdin < 1000)
	    printf("%0.2f kb/s\n", spdin);
	else
	    printf("%0.2f Mb/s\n", spdin/1000);

        close(inSocket);

	/* system("sleep 1"); */
	i = sizeof(local);
	getsockname(ctlSocket, (struct sockaddr *) &local, &i); 
	local_addr = local.sin_addr.s_addr;

	/* get web100 variables from server */
	tmpstr[i] = '\0';
	i = 0;
	for (;;) {
	    inlth = read(ctlSocket, buff, 512);
	    if (inlth <= 0) {
		/* printf("read %d buffers into tmpstr\n", i); */
		break;
	    }
	    i++;
	    /* printf("%d characters read into buff = '%s'\n", inlth, buff); */
	    strncat(tmpstr, buff, inlth);
	    /* printf("tmpstr = '%s'\n", tmpstr); */
	}

        /* close(inSocket); */
        close(ctlSocket);
	strcpy(varstr, tmpstr);
	testResults(tmpstr);
	middleboxResults(tmpstr2, local_addr, peer_addr);
	if (msglvl > 1)
	    printVariables(varstr);

}
