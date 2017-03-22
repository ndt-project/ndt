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

bin_PROGRAMS =
sbin_PROGRAMS = $(ADD_FAKEWWW)
noinst_PROGRAMS =
TESTS =

if HAVE_WEB100
if HAVE_SSL
if HAVE_JANSSON
TESTS += web100_websocket_unit_tests web100_testoptions_unit_tests
endif
endif
endif

if HAVE_WEB10G
if HAVE_SSL
if HAVE_JANSSON
TESTS += web10g_websocket_unit_tests web10g_testoptions_unit_tests
endif
endif
endif


if HAVE_SSL
if HAVE_JANSSON
bin_PROGRAMS += web100clt
endif
endif

if HAVE_WEB100
bin_PROGRAMS += analyze viewtrace tr-mkmap genplot
if HAVE_PCAP_H
if HAVE_SSL
if HAVE_JANSSON
sbin_PROGRAMS += web100srv
endif
endif
endif
endif

if HAVE_WEB10G
bin_PROGRAMS += genplot10g
if HAVE_PCAP_H
if HAVE_SSL
if HAVE_JANSSON
sbin_PROGRAMS += web10gsrv
endif
endif
endif
endif

noinst_PROGRAMS += $(TESTS)

web100clt_SOURCES = web100clt.c network.c network_clt.c usage.c logging.c utils.c protocol.c runningtest.c ndtptestconstants.c \
                    test_sfw_clt.c test_mid_clt.c test_c2s_clt.c test_s2c_clt.c test_meta_clt.c strlutils.c \
                    test_results_clt.c jsonutils.c websocket.c
web100clt_LDADD = $(I2UTILLIBDEPS) -lpthread $(ZLIB) $(JSONLIB) $(OPENSSL_LDFLAGS) $(OPENSSL_LIBS)
web100clt_CPPFLAGS ='-DBASEDIR="$(ndtdir)"' $(OPENSSL_INCLUDES)
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
fakewww_SOURCES = fakewww.c troute.c troute6.c tr-tree.c tr-tree6.c network.c network_clt.c usage.c logging.c \
                  runningtest.c ndtptestconstants.c strlutils.c jsonutils.c websocket.c
fakewww_LDADD = $(I2UTILLIBDEPS) $(ZLIB) $(JSONLIB) $(OPENSSL_LDFLAGS) $(OPENSSL_LIBS)
fakewww_CPPFLAGS ='-DBASEDIR="$(ndtdir)"' $(OPENSSL_INCLUDES)
endif

web100srv_SOURCES = web100srv.c web100-util.c web100-pcap.c web100-admin.c runningtest.c \
                    network.c usage.c utils.c mrange.c logging.c testoptions.c ndtptestconstants.c \
                    protocol.c test_sfw_srv.c test_meta_srv.c ndt_odbc.c strlutils.c heuristics.c \
                    test_c2s_srv.c test_s2c_srv.c test_mid_srv.c testutils.c jsonutils.c websocket.c
web100srv_LDFLAGS = $(NDTLDFLAGS) $(I2UTILLDFLAGS)
web100srv_LDADD = $(NDTLIBS) $(I2UTILLIBS) $(I2UTILLIBDEPS) -lpthread $(ZLIB) $(JSONLIB) $(OPENSSL_LDFLAGS) $(OPENSSL_LIBS)
web100srv_CPPFLAGS ='-DBASEDIR="$(ndtdir)"' -DFORCE_WEB100 $(OPENSSL_INCLUDES)
web100srv_DEPENDENCIES = $(I2UTILLIBDEPS)

web100srv_unit_tests_SOURCES = web100srv_unit_tests.c unit_testing.c $(web100srv_SOURCES)
web100srv_unit_tests_LDFLAGS = $(NDTLDFLAGS) $(I2UTILLDFLAGS)
web100srv_unit_tests_LDADD = $(NDTLIBS) $(I2UTILLIBS) $(I2UTILLIBDEPS) -lpthread $(ZLIB) $(JSONLIB) $(OPENSSL_LDFLAGS) $(OPENSSL_LIBS)
web100srv_unit_tests_CPPFLAGS ='-DBASEDIR="$(ndtdir)"' -DFORCE_WEB100 -DUSE_WEB100SRV_ONLY_AS_LIBRARY -Wall -Wno-unused-variable -Wno-unused-function $(OPENSSL_INCLUDES)
web100srv_unit_tests_DEPENDENCIES = $(I2UTILLIBDEPS)

