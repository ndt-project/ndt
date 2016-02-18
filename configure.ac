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
AC_INIT([NDT],[4.0.0.1],[ndt-user@internet2.edu])
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

# Enabling/disabling extended tests code
AC_MSG_CHECKING(whether to enable extended tests code)
AC_ARG_ENABLE(exttests, [  --enable-exttests=[yes/no] turn on extended tests code
              [default=yes]],, enable_exttests=yes)
if test "x$enable_exttests" = "xno"; then
    AC_MSG_RESULT(no)
else
    CFLAGS="$CFLAGS -DEXTTESTS_ENABLED"
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

AC_ARG_WITH([java],
  [AS_HELP_STRING([--with-java],
    [Enable java client @<:@default=auto@:>@ (yes/no/precompiled/auto)])],
  [],
  [with_java=auto])

if test "x$with_java" == "xauto"; then
    AC_CHECK_PROGS(JAVAC, javac)
    if test -n "$JAVAC"; then
        with_java=yes
    else
        with_java=precompiled
    fi
fi

if test "x$with_java" == "xyes"; then
    AC_CHECK_PROGS(JAVAC, javac)
    if test -z "$JAVAC"; then
      AC_MSG_FAILURE([javac required to rebuild java applet but not available])
    fi
fi

if test "x$with_java" != "xno"; then
	AM_CONDITIONAL(INCLUDE_APPLET, true)
else
	AM_CONDITIONAL(INCLUDE_APPLET, false)
fi

if test "x$with_java" != "xprecompiled"; then
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
AC_ARG_WITH(flash, [  --enable-flash=[yes/no/precompiled/auto] turn on flash client build
              [default=auto]],, with_flash=auto)

if test "x$with_flash" == "xauto"; then
    AC_CHECK_PROGS(NDTMXMLC, mxmlc)
    if test -n "$NDTMXMLC"; then
        with_flash=yes
    else
        with_flash=precompiled
    fi
fi

if test "x$with_flash" == "xyes"; then
    AC_CHECK_PROGS(NDTMXMLC, mxmlc)
    if test -z "$NDTMXMLC"; then
      AC_MSG_FAILURE([mxmlc required to rebuild flash client but not available])
    fi
fi

if test "x$with_flash" != "xno"; then
	AM_CONDITIONAL(INCLUDE_FLASH, true)
else
	AM_CONDITIONAL(INCLUDE_FLASH, false)
fi

if test "x$with_flash" != "xprecompiled"; then
	AM_CONDITIONAL(BUILD_FLASH, true)
else
	AM_CONDITIONAL(BUILD_FLASH, false)
fi


AC_ARG_WITH(I2util,
		AC_HELP_STRING([--with-I2util=<dir>],
				[defaults to building I2util under NDT if exists]),
		with_I2util=$withval, with_I2util=yes)		

#
# find I2util
#
I2UTILLDFLAGS=""
I2UTILLIBS=""
I2UTILLIBDEPS=""
I2UTILLIBMAKE=""
I2UTILINCS=""

