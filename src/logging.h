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

#define LOGFILE "web100srv.log"   /* Name of log file */

void log_init(char* progname, int debuglvl);
void set_debuglvl(int debuglvl);
void set_logfile(char* filename);
int get_debuglvl();
char* get_logfile();
I2ErrHandle get_errhandle();
void log_print(int lvl, const char* format, ...);
void log_println(int lvl, const char* format, ...);
void set_timestamp();
time_t get_timestamp();

#endif