web100_websocket_unit_tests_SOURCES = unit_testing.c websocket_unit_tests.c websocket.c \
                               network.c logging.c strlutils.c jsonutils.c ndtptestconstants.c runningtest.c
web100_websocket_unit_tests_LDFLAGS = $(NDTLDFLAGS) $(I2UTILLDFLAGS)
web100_websocket_unit_tests_LDADD = $(NDTLIBS) $(I2UTILLIBS) $(I2UTILLIBDEPS) -lpthread $(ZLIB) $(JSONLIB) $(OPENSSL_LDFLAGS) $(OPENSSL_LIBS)
web100_websocket_unit_tests_CPPFLAGS ='-DBASEDIR="$(ndtdir)"' -DFORCE_WEB100 -Wall -Wno-unused-variable  -Wno-unused-function $(OPENSSL_INCLUDES)
web100_websocket_unit_tests_DEPENDENCIES = $(I2UTILLIBDEPS)

web100_testoptions_unit_tests_SOURCES = testoptions_unit_tests.c testoptions.c unit_testing.c \
                                 heuristics.c jsonutils.c logging.c mrange.c ndt_odbc.c ndtptestconstants.c \
                                 network.c protocol.c runningtest.c strlutils.c test_c2s_srv.c test_meta_srv.c \
                                 test_mid_srv.c test_s2c_srv.c test_sfw_srv.c testutils.c utils.c web100-pcap.c \
                                 web100-util.c web100srv.c websocket.c
web100_testoptions_unit_tests_LDFLAGS = $(NDTLDFLAGS) $(I2UTILLDFLAGS)
web100_testoptions_unit_tests_LDADD = $(NDTLIBS) $(I2UTILLIBS) $(I2UTILLIBDEPS) -lpthread $(ZLIB) $(JSONLIB) $(OPENSSL_LDFLAGS) $(OPENSSL_LIBS)
web100_testoptions_unit_tests_CPPFLAGS ='-DBASEDIR="$(ndtdir)"' -DFORCE_WEB100 -DUSE_WEB100SRV_ONLY_AS_LIBRARY -Wall -Wno-unused-variable -Wno-unused-function $(OPENSSL_INCLUDES)
web100_testoptions_unit_tests_DEPENDENCIES = $(I2UTILLIBDEPS)

web10gsrv_SOURCES = web100srv.c web100-util.c web100-pcap.c web100-admin.c runningtest.c \
		    network.c usage.c utils.c mrange.c logging.c testoptions.c ndtptestconstants.c \
		    protocol.c test_sfw_srv.c test_meta_srv.c ndt_odbc.c strlutils.c heuristics.c \
		    test_c2s_srv.c test_s2c_srv.c test_mid_srv.c testutils.c web10g-util.c jsonutils.c websocket.c
web10gsrv_LDFLAGS = $(NDTLDFLAGS) $(I2UTILLDFLAGS)
web10gsrv_LDADD = $(NDTLIBS) $(I2UTILLIBS) $(I2UTILLIBDEPS) -lpthread $(ZLIB) $(JSONLIB) $(OPENSSL_LDFLAGS) $(OPENSSL_LIBS)
web10gsrv_CPPFLAGS = '-DBASEDIR="$(ndtdir)"' $(OPENSSL_INCLUDES)
web10gsrv_DEPENDENCIES = $(I2UTILLIBDEPS)

web10g_websocket_unit_tests_SOURCES = unit_testing.c websocket_unit_tests.c websocket.c \
                               network.c logging.c strlutils.c jsonutils.c ndtptestconstants.c runningtest.c
