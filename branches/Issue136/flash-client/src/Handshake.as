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
  import flash.events.ProgressEvent;
  import flash.net.Socket;
  import flash.utils.ByteArray;
  import mx.resources.ResourceManager;
  import mx.utils.StringUtil;

  /**
   * This class handles the initial communication with the server before
   * starting the measurement tests.
   */
  public class Handshake {
    // Valid values for _testStage.
    private const KICK_CLIENTS:int    = 0;
    private const SRV_QUEUE1:int      = 1;
    private const SRV_QUEUE2:int      = 2;
    private const VERIFY_VERSION1:int = 3;
    private const VERIFY_VERSION2:int = 4;
    private const VERIFY_SUITE1:int   = 5;
    private const VERIFY_SUITE2:int   = 6;

    private var _callerObj:NDTPController;
    private var _ctlSocket:Socket;
    private var _msg:Message;
    private var _testStage:int;
    private var _testsRequestByClient:int;
    // Has the client already received a wait message?
    private var _isNotFirstWaitFlag:Boolean;
    private static var _loginRetried:Boolean = false;

    public function Handshake(ctlSocket:Socket, testsRequestByClient:int,
                              callerObject:NDTPController) {
      _callerObj = callerObject;
      _ctlSocket = ctlSocket;
      _testsRequestByClient = testsRequestByClient;

      _isNotFirstWaitFlag = false;  // No wait messages received yet.
    }

    public function run():void {
      var msgBody:ByteArray = new ByteArray();
      var clientVersion:String = NDTConstants.CLIENT_VERSION;

      msgBody.writeByte(_testsRequestByClient);
      if (_loginRetried) {
        _msg = new Message(MessageType.MSG_LOGIN, msgBody, false);
      } else {
        msgBody.writeMultiByte(clientVersion, "us-ascii");
        _msg = new Message(MessageType.MSG_EXTENDED_LOGIN, msgBody,
                           Main.jsonSupport);
      }

      if (!_msg.sendMessage(_ctlSocket)) {
        failHandshake();
      }

      addOnReceivedDataListener();
      _msg = new Message();
      _testStage = KICK_CLIENTS;
      TestResults.appendDebugMsg("Handshake: KICK_CLIENTS stage.");
      if (_ctlSocket.bytesAvailable > 0)
        // In case data arrived before starting the onReceiveData listener.
        kickOldClients();
    }

    private function addOnReceivedDataListener():void {
      _ctlSocket.addEventListener(ProgressEvent.SOCKET_DATA, onReceivedData);
    }

    private function removeOnReceivedDataListener():void {
      _ctlSocket.removeEventListener(ProgressEvent.SOCKET_DATA, onReceivedData);
    }

    private function onReceivedData(e:ProgressEvent):void {
      switch (_testStage) {
        case KICK_CLIENTS:    kickOldClients();
                              break;
        case SRV_QUEUE1:      srvQueue1();
                              break;
        case SRV_QUEUE2:      srvQueue2();
                              break;
        case VERIFY_VERSION1: verifyVersion1();
                              break;
        case VERIFY_VERSION2: verifyVersion2();
                              break;
        case VERIFY_SUITE1:   verifySuite1();
                              break;
        case VERIFY_SUITE2:   verifySuite2();
                              break;
      }
    }

    private function kickOldClients():void {
      _msg = new Message();
      if (!_msg.readBody(_ctlSocket, NDTConstants.KICK_OLD_CLIENTS_MSG_LENGTH))
         return;

      _msg = new Message();
      _testStage = SRV_QUEUE1;
      TestResults.appendDebugMsg("Handshake: SRV_QUEUE stage.");
      if (_ctlSocket.bytesAvailable > 0) {
        // If KICK_CLIENTS and SRV_QUEUE messages arrive together at the client,
        // they trigger a single ProgressEvent.SOCKET_DATA event. In such case,
        // it's necessary to explicitly call the following function to move to
        // the next step.
        srvQueue1();
      }
    }

    private function srvQueue1():void {
      if (!_msg.readHeader(_ctlSocket))
        return;

      _testStage = SRV_QUEUE2;
      if (_ctlSocket.bytesAvailable > 0)
        // In case header and body have arrive together at the client, they
        // trigger a single ProgressEvent.SOCKET_DATA event. In such case,
        // it's necessary to explicitly call the following function to move to
        // the next step.
        srvQueue2();
    }

    private function srvQueue2():void {
      if (!_msg.readBody(_ctlSocket, _msg.length))
        return;

      if (_msg.type != MessageType.SRV_QUEUE) {
        if (!_loginRetried) {
          removeOnReceivedDataListener();
          TestResults.appendDebugMsg(ResourceManager.getInstance().getString(
                                     NDTConstants.BUNDLE_NAME,
                                     "loggingUsingExtendedMsgFailed", null,
                                     Main.locale));
          // try to login using old MSG_LOGIN message
          _testStage = KICK_CLIENTS;
          _loginRetried = true;
          Main.jsonSupport = false;
          _callerObj.startNDTTest();
        } else {
          TestResults.appendErrMsg(ResourceManager.getInstance().getString(
              NDTConstants.BUNDLE_NAME, "loggingWrongMessage", null,
              Main.locale));
          failHandshake();
		}
      }

      var waitFlagString:String = Main.jsonSupport ?
                                  new String(NDTUtils.readJsonMapValue(
                                    String(_msg.body),
                                  NDTUtils.JSON_DEFAULT_KEY))
                                : new String(_msg.body);
      TestResults.appendDebugMsg("Received wait flag = " + waitFlagString);
      var waitFlag:int = parseInt(waitFlagString);

      // Handle different queued-client cases.
      // See https://code.google.com/p/ndt/issues/detail?id=101.
      switch(waitFlag) {
        case NDTConstants.SRV_QUEUE_TEST_STARTS_NOW:
          // No more waiting. Proceed.
          TestResults.appendDebugMsg("Finished waiting.");
          _msg = new Message();
          _testStage = VERIFY_VERSION1;
          TestResults.appendDebugMsg("Handshake: VERIFY_VERSION stage.");

          if(_ctlSocket.bytesAvailable > 0)
            // If SRV_QUEUE and VERIFY_VERSION messages arrive together at the
            // client, they trigger a single ProgressEvent.SOCKET_DATA event. In
            // such case, it's necessary to explicitly call the following
            // function.
            verifyVersion1();
          return;

        case NDTConstants.SRV_QUEUE_SERVER_FAULT:
          // Server fault. Fail.
          // TODO(tiziana): Change when issue #102 is fixed.
          // See https://code.google.com/p/ndt/issues/detail?id=102.
          TestResults.appendErrMsg(ResourceManager.getInstance().getString(
              NDTConstants.BUNDLE_NAME, "serverFault", null, Main.locale));
          failHandshake();
          return;

        case NDTConstants.SRV_QUEUE_SERVER_BUSY:
          if (!_isNotFirstWaitFlag) {
            // Server busy. Fail.
            // TODO(tiziana): Change when issue #102 is fixed.
            // See https://code.google.com/p/ndt/issues/detail?id=102.
            TestResults.appendErrMsg(ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "serverBusy",null, Main.locale));
            failHandshake();
          } else {
            // Server fault. Fail.
            // TODO(tiziana): Change when issue #102 is fixed.
            // See https://code.google.com/p/ndt/issues/detail?id=102.
            TestResults.appendErrMsg(ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "serverFault", null, Main.locale));
            failHandshake();
          }
          return;

        case NDTConstants.SRV_QUEUE_SERVER_BUSY_60s:
          // Server busy for 60s. Fail.
          // TODO(tiziana): Change when issue #102 is fixed.
          // See https://code.google.com/p/ndt/issues/detail?id=102.
          TestResults.appendErrMsg(ResourceManager.getInstance().getString(
              NDTConstants.BUNDLE_NAME, "serverBusy60s", null, Main.locale));
          failHandshake();
          return;

        case NDTConstants.SRV_QUEUE_HEARTBEAT:
          // Server sends signal to see if client is still alive.
          // Client should respond with a MSG_WAITING message.
          var _msgBody:ByteArray = new ByteArray();
          _msgBody.writeByte(_testsRequestByClient);
          _msg = new Message(MessageType.MSG_WAITING, _msgBody,
                             Main.jsonSupport);
          if (!_msg.sendMessage(_ctlSocket)) {
            failHandshake();
          }

        default:
          // Server sends the number of queued clients (== number of minutes
          // to wait before starting tests).
          // See https://code.google.com/p/ndt/issues/detail?id=103.
          TestResults.appendDebugMsg(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "otherClient", null, Main.locale)
              + (waitFlag * 60)
              + ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "seconds", null, Main.locale));
          _isNotFirstWaitFlag = false;  // First message from server received.
      }
    }

    private function verifyVersion1():void {
      if (!_msg.readHeader(_ctlSocket))
        return;

      _testStage = VERIFY_VERSION2;
      if (_ctlSocket.bytesAvailable > 0)
        // In case header and body have arrive together at the client, they
        // trigger a single ProgressEvent.SOCKET_DATA event. In such case,
        // it's necessary to explicitly call the following function to move to
        // the next step.
        verifyVersion2();
    }

    private function verifyVersion2():void {
      if (!_msg.readBody(_ctlSocket, _msg.length))
        return;

      if (_msg.type != MessageType.MSG_LOGIN) {
        TestResults.appendErrMsg(ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "versionWrongMessage", null,
            Main.locale));
        failHandshake();
        return;
      }

      var receivedServerVersion:String = Main.jsonSupport ?
                                         new String(NDTUtils.readJsonMapValue(
                                           String(_msg.body),
                                           NDTUtils.JSON_DEFAULT_KEY))
                                       : new String(_msg.body);
      TestResults.appendDebugMsg("Server version: " + receivedServerVersion);
      // See https://code.google.com/p/ndt/issues/detail?id=104.
      if (receivedServerVersion != NDTConstants.EXPECTED_SERVER_VERSION)
        TestResults.appendDebugMsg(
            "The server version sent by the server is: "
            + receivedServerVersion
            + ", while the client expects: "
            + NDTConstants.EXPECTED_SERVER_VERSION);
      if (receivedServerVersion < NDTConstants.LAST_VALID_SERVER_VERSION) {
        TestResults.appendErrMsg(
            ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "incompatibleVersion",null,
                Main.locale));
        failHandshake();
        return;
      }

      _msg = new Message();
      _testStage = VERIFY_SUITE1;
      TestResults.appendDebugMsg("Handshake: VERIFY_SUITE stage.");

      if (_ctlSocket.bytesAvailable > 0) {
        // If VERIFY_VERSION and VERIFY_SUITE messages arrive together at the
        // client, they trigger a single ProgressEvent.SOCKET_DATA event. In
        // such case, it's necessary to explicitly call the following function
        // to  move to the next step.
        verifySuite1();
      }
    }

    private function verifySuite1():void {
      if (!_msg.readHeader(_ctlSocket))
        return;

      _testStage = VERIFY_SUITE2;
      if (_ctlSocket.bytesAvailable > 0)
        // In case header and body have arrive together at the client, they
        // trigger a single ProgressEvent.SOCKET_DATA event. In such case,
        // it's necessary to explicitly call the following function to move to
        // the next step.
        verifySuite2();
    }

    private function verifySuite2():void {
      if (!_msg.readBody(_ctlSocket, _msg.length))
        return;

      if (_msg.type != MessageType.MSG_LOGIN) {
        TestResults.appendErrMsg(ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "testsuiteWrongMessage", null,
            Main.locale));
        return;
      }

      var confirmedTests:String = Main.jsonSupport ?
                                  new String(NDTUtils.readJsonMapValue(
                                    String(_msg.body),
                                  NDTUtils.JSON_DEFAULT_KEY))
                                : new String(_msg.body);
      TestResults.ndt_test_results::testsConfirmedByServer =
          TestType.listToBitwiseOR(confirmedTests);

      TestResults.appendDebugMsg("Test suite: " + confirmedTests);
      endHandshake(StringUtil.trim(confirmedTests));
    }

    private function failHandshake():void {
      TestResults.appendDebugMsg("Handshake: FAIL.<br>");

      removeOnReceivedDataListener();
      _callerObj.failNDTTest();
    }

    private function endHandshake(confirmedTests:String):void {
      TestResults.appendDebugMsg("Handshake: END.<br>");

      removeOnReceivedDataListener();
      _callerObj.initiateTests(confirmedTests);
    }
  }
}

