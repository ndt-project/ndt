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
NDTGCJFLAGS = -cp external:src -d bin
GCJFLAGS =
TEMPDIRS = bin dist

ndtdir = $(prefix)/ndt

ndt_DATA  = JAnalyze.jar
JAnalyzedir = $(ndtdir)/janalyze

noinst_PROGRAMS = JAnalyze.jar

JAnalyze_JAVA  = src/JAnalyze.java src/DBConfFrame.java src/FilterFrame.java \
				 src/Helpers.java src/LoadingDBFrame.java src/LoadingFrame.java \
				 src/MaxPeakInfo.java src/MinPeakInfo.java src/PeakInfo.java \
				 src/PropertyListener.java src/ResultsContainer.java \
				 src/ResultsList.java src/SimpleTextPane.java src/SnaplogFrame.java \
				 src/SwingWorker.java

JAnalyze_jar_SOURCES  = $(JAnalyze_JAVA)

JAnalyze$(EXEEXT): $(JAnalyze_OBJECTS) $(JAnalyze_DEPENDENCIES)
	@rm -f JAnalyze$(EXEEXT)
	@if test ! -s "classJAnalyze.stamp"; then \
		$(GCJLINK) $(JAnalyze_LDFLAGS) $(JAnalyze_OBJECTS) $(JAnalyze_LDADD) $(LIBS); \
		echo timestamp > classJAnalyze.stamp; \
		else :; fi

install-JAnalyzeJAVA: classJAnalyze.stamp
	@$(NORMAL_INSTALL)
	test -z "$(JAnalyzedir)" || $(mkdir_p) "$(DESTDIR)$(JAnalyzedir)"
	@test -z "$(JAnalyze_JAVA/bin)" || for p in *.class; do \
		echo " $(INSTALL_DATA) '$$p' '$(DESTDIR)$(JAnalyzedir)/$$p'"; \
		$(INSTALL_DATA) "$$p" "$(DESTDIR)$(JAnalyzedir)/$$p"; \
	done

install-ndtDATA: $(ndt_DATA)
	@$(NORMAL_INSTALL)
	test -z "$(ndtdir)" || $(mkdir_p) "$(DESTDIR)$(ndtdir)"
	@list='dist/$(ndt_DATA)'; for p in $$list; do \
	  if test -f "$$p"; then d=; else d="$(srcdir)/"; fi; \
	  $(am__strip_dir) \
	  echo " $(INSTALL_DATA) '$$d$$p' '$(DESTDIR)$(JAnalyzedir)/$$f'"; \
	   $(INSTALL_DATA) "$$d$$p" "$(DESTDIR)$(JAnalyzedir)/$$f"; \
	done
	@list='bin/*.class'; for p in $$list; do \
	  if test -f "$$p"; then d=; else d="$(srcdir)/"; fi; \
	  $(am__strip_dir) \
	  echo " $(INSTALL_DATA) '$$d$$p' '$(DESTDIR)$(JAnalyzedir)/$$f'"; \
	   $(INSTALL_DATA) "$$d$$p" "$(DESTDIR)$(JAnalyzedir)/$$f"; \
	done
	/bin/cp -Rp external/de $(DESTDIR)$(JAnalyzedir)/

classJAnalyzestamp: $(JAnalyze_JAVA)
	if test -n "$?"; then \
		echo '$(GCJ) $(NDTGCJFLAGS) $?' ; \
		$(GCJ) $(NDTGCJFLAGS) $?; \
		else :; fi
	echo timestamp > classJAnalyze.stamp

bin:
	mkdir -p bin

dist:
	mkdir -p dist

JAnalyze.class: $(JAnalyze_JAVA) $(TEMPDIRS)
	@if test ! -f "classJAnalyze.stamp"; then \
		$(GCJ) $(NDTGCJFLAGS) $(JAnalyze_jar_SOURCES); \
		else :; fi

JAnalyze.jar:  JAnalyze.class
	@if test ! -f "classJAnalyze.stamp"; then \
		find ./bin -name '*.class' -print | awk '{sub("./bin/", "-C bin "); print}' > classes.list; \
		find ./external -name '*.class' -print | awk '{sub("./external/", "-C external "); print}' >> classes.list; \
		$(NDTJAR) $(NDTJARFLAG) MANIFEST.MF dist/JAnalyze.jar @classes.list; \
		echo timestamp > classJAnalyze.stamp; \
		rm classes.list; \
		else :; fi

