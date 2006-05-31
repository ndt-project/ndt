/*
Copyright © 2003 University of Chicago.  All rights reserved.
The Web100 Network Diagnostic Tool (NDT) is distributed subject to
the following license conditions:
SOFTWARE LICENSE AGREEMENT
Software: Web100 Network Diagnostic Tool (NDT)

1. The "Software", below, refers to the Web100 Network Diagnostic Tool (NDT)
(in either source code, or binary form and accompanying documentation). Each
licensee is addressed as "you" or "Licensee."

2. The copyright holder shown above hereby grants Licensee a royalty-free
nonexclusive license, subject to the limitations stated herein and U.S. Government
license rights.

3. You may modify and make a copy or copies of the Software for use within your
organization, if you meet the following conditions: 
    a. Copies in source code must include the copyright notice and this Software
    License Agreement.
    b. Copies in binary form must include the copyright notice and this Software
    License Agreement in the documentation and/or other materials provided with the copy.

4. You may make a copy, or modify a copy or copies of the Software or any
portion of it, thus forming a work based on the Software, and distribute copies
outside your organization, if you meet all of the following conditions: 
    a. Copies in source code must include the copyright notice and this
    Software License Agreement;
    b. Copies in binary form must include the copyright notice and this
    Software License Agreement in the documentation and/or other materials
    provided with the copy;
    c. Modified copies and works based on the Software must carry prominent
    notices stating that you changed specified portions of the Software.

5. Portions of the Software resulted from work developed under a U.S. Government
contract and are subject to the following license: the Government is granted
for itself and others acting on its behalf a paid-up, nonexclusive, irrevocable
worldwide license in this computer software to reproduce, prepare derivative
works, and perform publicly and display publicly.

6. WARRANTY DISCLAIMER. THE SOFTWARE IS SUPPLIED "AS IS" WITHOUT WARRANTY
OF ANY KIND. THE COPYRIGHT HOLDER, THE UNITED STATES, THE UNITED STATES
DEPARTMENT OF ENERGY, AND THEIR EMPLOYEES: (1) DISCLAIM ANY WARRANTIES,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO ANY IMPLIED WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE OR NON-INFRINGEMENT,
(2) DO NOT ASSUME ANY LEGAL LIABILITY OR RESPONSIBILITY FOR THE ACCURACY,
COMPLETENESS, OR USEFULNESS OF THE SOFTWARE, (3) DO NOT REPRESENT THAT USE
OF THE SOFTWARE WOULD NOT INFRINGE PRIVATELY OWNED RIGHTS, (4) DO NOT WARRANT
THAT THE SOFTWARE WILL FUNCTION UNINTERRUPTED, THAT IT IS ERROR-FREE OR THAT
ANY ERRORS WILL BE CORRECTED.

7. LIMITATION OF LIABILITY. IN NO EVENT WILL THE COPYRIGHT HOLDER, THE
UNITED STATES, THE UNITED STATES DEPARTMENT OF ENERGY, OR THEIR EMPLOYEES:
BE LIABLE FOR ANY INDIRECT, INCIDENTAL, CONSEQUENTIAL, SPECIAL OR PUNITIVE
DAMAGES OF ANY KIND OR NATURE, INCLUDING BUT NOT LIMITED TO LOSS OF PROFITS
OR LOSS OF DATA, FOR ANY REASON WHATSOEVER, WHETHER SUCH LIABILITY IS ASSERTED
ON THE BASIS OF CONTRACT, TORT (INCLUDING NEGLIGENCE OR STRICT LIABILITY), OR
OTHERWISE, EVEN IF ANY OF SAID PARTIES HAS BEEN WARNED OF THE POSSIBILITY OF
SUCH LOSS OR DAMAGES.
The Software was developed at least in part by the University of Chicago,
as Operator of Argonne National Laboratory (http://miranda.ctd.anl.gov:7123/). 
 */

/* local include file contains needed structures */
#include "web100srv.h"
#include "../config.h"
#include "network.h"

/* list of global variables used throughout this program. */
int window = 64000;
int randomize=0;
int count_vars=0;
int debug=0;
int dumptrace=0;
int usesyslog=0;
int multiple=0;
int set_buff=0;
int port2=PORT2, port3=PORT3;
int mon_pipe1[2], mon_pipe2[2];
int admin_view=0;
int queue=1;
int view_flag=0;
int port=PORT, record_reverse=0;
int testing, waiting;
int experimental=0;
int old_mismatch=0;	/* use the old duplex mismatch detection heuristic */
int sig1, sig2, sig17;

/* experimental limit code, can remove later */
u_int32_t limit=0;

extern int errno;

char *VarFileName=NULL;
char *LogFileName=NULL;
char *ProcessName={"web100srv"};
char *ConfigFileName=NULL;
char buff[BUFFSIZE+1];
char *rmt_host;
char spds[4][256], buff2[32];
char *device=NULL;

/* static pcap_t *pd; */
pcap_t *pd;
pcap_dumper_t *pdump;
float run_ave[4];

struct ndtchild *head_ptr;
int ndtpid;

/* This routine does the main work of reading packets from the network
 * interface.  It should really be in the web100-pcap.c file, but it uses
 * some shared pcap_t and pcap_dumper_t variables and I haven't figured out
 * how to pass these around.  It steps through the input file
 * and calculates the link speed between each packet pair.  It then
 * increments the proper link bin.
 */
/* pcap_handler print_speed(u_char *user, const struct pcap_pkthdr *h, const u_char *p) */
int print_speed(u_char *user, const struct pcap_pkthdr *h, const u_char *p)
{

	u_int caplen=h->caplen;
	u_int length=h->len;
	u_int32_t sec, usec;
	struct ether_header *enet;
	const struct ip *ip;
	const struct ip6_hdr *ip6;
	const struct tcphdr *tcp;
	struct spdpair current;
	/* float bits, spd, time; */
	int i, v4_or_v6=0;

	if (dumptrace == 1)
	    pcap_dump((u_char *)pdump, h, p);

	if (pd == NULL) {
		fprintf(stderr, "!#!#!#!# Error, trying to process IF data, but pcap fd closed\n");
		return;
	}

	if ((sig1 == 1) || (sig2 == 1)) {
		if (debug > 4)
			fprintf(stderr, "Received SIGUSRx signal terminating data collection loop for pid=%d\n", getpid());
		if (sig1 == 1) {
			if (debug > 3) {
	    	      	    fprintf(stderr, "Sending pkt-pair data back to parent on pipe %d, %d\n",
			    	mon_pipe1[0], mon_pipe1[1]);
				fprintf(stderr, "fwd.saddr = %x:%d, rev.saddr = %x:%d\n",
					fwd.saddr, fwd.sport, rev.saddr, rev.sport);
			}
			print_bins(&fwd, mon_pipe1, LogFileName, debug);
			print_bins(&rev, mon_pipe1, LogFileName, debug);
			if (pd != NULL)
				pcap_close(pd);
			if (dumptrace == 1)
		    		pcap_dump_close(pdump);
			sig1 = 2;
		}

		if (sig2 == 1) {
			if (debug > 3) {
	    	      	    fprintf(stderr, "Sending pkt-pair data back to parent on pipe %d, %d\n",
			    	mon_pipe2[0], mon_pipe2[1]);
				fprintf(stderr, "fwd.saddr = %x:%d, rev.saddr = %x:%d\n",
					fwd.saddr, fwd.sport, rev.saddr, rev.sport);
			}
			print_bins(&fwd, mon_pipe2, LogFileName, debug);
			print_bins(&rev, mon_pipe2, LogFileName, debug);
			if (pd != NULL)
				pcap_close(pd);
			if (dumptrace == 1)
				pcap_dump_close(pdump);
			sig2 = 2;
		}
		if (debug > 5)
			fprintf(stderr, "Finished reading pkt-pair data from network, process %d should terminate now\n", getpid());
		return;

	}

	current.sec = h->ts.tv_sec;
	current.usec = h->ts.tv_usec;
	current.time = (current.sec*1000000) + current.usec;

	enet = (struct ether_header *)p;
	p += sizeof(struct ether_header); 	/* move packet pointer past ethernet fields */

	v4_or_v6 = ((*p & 0xF0) >> 4);

	/* fprintf(stderr, "::::::::::: IP version number = %d\n", v4_or_v6); */

	if (v4_or_v6 == 4) {
	    ip = (const struct ip *)p;
	    p += (ip->ip_hl) * 4;
	} else {
	    ip6 = (const struct ip6_hdr *)p;
	    if (ip6->ip6_nxt == IPPROTO_TCP)
		p += 64 * 5;	/* IPv6 header is 2 128 bit addresses + 64 bits of other stuff */
	}

	tcp = (const struct tcphdr *)p;

	if (v4_or_v6 == 4) {
	    current.saddr = ip->ip_src.s_addr;
	    current.daddr = ip->ip_dst.s_addr;
	} else {
	    /* memcpy(&current.s6addr, &ip->ip6_src, 16);
	     * memcpy(&current.d6addr, &ip->ip6_dst, 16);
	     */
	}

	/* printf("Data received from src[%s]/", inet_ntoa(current.saddr));
	 * printf("dst[%s] socket\n", inet_ntoa(current.daddr));
	 */

	current.sport = ntohs(tcp->source);
	current.dport = ntohs(tcp->dest);
	current.seq = ntohl(tcp->seq);
	current.ack = ntohl(tcp->ack_seq);
	current.win = ntohs(tcp->window);

	if (fwd.saddr == 0) {
	    if (debug > 0)
		fprintf(stderr, "New packet trace started -- initializing counters\n");
	    fwd.saddr = current.saddr;
	    fwd.daddr = current.daddr;
	    memcpy(&fwd.s6addr, &current.s6addr, 16);
	    memcpy(&fwd.d6addr, &current.d6addr, 16);
	    fwd.sport = current.sport;
	    fwd.dport = current.dport;
	    fwd.st_sec = current.sec;
	    fwd.st_usec = current.usec;
	    rev.saddr = current.daddr;
	    rev.daddr = current.saddr;
	    memcpy(&rev.s6addr, &current.d6addr, 16);
	    memcpy(&rev.d6addr, &current.s6addr, 16);
	    rev.sport = current.dport;
	    rev.dport = current.sport;
	    rev.st_sec = current.sec;
	    rev.st_usec = current.usec;
	    fwd.dec_cnt = 0;
	    fwd.inc_cnt = 0;
	    fwd.same_cnt = 0;
	    fwd.timeout = 0;
	    fwd.dupack = 0;
	    rev.dec_cnt = 0;
	    rev.inc_cnt = 0;
	    rev.same_cnt = 0;
	    rev.timeout = 0;
	    rev.dupack = 0;
	    return;
	}

	if (current.sport == port2) {
			calculate_spd(&current, &rev, port2, port3);
			return;
	}
	if (current.sport == port3) {
			calculate_spd(&current, &fwd, port2, port3);
			return;
	}
	if (current.dport == port2) {
			calculate_spd(&current, &fwd, port2, port3);
			return;
	}
	if (current.dport == port3) {
			calculate_spd(&current, &rev, port2, port3);
			return;
	}
	
	if (debug > 5)
		fprintf(stderr, "Fault: unknown packet received with src/dst port = %d/%d\n,", 
				current.sport, current.dport);

/* 
 * 	if (fwd.saddr == current.saddr) {
 * 	    if (current.dport == port2)
 * 		calculate_spd(&current, &fwd, port2, port3);
 * 	    else if (current.sport == port3)
 * 		calculate_spd(&current, &fwd, port2, port3);
 * 	    if ((current.sport != port2) && (debug > 15))
 * 		fprintf(stderr, "Fault: forward packet received with dst port = %d\n", current.dport);
 * 	    return;
 * 	}
 * 	    if (rev.saddr == current.saddr) {
 * 	    if (current.sport == port2)
 * 		calculate_spd(&current, &rev, port2, port3);
 * 	    else if (current.dport == port3)
 * 		calculate_spd(&current, &rev, port2, port3);
 * 	    else if (debug > 15)
 * 		fprintf(stderr, "Fault: reverse packet received with dst port = %d\n", current.dport);
 * 	    return;
 * 	}
 */

}

