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
jsonlib = lib/json-smart-1.1.1.jar
NDTGCJFLAGS = -d bin -sourcepath src -cp src/edu/internet2/ndt/$(jsonlib)
NDTJARSIGNERFLAG = -keystore
TEMPDIRS = bin dist

ndtdir = $(prefix)/ndt
jsonlibdir = $(ndtdir)/lib

ndt_DATA = dist/Tcpbw100.jar
jsonlib_DATA = dist/$(jsonlib)

Tcpbw100dir = $(ndtdir)

noinst_PROGRAMS = Tcpbw100.jar

Tcpbw100_JAVA	= src/edu/internet2/ndt/Tcpbw100.java
Tcpbw100_jar_SOURCES	= src/edu/internet2/ndt/Tcpbw100.java src/edu/internet2/ndt/StatusPanel.java \
				src/edu/internet2/ndt/Protocol.java src/edu/internet2/ndt/Message.java \
				src/edu/internet2/ndt/UserAgentTools.java src/edu/internet2/ndt/NDTConstants.java \
				src/edu/internet2/ndt/OsfwWorker.java src/edu/internet2/ndt/NewFrame.java \
				src/edu/internet2/ndt/ResultsTextPane.java src/edu/internet2/ndt/MessageType.java \
				src/edu/internet2/ndt/NDTUtils.java src/edu/internet2/ndt/JSONUtils.java
Tcpbw100$(EXEEXT): $(Tcpbw100_OBJECTS) $(Tcpbw100_DEPENDENCIES)
	@rm -f Tcpbw100$(EXEEXT)
	@if test ! -s "classTcpbw100.stamp"; then \
	    $(GCJLINK) $(Tcpbw100_LDFLAGS) $(Tcpbw100_OBJECTS) $(Tcpbw100_LDADD) $(LIBS); \
	    echo timestamp > classTcpbw100.stamp; \
	else :; fi

#classTcpbw100stamp: $(Tcpbw100_JAVA)
#	if test -n "$?"; then \
#	    echo '$(GCJ) $(NDTGCJFLAGS) $?' ; \
#	    $(GCJ) $(NDTGCJFLAGS) $?; \
#	else :; fi
#	echo timestamp > classTcpbw100.stamp

classTcpbw100stamp: $(Tcpbw100_jar_SOURCES)
	if test -n "$?"; then \
	    echo '$(GCJ) $(NDTGCJFLAGS) $?' ; \
	    $(GCJ) $(NDTGCJFLAGS) $?; \
	else :; fi
	echo timestamp > classTcpbw100.stamp

bin:
	mkdir -p bin

dist:
	mkdir -p dist

#Tcpbw100.class: $(Tcpbw100_JAVA)
#	$(GCJ) $(NDTGCJFLAGS) $(Tcpbw100_JAVA)

#Tcpbw100.jar:	Tcpbw100.class
#	$(NDTJAR) $(NDTJARFLAG) MANIFEST.MF Tcpbw100.jar Tcpbw100*.class Tcpbw100*.properties
#	echo timestamp > classTcpbw100.stamp
#All classes to be compiled into their corresponding class files
.java.class:
	$(GCJ) $(NDTGCJFLAGS) $<

#All classes to be compiled into their corresponding class files
all.class: $(Tcpbw100_jar_SOURCES:.java=.class)
#
#	$(GCJ) $(NDTGCJFLAGS) $(Tcpbw100_jar_SOURCES)

#dependency management
Tcpbw100.class: StatusPanel.class Protocol.class Message.class UserAgentTools.class NDTConstants.class OsfwWorker.class NewFrame.class ResultsTextPane.class MessageType.class NDTUtils.class
Protocol.class: Message.class
OsfwWorker.class: NDTConstants.class Message.class Protocol.class

	
#Include all classes into jar
Tcpbw100.jar:	$(TEMPDIRS) all.class
	cp -r src/edu/internet2/ndt/locale  bin/edu/internet2/ndt/
	$(NDTJAR) $(NDTJARFLAG) MANIFEST.MF dist/Tcpbw100.jar -C bin .
	cp -r src/edu/internet2/ndt/lib  dist
	if test -n "$(CERTIFICATE_FILE)"; then \
		$(NDTJARSIGNER) $(NDTJARSIGNERFLAG) $(CERTIFICATE_FILE) $(ndt_DATA) $(ALIAS); \
		$(NDTJARSIGNER) $(NDTJARSIGNERFLAG) $(CERTIFICATE_FILE) dist/$(jsonlib) $(ALIAS); \
	else :; fi
	echo timestamp > classTcpbw100.stamp


mostlyclean-generic:
	rm -rf $(TEMPDIRS) *~

EXTRA_DIST = MANIFEST.MF src/edu/internet2/ndt/locale/Tcpbw100_msgs_ca_ES.properties \
			src/edu/internet2/ndt/locale/Tcpbw100_msgs_nb_NO.properties \
			src/edu/internet2/ndt/locale/Tcpbw100_msgs_en_US.properties \
			src/edu/internet2/ndt/locale/Tcpbw100_msgs_nl_NL.properties \
			src/edu/internet2/ndt/locale/Tcpbw100_msgs_fr_FR.properties \
			src/edu/internet2/ndt/locale/Tcpbw100_msgs_ru_RU.properties \
			src/edu/internet2/ndt/locale/Tcpbw100_msgs_pt_BR.properties \
			src/edu/internet2/ndt/locale/Tcpbw100_msgs_el_GR.properties
