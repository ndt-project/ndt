/*
 * This file contains the functions of the logging system.
 *
 * Jakub S�awi�ski 2006-06-14
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

/* #ifdef HAVE_ZLIB_H */
#include <zlib.h>
/* #endif */

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

/**
 * Compress snaplog, tcpdump, and cputime files to save disk space.
 * These files compress by 2 to 3 orders of
 * magnitude (100 - 1000 times).  This can save a lot of disk space.
 *   9/9/09  RAC
 *
 *  @param pointer to string containing the source file name
 *  @return integer 0 on success, error code on failure
 *   Possible error codes are:
 *  	-3: Failure to open source file for reading
 *  	-4: Failure to open dest file for writing
 *  	-1: Z_ERRNOd as defind by zlib library
 *  	other error codes as defined by zlib's deflateInit(0 method:
 *  		Z_MEM_ERROR if there was not enough memory,
 *  		Z_STREAM_ERROR if level is not a valid compression level,
 *  		Z_VERSION_ERROR if the zlib library version (zlib_version)
 *  			is incompatible with the version assumed by the caller (ZLIB_VERSION)
 *
 */

int zlib_def(char *src_fn) {

  int ret, flush, level=Z_DEFAULT_COMPRESSION;
  char dest_fn[256];
  FILE *dest, *source;
  unsigned have;
  z_stream strm;
  unsigned char in[16384];
  unsigned char out[16384];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, level);
    if (ret != Z_OK) {
	log_println(6, "zlib deflateInit routine failed with %d", ret);
        return ret;
    }

    sprintf(dest_fn, "%s.gz", src_fn);
    if ((source = fopen(src_fn, "r")) == NULL) {
	log_println(6, "zlib_def(): failed to open src file '%s' for reading", src_fn);
	return -3;
    }
    if ((dest = fopen(dest_fn, "w")) == NULL) {
	log_println(6, "zlib_def(): failed to open dest file '%s' for writing", dest_fn);
	return -4;
    }
    /* compress until end of file */
    do {
        strm.avail_in = fread(in, 1, 16384, source);
        if (ferror(source)) {
            (void)deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        /* run deflate() on input until output buffer not full, finish
 *            compression if all of source has been read in */
        do {
            strm.avail_out = 16384;
            strm.next_out = out;

            ret = deflate(&strm, flush);    /* no bad return value */
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            have = 16384 - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }

        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);     /* all input will be used */

        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    assert(ret == Z_STREAM_END);        /* stream will be complete */

    /* clean up and return */
    (void)deflateEnd(&strm);

    /* compressed version of file is now created, remove the original uncompressed version */
   remove(src_fn); 
   
   return Z_OK;
}
/* #endif */

/**
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

/**
 * Function name: log_free
 * Description: Free malloc'ed memmory after child process ends.  Allocation without 
 * a corresponding free() causes a memory leak, and the main process never ends so
 * memory is not free'ed on a close.
 * Arguments: none
 * Added RAC 10/13/09
 */

void
log_free(void)
{
  free(_errorhandler);
  free(_errorhandler_nl);
}

/**
 * Function name: set_debuglvl
 * Description: Sets the debug level to the given value.
 * Arguments: debuglvl - the new debug level
 */

void
set_debuglvl(int debuglvl)
{
  _debuglevel = debuglvl;
}

/**
 * Function name: set_logfile
 * Description: Sets the log filename.
 * Arguments: filename - new log filename
 */

void
set_logfile(char* filename)
{
  LogFileName = filename;
}

/**
 * Function name: get_debuglvl
 * Description: Returns the current debug level.
 * Returns: The current debug level
 */

int
get_debuglvl()
{
  return _debuglevel;
}

/**
 * Function name: get_logfile
 * Description: Returns the log filename.
 * Returns: The log filename
 */

char*
get_logfile()
{
  return LogFileName;
}

