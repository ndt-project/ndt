#
#      $Id$
#
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
#	Author:		Rich Carlson
#			Internet2
#
#	Date:		Mon Mar 1 13:33:27 CST 2004
#
#	Description:

# JAVAROOT = /home/rcarlson/jdk1.4.2
# JAVAC = $(JAVAROOT)bin/javac
GCJLINK = $(GCJLD)

CLASSPATH_ENV = 

Admindir = $(srcdir)/Admin

noinst_PROGRAMS	= Admin

ndtdir = $(prefix)/ndt
ndt_DATA	= Admin.class

Admin_JAVA	= Admin.java
Admin_SOURCES	= Admin.java

am_Admin_OBJECTS = Admin.java
Admin_LDADD = 

classAdmin.stamp: $(Admin_JAVA)
	@if test -n "$?"; then \
	    echo '$(GCJ) $?' ; \
	    $(GCJ) $?; \
	else :; fi
	echo timestamp > classAdmin.stamp