/* This routine performs the open and initialization functions needed by the
 * libpcap routines.  The print_speed function above, is passed to the
 * pcap_open_live() function as the pcap_handler.  This routine really should
 * be located in the web100-pcap.c file, but for the same reason as described
 * above, it's here for now.  Maybe some day it will move back to where it
 * belongs.
 */
void init_pkttrace(struct sockaddr_in *sock_addr, struct sockaddr_in6 *sock6_addr,
	int monitor_pipe[2], char *device, int family)
{

	char cmdbuf[256];
	pcap_handler printer;
	u_char * pcap_userdata;
	struct bpf_program fcode;
	char errbuf[PCAP_ERRBUF_SIZE];
	char tmpstr[64];
	int rc, cnt, pflag;
	/* pcap_t *pd; */
	char c;

	cnt = -1;	/* read forever, or until end of file */
	sig1 = 0;
	sig2 = 0;
	/* device = NULL; */

	init_vars(&fwd);
	init_vars(&rev);

	if (device == NULL) {
	    device = pcap_lookupdev(errbuf);
	    if (device == NULL) {
		fprintf(stderr, "pcap_lookupdev failed: %s\n", errbuf);
	    }
	}

	/* special check for localhost, set device accordingly */
 	if (strcmp(inet_ntoa(sock_addr->sin_addr), "127.0.0.1") == 0)
 	    strncpy(device, "lo", 3);

	if (debug > 0)
	    fprintf(stderr, "Opening network interface '%s' for packet-pair timing\n", device);
	
	if ((pd = pcap_open_live(device, 68, !pflag, 1000, errbuf)) == NULL) {
	    fprintf(stderr, "pcap_open_live failed: %s\n", errbuf);
	}

	if (debug > 1)
	    fprintf(stderr, "pcap_open_live() returned pointer 0x%x\n", pd);

	if (family == AF_INET)
	    sprintf(cmdbuf, "host %s and port %d\0", inet_ntoa(sock_addr->sin_addr),
		 ntohs(sock_addr->sin_port));
	else {
	    inet_ntop(family, &sock6_addr->sin6_addr, tmpstr, 46);
	    sprintf(cmdbuf, "host %s and port %d\0", tmpstr, ntohs(sock6_addr->sin6_port));
	}

	if (debug > 0) {
	    fprintf(stderr, "installing pkt filter for '%s'\n", cmdbuf);
	    fprintf(stderr, "Initial pkt src data = %x\n", fwd.saddr);
	}

	if (pcap_compile(pd, &fcode, cmdbuf, 0, 0xFFFFFF00) < 0) {
	    fprintf(stderr, "pcap_compile failed %s\n", pcap_geterr(pd));
	}
	
	if (pcap_setfilter(pd, &fcode) < 0) {
	    fprintf(stderr, "pcap_setfiler failed %s\n", pcap_geterr(pd));
	}

	if (dumptrace == 1) {
	    fprintf(stderr, "Creating trace file for connection\n");
	    sprintf(cmdbuf, "ndttrace.%s.%d", inet_ntoa(sock_addr->sin_addr),
			ntohs(sock_addr->sin_port));
	    pdump = pcap_dump_open(pd, cmdbuf);
	    if (pdump == NULL) {
		fprintf(stderr, "Unable to create trace file '%s'\n", cmdbuf);
		dumptrace = 0;
	    }
	}

	printer = (pcap_handler) print_speed;
	write(monitor_pipe[1], "Ready", 128);

	/* kill process off if parent doesn't send a signal. */
	alarm(45);

	/* rc = pcap_loop(pd, cnt, printer, pcap_userdata);
	 * printf("pcap_loop() returned %d\n", rc);
	 * if (rc < 0) {
	 */
	if (pcap_loop(pd, cnt, printer, pcap_userdata) < 0) {
	    if (debug > 4)
		fprintf(stderr, "pcap_loop failed %s\n", pcap_geterr(pd));
	}
	if (debug > 4)
		fprintf(stderr, "Pkt-Pair data collection ended, waiting for signal to terminate process\n");

	if (sig1 == 2) {
		read(mon_pipe1[0], &c, 1);
		close(mon_pipe1[0]);
		close(mon_pipe1[1]);
		sig1 = 0;
	}
	if (sig2 == 2) {
		while ((read(mon_pipe2[0], &c, 1)) < 0)
		    ;
		sleep(5);
		close(mon_pipe2[0]);
		close(mon_pipe2[1]);
		sig2 = 0;
	}

	if (debug > 7)
		fprintf(stderr, "Finally Finished reading data from network, process %d should terminate now\n", getpid());
}

/* Process a SIGCHLD signal */
void child_sig(void)
{

	char c;
	FILE *fp;
        int pid, status, i;
	struct ndtchild *tmp1, *tmp2;

	        /*
		 * avoid zombies, since we run forever
	         * Use the wait3() system call with the WNOHANG option.
	         */
			  tmp1 = head_ptr;
			  i=0;
			  if (debug > 1)
  			      fprintf(stderr,"Received SIGCHLD signal for active web100srv process [%d]\n",
				    getpid());
			  
        		  while ((pid = wait3(&status, WNOHANG, (struct rusage *) 0)) > 0) {
				if (debug > 2) {
				    fprintf(stderr, "wait3() returned %d for PID=%d\n", status, pid);
				    if (WIFEXITED(status) != 0)
					fprintf(stderr, "wexitstatus = '%s'\n", WEXITSTATUS(status));
				    if (WIFSIGNALED(status) == 1)
					fprintf(stderr, "wtermsig = %d\n", WTERMSIG(status));
				    if (WIFSTOPPED(status) == 1)
					fprintf(stderr, "wstopsig = %d\n", WSTOPSIG(status));
				}
				if (status != 0) 
				    return;
				if (multiple == 1)
				    return;

				if (debug > 3)
				    fprintf(stderr, "Attempting to clean up child %d, head pid = %d\n",
						    pid, head_ptr->pid);
				if (head_ptr->pid == pid) {
				    if (debug > 4)
					fprintf(stderr, "Child process %d causing head pointer modification\n", pid);
				    tmp1 = head_ptr;
				    head_ptr = head_ptr->next;
				    free(tmp1);
				    testing = 0;
				    waiting--;
				    if (debug > 3) 
					fprintf(stderr, "Removing Child from head, decrementing waiting now = %d\n", waiting);
				    return;
				}
				else {
				    while (tmp1->next != NULL) {
				        if (tmp1->next->pid == pid) {
						if (debug > 3)
						    fprintf(stderr, "Child process %d causing task list modification\n", pid);
					    tmp2 = tmp1->next;
					    tmp1->next = tmp2->next;
					    free(tmp2);
					    testing = 0;
					    waiting--;
				    if (debug > 3) 
					fprintf(stderr, "Removing Child from list, decrementing waiting now = %d\n", waiting);
					    return;
					}
					tmp1 = tmp1->next;
					if (debug > 5)
					    fprintf(stderr, "Looping through service queue ptr = 0x%x\n",
							    tmp1);
					if (debug > 5) {
					    i++;
					    if (i > 10)
						    return;
					}
				    }
				}
				if (debug > 2)
				    fprintf(stderr, "SIGCHLD routine finished!\n");
				return;
			  }


}

/* Catch termination signal(s) and print message in log file */
void cleanup(int signo)
{
	FILE *fp;
        int pid;

	if (debug > 0) {
	    fprintf(stderr, "Signal %d received by process %d\n", signo, getpid());

	    fp = fopen(LogFileName,"a");
	    if (fp != NULL)
	        fprintf(fp, "Signal %d received by process %d\n", signo, getpid());
	    fclose(fp);
	}
	switch (signo) {
	    default:
  			fp = fopen(LogFileName,"a");
			fprintf(fp, "Unexpected signal (%d) received, process (%d) may terminate\n",
				signo, getpid());
			fclose(fp);
			break;
	    case SIGSEGV:
			if (debug > 5)
			    fprintf(stderr, "DEBUG, caught SIGSEGV signal and terminated process (%d)\n", getpid());
	    case SIGINT:
	    case SIGTERM:
			exit(0);
	    case SIGUSR1:
			  if (debug > 5)
			      fprintf(stderr, "DEBUG, caught SIGUSR1, setting sig1 flag to force exit\n");
			  sig1 = 1;
			  break;

	    case SIGUSR2:
			  sig2 = 1;
			  if (debug > 5)
			      fprintf(stderr, "DEBUG, caught SIGUSR2, setting sig2 flag to force exit\n");
			  break;
			  
	    case SIGALRM:
  			  fp = fopen(LogFileName,"a");
  			  if (fp  != NULL) {
				fprintf(fp,"Received SIGALRM signal: terminating active web100srv process [%d]\n",
			  	        getpid());
				fclose(fp);
			  }
			  exit(0);
	    case SIGPIPE:
  			  fp = fopen(LogFileName,"a");
			  if (fp != NULL) {
  			          fprintf(fp,"Received SIGPIPE signal: terminating active web100srv process [%d]\n",
				        getpid());
			      fclose(fp);
			  }
			  break;

	    case SIGHUP:
	  		/* Initialize Web100 structures */
			  count_vars = web100_init(VarFileName, debug);

			/* The administrator view automatically generates a usage page for the
	 		 * NDT server.  This page is then accessable to the general public.
	 		 * At this point read the existing log file and generate the necessary
	 		 * data.  This data is then updated at the end of each test.
	 		 * RAC 3/11/04
	 		 */
			  if (admin_view == 1)
	    		    view_init(LogFileName, debug);
			  break;

	    case SIGCHLD:
				/* moved actions to child_sig() routine on 3/10/05  RAC
				 * Now all we do here is set a flag and return the flag
				 * is checked near the top of the main wait loop, so it
				 * will only be accessed once and only the testing proces
				 * will attempt to do something with it.
				 */
			  sig17 = 1;
			  break;
	}
}


