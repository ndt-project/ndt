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

if HAVE_GCJ
SUBDIRS = @TOP_BUILD_DIRS@ src Admin Applet conf doc
else
SUBDIRS = @TOP_BUILD_DIRS@ src conf doc
endif

EXTRA_DIST = admin_description.html admin.html \
	tcpbw100.template copyright.html web100variables.html \
	web100_variables COPYRIGHT \
	DISCUSSION-LIST FILES Readme-fakewww Readme-web100srv

ndtdir = $(prefix)/ndt
ndt_DATA = admin_description.html admin.html \
	   copyright.html web100variables.html web100_variables

# uncomment this when we stop using broken automake 1.5
# (then remove from DIST_EXTRA in subdir)
#nobase_include_HEADERS = bwlib/bwlib.h

#EXTRA_DIST	=		bootstrap
#MAINTAINERCLEANFILES	=	aclocal.m4 Makefile.in configure config.log \
#				config.status
