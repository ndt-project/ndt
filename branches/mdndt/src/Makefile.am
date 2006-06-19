#########################################################################
#									#
#			   Copyright (C)  2003				#
#	     			Internet2				#
#			   All Rights Reserved				#
#									#
#########################################################################
#
#	File:		Makefile.am
#
#	Author:		Jeff Boote
#			Internet2
#
#	Date:		Fri Sep 12 13:33:27 MDT 2003
#
#	Description:

INCLUDES	= $(NDTINCDIR) $(I2UTILINCS)
AM_LDFLAGS	= $(NDTLDFLAGS)
AM_LIBS		= $(I2UTILLIBS)
AM_CFLAGS	= $(NDTCFLAGS)

ndtdir = $(prefix)/ndt

bin_PROGRAMS	= analyze viewtrace tr-mkmap web100clt
sbin_PROGRAMS	= fakewww web100srv 

analyze_SOURCES	= analyze.c usage.c logging.c
analyze_LDADD = $(NDTLIBS) $(I2UTILLIBDEPS)
analyze_CPPFLAGS ='-DBASEDIR="$(ndtdir)"'

fakewww_SOURCES	= fakewww.c troute.c troute6.c tr-tree.c tr-tree6.c network.c usage.c logging.c
fakewww_LDADD = $(I2UTILLIBDEPS) $(I2UTILLIBDEPS)
fakewww_CPPFLAGS ='-DBASEDIR="$(ndtdir)"'

web100srv_SOURCES	= web100srv.c web100-util.c web100-pcap.c web100-admin.c \
										network.c usage.c utils.c mrange.c logging.c
web100srv_LDFLAGS = $(NDTLDFLAGS) $(I2UTILLDFLAGS)
web100srv_LDADD = $(NDTLIBS) $(I2UTILLIBS) $(I2UTILLIBDEPS)
web100srv_CPPFLAGS ='-DBASEDIR="$(ndtdir)"'
web100srv_DEPENDENCIES = $(I2UTILLIBDEPS)

viewtrace_SOURCES	= viewtrace.c usage.c logging.c
viewtrace_LDADD = $(NDTLIBS) $(I2UTILLIBDEPS)
viewtrace_CPPFLAGS ='-DBASEDIR="$(ndtdir)"'

web100clt_SOURCES	= web100clt.c network.c usage.c logging.c
web100clt_LDADD = $(I2UTILLIBDEPS) $(I2UTILLIBDEPS)
web100clt_CPPFLAGS ='-DBASEDIR="$(ndtdir)"'

tr_mkmap_SOURCES	= tr-mkmap.c tr-tree.c tr-tree6.c usage.c logging.c
tr_mkmap_LDADD = $(NDTLIBS) $(I2UTILLIBDEPS)
tr_mkmap_CPPFLAGS ='-DBASEDIR="$(ndtdir)"'

