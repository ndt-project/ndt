// Copyright 2013 M-Lab
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package  {
  import flash.errors.IOError;
  import flash.events.Event;
  import flash.events.IOErrorEvent;
  import flash.events.ProgressEvent;
  import flash.events.SecurityErrorEvent;
  import flash.events.TimerEvent;
  import flash.net.Socket;
  import flash.system.Security;
  import flash.utils.Timer;
  import mx.resources.ResourceManager;
  import flash.external.ExternalInterface;

  /**
   * This class creates (and closes) the Control socket with the server and
   * coordinates the NDT protocol, which includes the initial handshake, all
   * the measurement tests and the retrieval of the test results.
   */
  public class NDTPController {
    private static var _instance:NDTPController = null;
    private const READ_TIMEOUT:int = 10000;  // 10sec
    // Valid values for _testStage.
    private const REMOTE_RESULTS1:int = 0;
    private const REMOTE_RESULTS2:int = 1;

    private var _ctlSocket:Socket = null;
    private var _hostname:String;
    private var _msg:Message;
    private var _readResultsTimer:Timer;
    private var _remoteTestResults:String;
    private var _testsToRun:Array;
    private var _testStage:int;
    private var _clientApplication:String;
    private var _currentTest:int = 0;

    public function NDTPController(hostname:String, clientApplication:String) {
      _hostname = hostname;
      _clientApplication = clientApplication;

      _remoteTestResults = ""
    }

    public static function getInstance():NDTPController {
      if (_instance == null) {
        _instance = new NDTPController(Main.server_hostname,
                                       Main.client_application);
      }
      return _instance;
    }

    public function startNDTTest():void {
      _currentTest = 0;
      // socket was once opened so do not clear previous debug messages
      // and just try to connect again to server using old MSG_LOGIN message
      if (_ctlSocket) {
        removeSocketEventListeners();
      } else {
        TestResults.clearResults();
      }

      var hostnameQueryFn:String = "getNDTServer";
      if (Main.bad_runtime_action != NDTConstants.ENV_OK) {
        TestResults.appendDebugMsg("Download-limited Flash Runtime detected");
        if (Main.bad_runtime_action == NDTConstants.BAD_ENV_WARN_AND_LIMIT) {
          hostnameQueryFn = "getFallbackNDTServer"
          TestResults.appendDebugMsg("Reverting to fallback " 
              + "test server due to bad flash runtime");
        }
        if (Main.bad_runtime_action == NDTConstants.BAD_ENV_ERROR) {
          failNDTTest();
          TestResults.appendErrMsg(NDTConstants.BAD_ENV_MESG);
          TestResults.ndt_test_results::ndtGuiErrMsg = NDTConstants.BAD_ENV_MESG;
          return;
        }
      }

      var js_server_hostname:String = NDTUtils.hostnameFromJS(hostnameQueryFn);
      if (js_server_hostname) {
        Main.server_hostname = js_server_hostname;
        _hostname = Main.server_hostname;
      } else if (Main.bad_runtime_action == NDTConstants.BAD_ENV_WARN_AND_LIMIT) {
	  TestResults.appendErrMsg("This Flash runtime is download-limited, client is "
	    + "set to restrict limited runtimes to fallback servers only, and no "
	    + "getFallbackNDTServer JavaScript function was defined.");
	  failNDTTest();
	  return;
      } else if (Main.server_hostname) {
		_hostname = Main.server_hostname;
        }

      TestResults.recordStartTime();
      TestResults.ndt_test_results::ndtTestFailed = false;
      TestResults.ndt_test_results::ndtErrStatus = "Test in progress.";
      TestResults.appendDebugMsg(ResourceManager.getInstance().getString(
              NDTConstants.BUNDLE_NAME, "connectingTo", null, Main.locale)
          + " " + _hostname + " " + ResourceManager.getInstance().getString(
              NDTConstants.BUNDLE_NAME, "toRunTest", null, Main.locale));

      _ctlSocket = new Socket();
      addSocketEventListeners();
      try {
        _ctlSocket.connect(_hostname, NDTConstants.DEFAULT_CONTROL_PORT);
      } catch(e:IOError) {
        TestResults.appendErrMsg("Control socket connect IO error: " + e);
        failNDTTest();
      } catch(e:SecurityError) {
        TestResults.appendErrMsg("Control socket connect security error: " + e);
        failNDTTest();
      }
      TestResults.appendDebugMsg(                                                
          ResourceManager.getInstance().getString(                               
              NDTConstants.BUNDLE_NAME, "connected", null, Main.locale)
              + " " + _hostname);
    }

    private function addSocketEventListeners():void {
      _ctlSocket.addEventListener(Event.CONNECT, onConnect);
      _ctlSocket.addEventListener(Event.CLOSE, onClose);
      _ctlSocket.addEventListener(IOErrorEvent.IO_ERROR, onIOError);
      _ctlSocket.addEventListener(SecurityErrorEvent.SECURITY_ERROR,
                                  onSecurityError);
    }

    private function removeSocketEventListeners():void {
      _ctlSocket.removeEventListener(Event.CONNECT, onConnect);
      _ctlSocket.removeEventListener(Event.CLOSE, onClose);
      _ctlSocket.removeEventListener(IOErrorEvent.IO_ERROR, onIOError);
      _ctlSocket.removeEventListener(SecurityErrorEvent.SECURITY_ERROR,
                                     onSecurityError);
    }

    private function onConnect(e:Event):void {
      TestResults.appendDebugMsg("Control socket connected.");
      startHandshake();
    }

    private function onClose(e:Event):void {
      TestResults.appendDebugMsg("Control socket closed by server.");
    }

    private function onIOError(e:IOErrorEvent):void {
      TestResults.appendErrMsg("IOError on Control socket: " + e);
      failNDTTest();
    }

    private function onSecurityError(e:SecurityErrorEvent):void {
      TestResults.appendErrMsg("Security error on Control socket: " + e);
      failNDTTest();
    }

    private function startHandshake():void {
      var handshake:Handshake = new Handshake(
          _ctlSocket, NDTConstants.TESTS_REQUESTED_BY_CLIENT, this);
      handshake.run();
    }

    public function initiateTests(testsConfirmedByServer:String):void {
      _testsToRun = testsConfirmedByServer.split(" ");
      runTests();
    }

    public function runTests():void {
      if (Main.guiEnabled) {
        Main.gui.updateProgressText(_currentTest, _testsToRun.length);
      }

      if (_testsToRun.length > _currentTest) {
        var currentTestCode:int = parseInt(_testsToRun[_currentTest]);
        _currentTest++;
        switch (currentTestCode) {
          case TestType.C2S:
              var c2s:TestC2S = new TestC2S(_ctlSocket, _hostname, this);
              c2s.run();
              break;
          case TestType.S2C:
              var s2c:TestS2C = new TestS2C(_ctlSocket, _hostname, this);
              s2c.run();
              break;
          case TestType.META:
              var meta:TestMETA = new TestMETA(_ctlSocket,
	                                       _clientApplication,
					       this);
              meta.run();
              break;
          default:
              TestResults.appendErrMsg(ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "unknownID", null, Main.locale));
              failNDTTest();
        }
      } else {
        addOnReceivedDataListener();
        _msg = new Message();
        _testStage = REMOTE_RESULTS1;
        _readResultsTimer = new Timer(READ_TIMEOUT);
        _readResultsTimer.addEventListener(TimerEvent.TIMER, onReadTimeout);
        _readResultsTimer.start();
        if (_ctlSocket.bytesAvailable > 0)
          // In case data arrived before starting the onReceiveData listener.
          getRemoteResults1();
      }
    }

    private function addOnReceivedDataListener():void {
      _ctlSocket.addEventListener(ProgressEvent.SOCKET_DATA, onReceivedData);
    }

    private function removeOnReceivedDataListener():void {
      _ctlSocket.removeEventListener(ProgressEvent.SOCKET_DATA, onReceivedData);
    }

    private function onReceivedData(e:ProgressEvent):void {
      switch (_testStage) {
        case REMOTE_RESULTS1:
          getRemoteResults1()
          break;
        case REMOTE_RESULTS2:
          getRemoteResults2()
          break;
      }
    }

    private function onReadTimeout(e:TimerEvent):void {
      _readResultsTimer.stop();
      TestResults.appendErrMsg(ResourceManager.getInstance().getString(
          NDTConstants.BUNDLE_NAME, "resultsTimeout", null, Main.locale));
      failNDTTest();
    }

    private function getRemoteResults1():void {
      if (!_msg.readHeader(_ctlSocket))
        return;
      if (!_msg.readBody(_ctlSocket, _msg.length))
        return;

      if (_msg.type == MessageType.MSG_LOGOUT) {
        // All results obtained.
        _readResultsTimer.stop();
        removeOnReceivedDataListener();
        succeedNDTTest();
        return;
      }

      _testStage = REMOTE_RESULTS2;
      if (_ctlSocket.bytesAvailable > 0)
        // In case header and body have arrive together at the client, they
        // trigger a single ProgressEvent.SOCKET_DATA event. In such case, it's
        // necessary to explicitly call the following function to move to the
        // next step.
        getRemoteResults2();
    }

    private function getRemoteResults2():void {
      if (_msg.type != MessageType.MSG_RESULTS) {
        TestResults.appendErrMsg(ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "resultsWrongMessage", null,
            Main.locale));
        _readResultsTimer.stop();
        removeOnReceivedDataListener();
        failNDTTest();
        return;
      }

      _remoteTestResults += Main.jsonSupport ?
                            new String(NDTUtils.readJsonMapValue(
                              String(_msg.body),
                              NDTUtils.JSON_DEFAULT_KEY))
                          : new String(_msg.body);
      _msg = new Message();
      _testStage = REMOTE_RESULTS1;
      if (_ctlSocket.bytesAvailable > 0)
        // In case two consecutive MSG_RESULTS messages arrive together at the
        // client they trigger a single ProgressEvent.SOCKET_DATA event. In such
        // case, it's necessary to explicitly call the following function to
        // move to the next step.
        getRemoteResults1();
    }

    public function failNDTTest():void {
      TestResults.ndt_test_results::ndtTestFailed = true;
      NDTUtils.callExternalFunction("fatalErrorOccured");
      TestResults.appendErrMsg("NDT test failed.");
      finishNDTTest();
    }

    public function succeedNDTTest():void {
      TestResults.ndt_test_results::ndtTestFailed = false;
      NDTUtils.callExternalFunction("allTestsCompleted");
      TestResults.ndt_test_results::ndtTestStatus = "allTestsCompleted";
      TestResults.appendDebugMsg("<font color=\"#7CFC00\">"
        + "All the tests completed successfully." + "</font>");
      finishNDTTest();
    }

    private function finishNDTTest():void {
      try {
      } catch (e:IOError) {
        TestResults.appendErrMsg(
            "Client failed to close Control socket. Error" + e);
      }

      if (!TestResults.ndt_test_results::ndtTestFailed) {
        TestResults.ndt_test_results::ndtErrStatus = "All tests completed OK.";
      } else {
        TestResults.ndt_test_results::ndtErrStatus = TestResults.getErrMsg();
      }
      TestResults.recordEndTime();
      TestResults.ndt_test_results::remoteTestResults += _remoteTestResults;

      TestResults.interpretResults();
      if (Main.guiEnabled) {
        Main.gui.displayResults();
      }
    }
  }
}

