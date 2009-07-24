/*
 * This file contains the functions of the logging system.
 *
 * Jakub S³awiñski 2006-06-14
 * jeremian@poczta.fm
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>

#include "logging.h"

static int                _debuglevel        = 0;
static char*              _programname       = "";
static char*              LogFileName        = BASEDIR"/"LOGFILE;
static I2ErrHandle        _errorhandler_nl   = NULL;
static I2ErrHandle        _errorhandler      = NULL;
static I2LogImmediateAttr _immediateattr_nl;
static I2LogImmediateAttr _immediateattr;
static time_t             timestamp;
static long int		  utimestamp;

/*
 * Function name: log_init
 * Description: Initializes the logging system.
 * Arguments: progname - the name of the program
 *            debuglvl - the debug level
 */

void
log_init(char* progname, int debuglvl)
{
  assert(progname);

  _programname = (_programname = strrchr(progname,'/')) ? _programname+1 : progname;

  _immediateattr.fp = _immediateattr_nl.fp = stderr;
  _immediateattr.line_info = I2MSG | I2NONL;
  _immediateattr_nl.line_info = I2MSG;
  _immediateattr.tformat = _immediateattr_nl.tformat = NULL;
  
  _errorhandler = I2ErrOpen(progname, I2ErrLogImmediate, &_immediateattr, NULL, NULL);
  _errorhandler_nl = I2ErrOpen(progname, I2ErrLogImmediate, &_immediateattr_nl, NULL, NULL);

  if (!_errorhandler || !_errorhandler_nl) {
    fprintf(stderr, "%s : Couldn't init error module\n", progname);
    exit(1);
  }

  _debuglevel = debuglvl;
}

/*
 * Function name: set_debuglvl
 * Description: Sets the debug level to the given value.
 * Arguments: debuglvl - the new debug level
 */

void
set_debuglvl(int debuglvl)
{
  _debuglevel = debuglvl;
}

/*
 * Function name: set_logfile
 * Description: Sets the log filename.
 * Arguments: filename - new log filename
 */

void
set_logfile(char* filename)
{
  LogFileName = filename;
}

/*
 * Function name: get_debuglvl
 * Description: Returns the current debug level.
 * Returns: The current debug level
 */

int
get_debuglvl()
{
  return _debuglevel;
}

/*
 * Function name: get_logfile
 * Description: Returns the log filename.
 * Returns: The log filename
 */

char*
get_logfile()
{
  return LogFileName;
}

/*
 * Function name: get_errhandle
 * Description: Returns the error handle, that writes the messages
 *              with the new line.
 * Returns: The error handle
 */

I2ErrHandle
get_errhandle()
{
  return _errorhandler_nl;
}

/*
 * Function name: ndt_print
 * Description: Logs the message with the given lvl.
 * Arguments: lvl - the level of the message
 *            format - the format of the message
 *            ... - the additional arguments
 */

void
log_print(int lvl, const char* format, ...)
{
  va_list   ap;

  if (lvl > _debuglevel) {
    return;
  }

  va_start(ap, format);
  I2ErrLogVT(_errorhandler,-1,0,format,ap);
  va_end(ap);
}

/*
 * Function name: ndt_println
 * Description: Logs the message with the given lvl. New line character
 *              is appended to the error stream.
 * Arguments: lvl - the level of the message
 *            format - the format of the message
 *            ... - the additional arguments
 */

void
log_println(int lvl, const char* format, ...)
{
  va_list   ap;

  if (lvl > _debuglevel) {
    return;
  }

  va_start(ap, format);
  I2ErrLogVT(_errorhandler_nl,-1,0,format,ap);
  va_end(ap);
}

/**
 * Function name: set_timestamp
 * Description: Sets the timestamp to actual time.
 */
void
set_timestamp()
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    timestamp = tv.tv_sec;
    utimestamp = tv.tv_usec;

/*  Changed function to use gettimeofday() need usec value for ISO8601 file names
 *  RAC 5/6/09
 *  timestamp = time(NULL);
 */

}

/**
 * Function name: get_timestamp
 * Description: Returns the previously recorded timestamp.
 * Returns: The timestamp
 */
time_t
get_timestamp()
{
    return timestamp;
}

/**
 * Function name: get_utimestamp
 * Description: Returns the previously recorded utimestamp.
 * Returns: The utimestamp
 */
long int
get_utimestamp()
{
    return utimestamp;
}

/**
 * Function name get_YYYY
 * Description: Returns a character string YYYY for the current year
 * Author: Rich Carlson - 6/29/09
 */

void
 get_YYYY (char *year)
{

struct tm *result;
time_t now;

setenv("TZ", "UTC", 0);
now = get_timestamp();
result = gmtime(&now);

sprintf(year, "%d", 1900+result->tm_year);
}


/**
 * Function name get_MM
 * Description: Returns a character string MM for the current year
 * Author: Rich Carlson - 6/29/09
 */

void
 get_MM (char *month)
{

struct tm *result;
time_t now;

/* setenv("TZ", NULL, 0); */
setenv("TZ", "UTC", 0);
now = get_timestamp();
result = gmtime(&now);

if (1+result->tm_mon < 10)
    sprintf(month, "0%d", 1+result->tm_mon);
else
    sprintf(month, "%d", 1+result->tm_mon);
}

/**
 * Function name get_DD
 * Description: Returns a character string DD for the current year
 * Author: Rich Carlson - 6/29/09
 */

void
 get_DD (char *day)
{

struct tm *result;
time_t now;

setenv("TZ", "UTC", 0);
now = get_timestamp();
result = gmtime(&now);

if (result->tm_mday < 10)
    sprintf(day, "0%d", result->tm_mday);
else
    sprintf(day, "%d", result->tm_mday);
}

