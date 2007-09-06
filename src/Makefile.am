#########################################################################
#                  #
#         Copyright (C)  2003        #
#             Internet2        #
#         All Rights Reserved        #
#                  #
#########################################################################
#
#  File:    Makefile.am
#
#  Author:    Jeff Boote
#      Internet2
#
#  Date:    Fri Sep 12 13:33:27 MDT 2003
#
#  Description:

INCLUDES  = $(NDTINCDIR) $(I2UTILINCS)
AM_LDFLAGS  = $(NDTLDFLAGS)
AM_LIBS    = $(I2UTILLIBS)
AM_CFLAGS  = $(NDTCFLAGS)

ndtdir = $(prefix)/ndt

if HAVE_WEB100
bin_PROGRAMS = web100clt analyze viewtrace tr-mkmap genplot
sbin_PROGRAMS = fakewww web100srv 
else
bin_PROGRAMS = web100clt
endif

web100clt_SOURCES = web100clt.c network.c usage.c logging.c utils.c protocol.c \
                    test_sfw_clt.c test_mid_clt.c test_c2s_clt.c test_s2c_clt.c
web100clt_LDADD = $(I2UTILLIBDEPS) -lpthread
web100clt_CPPFLAGS ='-DBASEDIR="$(ndtdir)"'
web100clt_DEPENDENCIES = $(I2UTILLIBDEPS)

genplot_SOURCES = genplot.c usage.c
genplot_LDADD = $(NDTLIBS) $(I2UTILLIBDEPS)
genplot_CPPFLAGS ='-DBASEDIR="$(ndtdir)"'

analyze_SOURCES = analyze.c usage.c logging.c
analyze_LDADD = $(NDTLIBS) $(I2UTILLIBDEPS)
analyze_CPPFLAGS ='-DBASEDIR="$(ndtdir)"'

fakewww_SOURCES = fakewww.c troute.c troute6.c tr-tree.c tr-tree6.c network.c usage.c logging.c
fakewww_LDADD = $(I2UTILLIBDEPS)
fakewww_CPPFLAGS ='-DBASEDIR="$(ndtdir)"'

web100srv_SOURCES = web100srv.c web100-util.c web100-pcap.c web100-admin.c \
                    network.c usage.c utils.c mrange.c logging.c testoptions.c \
                    protocol.c test_sfw_srv.c ndt_odbc.c
web100srv_LDFLAGS = $(NDTLDFLAGS) $(I2UTILLDFLAGS)
web100srv_LDADD = $(NDTLIBS) $(I2UTILLIBS) $(I2UTILLIBDEPS) -lpthread
web100srv_CPPFLAGS ='-DBASEDIR="$(ndtdir)"'
web100srv_DEPENDENCIES = $(I2UTILLIBDEPS)

viewtrace_SOURCES = viewtrace.c usage.c logging.c utils.c
viewtrace_LDADD = $(NDTLIBS) $(I2UTILLIBDEPS)
viewtrace_CPPFLAGS ='-DBASEDIR="$(ndtdir)"'

tr_mkmap_SOURCES = tr-mkmap.c tr-tree.c tr-tree6.c usage.c logging.c
tr_mkmap_LDADD = $(NDTLIBS) $(I2UTILLIBDEPS)
tr_mkmap_CPPFLAGS ='-DBASEDIR="$(ndtdir)"'

$(I2UTILLIBDEPS):
	$(I2UTILLIBMAKE)

EXTRA_DIST = clt_tests.h logging.h mrange.h network.h protocol.h testoptions.h test_sfw.h \
             troute.h tr-tree.h usage.h utils.h varinfo.h web100-admin.h web100srv.h