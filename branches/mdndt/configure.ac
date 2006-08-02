#
#      $Id$
#
#########################################################################
#									#
#			   Copyright (C)  2003				#
#				Internet2				#
#			   All Rights Reserved				#
#									#
#########################################################################
#
#	File:		configure.ac
#
#	Author:		Rich Carlson
#			Internet2
#
#	Date:		Thur Feb 26 14:31:03 CST 2004
#
#	Description:	autoconfig input script for NDT build
#
#	Usage:		see bootstrap...
#
#	Environment:
#
#	Files:
#
#
#	Options:
# Process this file with autoconf to produce a configure script.
# 
# AC_PREREQ(2.57)
AC_INIT([NDT],[3.3.7],[rcarlson@internet2.edu])
AC_CONFIG_AUX_DIR(config)
# AM_INIT_AUTOMAKE(NDT, v1.0, [no-define])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/analyze.c])
# AM_CONFIG_HEADER([config.h])
AC_CONFIG_HEADER([config.h])

TOP_BUILD_DIRS=""

CFLAGS="-pedantic -Wall -O2"

# Enabling/disabling asserts and debugging
AC_MSG_CHECKING(whether to enable debugging)
AC_ARG_ENABLE(debug, [  --enable-debug=[no/yes] turn on debugging
              [default=no]],, enable_debug=no)
if test "x$enable_debug" = "xyes"; then
  CFLAGS="$CFLAGS -g"
  AC_MSG_RESULT(yes)
else
  CFLAGS="$CFLAGS -DNDEBUG"
  AC_MSG_RESULT(no)
fi

AC_ARG_WITH(I2util,
		AC_HELP_STRING([--with-I2util=<dir>],
				[defaults to building I2util under owamp if exists]),
		with_I2util=$withval, with_I2util=yes)

#
# find I2util
#
I2UTILLDFLAGS=""
I2UTILLIBS=""
I2UTILLIBDEPS=""
I2UTILINCS=""
if test "$with_I2util" != "no"; then
	# first, check for directory given.
	if test "$with_I2util" != "yes"; then
		I2util_dir=`dirname $with_I2util`
		case $I2util_dir in
			/*) ;; # already an absolute path
			*) I2util_dir="`pwd`/$I2util_dir" ;;
		esac
		I2UTILINCS="-I$I2util_dir/include $I2UTILINCS"
		I2UTILLDFLAGS="-L$I2util_dir/lib $I2UTILLDFLAGS"
		I2UTILLIBDEPS="$I2util_dir/lib/libI2util.a"
	# now, check for sub-build/sub-configure
	elif test -d I2util/I2util; then
		AC_CONFIG_SUBDIRS(I2util)
		TOP_BUILD_DIRS="I2util $TOP_BUILD_DIRS"
		I2util_dir='${top_srcdir}/I2util'
		I2UTILINCS="-I$I2util_dir $I2UTILINCS"
		I2UTILLDFLAGS="-L$I2util_dir/I2util $I2UTILLDFLAGS"
		I2UTILLIBDEPS="$I2util_dir/I2util/libI2util.a"
		
	# now, check for installed I2util
	elif test -d /usr/local/I2util; then
		I2UTILINCS="-I/usr/local/I2util/include $I2UTILINCS"
		I2UTILLDFLAGS="-L/usr/local/I2util/lib $I2UTILLDFLAGS"
		I2UTILLIBDEPS="/usr/local/I2util/lib/libI2util.a"
	fi

	if test -z "$I2UTILLDFLAGS"; then
		AC_MSG_ERROR([couldn't find I2util library])
	else
		I2UTILLIBS="$I2UTILLDFLAGS -lI2util"
	fi
fi

AC_SUBST(I2UTILLDFLAGS)
AC_SUBST(I2UTILLIBS)
AC_SUBST(I2UTILLIBDEPS)
AC_SUBST(I2UTILINCS)

#
# set local vars
#

NDTINCDIR='-I/usr/local/include/web100 -I/usr/include/pcap'
NDTLIBDIR=/usr/local/lib
NDTLDFLAGS='-L$(NDTLIBDIR) -Wl,-rpath,$(NDTLIBDIR)'
NDTLIBS='-lweb100 -lpcap -lm'
NDTINCS='-I$(NDTINCDIR)'

AC_SUBST(NDTINCS)
AC_SUBST(NDTLDFLAGS)
AC_SUBST(NDTLIBS)
AC_SUBST(NDTLIBDIR)
AC_SUBST(NDTINCDIR)

AC_SUBST(TOP_BUILD_DIRS)

#
# Java config files
#
GCJ=javac
GCJFLAGS=

AC_SUBST(GCJ)
AC_SUBST(GCJFLAGS)

NDTJAR=jar
NDTJARFLAG='-cmf'

AC_SUBST(NDTJAR)
AC_SUBST(NDTJARFLAG)

# Checks for programs.
AC_PROG_CC
AM_PROG_GCJ

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h linux/sockios.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_WAIT3
AC_CHECK_FUNCS([alarm bzero gethostbyaddr gettimeofday inet_ntoa memset select socket sqrt strchr strstr])

AC_SUBST(ac_aux_dir)
AC_CONFIG_FILES([Makefile
		src/Makefile Admin/Makefile Applet/Makefile
		conf/Makefile doc/Makefile])
AC_OUTPUT
