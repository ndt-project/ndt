/**
 * This file contains the function declarations of the logging
 * system.
 *
 * Jakub S�awi�ski 2006-06-14
 * jeremian@poczta.fm
 */

#ifndef SRC_LOGGING_H_
#define SRC_LOGGING_H_

#include <I2util/util.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ndtptestconstants.h"
#include "runningtest.h"  // protocol validation
#define LOG_FACILITY LOG_LOCAL0  /* Syslog facility to log at */
#define LOGDIR "serverdata"  // directory for detailed snaplog and tcpdump files
#define LOGFILE "web100srv.log"  /* Name of log file */
#define PROTOLOGFILE "web100srvprotocol.log"  // name of protocol log file
#define PROTOLOGPREFIX "web100srvprotocol_"  // prefix for protocol log file
#define PROTOLOGSUFFIX ".log"  // suffix for protocol validation log file
#define FILENAME_SIZE 1024
/* #define PROTOLOGDIR "protocollog"     Protocol log dir */
void log_init(char* progname, int debuglvl);
void set_debuglvl(int debuglvl);
void set_logfile(char* filename);
int get_debuglvl();
char* get_logfile();

I2ErrHandle get_errhandle();

#define log_print log_println
void log_println_impl(int lvl, const char* file, int line, const char* format, ...);
#define log_println(lvl, ...) \
    log_println_impl((lvl), __FILE__, __LINE__, __VA_ARGS__)

void log_free(void);
void set_timestamp();
time_t get_timestamp();
long int get_utimestamp();
char * get_ISOtime(char * isoTime, int isoTimeArrSize);
char *get_currenttime(char *isoTime, int isotimearrsize);
void get_YYYY(char * year, size_t year_strlen);
void get_MM(char * month, size_t month_strlen);
void get_DD(char * day, size_t day_strlen);
char * DataDirName;

int zlib_def(char *src_fn);

/**
 * Format used to exchange meta test data between client->server.
 * */
struct metaentry {
  char key[1024];  // key name
  char value[1024];  // value associated with this meta key
  struct metaentry* next;  // pointer to next link
};

struct throughputSnapshot {
  double time;
  double throughput;
  struct throughputSnapshot* next;
};

/**
 * Used to save results of meta tests.
 * These values (most) are thes logged in the
 *  meta data file created  for every session
 * */
// TODO: ensure every write to a char array in meta will always result in a
//       null-terminated string of less than the buffer size.  This will also
//       require auditing of FILENAME_SIZE and the constants used in
//       writeMeta() in web100srv.c
struct metadata {
  char c2s_snaplog[FILENAME_SIZE];  // C->S test Snaplog file name
  char c2s_ndttrace[FILENAME_SIZE];  // C->S NDT trace file name
  char s2c_snaplog[MAX_STREAMS][FILENAME_SIZE];  // S->C test Snaplog file name
  char s2c_ndttrace[FILENAME_SIZE];  // S->C NDT trace file name
  char CPU_time[FILENAME_SIZE];  // CPU time file
  char web_variables_log[FILENAME_SIZE];  // web100/web10g variables log
  char summary[1024];  // Summary data
  char date[1024];  // Date and,
  char time[1024];  // time
  char client_ip[1024];  // Client IP Address
  struct sockaddr_storage c_addr;  // client socket details, not logged
  char client_name[1024];  // client's host-name
  char client_os[1024];  // client OS name
  char client_browser[1024];  // client's browser name
  char client_application[1024];  // client application name
  int ctl_port;  // ctl port
  char server_ip[1024];  // server IP address
  char server_name[1024];  // server's host-name
  char server_os[1024];  // server os name
  int family;  // IP family
  struct metaentry* additional;  // all other additional data
};

void set_protologdir(char* dirname);
void set_protologfile(char* client_ip, char *protologfileparam);
char*
get_protologfile(int socketNum, char *protologfilename, size_t filename_size);
char* get_protologdir();
void enableprotocollogging();
char *createprotologfilename(char* client_ip, char* textappendarg);
void create_named_logdir(char *dirnamedestarg, int destnamearrsize,
                         char *finalsuffix, char isProtoLog);
void create_client_logdir(struct sockaddr *cliaddrarg, socklen_t clilenarg,
                          char *dirnamedestarg, int destnamearrsize,
                          char *finalsuffix, int finalsuffixsize);
void log_linkspeed(int index);

void protolog_printgeneric(const char* key, const char* val, int socketnum);
void protolog_status(int pid, enum TEST_ID testid,
                     enum TEST_STATUS_INT teststatus, int socketnum);
void protolog_sendprintln(const int type, const void* msg, const int len,
                          const int processid, const int ctlSocket);
void protolog_rcvprintln(const int type, void* msg, const int len,
                         const int processid, const int ctlSocket);
void protolog_procstatus(int pid, enum TEST_ID testidarg,
                         enum PROCESS_TYPE_INT procidarg,
                         enum PROCESS_STATUS_INT teststatusarg, int socketnum);
void protolog_procstatuslog(int pid, enum TEST_ID testidarg,
                            enum PROCESS_TYPE_INT procidarg,
                            enum PROCESS_STATUS_INT teststatusarg,
                            int socketnum);
char get_protocolloggingenabled();
void create_protolog_dir();

struct metadata meta;
#endif  // SRC_LOGGING_H_
