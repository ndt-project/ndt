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

noinst_PROGRAMS	= Admin.class

ndtdir = $(prefix)/ndt
ndt_DATA	= Admin.class
Admindir = $(ndtdir)

Admin_JAVA	= Admin.java
Admin_class_SOURCES	= Admin.java

Admin$(EXEEXT): $(Admin_OBJECTS) $(Admin_DEPENDENCIES)
	@rm -f Admin$(EXEEXT)
	@if test ! -s "classAdmin.stamp"; then \
	    $(GCJLINK) $(Admin_LDFLAGS) $(Admin_OBJECTS) $(Admin_LDADD) $(LIBS); \
	    echo timestamp > classAdmin.stamp; \
	else :; fi

classAdmin.stamp: $(Admin_JAVA)
	if test -n "$?"; then \
	    echo '$(GCJ) $?' ; \
	    $(GCJ) $?; \
	else :; fi
	echo timestamp > classAdmin.stamp

Admin.class: $(Admin_JAVA)
	$(GCJ) $(Admin_JAVA)
