#!/bin/csh -f
#
# This script will install the ndt startup script into
# the /etc/init.d directory and cause chkconfig to add
# this to the list of processes to autostart at boot time

echo "This script will configure your system to automatically"
echo "start the NDT processes at boot time."
echo ""

# First check to see if the startup script is available
if (-e ndt) then
    # Yes, the autostart.sh script is being run from the conf subdir
    set FILE = ndt
else
    # no, check to see if the conf subdir exists
    if (-d conf) then
	# yes, we can continue
	if (-e conf/ndt) set FILE = conf/ndt
    else
	set FILE = ""
    endif
endif

# Now check to see what state we are in.
if ($FILE == "") then
    echo "Unable to continue.  'ndt' startup script not found."
    exit
else
# First copy the startup script over to the system directory
    /bin/cp  $FILE /etc/rc.d/init.d/ndt
    /bin/chmod 755 /etc/rc.d/init.d/ndt
# Then add the NDT package to the list of managed packages
    /sbin/chkconfig --add ndt
# Finally show the user that it has been done
    /sbin/chkconfig --list ndt
endif
	  
