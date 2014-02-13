#!/bin/bash

# This script creates the basic tcpbw100.html and tcpbw100-java.html pages needed by
# the NDT server.
#
# First prompt the user for some local data variables, site name
# and location, email address and subject line for reports, and
# the local link connection info.
#
# Then create the html files using the tcpbw100.template (for page with flash client) 
# and tcpbw100-java.template (for page with java applet) files modified by sed.
#
# Richard Carlson
# rcarlson@internet2.edu
# April 1, 2004
#

echo "Welcome to the NDT server configuration program.  This"
echo "program will create a custom tcpbw100.html and tcpbw100-java.html files for your site."
echo ""

echo -n "Enter your site name [Internet2]  : "
read yoursite
if test "$yoursite" = ""; then
	yoursite="Internet2"
fi

echo -n "Enter your site's location [Ann Arbor - MI]  : "
read yourloc
if test "$yourloc" = ""; then
	yourloc="Ann Arbor - MI"
fi

echo -n "Server connection info, enter 1 for 100 Mbps, 2 for 1 Gbps [2]  : "
read yourspd
if test "$yourspd" = "" -o "$yourspd" == "2"; then
	yourspeed="1000 Mbps (Gigabit Ethernet)"
else
	yourspeed="100 Mbps (Fast Ethernet)"
fi

echo ""
echo "Information for email trouble reporting"
echo -n "Enter email userid [rcarlson]  : "
read youremail
if test "$youremail" = ""; then
	youremail="rcarlson"
fi

echo -n "Enter email domain name [internet2.edu]  : "
read yourdomain
if test "$yourdomain" = ""; then
	yourdomain="internet2.edu"
fi

hostnm=`uname -n`
echo -n "Enter default subject line [Trouble report from $hostnm]  : "
read yoursubject
if test "$yoursubject" = ""; then
	yoursubject="Trouble report from $hostnm"
fi

# All the questions have been asked, now it's time to put them
# to use.  Run sed a number of times and replace the variable names
# with the info the user supplied.

echo "s/YOURSUBJECT/$yoursubject/" > /tmp/$$
echo "s/YOURDOMAIN/$yourdomain/" >> /tmp/$$
echo "s/YOUREMAIL/$youremail/" >> /tmp/$$
echo "s/YOURSITE/$yoursite/" >> /tmp/$$
echo "s/YOURLOCATION/$yourloc/" >> /tmp/$$
echo "s/YOURSPEED/$yourspeed/"   >> /tmp/$$


if test -f tcpbw100.template ; then
	TEMPLATE_FLASH=../tcpbw100.template
elif test -f ../tcpbw100.template ; then
	TEMPLATE_FLASH=../tcpbw100.template
fi

if test -f tcpbw100-java.template ; then
	TEMPLATE_JAVA=tcpbw100-java.template
elif test -f ../tcpbw100.template ; then
	TEMPLATE_JAVA=../tcpbw100-java.template
fi

# /bin/sed -f /tmp/$$ tcpbw100.template > tcpbw100.html
/bin/sed -f /tmp/$$ $TEMPLATE_FLASH > tcpbw100.html
/bin/sed -f /tmp/$$ $TEMPLATE_JAVA > tcpbw100-java.html
/bin/rm -f /tmp/$$

echo "The base web pages 'tcpbw100.html' (with java applet) and 'tcpbw100-java.html (with flash client) has now been created.  You"
echo "must move these files into the ndt_DATA directory [/usr/local/ndt]"
echo "created during the 'make' process."

echo -n "Do you want to install these files now? [yes]  : "
read answer
if test "$answer" = "n" -o "$answer" = "no"; then
	exit
fi

echo -n "Enter location [/usr/local/ndt]  : "
read answer
if test "$answer" = ""; then
	answer="/usr/local/ndt"
fi

if test -w $answer; then
	/bin/cp -p tcpbw100.html $answer
	/bin/cp -p tcpbw100-java.html $answer
	# create a default directory to store snaplog and tcpdump files in
	if test ! -d $answer/serverdata; then
		/bin/mkdir $answer/serverdata
	fi
else
	echo "Unable to write into directory $answer, check permissions"
fi
