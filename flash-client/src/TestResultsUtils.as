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
  import flash.system.Capabilities;
  import mx.resources.ResourceManager;
  import mx.utils.StringUtil;

  public class TestResultsUtils {
    /**
     *  Return text representation of data speed values.
     */
    public static function getDataRateString(dataRateInt:int):String {
      switch (dataRateInt) {
      case NDTConstants.DATA_RATE_SYSTEM_FAULT:
        return ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "systemFault", null, Main.locale);
      case NDTConstants.DATA_RATE_RTT:
        return ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "rtt", null, Main.locale);
      case NDTConstants.DATA_RATE_DIAL_UP:
        return ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "dialup2", null, Main.locale);
      case NDTConstants.DATA_RATE_T1:
        return ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "t1Str", null, Main.locale);
      case NDTConstants.DATA_RATE_ETHERNET:
        return ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "ethernetStr", null, Main.locale);
      case NDTConstants.DATA_RATE_T3:
        return ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "t3Str", null, Main.locale);
      case NDTConstants.DATA_RATE_FAST_ETHERNET:
        return ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "fastEthernet", null, Main.locale);
      case NDTConstants.DATA_RATE_OC_12:
        return ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "oc12Str", null, Main.locale);
      case NDTConstants.DATA_RATE_GIGABIT_ETHERNET:
        return ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "gigabitEthernetStr", null, Main.locale);
      case NDTConstants.DATA_RATE_OC_48:
        return ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "oc48Str", null, Main.locale);
      case NDTConstants.DATA_RATE_10G_ETHERNET:
        return ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "tengigabitEthernetStr", null,
            Main.locale);
      }
      return null;
    }

    /**
     * Function that return a variable corresponding to the parameter passed to
     * it as a request.
     * @param {String} The parameter which the caller is seeking.
     * @return {String} The value of the desired parameter.
     */
    public static function getNDTVariable(varName:String):String {
      switch(varName) {
        case "TestList":
          return TestResults.testList;
        case "TestDuration":
          return TestResults.duration.toString();
        case "ClientToServerSpeed":
          return (TestResults.ndt_test_results::c2sSpeed
                  / NDTConstants.KBITS2BITS).toString();
        case "ServerToClientSpeed":
          return (TestResults.ndt_test_results::s2cSpeed
                  / NDTConstants.KBITS2BITS).toString();
        case "Jitter":
          return TestResults.jitter.toString();
        case "OperatingSystem":
          return Capabilities.os;
        case "ClientVersion":
          return NDTConstants.CLIENT_VERSION;
        case "PluginVersion":
          return Capabilities.version;
        case "OsArchitecture":
          return Capabilities.cpuArchitecture;
        case NDTConstants.MISMATCH:
          if (TestResults.ndtVariables[varName]
              == NDTConstants.DUPLEX_OK_INDICATOR)
            return "no";
          else
            return "yes";
        case NDTConstants.BAD_CABLE:
          if (TestResults.ndtVariables[varName]
              == NDTConstants.CABLE_STATUS_OK)
            return "no";
          else
            return "yes";
        case NDTConstants.CONGESTION:
          if (TestResults.ndtVariables[varName]
              == NDTConstants.CONGESTION_NONE)
            return "no";
          else
            return "yes";
        case NDTConstants.RWINTIME:
          return String(TestResults.ndtVariables[varName] * NDTConstants.PERCENTAGE);
        case NDTConstants.OPTRCVRBUFF:
          return String(TestResults.ndtVariables[NDTConstants.MAXRWINRCVD] * NDTConstants.KBITS2BITS);
        case NDTConstants.ACCESS_TECH:
         return TestResults.ndt_test_results::accessTech;
        default:
          if (varName in TestResults.ndtVariables) {
            return TestResults.ndtVariables[varName].toString();
          }
      }

      return null;
    }

    public static function parseNDTVariables(testResults:String):void {
      // Extract the key-value pairs.
      var pairs:Array = StringUtil.trim(testResults).split(/\s/);
      var i:int;
      var varName:String;
      var varValue:String;
      var intValue:int;
      var floatValue:Number;
      for (i = 0; i < pairs.length; i = i + 2) {
        // Strips ":" from the web100 variable name.
        varName = pairs[i].split(":")[0];
        varValue = pairs[i + 1];
        // Figure out if varValue is integer or float.
        // 1) Try integer.
        intValue = parseInt(varValue);
        if (varValue == String(intValue)) {
          // Yes, it's a valid integer.
          TestResults.ndtVariables[varName] = intValue;
          continue;
        }
        // 2) Try float.
        floatValue = parseFloat(varValue);
        if (!isNaN(floatValue)) {
          // Yes, it's a valid float.
          TestResults.ndtVariables[varName] = floatValue;
          continue;
        }
        // 3) Neither a valid integer nor a valid float.
        TestResults.appendErrMsg("Error parsing web100 var. varName: "
                                 + varName + "; varValue: " + varValue);
      }
    }

    public static function appendClientInfo():void {
      TestResults.appendResultDetails(ResourceManager.getInstance().getString(
          NDTConstants.BUNDLE_NAME, "clientInfo", null, Main.locale));
      TestResults.appendResultDetails(ResourceManager.getInstance().getString(
          NDTConstants.BUNDLE_NAME, "clientVersion", null, Main.locale)
          + ": " + CONFIG::clientVersion);
      TestResults.appendResultDetails(
          ResourceManager.getInstance().getString(
              NDTConstants.BUNDLE_NAME, "osData", null, Main.locale)
          + ": " + Capabilities.os + ", "
          + ResourceManager.getInstance().getString(
              NDTConstants.BUNDLE_NAME, "architecture", null, Main.locale)
          + ": " + Capabilities.cpuArchitecture);
      TestResults.appendResultDetails(
          "Flash Info: " + ResourceManager.getInstance().getString(
              NDTConstants.BUNDLE_NAME, "version", null, Main.locale)
          + " = " + Capabilities.version);
    }

    private static function getClient(osArchitecture:String):String {
      if (osArchitecture.indexOf("x86") == 0)
        return ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "pc", null, Main.locale);
      else
        return ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "workstation", null, Main.locale);
    }

    public static function getAccessLinkSpeed():void {
      if (TestResults.ndtVariables[NDTConstants.C2SDATA]
          < NDTConstants.DATA_RATE_ETHERNET) {
        if (TestResults.ndtVariables[NDTConstants.C2SDATA]
            < NDTConstants.DATA_RATE_RTT) {
           // Data was not sufficient to determine bottleneck type.
           TestResults.appendResultDetails(
               ResourceManager.getInstance().getString(
                   NDTConstants.BUNDLE_NAME, "unableToDetectBottleneck", null,
                   Main.locale));
           TestResults.accessTech = NDTConstants.ACCESS_TECH_UNKNOWN;
        } else {
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "your", null, Main.locale)
              + " " + getClient(Capabilities.cpuArchitecture) + " "
              + ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "connectedTo", null, Main.locale));

          if (TestResults.ndtVariables[NDTConstants.C2SDATA]
              == NDTConstants.DATA_RATE_DIAL_UP) {
            TestResults.appendResultDetails(
                ResourceManager.getInstance().getString(
                    NDTConstants.BUNDLE_NAME, "dialup", null, Main.locale));
            TestResults.accessTech = NDTConstants.ACCESS_TECH_DIALUP;
          } else {
            TestResults.appendResultDetails(
                ResourceManager.getInstance().getString(
                    NDTConstants.BUNDLE_NAME, "cabledsl", null, Main.locale));
            TestResults.accessTech = NDTConstants.ACCESS_TECH_CABLEDSL;
          }
        }
      } else {
        TestResults.appendResultDetails(ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "theSlowestLink", null, Main.locale));
        switch(TestResults.ndtVariables[NDTConstants.C2SDATA]) {
          case NDTConstants.DATA_RATE_ETHERNET:
            TestResults.appendResultDetails(
                ResourceManager.getInstance().getString(
                    NDTConstants.BUNDLE_NAME, "10mbps", null, Main.locale));
            TestResults.accessTech = NDTConstants.ACCESS_TECH_10MBPS;
            break;
          case NDTConstants.DATA_RATE_T3:
            TestResults.appendResultDetails(
                ResourceManager.getInstance().getString(
                    NDTConstants.BUNDLE_NAME, "45mbps", null, Main.locale));
            TestResults.accessTech = NDTConstants.ACCESS_TECH_45MBPS;
            break;
          case NDTConstants.DATA_RATE_FAST_ETHERNET:
            TestResults.appendResultDetails(
                ResourceManager.getInstance().getString(
                    NDTConstants.BUNDLE_NAME, "100mbps", null, Main.locale));
            TestResults.accessTech = NDTConstants.ACCESS_TECH_100MBPS;
            // Determine if half/full duplex link was found.
            if (TestResults.ndtVariables[NDTConstants.HALF_DUPLEX] == 0)
              TestResults.appendResultDetails(
                  ResourceManager.getInstance().getString(
                      NDTConstants.BUNDLE_NAME, "fullDuplex", null,
                      Main.locale));
            else
              TestResults.appendResultDetails(
                  ResourceManager.getInstance().getString(
                      NDTConstants.BUNDLE_NAME, "halfDuplex", null,
                      Main.locale));
            break;
        case NDTConstants.DATA_RATE_OC_12:
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "622mbps", null, Main.locale));
           TestResults.accessTech = NDTConstants.ACCESS_TECH_622MBPS;
           break;
         case NDTConstants.DATA_RATE_GIGABIT_ETHERNET:
           TestResults.appendResultDetails(
               ResourceManager.getInstance().getString(
                   NDTConstants.BUNDLE_NAME, "1gbps", null, Main.locale));
           TestResults.accessTech = NDTConstants.ACCESS_TECH_1GBPS;
           break;
         case NDTConstants.DATA_RATE_OC_48:
           TestResults.appendResultDetails(
               ResourceManager.getInstance().getString(
                   NDTConstants.BUNDLE_NAME, "2.4gbps", null, Main.locale));
           TestResults.accessTech = NDTConstants.ACCESS_TECH_2GBPS;
           break;
         case NDTConstants.DATA_RATE_10G_ETHERNET:
           TestResults.appendResultDetails(
               ResourceManager.getInstance().getString(
                   NDTConstants.BUNDLE_NAME, "10gbps", null, Main.locale));
           TestResults.accessTech = NDTConstants.ACCESS_TECH_10GBPS;
           break;
         default:
           TestResults.appendErrMsg(
               "Non valid value for NDTConstants.C2SDATA: " +
               TestResults.ndtVariables[NDTConstants.C2SDATA]);
        }
      }
      TestResults.linkSpeed = NDTConstants.ACCESS_TECH2LINK_SPEED[
          TestResults.accessTech]
    }

    public static function appendDuplexMismatchResults():void {
      switch(TestResults.ndtVariables[NDTConstants.MISMATCH]) {
        case NDTConstants.DUPLEX_NOK_INDICATOR:
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "oldDuplexMismatch", null,
                  Main.locale));
          break;
        case NDTConstants.DUPLEX_SWITCH_FULL_HOST_HALF:
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "duplexFullHalf", null,
                  Main.locale));
          break;
        case NDTConstants.DUPLEX_SWITCH_HALF_HOST_FULL:
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "duplexHalfFull", null,
                  Main.locale));
          break;
        case NDTConstants.DUPLEX_SWITCH_FULL_HOST_HALF_POSS:
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "possibleDuplexFullHalf", null,
                  Main.locale));
          break;
        case NDTConstants.DUPLEX_SWITCH_HALF_HOST_FULL_POSS:
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "possibleDuplexHalfFull", null,
                  Main.locale));
          break;
        case NDTConstants.DUPLEX_SWITCH_HALF_HOST_FULL_WARN:
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "possibleDuplexHalfFullWarning",
                  null, Main.locale));
          break;
        case NDTConstants.DUPLEX_OK_INDICATOR:
          if (TestResults.ndtVariables[NDTConstants.BAD_CABLE]
              == NDTConstants.CABLE_STATUS_NOK)
            TestResults.appendResultDetails(
                ResourceManager.getInstance().getString(
                    NDTConstants.BUNDLE_NAME, "excessiveErrors", null,
                    Main.locale));

           if (TestResults.ndtVariables[NDTConstants.CONGESTION]
               == NDTConstants.CONGESTION_YES)
             TestResults.appendResultDetails(
                 ResourceManager.getInstance().getString(
                     NDTConstants.BUNDLE_NAME, "otherTraffic", null,
                     Main.locale));
          appendRecommendedBufferSize();
          break;
        default:
          TestResults.appendErrMsg("Non valid duplex indicator");
      }
    }

    private static function appendRecommendedBufferSize():void {
      // If we seem to be transmitting less than link speed (i.e calculated
      // bandwidth is greater than measured throughput), it is possibly due to a
      // receiver window setting. Advise appropriate size.
      // Note: All comparisons henceforth of
      // ((window size * 2/rttsec) < link speed)
      // are along the same logic.
      // Window size is multiplied by 2 to counter round-trip.
      if ((2 * TestResults.ndtVariables[NDTConstants.RWIN]
           / TestResults.ndtVariables[NDTConstants.RTTSEC])
           < TestResults.linkSpeed) {
        // Link speed is in Mbps. Convert to kilo bytes per secs.
        var linkSpeedInKBps:Number = Number(TestResults.linkSpeed
                                            * NDTConstants.KBITS2BITS
                                            / NDTConstants.BYTES2BITS)
        var theoreticalMaxBuffer:Number = (
            linkSpeedInKBps * NDTConstants.SEC2MSEC
            * TestResults.ndtVariables[NDTConstants.AVGRTT])
        if (theoreticalMaxBuffer
            > TestResults.ndtVariables[NDTConstants.MAXRWINRCVD]) {
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "receiveBufferShouldBe", null,
                  Main.locale)
              + " " + theoreticalMaxBuffer.toFixed(2)
              + ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "toMaximizeThroughput", null,
                  Main.locale));
        }
      }
    }

    public static function appendC2SPacketQueueingResults():void {
      if (TestResults.ndt_test_results::sc2sSpeed
          < (TestResults.ndt_test_results::c2sSpeed
          * (1.0 - NDTConstants.SPD_DIFF)))
        TestResults.appendResultDetails(
            ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "c2sPacketQueuingDetected", null,
                Main.locale));
    }

    public static function appendS2CPacketQueueingResults():void {
       if (TestResults.ndt_test_results::s2cSpeed
           < (TestResults.ndt_test_results::ss2cSpeed
           * (1.0 - NDTConstants.SPD_DIFF)))
         TestResults.appendResultDetails(
             ResourceManager.getInstance().getString(
                 NDTConstants.BUNDLE_NAME, "s2cPacketQueuingDetected", null,
                 Main.locale));
    }

    public static function appendBottleneckResults():void {
      // 1) Is the connection receiver limited?
      if (TestResults.ndtVariables[NDTConstants.RWINTIME]
          > NDTConstants.SND_LIM_TIME_THRESHOLD) {
        TestResults.appendResultDetails(
            ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "thisConnIs", null, Main.locale)
            + " " + ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME,  "limitRx", null, Main.locale)
            + " " + (TestResults.ndtVariables[NDTConstants.RWINTIME]
                     * NDTConstants.PERCENTAGE).toFixed(2)
            + ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "pctOfTime", null, Main.locale));
        // Multiplying by 2 to counter round-trip.
        var receiverLimit:Number = (
            2 * TestResults.ndtVariables[NDTConstants.RWIN]
            / TestResults.ndtVariables[NDTConstants.RTTSEC]);
        var idealReceiverBuffer:Number = (
             TestResults.ndtVariables[NDTConstants.MAXRWINRCVD]
             / NDTConstants.KBITS2BITS);
        if (receiverLimit < TestResults.linkSpeed)
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "incrRxBuf", null, Main.locale)
              + " (" + idealReceiverBuffer.toFixed(2) + " KB)"
              + ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "willImprove", null, Main.locale));
      }

      // 2) Is the connection sender limited?
      if (TestResults.ndtVariables[NDTConstants.SENDTIME]
          > NDTConstants.SND_LIM_TIME_THRESHOLD) {
        TestResults.appendResultDetails(
            ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "thisConnIs", null, Main.locale)
            + " " + ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "limitTx", null, Main.locale)
          + " " + (TestResults.ndtVariables[NDTConstants.SENDTIME]
                   * NDTConstants.PERCENTAGE).toFixed(2)
          + ResourceManager.getInstance().getString(
              NDTConstants.BUNDLE_NAME, "pctOfTime", null, Main.locale));

        var senderLimit:Number = (
            2 * TestResults.ndtVariables[NDTConstants.SWIN]
            / TestResults.ndtVariables[NDTConstants.RTTSEC]);
        var idealSenderBuffer:Number = (
             TestResults.ndtVariables[NDTConstants.SNDBUF]
             / (2 * NDTConstants.KBITS2BITS));

        if (senderLimit < TestResults.linkSpeed)
          // Dividing by 2 to counter round-trip.
          TestResults.appendResultDetails(
            ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "incrTxBuf", null, Main.locale)
            + " (" + idealSenderBuffer.toFixed(2) + " KB)"
            + ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "willImprove", null, Main.locale));
      }

      // 3) Is the connection network limited?
      if (TestResults.ndtVariables[NDTConstants.CWNDTIME]
          > NDTConstants.CWND_LIM_TIME_THRESHOLD) {
        TestResults.appendResultDetails(
            ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "thisConnIs", null, Main.locale)
            + " " + ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "limitNet", null, Main.locale)
            + " " + (TestResults.ndtVariables[NDTConstants.CWNDTIME]
                     * NDTConstants.PERCENTAGE).toFixed(2)
            + ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "pctOfTime", null, Main.locale));
      }

      // 4) Is the loss excessive?
      if ((TestResults.ndtVariables[NDTConstants.SPD]
           < NDTConstants.DATA_RATE_T3)
          && (TestResults.ndtVariables[NDTConstants.LOSS]
           > NDTConstants.LOSS_THRESHOLD)) {
        TestResults.appendResultDetails(
            ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "excLoss", null, Main.locale));
      }
    }

    public static function appendDataRateResults():void {
      switch(TestResults.ndtVariables[NDTConstants.C2SDATA]) {
        case NDTConstants.DATA_RATE_INSUFFICIENT_DATA:
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "insufficient", null, Main.locale));
          break;
        case NDTConstants.DATA_RATE_SYSTEM_FAULT:
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "ipcFail", null, Main.locale));
          break;
        case NDTConstants.DATA_RATE_RTT:
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "rttFail", null, Main.locale));
          break;
        case NDTConstants.DATA_RATE_DIAL_UP:
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "foundDialup", null, Main.locale));
          break;
        case NDTConstants.DATA_RATE_T1:
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "foundDsl", null, Main.locale));
          break;
        case NDTConstants.DATA_RATE_ETHERNET:
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "found10mbps", null, Main.locale));
          break;
        case NDTConstants.DATA_RATE_T3:
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "found45mbps", null, Main.locale));
          break;
        case NDTConstants.DATA_RATE_FAST_ETHERNET:
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "found100mbps", null, Main.locale));
          break;
        case NDTConstants.DATA_RATE_OC_12:
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "found622mbps", null, Main.locale));
          break;
        case NDTConstants.DATA_RATE_GIGABIT_ETHERNET:
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "found1gbps", null, Main.locale));
          break;
        case NDTConstants.DATA_RATE_OC_48:
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "found2.4gbps", null, Main.locale));
          break;
        case NDTConstants.DATA_RATE_10G_ETHERNET:
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "found10gbps", null, Main.locale));
            break;
      }
    }

    public static function appendDuplexCongestionMismatchResults():void {
      if (TestResults.ndtVariables[NDTConstants.HALF_DUPLEX]
          == NDTConstants.DUPLEX_OK_INDICATOR)
        TestResults.appendResultDetails(ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "linkFullDpx", null, Main.locale));
      else
        TestResults.appendResultDetails(ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "linkHalfDpx", null, Main.locale));

      if (TestResults.ndtVariables[NDTConstants.CONGESTION]
          == NDTConstants.CONGESTION_NONE)
        TestResults.appendResultDetails(ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "congestNo", null, Main.locale));
      else
        TestResults.appendResultDetails(ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "congestYes", null, Main.locale));

      if (TestResults.ndtVariables[NDTConstants.BAD_CABLE]
          == NDTConstants.CABLE_STATUS_OK)
        TestResults.appendResultDetails(ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "cablesOk", null, Main.locale));
      else
        TestResults.appendResultDetails(ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "cablesNok", null, Main.locale));

      switch(TestResults.ndtVariables[NDTConstants.MISMATCH]) {
        case NDTConstants.DUPLEX_OK_INDICATOR:
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "duplexOk", null, Main.locale));
          break;
        case NDTConstants.DUPLEX_NOK_INDICATOR:
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "duplexNok", null, Main.locale));
          break;
        case NDTConstants.DUPLEX_SWITCH_FULL_HOST_HALF:
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "duplexFullHalf", null,
                  Main.locale));
          break;
        case NDTConstants.DUPLEX_SWITCH_HALF_HOST_FULL:
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "duplexHalfFull", null,
                  Main.locale));
      }
    }

    public static function appendAvgRTTAndPAcketSizeResults():void {
      TestResults.appendResultDetails(ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "web100rtt", null, Main.locale)
            + " = " + TestResults.ndtVariables[NDTConstants.AVGRTT] + "ms");
      TestResults.appendResultDetails(ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "packetsize", null, Main.locale)
            + " = " + TestResults.ndtVariables[NDTConstants.CURMSS]
            + "bytes");
    }

    public static function appendPacketRetrasmissionResults():void {
      if (TestResults.ndtVariables[NDTConstants.PKTSRETRANS] > 0) {
        // Packet retransmissions found.
        TestResults.appendResultDetails(
            TestResults.ndtVariables[NDTConstants.PKTSRETRANS] + " "
            + ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "pktsRetrans", null, Main.locale));
        TestResults.appendResultDetails(
            TestResults.ndtVariables[NDTConstants.DUPACKSIN] + " "
            + ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "dupAcksIn", null, Main.locale));
        TestResults.appendResultDetails(
            TestResults.ndtVariables[NDTConstants.SACKSRCVD] + " "
            + ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "sackReceived", null, Main.locale));

        if (TestResults.ndtVariables[NDTConstants.TIMEOUTS] > 0)
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "connStalled", null, Main.locale)
              + " " + TestResults.ndtVariables[NDTConstants.TIMEOUTS] + " "
              + ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "timesPktLoss", null, Main.locale));

        var percIdleTime:Number = (
            TestResults.ndtVariables[NDTConstants.WAITSEC]
            / TestResults.ndtVariables[NDTConstants.TIMESEC])
            * NDTConstants.PERCENTAGE
        TestResults.appendResultDetails(
            ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "connIdle", null, Main.locale)
            + " " + TestResults.ndtVariables[NDTConstants.WAITSEC].toFixed(2)
            + " " + ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "seconds", null, Main.locale)
            + " (" + percIdleTime.toFixed(2) +
            ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "pctOfTime", null, Main.locale)
            + ")");
      } else if (TestResults.ndtVariables[NDTConstants.DUPACKSIN] > 0) {
        // No packet loss, but packets arrived out-of-order.
        var percOrder:Number = (TestResults.ndtVariables[NDTConstants.ORDER]
                                * NDTConstants.PERCENTAGE);
        TestResults.appendResultDetails(
            ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "noPktLoss1", null, Main.locale)
            + " - " + ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "ooOrder", null, Main.locale)
            + " " + percOrder.toFixed(2) +
            ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "pctOfTime", null, Main.locale));
      } else {
        // No packet retransmissions found.
        TestResults.appendResultDetails(ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "noPktLoss2", null, Main.locale) + ".");
      }
    }

    public static function appendPacketQueueingResults(requestedTests:int):void {
      // Add Packet queueing details found during C2S throughput test.
      // Data is displayed as percentage.
      if ((requestedTests & TestType.C2S) == TestType.C2S) {
        // TODO(tiziana): Change when issue #98 is fixed.
        // https://code.google.com/p/ndt/issues/detail?id=98
        if (TestResults.ndt_test_results::c2sSpeed
            > TestResults.ndt_test_results::sc2sSpeed) {
          var c2sQueue:Number = (TestResults.ndt_test_results::c2sSpeed
                                 - TestResults.ndt_test_results::sc2sSpeed)
                                 / TestResults.ndt_test_results::c2sSpeed;
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "c2s", null, Main.locale)
              + " "  +
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "qSeen", null, Main.locale)
              + ": " + (NDTConstants.PERCENTAGE * c2sQueue).toFixed(2) + "%");
        }
      }

      // Add packet queueing details found during S2C throughput test.
      // Data is displayed as a percentage.
      if ((requestedTests & TestType.S2C) == TestType.S2C) {
        // TODO(tiziana): Change when issue #98 is fixed.
        // https://code.google.com/p/ndt/issues/detail?id=98
        if (TestResults.ndt_test_results::ss2cSpeed
            > TestResults.ndt_test_results::s2cSpeed) {
          var s2cQueue:Number = (TestResults.ndt_test_results::s2cSpeed
                                 - TestResults.ndt_test_results::ss2cSpeed)
                                 / TestResults.ndt_test_results::s2cSpeed;
          TestResults.appendResultDetails(
              ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "s2c", null, Main.locale)
              + " " + ResourceManager.getInstance().getString(
                  NDTConstants.BUNDLE_NAME, "qSeen", null, Main.locale)
              + ": " + (NDTConstants.PERCENTAGE * s2cQueue).toFixed(2) + "%");
        }
      }
    }

    public static function appendTCPNegotiatedOptions():void {
      TestResults.appendResultDetails(ResourceManager.getInstance().getString(
          NDTConstants.BUNDLE_NAME, "web100tcpOpts", null, Main.locale));

      TestResults.appendResultDetails("RFC 2018 Selective Acknowledgement:");
      if (TestResults.ndtVariables[NDTConstants.SACKENABLED]
          == NDTConstants.SACKENABLED_OFF)
        TestResults.appendResultDetails(ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "off", null, Main.locale));
      else
        TestResults.appendResultDetails(ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "on", null, Main.locale));

      TestResults.appendResultDetails("RFC 896 Nagle Algorithm:");
      if (TestResults.ndtVariables[NDTConstants.NAGLEENABLED]
          == NDTConstants.NAGLEENABLED_OFF)
        TestResults.appendResultDetails(ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "off", null, Main.locale));
      else
        TestResults.appendResultDetails(ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "on", null, Main.locale));

      TestResults.appendResultDetails(
          "RFC 3168 Explicit Congestion Notification:");
      if (TestResults.ndtVariables[NDTConstants.ECNENABLED]
          == NDTConstants.ECNENABLED_OFF)
        TestResults.appendResultDetails(ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "off", null, Main.locale));
      else
        TestResults.appendResultDetails(ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "on", null, Main.locale));

      TestResults.appendResultDetails("RFC 1323 Time Stamping:");
      if (TestResults.ndtVariables[NDTConstants.TIMESTAMPSENABLED]
          == NDTConstants.TIMESTAMPSENABLED_OFF)
        TestResults.appendResultDetails(ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "off", null, Main.locale));
      else
        TestResults.appendResultDetails(ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "on", null, Main.locale));

      TestResults.appendResultDetails("RFC 1323 Window Scaling:");
      if ((TestResults.ndtVariables[NDTConstants.MAXRWINRCVD]
           < NDTConstants.TCP_MAX_RECV_WIN_SIZE)
          || (TestResults.ndtVariables[NDTConstants.WINSCALERCVD]
           > NDTConstants.TCP_MAX_WINSCALERCVD)) {
        // Max rcvd window size lesser than TCP's max value, so no scaling
        // requested.
        TestResults.appendResultDetails(ResourceManager.getInstance().getString(
            NDTConstants.BUNDLE_NAME, "off", null, Main.locale));
      } else
        TestResults.appendResultDetails(
            ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "on", null, Main.locale) + "; "
            + ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "scalingFactors", null, Main.locale)
            + " - " + ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "server", null, Main.locale) + "="
            + TestResults.ndtVariables[NDTConstants.WINSCALERCVD] + ", "
            + ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "client", null, Main.locale) + "="
            + TestResults.ndtVariables[NDTConstants.WINSCALESENT]);
    }

    public static function appendThroughputLimitResults():void {
      // Adding more details related to factors influencing throughput.

      // 1) Theoretical network limit.
      TestResults.appendResultDetails(
          ResourceManager.getInstance().getString(
              NDTConstants.BUNDLE_NAME, "theoreticalLimit", null, Main.locale)
          + " " + TestResults.ndtVariables[NDTConstants.BW].toFixed(2) + " "
          + "Mbps");

      // 2) NDT server buffer imposed limit.
      // Divide by 2 to counter round-trip.
      var ndtServerBuffer:Number = (
          TestResults.ndtVariables[NDTConstants.SNDBUF]
          / (2 * NDTConstants.KBITS2BITS))
      var ndtServerLimit:Number = (
          TestResults.ndtVariables[NDTConstants.SWIN]
          / TestResults.ndtVariables[NDTConstants.RTTSEC])
      TestResults.appendResultDetails(
          ResourceManager.getInstance().getString(
              NDTConstants.BUNDLE_NAME, "ndtServerHas", null, Main.locale)
          + " " + ndtServerBuffer.toFixed(2) + " "
          + ResourceManager.getInstance().getString(
              NDTConstants.BUNDLE_NAME, "kbyteBufferLimits", null, Main.locale)
          + " " + ndtServerLimit.toFixed(2) + " Mbps");

      // PC buffer imposed throughput limit.
      var pcReceiverBuffer:Number = (
          TestResults.ndtVariables[NDTConstants.MAXRWINRCVD]
          / NDTConstants.KBITS2BITS)
      var pcLimit:Number = (TestResults.ndtVariables[NDTConstants.RWIN]
                            / TestResults.ndtVariables[NDTConstants.RTTSEC]);
      TestResults.appendResultDetails(
          ResourceManager.getInstance().getString(
              NDTConstants.BUNDLE_NAME, "yourPcHas", null, Main.locale)
          + " " + pcReceiverBuffer.toFixed(2) + " "
          + ResourceManager.getInstance().getString(
              NDTConstants.BUNDLE_NAME, "kbyteBufferLimits", null, Main.locale)
          + " " + pcLimit.toFixed(2) + " Mbps");

      // Network based flow control limit imposed throughput limit.
      var networkLimit:Number = (
          TestResults.ndtVariables[NDTConstants.CWIN]
          / TestResults.ndtVariables[NDTConstants.RTTSEC])
      TestResults.appendResultDetails(
          ResourceManager.getInstance().getString(
              NDTConstants.BUNDLE_NAME, "flowControlLimits", null, Main.locale)
          + " " + networkLimit.toFixed(2) + " Mbps");

      // Client and server data reports on link capacity.
      var c2sData:String = TestResultsUtils.getDataRateString(
          TestResults.ndtVariables[NDTConstants.C2SDATA]);
      var c2sAck:String = TestResultsUtils.getDataRateString(
          TestResults.ndtVariables[NDTConstants.C2SACK]);
      var s2cData:String = TestResultsUtils.getDataRateString(
          TestResults.ndtVariables[NDTConstants.S2CDATA]);
      var s2cAck:String = TestResultsUtils.getDataRateString(
          TestResults.ndtVariables[NDTConstants.S2CACK]);

      if (c2sData != null)
        TestResults.appendResultDetails(
            ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "clientDataReports", null, Main.locale)
            + " " + c2sData);
      if (c2sAck != null)
        TestResults.appendResultDetails(
            ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "clientAcksReport", null, Main.locale)
            + " " + c2sAck);
      if (s2cData != null)
        TestResults.appendResultDetails(
            ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "serverDataReports", null, Main.locale)
            + " " + s2cData);
      if (s2cAck != null)
        TestResults.appendResultDetails(
            ResourceManager.getInstance().getString(
                NDTConstants.BUNDLE_NAME, "serverAcksReport", null, Main.locale)
            + " " + s2cAck);
    }
  }
}

