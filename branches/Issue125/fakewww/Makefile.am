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
#  Author:    Sebastian Malecki
#
#  Date:    Tue Mar 6 11:04:11 CET 2014
#
#  Description:

ndtdir = $(prefix)/ndt

sbin_PROGRAMS = fakewww

fakewww_SOURCES = fakewww.c troute.c troute6.c ../src/tr-tree.c ../src/tr-tree6.c ../src/network.c ../src/usage.c ../src/logging.c \
                  ../src/runningtest.c ../src/ndtptestconstants.c ../src/strlutils.c
fakewww_LDADD = $(I2UTILLIBDEPS) $(ZLIB)
fakewww_CPPFLAGS ='-DBASEDIR="$(ndtdir)"'
