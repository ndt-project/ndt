
$(function(){
  jQuery.fx.interval = 50;
  if (simulate) {
    setTimeout(initializeTest, 1000);
    return;
  }
  initializeTest();
});



// CONSTANTS

// Testing phases

var PHASE_LOADING   = 0;
var PHASE_WELCOME   = 1;
var PHASE_PREPARING = 2;
var PHASE_UPLOAD    = 3;
var PHASE_DOWNLOAD  = 4;
var PHASE_RESULTS   = 5;


// STATUS VARIABLES

var currentPhase = PHASE_LOADING;
var currentPage = 'welcome';
var transitionSpeed = 400;

// Stats from the previous test, for retesting
var lastUploadSpeed = false;


// PRIMARY METHODS

function initializeTest() {

  // Test throbber
  initializeThrobber();

  // Initialize start buttons
  $('.start.button').click(startTest);

  // Results view selector
  $("#results .view-selector .summary").click(showResultsSummary);
  $("#results .view-selector .details").click(showResultsDetails);
  $("#results .view-selector .advanced").click(showResultsAdvanced);

  document.body.className = 'ready';
  //return showPage('results');
  setPhase(PHASE_WELCOME);
}

function startTest(evt) {
  evt.stopPropagation();
  evt.preventDefault();
  showPage('test');
  if (simulate) return simulateTest();
  currentPhase = PHASE_WELCOME;
  testApplet().run_test();
  monitorTest();
}

function simulateTest() {
  setPhase(PHASE_RESULTS);
  return;
  setPhase(PHASE_PREPARING);
  setTimeout(function(){ setPhase(PHASE_UPLOAD) }, 2000);
  setTimeout(function(){ setPhase(PHASE_DOWNLOAD) }, 4000);
  setTimeout(function(){ setPhase(PHASE_RESULTS) }, 6000);
}

function monitorTest() {
  var message = testError();

  /*
  var currentStatus = testStatus();
  debug(currentStatus);
  var diagnosis = testDiagnosis();
  debug(diagnosis);
  */

  if (message.match(/not run/) && currentPhase != PHASE_LOADING) {
    setPhase(PHASE_WELCOME);
    return false;
  }
  if (message.match(/completed/) && currentPhase < PHASE_RESULTS) {
    setPhase(PHASE_RESULTS);
    return true;
  }

  var uploadTestComplete = false;
  if (currentPhase == PHASE_UPLOAD) {
    if (lastUploadSpeed != false) {
      uploadTestComplete = lastUploadSpeed != uploadSpeed(true);
    }
    else {
      uploadTestComplete = uploadSpeed(true) > 0;
    }
    if (uploadTestComplete) {
      lastUploadSpeed = uploadSpeed(true);
      debug("Upload speed is " + uploadSpeed(true));
      setPhase(PHASE_DOWNLOAD);
    }
  }
  if (!remoteServer().match(/ndt/) && currentPhase == PHASE_PREPARING) {
    debug("Remote server is " + remoteServer());
    setPhase(PHASE_UPLOAD);
  }
  if (remoteServer() !== 'unknown' && currentPhase < PHASE_PREPARING) {
    setPhase(PHASE_PREPARING);
  }
  setTimeout(monitorTest, 100);
}



// PHASES

function setPhase(phase) {
  switch (phase) {

    case PHASE_WELCOME:
      debug("WELCOME");
      showPage('welcome');
      break;

    case PHASE_PREPARING:
      debug("PREPARING TEST");

      $('#loading').show();
      $('#upload').hide();
      $('#download').hide();

      showPage('test', startThrobber);
      break;

    case PHASE_UPLOAD:
      debug("UPLOAD TEST");

      $("#test .remote.location .address").get(0).innerHTML = remoteServer();

      stopThrobber();
      $('#loading').fadeOut(transitionSpeed, function(){
        $('#upload').fadeIn(transitionSpeed, function(){
          startBar('upload');
        });
      });
      break;

    case PHASE_DOWNLOAD:
      debug("DOWNLOAD TEST");
      stopBar('upload');
      $('#upload').fadeOut(transitionSpeed, function(){
        $('#download').fadeIn(transitionSpeed, function(){
          startBar('download');
        });
      });
      break;

    case PHASE_RESULTS:
      debug("SHOW RESULTS");
      debug('Testing complete');
      stopBar('download');

      document.getElementById('upload-speed').innerHTML = uploadSpeed().toPrecision(2); 
      document.getElementById('download-speed').innerHTML = downloadSpeed().toPrecision(2); 
      document.getElementById('latency').innerHTML = averageRoundTrip().toPrecision(2); 
      document.getElementById('jitter').innerHTML = jitter().toPrecision(2); 
      document.getElementById("test-details").innerHTML = testDetails();
      document.getElementById("test-advanced").innerHTML = testDiagnosis();

      showPage('results');
      break;

    default:
      return false;
  }
  currentPhase = phase;
}


