#!/bin/bash

# This script changes the widget.html page to use flash or java NDT client
# as background for JS UI. 

flash="flash"
java="java"
widget="widget.html"
embed_tag="<embed id=\"NDT\" name=\"NDT\" type=\"application\/x-shockwave-flash\" src=\"FlashClt.swf\" width=\"600\" height=\"400\" \/>"
applet_tag="<applet id=\"NDT\" name=\"NDT\" code=\"edu.internet2.ndt.Tcpbw100.class\" codebase=\"<?php print \$applet_url ?>\" archive=\"Tcpbw100.jar\" width=\"400\" height=\"400\"><\/applet>"

echo "This script allows you to change NDT client being used in $widget page."
if test ! -f "$widget"; then
        echo "$widget not found. Aborting..."
        exit
fi

echo "Choose which client you want to use ($java/$flash): "
read client
while test "$client" != "$java" && test "$client" != "$flash"; do
        echo "Choose which client you want to use ($java/$flash): "
        read client
done

if test "$client" = "$java"; then
        /bin/sed -i "s/$embed_tag/$applet_tag/" $widget
        /bin/sed -i "s/<!--[ \t\n]*$applet_tag[ \t\n]*-->/<!--$embed_tag-->/" $widget
fi

if test "$client" = "$flash"; then
        /bin/sed -i "s/$applet_tag/$embed_tag/" $widget
        /bin/sed -i "s/<!--[ \t\n]*$embed_tag[ \t\n]*-->/<!--$applet_tag-->/" $widget
fi

echo "$widget has been successfully changed."

