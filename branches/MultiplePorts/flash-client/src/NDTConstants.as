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
  public class NDTConstants {
    public static const MLAB_SITE:String = "http://www.measurementlab.net";

    public static const CLIENT_VERSION:String = CONFIG::clientVersion;
    public static const EXPECTED_SERVER_VERSION:String = CONFIG::serverVersion;
    // The current version of the protocol is backward compatible to v3.3.12.
    public static const LAST_VALID_SERVER_VERSION:String = "v3.3.12";
    public static const CLIENT_ID:String = "swf";
    public static const LIMITED_CLIENT_ID:String = "swf-ltd";
    public static const BAD_ENV_ACTION:String = "error";
    // Client MUST request the TEST_STATUS.
    public static const TESTS_REQUESTED_BY_CLIENT:int =
        TestType.C2S | TestType.S2C | TestType.META | TestType.STATUS;
    // TODO(tiziana): Get hostname from mlab-ns.
    public static const SERVER_HOSTNAME:String =
        "ndt.iupui.mlab1.nuq0t.measurement-lab.org";
    // For localization.
    public static const BUNDLE_NAME:String = "DisplayMessages";
    public static const NDT_DESCRIPTION:String =
        "Network Diagnostic Tool (NDT) provides a "
        + "sophisticated speed and diagnostic test. An NDT "
        + "test reports more than just the upload and "
        + "download speeds — it also attempts to determine "
        + "what, if any, problems limited these speeds, "
        + "differentiating between computer configuration "
        + "and network infrastructure problems. While the "
        + "diagnostic messages are most useful for expert "
        + "users, they can also help novice users by "
        + "allowing them to provide detailed trouble "
        + "reports to their network administrator.";
    public static const PROTOCOL_MSG_READ_SUCCESS:int = 0;
    public static const PROTOCOL_MSG_READ_ERROR:int = 1;
    public static const MSG_HEADER_LENGTH:int = 3;
    public static const KICK_OLD_CLIENTS_MSG_LENGTH:int = 13;
    public static const SRV_QUEUE_MSG_LENGTH:int = 4;
    public static const DEFAULT_CONTROL_PORT:uint = 3001;

    // SRV-QUEUE status.
    public static const SRV_QUEUE_HEARTBEAT:int = 9990;
    public static const SRV_QUEUE_SERVER_BUSY:int = 9988;
    public static const SRV_QUEUE_SERVER_BUSY_60s:int = 9999;
    public static const SRV_QUEUE_SERVER_FAULT:int = 9977;
    public static const SRV_QUEUE_TEST_STARTS_NOW:int = 0;

    // META test result fields.
    public static const META_CLIENT_OS:String = "client.os.name";
    public static const META_CLIENT_BROWSER:String = "client.browser.name";
    public static const META_CLIENT_KERNEL_VERSION:String =
        "client.kernel.version";
    public static const META_CLIENT_VERSION:String = "client.version";
    public static const META_CLIENT_APPLICATION:String = "client.application";

    // Allow the client to run the S2C and the C2S tests longer than 10sec.
    // This will cause the server to close the test sockets.
    public static const C2S_DURATION:int = 15000;  // 15sec
    public static const S2C_DURATION:int = 15000;  // 15sec
    // TCP constants.
    // Max size that can be sent, because 2 bytes are used to hold data length.
    public static const TCP_MAX_RECV_WIN_SIZE:int = 65535; // bytes
    // According to RFC1323, Section 2.3 the max valid value of iWinScaleRcvd is
    // 14. NDT uses 20 for this, leaving for now in case it is an error value.
    // TODO(tiziana): Check if the value is correct.
    public static const TCP_MAX_WINSCALERCVD:int = 20;
    public static const PREDEFINED_BUFFER_SIZE:int = 8192; // bytes

    // NDT specific limits.
    // See http://www.web100.org/download/kernel/tcp-kis.txt.
    public static const SND_LIM_TIME_THRESHOLD:Number = 0.15;
    // If the congestion windows is limited more than 0.5% of the time, NDT
    // claims that the connection is network limited.
    public static const CWND_LIM_TIME_THRESHOLD:Number = 0.005;
    // If the link speed is less than a T3, and loss is greater than 1 percent,
    // loss is determined to be excessive.
    public static const LOSS_THRESHOLD:Number = 0.01;

    // Line speed indicators as defined by the NDT server.
    public static const DATA_RATE_INSUFFICIENT_DATA:int = -2;
    public static const DATA_RATE_SYSTEM_FAULT:int = -1;
    public static const DATA_RATE_RTT:int = 0;
    public static const DATA_RATE_DIAL_UP:int = 1;
    public static const DATA_RATE_T1:int = 2;
    public static const DATA_RATE_ETHERNET:int = 3;
    public static const DATA_RATE_T3:int = 4;
    public static const DATA_RATE_FAST_ETHERNET:int = 5;
    public static const DATA_RATE_OC_12:int = 6;
    public static const DATA_RATE_GIGABIT_ETHERNET:int = 7;
    public static const DATA_RATE_OC_48:int = 8;
    public static const DATA_RATE_10G_ETHERNET:int = 9;

    public static const ACCESS_TECH:String = "accessTech";
    public static const ACCESS_TECH_UNKNOWN:String = "Connection type unknown";
    public static const ACCESS_TECH_DIALUP:String = "Dial-up Modem";
    public static const ACCESS_TECH_CABLEDSL:String = "Cable/DSL modem";
    public static const ACCESS_TECH_10MBPS:String = "10 Mbps Ethernet";
    public static const ACCESS_TECH_45MBPS:String = "45 Mbps T3/DS3 subnet";
    public static const ACCESS_TECH_100MBPS:String = "100 Mbps Ethernet";
    public static const ACCESS_TECH_622MBPS:String = "622 Mbps OC-12";
    public static const ACCESS_TECH_1GBPS:String = "1.0 Gbps Gigabit Ethernet";
    public static const ACCESS_TECH_2GBPS:String = "2.4 Gbps OC-48";
    public static const ACCESS_TECH_10GBPS:String = "10 Gigabit Ethernet/OC-192";

    public static const ACCESS_TECH2LINK_SPEED:Object = {
        ACCESS_TECH_UNKNOWN:NaN,
        ACCESS_TECH_DIALUP:0.064,  // 64 kbps
        ACCESS_TECH_CABLEDSL:3,    // 3 Mbps
        ACCESS_TECH_10MBPS:10,
        ACCESS_TECH_45MBPS:45,
        ACCESS_TECH_100MBPS:100,
        ACCESS_TECH_622MBPS:622,
        ACCESS_TECH_1GBPS:1000,
        ACCESS_TECH_2GBPS:2400,
        ACCESS_TECH_10GBPS:10000
    }

    // Duplex indicators as defined by the NDT server.
    public static const DUPLEX_OK_INDICATOR:int = 0;
    public static const DUPLEX_NOK_INDICATOR:int = 1;
    public static const DUPLEX_SWITCH_FULL_HOST_HALF:int = 2;
    public static const DUPLEX_SWITCH_HALF_HOST_FULL:int = 3;
    public static const DUPLEX_SWITCH_FULL_HOST_HALF_POSS:int = 4;
    public static const DUPLEX_SWITCH_HALF_HOST_FULL_POSS:int = 5;
    public static const DUPLEX_SWITCH_HALF_HOST_FULL_WARN:int = 7;

    // Cable status.
    public static const CABLE_STATUS_OK:int = 0;
    public static const CABLE_STATUS_NOK:int = 1;

    // Congestion status.
    public static const CONGESTION_NONE:int = 0;
    public static const CONGESTION_YES:int = 1;

    // Values of TCP options.
    public static const SACKENABLED_OFF:int = 0;
    public static const NAGLEENABLED_OFF:int = 0;
    public static const ECNENABLED_OFF:int = 0;
    public static const TIMESTAMPSENABLED_OFF:int = 0;

    // Speed difference to detect packet queueing.
    public static const SPD_DIFF:Number = 0.1;

    // Constants for unit convertions.
    public static const SEC2MSEC:uint = 1000;
    public static const KBITS2BITS:uint = 1024;
    public static const BYTES2BITS:uint = 8;
    public static const PERCENTAGE:uint = 100;

    // HTML tags.
    public static const HTML_LOCALE:String = "Locale";
    public static const HTML_USERAGENT:String = "UserAgentString";
    public static const HTML_SERVER_HOSTNAME:String = "ServerHostname";
    public static const HTML_BAD_ENV_ACTION:String = "BadRuntimeAction";

    // Names of NDT variables sent by the server.
    public static const MSSSENT:String = "MSSSent";
    public static const MSSRCVD:String = "MSSRcvd";
    public static const ECNENABLED:String = "ECNEnabled";
    public static const NAGLEENABLED:String = "NagleEnabled";
    public static const SACKENABLED:String = "SACKEnabled";
    public static const TIMESTAMPSENABLED:String = "TimestampsEnabled";
    public static const WINSCALERCVD:String = "WinScaleRcvd";
    public static const WINSCALESENT:String = "WinScaleSent";
    public static const SUMRTT:String = "SumRTT";
    public static const COUNTRTT:String = "CountRTT";
    public static const CURMSS:String = "CurMSS";
    public static const TIMEOUTS:String = "Timeouts";
    public static const PKTSRETRANS:String = "PktsRetrans";
    public static const SACKSRCVD:String = "SACKsRcvd";
    public static const DUPACKSIN:String = "DupAcksIn";
    public static const MAXRWINRCVD:String = "MaxRwinRcvd";
    public static const MAXRWINSENT:String = "MaxRwinSent";
    public static const SNDBUF:String = "Sndbuf";
    public static const X_RCVBUF:String = "X_Rcvbuf";
    public static const DATAPKTSOUT:String = "DataPktsOut";
    public static const FASTRETRAN:String = "FastRetran";
    public static const ACKPKTSOUT:String = "AckPktsOut";
    public static const SMOOTHEDRTT:String = "SmoothedRTT";
    public static const CURCWND:String = "CurCwnd";
    public static const MAXCWND:String = "MaxCwnd";
    public static const SNDLIMTIMERWIN:String = "SndLimTimeRwin";
    public static const SNDLIMTIMECWND:String = "SndLimTimeCwnd";
    public static const SNDLIMTIMESENDER:String = "SndLimTimeSender";
    public static const DATABYTESOUT:String = "DataBytesOut";
    public static const ACKPKTSIN:String = "AckPktsIn";
    public static const SNDLIMTRANSRWIN:String = "SndLimTransRwin";
    public static const SNDLIMTRANSCWND:String = "SndLimTransCwnd";
    public static const SNDLIMTRANSSENDER:String = "SndLimTransSender";
    public static const MAXSSTHRESH:String = "MaxSsthresh";
    public static const CURRTO:String = "CurRTO";
    public static const MAXRTO:String = "MaxRTO";
    public static const MINRTO:String = "MinRTO";
    public static const MINRTT:String = "MinRTT";
    public static const MAXRTT:String = "MaxRTT";
    public static const CURRWINRCVD:String = "CurRwinRcvd";
    public static const C2SDATA:String = "c2sData";
    public static const C2SACK:String = "c2sAck";
    public static const S2CDATA:String = "s2cData";
    public static const S2CACK:String = "s2cAck";
    public static const PKTSOUT:String = "PktsOut";
    public static const MISMATCH:String = "mismatch";
    public static const CONGESTION:String = "congestion";
    public static const BAD_CABLE:String = "bad_cable";
    public static const HALF_DUPLEX:String = "half_duplex";
    public static const CONGESTIONSIGNALS:String = "CongestionSignals";
    public static const RCVWINSCALE:String = "RcvWinScale";
    public static const BW:String = "bw";
    public static const LOSS:String = "loss";
    public static const AVGRTT:String = "avgrtt";
    public static const WAITSEC:String = "waitsec";
    public static const TIMESEC:String = "timesec";
    public static const ORDER:String = "order";
    public static const RWINTIME:String = "rwintime";
    public static const SENDTIME:String = "sendtime";
    public static const CWNDTIME:String = "cwndtime";
    public static const RTTSEC:String = "rttsec";
    public static const RWIN:String = "rwin";
    public static const SWIN:String = "swin";
    public static const CWIN:String = "cwin";
    public static const SPD:String = "spd";
    public static const ASPD:String = "aspd";
    public static const OPTRCVRBUFF:String = "optimalRcvrBuffer";

    //Bad environment handlers
    public static const ENV_OK:String = "none"
    public static const BAD_ENV_WARN:String = "warn"
    public static const BAD_ENV_WARN_AND_LIMIT:String = "warn-limit"
    public static const BAD_ENV_ERROR:String = "error"
    public static const BAD_ENV_MESG:String = 
        "Due to performance limitations in the Adobe Flash "
        + "Runtime, the NDT flash test cannot measure "
        + "high-speed connections accurately unless "
        + "it is run within Google Chrome "
        + "on Mac OS and Linux, or any web-browser running "
        + "on Windows operating systems. Please use one of "
        + "these platforms or run the NDT Java client "
        + "to obtain precise measurements.";
  }
}

