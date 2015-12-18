/*
 * This file contains the definitions and function declarations to
 * handle various tests.
 *
 * Jakub S�awi�ski 2006-06-24
 * jeremian@poczta.fm
 */

#ifndef SRC_TESTOPTIONS_H_
#define SRC_TESTOPTIONS_H_

#include "web100srv.h"
#include "protocol.h"
#include "connection.h"

#define LISTENER_SOCKET_CREATE_FAILED  -1
#define SOCKET_CONNECT_TIMEOUT  -100
#define RETRY_EXCEEDED_WAITING_CONNECT -101
#define RETRY_EXCEEDED_WAITING_DATA -102
#define SOCKET_STATUS_FAILED -1

#define JSON_SUPPORT 1
#define WEBSOCKET_SUPPORT 2
#define TLS_SUPPORT 4

typedef struct testoptions {
  int multiple;  // multiples tests enabled
  int mainport;  // main port used for test

  char client_version[CS_VERSION_LENGTH_MAX + 1]; // client version number.

  // indicates if client supports JSON messages, websockets, or TLS
  int connection_flags;

  int midopt;  // middlebox test to be perfomed?
  int midsockfd;  // socket file desc for middlebox test
  int midsockport;  // port used for middlebox test

  int c2sopt;  // C2S test to be perfomed?
  int c2ssockfd;  // socket fd for C2S test
  int c2ssockport;  // port used for C2S test

  int s2copt;  // S2C test to be perfomed?
  int s2csockfd;  // socket fd for S2C test
  int s2csockport;  // port used for S2C test

  // child pids
  pid_t child0;
  pid_t child1;
  pid_t child2;

  int sfwopt;  // Is firewall test to be performed?
  int metaopt;  // meta test to be perfomed?
  int c2sextopt; // extended C2S test to be performed?
  int s2cextopt; // extended S2C test to be performed?
} TestOptions;

// Snap log characteristics
typedef struct snapArgs {
  tcp_stat_connection conn;
  tcp_stat_snap* snap;
  tcp_stat_log* log;
  int delay;  // periodicity, in ms, of collecting snap
} SnapArgs;

int initialize_tests(Connection* ctl, TestOptions* testOptions,
                     char* test_suite, size_t test_suite_strlen);

void catch_s2c_alrm(int signo);

int test_sfw_srv(Connection* ctl, tcp_stat_agent* agent, TestOptions* options,
                 int conn_options);
int test_meta_srv(Connection* ctl, tcp_stat_agent* agent,
                  TestOptions* test_options, int conn_options,
                  Options* options);

int getCurrentTest();
void setCurrentTest(int testId);

// void start_snap_worker(SnapArgs *snaparg, tcp_stat_agent *agentarg,
void start_snap_worker(SnapArgs *snaparg, tcp_stat_agent *agentarg,
                       CwndPeaks* peaks, char snaplogenabled,
                       pthread_t *wrkrthreadidarg, char *metafilename,
                       tcp_stat_connection conn, tcp_stat_group* group);

void stop_snap_worker(pthread_t *workerThreadId, char snaplogenabled,
                      SnapArgs* snapArgs_ptr);

void setCwndlimit(tcp_stat_connection connarg, tcp_stat_group* grouparg,
                  tcp_stat_agent* agentarg, Options* optionsarg);

int is_buffer_clogged(int nextseqtosend, int lastunackedseq);
void stop_packet_trace(int *monpipe_arr);

void addAdditionalMetaEntry(char* key, char* value);
void addAdditionalMetaIntEntry(char* key, int value);
void addAdditionalMetaBoolEntry(char* key, int value);

#endif  // SRC_TESTOPTIONS_H_