err_sys(s) char *s;
{
	perror(s);
	exit(1);
}

double secs()
{
        struct timeval ru;
        gettimeofday(&ru, (struct timezone *)0);
        return(ru.tv_sec + ((double)ru.tv_usec)/1000000);
}

/* LoadConfig routine copied from Internet2. */
static void LoadConfig(char **lbuf, size_t *lbuf_max)
{
	FILE *conf;
	char keybuf[256], valbuf[256];
	char *key=keybuf, *val=valbuf;
	int rc=0;


	if (!(conf = fopen(ConfigFileName, "r")))
	    return;

	if (debug > 0)
	    fprintf(stderr, " Reading config file %s to obtain options\n", ConfigFileName);

	while ((rc = I2ReadConfVar(conf, rc, key, val, 256, lbuf, lbuf_max)) > 0) {
	    if (strncasecmp(key, "administrator_view", 5) == 0) {
		admin_view = 1;
		continue;
	    }

	    else if (strncasecmp(key, "multiple_clients", 5) == 0) {
		multiple = 1;
		continue;
	    }

	    else if (strncasecmp(key, "record_reverse", 6) == 0) {
		record_reverse = 1;
		continue;
	    }

	    else if (strncasecmp(key, "write_trace", 5) == 0) {
		dumptrace = 1;
		continue;
	    }

	    else if (strncasecmp(key, "TCP_Buffer_size", 3) == 0) {
		window = atoi(val);
		set_buff = 1;
		continue;
	    }

	    else if (strncasecmp(key, "debug", 5) == 0) {
		debug = atoi(val);
		continue;
	    }

	    else if (strncasecmp(key, "variable_file", 6) == 0) {
		VarFileName = val;
		continue;
	    }

	    else if (strncasecmp(key, "log_file", 3) == 0) {
		LogFileName = val;
		continue;
	    }

	    else if (strncasecmp(key, "device", 5) == 0) {
		device = val;
		continue;
	    }

	    else if (strncasecmp(key, "port", 4) == 0) {
		port = atoi(val);
		continue;
	    }

	    else if (strncasecmp(key, "syslog", 6) == 0) {
		usesyslog = 1;
		continue;
	    }

	    else if (strncasecmp(key, "disable_FIFO", 5) == 0) {
		queue = 0;
		continue;
	    }
	}
	fclose(conf);
}
void run_test(web100_agent* agent, int ctlsockfd, int family) {

	char date[32], *ctime();
	char spds[4][256], buff2[32], tmpstr[256];
	char logstr1[4096], logstr2[1024];

	int maxseg=1456, largewin=16*1024*1024;
	int sock2, sock3, sock4; 
	int n, sockfd, midsockfd, recvsfd, xmitsfd, clilen, childpid, servlen, one=1;
	int Timeouts, SumRTT, CountRTT, PktsRetrans, FastRetran, DataPktsOut;
	int AckPktsOut, CurrentMSS, DupAcksIn, AckPktsIn, MaxRwinRcvd, Sndbuf;
	int CurrentCwnd, SndLimTimeRwin, SndLimTimeCwnd, SndLimTimeSender, DataBytesOut;
	int SndLimTransRwin, SndLimTransCwnd, SndLimTransSender, MaxSsthresh;
	int CurrentRTO, CurrentRwinRcvd, MaxCwnd, CongestionSignals, PktsOut, MinRTT;
	int CongAvoid, CongestionOverCount, MaxRTT, OtherReductions, CurTimeoutCount;
	int AbruptTimeouts, SendStall, SlowStart, SubsequentTimeouts, ThruBytesAcked;
	int RcvWinScale, SndWinScale;
	int link=100, mismatch=0, bad_cable=0, half_duplex=0, congestion=0, totaltime;
	int spdi, direction=0, ret, spd_index;
	int index, links[16], max, total;
	int c2sdata, c2sack, s2cdata, s2cack;
	int spd_sock, spd_sock2, spdfd, j, i, rc;
	int mon_pid1, mon_pid2, totalcnt, spd;
	int seg_size, win_size;
	int autotune, rbuff, sbuff, k;
	int dec_cnt, same_cnt, inc_cnt, timeout, dupack;
	int cli_c2sspd, cli_s2cspd;

	time_t stime;

	double loss, rttsec, bw, rwin, swin, cwin, speed;
	double rwintime, cwndtime, sendtime;
	double oo_order, waitsec;
	double bw2, avgrtt, timesec, loss2, RTOidle;
	double x2cspd, s2cspd, c2sspd, bytes=0;
	double s2c2spd;
 	double s, t, secs();
	double acks, aspd, tmouts, rtran, dack;
	float runave;

	struct sockaddr_in spd_addr, spd_addr2, spd_cli;
	struct sockaddr_in cli_addr, cli2_addr, srv2_addr, srv3_addr, srv4_addr, serv_addr;
	struct sockaddr_in6 serv6_addr, srv62_addr, srv63_addr, cli6_addr, cli62_addr;
	struct timeval sel_tv;
	fd_set rfd, wfd, efd;
	FILE *fp;

	/* experimental code to capture and log multiple copies of the the
	 * web100 variables using the web100_snap() & log() functions.
	 */
	web100_group* group;
	web100_group* tgroup;
	web100_group* rgroup;
	web100_connection* conn;
	web100_connection* xconn;
	web100_snapshot* snap;
	web100_snapshot* tsnap;
	web100_snapshot* rsnap;
	web100_log* log;
	char logname[128];
	web100_var* var;
	int SndMax=0, SndUna=0;
	int c1=0, c2=0;


	stime = time(0);
	if (debug > 3)
	    fprintf(stderr, "Child process %d started\n", getpid());

	for (spd_index=0; spd_index<4; spd_index++)
	    for (ret=0; ret<256; ret++)
		spds[spd_index][ret] = 0x00;
	spd_index = 0;

	autotune = web100_autotune(ctlsockfd, agent, family, debug);

 	if ( (sock2 = socket(AF_INET, SOCK_STREAM, 0)) < 0)
 		err_sys("server: can't open stream socket");
 	setsockopt(sock2, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
 	bzero((char *) &srv2_addr, sizeof(srv2_addr));
 	srv2_addr.sin_family = AF_INET;
 	srv2_addr.sin_addr.s_addr = htonl(INADDR_ANY);
 	if (multiple == 1)
 	    srv2_addr.sin_port = 0;  
 	else
    	    srv2_addr.sin_port = htons(port2);  
 
 	if (bind(sock2, (struct sockaddr *) &srv2_addr, sizeof(serv_addr)) < 0)
 		err_sys("server: can't bind local address");

	if (set_buff > 0) {
	    setsockopt(sock2, SOL_SOCKET, SO_SNDBUF, &window, sizeof(window));
	    setsockopt(sock2, SOL_SOCKET, SO_RCVBUF, &window, sizeof(window));
	}
	if (autotune > 0) {
	    setsockopt(sock2, SOL_SOCKET, SO_SNDBUF, &largewin, sizeof(largewin));
	    setsockopt(sock2, SOL_SOCKET, SO_RCVBUF, &largewin, sizeof(largewin));
	}
	if (debug > 0)
	    fprintf(stderr, "listening for Inet connection on sock2, fd=%d\n", sock2);
	listen(sock2, 2);

  	if ( (sock3 = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  		err_sys("server: can't open stream socket");
  	setsockopt(sock3, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
  	bzero((char *) &srv3_addr, sizeof(srv3_addr));
  	srv3_addr.sin_family = AF_INET;
  	srv3_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  	if (multiple == 1)
  	    srv3_addr.sin_port = 0;
  	else
  	    srv3_addr.sin_port = htons(port3);
  
  	if (bind(sock3, (struct sockaddr *) &srv3_addr, sizeof(serv_addr)) < 0)
  		err_sys("server: can't bind local address");

	/* send server ports to client */

	if ( (sock3 = socket(family, SOCK_STREAM, 0)) < 0)
		err_sys("server: can't open stream socket");

	bzero((char *) &srv3_addr, sizeof(srv3_addr));
	srv3_addr.sin_family = AF_INET;
	srv3_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (multiple == 1)
	    srv3_addr.sin_port = 0;  /* any */
	else
	    srv3_addr.sin_port = htons(port3);
	setsockopt(sock3, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	if (bind(sock3, (struct sockaddr *) &srv3_addr, sizeof(srv3_addr)) < 0)
	    err_sys("Server fault: (Test engine port3) can't bind to IPv4 address");

	if (debug > 0)
	    fprintf(stderr, "server ports %d %d\n", port2, port3);
	alarm(30);  /* reset alarm() signal to gain another 30 seconds */
	sprintf(buff, "%d %d", port2, port3);

	/* This write kicks off the whole test cycle.  The client will start the
	 * middlebox test after receiving this string
	 */
	write(ctlsockfd, buff, strlen(buff));  

	/* send 3rd server port to client */
	clilen = sizeof(cli_addr);
	getsockname(sock3, (struct sockaddr *) &cli_addr, &clilen);
	port3 = ntohs(cli_addr.sin_port);

	/* set mss to 1456 (strange value), and large snd/rcv buffers
	 * should check to see if server supports window scale ?
	 */
	setsockopt(sock3, SOL_TCP, TCP_MAXSEG, &maxseg, sizeof(maxseg));
	setsockopt(sock3, SOL_SOCKET, SO_SNDBUF, &largewin, sizeof(largewin));
	setsockopt(sock3, SOL_SOCKET, SO_RCVBUF, &largewin, sizeof(largewin));
	if (debug > 0)
	    fprintf(stderr, "listening for Inet connection on sock3, fd=%d\n", sock3);
	listen(sock3, 2);
	if (debug > 1) {
	    fprintf(stderr, "Middlebox test, Port %d waiting for incoming connection\n", port3);
	    i = sizeof(seg_size);
	    getsockopt(sock3, SOL_TCP, TCP_MAXSEG, &seg_size, &i);
	    getsockopt(sock3, SOL_SOCKET, SO_RCVBUF, &win_size, &i);
	    fprintf(stderr, "Set MSS to %d, Receiving Window size set to %dKB\n", seg_size, win_size);
	    getsockopt(sock3, SOL_SOCKET, SO_SNDBUF, &win_size, &i);
	    fprintf(stderr, "Sending Window size set to %dKB\n", seg_size, win_size);
	}

	/* Post a listen on port 3003.  Client will connect here to run the 
	 * middlebox test.  At this time it really only checks the MSS value
	 * and does NAT detection.  More analysis functions (window scale)
	 * will be done in the future.
	 */
	/* printf("Trying to accept new connection for middlebox test\n"); */
	midsockfd = accept(sock3, (struct sockaddr *) &cli2_addr, &clilen);

	/* printf("Accepted new middlebox connection\n"); */
	buff[0] = '\0';
	web100_middlebox(midsockfd, agent, buff, family, debug);
	write(ctlsockfd, buff, strlen(buff));  
	ret = read(ctlsockfd, buff, 32);  
	buff[ret] = '\0';
	s2c2spd = atof(buff);
	if (debug > 3)
	    fprintf(stderr, "CWND limited throubhput = %0.0f kbps (%s)\n", s2c2spd, buff);
	shutdown(midsockfd, SHUT_WR);
	close(midsockfd);
	close(sock3);

	/* Middlebox testing done, now get ready to run throughput tests.
	 * First send client a signal to open a new connection.  Then grap
	 * the client's IP address and TCP port number.  Next fork() a child
	 * process to read packets from the libpcap interface.  Finally sent
	 * the client another signal, via the control channel
	 * when server is ready to go.
	 */
	/* write(ctlsockfd, buff, strlen(buff));  */

	if (debug > 0)
	    fprintf(stderr, "Sending 'GO' signal, to tell client to head for the next test\n");

	write(ctlsockfd, "Open-c2s-connection", 21);  

	recvsfd = accept(sock2, (struct sockaddr *) &cli_addr, &clilen);

	if (getuid() == 0) {
	    pipe(mon_pipe1);
	    if ((mon_pid1 = fork()) == 0) {
		close(ctlsockfd);
		close(sock2);
		close(recvsfd);
		if (debug > 4) {
		    fprintf(stderr, "C2S test Child thinks pipe() returned fd0=%d, fd1=%d\n",
				    mon_pipe1[0], mon_pipe1[1]);
		}
		if (debug > 1)
		    fprintf(stderr, "C2S test calling init_pkttrace() with pd=0x%x\n", cli_addr);
	        init_pkttrace(&cli_addr, NULL, mon_pipe1, device, family);
		exit(0);		/* Packet trace finished, terminate gracefully */
	    }
	    if (read(mon_pipe1[0], tmpstr, 128) <= 0) {
	        printf("error & exit");
		exit(0);
	    }
	}

	if (debug > 4) {
	    fprintf(stderr, "C2S test Parent thinks pipe() returned fd0=%d, fd1=%d\n",
			    mon_pipe1[0], mon_pipe1[1]);
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
	/* autotune = web100_autotune(recvsfd, agent, family, debug);
	 * if (debug > 3)
	 *     fprintf(stderr, "C2S Test Autotune = %d Recv Buff now %d, Send Buff now %d\n",
	 * 	autotune, rbuff, sbuff);
	 */

	if (autotune > 0) 
	    web100_setbuff(recvsfd, agent, autotune, family, debug);

	/* ok, We are now read to start the throughput tests.  First
	 * the client will stream data to the server for 10 seconds.
	 * Data will be processed by the read loop below.
	 */

/* experimental code, delete when finished */
	web100_var *LimRwin, *yar;
	u_int32_t limrwin_val;
	char yuff[32];
	
	if (limit > 0) {
	    fprintf(stderr, "Setting Cwnd limit - ");
	    if ((conn = web100_connection_from_socket(agent, recvsfd)) != NULL) {
		web100_agent_find_var_and_group(agent, "CurMSS", &group, &yar);
		web100_raw_read(yar, conn, yuff);
		fprintf(stderr, "MSS = %s, multiplication factor = %d\n",
			web100_value_to_text(web100_get_var_type(yar), yuff), limit);
		limrwin_val = limit * (atoi(web100_value_to_text(web100_get_var_type(yar), yuff)));
		web100_agent_find_var_and_group(agent, "LimRwin", &group, &LimRwin);
		fprintf(stderr, "now write %d to limit the Receive window", limrwin_val);
		web100_raw_write(LimRwin, conn, &limrwin_val);
		fprintf(stderr, "  ---  Done\n");
	    }
	}

/* End of test code */

	write(ctlsockfd, "Start-c2s-test", 15);
	alarm(45);  /* reset alarm() again, this 10 sec test should finish before this signal
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

	/* while((n = read(recvsfd, buff, sizeof(buff))) > 0)
	 *       bytes += n;
	 */
	t = secs()-t;
	c2sspd = (8.e-3 * bytes) / t;
	sprintf(buff, "%6.0f Kbs outbound", c2sspd);
	if (debug > 0)
	    fprintf(stderr, "%s\n", buff);
	/* get receiver side Web100 stats and write them to the log file */
	if (record_reverse == 1)
	        web100_get_data_recv(recvsfd, agent, LogFileName, count_vars, family, debug);
	shutdown(recvsfd, SHUT_RD);
	close(recvsfd);
	close(sock2);

	/* Next send speed-chk a flag to retrieve the data it collected.
	 * Skip this step if speed-chk isn't running.
	 */

	if (getuid() == 0) {
	    if (debug > 0)
	        fprintf(stderr, "Signal USR1(%d) sent to child [%d]\n", SIGUSR1, mon_pid1);
	    kill(mon_pid1, SIGUSR1);
	    FD_ZERO(&rfd);
	    FD_SET(mon_pipe1[0], &rfd);
	    sel_tv.tv_sec = 1;
	    sel_tv.tv_usec = 100000;
	read3:
	    if ((select(32, &rfd, NULL, NULL, &sel_tv)) > 0) {
                if ((ret = read(mon_pipe1[0], spds[spd_index], 128)) < 0)
	    	    sprintf(spds[spd_index], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0");
	        if (debug > 0) {
	            fprintf(stderr, "%d bytes read ", ret);
	            fprintf(stderr, "'%s' from monitor pipe\n", spds[spd_index]);
		}
	        spd_index++;
                if ((ret = read(mon_pipe1[0], spds[spd_index], 128)) < 0)
	    	    sprintf(spds[spd_index], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0");
	        if (debug > 0)
	            fprintf(stderr, "%d bytes read '%s' from monitor pipe\n", ret, spds[spd_index]);
	        spd_index++;
	    } else {
		if (errno == EINTR)
		    goto read3;
		if (debug > 3)
		    fprintf(stderr, "Failed to read pkt-pair data from C2S flow, reason = %d\n", errno);
	        sprintf(spds[spd_index++], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0");
	        sprintf(spds[spd_index++], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0");
	    }
	} else {
	    sprintf(spds[spd_index++], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0");
	    sprintf(spds[spd_index++], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0");
	}

/* old v4 only code */
/*
 * 	if ( (sock3 = socket(AF_INET, SOCK_STREAM, 0)) < 0)
 * 		err_sys("server: can't open stream socket");
 * 	setsockopt(sock3, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
 * 	bzero((char *) &srv3_addr, sizeof(srv3_addr));
 * 	srv3_addr.sin_family = AF_INET;
 * 	srv3_addr.sin_addr.s_addr = htonl(INADDR_ANY);
 *  
 *  	if (bind(sock3, (struct sockaddr *) &srv3_addr, sizeof(serv_addr)) < 0)
 *  		err_sys("server: can't bind local address");
 */

        if ((sock3 = socket(family, SOCK_STREAM, 0)) < 0)
            err_sys("Server Failed: cannot open socket");

        if (family == AF_INET) {
            bzero((char *) &srv3_addr, sizeof(srv3_addr));
            srv3_addr.sin_family = AF_INET;
            srv3_addr.sin_addr.s_addr = htonl(INADDR_ANY);
            if (multiple == 1)
                srv3_addr.sin_port = 0;  /* any */
            else
                srv3_addr.sin_port = htons(port3);
            setsockopt(sock3, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
            if (bind(sock3, (struct sockaddr *) &srv3_addr, sizeof(srv3_addr)) < 0)
                    err_sys("Server fault: (Test engine port3) can't bind to IPv4 address");
            }
        else if (family == AF_INET6) {
            bzero((char *) &serv6_addr, sizeof(serv6_addr));
            if (multiple == 1)
                srv63_addr.sin6_port = 0;  /* any */
            else
                srv63_addr.sin6_port = htons(port3);
            setsockopt(sock3, IPPROTO_IPV6, IPV6_V6ONLY, &one, sizeof(one));
            setsockopt(sock3, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
            if (bind(sock3, (struct sockaddr *) &srv63_addr, sizeof(srv63_addr)) < 0)
                err_sys("Server fault: (Test engine port3) can't bind to IPv6 address");
        }

	if (set_buff > 0) {
	    setsockopt(sock3, SOL_SOCKET, SO_SNDBUF, &window, sizeof(window));
	    setsockopt(sock3, SOL_SOCKET, SO_RCVBUF, &window, sizeof(window));
	}
	if (autotune > 0) {
	    setsockopt(sock3, SOL_SOCKET, SO_SNDBUF, &largewin, sizeof(largewin));
	    setsockopt(sock3, SOL_SOCKET, SO_RCVBUF, &largewin, sizeof(largewin));
	}
	if (debug > 0)
	    fprintf(stderr, "listening for Inet connection on sock3, fd=%d\n", sock3);
	listen(sock3, 2); 

	/* Data received from speed-chk, tell applet to start next test */
	write(ctlsockfd, "Open-s2c-connection", 21);

	/* ok, await for connect on 3rd port
	 * This is the second throughput test, with data streaming from
	 * the server back to the client.  Again stream data for 10 seconds.
	 */
	if (debug > 0)
	    fprintf(stderr, "waiting for data on sock3\n");

	j = 0;
	for (;;) {
	    if ((xmitsfd = accept(sock3, (struct sockaddr *) &cli_addr, &clilen)) > 0)
		break;
	
	    sprintf(tmpstr, "-------     S2C connection setup returned because (%d)", errno);
	    if (debug > 1)
		perror(tmpstr);
	    if (++j == 4)
		break;
	} 

	if (xmitsfd > 0) {
	    if (getuid() == 0) {
	        pipe(mon_pipe2);
	        if ((mon_pid2 = fork()) == 0) {
		    close(ctlsockfd);
		    close(sock3);
		    close(xmitsfd);
		    if (debug > 4) {
		        fprintf(stderr, "S2C test Child thinks pipe() returned fd0=%d, fd1=%d\n",
				    mon_pipe2[0], mon_pipe2[1]);
		    }
		    if (debug > 1)
		        fprintf(stderr, "S2C test calling init_pkttrace() with pd=0x%x\n", cli_addr);
		    if (debug > 1)
		        fprintf(stderr, "S2C test calling init_pkttrace() with pd=0x%x\n", cli_addr);
	            init_pkttrace(&cli_addr, NULL, mon_pipe2, device, family);
		    exit(0);		/* Packet trace finished, terminate gracefully */
	        }
	        if (read(mon_pipe2[0], tmpstr, 128) <= 0) {
	            printf("error & exit");
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
	    /* autotune = web100_autotune(xmitsfd, agent, family, debug);
	     * if (debug > 3)
	     *     fprintf(stderr, "S2C Test Autotune = %d Recv Buff now %d, Send Buff now %d\n",
	     * 	    autotune, rbuff, sbuff);
	     */

	    if (autotune > 0) 
		web100_setbuff(xmitsfd, agent, autotune, family, debug);

	    /* write(ctlsockfd, buff, strlen(buff));   */

/* experimental code, delete when finished */
	web100_var *LimCwnd;
	u_int32_t limcwnd_val;
	
	if (limit > 0) {
	    fprintf(stderr, "Setting Cwnd Limit\n");
	    if ((conn = web100_connection_from_socket(agent, xmitsfd)) != NULL) {
		web100_agent_find_var_and_group(agent, "CurMSS", &group, &yar);
		web100_raw_read(yar, conn, yuff);
		web100_agent_find_var_and_group(agent, "LimCwnd", &group, &LimCwnd);
		limcwnd_val = limit * (atoi(web100_value_to_text(web100_get_var_type(yar), yuff)));
		web100_raw_write(LimCwnd, conn, &limcwnd_val);
	    }
	}

/* End of test code */

	    if (experimental > 0) {
		sprintf(logname, "snaplog-%s.%d", inet_ntoa(cli_addr.sin_addr),
					ntohs(cli_addr.sin_port));
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

	    write(ctlsockfd, "Start-s2c-test", 15);
	    alarm(30);  /* reset alarm() again, this 10 sec test should finish before this signal
	        	 * is generated.  */
	    t = secs();
	    s = t + 10.0;
	/* 
         *    if (experimental > 0) {
         *        web100_snap(snap);
         *        if (experimental > 1)
         *            web100_log_write(log, snap);
         *        web100_agent_find_var_and_group(agent, "SndNxt", &group, &var);
         *        web100_snap_read(var, snap, tmpstr);
         *        SndMax = atoi(web100_value_to_text(web100_get_var_type(var), tmpstr));
         *        web100_agent_find_var_and_group(agent, "SndUna", &group, &var);
         *        web100_snap_read(var, snap, tmpstr);
         *        SndUna = atoi(web100_value_to_text(web100_get_var_type(var), tmpstr));
         *    }
	 */
 
	    int c3=0;
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
	        if (n < 0 )
		    break;
	        bytes += n;

                if (experimental > 0) {
                    c2++;
                }
            }

            shutdown(xmitsfd, SHUT_WR);  /* end of write's */
            s = secs() - t;
            x2cspd = (8.e-3 * bytes) / s;
            if (experimental > 0) {
                web100_snapshot_free(snap);
                if (experimental > 1)
                    web100_log_close_write(log);
	    }

	    /* printf("debug: xmit socket fd = %d\n", xmitsfd); */
	      /* web100_get_data(xmitsfd, ctlsockfd, agent, count_vars, family, debug);  */
	    web100_snap(rsnap);
	    web100_snap(tsnap);

	    if (debug > 0) {
	        fprintf(stderr, "sent %d bytes to client in %0.2f seconds\n", bytes, t);
		fprintf(stderr, "Buffer control counters Total = %d, new data = %d, Draining Queue = %d\n", c3, c2, c1);
	    }
	    /* Next send speed-chk a flag to retrieve the data it collected.
	     * Skip this step if speed-chk isn't running.
	     */

	    if (getuid() == 0) {
	        if (debug > 0)
	            fprintf(stderr, "Signal USR2(%d) sent to child [%d]\n", SIGUSR2, mon_pid2);
	        kill(mon_pid2, SIGUSR2);
	        FD_ZERO(&rfd);
	        FD_SET(mon_pipe2[0], &rfd);
	        sel_tv.tv_sec = 1;
	        sel_tv.tv_usec = 100000;
	read2:
	        if (ret = (select(mon_pipe2[0]+1, &rfd, NULL, NULL, &sel_tv)) > 0) {
                    if ((ret = read(mon_pipe2[0], spds[spd_index], 128)) < 0)
	                sprintf(spds[spd_index], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0");
	            if (debug > 0)
	                fprintf(stderr, "Read '%s' from monitor pipe\n", spds[spd_index]);
	            spd_index++;
                    if ((ret = read(mon_pipe2[0], spds[spd_index], 128)) < 0)
	                sprintf(spds[spd_index], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0");
	            if (debug > 0)
	                fprintf(stderr, "Read '%s' from monitor pipe\n", spds[spd_index]);
	            spd_index++;
		} else {
		    if (debug > 3)
			fprintf(stderr, "Failed to read pkt-pair data from S2C flow, retcode=%d, reason=%d\n", ret, errno);
		    if (errno == EINTR)
			goto read2;
	            sprintf(spds[spd_index++], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0");
	            sprintf(spds[spd_index++], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0");
	        }
	    } else {
	        sprintf(spds[spd_index++], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0");
	        sprintf(spds[spd_index++], " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0");
	    }

	    if (debug > 0)
	        fprintf(stderr, "%6.0f Kbps inbound\n", x2cspd);
	}

	alarm(30);  /* reset alarm() again, this 10 sec test should finish before this signal
		     * is generated.  */
	printf("debug: xmit socket fd = %d\n", xmitsfd);
	ret = web100_get_data(tsnap, ctlsockfd, agent, count_vars, family, debug);
	web100_snapshot_free(tsnap);
	ret = web100_get_data(rsnap, ctlsockfd, agent, count_vars, family, debug);
	web100_snapshot_free(rsnap);

	/* shutdown(xmitsfd, SHUT_WR); */
	/* end of write's */
	/* now when client closes other end, read will fail */
	/* read(xmitsfd, buff, 1);  */ /* read fail/complete */ 
	read(ctlsockfd, buff, 32);  
	s2cspd = atoi(buff);

	if (debug > 3)
	    fprintf(stderr, "Finished testing C2S = %0.2f Mbps, S2C = %0.2f Mbps\n",
			    c2sspd/1000, s2cspd/1000);
	for (n=0; n<spd_index; n++) {
	    sscanf(spds[n], "%d %d %d %d %d %d %d %d %d %d %d %d %0.2f %d %d %d %d %d", &links[0],
		    &links[1], &links[2], &links[3], &links[4], &links[5], &links[6],
		    &links[7], &links[8], &links[9], &links[10], &links[11], &runave,
		    &inc_cnt, &dec_cnt, &same_cnt, &timeout, &dupack);
	    max = 0;
	    index = 0;
	    total = 0;
	    for (j=0; j<10; j++) {
		total += links[j];
		if (max < links[j]) {
		    max = links[j];
		    index = j;
		}
	    }
	    if (links[index] == -1)
		index = -1;
            fp = fopen(LogFileName,"a");
	    if (fp == NULL)
		fprintf(stderr, "Unable to open log file '%s', continuing on without logging\n",
		    LogFileName);
	    else {
		fprintf(fp, "spds[%d] = '%s' max=%d [%0.2f%%]\n", n, spds[n],
		    max, (float) max*100/total);
		fclose(fp);
	    }
  
	    switch (n) {
		case 0: c2sdata = index;
		    if (debug > 0)
			fprintf(stderr, "Client --> Server data detects link = ");
		    break;
		case 1: c2sack = index;
		    if (debug > 0)
			fprintf(stderr, "Client <-- Server Ack's detect link = ");
		    break;
		case 2: s2cdata = index;
		    if (debug > 0)
			fprintf(stderr, "Server --> Client data detects link = ");
		    break;
		case 3: s2cack = index;
		    if (debug > 0)
			fprintf(stderr, "Server <-- Client Ack's detect link = ");
	    }
	    if (debug > 0) {
	        switch (index) {
		    case -1: fprintf(stderr, "System Fault\n");
			     break;
		    case 0: fprintf(stderr, "RTT\n");
		    	    break;
		    case 1: fprintf(stderr, "Dial-up\n");
		    	    break;
		    case 2: fprintf(stderr, "T1\n");
		    	    break;
		    case 3: fprintf(stderr, "Ethernet\n");
		    	    break;
		    case 4: fprintf(stderr, "T3\n");
		    	    break;
		    case 5: fprintf(stderr, "FastEthernet\n");
		    	    break;
		    case 6: fprintf(stderr, "OC-12\n");
			    break;
		    case 7: fprintf(stderr, "Gigabit Ethernet\n");
			    break;
		    case 8: fprintf(stderr, "OC-48\n");
			    break;
		    case 9: fprintf(stderr, "10 Gigabit Enet\n");
			    break;
		    case 10: fprintf(stderr, "Retransmissions\n");
	        }
	    }
	}

	if (getuid() == 0) {
	    write(mon_pipe1[1], "", 1);
	    close(mon_pipe1[0]);
	    close(mon_pipe1[1]);
	    write(mon_pipe2[1], "", 1);
	    close(mon_pipe2[0]);
	    close(mon_pipe2[1]);
	}
	/* close(ctlsockfd); */

	/* get some web100 vars */
	/* printf("Going to get some web100 data\n"); */
	if (experimental > 2) {
		dec_cnt = inc_cnt = same_cnt = 0;
		CwndDecrease(agent, logname, &dec_cnt, &same_cnt, &inc_cnt, debug);
		if (debug > 1)
			fprintf(stderr, "####### decreases = %d, increases = %d, no change = %d\n",
				dec_cnt, inc_cnt, same_cnt);
	}

	web100_logvars(&Timeouts, &SumRTT, &CountRTT,
		&PktsRetrans, &FastRetran, &DataPktsOut,
		&AckPktsOut, &CurrentMSS, &DupAcksIn,
		&AckPktsIn, &MaxRwinRcvd, &Sndbuf,
		&CurrentCwnd, &SndLimTimeRwin, &SndLimTimeCwnd,
		&SndLimTimeSender, &DataBytesOut,
		&SndLimTransRwin, &SndLimTransCwnd,
		&SndLimTransSender, &MaxSsthresh,
		&CurrentRTO, &CurrentRwinRcvd, &MaxCwnd, &CongestionSignals,
		&PktsOut, &MinRTT, count_vars, &RcvWinScale, &SndWinScale,
		&CongAvoid, &CongestionOverCount, &MaxRTT, &OtherReductions,
		&CurTimeoutCount, &AbruptTimeouts, &SendStall, &SlowStart, 
		&SubsequentTimeouts, &ThruBytesAcked);

	/* if (rc == 0) { */
	    /* Calculate some values */
	    avgrtt = (double) SumRTT/CountRTT;
	    rttsec = avgrtt * .001;
	    /* loss = (double)(PktsRetrans- FastRetran)/(double)(DataPktsOut-AckPktsOut); */
	    loss2 = (double)CongestionSignals/PktsOut;
	    if (loss2 == 0)
		if (c2sdata > 5)
		    loss2 = .0000000001;	/* set to 10^-10 for links faster than FastE */
		else
    		    loss2 = .000001;		/* set to 10^-6 for now */

	    oo_order = (double)DupAcksIn/AckPktsIn;
	    bw2 = (CurrentMSS / (rttsec * sqrt(loss2))) * 8 / 1024 / 1024;

	    if ((SndWinScale > 15) || (Sndbuf < 65535))
		SndWinScale = 0;
	    if ((RcvWinScale > 15) || (MaxRwinRcvd < 65535))
		RcvWinScale = 0;
	    /* MaxRwinRcvd <<= RcvWinScale; */
	    /* if ((SndWinScale > 0) && (RcvWinScale > 0))
	     *    Sndbuf = (64 * 1024) << RcvWinScale;
	     */
	    /* MaxCwnd <<= RcvWinScale; */

	    rwin = (double)MaxRwinRcvd * 8 / 1024 / 1024;
	    swin = (double)Sndbuf * 8 / 1024 / 1024;
	    cwin = (double)MaxCwnd * 8 / 1024 / 1024;

	    totaltime = SndLimTimeRwin + SndLimTimeCwnd + SndLimTimeSender;
	    rwintime = (double)SndLimTimeRwin/totaltime;
	    cwndtime = (double)SndLimTimeCwnd/totaltime;
	    sendtime = (double)SndLimTimeSender/totaltime;
	    timesec = totaltime/1000000;
	    RTOidle = (Timeouts * ((double)CurrentRTO/1000))/timesec;
	    tmouts = (double)Timeouts / PktsOut;
	    rtran = (double)PktsRetrans / PktsOut;
	    acks = (double)AckPktsIn / PktsOut;
	    dack = (double)DupAcksIn / (double)AckPktsIn;

	    spd = ((double)DataBytesOut / (double)totaltime) * 8;
	    waitsec = (double) (CurrentRTO * Timeouts)/1000;
	    if (debug > 1) {
		fprintf(stderr, "CWND limited test = %0.2f while unlimited = %0.2f\n", s2c2spd, s2cspd);
		if (s2c2spd > s2cspd) 
			fprintf(stderr, "Better throughput when CWND is limited, may be duplex mismatch\n");
		else
			fprintf(stderr, "Better throughput without CWND limits - normal operation\n");
	    }


	    /* remove the following line when the new detection code is ready for release */
	    old_mismatch = 1;
	    if (old_mismatch == 1) {
	        if ((cwndtime > .9) && (bw2 > 2) && (PktsRetrans/timesec > 2) &&
	            (MaxSsthresh > 0) && (RTOidle > .01) && (link > 2))
  	        {	if (s2cspd < c2sspd)
			    mismatch = 1;
			else
			    mismatch = 2;
    		        link = 0;
  	        }

	        /* test for uplink with duplex mismatch condition */
	        if (((s2cspd/1000) > 50) && (spd < 5) && (rwintime > .9) && (loss2 < .01)) {
    		    mismatch = 2;
    		    link = 0;
	        }
	    } else {
		/* This section of code is being held up for non-technical reasons.
		 * Once those issues are resolved a new mismatch detection algorim 
		 * will be placed here.
		 *  RAC 5-11-06
		 */
	    }

	    /* estimate is less than throughput, something is wrong */
	    if (bw2 < spd)
  		link = 0;

	    if (((loss2*100)/timesec > 15) && (cwndtime/timesec > .6) &&
	        (loss2 < .01) && (MaxSsthresh > 0))
  		    bad_cable = 1;

	    /* test for Ethernet link (assume Fast E.) */
	    /* if ((bw2 < 25) && (loss2 < .01) && (rwin/rttsec < 25) && (link > 0)) */
	    if ((spd < 9.5) && (spd > 3.0) && ((s2cspd/1000) < 9.5) &&
	        (loss2 < .01) && (oo_order < .035) && (link > 0))
  		    link = 10;

	    /* test for wireless link */
	    if ((sendtime == 0) && (spd < 5) && (bw2 > 50) &&
	        ((SndLimTransRwin/SndLimTransCwnd) == 1) &&
	        (rwintime > .90) && (link > 0))
  		    link = 3;

	    /* test for DSL/Cable modem link */
	    /* if ((sendtime == 0) && (SndLimTransSender == 0) && (spd < 2) && */
	    if ((SndLimTimeSender < 600) && (SndLimTransSender == 0) && (spd < 2) &&
	        (spd < bw2) && (link > 0))
  		    link = 2;

	    /* if (((rwintime > .95) && (SndLimTransRwin/timesec > 30) &&
	     *    (SndLimTransSender/timesec > 30)) || (link <= 10))
	     */
	    if ((rwintime > .95) && (SndLimTransRwin/timesec > 30) &&
	        (SndLimTransSender/timesec > 30))
  		    half_duplex = 1;

	    if ((cwndtime > .02) && (mismatch == 0) && ((cwin/rttsec) < (rwin/rttsec)))
    		    congestion = 1;
	/* }     */
    
	sprintf(buff, "c2sData: %d\nc2sAck: %d\ns2cData: %d\ns2cAck: %d\n",
		c2sdata, c2sack, s2cdata, s2cack);
	write(ctlsockfd, buff, strlen(buff));
	/* fprintf(stderr, "%s", buff); */
	sprintf(buff, "half_duplex: %d\nlink: %d\ncongestion: %d\nbad_cable: %d\nmismatch: %d\nspd: %0.2f\n",
		half_duplex, link, congestion, bad_cable, mismatch, spd);
	write(ctlsockfd, buff, strlen(buff));

	sprintf(buff, "bw: %0.2f\nloss: %0.9f\navgrtt: %0.2f\nwaitsec: %0.2f\ntimesec: %0.2f\norder: %0.4f\n",
		bw2, loss2, avgrtt, waitsec, timesec, oo_order);
	write(ctlsockfd, buff, strlen(buff));

	sprintf(buff, "rwintime: %0.4f\nsendtime: %0.4f\ncwndtime: %0.4f\nrwin: %0.4f\nswin: %0.4f\n",
		rwintime, sendtime, cwndtime, rwin, swin);
	write(ctlsockfd, buff, strlen(buff));

	sprintf(buff, "cwin: %0.4f\nrttsec: %0.6f\nSndbuf: %d\naspd: %0.5f\nCWND-Limited: %0.2f\n",
		cwin, rttsec, Sndbuf, aspd, s2c2spd);
	write(ctlsockfd, buff, strlen(buff));
	
       	fp = fopen(LogFileName,"a");
	if (fp == NULL)
	    fprintf(stderr, "Unable to open log file '%s', continuing on without logging\n",
			    LogFileName);
	else {
	    sprintf(date,"%15.15s", ctime(&stime)+4);
	    fprintf(fp, "%s,", date);
       	    fprintf(fp,"%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,",
      	        rmt_host,(int)s2c2spd,(int)s2cspd,(int)c2sspd, Timeouts, SumRTT, CountRTT, PktsRetrans,
 	        FastRetran, DataPktsOut, AckPktsOut, CurrentMSS, DupAcksIn, AckPktsIn);
       	    fprintf(fp,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,",
 	        MaxRwinRcvd, Sndbuf, MaxCwnd, SndLimTimeRwin, SndLimTimeCwnd,
 	        SndLimTimeSender, DataBytesOut, SndLimTransRwin, SndLimTransCwnd,
 	        SndLimTransSender, MaxSsthresh, CurrentRTO, CurrentRwinRcvd);
       	    fprintf(fp,"%d,%d,%d,%d,%d",
  	        link, mismatch, bad_cable, half_duplex, congestion);
	    fprintf(fp, ",%d,%d,%d,%d,%d,%d,%d,%d,%d", c2sdata, c2sack, s2cdata, s2cack,
		    CongestionSignals, PktsOut, MinRTT, RcvWinScale, autotune);
	    fprintf(fp, ",%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", CongAvoid, CongestionOverCount, MaxRTT, 
			OtherReductions, CurTimeoutCount, AbruptTimeouts, SendStall, SlowStart,
			SubsequentTimeouts, ThruBytesAcked);
       	    fclose(fp);
	}
	if (usesyslog == 1) {
       	    sprintf(logstr1,"client_IP=%s,c2s_spd=%2.0f,s2c_spd=%2.0f,Timeouts=%d,SumRTT=%d,CountRTT=%d,PktsRetrans=%d,FastRetran=%d,DataPktsOut=%d,AckPktsOut=%d,CurrentMSS=%d,DupAcksIn=%d,AckPktsIn=%d,",
      	        rmt_host, s2cspd, c2sspd, Timeouts, SumRTT, CountRTT, PktsRetrans,
 	        FastRetran, DataPktsOut, AckPktsOut, CurrentMSS, DupAcksIn, AckPktsIn);
       	    sprintf(logstr2,"MaxRwinRcvd=%d,Sndbuf=%d,MaxCwnd=%d,SndLimTimeRwin=%d,SndLimTimeCwnd=%d,SndLimTimeSender=%d,DataBytesOut=%d,SndLimTransRwin=%d,SndLimTransCwnd=%d,SndLimTransSender=%d,MaxSsthresh=%d,CurrentRTO=%d,CurrentRwinRcvd=%d,",
 	        MaxRwinRcvd, Sndbuf, MaxCwnd, SndLimTimeRwin, SndLimTimeCwnd,
 	        SndLimTimeSender, DataBytesOut, SndLimTransRwin, SndLimTransCwnd,
 	        SndLimTransSender, MaxSsthresh, CurrentRTO, CurrentRwinRcvd);
	    strcat(logstr1, logstr2);
       	    sprintf(logstr2,"link=%d,mismatch=%d,bad_cable=%d,half_duplex=%d,congestion=%d,c2sdata=%d,c2sack=%d,s2cdata=%d,s2cack=%d,CongestionSignals=%d,PktsOut=%d,MinRTT=%d,RcvWinScale=%d\n",
  	        link, mismatch, bad_cable, half_duplex, congestion,
 		c2sdata, c2sack, s2cdata, s2cack,
		CongestionSignals, PktsOut, MinRTT, RcvWinScale);
	    strcat(logstr1, logstr2);
	    syslog(LOG_FACILITY|LOG_INFO, "%s", logstr1);
	    closelog();
	    if (debug > 3)
	        fprintf(stderr, "%s", logstr1);
	}
	/* send web100 results for xmitsfd */
	close(xmitsfd); 
	close(sock3); 

	/* If the admin view is turned on then the client process is going to update these
	 * variables.  The need to be shipped back to the parent so the admin page can be
	 * updated.  Otherwise the changes are lost when the client terminates.
	 */
	if (admin_view == 1) {
	    totalcnt = calculate(date, SumRTT, CountRTT, CongestionSignals, PktsOut, DupAcksIn, AckPktsIn,
			CurrentMSS, SndLimTimeRwin, SndLimTimeCwnd, SndLimTimeSender,
			MaxRwinRcvd, CurrentCwnd, Sndbuf, DataBytesOut, mismatch, bad_cable,
			(int)c2sspd, (int)s2cspd, c2sdata, s2cack, 1, debug);
	    gen_html((int)c2sspd, (int)s2cspd, MinRTT, PktsRetrans, Timeouts,
			Sndbuf, MaxRwinRcvd, CurrentCwnd, mismatch, bad_cable, totalcnt,
			debug);
	}
	shutdown(ctlsockfd, SHUT_RDWR);

	/* printf("Saved data to log file\n"); */

}

main(argc, argv)
int	argc;
char	*argv[];
{
	int chld_pid, rc;
	int n, midsockfd, recvsfd, xmitsfd, ctlsockfd, clilen, servlen, one=1;
	int c, chld_pipe[2];
	int i, loopcnt;
	struct sockaddr_in /*cli_addr,*/ cli2_addr, srv2_addr, srv3_addr, srv4_addr, serv_addr;
	struct sockaddr_in6 serv6_addr, cli6_addr;
  struct sockaddr_storage cli_addr;
	struct sigaction new;
	web100_agent* agent;
	char view_string[256], *lbuf=NULL, *ctime();
	char buff[32], tmpstr[256], *tmp2str;
	FILE *fp;
	size_t lbuf_max=0;
	fd_set rfd;
	struct timeval sel_tv;
	struct ndtchild *tmp_ptr, *new_child;
	time_t tt;

	int v4only=0, v6only=0, af_family;
	struct addrinfo hints, *ai;
  I2Addr listenaddr = NULL;
  int listenfd;
  char* srcname = NULL;
  char* listenport = sPORT;

	opterr = 0;
	while ((c = getopt(argc, argv, "46adxhmoqrstvb:c:p:f:i:l:y:")) != -1){
	    switch (c) {
		case 'c':
			ConfigFileName = optarg;
			break;
		case 'd':
			debug++;
			break;
	    }
	}

	if (ConfigFileName == NULL)
	    ConfigFileName = CONFIGFILE;

	/*
	 * Load Config file.
	 * lbuf/lbuf_max keep track of a dynamically grown "line" buffer.
	 * (It is grown using realloc.)
	 * This will be used throughout all the config file reading and
	 * should be free'd once all config files have been read.
	 */

	opterr =  optind = 1;
	LoadConfig(&lbuf, &lbuf_max);
	debug = 0;

	while ((c = getopt(argc, argv, "46adxhmoqrstvb:c:p:f:i:l:y:")) != -1){
	    switch (c) {
		case '4':
			v4only = 1;
			v6only = 0;
			break;
		case '6':
			v6only = 1;
			v4only = 0;
			break;
		case 'r':
			record_reverse = 1;
			break;
		case 'h':
			printf("Usage: %s {options}\n", argv[0]);
			printf("\t-a \tGenerate administrator view html page\n");
			printf("\t-d \tprint additional diagnostic messages\n");
			printf("\t\tNote: add multiple d's (-ddd) for more details\n");
			printf("\t-h \tprint help message (this message)\n");
			printf("\t-m \tselect single or multi-test mode\n");
			printf("\t-o \tuse old Duplex Mismatch heuristic\n");
			printf("\t-q \tdisable FIFO queuing of client requests\n");
			printf("\t-r \trecord client to server Web100 variables\n");
			printf("\t-s \texport Web100 data via the syslog() LOG_LOCAL0 facility\n");
			printf("\t-t \twrite tcpdump formatted file to disk\n");
			printf("\t-x \tenable any experimental code\n");
			printf("\t-v \tprint version number\n");
			printf("\n");
			printf("\t-b buffer size \tSet TCP send/recv buffers to user value\n");
			printf("\t-f variable_FN \tspecify alternate 'web100_variables' file\n");
			printf("\t-i device \tspecify network interface (libpcap device)\n");
			printf("\t-l Log_FN \tspecify alternate 'web100srv.log' file\n");
			printf("\t-p port# \tspecify primary port number (default 3001)\n");
		case 'v':
			printf("ANL/Internet2 NDT version %s\n", VERSION);
			exit(0);
		case 'p':
			port = atoi(optarg);
			port2 = port + 1;
			port3 = port + 2;
			break;
		case 'a':
			admin_view = 1;
			break;
		case 'f':
			VarFileName = optarg;
			break;
		case 'i':
			device = optarg;
			break;
		case 'l':
			LogFileName = optarg;
			break;
		case 'o':
			old_mismatch = 1;
			break;
		case 'm':
			multiple = 1;
			break;
		case 'q':
			queue = 0;
			break;
		case 's':
			usesyslog = 1;
			break;
		case 't':
			dumptrace = 1;
			break;
		case 'b':
			if (optarg != NULL)
			    window = atoi(optarg);
			set_buff = 1;
			break;
		case 'x':
			experimental++;
			break;
		case 'd':
			debug++;
			break;
		case 'y':
			limit = atoi(optarg);
			break;
	    }
	}

	/* First check to see if program is running as root.  If not, then warn
	 * the user that link type detection is suspect.  Then downgrade effective
	 * userid to non-root user until needed.
	 */
	if (getuid() != 0) {
	    fprintf(stderr, "Warning: This program must be run as root to enable the Link Type");
	    fprintf(stderr, " detection algorithm.\nContinuing execution without this algorithm\n");
	}

	if (LogFileName == NULL) {
		sprintf(lgfn, "%s/%s", BASEDIR, LOGFILE);
		 LogFileName = lgfn; 
	}
	if (VarFileName == NULL) {
		sprintf(wvfn, "%s/%s", BASEDIR, WEB100_FILE);
		VarFileName = wvfn;
	}
	if (debug > 0) {
	    fprintf(stderr, "ANL/Internet2 NDT ver %s\n", VERSION);
	    fprintf(stderr, "\tVariables file = %s\n\tlog file = %s\n", VarFileName, LogFileName);
	    fprintf(stderr, "\tDebug level set to %d\n", debug);
	}

	memset(&new, 0, sizeof(new));
	new.sa_handler = cleanup;

	/* Grab all signals and run them through my cleanup routine.  2/24/05 */
	for (i=1; i<32; i++) {
	    if ((i == SIGKILL) || (i == SIGSTOP))
		continue;		/* these signals can't be caught */
	    sigaction(i, &new, NULL);
	}


	/*
	 * Bind our local address so that the client can send to us.
	 */
  if (srcname && !(listenaddr = I2AddrByNode(NULL, srcname))) {
    err_sys("server: Invalid source address specified: %s", srcname);
  }
  if ((listenaddr = CreateListenSocket(listenaddr, listenport, 0)) == NULL) {
    err_sys("server: CreateListenSocket failed");
  }
  listenfd = I2AddrFD(listenaddr);
  
  setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
  if (set_buff > 0) {
    setsockopt(listenfd, SOL_SOCKET, SO_SNDBUF, &window, sizeof(window));
    setsockopt(listenfd, SOL_SOCKET, SO_RCVBUF, &window, sizeof(window));
  }

	if (debug > 0)
	    fprintf(stderr, "server ready on port %d\n",port);

	  /* Initialize Web100 structures */
	count_vars = web100_init(VarFileName, debug);

	/* The administrator view automatically generates a usage page for the
	 * NDT server.  This page is then accessable to the general public.
	 * At this point read the existing log file and generate the necessary
	 * data.  This data is then updated at the end of each test.
	 * RAC 11/28/03
	 */
	if (admin_view == 1)
	    view_init(LogFileName, debug);

	/* create a log file entry every time the web100srv process starts up. */
	ndtpid = getpid();
        tt = time(0);
        fp = fopen(LogFileName,"a");
	if (fp == NULL)
	    fprintf(stderr, "Unable to open log file '%s', continuing on without logging\n",
			    LogFileName);
	else {
            fprintf(fp, "Web100srv (ver %s) process (%d) started %15.15s\n",
        	VERSION, ndtpid, ctime(&tt)+4);
            fclose(fp);
	}
	if (usesyslog == 1)
	    syslog(LOG_FACILITY|LOG_INFO, "Web100srv (ver %s) process started\n",
                VERSION);
	/*
	 * Wait at accept() for a new connection from a client process.
	 */

	/* These 2 flags keep track of running processes.  The 'testing' flag
	 * indicated a test is currently being performed.  The 'waiting' counter
	 * shows how many tests are pending.
	 * Rich Carlson 3/11/04
	 */

	testing = 0;
	waiting = 0;
	loopcnt = 0;
	head_ptr = NULL;
	sig17 = 0;
	for(;;){
		int i, ret;
		char *name;
		struct hostent *hp;

		/* if ((agent = web100_attach(WEB100_AGENT_TYPE_LOCAL, NULL)) == NULL) {
                 *    web100_perror("web100_attach");
                 *    return;
                 * }
		 */

		if (debug > 2)
		    if (head_ptr == NULL)
			fprintf(stderr, "nothing in queue\n");
		    else
		        fprintf(stderr, "Queue pointer = %d, testing = %d, waiting = %d\n",
			    head_ptr->pid, testing, waiting);

		if (sig17 == 1) {
		    child_sig();
		    sig17 = 0;
		}

		FD_ZERO(&rfd);
    FD_SET(listenfd, &rfd);
		if (waiting > 0) {
		    sel_tv.tv_sec = 3;
		    sel_tv.tv_usec = 0;
		    if (debug > 2)
			fprintf(stderr, "Waiting for new connection, timer running\n");
		    rc = select(listenfd+1, &rfd, NULL, NULL, &sel_tv);
		    tt = time(0);
		    if (head_ptr != NULL) {
		        if (debug > 2) 
			    fprintf(stderr, "now = %d Process started at %d, run time = %d\n", 
					tt, head_ptr->stime, (tt - head_ptr->stime));
		        if (tt - head_ptr->stime > 60) {
			    /* process is stuck at the front of the queue. */
        		    fp = fopen(LogFileName,"a");
			        if (fp != NULL) {
			        fprintf(fp, "%d children waiting in queue: Killing off stuck process %d at %15.15s\n", 
					waiting, head_ptr->pid, ctime(&tt)+4);
				fclose(fp);
			    }
			    tmp_ptr = head_ptr->next;
			    kill(head_ptr->pid, SIGKILL);
			    free(head_ptr);
			    head_ptr = tmp_ptr;
			    testing = 0;
			    if (waiting > 0)
				waiting--;
		        }
		    }
		}
		else {
		    if (debug > 2)
			fprintf(stderr, "Timer not running, waiting for new connection\n");
		    rc = select(listenfd+1, &rfd, NULL, NULL, NULL);
		}

		if (rc < 0) {
		    if (debug > 4)
		        fprintf(stderr, "Select exited with rc = %d\n", rc);
		    continue;
		}

		if (rc == 0) {		/* select exited due to timer expired */
		    if (debug > 2)
			fprintf(stderr, "Timer exired while waiting for a new connection\n");
		    if (multiple == 0) {
		        if ((waiting > 0) && (testing == 0))
		            goto ChldRdy;
		    }
		    continue;
		}
		/* if (FD_ISSET(sockfd, &rfd)) { */
		else {
		    if (multiple == 0) {
		        if ((waiting > 0) && (testing == 0))
		            goto ChldRdy;
		        if (debug > 2)
			    fprintf(stderr, "New connection received, waiting for accept() to complete\n");
		    }
		    clilen = sizeof(cli_addr);
        ctlsockfd = accept(listenfd, (struct sockaddr *) &cli_addr, &clilen);
        {
          int tmpstrlen = sizeof(tmpstr);
          /* FIXME: check if the I2Addr have to be released */
          I2AddrNodeName(I2AddrByLocalSockFD(NULL, ctlsockfd, False), tmpstr, &tmpstrlen);
          printf("A: %s, len=%d\n", tmpstr, tmpstrlen);
        }
        if (debug > 3)
          fprintf(stderr, "New connection received from [%s].\n", tmpstr);
        if (ctlsockfd < 0){
          if (errno == EINTR)
            continue; /*sig child */
          perror("Web100srv server: accept error");
          continue;
        }
		    new_child = (struct ndtchild *) malloc(sizeof(struct ndtchild));
                    tt = time(0);
                    /*
		     * hp = gethostbyaddr(&cli_addr.sin_addr, sizeof(struct in_addr), AF_INET);
                     * name =  (hp == NULL) ? inet_ntoa(cli_addr.sin_addr) : hp->h_name;
		     * rmt_host = inet_ntoa(cli_addr.sin_addr);
		     */

		    if ((getaddrinfo(tmpstr, NULL, NULL, &ai)) < 0) {
			perror("getaddrinfo() failed! ");
		    }
		    name = (ai->ai_canonname == NULL) ? tmpstr : ai->ai_canonname;
		    rmt_host = tmpstr;

		    /* At this point we have received a connection from a client, meaning that
		     * a test is being requested.  At this point we should apply any policy 
		     * or AAA functions to the incoming connection.  If we don't like the
		     * client, we can refuse the connection and loop back to the begining.
		     * There would need to be some additional logic installed if this AAA
		     * test relied on more than the client's IP address.  The client would
		     * also require modification to allow more credentials to be created/passed
		     * between the user and this application.
		     */
		}

		pipe(chld_pipe);
		chld_pid = fork();

		switch (chld_pid) {
		    case -1:		/* an error occured, log it and quit */
			fprintf(stderr, "fork() failed, errno = %d\n", errno);
			break;
		    default:		/* this is the parent process, handle scheduling and
					 * queuing of multiple incoming clients
					 */
			if (debug > 4) {
			    fprintf(stderr, "Parent process spawned child = %d\n", chld_pid);
			    fprintf(stderr, "Parent thinks pipe() returned fd0=%d, fd1=%d\n", chld_pipe[0],
					    chld_pipe[1]);
			}

			/* close(ctlsockfd); */
			close(chld_pipe[0]);
			if (multiple == 1)
			    goto multi_client;

			new_child->pid = chld_pid;
			strncpy(new_child->addr, rmt_host, strlen(rmt_host));
			strncpy(new_child->host, name, strlen(name));
			new_child->stime = tt + (waiting*45);
			new_child->qtime = tt;
			new_child->pipe = chld_pipe[1];
			new_child->ctlsockfd = ctlsockfd;
			new_child->family = ai->ai_family;
			new_child->next = NULL;

			if ((testing == 1) && (queue == 0)) {
			    if (debug > 2)
				fprintf(stderr, "queuing disabled and testing in progress, tell client no\n");
			    write(ctlsockfd, "9999", 4);
			    free(new_child);
			    continue;
			}

			waiting++;
			if (debug > 4)
			    fprintf(stderr, "Incrementing waiting variable now = %d\n", waiting);
			if (head_ptr == NULL)
			    head_ptr = new_child;
			else {
			    if (debug > 3)
				fprintf(stderr, "New request has arrived, adding request to queue list\n");
			    tmp_ptr = head_ptr;
			    while (tmp_ptr->next != NULL)
				tmp_ptr = tmp_ptr->next;
			    tmp_ptr->next = new_child;

			    if (debug > 3) {
				fprintf(stderr, "Walking scheduling queue\n");
				tmp_ptr = head_ptr;
				while (tmp_ptr != NULL) {
				    fprintf(stderr, "\tChild %d, host: %s [%s], next=0x%x\n", tmp_ptr->pid,
						    tmp_ptr->host, tmp_ptr->addr, tmp_ptr->next);
				    if (tmp_ptr->next == NULL)
					break;
				    tmp_ptr = tmp_ptr->next;
				}
			    }
			}

			/* At this point send a message to the client via the ctlsockfd
			 * saying that N clients are waiting in the queue & testing will
			 * begin within Nx60 seconds.  Only if (waiting > 0)
			 * Clients who leave will be handled in the run_test routine.
			 */
			if (waiting > 1) {
			    if (debug > 2)
				fprintf(stderr, "%d clients waiting, telling client %d testing will begin within %d minutes\n",
					(waiting-1), tmp_ptr->pid, (waiting-1));

			    sprintf(tmpstr, "%d", (waiting-1));
			    write(ctlsockfd, tmpstr, strlen(tmpstr));
			}
			if (testing == 1) 
			    continue;

		      ChldRdy:
			testing = 1;

			/* update all the clients so they have some idea that progress is occurring */
			if (waiting > 1) {
			    i = waiting - 1;
			    tmp_ptr = head_ptr->next;
			    while (tmp_ptr != NULL) {
			        if (debug > 2)
				    fprintf(stderr, "%d clients waiting, updating client %d testing will begin within %d minutes\n",
					(waiting-1), tmp_ptr->pid, (waiting-i));

			        sprintf(tmpstr, "%d", (waiting-i));
			        write(tmp_ptr->ctlsockfd, tmpstr, strlen(tmpstr));
				tmp_ptr = tmp_ptr->next;
				i--;
			    }
			}
			if (debug > 2)
			    fprintf(stderr, "Telling client %d testing will begin now\n",
					head_ptr->pid);

			/* at this point we have successfully set everything up and are ready to
			 * start testing.  The write() on the control socket tells the client
			 * that the applet should drop out of the wait loop and get ready to begin
			 * testing.  The write() on the pipe tells the child process it's at the
			 * head of the queue, or it's in multi-client mode, and its OK for this 
			 * client process to wake up and begin testing.
			 *
			 * This is the point in time where we can/should contact any meta
			 * scheduler to verify that it is OK to really start a test.  If
			 * we are sharing the server host with other applications, and we
			 * want unfettered access to the link, now is the time to make this
			 * request.
			 */

		      head_ptr->stime = time(0);
		      sprintf(tmpstr, "go%d", head_ptr->family);
		      multi_client:
			if (multiple == 1) {
			    write(ctlsockfd, "0", 1);
			    write(chld_pipe[1], tmpstr, 3);
			    close(chld_pipe[1]);
			    close(ctlsockfd);
			}
			else {
			    write(head_ptr->ctlsockfd, "0", 1);
			    write(head_ptr->pipe, tmpstr, 3);
			    close(head_ptr->pipe);
			    close(head_ptr->ctlsockfd);
			}
			/* web100_detach(agent); */
			continue;
			break;
		    case 0:		/* this is the child process, it handles
					 * the testing function 
					 */
			if (debug > 3) {
			    fprintf(stderr, "Child thinks pipe() returned fd0=%d, fd1=%d for pid=%d\n",
					    chld_pipe[0], chld_pipe[1], chld_pid);
			}
			int j;
      close(listenfd);
			close(chld_pipe[1]);
			if ((agent = web100_attach(WEB100_AGENT_TYPE_LOCAL, NULL)) == NULL) {
	                    web100_perror("web100_attach");
	                    return;
	                }
		
			/* This is the child process from the above fork().  The parent
			 * is in control, and will send this child a signal when it gets
			 * to the head of the testing queue.  Post a read() and simply
			 * wait for the parent to let us know it's time to move on.
			 * Rich Carlson 3/11/04
			 */
			for (;;) {
			    read(chld_pipe[0], buff, 32);
			    if (strncmp(buff, "go", 2) == 0) {
				if (debug > 5)
				    fprintf(stderr, "Got 'go' signal from parent, ready to start testing\n");
				break;
			    }
			}
		
			/* write the incoming connection data into the log file */
                	fp = fopen(LogFileName,"a");
			if (fp == NULL)
			    fprintf(stderr, "Unable to open log file '%s', continuing on without logging\n",
					    LogFileName);
			else {
                	    fprintf(fp,"%15.15s  %s port %d\n",
                  		ctime(&tt)+4, name, 666/*FIXME: ntohs(cli_addr.sin_port)*/);
                	    fclose(fp);
			}
			close(chld_pipe[0]);
			buff[0] = buff[2];
			buff[1] = buff[3];
			/* alarm(60);   die in 60 seconds */
			alarm(30);  /* die in 30 seconds, but only if a test doesn't get started 
				     * reset alarm() before every test */

			run_test(agent, ctlsockfd, atoi(buff));
		
			if (debug > 2)
			    fprintf(stderr, "Successfully returned from run_test() routine\n");
			close(ctlsockfd);
			web100_detach(agent);
			exit(0);
			break;
		}
	}
}
