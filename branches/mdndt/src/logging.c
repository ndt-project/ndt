/*
 * This file contains the functions of the logging system.
 *
 * Jakub S³awiñski 2006-06-14
 * jeremian@poczta.fm
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "logging.h"

static int                _debuglevel   = 0;
static char*              _programname  = "";
static I2ErrHandle        _errorhandler = NULL;
static I2LogImmediateAttr _immediateattr;

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

  _immediateattr.fp = stderr;
  _immediateattr.line_info = I2MSG;
  _immediateattr.tformat = NULL;
  
  _errorhandler = I2ErrOpen(progname, I2ErrLogImmediate, &_immediateattr, NULL, NULL);

  if (!_errorhandler) {
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
 * Function name: ndt_log
 * Description: Logs the message with the given lvl.
 * Arguments: lvl - the level of the message
 *            format - the format of the message
 *            ... - the additional arguments
 */

void
ndt_log(int lvl, const char* format, ...)
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
 * Function name: get_debuglvl
 * Description: Returns the current debug level.
 * Returns: The current debug level
 */

int
get_debuglvl()
{
  return _debuglevel;
}
