/*
 * This file contains the function declarations of the logging
 * system.
 *
 * Jakub S�awi�ski 2006-06-14
 * jeremian@poczta.fm
 */

#ifndef _JS_LOGGING_H
#define _JS_LOGGING_H

#include <I2util/util.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#define LOGFILE "web100srv.log"   /* Name of log file */
#define PROTOLOGFILE "web100srvprotocol.log"   /* Name of protocol validation log file */

void log_init(char* progname, int debuglvl);
void set_debuglvl(int debuglvl);
void set_logfile(char* filename);
int get_debuglvl();
char* get_logfile();
I2ErrHandle get_errhandle();
void log_print(int lvl, const char* format, ...);
void log_println(int lvl, const char* format, ...);
void log_free(void);
void set_timestamp();
time_t get_timestamp();
long int get_utimestamp();
char * get_ISOtime(char * isoTime);
void   get_YYYY(char * year);
void   get_MM(char * month);
void   get_DD(char * day);
char * DataDirName;

int zlib_def(char *src_fn);

/**
 * Format used to exchange meta test data between client->server.
 * */
struct metaentry {
  char key[64]; // key name
  char value[256]; // value associated with this meta key
  struct metaentry* next; // pointer to next link
};

/**
 * Used to save results of meta tests.
 * These values (most) are thes logged in the
 *  meta data file created  for every session
 * */
struct metadata {
    char c2s_snaplog[64]; // C->S test Snaplog file name
    char c2s_ndttrace[64]; // C->S NDT trace file name
    char s2c_snaplog[64]; // S->C test Snaplog file name
    char s2c_ndttrace[64]; // S->C NDT trace file name
    char CPU_time[64]; // CPU time file
    char summary[256]; // Summary data
    char date[32]; // Date and,
    char time[16]; // time
    char client_ip[64]; // Client IP Address
    struct sockaddr_storage c_addr; // client socket details, not logged
    char client_name[64]; // client's host-name
    char client_os[32]; // client OS name
    char client_browser[32]; // client's browser name
    int  ctl_port; // ? todo map use
    char server_ip[64]; // server IP address
    char server_name[64]; // server's host-name
    char server_os[32]; // server os name
    int  family; // IP family
    struct metaentry* additional; // all other additional data
};

struct metadata meta;
#endif