if test -n "$with_I2util" && test "$with_I2util" != "yes"; then
	I2util_dir=${with_I2util}
	case $I2util_dir in
		/*) ;; # already an absolute path
		*) I2util_dir="`pwd`/$I2util_dir" ;;
	esac
	I2util_include_dir=`dirname $with_I2util`/include
	I2UTILINCS="-I$I2util_include_dir $I2UTILINCS"
	I2UTILLDFLAGS="-L$I2util_dir $I2UTILLDFLAGS"
	I2UTILLIBDEPS="$I2util_dir/libI2util.a"
fi

# Save the old CFLAGS and LDFLAGS
OLD_LDFLAGS="$LDFLAGS"
OLD_CFLAGS="$CFLAGS"

CFLAGS="$CFLAGS $I2UTILINCS"
LDFLAGS="$CFLAGS $I2UTILLDFLAGS"

AC_SEARCH_LIBS([I2AddrByNode],I2util, HAVE_I2UTIL_LIBS=1,HAVE_I2UTIL_LIBS=0)
AC_CHECK_HEADERS([I2util/util.h I2util/conf.h], HAVE_I2UTIL_HEADERS=1,HAVE_I2UTIL_HEADERS=0)

LDFLAGS="$OLD_LDFLAGS"
CFLAGS="$OLD_CFLAGS"

if test "$HAVE_I2UTIL_HEADERS" != "1" || test "$HAVE_I2UTIL_LIBS" != "1"; then
	# now, check for sub-build/sub-configure
	if test -d I2util/I2util; then
		AC_CONFIG_SUBDIRS(I2util)
		TOP_BUILD_DIRS="I2util $TOP_BUILD_DIRS"
		I2util_dir='${top_srcdir}/I2util'
		I2UTILINCS="-I$I2util_dir $I2UTILINCS"
		I2UTILLDFLAGS="-L$I2util_dir/I2util $I2UTILLDFLAGS"
		I2UTILLIBDEPS="$I2util_dir/I2util/libI2util.a"
		I2UTILLIBMAKE="cd $I2util_dir; make"
	else
		AC_MSG_FAILURE([I2util required to build NDT])
	fi

        # Save the old CFLAGS and LDFLAGS
	OLD_LDFLAGS="$LDFLAGS"
	OLD_CFLAGS="$CFLAGS"

        CFLAGS="$CFLAGS $I2UTILINCS"
        LDFLAGS="$CFLAGS $I2UTILLDFLAGS"

        AC_SEARCH_LIBS([I2AddrByNode],I2util, ,AC_MSG_ERROR([Couldn't find I2util library]))
        AC_CHECK_HEADERS([I2util/util.h I2util/conf.h], ,AC_MSG_ERROR([Couldn't find I2util header files]), [AC_INCLUDES_DEFAULT])

	LDFLAGS="$OLD_LDFLAGS"
	CFLAGS="$OLD_CFLAGS"

	I2UTILLIBS="$I2UTILLDFLAGS -lI2util"
fi

I2UTILLIBS="$I2UTILLDFLAGS -lI2util"

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

AC_CHECK_LIB([jansson], [json_true],
             [
              JSONLIB="-ljansson"
              AC_DEFINE(HAVE_JANSSON, 1, [Define to 1 if you have the 'jansson' library (-ljansson).])
              AM_CONDITIONAL(HAVE_JANSSON, true)
              ],
              [
               JSONLIB=""
               AM_CONDITIONAL(HAVE_JANSSON, false)
               ])

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
AX_CHECK_OPENSSL(AM_CONDITIONAL(HAVE_SSL, true), AM_CONDITIONAL(HAVE_SSL, false))

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
		conf/Makefile doc/Makefile janalyze/Makefile flash-client/Makefile HTML5-frontend/Makefile])
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
if test -z "$HAVE_JANSSON_TRUE" && test -n "$HAVE_JANSSON_FALSE"; then
if test -z "$HAVE_SSL_TRUE" && test -n "$HAVE_SSL_FALSE"; then
  SUMMARY_WEB10GSRV="YES"
  SUMMARY_GENPLOT10G="YES"
else
  SUMMARY_GENPLOT10G="YES"
  SUMMARY_WEB10GSRV="NO    (missing OpenSSL library)"
fi
else
  SUMMARY_GENPLOT10G="YES"
  SUMMARY_WEB10GSRV="NO    (missing jansson library)"
fi
else
  SUMMARY_GENPLOT10G="YES"
  SUMMARY_WEB10GSRV="NO    (missing pcap headers)"
fi
else
  SUMMARY_WEB10GSRV="NO (missing web10g userland library >=2.0.6.2)"
  SUMMARY_GENPLOT10G="NO (missing web10g userland library >=2.0.6.2)"
fi

if test -z "$HAVE_SSL_TRUE" && test -n "$HAVE_SSL_FALSE"; then
if test -z "$HAVE_JANSSON_TRUE" && test -n "$HAVE_JANSSON_FALSE"; then
  SUMMARY_WEB100CLT="YES"
else
  SUMMARY_WEB100CLT="NO    (missing jansson library)"
fi
else
  SUMMARY_WEB100CLT="NO    (missing OpenSSL library)"
fi

if test "x$enable_fakewww" == "xno"; then
SUMMARY_FAKEWWW="NO    (disabled by user)"
elif test "x$enable_fakewww" != "xyes"; then
SUMMARY_FAKEWWW="NO    (disabled by default)"
else
SUMMARY_FAKEWWW="YES"
fi

if test -z "$HAVE_WEB100_TRUE" && test -n "$HAVE_WEB100_FALSE"; then
if test -z "$HAVE_PCAP_H_TRUE" && test -n "$HAVE_PCAP_H_FALSE"; then
if test -z "$HAVE_JANSSON_TRUE" && test -n "$HAVE_JANSSON_FALSE"; then
if test -z "$HAVE_SSL_TRUE" && test -n "$HAVE_SSL_FALSE"; then
SUMMARY_WEB100SRV="YES"
else
SUMMARY_WEB100SRV="NO    (missing OpenSSL library)"
fi
else
SUMMARY_WEB100SRV="NO    (missing jansson library)"
fi
else
SUMMARY_WEB100SRV="NO    (missing pcap headers)"
fi
SUMMARY_ANALYZE="YES"
SUMMARY_VIEWTRACE="YES"
SUMMARY_TRMKMAP="YES"
SUMMARY_GENPLOT="YES"
else
SUMMARY_WEB100SRV="NO    (missing web100 library)"
SUMMARY_ANALYZE="NO    (missing web100 library)"
SUMMARY_VIEWTRACE="NO    (missing web100 library)"
SUMMARY_TRMKMAP="NO    (missing web100 library)"
SUMMARY_GENPLOT="NO    (missing web100 library)"
fi

if test "x$with_java" == "xno"; then
SUMMARY_TCPBW100JAR="NO"
else
SUMMARY_TCPBW100JAR="YES"
fi

if test -z "$HAVE_GCJ_TRUE" && test -n "$HAVE_GCJ_FALSE"; then
SUMMARY_ADMINCLASS="YES"
SUMMARY_JANALYZEJAR="YES"
else
SUMMARY_ADMINCLASS="NO    (missing java compiler)"
SUMMARY_JANALYZEJAR="NO    (missing java compiler)"
fi

if test "x$with_flash" != "xno"; then
SUMMARY_FLASHCLT="YES"
else
SUMMARY_FLASHCLT="NO"
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


