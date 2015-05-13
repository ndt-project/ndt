
$(function(){
  jQuery.fx.interval = 50;
  if (simulate) {
    setTimeout(initializeTest, 1000);
    return;
  }
  checkInstalledPlugins();
  initializeTest();
});

// CONSTANTS

// Testing phases

var PHASE_LOADING   = 0;
var PHASE_WELCOME   = 1;
var PHASE_INITIALIZING = 2;
var PHASE_PREPARING = 3;
var PHASE_UPLOAD    = 4;
var PHASE_DOWNLOAD  = 5;
var PHASE_RESULTS   = 6;


var allowDebug = false;

// STATUS VARIABLES

var use_websocket_client = false;

var websocket_client = null;

var backendInitialized = false;
var backendError = false;

var currentPhase = PHASE_LOADING;
var currentPage = 'welcome';
var transitionSpeed = 400;

// Gauges used for showing download/upload speed
var downloadGauge, uploadGauge;
var gaugeUpdateInterval;
var gaugeMaxValue = 1000; 

// PRIMARY METHODS

function initializeTest() {

  // Initialize gauges
  initializeGauges();

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
  createBackend();
  document.getElementById('javaButton').disabled = true;
  document.getElementById('websocketButton').disabled = true;
  showPage('test', resetGauges);
  document.getElementById('rttValue').innerHTML = "";
  if (simulate) return simulateTest();
  setPhase(PHASE_INITIALIZING);
  monitorTest();
  waitForBackend(5, 1000);
}

function simulateTest() {
  setPhase(PHASE_RESULTS);
  return;
  setPhase(PHASE_INITIALIZING);
  setTimeout(function(){ setPhase(PHASE_PREPARING) }, 1000);
  setTimeout(function(){ setPhase(PHASE_UPLOAD) }, 2000);
  setTimeout(function(){ setPhase(PHASE_DOWNLOAD) }, 4000);
  setTimeout(function(){ setPhase(PHASE_RESULTS) }, 6000);
}

function monitorTest() {
  var message = testError();
  var currentStatus = testStatus();

  /*
  var currentStatus = testStatus();
  debug(currentStatus);
  var diagnosis = testDiagnosis();
  debug(diagnosis);
  */

  debug(currentStatus);
  debug(message);

  if (message.match(/completed/) && currentPhase < PHASE_RESULTS) {
    setPhase(PHASE_RESULTS);
    return true;
  }
  if (message.match(/failed/) && currentPhase < PHASE_RESULTS) {
    setPhase(PHASE_RESULTS);
    return false;
  }
  if (currentStatus.match(/Outbound/) && currentPhase < PHASE_UPLOAD) {
    setPhase(PHASE_UPLOAD);
  }
  if (currentStatus.match(/Inbound/) && currentPhase < PHASE_DOWNLOAD) {
    setPhase(PHASE_DOWNLOAD);
  }

  if (!currentStatus.match(/Middleboxes/) && !currentStatus.match(/notStarted/) 
        && !remoteServer().match(/ndt/) && currentPhase == PHASE_PREPARING) {
    debug("Remote server is " + remoteServer());
    setPhase(PHASE_UPLOAD);
  }

  if (backendInitialized && currentPhase == PHASE_INITIALIZING) {
    setPhase(PHASE_PREPARING);
  }

  setTimeout(monitorTest, 1000);
}



// PHASES