// PAGES

function showPage(page, callback) {
  debug("Show page: " + page);
  if (page == currentPage) {
    if (callback !== undefined) callback();
    return true;
  }
  if (currentPage !== undefined) {
    $("#" + currentPage).fadeOut(transitionSpeed, function(){
      $("#" + page).fadeIn(transitionSpeed, callback);
    });
  }
  else {
    debug("No current page");
    $("#" + page).fadeIn(transitionSpeed, callback);
  }
  currentPage = page;
}


// RESULTS

function showResultsSummary() {
  showResultsPage("summary");
}

function showResultsDetails() {
  showResultsPage("details");
}

function showResultsAdvanced() {
  showResultsPage("advanced");
}

function showResultsPage(page) {
  debug("Results: show " + page);
  var pages = ["summary", "details", "advanced"];
  for (var i=0, len=pages.length; i < len; i++) {
    $("#results")[(page == pages[i]) ? "addClass" : "removeClass"](pages[i]);
  }
}



// THROBBER 

var throbberPos = -1;
var throbberSpeed = 500;

function initializeThrobber() {
  var dot;
  var b = bar('loading');
  for (var i=0, len=8; i < len; i++) {
    dot = document.createElement('div');
    dot.className = 'dot';
    dot.style.left = (i * 80) + "px";
    dot.style.display = "none";
    b.appendChild(dot);
  }
}

function startThrobber() {
  debug('Start throbber');
  throbberPos = -1;
  advanceThrobber();
}

function advanceThrobber() {
  if (throbberPos >= 0) {
    $("#loading .dot:eq(" + throbberPos + ")").fadeOut(throbberSpeed);
  }
  incrementThrobber();
  $("#loading .dot:eq(" + throbberPos + ")").fadeIn(throbberSpeed, advanceThrobber);
}

function incrementThrobber() {
  var len = $("#loading .dot").size();
  throbberPos++;
  if (throbberPos >= len) throbberPos = 0;
}

function stopThrobber() {
  debug('Stop throbber');
  $("#loading .dot").stop(true).fadeOut(throbberSpeed);
}

// Upload/download animations

var barSpeed = 3000;

function bar(test) {
  return $(".progress-bar", document.getElementById(test)).get(0);
}

function startBar(test) {
  addSegment(test);
}

function stopBar(test) {
  $(".segment", bar(test)).stop(true).fadeOut();
}

function addSegment(test) {
  var seg = newSegment(test);
  seg.style.left = test == "upload" ? "640px" : "-640px";
  var b = bar(test); 
  b.appendChild(seg);
  $(seg).animate({ left: "0" }, barSpeed, 'linear', removeSegment);
}

function removeSegment() {
  var test = $(this).parents(".test").get(0).id;
  var pos = test == "upload" ? "-640" : "640";
  $(this).animate({ left: pos }, barSpeed, 'linear', deleteSegment);
  addSegment(test);
}

function deleteSegment() {
  var test = $(this).parents(".test").get(0).id;
  bar(test).removeChild(this);
}
  
function newSegment() {
  var seg = document.createElement('div'), dot = null;
  seg.className = 'segment';
  /*
  for (var i=0, len=8; i < len; i++) {
    dot = document.createElement('div');
    dot.className = 'dot';
    dot.style.left = (i * 80) + "px";
    seg.appendChild(dot);
  }
  */
  return seg;
}





