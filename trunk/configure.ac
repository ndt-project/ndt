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
AC_INIT([NDT],[3.7.0-rc1],[ndt-user@internet2.edu])
AC_CONFIG_AUX_DIR(config)
# AM_INIT_AUTOMAKE(NDT, v1.0, [no-define])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/analyze.c])
# AM_CONFIG_HEADER([config.h])
AC_CONFIG_HEADER([config.h])

TOP_BUILD_DIRS=""

# Note: -Werror is useful for debugging but a little strict.
# Note: -Wno-unused-result is not supported on all platforms.
# CFLAGS="-pedantic -Wall -Werror -Wno-unused-result -fno-strict-aliasing -std=gnu99"
CFLAGS="-pedantic -Wall -fno-strict-aliasing -std=gnu99"

# Enabling/disabling asserts and debugging
AC_MSG_CHECKING(whether to enable debugging)
AC_ARG_ENABLE(debug, [  --enable-debug=[no/yes] turn on debugging
              [default=no]],, enable_debug=no)
if test "x$enable_debug" = "xyes"; then
  CFLAGS="$CFLAGS -g -O0"
  AC_MSG_RESULT(yes)
else
  CFLAGS="$CFLAGS -DNDEBUG -O2"
  AC_MSG_RESULT(no)
fi

# Enabling/disabling experimental code
AC_MSG_CHECKING(whether to enable experimental code)
AC_ARG_ENABLE(experimental, [  --enable-experimental=[yes/no] turn on experimental code
              [default=yes]],, enable_experimental=yes)
if test "x$enable_experimental" = "xno"; then
    AC_MSG_RESULT(no)
else
    CFLAGS="$CFLAGS -DEXPERIMENTAL_ENABLED"
    AC_MSG_RESULT(yes)
fi

# Enabling/disabling database support
AC_MSG_CHECKING(whether to enable database support)
AC_ARG_ENABLE(database, [  --enable-database=[yes/no] turn on database support
              [default=yes]],, enable_database=yes)
if test "x$enable_database" = "xno"; then
    AC_MSG_RESULT(no)
else
    CFLAGS="$CFLAGS -DDATABASE_ENABLED"
    AC_MSG_RESULT(yes)
fi

# Enabling/disabling java client build
AC_ARG_ENABLE(java, [  --enable-java=[yes/no] turn on java applet build
              [default=yes]],, enable_java=yes)

if test "x$enable_java" != "xno"; then
	AM_CONDITIONAL(BUILD_APPLET, true)
else
	AM_CONDITIONAL(BUILD_APPLET, false)
fi

# Enabling/disabling fakewww build
AC_ARG_ENABLE(fakewww, [  --enable-fakewww=[yes/no] turn on fakewww build
              [default=no]],, enable_fakewww=no_def)

if test "x$enable_fakewww" != "xyes"; then
	AM_CONDITIONAL(BUILD_FAKEWWW, false)
else
	AM_CONDITIONAL(BUILD_FAKEWWW, true)
fi

# Using certificate to sign java applet
AC_ARG_WITH(cert,
    		AC_HELP_STRING([--with-cert=<file>],
				[certificate file to be used to sign java applet]),
    		[cert="$withval"]
)

