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

#define   _USE_BSD
#include  <stdio.h>
#include  <netdb.h>
#include  <signal.h>
#include  <web100.h>
#include  <pcap.h>
#include  <stdlib.h>
#include  <string.h>
#include  <unistd.h>
#include  <fcntl.h>
#include  <syslog.h>
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
#include  <netinet/ip6.h>
#include  <net/ethernet.h>
#include  <arpa/inet.h>


/* move version to configure.ac file for package name */
/* #define VERSION   "3.0.7" */		/* version number */
#define BUFFSIZE  8192
#define RECLTH    8192

#define LOGFILE "web100srv.log"		/* Name of log file */
#define WEB100_VARS 128			/* number of web100 variables you want to access*/
#define WEB100_FILE "web100_variables"  /* names of the variables to access*/
#define LOG_FACILITY LOG_LOCAL0		/* Syslog facility to log at */

/* this file needs to be fixed, as fakewww needs to export it */
#define ADMINFILE "admin.html" 

/* Location of default config file */
#define CONFIGFILE  "/etc/ndt.conf"

#define PORT 3001
#define PORT2 3002
#define PORT3 3003
#define PORT4 3004

/* hard-coded port values */
#define sPORT "3001"
#define sPORT2 "3002"
#define sPORT3 "3003"
#define sPORT4 "3004"

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
	u_int32_t saddr;
	u_int32_t daddr;
	struct in6_addr s6addr;
	struct in6_addr d6addr;
	u_int16_t sport;
	u_int16_t dport;
	u_int32_t seq;
	u_int32_t ack;
	u_int32_t win;
	int family;
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

int32_t gmt2local(time_t);
int err_sys(char* s);

/* web100-pcap */
void init_vars(struct spdpair *cur);
void print_bins(struct spdpair *cur, int monitor_pipe[2], char *LogFileName, int debug);
void calculate_spd(struct spdpair *cur, struct spdpair *cur2, int port2, int port3);

/* web100-util */
int web100_init(char *VarFileName, int debug);
int web100_autotune(int sock, web100_agent* agent, int debug);
void web100_middlebox(int sock, web100_agent* agent, char *results, int debug);
int web100_setbuff(int sock, web100_agent* agent, int autotune, int debug);
void web100_get_data_recv(int sock, web100_agent* agent, char *LogFileName, int count_vars, int debug);
int web100_get_data(web100_snapshot* snap, int ctlsock, web100_agent* agent, int count_vars, int debug);
int CwndDecrease(web100_agent* agent, char* logname, int *dec_cnt, int *same_cnt, int *inc_cnt, int debug);
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

/* web100-admin */
void view_init(char *LogFileName, int refresh, int debug);
int calculate(char now[32], int SumRTT, int CountRTT, int CongestionSignals, int PktsOut,
    int DupAcksIn, int AckPktsIn, int CurrentMSS, int SndLimTimeRwin,
    int SndLimTimeCwnd, int SndLimTimeSender, int MaxRwinRcvd, int CurrentCwnd,
    int Sndbuf, int DataBytesOut, int mismatch, int bad_cable, int c2sspd, int s2cspd,
    int c2sdata, int s2cack, int view_flag, int debug);
void gen_html(int c2sspd, int s2cspd, int MinRTT, int PktsRetrans, int Timeouts, int Sndbuf,
    int MaxRwinRcvd, int CurrentCwnd, int mismatch, int bad_cable, int totalcnt, int refresh, int debug);

#endif