/**
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

/**
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

/**
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
 * @param Pointer to the string indicating the year
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
 * @param Pointer to the string indicating month
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
 * @param Pointer to the string indicating day
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
 * @param Pointer to the string indicating ISO time
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

/**
 * Write meta data out to log file.  This file contains details and
 * names of the other log files.
 * @param compress integer flag indicating whether log file compression
 * 			is enabled
 * @param cputime integer flag indicating if cputime trace logging is on
 * @param snaplog integer flag indicating if snaplogging is enabled
 * @param tcpdump integer flag indicating if tcpdump trace logging is on
 *
 * RAC 7/7/09
 */

void writeMeta(int compress, int cputime, int snaplog, int tcpdump)
{
	FILE * fp;
	char tmpstr[256], dir[128], tmp2str[256];
	char isoTime[64], filename[256];
	size_t tmpstrlen = sizeof(tmpstr);
	socklen_t len;
	DIR *dp;

	/* Get the clients domain name and same in metadata file
	 * changed to use getnameinfo 7/24/09
	 * RAC 7/7/09
	 */

	// get socketaddr size based on whether IPv6/IPV4 address was used
#ifdef AF_INET6
	if (meta.family == AF_INET6)
		len = sizeof(struct sockaddr_in6);
#endif
	if (meta.family == AF_INET)
		len = sizeof(struct sockaddr_in);

	// Look up the host name and service name information for given struct sockaddress and length
	if (getnameinfo((struct sockaddr *) &meta.c_addr, len, tmpstr, tmpstrlen,
			NULL, 0, NI_NAMEREQD))
	{
		// No fully qualified domain name
		memcpy(meta.client_name, "No FQDN name", 12);
	}
	else
	{
		// copy client's hostname into meta structure
		log_println(2, "extracting hostname %s", tmpstr);
		memcpy(meta.client_name, tmpstr, strlen(tmpstr));
	}

	// reset tmpstr
	memset(tmpstr, 0, tmpstrlen);

	// The following section of code creates a log directory to record meta data,
	// if one is not already present with the details needed. The log file location,
	// and name themselves record the year, month, date, time and client name.
	// The "primary" location for this log file is obtained as a command line option
	// from the user, or is the default location of BASE_DIR+LOG_DIR of the NDT installation

	strncpy(tmpstr, DataDirName, strlen(DataDirName));
	// Open the directory or create one if not yet present
	if ((dp = opendir(tmpstr)) == NULL && errno == ENOENT)
		mkdir(tmpstr, 0755);
	closedir(dp); // close opened directory
	get_YYYY(dir); // get current year
	strncat(tmpstr, dir, 4);
	if ((dp = opendir(tmpstr)) == NULL && errno == ENOENT)
		mkdir(tmpstr, 0755); // create directory with year appended
	closedir(dp); // close the opened directory
	strncat(tmpstr, "/", 1);
	get_MM(dir); // get month info
	strncat(tmpstr, dir, 2); // append month info to the directory name
	if ((dp = opendir(tmpstr)) == NULL && errno == ENOENT)
		mkdir(tmpstr, 0755);
	closedir(dp);
	strncat(tmpstr, "/", 1);
	get_DD(dir); // get date
	strncat(tmpstr, dir, 2); // append date info to directory name
	if ((dp = opendir(tmpstr)) == NULL && errno == ENOENT)
		mkdir(tmpstr, 0755);
	closedir(dp);

	// end block creating log directory


	memcpy(tmp2str, tmpstr, tmpstrlen); // tmp2str now contains the dir name intended
	// tmpstr will henceforth refer to the log file's name
	strncat(tmpstr, "/", 1);
	sprintf(dir, "%s_%s:%d.meta", get_ISOtime(isoTime), meta.client_ip,
			meta.ctl_port); // now get ISO time, the client's name and port
	strncat(tmpstr, dir, strlen(dir)); // append above details to log filename


	log_println(6, "Should compress snaplog and tcpdump files compress=%d",
			compress);
	/* #ifdef HAVE_ZLIB_H */

	// If compression is enabled, compress files in the "log" directory
	if (compress == 1)
	{
		log_println(5,
				"Compression is enabled, compress all files in '%s' basedir",
				tmp2str);
		if (snaplog)
		{ // if snaplog is enabled, compress those into .gz formats

			// Try compressing C->S test snaplogs
			memset(filename, 0, 256);
			sprintf(filename, "%s/%s", tmp2str, meta.c2s_snaplog);
			if (zlib_def(filename) != 0)
				log_println(5, "compression failed");
			else
				strncat(meta.c2s_snaplog, ".gz", 3);

			// Try compressing S->C test snaplogs
			memset(filename, 0, 256);
			sprintf(filename, "%s/%s", tmp2str, meta.s2c_snaplog);
			if (zlib_def(filename) != 0)
				log_println(5, "compression failed");
			else
				strncat(meta.s2c_snaplog, ".gz", 3);
		}

		// If tcpdump file writing is enabled, compress those
		if (tcpdump)
		{

			// Try compressing C->S test tcpdump
			// the tcpdump file extension is as specified
			memset(filename, 0, 256);
			sprintf(filename, "%s/%s", tmp2str, meta.c2s_ndttrace);
			if (zlib_def(filename) != 0)
				log_println(5, "compression failed");
			else
				strncat(meta.c2s_ndttrace, ".gz", 3);

			// Try compressing S->C test tcpdumps
			memset(filename, 0, 256);
			sprintf(filename, "%s/%s", tmp2str, meta.s2c_ndttrace);
			if (zlib_def(filename) != 0)
				log_println(5, "compression failed");
			else
				strncat(meta.s2c_ndttrace, ".gz", 3);
		}

		// If writing "cputime" file is enabled, compress those log files too
		if (cputime)
		{
			memset(filename, 0, 256);
			sprintf(filename, "%s/%s", tmp2str, meta.CPU_time);
			if (zlib_def(filename) != 0)
				log_println(5, "compression failed");
			else
				strncat(meta.CPU_time, ".gz", 3);
		}
		else
			log_println(
					5,
					"Zlib compression disabled, log files will not be compressed");
	}
	/* #endif */

	// Try logging metadata into the metadata logfile
	fp = fopen(tmpstr, "w");
	if (fp == NULL)
	{ // error, unable to open file in write mode
		log_println(
				1,
				"Unable to open metadata log file, continuing on without logging");
	}
	else
	{
		log_println(5, "Opened '%s' metadata log file", tmpstr);
		fprintf(fp, "Date/Time: %s\n", meta.date);
		fprintf(fp, "c2s_snaplog file: %s\n", meta.c2s_snaplog);
		fprintf(fp, "c2s_ndttrace file: %s\n", meta.c2s_ndttrace);
		fprintf(fp, "s2c_snaplog file: %s\n", meta.s2c_snaplog);
		fprintf(fp, "s2c_ndttrace file: %s\n", meta.s2c_ndttrace);
		fprintf(fp, "cputime file: %s\n", meta.CPU_time);
		fprintf(fp, "server IP address: %s\n", meta.server_ip);
		fprintf(fp, "server hostname: %s\n", meta.server_name);
		fprintf(fp, "server kernel version: %s\n", meta.server_os);
		fprintf(fp, "client IP address: %s\n", meta.client_ip);
		fprintf(fp, "client hostname: %s\n", meta.client_name);
		fprintf(fp, "client OS name: %s\n", meta.client_os);
		fprintf(fp, "client_browser name: %s\n", meta.client_browser);
		fprintf(fp, "Summary data: %s\n", meta.summary);
		if (meta.additional)
		{
			fprintf(fp, " * Additional data:\n");
			struct metaentry* entry = meta.additional;
			while (entry)
			{
				fprintf(fp, "%s: %s\n", entry->key, entry->value);
				entry = entry->next;
			}
		}
		fclose(fp);
	}
}
