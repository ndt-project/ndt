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

#JAVAROOT = /home/rcarlson/jdk1.4.2
# JAVAC = $(JAVAROOT)bin/javac
GCJLINK = $(GCJLD)

# .SUFFIXES = .java .jar

CLASSPATH_ENV =

# Tcpbw100dir = $(srcdir)/Applet

ndtdir = $(prefix)/ndt
# ndt_DATA	= Tcpbw100 Tcpbw100.jar
ndt_DATA	= Tcpbw100.jar
Tcpbw100dir = $(ndtdir)

# noinst_PROGRAMS	= Tcpbw100 Tcpbw100.jar
noinst_PROGRAMS = Tcpbw100.jar

Tcpbw100_JAVA	= Tcpbw100.java
#Tcpbw100_SOURCES	= Tcpbw100.java

#Tcpbw100_jar_SOURCES = Tcpbw100.jar

#am_Tcpbw100_OBJECTS = Tcpbw100.java
#Tcpbw100_LDADD = 

Tcpbw100$(EXEEXT): $(Tcpbw100_OBJECTS) $(Tcpbw100_DEPENDENCIES)
	@rm -f Tcpbw100$(EXEEXT)
	@if test ! -s "classTcpbw100.stamp"; then \
	    $(GCJLINK) $(Tcpbw100_LDFLAGS) $(Tcpbw100_OBJECTS) $(Tcpbw100_LDADD) $(LIBS); \
	    echo timestamp > classTcpbw100.stamp; \
	else :; fi

classTcpbw100stamp: $(Tcpbw100_JAVA)
	if test -n "$?"; then \
	    echo '$(GCJ) $?' ; \
	    $(GCJ) $?; \
	else :; fi
	echo timestamp > classTcpbw100.stamp

Tcpbw100.class: $(Tcpbw100_JAVA)
	$(GCJ) $(Tcpbw100_JAVA)

Tcpbw100.jar:	Tcpbw100.class
	$(NDTJAR) $(NDTJARFLAG) Tcpbw100.jar Tcpbw100*.class
	echo timestamp > classTcpbw100.stamp
