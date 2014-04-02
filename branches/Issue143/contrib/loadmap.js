/********************************************************************
 * loadmap.js
 * Creates the Google Map on broadbandMap.php and adds marker points
 * Author:  Seth Peery,
 *          Virginia Polytechnic Institute and State University
 * Last modified:	2006-05-31
 *******************************************************************/

	// Create a base icon for all of our markers that specifies the
	// shadow, icon dimensions, etc.	
	var baseIcon = new GIcon();
	baseIcon.shadow = "icons/shadow50.png";
	baseIcon.iconSize = new GSize(20, 34);
	baseIcon.shadowSize = new GSize(37, 34);
	baseIcon.iconAnchor = new GPoint(9, 34);
	baseIcon.infoWindowAnchor = new GPoint(9, 2);
	baseIcon.infoShadowAnchor = new GPoint(18, 25); 
	
    var map = null;
    var geocoder = null;

	// return the appropriate symbology for a map marker
	function determineIcon(speed, tech) {
		var result = "marker";
		
		// Determine the marker letter based on connection type
		if (tech == "Dial-up") {
			result += "P";
		} else if (tech == "ISDN") {
			result += "I";
		} else if (tech == "Cable Modem") {
			result += "C";
		} else if (tech == "DSL") {
			result += "D";
		} else if (tech == "Wireless (802.11/Wi-Fi)") {
			result += "W";
		} else if (tech == "Other Wireless") {
			result += "W";
		} else if (tech == "Satellite") {
			result += "S";
		} else if (tech == "Ethernet") {
			result += "E";
		} else if (tech == "Leased Line (T1 or lower)") {
			result += "L";			
		} else if (tech == "Leased Line (DS3 or higher)") {
			result += "L";			
		} else if (tech == "Other Wired") {
			result += "O";
		} else {
			result += "U";
		}

		// Determine the marker color based on downstream speed
		var speedFloat = parseFloat(speed);
		if (tech == "Dial-up") {
			result += "1";
		} else { 
			if (speedFloat <=56.0) {
				result += "1";
			} else if (speedFloat > 56.0 && speedFloat <= 256.0) {
				result += "2";
			} else if (speedFloat > 256.0 && speedFloat <= 1000) {
				result += "3";
			} else if (speedFloat > 1000 && speedFloat <= 5000) {
				result += "4";
			} else {
				result += "5";
			} // end inner if
		} // end outer if		
		return result;
	} // end of function determineIcon
	
	
	// create a marker with the appropriate icon, build info window, and bind to click listener
	function createMarker(point, zip, locname, provider, downstream_bw, upstream_bw, cost, accesstech, conntype, connrate) {
	  var iconFile = determineIcon(downstream_bw, accesstech);
	  var icon = new GIcon(baseIcon);
	  icon.image = "http://www.ecorridors.vt.edu/maps/icons/" + iconFile + ".png";
	  var marker = new GMarker(point, icon);
		//	  var marker = new GMarker(point);
      GEvent.addListener(marker, "click", function() {

		// handle the display of null values
		var infoWindowCost = "Unknown";
		var formFieldCost = "";
	  	if (!(cost=="-1.00")) {
			infoWindowCost=cost;
			formFieldCost=cost;
		}	

		var infoWindowProvider = "Unknown";		
		var formFieldProvider = "";
		if (!(provider=="Unknown")) {
			infoWindowProvider=provider;
			formFieldProvider=provider;
		}
		
		var infoWindowZip = "Unknown";
		var formFieldZip = "";
		if (!(zip=="Unknown")) {
			infoWindowZip=zip;
			formFieldZip=zip;
		}	
		
		// Build the info window
        marker.openInfoWindowHtml("<table><tr><td><font color=\"#000000\">Location name</font></td><td><font color=\"#000000\"><strong>"+locname+"</strong></font></td></tr><tr><td><font color=\"#000000\">ZIP code</font></td><td><font color=\"#000000\"><strong>"+infoWindowZip+"</strong></font></td></tr><tr><td><font color=\"#000000\">Downstream speed</font></td><td><font color=\"#000000\"><strong>"+downstream_bw+"</strong></font></td></tr><tr><td><font color=\"#000000\">Upstream speed</font></td><td><font color=\"#000000\"><strong>"+upstream_bw+"</strong></font></td></tr><tr><td><font color=\"#000000\">Provider</font></td><td><font color=\"#000000\"><strong>"+infoWindowProvider+"</strong></font></td></tr><tr><td><font color=\"#000000\">Cost</font></td><td><font color=\"#000000\"><strong>"+infoWindowCost+"</strong></font></td></tr><tr><td><font color=\"#000000\">Access technology</font></td><td><font color=\"#000000\"><strong>"+accesstech+"</strong></font></td></tr><tr><td><font color=\"#000000\">Service type</font></td><td><font color=\"#000000\"><strong>"+conntype+"</strong></font></td></tr><tr><td><font color=\"#000000\">Reported Performance</font></td><td><font color=\"#000000\"><strong>"+connrate+"</strong></font></td></tr></table>");
//<br><a href=\"javascript:copyAttributes(point, zip, locname, provider, downstream_bw, upstream_bw, cost, accesstech, conntype, connrate)\">Use as my current location</a>
      });
      return marker;
    }

	function copyAttributes(point, zip, locname, provider, downstream_bw, upstream_bw, cost, accesstech, conntype, connrate) {
			// Populate the form fields
	  	document.getElementById("addyourself").latitude.value = point.lat().toString();
		document.getElementById("addyourself").longitude.value = point.lng().toString();
		document.getElementById("addyourself").locname.value = locname;
		document.getElementById("addyourself").zip.value = formFieldZip;
		document.getElementById("addyourself").provider.value = formFieldProvider;
		document.getElementById("addyourself").cost.value = formFieldCost;
		document.getElementById("addyourself").accesstech.selectedIndex = getIndexOf("accesstech",accesstech);
		document.getElementById("addyourself").conntype.selectedIndex = getIndexOf("conntype",conntype);
		document.getElementById("addyourself").connrate.selectedIndex = getIndexOf("connrate",connrate);
	}
	
	// load all the markers in the current viewing extent
	function loadMarkers(map){
		var bounds = map.getBounds();
		var sw = bounds.getSouthWest();
		var ne = bounds.getNorthEast();
		var envelopeString="?right="+ne.lng()+"&left="+sw.lng()+"&top="+ne.lat()+"&bottom="+sw.lat();
		var request = GXmlHttp.create();
		request.open("POST", "dynamicxml.php", true);
		request.setRequestHeader("Content-Type","application/x-www-form-urlencoded; charset=UTF-8");
		request.send(envelopeString);
				request.onreadystatechange = function() {
			if (request.readyState == 4) {
				var xmlDoc = request.responseXML;
				var markers = xmlDoc.documentElement.getElementsByTagName("marker");
					  for (var i = 0; i < markers.length; i++) {
						var point = new GLatLng(
						  parseFloat(markers[i].getAttribute("lat")),
						  parseFloat(markers[i].getAttribute("lng")));
						  map.addOverlay(createMarker(
							point, 
							markers[i].getAttribute("zip"), 
							markers[i].getAttribute("locname"), 
							markers[i].getAttribute("provider"), 
							markers[i].getAttribute("downstream_bw"), 
							markers[i].getAttribute("upstream_bw"), 
							markers[i].getAttribute("cost"),
							markers[i].getAttribute("accesstech"),
							markers[i].getAttribute("conntype"),
							markers[i].getAttribute("connrate")));
					  }
				}
			}
		} // end of function loadMarkers


	// create the google map, load markers for initial extent, and define click listener for clicking on something other than an existing marker
    function load() {	
	
      if (GBrowserIsCompatible()) {	
		
		// create the map object 
		map = new GMap2(document.getElementById("map"));
		
		//create the geocoder object
		geocoder = new GClientGeocoder();
	
		// add the map elements
		map.addControl(new GLargeMapControl());
		map.addControl(new GMapTypeControl());
		map.addControl(new GScaleControl());
		map.setCenter(new GLatLng(37.75, -79.6), 7);
		
		// load map markers for initial extent
		loadMarkers(map);
		
		
		GEvent.addListener(map, "click", function(marker, point) {
			if (marker) {
				theIcon = marker.getIcon();
				if (theIcon == G_DEFAULT_ICON) {
					map.removeOverlay(marker);
					document.getElementById("addyourself").latitude.value = "Click on map"; 
					document.getElementById("addyourself").longitude.value = "Click on map"; 

				} // else, do nothing becuause it is a "real" marker with its own event listener.  And we don't want to delete real markers, only temporary ones.
			} else {
				newLocation = new GMarker(point);
				map.addOverlay(newLocation);
				document.getElementById("addyourself").latitude.value = newLocation.getPoint().lat().toString();
				document.getElementById("addyourself").longitude.value = newLocation.getPoint().lng().toString();
/*
				document.getElementById("addyourself").locname.value = "";
				document.getElementById("addyourself").zip.value = "";
				document.getElementById("addyourself").provider.value = ""
				document.getElementById("addyourself").cost.value = "";
				document.getElementById("addyourself").accesstech.selectedIndex = 0;
				document.getElementById("addyourself").conntype.selectedIndex = 0;
				document.getElementById("addyourself").connrate.selectedIndex = 0;
*/
			} // end if
			}); // end addListener
			
//			GEvent.addListener(map, "moveend", function(){
//				map.clearOverlays();
//				loadMarkers(map);
//			}); //end addListener
			
		} // end if
    }  //end load() function 

	
	function testmyspeed() {
		//document.getElementById("addyourself").upstream_bw.value = "foo";
		window.open('speedtest.php', 'speedtestwindow','width=650,height=600,menubar=0,toolbar=0,scrollbars=1');
	}
	
	function showmaplegend() {
		window.open('maplegend.htm', 'maplegendwindow','width=375,height=475,menubar=0,toolbar=0,scrollbars=0');
	}
	
	function zoomtozip(zip) {
		if (geocoder) {
		  geocoder.getLatLng(
			zip,
			function(point) {
		  		map.setCenter(point, 13);
			}
	  	  );
		}
	}
	
	function zoomtostate(state_abbr) {
		state_latitude = get_state_latitude(state_abbr);
		//alert("state_latitude=" + state_latitude);
		state_longitude = get_state_longitude(state_abbr);
		//alert("state_longitude=" + state_longitude);
		state_centroid = new GLatLng(state_latitude, state_longitude, true);
		//alert ("state_centroid=" + state_centroid.toString());
		state_zoomlevel = get_state_zoomlevel(state_abbr);
		//alert("state_zoomlevel=" + state_zoomlevel);
		map.setCenter(state_centroid, state_zoomlevel);
	}
	
	function get_state_latitude(state_abbr) {
		if (state_abbr == "AL") {
			result = 32.846574;
		} else if (state_abbr == "AK") {
			result = 62.91;
		} else if (state_abbr == "AZ") {
			result = 34.554393;
		} else if (state_abbr == "AR") {
			result = 34.810246;
		} else if (state_abbr == "CA") {
			result = 36.792988;
		} else if (state_abbr == "CO") {
			result = 39.246593;
		} else if (state_abbr == "CT") {
			result = 41.767153;
		} else if (state_abbr == "DE") {
			result = 39.158821;
		} else if (state_abbr == "FL") {
			result = 28.133;
		} else if (state_abbr == "GA") {
			result = 32.837452;
		} else if (state_abbr == "HI") {
			result = 21.98;
		} else if (state_abbr == "ID") {
			result = 44.504722;
		} else if (state_abbr == "IL") {
			result = 39.720476;
		} else if (state_abbr == "IN") {
			result = 39.786;
		} else if (state_abbr == "IA") {
			result = 42.031423;
		} else if (state_abbr == "KS") {
			result = 38.349457;
		} else if (state_abbr == "KY") {
			result = 37.828427;
		} else if (state_abbr == "LA") {
			result = 31.3;
		} else if (state_abbr == "ME") {
			result = 45.62541;
		} else if (state_abbr == "MD") {
			result = 39.084842;
		} else if (state_abbr == "MA") {
			result = 42.336045;
		} else if (state_abbr == "MI") {
			result = 44.211554;
		} else if (state_abbr == "MN") {
			result = 46.388173;
		} else if (state_abbr == "MS") {
			result = 32.972133;
		} else if (state_abbr == "MO") {
			result = 38.570928;
		} else if (state_abbr == "MT") {
			result = 47.060711;
		} else if (state_abbr == "NE") {
			result = 41.401758;
		} else if (state_abbr == "NV") {
			result = 39.491595;
		} else if (state_abbr == "NH") {
			result = 43.767344;
		} else if (state_abbr == "NJ") {
			result = 40.228919;
		} else if (state_abbr == "NM") {
			result = 34.664;
		} else if (state_abbr == "NY") {
			result = 42.59;
		} else if (state_abbr == "NC") {
			result = 35.634423;
		} else if (state_abbr == "ND") {
			result = 47.76972;
		} else if (state_abbr == "OH") {
			result = 39.95452;
		} else if (state_abbr == "OK") {
			result = 35.509605;
		} else if (state_abbr == "OR") {
			result = 44.098068;
		} else if (state_abbr == "PA") {
			result = 40.768873;
		} else if (state_abbr == "RI") {
			result = 41.693411;
		} else if (state_abbr == "SC") {
			result = 34.08221;
		} else if (state_abbr == "SD") {
			result = 44.371132;
		} else if (state_abbr == "TN") {
			result = 35.836064;
		} else if (state_abbr == "TX") {
			result = 31.709167;
		} else if (state_abbr == "UT") {
			result = 39.37105;
		} else if (state_abbr == "VT") {
			result = 43.909631;
		} else if (state_abbr == "VA") {
			result = 37.78;
		} else if (state_abbr == "WA") {
			result = 47.427017;
		} else if (state_abbr == "WV") {
			result = 38.993115;
		} else if (state_abbr == "WI") {
			result = 44.523452;
		} else if (state_abbr == "WY") {
			result = 43.032515;
		} else if (state_abbr == "us") {
			result = 39.81;
		} else if (state_abbr == "DC") {
			result = 38.88945;
		}
		return result;
	} // end of function get_state_latitude
	
		function get_state_longitude(state_abbr) {
		if (state_abbr == "AL") {
			result = -86.619451;
		} else if (state_abbr =="AK") {
			result = -154.94;
		} else if (state_abbr =="AZ") {
			result = -112.484934;
		} else if (state_abbr =="AR") {
			result = -92.465834;
		} else if (state_abbr =="CA") {
			result = -119.763404;
		} else if (state_abbr =="CO") {
			result = -106.290964;
		} else if (state_abbr =="CT") {
			result = -72.67558;
		} else if (state_abbr =="DE") {
			result = -75.524696;
		} else if (state_abbr =="FL") {
			result = -84.28;
		} else if (state_abbr =="GA") {
			result = -83.637898;
		} else if (state_abbr =="HI") {
			result = -159.84;
		} else if (state_abbr =="ID") {
			result = -114.230833;
		} else if (state_abbr =="IL") {
			result = -89.663809;
		} else if (state_abbr =="IN") {
			result = -86.157;
		} else if (state_abbr =="IA") {
			result = -93.655519;
		} else if (state_abbr =="KS") {
			result = -98.81239;
		} else if (state_abbr =="KY") {
			result = -85.47277;
		} else if (state_abbr =="LA") {
			result = -92.45;
		} else if (state_abbr =="ME") {
			result = -68.57514;
		} else if (state_abbr =="MD") {
			result = -77.162498;
		} else if (state_abbr =="MA") {
			result = -71.835863;
		} else if (state_abbr =="MI") {
			result = -85.415967;
		} else if (state_abbr =="MN") {
			result = -94.37064;
		} else if (state_abbr =="MS") {
			result = -89.982145;
		} else if (state_abbr =="MO") {
			result = -92.17949;
		} else if (state_abbr =="MT") {
			result = -109.442425;
		} else if (state_abbr =="NE") {
			result = -99.62321;
		} else if (state_abbr =="NV") {
			result = -117.06997;
		} else if (state_abbr =="NH") {
			result = -71.716137;
		} else if (state_abbr =="NJ") {
			result = -74.7764;
		} else if (state_abbr =="NM") {
			result = -106.73;
		} else if (state_abbr =="NY") {
			result = -76.18;
		} else if (state_abbr =="NC") {
			result = -79.782275;
		} else if (state_abbr =="ND") {
			result = -99.935496;
		} else if (state_abbr =="OH") {
			result = -82.999;
		} else if (state_abbr =="OK") {
			result = -98.99736;
		} else if (state_abbr =="OR") {
			result = -121.303934;
		} else if (state_abbr =="PA") {
			result = -77.882407;
		} else if (state_abbr =="RI") {
			result = -71.571999;
		} else if (state_abbr =="SC") {
			result = -80.996562;
		} else if (state_abbr =="SD") {
			result = -100.361502;
		} else if (state_abbr =="TN") {
			result = -86.392946;
		} else if (state_abbr =="TX") {
			result = -98.990833;
		} else if (state_abbr =="UT") {
			result = -111.587016;
		} else if (state_abbr =="VT") {
			result = -72.663091;
		} else if (state_abbr =="VA") {
			result = -79.44;
		} else if (state_abbr =="WA") {
			result = -120.313541;
		} else if (state_abbr =="WV") {
			result = -80.235537;
		} else if (state_abbr =="WI") {
			result = -89.533419;
		} else if (state_abbr =="WY") {
			result = -108.380633;
		} else if (state_abbr =="us") {
			result = -98.56;
		} else if (state_abbr =="DC") {
			result = -77.04442;
		}
		return result;
	} // end of function get_state_longitude
	
	function get_state_zoomlevel(state_abbr) {
		if (state_abbr == "AL") {
			result = 6;
		} else if (state_abbr == "AK") {
			result = 4;
		} else if (state_abbr == "AZ") {
			result = 6;
		} else if (state_abbr == "AR") {
			result = 7;
		} else if (state_abbr == "CA") {
			result = 5;
		} else if (state_abbr == "CO") {
			result = 6;
		} else if (state_abbr == "CT") {
			result = 8;
		} else if (state_abbr == "DE") {
			result = 8;
		} else if (state_abbr == "FL") {
			result = 6;
		} else if (state_abbr == "GA") {
			result = 7;
		} else if (state_abbr == "HI") {
			result = 6;
		} else if (state_abbr == "ID") {
			result = 6;
		} else if (state_abbr == "IL") {
			result = 6;
		} else if (state_abbr == "IN") {
			result = 7;
		} else if (state_abbr == "IA") {
			result = 7;
		} else if (state_abbr == "KS") {
			result = 6;
		} else if (state_abbr == "KY") {
			result = 7;
		} else if (state_abbr == "LA") {
			result = 7;
		} else if (state_abbr == "ME") {
			result = 6;
		} else if (state_abbr == "MD") {
			result = 7;
		} else if (state_abbr == "MA") {
			result = 8;
		} else if (state_abbr == "MI") {
			result = 6;
		} else if (state_abbr == "MN") {
			result = 6;
		} else if (state_abbr == "MS") {
			result = 6;
		} else if (state_abbr == "MO") {
			result = 6;
		} else if (state_abbr == "MT") {
			result = 6;
		} else if (state_abbr == "NE") {
			result = 6;
		} else if (state_abbr == "NV") {
			result = 6;
		} else if (state_abbr == "NH") {
			result = 7;
		} else if (state_abbr == "NJ") {
			result = 8;
		} else if (state_abbr == "NM") {
			result = 6;
		} else if (state_abbr == "NY") {
			result = 6;
		} else if (state_abbr == "NC") {
			result = 6;
		} else if (state_abbr == "ND") {
			result = 6;
		} else if (state_abbr == "OH") {
			result = 6;
		} else if (state_abbr == "OK") {
			result = 6;
		} else if (state_abbr == "OR") {
			result = 6;
		} else if (state_abbr == "PA") {
			result = 7;
		} else if (state_abbr == "RI") {
			result = 9;
		} else if (state_abbr == "SC") {
			result = 7;
		} else if (state_abbr == "SD") {
			result = 6;
		} else if (state_abbr == "TN") {
			result = 6;
		} else if (state_abbr == "TX") {
			result = 5;
		} else if (state_abbr == "UT") {
			result = 6;
		} else if (state_abbr == "VT") {
			result = 7;
		} else if (state_abbr == "VA") {
			result = 6;
		} else if (state_abbr == "WA") {
			result = 6;
		} else if (state_abbr == "WV") {
			result = 7;
		} else if (state_abbr == "WI") {
			result = 6;
		} else if (state_abbr == "WY") {
			result = 6;
		} else if (state_abbr == "us") {
			result = 3;
		} else if (state_abbr == "DC") {
			result = 10;
		} 
		return result;	
	}// end of function get_state_zoomlevel

	function getIndexOf(varName, varValue) {
		var indexVal=0;
		if (varName == "accesstech") {
			if (varValue =="Dial-up") {
				indexVal = 1;
			} else if (varValue =="ISDN") {
				indexVal = 2;
			} else if (varValue =="Cable Modem") {
				indexVal = 3;
			} else if (varValue =="DSL") {
				indexVal = 4;
			} else if (varValue =="Wireless (802.11/Wi-Fi)") {
				indexVal = 5;
			} else if (varValue =="Satellite") {
				indexVal = 6;
			} else if (varValue =="Leased Line (T1 or lower)") {
				indexVal = 7;
			} else if (varValue =="Ethernet") {
				indexVal = 8;
			} else if (varValue =="Leased Line (DS3 or higher)") {
				indexVal = 9;
			} else if (varValue =="Other Wired") {
				indexVal = 10;
			} else if (varValue =="Other Wireless") {
				indexVal = 11;
			} // else, indexVal remains 0, Unknown.																								
		} else if (varName == "conntype") {
			if (varValue =="Residential") {
				indexVal = 1;
			} else if (varValue =="Business") {
				indexVal = 2;
			} else if (varValue =="Government") {
				indexVal = 3;
			} else if (varValue =="Education") {
				indexVal = 4;
			} else if (varValue =="Other)") {
				indexVal = 5;
			} // else, indexVal remains 0, Unknown.
		} else if (varName == "connrate") {
			if (varValue =="Inadequate") {
				indexVal = 1;
			} // else, indexVal remains 0, Inadequate.
		} // end if
		return indexVal;
	}
