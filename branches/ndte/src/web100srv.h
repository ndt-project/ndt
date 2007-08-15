/* 
 * Start of process to break the web100srv.c file into several
 * pieces.  This should make maintenance easier.  Define
 * the global variables here.
 *
 * Rich Carlson 3/10/04
 * rcarlson@interenet2.edu
 */

#ifndef _JS_WEB100_H
#define _JS_WEB100_H

#include "../config.h"

#define   _USE_BSD
#include  <stdio.h>
#include  <netdb.h>
#include  <signal.h>
#ifdef HAVE_LIBWEB100
#include  <web100.h>
#endif
#ifdef HAVE_LIBPCAP
#include  <pcap.h>
#endif
#include  <stdlib.h>
#include  <string.h>
#include  <unistd.h>
#include  <fcntl.h>
#include  <getopt.h>

#include  <sys/types.h>
#include  <sys/socket.h>
#include  <sys/un.h>
#include  <sys/errno.h>
#include  <sys/select.h>
#include  <sys/resource.h>
#include  <sys/wait.h>
#include  <sys/time.h>

#include  <netinet/in.h>
#include  <netinet/tcp.h>
#include  <netinet/ip.h>
#ifdef HAVE_NETINET_IP6_H
#include  <netinet/ip6.h>
#endif
#ifdef HAVE_NET_ETHERNET_H
#include  <net/ethernet.h>
#endif
#include  <arpa/inet.h>

/* move version to configure.ac file for package name */
/* #define VERSION   "3.0.7" */		/* version number */
#define RECLTH    8192

#define WEB100_VARS 128			/* number of web100 variables you want to access*/
#define WEB100_FILE "web100_variables"  /* names of the variables to access*/
#define LOG_FACILITY LOG_LOCAL0		/* Syslog facility to log at */


/* Location of default config file */
#define CONFIGFILE  "/etc/ndt.conf"

/* hard-coded port values */
#define PORT  "3001"
#define PORT2 "3002"
#define PORT3 "3003"
#define PORT4 "3003"

typedef struct CwndPeaks {
    int min;
    int max;
    int amount;
} CwndPeaks;

typedef struct options {
    u_int32_t limit;
    int snapDelay;
    char avoidSndBlockUp;
    char snaplog;
    char cwndDecrease;
    char logname[128];
} Options;

typedef struct portpair {
  int port1;
  int port2;
} PortPair;

struct ndtchild {
	int pid;
	char addr[64];
	char host[256];
	time_t stime;
	time_t qtime;
	int pipe;
	int ctlsockfd;
	struct ndtchild *next;
};

struct spdpair {
  int family;
#if defined(AF_INET6)
	u_int32_t saddr[4];
	u_int32_t daddr[4];
#else
	u_int32_t saddr;
	u_int32_t daddr;
#endif
	u_int16_t sport;
	u_int16_t dport;
	u_int32_t seq;
	u_int32_t ack;
	u_int32_t win;
	int links[16];
	u_int32_t sec;
	u_int32_t usec;
	u_int32_t st_sec;
	u_int32_t st_usec;
	u_int32_t inc_cnt;
	u_int32_t dec_cnt;
	u_int32_t same_cnt;
	u_int32_t timeout;
	u_int32_t dupack;
	double time;
	double totalspd;
	double totalspd2;
	u_int32_t totalcount;
};

struct spdpair fwd, rev;

struct web100_variables {
	char name[256];
	char value[256];
} web_vars[WEB100_VARS];

struct pseudo_hdr {   /* used to compute TCP checksum */
    unsigned long s_addr;
    unsigned long d_addr;
    char pad;
    unsigned char protocol;
    unsigned short len;
};

int32_t gmt2local(time_t);

/* web100-pcap */
#ifdef HAVE_LIBPCAP
void init_vars(struct spdpair *cur);
void print_bins(struct spdpair *cur, int monitor_pipe[2]);
void calculate_spd(struct spdpair *cur, struct spdpair *cur2, int port2, int port3);
void init_pkttrace(struct sockaddr *sock_addr, socklen_t saddrlen, int monitor_pipe[2],
    char *device, PortPair* pair);
int check_signal_flags();
#endif

/* web100-util */
#ifdef HAVE_LIBWEB100
int web100_init(char *VarFileName);
int web100_autotune(int sock, web100_agent* agent, web100_connection* cn);
void web100_middlebox(int sock, web100_agent* agent, web100_connection* cn, char *results);
int web100_setbuff(int sock, web100_agent* agent, web100_connection* cn, int autotune);
void web100_get_data_recv(int sock, web100_agent* agent, web100_connection* cn, int count_vars);
int web100_get_data(web100_snapshot* snap, int ctlsock, web100_agent* agent, int count_vars);
int CwndDecrease(web100_agent* agent, char* logname, int *dec_cnt, int *same_cnt, int *inc_cnt);
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
    int *SubsequentTimeouts, int *ThruBytesAcked);
#endif
int KillHung(void);

#endif
