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

  import flash.net.Socket;
  import flash.events.ProgressEvent;
  import flash.utils.ByteArray;
  import flash.system.Capabilities;
  import mx.resources.ResourceManager;

  /**
   * This class performs the META test.
   */
  public class TestMETA {
    // Valid values for _testStage.
    private static const PREPARE_TEST1:int = 0;
    private static const PREPARE_TEST2:int = 1;
    private static const START_TEST:int    = 2;
    private static const SEND_DATA:int     = 3;
    private static const FINALIZE_TEST:int = 4;
    private static const END_TEST:int  = 5;

    private var _callerObj:NDTPController;
    private var _ctlSocket:Socket;
    private var _metaTestSuccess:Boolean;
    private var _msg:Message;
    private var _testStage:int;

    public function TestMETA(ctlSocket:Socket, callerObject:NDTPController) {
      _callerObj = callerObject;
      _ctlSocket = ctlSocket;

      _metaTestSuccess = true;  // Initially the test has not failed.
    }

    public function run():void {
      TestResults.appendDebugMsg(
          ResourceManager.getInstance().getString(
              NDTConstants.BUNDLE_NAME, "startingTest", null, Main.locale) +
          ResourceManager.getInstance().getString(
              NDTConstants.BUNDLE_NAME, "meta", null, Main.locale));
      NDTUtils.callExternalFunction("testStarted", "Meta");
      TestResults.appendDebugMsg("META test: PREPARE_TEST stage.");
      TestResults.appendDebugMsg(
          ResourceManager.getInstance().getString(
              NDTConstants.BUNDLE_NAME, "sendingMetaInformation", null,
              Main.locale));
      TestResults.ndt_test_results::ndtTestStatus = "sendingMetaInformation";

      addOnReceivedDataListener();
      _msg = new Message();
      _testStage = PREPARE_TEST1;
      if(_ctlSocket.bytesAvailable > 0)
        // In case data arrived before starting the ProgressEvent.SOCKET_DATA
        // listener.
        prepareTest1();
    }

    private function addOnReceivedDataListener():void {
      _ctlSocket.addEventListener(ProgressEvent.SOCKET_DATA, onReceivedData);
    }

    private function removeResponseListener():void {
      _ctlSocket.removeEventListener(ProgressEvent.SOCKET_DATA, onReceivedData);
    }

    private function onReceivedData(e:ProgressEvent):void {
      switch (_testStage) {
        case PREPARE_TEST1: prepareTest1();
                            break;
        case PREPARE_TEST2: prepareTest2();
                            break;
        case START_TEST:    startTest();
                            break;
        case FINALIZE_TEST: finalizeTest();
                            break;
        case END_TEST:      endTest();
                            break;
      }
    }

    private function prepareTest1():void {
      if (!_msg.readHeader(_ctlSocket))
        return;

      _testStage = PREPARE_TEST2;
      if (_ctlSocket.bytesAvailable > 0)
        // In case header and body have arrive together at the client, they
        // trigger a single ProgressEvent.SOCKET_DATA event. In such case, it's
        // necessary to explicitly call the following function to move to the
        // next step.
        prepareTest2();
    }

    private function prepareTest2():void {
      if (!_msg.readBody(_ctlSocket, _msg.length))
        return;

      if (_msg.type != MessageType.TEST_PREPARE) {
        TestResults.appendErrMsg(ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "metaWrongMessage", null, Main.locale));
        if (_msg.type == MessageType.MSG_ERROR) {
          TestResults.appendErrMsg("ERROR MSG: "
                                   + parseInt(new String(_msg.body), 16));
        }
        _metaTestSuccess = false;
        endTest();
        return;
      }

      _msg = new Message();
      _testStage = START_TEST;
      TestResults.appendDebugMsg("META test: START_TEST stage.");

      if (_ctlSocket.bytesAvailable > 0)
        // If TEST_PREPARE and TEST_START messages arrive together at the client
        // they trigger a single ProgressEvent.SOCKET_DATA event. In such case,
        // it's necessary to explicitly call the following function to  move to
        // the next step.
        startTest();
    }

    private function startTest():void {
      if (!_msg.readHeader(_ctlSocket))
        return;
      // TEST_START message has no body.

      if (_msg.type != MessageType.TEST_START) {
        TestResults.appendErrMsg(
            ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "metaWrongMessage", null,
                Main.locale));
        if (_msg.type == MessageType.MSG_ERROR) {
          TestResults.appendErrMsg(
              "ERROR MSG: " + parseInt(new String(_msg.body), 16));
        }
        _metaTestSuccess = false;
        endTest();
        return;
      }

      _testStage = SEND_DATA;
      TestResults.appendDebugMsg("META test: SEND_DATA stage.");
      sendData();
    }

    private function sendData():void {
      var bodyToSend:ByteArray = new ByteArray();

      bodyToSend.writeUTFBytes(new String(
          NDTConstants.META_CLIENT_OS + ":" + Capabilities.os));
      var _msg:Message = new Message(MessageType.TEST_MSG, bodyToSend);
      if (!_msg.sendMessage(_ctlSocket)) {
        _metaTestSuccess = false;
        endTest();
        return;
      }

      bodyToSend.clear();
      bodyToSend.writeUTFBytes(new String(
          NDTConstants.META_CLIENT_BROWSER + ":" + UserAgentTools.getBrowser(
              TestResults.ndt_test_results::userAgent)[2]));
      _msg = new Message(MessageType.TEST_MSG, bodyToSend);
      if (!_msg.sendMessage(_ctlSocket)) {
        _metaTestSuccess = false;
        endTest();
        return;
      }

      bodyToSend.clear();
      bodyToSend.writeUTFBytes(new String(
          NDTConstants.META_CLIENT_VERSION + ":"
          + NDTConstants.CLIENT_VERSION));
      _msg = new Message(MessageType.TEST_MSG, bodyToSend);
      if (!_msg.sendMessage(_ctlSocket)) {
        _metaTestSuccess = false;
        endTest();
        return;
      }

      bodyToSend.clear();
      bodyToSend.writeUTFBytes(new String(
          NDTConstants.META_CLIENT_APPLICATION + ":" + NDTConstants.CLIENT_ID));

      // Client can send any number of such meta data in a TEST_MSG format and
      // signal the send of the transmission using an empty TEST_MSG.
      _msg = new Message(MessageType.TEST_MSG, new ByteArray());
      if (!_msg.sendMessage(_ctlSocket)) {
        _metaTestSuccess = false;
        endTest();
        return;
      }

      _testStage = FINALIZE_TEST;
      TestResults.appendDebugMsg("META test: FINALIZE_TEST stage.");
      // The following check is probably not necessary. Added anyway, in case
      // the TEST_FINALIZE message does not trigger onReceivedData.
      if (_ctlSocket.bytesAvailable > 0)
        finalizeTest();
    }

    private function finalizeTest():void {
      if (!_msg.readHeader(_ctlSocket))
        return;
      // TEST_FINALIZE message has no body.

      if (_msg.type != MessageType.TEST_FINALIZE) {
        TestResults.appendErrMsg(
            ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME,"metaWrongMessage", null,
                Main.locale));
        if (_msg.type == MessageType.MSG_ERROR) {
          TestResults.appendErrMsg("ERROR MSG");
        }
        _metaTestSuccess = false;
        endTest();
        return;
      }
      _metaTestSuccess = true;
      endTest();
      return;
    }

    private function endTest():void {
      TestResults.appendDebugMsg("META test: END_TEST stage.");
      removeResponseListener();

      if (_metaTestSuccess)
        TestResults.appendDebugMsg(
             ResourceManager.getInstance().getString(
                 NDTConstants.BUNDLE_NAME, "meta", null, Main.locale)
            + " test <font color=\"#006400\"><b>" + ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "done", null, Main.locale) + "</b></font><br>");
      else
        TestResults.appendDebugMsg("<font color=\"#FE9A2E\">" +
	     ResourceManager.getInstance().getString(
            	NDTConstants.BUNDLE_NAME, "metaFailed", null, Main.locale) + "</font>");

      TestResults.ndt_test_results::ndtTestStatus = "done";
      NDTUtils.callExternalFunction(
          "testCompleted", "Meta", (!_metaTestSuccess).toString());

      _callerObj.runTests();
    }
  }
}