AC_ARG_WITH(alias,
    		AC_HELP_STRING([--with-alias=<alias>],
				[alias to be used during java applet signing (NOTE: specifying alias is required when using --with-cert option]),
    		[alias="$withval"]
)

if test "$with_cert" != "no"; then
	CERTIFICATE_FILE=$with_cert
	ALIAS=$with_alias
else
	CERTIFICATE_FILE=""
	ALIAS=$with_alias 
fi

AC_SUBST(CERTIFICATE_FILE)
AC_SUBST(ALIAS)
	

# Enabling/disabling flash client build
AC_ARG_ENABLE(flash, [  --enable-flash=[yes/no] turn on flash client build
              [default=yes]],, enable_flash=yes)

if test "x$enable_flash" != "xno"; then
	AM_CONDITIONAL(BUILD_FLASH_CLT, true)
else
	AM_CONDITIONAL(BUILD_FLASH_CLT, false)
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
I2UTILLIBMAKE=""
I2UTILINCS=""
if test "$with_I2util" != "no"; then
	# first, check for directory given.
	if test "$with_I2util" != "yes"; then
		I2util_dir=${with_I2util}
		case $I2util_dir in
			/*) ;; # already an absolute path
			*) I2util_dir="`pwd`/$I2util_dir" ;;
		esac
		I2util_include_dir=`dirname $with_I2util`/include
		I2UTILINCS="-I$I2util_include_dir $I2UTILINCS"
		I2UTILLDFLAGS="-L$I2util_dir $I2UTILLDFLAGS"
		I2UTILLIBDEPS="$I2util_dir/libI2util.a"
	# now, check for sub-build/sub-configure
	elif test -d I2util/I2util; then
		AC_CONFIG_SUBDIRS(I2util)
		TOP_BUILD_DIRS="I2util $TOP_BUILD_DIRS"
		I2util_dir='${top_srcdir}/I2util'
		I2UTILINCS="-I$I2util_dir $I2UTILINCS"
		I2UTILLDFLAGS="-L$I2util_dir/I2util $I2UTILLDFLAGS"
		I2UTILLIBDEPS="$I2util_dir/I2util/libI2util.a"
    I2UTILLIBMAKE="cd $I2util_dir; make"
		
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
AC_SUBST(I2UTILLIBMAKE)
AC_SUBST(I2UTILINCS)

AC_CHECK_LIB([web100], [web100_snap],
             [
              LINKED_WEB100LIB="-lweb100"
              INCLUDED_WEB100LIB="-I/usr/local/include/web100"
              AC_DEFINE(HAVE_LIBWEB100, 1, [Define to 1 if you have the 'Web100' library (-lweb100).])
              AM_CONDITIONAL(HAVE_WEB100, true)
              ],
              [
               LINKED_WEB100LIB=""
               INCLUDED_WEB100LIB=""
               AM_CONDITIONAL(HAVE_WEB100, false)
               ])

AC_CHECK_LIB([estats], [estats_record_open],
             [
              LINKED_WEB10GLIB="-lestats"
              INCLUDED_WEB10GLIB="-I/usr/local/include/estats"
              AC_DEFINE(HAVE_LIBWEB10G, 1, [Define to 1 if you have the 'Web10G' library (-lestats >=2.0.6.2).])
              AM_CONDITIONAL(HAVE_WEB10G, true)
             ],
             [
              LINKED_WEB10GLIB=""
              INCLUDED_WEB10GLIB=""
              AM_CONDITIONAL(HAVE_WEB10G, false)
            ])

AC_CHECK_LIB([pcap], [pcap_open_live],
             [
              LINKED_PCAPLIB="-lpcap"
              INCLUDED_PCAPLIB="-I/usr/include/pcap"
              AC_DEFINE(HAVE_LIBPCAP, 1, [Define to 1 if you have the 'pcap' library (-lpcap).])
              ],
              [
               LINKED_PCAPLIB=""
               INCLUDED_PCAPLIB=""
               ])

AC_CHECK_HEADER(pcap.h, [] [a=0])
if test $a == 0
then
	AC_DEFINE(HAVE_PCAP_H, 1, [Have PCAP header file.])
	AM_CONDITIONAL(HAVE_PCAP_H, true)
else
	AM_CONDITIONAL(HAVE_PCAP_H, false)
fi

AC_CHECK_HEADER(jansson.h, [] [b=0])
if test $b == 0
then
	JSONLIB="-ljansson"
	AC_DEFINE(HAVE_JANSSON_H, 1, [Have JANSSON header file.])
	AM_CONDITIONAL(HAVE_JANSSON_H, true)
else
	AM_CONDITIONAL(HAVE_JANSSON_H, false)
fi

AC_CHECK_LIB([z], [deflateInit_],
             [
              ZLIB="-lz"
              AC_DEFINE(HAVE_ZLIB, 1, [Define to 1 if you have the 'zlib' library (-lz).])
              AM_CONDITIONAL(HAVE_Z, true)
              ],
              [
               ZLIB=""
               INCLUDED_ZLIB=""
               AM_CONDITIONAL(HAVE_Z, false)
               ])

AC_CHECK_HEADER(zlib.h, [], [a=0])
if test $a == 0
then
	AM_CONDITIONAL(HAVE_ZLIB_H, true)
	AC_DEFINE(HAVE_ZLIB_H, 1, [Have ZLIB header file.])
else
	AM_CONDITIONAL(HAVE_ZLIB_H, false)
fi

AC_CHECK_LIB([odbc], [SQLDriverConnect],
             [
              LINKED_ODBCLIB="-lodbc"
              AC_DEFINE(HAVE_ODBC, 1, [Define to 1 if you have the 'odbc' library (-lodbc).])
              ],
              [
               LINKED_ODBCLIB=""
               ])


NDTINCDIR='$(INCLUDED_WEB100LIB) $(INCLUDED_PCAPLIB) $(INCLUDED_WEB10GLIB)'
NDTLIBDIR=/usr/local/lib
NDTLDFLAGS='-L$(NDTLIBDIR) -Wl,-rpath,$(NDTLIBDIR)'
NDTLIBS='$(LINKED_WEB100LIB) $(LINKED_PCAPLIB) $(LINKED_ODBCLIB) $(LINKED_WEB10GLIB) -lm'
NDTINCS='-I$(NDTINCDIR)'

AC_SUBST(LINKED_WEB100LIB)
AC_SUBST(INCLUDED_WEB100LIB)
AC_SUBST(LINKED_WEB10GLIB)
AC_SUBST(INCLUDED_WEB10GLIB)
AC_SUBST(LINKED_PCAPLIB)
AC_SUBST(INCLUDED_PCAPLIB)
AC_SUBST(LINKED_ODBCLIB)
AC_SUBST(NDTINCS)
AC_SUBST(NDTLDFLAGS)
AC_SUBST(NDTLIBS)
AC_SUBST(NDTLIBDIR)
AC_SUBST(NDTINCDIR)
AC_SUBST(ZLIB)
AC_SUBST(JSONLIB)

AC_SUBST(TOP_BUILD_DIRS)

#
# Java config files
#
GCJ=javac
NDTGCJFLAGS="-source 1.4"
GCJFLAGS="-source 1.4"

AC_SUBST(NDTGCJFLAGS)

NDTJAR=jar
NDTJARFLAG='-cmf'
NDTJARSIGNER=jarsigner

AC_SUBST(NDTJAR)
AC_SUBST(NDTJARFLAG)
AC_SUBST(NDTJARSIGNER)

# Flash application compiler
NDTMXMLC=mxmlc

AC_SUBST(NDTMXMLC)

# Checks for programs.
AC_PROG_CC
AC_PROG_LN_S
AM_PROG_GCJ
AC_CHECK_PROGS(SGCJ, javac)
AC_CHECK_PROGS(MXMLC, mxmlc)
AM_CONDITIONAL(HAVE_GCJ, test -n "$SGCJ")
AM_CONDITIONAL(HAVE_MXMLC, test -n "$MXMLC")

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h linux/sockios.h net/ethernet.h netinet/ip6.h linux/icmpv6.h sql.h])

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
		conf/Makefile doc/Makefile janalyze/Makefile flash-client/Makefile])
AC_OUTPUT

echo ""
echo ""
echo "***********************************"
echo "***   Network Diagnostic Tool   ***"
echo "***********************************"
echo "*       configured modules        *"
echo "***********************************"
echo ""
echo ""

if test -z "$HAVE_WEB10G_TRUE" && test -n "$HAVE_WEB10G_FALSE"; then
if test -z "$HAVE_PCAP_H_TRUE" && test -n "$HAVE_PCAP_H_FALSE"; then
if test -z "$HAVE_JANSSON_H_TRUE" && test -n "$HAVE_JANSSON_H_FALSE"; then
  SUMMARY_WEB10GSRV="YES"
  SUMMARY_GENPLOT10G="YES"
else
  SUMMARY_GENPLOT10G="YES"
  SUMMARY_WEB10GSRV="NO    (missing jansson headers)"
fi
else
  SUMMARY_GENPLOT10G="YES"
  SUMMARY_WEB10GSRV="NO    (missing pcap headers)"
fi
else
  SUMMARY_WEB10GSRV="NO (missing web10g userland library >=2.0.6.2)"
  SUMMARY_GENPLOT10G="NO (missing web10g userland library >=2.0.6.2)"
fi

if test -z "$HAVE_JANSSON_H_TRUE" && test -n "$HAVE_JANSSON_H_FALSE"; then
  SUMMARY_WEB100CLT="YES"
else
  SUMMARY_WEB100CLT="NO    (missing jansson headers)"
fi

if test -z "$HAVE_WEB100_TRUE" && test -n "$HAVE_WEB100_FALSE"; then
if test -z "$HAVE_PCAP_H_TRUE" && test -n "$HAVE_PCAP_H_FALSE"; then
if test -z "$HAVE_JANSSON_H_TRUE" && test -n "$HAVE_JANSSON_H_FALSE"; then
SUMMARY_WEB100SRV="YES"
else
SUMMARY_WEB100SRV="NO    (missing jansson headers)"
fi
else
SUMMARY_WEB100SRV="NO    (missing pcap headers)"
fi
if test "x$enable_fakewww" == "xno"; then
SUMMARY_FAKEWWW="NO    (disabled by user)"
elif test "x$enable_fakewww" != "xyes"; then
SUMMARY_FAKEWWW="NO    (disabled by default)"
else
SUMMARY_FAKEWWW="YES"
fi
SUMMARY_ANALYZE="YES"
SUMMARY_VIEWTRACE="YES"
SUMMARY_TRMKMAP="YES"
SUMMARY_GENPLOT="YES"
else
SUMMARY_WEB100SRV="NO    (missing web100 library)"
SUMMARY_FAKEWWW="NO    (missing web100 library)"
SUMMARY_ANALYZE="NO    (missing web100 library)"
SUMMARY_VIEWTRACE="NO    (missing web100 library)"
SUMMARY_TRMKMAP="NO    (missing web100 library)"
SUMMARY_GENPLOT="NO    (missing web100 library)"
fi

if test -z "$HAVE_GCJ_TRUE" && test -n "$HAVE_GCJ_FALSE"; then
SUMMARY_ADMINCLASS="YES"
if test "x$enable_java" != "xno"; then
SUMMARY_TCPBW100JAR="YES"
else
SUMMARY_TCPBW100JAR="NO    (disabled by user)"
fi
SUMMARY_JANALYZEJAR="YES"
else
SUMMARY_ADMINCLASS="NO    (missing java compiler)"
SUMMARY_TCPBW100JAR="NO    (missing java compiler)"
SUMMARY_JANALYZEJAR="NO    (missing java compiler)"
fi

if test -z "$HAVE_MXMLC_TRUE" && test -n "$HAVE_MXMLC_FALSE"; then
if test "x$enable_flash" != "xno"; then
SUMMARY_FLASHCLT="YES"
else
SUMMARY_FLASHCLT="NO (disabled by user)"
fi
else
SUMMARY_FLASHCLT="NO (missing action script compiler)"
fi

if test "$SUMMARY_WEB100CLT" = "YES" && test "$SUMMARY_FLASHCLT" = "YES" && test "$SUMMARY_TCPBW100JAR" = "YES"; then
echo "*** Client Tools - complete"
else
echo "*** Client Tools - incomplete"
fi
echo "* web100clt:    ${SUMMARY_WEB100CLT}"
echo "* Tcpbw100.jar: ${SUMMARY_TCPBW100JAR}"
echo "* flash-client: ${SUMMARY_FLASHCLT}"
echo ""

if test "$SUMMARY_WEB100SRV" = "YES" || test "$SUMMARY_WEB100GSRV" = "YES"; then
echo "*** Server Tools - complete"
else
echo "*** Server Tools - incomplete"
fi
echo "* fakewww:      ${SUMMARY_FAKEWWW}"
echo "* web100srv:    ${SUMMARY_WEB100SRV}"
echo "* web10gsrv:    ${SUMMARY_WEB10GSRV}"
echo ""

if test "$SUMMARY_ANALYZE" = "YES" && test "$SUMMARY_VIEWTRACE" = "YES" && test "$SUMMARY_TRMKMAP" = "YES" && test "$SUMMARY_GENPLOT" = "YES" && test "$SUMMARY_JANALYZEJAR" = "YES" && test "$SUMMARY_ADMINCLASS" = "YES"; then
echo "*** Administrative Tools - complete"
else
echo "*** Administrative Tools - incomplete"
fi
echo "* analyze:      ${SUMMARY_ANALYZE}"
echo "* viewtrace:    ${SUMMARY_VIEWTRACE}"
echo "* tr-mkmap:     ${SUMMARY_TRMKMAP}"
echo "* genplot:      ${SUMMARY_GENPLOT}"
echo "* genplot10g:   ${SUMMARY_GENPLOT10G}"
echo "* JAnalyze.jar: ${SUMMARY_JANALYZEJAR}"
echo "* Admin.class:  ${SUMMARY_ADMINCLASS}"
echo ""
echo ""


