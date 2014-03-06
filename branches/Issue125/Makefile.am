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

SUBDIRS = @TOP_BUILD_DIRS@ src conf doc
if HAVE_GCJ
SUBDIRS += Admin janalyze
if BUILD_APPLET
SUBDIRS += Applet
endif
if HAVE_MXMLC
if BUILD_FLASH_CLT
SUBDIRS += flash-client
endif
endif
if BUILD_FAKEWWW
SUBDIRS += fakewww
endif
endif

EXTRA_DIST = admin_description.html admin.html \
	tcpbw100.template tcpbw100-java.template copyright.html web100variables.html \
	web100_variables COPYRIGHT \
	DISCUSSION-LIST FILES Readme-fakewww Readme-web100srv \
	contrib/README contrib/loadmap.intro contrib/loadmap.js \
	contrib/loadmap.txt contrib/Tcpbw1001.java \
	tfw/client.py tfw/communication.py tfw/hosts.py tfw/network.py \
	tfw/Readme tfw/scenarios.py tfw/server.py tfw/traffics.py tfw/widgets.py

ndtdir = $(prefix)/ndt
ndt_DATA = admin_description.html admin.html \
	   copyright.html web100variables.html web100_variables

sdatadir:
	test -z "$(ndtdir)/serverdata" || $(mkdir_p) "$(ndtdir)/serverdata"

install: sdatadir install-recursive

# uncomment this when we stop using broken automake 1.5
# (then remove from DIST_EXTRA in subdir)
#nobase_include_HEADERS = bwlib/bwlib.h

#EXTRA_DIST	=		bootstrap
#MAINTAINERCLEANFILES	=	aclocal.m4 Makefile.in configure config.log \
#				config.status
