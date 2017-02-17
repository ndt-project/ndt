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
NDTGCJFLAGS = -d bin -sourcepath src
CLASSPATH_ENV = 
TEMPDIRS = bin
AUTOMAKE_OPTIONS = no-exeext

noinst_PROGRAMS	= Admin.class

ndtdir = $(prefix)/ndt
ndt_DATA	= bin/edu/internet2/ndt/Admin.class
Admindir = $(ndtdir)/Admin

Admin_JAVA	= src/edu/internet2/ndt/Admin.java
Admin_class_SOURCES	= src/edu/internet2/ndt/Admin.java

Admin$(EXEEXT): $(Admin_OBJECTS) $(Admin_DEPENDENCIES)
	@rm -f Admin$(EXEEXT)
	@if test ! -s "classAdmin.stamp"; then \
	    $(GCJLINK) $(Admin_LDFLAGS) $(Admin_OBJECTS) $(Admin_LDADD) $(LIBS); \
	    echo timestamp > classAdmin.stamp; \
	else :; fi

install-ndtDATA: $(ndt_DATA)
	@$(NORMAL_INSTALL)
	test -z "$(ndtdir)" || $(mkdir_p) "$(DESTDIR)$(ndtdir)"
	test -z "$(Admindir)/edu" || $(mkdir_p) "$(DESTDIR)$(Admindir)/edu"
	test -z "$(Admindir)/edu/internet2" || $(mkdir_p) "$(DESTDIR)$(Admindir)/edu/internet2"
	test -z "$(Admindir)/edu/internet2/ndt" || $(mkdir_p) "$(DESTDIR)$(Admindir)/edu/internet2/ndt"
	echo " $(INSTALL_DATA) "$(ndt_DATA)" '$(DESTDIR)$(Admindir)/edu/internet2/ndt/'"; \
	$(INSTALL_DATA) "$(ndt_DATA)" "$(DESTDIR)$(Admindir)/edu/internet2/ndt/"; 

bin:
	mkdir -p bin

classAdmin.stamp: $(Admin_JAVA)
	if test -n "$?"; then \
	    echo '$(GCJ) $(NDTGCJFLAGS) $?' ; \
	    $(GCJ) $(NDTGCJFLAGS) $?; \
	else :; fi
	echo timestamp > classAdmin.stamp

mostlyclean-generic:
	rm -rf $(TEMPDIRS) *~

Admin.class: $(Admin_JAVA) $(TEMPDIRS)
	$(GCJ) $(NDTGCJFLAGS) $(Admin_JAVA)
