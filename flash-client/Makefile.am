#########################################################################
#                                                                       #
#                         Copyright (C)  2014                           #
#                              Internet2                                #
#                         All Rights Reserved                           #
#                                                                       #
#########################################################################
#
#  File:    Makefile.am
#
#  Author:    Sebastian Kostuch
#
#  Date:    Tue Feb 4 13:43:12 CET 2014
#
#  Description:

NDTMXMLCFLAGS = -o dist/FlashClt.swf -load-config+=ndt-flash-config.xml
TEMPDIRS = dist

ndtdir = $(prefix)/ndt

ndt_DATA  = dist/FlashClt.swf flashpolicy.xml
FlashCltdir = $(ndtdir)

noinst_PROGRAMS = FlashClt.swf

sbin_SCRIPTS = ndt-flashpolicyd

FlashClt_swf_SOURCES = src/GUI.as src/Handshake.as src/Main.as src/Message.as \
			src/MessageType.as src/NDTConstants.as src/NDTPController.as \
			src/ndt_test_results.as src/NDTUtils.as src/TestC2S.as \
			src/TestMETA.as src/TestResults.as src/TestResultsUtils.as \
			src/TestS2C.as src/TestType.as src/UserAgentTools.as 							

dist:
	mkdir -p dist

FlashClt.swf: $(TEMPDIRS)
if BUILD_FLASH
		$(NDTMXMLC) src/Main.as $(NDTMXMLCFLAGS);
else
		cp precompiled/FlashClt.swf dist
endif

mostlyclean-generic:
	rm -rf $(TEMPDIRS) *~

EXTRA_DIST = README AUTHORS COPYING ndt-flash-config.xml assets/hover.png assets/mlab-logo.png \
		src/locale/ca_ES/DisplayMessages.properties src/locale/el_GR/DisplayMessages.properties \
		src/locale/en_US/DisplayMessages.properties src/locale/fr_FR/DisplayMessages.properties \
		src/locale/nb_NO/DisplayMessages.properties src/locale/nl_NL/DisplayMessages.properties \
		src/locale/pt_BR/DisplayMessages.properties src/locale/ru_RU/DisplayMessages.properties \
		precompiled/FlashClt.swf
		
