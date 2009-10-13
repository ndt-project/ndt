/*
 * This file contains the function declarations of the logging
 * system.
 *
 * Jakub S³awiñski 2006-06-14
 * jeremian@poczta.fm
 */

#ifndef _JS_LOGGING_H
#define _JS_LOGGING_H

#include <I2util/util.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#define LOGFILE "web100srv.log"   /* Name of log file */

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

struct metadata {
    char c2s_snaplog[64];
    char c2s_ndttrace[64];
    char s2c_snaplog[64];
    char s2c_ndttrace[64];
    char CPU_time[64];
    char summary[256];
    char date[32];
    char time[16];
    char client_ip[64];
    struct sockaddr_storage c_addr;
    char client_name[64];
    char client_os[32];
    char client_browser[32];
    int  ctl_port;
    char server_ip[64];
    char server_name[64];
    char server_os[32];
    int  family;
};

struct metadata meta;
#endif
