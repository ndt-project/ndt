#########################################################################
#									#
#			   Copyright (C)  2004				#
#	     			Internet2				#
#			   All Rights Reserved				#
#									#
#########################################################################
#
#	File:		Makefile.am
#
#	Author:		Jeff Boote
#			Internet2
#
#	Date:		Tue Feb 17 11:08:20 MST 2004
#
#	Description:

dist_man1_MANS = web100clt.man analyze.man tr-mkmap.man viewtrace.man genplot.man
dist_man5_MANS = ndt.conf.man
dist_man8_MANS = web100srv.man fakewww.man

# EXTRA_DIST = $(man1_MANS) $(man5_MANS) $(man8_MANS)
EXTRA_DIST = index.html

# TODO
# Build html versions of man pages and include them in dist?
# SUFFIXES = .man .man.html
# .man.man.html:
#	man2html -blah_args 
# dist-hook:
SUFFIXES = .man .man.html

.man.man.html:
	cp $< $@
