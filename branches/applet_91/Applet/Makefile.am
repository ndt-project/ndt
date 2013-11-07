#########################################################################
#									#
#			   Copyright (C)  2003				#
#		 			Internet2				#
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
GCJFLAGS =

ndtdir = $(prefix)/ndt
ndt_DATA = target/Tcpbw100.jar

EXTRA_DIST = src/main/java/edu/internet2/ndt/Message.java \
		src/main/java/edu/internet2/ndt/NewFrame.java \
		src/main/java/edu/internet2/ndt/StatusPanel.java \
		src/main/java/edu/internet2/ndt/MessageType.java \
		src/main/java/edu/internet2/ndt/OsfwWorker.java \
		src/main/java/edu/internet2/ndt/Tcpbw100.java \
		src/main/java/edu/internet2/ndt/NDTConstants.java \
		src/main/java/edu/internet2/ndt/Protocol.java \
		src/main/java/edu/internet2/ndt/UserAgentTools.java \
		src/main/java/edu/internet2/ndt/NDTUtils.java \
		src/main/java/edu/internet2/ndt/ResultsTextPane.java \
		src/main/resources/Tcpbw100_msgs_ca_ES.properties \
		src/main/resources/Tcpbw100_msgs_nb_NO.properties \
		src/main/resources/Tcpbw100_msgs_el_GR.properties \
		src/main/resources/Tcpbw100_msgs_nl_NL.properties \
		src/main/resources/Tcpbw100_msgs_en_US.properties \
		src/main/resources/Tcpbw100_msgs_pt_BR.properties \
		src/main/resources/Tcpbw100_msgs_fr_FR.properties \
		src/main/resources/Tcpbw100_msgs_ru_RU.properties \
		pom.xml

target/Tcpbw100.jar:
	$(SMVN) $(NDTMVNFLAGS)

clean:
	$(SMVN) clean
