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

EXTRA_DIST = MANIFEST.MF ./de/progra/charting/test/TestChartDataModel.class \
			 ./de/progra/charting/test/FunctionPlotApplet.class \
			 ./de/progra/charting/test/ImageFilter.class \
			 ./de/progra/charting/test/GraphFrame$1.class \
			 ./de/progra/charting/test/TestChart.class \
			 ./de/progra/charting/test/GraphFrame$2.class \
			 ./de/progra/charting/test/FunctionPlotApplet$1.class \
			 ./de/progra/charting/test/GraphFrame$3.class \
			 ./de/progra/charting/test/GraphFrame.class \
			 ./de/progra/charting/PointToPixelTranslator.class \
			 ./de/progra/charting/event/ChartDataModelListener.class \
			 ./de/progra/charting/event/ChartDataModelEvent.class \
			 ./de/progra/charting/Axis.class \
			 ./de/progra/charting/model/ObjectChartDataModel.class \
			 ./de/progra/charting/model/AbstractChartDataModel.class \
			 ./de/progra/charting/model/DefaultChartDataModelConstraints.class \
			 ./de/progra/charting/model/EditableDataSet.class \
			 ./de/progra/charting/model/JDBCPlotterException.class \
			 ./de/progra/charting/model/DefaultDataSet.class \
			 ./de/progra/charting/model/StackedChartDataModelConstraints.class \
			 ./de/progra/charting/model/DataSet.class \
			 ./de/progra/charting/model/JDBCPlotter.class \
			 ./de/progra/charting/model/ChartDataModel.class \
			 ./de/progra/charting/model/DefaultChartDataModel.class \
			 ./de/progra/charting/model/ChartDataModelConstraints.class \
			 ./de/progra/charting/model/EditableChartDataModel.class \
			 ./de/progra/charting/model/FunctionPlotter.class \
			 ./de/progra/charting/swing/ChartPanel.class \
			 ./de/progra/charting/DefaultChart.class \
			 ./de/progra/charting/servlet/ChartServlet.class \
			 ./de/progra/charting/CoordSystem$1.class \
			 ./de/progra/charting/Chart.class \
			 ./de/progra/charting/ChartUtilities.class \
			 ./de/progra/charting/CoordSystem.class \
			 ./de/progra/charting/render/BarChartRenderer.class \
			 ./de/progra/charting/render/ChartRenderingHints.class \
			 ./de/progra/charting/render/shape/Diamond2D.class \
			 ./de/progra/charting/render/shape/Triangle2D.class \
			 ./de/progra/charting/render/shape/Diamond2D$1.class \
			 ./de/progra/charting/render/shape/Triangle2D$1.class \
			 ./de/progra/charting/render/AbstractChartRenderer.class \
			 ./de/progra/charting/render/StackedBarChartRenderer.class \
			 ./de/progra/charting/render/PlotChartRenderer.class \
			 ./de/progra/charting/render/RowColorModel.class \
			 ./de/progra/charting/render/AbstractRenderer.class \
			 ./de/progra/charting/render/InterpolationChartRenderer.class \
			 ./de/progra/charting/render/Renderer.class \
			 ./de/progra/charting/render/LineChartRenderer.class \
			 ./de/progra/charting/render/PieChartRenderer.class \
			 ./de/progra/charting/render/RadarChartRenderer.class \
			 ./de/progra/charting/ChartEncoder.class \
			 ./de/progra/charting/Legend.class \
			 ./de/progra/charting/AbstractChart.class \
			 ./de/progra/charting/CoordSystemUtilities.class \
			 ./de/progra/charting/Title.class \
			 ./de/progra/charting/EncodingException.class
