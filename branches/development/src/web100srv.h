/* 
 * Start of process to break the web100srv.c file into several
 * pieces.  This should make maintenance easier.  Define
 * the global variables here.
 *
 * Rich Carlson 3/10/04
 * rcarlson@interenet2.edu
 */

#define   _USE_BSD
#include  <stdio.h>
#include  <netdb.h>
#include  <signal.h>
#include  <web100.h>
#include  <getopt.h>
#include  <pcap.h>
#include  <stdlib.h>
#include  <string.h>
#include  <unistd.h>
#include  <fcntl.h>

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
#include  <net/ethernet.h>
#include  <arpa/inet.h>


/* move version to configure.ac file for package name */
/* #define VERSION   "3.0.7" */		/* version number */
#define BUFFSIZE  8192
#define RECLTH    8192
#define NOLOSS	0.0000000001

#define LOGFILE "web100srv.log"
static char lgfn[256];
#define WEB100_VARS 128	/*number of web100 variables you want to access*/
#define WEB100_FILE "web100_variables"  /*names of the variables to access*/
static char wvfn[256];

/* this file needs to be fixed, as fakewww needs to export it */
#define ADMINFILE "admin.html" 

/* Location of default config file */
#define CONFIGFILE  "/etc/ndt.conf"

#define PORT 3001
#define PORT2 3002
#define PORT3 3003
#define PORT4 3004

struct ndtchild {
	int pid;
	char addr[64];
	char host[256];
	int stime;
	int pipe;
	int ctlsockfd;
	struct ndtchild *next;
};

struct spdpair {
	u_int32_t saddr;
	u_int32_t daddr;
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
