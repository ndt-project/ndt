/*
 * This file contains the definitions and function declarations to
 * handle various tests.
 *
 * Jakub S�awi�ski 2006-06-24
 * jeremian@poczta.fm
 */

#ifndef _JS_TESTOPTIONS_H
#define _JS_TESTOPTIONS_H

#include "web100srv.h"


#define LISTENER_SOCKET_CREATE_FAILED  -1
#define SOCKET_CONNECT_TIMEOUT  -100
#define RETRY_EXCEEDED_WAITING_CONNECT -101
#define RETRY_EXCEEDED_WAITING_DATA -102
#define SOCKET_STATUS_FAILED -1


typedef struct testoptions {
	int multiple; // multiples tests enabled
	int mainport; // main port used for test

	int midopt;	  // middlebox test to be perfomed?
	int midsockfd; // socket file desc for middlebox test
	int midsockport; // port used for middlebox test

	int c2sopt;     // C2S test to be perfomed?
	int c2ssockfd;  // socket fd for C2S test
	int c2ssockport; // port used for C2S test

	int s2copt;   // S2C test to be perfomed?
	int s2csockfd; // socket fd for S2C test
	int s2csockport; // port used for S2C test

	// child pids
	pid_t child0;
	pid_t child1;
	pid_t child2;

	int sfwopt;	// Is firewall test to be performed?
	int State; // seems unused currently

	int metaopt; // meta test to be perfomed?
} TestOptions;

// Snap log characteristics
typedef struct snapArgs {
	web100_snapshot* snap; // web_100 snapshot indicator
	web100_log* log; // web_100 log
	int delay; // periodicity, in ms, of collecting snap
} SnapArgs;

int wait_sig;

int initialize_tests(int ctlsockfd, TestOptions* testOptions,
		char * test_suite);

void catch_s2c_alrm(int signo);

int test_sfw_srv(int ctlsockfd, web100_agent* agent, TestOptions* options,
		int conn_options);
int test_meta_srv(int ctlsockfd, web100_agent* agent, TestOptions* options,
		int conn_options);

int getCurrentTest();
void setCurrentTest(int testId);

//void start_snap_worker(SnapArgs *snaparg, web100_agent *agentarg,
void start_snap_worker(SnapArgs *snaparg, web100_agent *agentarg, CwndPeaks* peaks,
                char snaplogenabled,  pthread_t *wrkrthreadidarg,
                char *metafilevariablename, char *metafilename, web100_connection* conn,
                web100_group* group);

void stop_snap_worker(pthread_t *workerThreadId, char snaplogenabled, SnapArgs* snapArgs_ptr);

void setCwndlimit(web100_connection* connarg, web100_group* grouparg,
		web100_agent* agentarg, Options* optionsarg);

int is_buffer_clogged(int nextseqtosend, int lastunackedseq);
void stop_packet_trace(int *monpipe_arr);
#endif
