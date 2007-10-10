#########################################################################
#                                                                       #
#                         Copyright (C)  2007                           #
#                              Internet2                                #
#                         All Rights Reserved                           #
#                                                                       #
#########################################################################
#
#  File:    Makefile.am
#
#  Author:    Jakub S³awiñski
#
#  Date:    Wed Oct 10 23:33:27 CEST 2007
#
#  Description:

GCJLINK = $(GCJLD)
CLASSPATH_ENV =
NDTGCJFLAGS =
GCJFLAGS =

ndtdir = $(prefix)/ndt

ndt_DATA  = JAnalyze.jar
JAnalyzedir = $(ndtdir)

noinst_PROGRAMS = JAnalyze.jar

JAnalyze_JAVA  = JAnalyze.java DBConfFrame.java FilterFrame.java \
				 Helpers.java LoadingDBFrame.java LoadingFrame.java \
				 MaxPeakInfo.java MinPeakInfo.java PeakInfo.java \
				 PropertyListener.java ResultsContainer.java \
				 ResultsList.java SimpleTextPane.java SnaplogFrame.java \
				 SwingWorker.java

JAnalyze_jar_SOURCES  = JAnalyze.java DBConfFrame.java FilterFrame.java \
						Helpers.java LoadingDBFrame.java LoadingFrame.java \
						MaxPeakInfo.java MinPeakInfo.java PeakInfo.java \
						PropertyListener.java ResultsContainer.java \
						ResultsList.java SimpleTextPane.java SnaplogFrame.java \
						SwingWorker.java

JAnalyze$(EXEEXT): $(JAnalyze_OBJECTS) $(JAnalyze_DEPENDENCIES)
	@rm -f JAnalyze$(EXEEXT)
	@if test ! -s "classJAnalyze.stamp"; then \
		$(GCJLINK) $(JAnalyze_LDFLAGS) $(JAnalyze_OBJECTS) $(JAnalyze_LDADD) $(LIBS); \
		echo timestamp > classJAnalyze.stamp; \
		else :; fi

classJAnalyzestamp: $(JAnalyze_JAVA)
	if test -n "$?"; then \
		echo '$(GCJ) $(NDTGCJFLAGS) $?' ; \
		$(GCJ) $(NDTGCJFLAGS) $?; \
		else :; fi
	echo timestamp > classJAnalyze.stamp

JAnalyze.class: $(JAnalyze_JAVA)
	$(GCJ) $(NDTGCJFLAGS) $(JAnalyze_jar_SOURCES)

JAnalyze.jar:  JAnalyze.class
	find . -name '*.class' -print > classes.list
	$(NDTJAR) $(NDTJARFLAG) MANIFEST.MF JAnalyze.jar @classes.list
	echo timestamp > classJAnalyze.stamp

EXTRA_DIST = MANIFEST.MF