/**
 * Function name get_ISOtime
 * Description: Returns a character string in the ISO8601 time foramt
 * 		used to create snaplog and trace file names
 * Returns: character string with ISO time string.
 * Author: Rich Carlson - 5/6/09
 */

char *
 get_ISOtime (char *isoTime)
{

struct tm *result;
time_t now;
char tmpstr[16];

setenv("TZ", "UTC", 0);
now = get_timestamp();
result = gmtime(&now);

sprintf(isoTime, "%d", 1900+result->tm_year);
if (1+result->tm_mon < 10)
    sprintf(tmpstr, "0%d", 1+result->tm_mon);
else
    sprintf(tmpstr, "%d", 1+result->tm_mon);
strncat(isoTime, tmpstr, 2);

if (result->tm_mday < 10)
    sprintf(tmpstr, "0%d", result->tm_mday);
else
    sprintf(tmpstr, "%d", result->tm_mday);
strncat(isoTime, tmpstr, 2);

if (result->tm_hour < 10)
    sprintf(tmpstr, "T0%d", result->tm_hour);
else
    sprintf(tmpstr, "T%d", result->tm_hour);
strncat(isoTime, tmpstr, 3);

if (result->tm_min < 10)
    sprintf(tmpstr, ":0%d", result->tm_min);
else
    sprintf(tmpstr, ":%d", result->tm_min);
strncat(isoTime, tmpstr, 3);

if (result->tm_sec < 10)
    sprintf(tmpstr, ":0%d", result->tm_sec);
else
    sprintf(tmpstr, ":%d", result->tm_sec);
strncat(isoTime, tmpstr, 3);

sprintf(tmpstr, ".%ldZ", get_utimestamp()*1000);
strncat(isoTime, tmpstr, 11);
return isoTime;
}

/* Write meta data out to log file.  This file contains details and
 * names of the other log files.
 * RAC 7/7/09
 */

void writeMeta(void)
{
    FILE *fp;
    char tmpstr[256], dir[128];
    char isoTime[64];
    size_t tmpstrlen=sizeof(tmpstr);
    /* struct hostent *hp; */
    socklen_t len;

/* Get the clients domain name and same in metadata file 
 * changed to use getnameinfo 7/24/09
 * RAC 7/7/09
 */
/*
 * #ifdef AF_INET6
 *     if (meta.family == AF_INET6)
 * 	hp = (struct hostent *)gethostbyaddr((char *) &meta.client_ip, 16, AF_INET6);
 * #endif
 *     if (meta.family == AF_INET)
 * 	hp = (struct hostent *)gethostbyaddr((char *) &meta.client_ip, 4, AF_INET);
 *     if (hp == NULL)
 */
    if (getnameinfo((struct sockaddr *)&meta.c_addr, len, tmpstr, tmpstrlen,
	NULL, 0, NI_NAMEREQD))
	memcpy(meta.client_name, "No FQDN name", 12);
    else
	memcpy(meta.client_name, tmpstr, tmpstrlen);

    memset(tmpstr, 0, tmpstrlen);
    strncpy(tmpstr, DataDirName, strlen(DataDirName));
    if ((opendir(tmpstr) == NULL) && (errno == ENOENT))
            mkdir(tmpstr, 0755);
    get_YYYY(dir);
    strncat(tmpstr, dir, 4);
    if ((opendir(tmpstr) == NULL) && (errno == ENOENT))
            mkdir(tmpstr, 0755);
    strncat(tmpstr, "/", 1);
    get_MM(dir);
    strncat(tmpstr, dir, 2);
    if ((opendir(tmpstr) == NULL) && (errno == ENOENT))
            mkdir(tmpstr, 0755);
    strncat(tmpstr, "/", 1);
    get_DD(dir);
    strncat(tmpstr, dir, 2);
    if ((opendir(tmpstr) == NULL) && (errno == ENOENT))
            mkdir(tmpstr, 0755);
    strncat(tmpstr, "/", 1);
    sprintf(dir, "%s_%s:%d.meta", get_ISOtime(isoTime), meta.client_ip, meta.ctl_port);
    strncat(tmpstr, dir, strlen(dir));

    fp = fopen(tmpstr,"w");
    if (fp == NULL) {
        log_println(1, "Unable to open metadata log file, continuing on without logging");
    }
    else {
        log_println(5, "Opened '%s' metadata log file", tmpstr);
        fprintf(fp, "Date/Time: %s\n", meta.date);
        fprintf(fp, "c2s_snaplog file: %s\n", meta.c2s_snaplog);
        fprintf(fp, "c2s_ndttrace file: %s\n", meta.c2s_ndttrace);
        fprintf(fp, "s2c_snaplog file: %s\n", meta.s2c_snaplog);
        fprintf(fp, "s2c_snaplog file: %s\n", meta.s2c_ndttrace);
        fprintf(fp, "cputime file: %s\n", meta.CPU_time);
        fprintf(fp, "server IP address: %s\n", meta.server_ip);
        fprintf(fp, "server hostname: %s\n", meta.server_name);
        fprintf(fp, "server kernel version: %s\n", meta.server_os);
        fprintf(fp, "client IP address: %s\n", meta.client_ip);
        fprintf(fp, "client hostname: %s\n", meta.client_name);
        fprintf(fp, "client OS name: %s\n", meta.client_os);
        fprintf(fp, "client_browser name: %s\n", meta.client_browser);
        fprintf(fp, "Summary data: %s\n", meta.summary);
        fclose(fp);
    }
}
