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

#include "logging.h"

static int                _debuglevel        = 0;
static char*              _programname       = "";
static char*              LogFileName        = BASEDIR"/"LOGFILE;
static I2ErrHandle        _errorhandler_nl   = NULL;
static I2ErrHandle        _errorhandler      = NULL;
static I2LogImmediateAttr _immediateattr_nl;
static I2LogImmediateAttr _immediateattr;
static time_t             timestamp;

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
    timestamp = time(NULL);
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
