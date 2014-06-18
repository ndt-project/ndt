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

if BUILD_FAKEWWW
ADD_FAKEWWW = fakewww
endif

if HAVE_WEB100
bin_PROGRAMS = web100clt analyze viewtrace tr-mkmap genplot
if HAVE_PCAP_H
sbin_PROGRAMS = $(ADD_FAKEWWW) web100srv
else
sbin_PROGRAMS = $(ADD_FAKEWWW)
endif
else
bin_PROGRAMS = web100clt
endif

if HAVE_WEB10G
if HAVE_PCAP_H
if HAVE_WEB100
sbin_PROGRAMS += web10gsrv genplot10g
else
sbin_PROGRAMS = $(ADD_FAKEWWW) web10gsrv genplot10g
endif
endif
endif

web100clt_SOURCES = web100clt.c network.c usage.c logging.c utils.c protocol.c runningtest.c ndtptestconstants.c \
                    test_sfw_clt.c test_mid_clt.c test_c2s_clt.c test_s2c_clt.c test_meta_clt.c strlutils.c \
                    test_results_clt.c jsonutils.c
web100clt_LDADD = $(I2UTILLIBDEPS) -lpthread $(ZLIB) $(JSONLIB)
web100clt_CPPFLAGS ='-DBASEDIR="$(ndtdir)"'
web100clt_DEPENDENCIES = $(I2UTILLIBDEPS)

genplot_SOURCES = genplot.c usage.c
genplot_LDADD = $(NDTLIBS) $(I2UTILLIBDEPS)
genplot_CPPFLAGS ='-DBASEDIR="$(ndtdir)"' -DFORCE_WEB100

genplot10g_SOURCES = genplot.c usage.c web10g-util.c utils.c
genplot10g_LDADD = $(NDTLIBS) $(I2UTILLIBDEPS)
genplot10g_CPPFLAGS ='-DBASEDIR="$(ndtdir)"'

analyze_SOURCES = analyze.c usage.c logging.c runningtest.c ndtptestconstants.c strlutils.c
analyze_LDADD = $(NDTLIBS) $(I2UTILLIBDEPS) $(ZLIB)
analyze_CPPFLAGS ='-DBASEDIR="$(ndtdir)"' -DFORCE_WEB100

if BUILD_FAKEWWW
fakewww_SOURCES = fakewww.c troute.c troute6.c tr-tree.c tr-tree6.c network.c usage.c logging.c \
                  runningtest.c ndtptestconstants.c strlutils.c jsonutils.c
fakewww_LDADD = $(I2UTILLIBDEPS) $(ZLIB) $(JSONLIB)
fakewww_CPPFLAGS ='-DBASEDIR="$(ndtdir)"'
endif

web100srv_SOURCES = web100srv.c web100-util.c web100-pcap.c web100-admin.c runningtest.c \
                    network.c usage.c utils.c mrange.c logging.c testoptions.c ndtptestconstants.c \
                    protocol.c test_sfw_srv.c test_meta_srv.c ndt_odbc.c strlutils.c heuristics.c \
                    test_c2s_srv.c test_s2c_srv.c test_mid_srv.c jsonutils.c
web100srv_LDFLAGS = $(NDTLDFLAGS) $(I2UTILLDFLAGS)
web100srv_LDADD = $(NDTLIBS) $(I2UTILLIBS) $(I2UTILLIBDEPS) -lpthread $(ZLIB) $(JSONLIB)
web100srv_CPPFLAGS ='-DBASEDIR="$(ndtdir)"' -DFORCE_WEB100
web100srv_DEPENDENCIES = $(I2UTILLIBDEPS)

web10gsrv_SOURCES = web100srv.c web100-util.c web100-pcap.c web100-admin.c runningtest.c \
		    network.c usage.c utils.c mrange.c logging.c testoptions.c ndtptestconstants.c \
		    protocol.c test_sfw_srv.c test_meta_srv.c ndt_odbc.c strlutils.c heuristics.c \
		    test_c2s_srv.c test_s2c_srv.c test_mid_srv.c web10g-util.c jsonutils.c
web10gsrv_LDFLAGS = $(NDTLDFLAGS) $(I2UTILLDFLAGS)
web10gsrv_LDADD = $(NDTLIBS) $(I2UTILLIBS) $(I2UTILLIBDEPS) -lpthread $(ZLIB) $(JSONLIB)
web10gsrv_CPPFLAGS = '-DBASEDIR="$(ndtdir)"'
web10gsrv_DEPENDENCIES = $(I2UTILLIBDEPS)

viewtrace_SOURCES = viewtrace.c usage.c logging.c utils.c runningtest.c ndtptestconstants.c strlutils.c
viewtrace_LDADD = $(NDTLIBS) $(I2UTILLIBDEPS) $(ZLIB)
viewtrace_CPPFLAGS ='-DBASEDIR="$(ndtdir)"' -DFORCE_WEB100

tr_mkmap_SOURCES = tr-mkmap.c tr-tree.c tr-tree6.c usage.c logging.c runningtest.c ndtptestconstants.c strlutils.c
tr_mkmap_LDADD = $(NDTLIBS) $(I2UTILLIBDEPS) $(ZLIB)
tr_mkmap_CPPFLAGS ='-DBASEDIR="$(ndtdir)"'

install-ln:
	@if test -f "$(prefix)/sbin/web100srv" && test ! -L "$(prefix)/sbin/ndtd"; then \
		echo '(cd $(prefix)/sbin && $(LN_S) web100srv ndtd )' ; \
		(cd $(prefix)/sbin && $(LN_S) web100srv ndtd ); \
	else :; fi
	@if test -f "$(prefix)/bin/web100clt" && test ! -L "$(prefix)/bin/ndtclt"; then \
		echo '(cd $(prefix)/bin && $(LN_S) web100clt ndtclt )' ; \
		(cd $(prefix)/bin && $(LN_S) web100clt ndtclt ); \
	else :; fi


install: install-am install-ln

$(I2UTILLIBDEPS):
	$(I2UTILLIBMAKE)

EXTRA_DIST = clt_tests.h logging.h mrange.h network.h protocol.h testoptions.h test_sfw.h test_meta.h \
             troute.h tr-tree.h usage.h utils.h varinfo.h web100-admin.h web100srv.h ndt_odbc.h runningtest.h ndtptestconstants.h \
             heuristics.h strlutils.h test_results_clt.h tests_srv.h jsonutils.h

