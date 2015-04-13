#########################################################################
#									#
#			   Copyright (C)  2002				#
#	     			Internet2				#
#			   All Rights Reserved				#
#									#
#########################################################################
#
#	File:		Makefile.am
#
#	Author:		Rich Carlson
#			Internet2
#
#	Date:		Fri Feb 27 13:37:24 CST 2004
#
#	Description:	toplevel build for bwctl
#

SUBDIRS = @TOP_BUILD_DIRS@ src conf doc HTML5-frontend

if INCLUDE_APPLET
SUBDIRS += Applet
endif

if INCLUDE_FLASH
SUBDIRS += flash-client
endif

if HAVE_GCJ
SUBDIRS += Admin janalyze
endif

sbin_SCRIPTS = ndt-flashpolicyd


ndtdir = $(prefix)/ndt
ndt_DATA = admin_description.html admin.html flashpolicy.xml \
	   copyright.html web100variables.html web100_variables

nobase_ndt_DATA = websocket-client/ndt_d3.html websocket-client/ndt_d3.js \
		websocket-client/ndt_d3.css websocket-client/ndt-browser-client.js

EXTRA_DIST = admin_description.html admin.html \
	tcpbw100.template tcpbw100-java.template copyright.html web100variables.html \
	web100_variables COPYRIGHT \
	DISCUSSION-LIST FILES Readme-fakewww Readme-web100srv \
	contrib/README contrib/loadmap.intro contrib/loadmap.js \
	contrib/loadmap.txt contrib/Tcpbw1001.java \
	tfw/client.py tfw/communication.py tfw/hosts.py tfw/network.py \
	tfw/Readme tfw/scenarios.py tfw/server.py tfw/traffics.py tfw/widgets.py \
        $(sbin_SCRIPTS) $(ndt_DATA) $(nobase_ndt_DATA)

sdatadir:
	test -z "$(DESTDIR)$(ndtdir)/serverdata" || $(mkdir_p) "$(DESTDIR)$(ndtdir)/serverdata"

install: sdatadir install-recursive
