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

GCJLINK = $(GCJLD)
CLASSPATH_ENV =

ndtdir = $(prefix)/ndt

ndt_DATA	= Tcpbw100.jar
Tcpbw100dir = $(ndtdir)

noinst_PROGRAMS = Tcpbw100.jar

Tcpbw100_JAVA	= Tcpbw100.java
Tcpbw100_jar_SOURCES	= Tcpbw100.java

Tcpbw100$(EXEEXT): $(Tcpbw100_OBJECTS) $(Tcpbw100_DEPENDENCIES)
	@rm -f Tcpbw100$(EXEEXT)
	@if test ! -s "classTcpbw100.stamp"; then \
	    $(GCJLINK) $(Tcpbw100_LDFLAGS) $(Tcpbw100_OBJECTS) $(Tcpbw100_LDADD) $(LIBS); \
	    echo timestamp > classTcpbw100.stamp; \
	else :; fi

classTcpbw100stamp: $(Tcpbw100_JAVA)
	if test -n "$?"; then \
	    echo '$(GCJ) $(NDTGCJFLAGS) $?' ; \
	    $(GCJ) $(NDTGCJFLAGS) $?; \
	else :; fi
	echo timestamp > classTcpbw100.stamp

Tcpbw100.class: $(Tcpbw100_JAVA)
	$(GCJ) $(NDTGCJFLAGS) $(Tcpbw100_JAVA)

Tcpbw100.jar:	Tcpbw100.class
	$(NDTJAR) $(NDTJARFLAG) MANIFEST.MF Tcpbw100.jar Tcpbw100*.class Tcpbw100*.properties
	echo timestamp > classTcpbw100.stamp

EXTRA_DIST = MANIFEST.MF Tcpbw100_msgs_ca_ES.properties Tcpbw100_msgs_nb_NO.properties Tcpbw100_msgs_en_US.properties Tcpbw100_msgs_nl_NL.properties Tcpbw100_msgs_fr_FR.properties Tcpbw100_msgs_ru_RU.properties Tcpbw100_msgs_pt_BR.properties