web10g_websocket_unit_tests_LDFLAGS = $(NDTLDFLAGS) $(I2UTILLDFLAGS)
web10g_websocket_unit_tests_LDADD = $(NDTLIBS) $(I2UTILLIBS) $(I2UTILLIBDEPS) -lpthread $(ZLIB) $(JSONLIB) $(OPENSSL_LDFLAGS) $(OPENSSL_LIBS)
web10g_websocket_unit_tests_CPPFLAGS ='-DBASEDIR="$(ndtdir)"' -DFORCE_WEB10G -Wall -Wno-unused-variable  -Wno-unused-function $(OPENSSL_INCLUDES)
web10g_websocket_unit_tests_DEPENDENCIES = $(I2UTILLIBDEPS)

web10g_testoptions_unit_tests_SOURCES = testoptions_unit_tests.c testoptions.c unit_testing.c \
                                 heuristics.c jsonutils.c logging.c mrange.c ndt_odbc.c ndtptestconstants.c \
                                 network.c protocol.c runningtest.c strlutils.c test_c2s_srv.c test_meta_srv.c \
                                 test_mid_srv.c test_s2c_srv.c test_sfw_srv.c testutils.c utils.c web100-pcap.c \
                                 web100-util.c web100srv.c web10g-util.c websocket.c usage.c web100-admin.c
web10g_testoptions_unit_tests_LDFLAGS = $(NDTLDFLAGS) $(I2UTILLDFLAGS)
web10g_testoptions_unit_tests_LDADD = $(NDTLIBS) $(I2UTILLIBS) $(I2UTILLIBDEPS) -lpthread $(ZLIB) $(JSONLIB) $(OPENSSL_LDFLAGS) $(OPENSSL_LIBS)
web10g_testoptions_unit_tests_CPPFLAGS ='-DBASEDIR="$(ndtdir)"' -DFORCE_WEB10G -DUSE_WEB100SRV_ONLY_AS_LIBRARY -Wall -Wno-unused-variable -Wno-unused-function $(OPENSSL_INCLUDES)
web10g_testoptions_unit_tests_DEPENDENCIES = $(I2UTILLIBDEPS)

viewtrace_SOURCES = viewtrace.c usage.c logging.c utils.c runningtest.c ndtptestconstants.c strlutils.c
viewtrace_LDADD = $(NDTLIBS) $(I2UTILLIBDEPS) $(ZLIB)
viewtrace_CPPFLAGS ='-DBASEDIR="$(ndtdir)"' -DFORCE_WEB100

tr_mkmap_SOURCES = tr-mkmap.c tr-tree.c tr-tree6.c usage.c logging.c runningtest.c ndtptestconstants.c strlutils.c
tr_mkmap_LDADD = $(NDTLIBS) $(I2UTILLIBDEPS) $(ZLIB)
tr_mkmap_CPPFLAGS ='-DBASEDIR="$(ndtdir)"'

install-ln:
	@if test -f "$(DESTDIR)$(prefix)/sbin/web100srv" && test ! -L "$(DESTDIR)$(prefix)/sbin/ndtd"; then \
		echo '(cd $(DESTDIR)$(prefix)/sbin && $(LN_S) web100srv ndtd )' ; \
		(cd $(DESTDIR)$(prefix)/sbin && $(LN_S) web100srv ndtd ); \
	else :; fi
	@if test -f "$(DESTDIR)$(prefix)/bin/web100clt" && test ! -L "$(DESTDIR)$(prefix)/bin/ndtclt"; then \
		echo '(cd $(DESTDIR)$(prefix)/bin && $(LN_S) web100clt ndtclt )' ; \
		(cd $(DESTDIR)$(prefix)/bin && $(LN_S) web100clt ndtclt ); \
	else :; fi


install: install-am install-ln

$(I2UTILLIBDEPS):
	$(I2UTILLIBMAKE)

EXTRA_DIST = clt_tests.h logging.h mrange.h network.h protocol.h testoptions.h test_sfw.h test_meta.h \
             troute.h tr-tree.h usage.h utils.h varinfo.h web100-admin.h web100srv.h ndt_odbc.h runningtest.h ndtptestconstants.h \
             heuristics.h strlutils.h test_results_clt.h tests_srv.h testutils.h jsonutils.h unit_testing.h websocket.h third_party/safe_iop.h