mostlyclean-generic:
	rm -rf META-INF $(TEMPDIRS) *~

EXTRA_DIST = Readme MANIFEST.MF ./external/de/progra/charting/test/TestChartDataModel.class \
			 ./external/de/progra/charting/test/FunctionPlotApplet.class \
			 ./external/de/progra/charting/test/ImageFilter.class \
			 ./external/de/progra/charting/test/GraphFrame$1.class \
			 ./external/de/progra/charting/test/TestChart.class \
			 ./external/de/progra/charting/test/GraphFrame$2.class \
			 ./external/de/progra/charting/test/FunctionPlotApplet$1.class \
			 ./external/de/progra/charting/test/GraphFrame$3.class \
			 ./external/de/progra/charting/test/GraphFrame.class \
			 ./external/de/progra/charting/PointToPixelTranslator.class \
			 ./external/de/progra/charting/event/ChartDataModelListener.class \
			 ./external/de/progra/charting/event/ChartDataModelEvent.class \
			 ./external/de/progra/charting/Axis.class \
			 ./external/de/progra/charting/model/ObjectChartDataModel.class \
			 ./external/de/progra/charting/model/AbstractChartDataModel.class \
			 ./external/de/progra/charting/model/DefaultChartDataModelConstraints.class \
			 ./external/de/progra/charting/model/EditableDataSet.class \
			 ./external/de/progra/charting/model/JDBCPlotterException.class \
			 ./external/de/progra/charting/model/DefaultDataSet.class \
			 ./external/de/progra/charting/model/StackedChartDataModelConstraints.class \
			 ./external/de/progra/charting/model/DataSet.class \
			 ./external/de/progra/charting/model/JDBCPlotter.class \
			 ./external/de/progra/charting/model/ChartDataModel.class \
			 ./external/de/progra/charting/model/DefaultChartDataModel.class \
			 ./external/de/progra/charting/model/ChartDataModelConstraints.class \
			 ./external/de/progra/charting/model/EditableChartDataModel.class \
			 ./external/de/progra/charting/model/FunctionPlotter.class \
			 ./external/de/progra/charting/swing/ChartPanel.class \
			 ./external/de/progra/charting/DefaultChart.class \
			 ./external/de/progra/charting/servlet/ChartServlet.class \
			 ./external/de/progra/charting/CoordSystem$1.class \
			 ./external/de/progra/charting/Chart.class \
			 ./external/de/progra/charting/ChartUtilities.class \
			 ./external/de/progra/charting/CoordSystem.class \
			 ./external/de/progra/charting/render/BarChartRenderer.class \
			 ./external/de/progra/charting/render/ChartRenderingHints.class \
			 ./external/de/progra/charting/render/shape/Diamond2D.class \
			 ./external/de/progra/charting/render/shape/Triangle2D.class \
			 ./external/de/progra/charting/render/shape/Diamond2D$1.class \
			 ./external/de/progra/charting/render/shape/Triangle2D$1.class \
			 ./external/de/progra/charting/render/AbstractChartRenderer.class \
			 ./external/de/progra/charting/render/StackedBarChartRenderer.class \
			 ./external/de/progra/charting/render/PlotChartRenderer.class \
			 ./external/de/progra/charting/render/RowColorModel.class \
			 ./external/de/progra/charting/render/AbstractRenderer.class \
			 ./external/de/progra/charting/render/InterpolationChartRenderer.class \
			 ./external/de/progra/charting/render/Renderer.class \
			 ./external/de/progra/charting/render/LineChartRenderer.class \
			 ./external/de/progra/charting/render/PieChartRenderer.class \
			 ./external/de/progra/charting/render/RadarChartRenderer.class \
			 ./external/de/progra/charting/ChartEncoder.class \
			 ./external/de/progra/charting/Legend.class \
			 ./external/de/progra/charting/AbstractChart.class \
			 ./external/de/progra/charting/CoordSystemUtilities.class \
			 ./external/de/progra/charting/Title.class \
			 ./external/de/progra/charting/EncodingException.class
