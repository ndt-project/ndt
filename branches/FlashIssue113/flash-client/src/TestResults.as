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
  use namespace ndt_test_results;
  import flash.utils.getTimer;
  import mx.resources.ResourceManager;
  /**
   * Class used throughout the test to store test results along with debug and
   * error messages. It also interprets the results of the tests. The results
   * are stored in variables that can be accessed through JavaScript.
   */
  public class TestResults {
    private static var _ndtTestStartTime:Number = 0.0;
    private static var _ndtTestEndTime:Number = 0.0;
    private static var _resultDetails:String = "";
    private static var _errMsg:String = "";
    private static var _debugMsg:String = "";

    // Variables accessed by other classes to get and/or set values.
    ndt_test_results static var accessTech:String = null;
    ndt_test_results static var linkSpeed:Number = 0.0;
    ndt_test_results static var ndtVariables:Object = new Object();
    ndt_test_results static var userAgent:String;
    // Valid only when ndtTestFailed == false.
    ndt_test_results static var ndtTestStatus:String = null;
    ndt_test_results static var ndtTestFailed:Boolean = false;
    ndt_test_results static var c2sSpeed:Number = 0.0;
    ndt_test_results static var s2cSpeed:Number = 0.0;
    ndt_test_results static var sc2sSpeed:Number = 0.0;
    ndt_test_results static var ss2cSpeed:Number = 0.0;
    ndt_test_results static var c2sTestSuccess:Boolean;
    ndt_test_results static var s2cTestSuccess:Boolean;
    ndt_test_results static var s2cTestResults:String;
    // Results sent by the server.
    ndt_test_results static var remoteTestResults:String = "";
    ndt_test_results static var testsConfirmedByServer:int;

    public static function get jitter():Number {
      return ndtVariables[NDTConstants.MAXRTT] -
             ndtVariables[NDTConstants.MINRTT];
    }

    public static function get duration():Number {
      return _ndtTestEndTime - _ndtTestStartTime;
    }

    public static function get testList():String {
       var testSuite:String = "";
       if(testsConfirmedByServer & TestType.C2S)
          testSuite += "CLIENT_TO_SERVER_THROUGHPUT\n";
       if(testsConfirmedByServer & TestType.S2C)
          testSuite += "SERVER_TO_CLIENT_THROUGHPUT\n";
       if(testsConfirmedByServer & TestType.META)
          testSuite += "META_TEST\n";
      return testSuite;
    }

    public static function recordStartTime():void {
      _ndtTestStartTime = getTimer();
    }

    public static function recordEndTime():void {
      _ndtTestEndTime = getTimer();
    }

    public static function appendResultDetails(results:String):void {
      _resultDetails += results  + "\n";
    }

    public static function appendErrMsg(msg:String):void {
      _errMsg += msg + "\n";
      NDTUtils.callExternalFunction("appendErrors", msg);
      appendDebugMsg(msg);
    }

    public static function appendDebugMsg(msg:String):void {
      if (!CONFIG::debug) {
          return;
      }
      var formattedMsg:String = (new Date().toUTCString()) + ": " + msg + "\n";
      _debugMsg += formattedMsg;
      NDTUtils.callExternalFunction("appendDebugOutput", msg);
      // _ndtTestStartTime > 0 ensures the console window has been created.
      // TODO(tiziana): Verify if there is cleaner alternative.
      if (Main.guiEnabled && _ndtTestStartTime > 0)
        // TODO(tiziana): Handle the communication with GUI via events, instead
        // of blocking calls.
        Main.gui.addConsoleOutput(formattedMsg);
    }

    public static function getDebugMsg():String {
      return _debugMsg;
    }

    public static function getResultDetails():String {
      return _resultDetails;
    }

    public static function getErrMsg():String {
      if (_errMsg == "")
        return "No errors!";
      else
        return _errMsg;
    }

    public static function interpretResults():void {
      TestResultsUtils.parseNDTVariables(s2cTestResults + remoteTestResults);
      TestResultsUtils.appendClientInfo();
      if (ndtVariables[NDTConstants.COUNTRTT] > 0) {
        TestResultsUtils.getAccessLinkSpeed();
        TestResultsUtils.appendDuplexMismatchResults();
        if ((testsConfirmedByServer & TestType.C2S) == TestType.C2S)
          TestResultsUtils.appendC2SPacketQueueingResults();
        if ((testsConfirmedByServer & TestType.S2C) == TestType.S2C)
          TestResultsUtils.appendC2SPacketQueueingResults();
        TestResultsUtils.appendBottleneckResults();
        // TODO(tiziana): Verify overlap with getAccessLinkSpeed.
        TestResultsUtils.appendDataRateResults();
        // TODO(tiziana): Verify overlap with appendDuplexMismatchResult.
        TestResultsUtils.appendDuplexCongestionMismatchResults();
        TestResultsUtils.appendAvgRTTAndPAcketSizeResults();
        TestResultsUtils.appendPacketRetrasmissionResults();
        // TODO(tiziana): Verify overlap with appendC2SPacketQueueingResult and
        // appendC2SPacketQueueingResult.
        TestResultsUtils.appendPacketQueueingResults(testsConfirmedByServer);
        TestResultsUtils.appendTCPNegotiatedOptions();
        TestResultsUtils.appendThroughputLimitResults();
        NDTUtils.callExternalFunction("resultsProcessed");
      }
      TestResults.appendResultDetails("=== Results sent by the server ===");
      TestResults.appendResultDetails(s2cTestResults + remoteTestResults);
      // TODO(tiziana): If parsing mistake, log message "resultsParseError".
    }
	
	public static function clearResults():void {
		_ndtTestStartTime = 0.0;
		_ndtTestEndTime = 0.0;
		_resultDetails = "";
		_errMsg = "";
		_debugMsg = "";
		
		accessTech = null;
		linkSpeed = 0.0;
		ndtVariables = new Object();
		ndtTestStatus = null;
		ndtTestFailed = false;
		c2sSpeed = 0.0;
		s2cSpeed = 0.0;
		sc2sSpeed = 0.0;
		ss2cSpeed = 0.0;
		s2cTestResults = "";
		remoteTestResults = "";
	}
  }
}