function setPhase(phase) {
  switch (phase) {

    case PHASE_WELCOME:
      debug("WELCOME");
      showPage('welcome');
      break;

    case PHASE_INITIALIZING:
      debug("INITIALIZING BACKEND");
      showPage('test', resetGauges);
      $('#loading').hide();
      $('#initializingBackend').show();
      $('#upload').hide();
      $('#download').hide();
      break;

    case PHASE_PREPARING:
      uploadGauge.setValue(0);
      downloadGauge.setValue(0);
      debug("PREPARING TEST");


      $('#loading').show();
      $('#initializingBackend').hide();
      $('#upload').hide();
      $('#download').hide();
      testNDT().run_test();
      break;

    case PHASE_UPLOAD:
      var pcBuffSpdLimit, rtt, gaugeConfig = [];
      debug("UPLOAD TEST");

      pcBuffSpdLimit = speedLimit();
      rtt = averageRoundTrip();

      if (isNaN(rtt)) {
        document.getElementById("rttValue").innerHTML = "n/a";
      } else {
        document.getElementById("rttValue").innerHTML = printNumberValue(Math.round(rtt)) + " ms";
      }

      if (!isNaN(pcBuffSpdLimit)) {
        if (pcBuffSpdLimit > gaugeMaxValue) {
          pcBuffSpdLimit = gaugeMaxValue; 
        }
        gaugeConfig.push({
          from: 0,   to: pcBuffSpdLimit, color: 'rgb(0, 255, 0)'
        });

        gaugeConfig.push({
          from: pcBuffSpdLimit, to: gaugeMaxValue, color: 'rgb(255, 0, 0)'
        });

        uploadGauge.updateConfig({ 
          highlights: gaugeConfig
        });

        downloadGauge.updateConfig({ 
          highlights: gaugeConfig
        });
      }

      $('#loading').hide();
      $('#upload').show();

      gaugeUpdateInterval = setInterval(function(){
        updateGaugeValue();
      },1000);

      $("#testServerAddress").get(0).innerHTML = remoteServer();
      break;

    case PHASE_DOWNLOAD:
      debug("DOWNLOAD TEST");

      $('#upload').hide();
      $('#download').show();
      break;

    case PHASE_RESULTS:
      debug("SHOW RESULTS");
      debug('Testing complete');

      printDownloadSpeed();
      printUploadSpeed();
      document.getElementById('latency').innerHTML = printNumberValue(Math.round(averageRoundTrip())); 
      document.getElementById('jitter').innerHTML = printJitter(false); 
      document.getElementById("test-details").innerHTML = testDetails();
      document.getElementById("test-advanced").appendChild(testDiagnosis());
      document.getElementById('javaButton').disabled = false;

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


// GAUGE

function initializeGauges() {
  var gaugeValues = [];

  for (var i=0; i<=10; i++) {
    gaugeValues.push(0.1 * gaugeMaxValue * i);
  }
  uploadGauge = new Gauge({
    renderTo    : 'uploadGauge',
    width       : 270,
    height      : 270,
    units       : 'Mb/s',
    title       : "Upload",
    minValue    : 0,
    maxValue    : gaugeMaxValue,
    majorTicks  : gaugeValues,
    highlights  : [{ from: 0, to: gaugeMaxValue, color: 'rgb(0, 255, 0)' }]
  });;

  gaugeValues = [];
  for (var i=0; i<=10; i++) {
    gaugeValues.push(0.1 * gaugeMaxValue * i);
  }
  downloadGauge = new Gauge({
    renderTo    : 'downloadGauge',
    width       : 270,
    height      : 270,
    units       : 'Mb/s',
    title       : "Download",
    minValue    : 0,
    maxValue    : gaugeMaxValue,
    majorTicks  : gaugeValues,
    highlights  : [{ from: 0, to: gaugeMaxValue, color: 'rgb(0, 255, 0)' }]
  });;
}

function resetGauges() {
  var gaugeConfig = [];

  gaugeConfig.push({
    from: 0, to: gaugeMaxValue, color: 'rgb(0, 255, 0)'
  });  

  uploadGauge.updateConfig({
    highlights: gaugeConfig
  });
  uploadGauge.setValue(0);

  downloadGauge.updateConfig({ 
    highlights: gaugeConfig
  });
  downloadGauge.setValue(0);
}

function updateGaugeValue() {
  var downloadSpeedVal = downloadSpeed();
  var uploadSpeedVal = uploadSpeed(false);
  
  if (currentPhase == PHASE_UPLOAD) {
    uploadGauge.updateConfig({ 
	  units: getSpeedUnit(uploadSpeedVal)
	});
	uploadGauge.setValue(getJustifiedSpeed(uploadSpeedVal));
  } else if (currentPhase == PHASE_DOWNLOAD) {
    downloadGauge.updateConfig({ 
	  units: getSpeedUnit(downloadSpeedVal) 
	});
    downloadGauge.setValue(getJustifiedSpeed(downloadSpeedVal));
  } else {
    clearInterval(gaugeUpdateInterval);  
  }   
}

// TESTING JAVA/WEBSOCKET CLIENT

function testNDT() {
  if (websocket_client) {
    return websocket_client;
  }

  return document.getElementById('NDT');
}

function testStatus() {
  if (!backendInitialized) {
    return "notStarted";
  }

  if (backendError) {
    return "failed";
  }

  return testNDT().get_status();
}

function testDiagnosis() {
  var div = document.createElement("div");
  
  if (simulate) {
    div.innerHTML = "Test diagnosis";
    return div;
  }

  if (backendError) {
    div.innerHTML = testError();
    return div;
  }

  var diagnosisArray = testNDT().get_diagnosis().split('\n');
  var txt = '';
  var table;
  var isTable = false;

  diagnosisArray.forEach(
    function addRow(value) { 
      if (isTable) {
        rowArray = value.split(':');
        if (rowArray.length>1) {
          var row = table.insertRow(-1);
          rowArray.forEach(
            function addCell(cellValue, idx) { 
              var cell =row.insertCell(idx);
              cell.innerHTML = cellValue;
            }
          );
        } else { 
          isTable = false;
          txt = txt + value;
        }		
      } else {  
        if (value.indexOf('=== Results sent by the server ===') != -1) { 
          table = document.createElement("table");
          isTable = true;
        } else {
          txt = txt + value + "\n";
        }
      }
    }
  );
  txt = txt + "=== Results sent by the server ===";
  div.innerHTML = txt;
  if (isTable) {
    div.appendChild(table);
  }

  return div;
}

function testError() {
  if (backendError) {
    return "Test failed: Backend did not load properly. Make sure that you have proper plugin installed, and that it is not being blocked by your browser.";
  }

  if (!backendInitialized) {
    return "";
  }

  return testNDT().get_errmsg();
}

function remoteServer() {
  if (simulate) return '0.0.0.0';
  if (!backendInitialized || backendError) {
    return "unknown";
  }
  return testNDT().get_host();
}

function uploadSpeed(raw) {
  if (simulate || backendError) return 0;
  var speed = testNDT().getNDTvar("ClientToServerSpeed");
  return raw ? speed : parseFloat(speed);
}

function downloadSpeed() {
  if (simulate || backendError) return 0;
  return parseFloat(testNDT().getNDTvar("ServerToClientSpeed"));
}

function averageRoundTrip() {
  if (simulate || backendError) return 0;
  return parseFloat(testNDT().getNDTvar("avgrtt"));
}

function jitter() {
  if (simulate || backendError) return 0;
  return parseFloat(testNDT().getNDTvar("Jitter"));
}

function speedLimit() {
  if (simulate || backendError) return 0;
  return parseFloat(testNDT().get_PcBuffSpdLimit());
}

function printPacketLoss() {
  var packetLoss = parseFloat(testNDT().getNDTvar("loss"));
  packetLoss = (packetLoss*100).toFixed(2);
  return packetLoss;
}

function printJitter(boldValue) {
  var retStr = '';
  var jitterValue = jitter();
  if (jitterValue >= 1000) {
    retStr += (boldValue ? '<b>' : '') + printNumberValue(jitterValue/1000) + (boldValue ? '</b>' : '') + ' sec';
  } else {
    retStr += (boldValue ? '<b>' : '') + printNumberValue(jitterValue) + (boldValue ? '</b>' : '') + ' msec';
  }
  return retStr;
}

function getSpeedUnit(speedInKB) {
  var unit = ['kb/s', 'Mb/s', 'Gb/s', 'Tb/s', 'Pb/s', 'Eb/s'];
  var e = Math.floor(Math.log(speedInKB*1000) / Math.log(1000));
  return unit[e];
}

function getJustifiedSpeed(speedInKB) {
  var e = Math.floor(Math.log(speedInKB) / Math.log(1000));
  return (speedInKB / Math.pow(1000, e)).toFixed(2);
}

function printDownloadSpeed() {
  var downloadSpeedVal = downloadSpeed();
  document.getElementById('download-speed').innerHTML = getJustifiedSpeed(downloadSpeedVal);  
  document.getElementById('download-speed-units').innerHTML = getSpeedUnit(downloadSpeedVal);  
}

function printUploadSpeed() {
  var uploadSpeedVal = uploadSpeed(false);
  document.getElementById('upload-speed').innerHTML = getJustifiedSpeed(uploadSpeedVal); 
  document.getElementById('upload-speed-units').innerHTML = getSpeedUnit(uploadSpeedVal);    
}

function readNDTvar(variable) {
  var ret = testNDT().getNDTvar(variable);
  return !ret ? "-" : ret; 
}

function printNumberValue(value) {
  return isNaN(value) ? "-" : value;
}

function testDetails() {
  if (simulate) return 'Test details';
  if (backendError) return testError();

  var d = '';

  var errorMsg = testError();
  if (errorMsg.match(/failed/)) {
    d += "Error occured while performing test: <br>".bold();
    if (errorMsg.match(/#2048/)) {
      d += "Security error. This error may be caused by firewall issues, make sure that port 843 is available on the NDT server, and that you can access it.".bold().fontcolor("red") + "<br><br>";
    } else {    
      d += errorMsg.bold().fontcolor("red") + "<br><br>";
    }
  }

  d += "Your system: " + readNDTvar("OperatingSystem").bold() + "<br>";
  d += "Plugin version: " + (readNDTvar("PluginVersion") + " (" + readNDTvar("OsArchitecture") + ")<br>").bold();

  d += "<br>";

  d += "TCP receive window: " + readNDTvar("CurRwinRcvd").bold() + " current, " + readNDTvar("MaxRwinRcvd").bold() + " maximum<br>";
  d += "<b>" + printNumberValue(printPacketLoss()) + "</b> % of packets lost during test<br>";
  d += "Round trip time: " + readNDTvar("MinRTT").bold() + " msec (minimum), " + readNDTvar("MaxRTT").bold() + " msec (maximum), <b>" + printNumberValue(Math.round(averageRoundTrip())) + "</b> msec (average)<br>";
  d += "Jitter: " + printNumberValue(printJitter(true)) + "<br>";
  d += readNDTvar("waitsec").bold() + " seconds spend waiting following a timeout<br>";
  d += "TCP time-out counter: " + readNDTvar("CurRTO").bold() + "<br>";
  d += readNDTvar("SACKsRcvd").bold() + " selective acknowledgement packets received<br>";

  d += "<br>";

  if (readNDTvar("mismatch") == "yes") {
    d += "A duplex mismatch condition was detected.<br>".fontcolor("red").bold();
  }
  else {
    d += "No duplex mismatch condition was detected.<br>".fontcolor("green");
  }

  if (readNDTvar("bad_cable") == "yes") {
    d += "The test detected a cable fault.<br>".fontcolor("red").bold();
  }
  else {
    d += "The test did not detect a cable fault.<br>".fontcolor("green");
  }

  if (readNDTvar("congestion") == "yes") {
    d += "Network congestion may be limiting the connection.<br>".fontcolor("red").bold();
  }
  else {
    d += "No network congestion was detected.<br>".fontcolor("green");
  }

  /*if (testNDT().get_natStatus() == "yes") {
    d += "A network address translation appliance was detected.<br>";
  }
  else {
    d += "No network addess translation appliance was detected.<br>";
  }*/

  d += "<br>";

  d += printNumberValue(readNDTvar("cwndtime")).bold() + " % of the time was not spent in a receiver limited or sender limited state.<br>";
  d += printNumberValue(readNDTvar("rwintime")).bold() + " % of the time the connection is limited by the client machine's receive buffer.<br>";
  d += "Optimal receive buffer: " + printNumberValue(readNDTvar("optimalRcvrBuffer")).bold() + " bytes<br>";
  d += "Bottleneck link: " + readNDTvar("accessTech").bold() + "<br>";
  d += readNDTvar("DupAcksIn").bold() + " duplicate ACKs set<br>";

  return d;
}

// BACKEND METHODS
function useJavaAsBackend() {
  $('#warning-websocket').hide();
  $("#rtt").show();  
  $("#rttValue").show();  

  $('.warning-environment').innerHTML = "";

  use_websocket_client = false;

  createBackend();

  $('#websocketButton').removeClass("active");
  $('#javaButton').addClass("active");
}

function useWebsocketAsBackend() {
  $("#rtt").hide();  
  $("#rttValue").hide();  
  $('#warning-websocket').show();

  use_websocket_client = true;

  createBackend();

  $('#javaButton').removeClass("active");
  $('#websocketButton').addClass("active");
}

function createBackend() {
  $('#backendContainer').empty();

  backendInitialized = false;
  backendError = false;

  if (use_websocket_client) {
    websocket_client = new NDTWrapper();
    $("#backendDescription").get(0).innerHTML = "WebSocket";
  }
  else {
    var app = document.createElement('applet');
    app.id = 'NDT';
    app.name = 'NDT';
    app.archive = 'Tcpbw100.jar';
    app.code = 'edu.internet2.ndt.Tcpbw100.class';
    app.width = '600';
    app.height = '10';
    $('#backendContainer').append(app);
    $("#backendDescription").get(0).innerHTML = "Java";
  }
}

// UTILITIES

/* Attempt to load the applet up to "X" times with a delay. If it succeeds, then execute the callback function. */
function waitForBackend(attempts, delay) {
  if (isPluginLoaded()) {
    backendInitialized = true;
  }
  else if (attempts == 0) {
    backendError = true;
    backendInitialized = false;
  }
  else {
    setTimeout(function () {
      waitForBackend(--attempts, delay);
    }, delay);
  }
}

function debug(message) {
  if (allowDebug && window.console) console.debug(message);
}

function isPluginLoaded() {
  try {
    testNDT().get_status();
    return true;
  } catch(e) {
    return false;
  }
}

function checkInstalledPlugins() {
  var hasJava = false;
  var hasWebsockets = false;

  $('#warning-plugin').hide();
  $('#warning-websocket').hide();

  hasJava = true;
  if (deployJava.getJREs() == '') {
    hasJava = false;
  }

  hasWebsockets = false;
  try {
    ndt_js = new NDTjs();
    if (ndt_js.checkBrowserSupport()) {
      hasWebsockets = true; 
    }
  } catch(e) {
    hasWebsockets = false;
  }

  if (!hasWebsockets) {
    document.getElementById('websocketButton').disabled = true;
  }

  if (!hasJava) {
    document.getElementById('javaButton').disabled = true;
  }
 
  if (hasJava) { 
    useJavaAsBackend();
  }
  else if (hasWebsockets) {
    useWebsocketAsBackend();
  }
}