// TESTING APPLET

function testApplet() {
  return applet = document.applets['NDT'];
}

function testStatus() {
  return testApplet().get_status();
}

function testDiagnosis() {
  if (simulate) return "Test diagnosis";

  var tempArray = testNDT().get_diagnosis().split('\n');
  var d = '';

  var table = false;
  for (var i = 0; i < tempArray.length; i++) {
    if (table) {
      d = d + "<tr><td>";
      d = d + tempArray[i].replace(":", "</td><td>");
      d = d + "</td></tr>";
    }  
    else {
      d = d + tempArray[i] + "\n";
    }
    if (tempArray[i].indexOf('=== Results sent by the server ===') != -1) { 
      d = d + "<table>";
      table = true;
      continue;
    }
    if (table && (i+1 == tempArray.length || tempArray[i+1] == "")) {
      d = d + "</table>";
      table = false;
    }
  }
  return d;
}

function testError() {
  return testApplet().get_errmsg();
}

function remoteServer() {
  if (simulate) return '0.0.0.0';
  return testApplet().get_host();
}

function uploadSpeed(raw) {
  if (simulate) return 0;
  var speed = testApplet().get_c2sspd();
  return raw ? speed : parseFloat(speed);
}

function downloadSpeed() {
  if (simulate) return 0;
  return parseFloat(testApplet().get_s2cspd());
}

function averageRoundTrip() {
  if (simulate) return 0;
  return parseFloat(testApplet().get_avgrtt());
}

function jitter() {
  if (simulate) return 0;
  return parseFloat(testApplet().get_jitter());
}

function testDetails() {
  if (simulate) return 'Test details';

  var a = testApplet();
  var d = '';

  d += "Your system: " + a.get_osName() + " version " + a.get_osVer() + "<br>";
  d += "Java version: " + a.get_javaVer() + " (" + a.get_osArch() + ")<br>";

  d += "<br>";

  d += "TCP receive window: " + a.get_CurRwinRcvd() + " current, " + a.get_MaxRwinRcvd() + " maximum<br>";
  d += a.get_loss() + " packets lost during test<br>";
  d += "Round trip time: " + a.get_Ping() + " msec (minimum), " + a.get_MaxRTT() + " msec (maximum), " + a.get_avgrtt() + " msec (average)<br>";
  d += "Jitter: " + a.get_jitter() + " msec<br>";
  d += a.get_WaitSec() + " seconds spend waiting following a timeout<br>";
  d += "TCP time-out counter: " + a.get_CurRTO() + "<br>";
  d += a.get_SACKsRcvd() + " selective acknowledgement packets received<br>";

  d += "<br>";

  if (a.get_mismatch() == "yes") {
    d += "A duplex mismatch condition was detected.<br>";
  }
  else {
    d += "No duplex mismatch condition was detected.<br>";
  }

  if (a.get_Bad_cable() == "yes") {
    d += "The test detected a cable fault.<br>";
  }
  else {
    d += "The test did not detect a cable fault.<br>";
  }

  if (a.get_congestion() == "yes") {
    d += "Network congestion may be limiting the connection.<br>";
  }
  else {
    d += "No network congestion was detected.<br>";
  }

  if (a.get_natStatus() == "yes") {
    d += "A network addess translation appliance was detected.<br>";
  }
  else {
    d += "No network addess translation appliance was detected.<br>";
  }

  d += "<br>";

  d += a.get_cwndtime() + "% of the time was not spent in a receiver limited or sender limited state.<br>";
  d += a.get_rcvrLimiting() + "% of the time the connection is limited by the client machine's receive buffer.<br>";
  d += "Optimal receive buffer: " + a.get_optimalRcvrBuffer() + " bytes<br>";
  d += "Bottleneck link: " + a.get_AccessTech() + "<br>";
  d += a.get_DupAcksOut() + " duplicate ACKs set<br>";

  return d;
}


// UTILITIES

function debug(message) {
  if (allowDebug && window.console) console.debug(message);
}

