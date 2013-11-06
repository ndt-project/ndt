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
ndtdir = $(prefix)/ndt
ndt_DATA = target/Tcpbw100.jar

target/Tcpbw100.jar:
	$(SMVN) $(NDTMVNFLAGS)

clean:
	$(SMVN) clean
