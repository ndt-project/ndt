/*
Copyright 2003 University of Chicago.  All rights reserved.
The Web100 Network Diagnostic Tool (NDT) is distributed subject to
the following license conditions:
SOFTWARE LICENSE AGREEMENT
Software: Web100 Network Diagnostic Tool (NDT)

1. The "Software", below, refers to the Web100 Network Diagnostic Tool (NDT)
(in either source code, or binary form and accompanying documentation). Each
licensee is addressed as "you" or "Licensee."

2. The copyright holder shown above hereby grants Licensee a royalty-free
nonexclusive license, subject to the limitations stated herein and U.S. Government
license rights.

3. You may modify and make a copy or copies of the Software for use within your
organization, if you meet the following conditions: 
    a. Copies in source code must include the copyright notice and this Software
    License Agreement.
    b. Copies in binary form must include the copyright notice and this Software
    License Agreement in the documentation and/or other materials provided with the copy.

4. You may make a copy, or modify a copy or copies of the Software or any
portion of it, thus forming a work based on the Software, and distribute copies
outside your organization, if you meet all of the following conditions: 
    a. Copies in source code must include the copyright notice and this
    Software License Agreement;
    b. Copies in binary form must include the copyright notice and this
    Software License Agreement in the documentation and/or other materials
    provided with the copy;
    c. Modified copies and works based on the Software must carry prominent
    notices stating that you changed specified portions of the Software.

5. Portions of the Software resulted from work developed under a U.S. Government
contract and are subject to the following license: the Government is granted
for itself and others acting on its behalf a paid-up, nonexclusive, irrevocable
worldwide license in this computer software to reproduce, prepare derivative
works, and perform publicly and display publicly.

6. WARRANTY DISCLAIMER. THE SOFTWARE IS SUPPLIED "AS IS" WITHOUT WARRANTY
OF ANY KIND. THE COPYRIGHT HOLDER, THE UNITED STATES, THE UNITED STATES
DEPARTMENT OF ENERGY, AND THEIR EMPLOYEES: (1) DISCLAIM ANY WARRANTIES,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO ANY IMPLIED WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE OR NON-INFRINGEMENT,
(2) DO NOT ASSUME ANY LEGAL LIABILITY OR RESPONSIBILITY FOR THE ACCURACY,
COMPLETENESS, OR USEFULNESS OF THE SOFTWARE, (3) DO NOT REPRESENT THAT USE
OF THE SOFTWARE WOULD NOT INFRINGE PRIVATELY OWNED RIGHTS, (4) DO NOT WARRANT
THAT THE SOFTWARE WILL FUNCTION UNINTERRUPTED, THAT IT IS ERROR-FREE OR THAT
ANY ERRORS WILL BE CORRECTED.

7. LIMITATION OF LIABILITY. IN NO EVENT WILL THE COPYRIGHT HOLDER, THE
UNITED STATES, THE UNITED STATES DEPARTMENT OF ENERGY, OR THEIR EMPLOYEES:
BE LIABLE FOR ANY INDIRECT, INCIDENTAL, CONSEQUENTIAL, SPECIAL OR PUNITIVE
DAMAGES OF ANY KIND OR NATURE, INCLUDING BUT NOT LIMITED TO LOSS OF PROFITS
OR LOSS OF DATA, FOR ANY REASON WHATSOEVER, WHETHER SUCH LIABILITY IS ASSERTED
ON THE BASIS OF CONTRACT, TORT (INCLUDING NEGLIGENCE OR STRICT LIABILITY), OR
OTHERWISE, EVEN IF ANY OF SAID PARTIES HAS BEEN WARNED OF THE POSSIBILITY OF
SUCH LOSS OR DAMAGES.
The Software was developed at least in part by the University of Chicago,
as Operator of Argonne National Laboratory (http://miranda.ctd.anl.gov:7123/). 
 */

package net.measurementlab.ndt;

import java.io.*;
import java.net.*;
import java.util.*;

public class NdtTests implements Runnable {
  public static final String VERSION = "3.6.3";

  // Network (physical layer) types
  public static final String NETWORK_WIFI = "WIFI";
  public static final String NETWORK_MOBILE = "MOBILE";
  public static final String NETWORK_WIRED = "WIRED";
  public static final String NETWORK_UNKNOWN = "UNKNOWN";

  private static final byte TEST_MID = (1 << 0);
  private static final byte TEST_C2S = (1 << 1);
  private static final byte TEST_S2C = (1 << 2);
  private static final byte TEST_SFW = (1 << 3);
  private static final byte TEST_STATUS = (1 << 4);

  /* we really should do some clean-up in this java code... maybe later ;) */
  private static final byte COMM_FAILURE  = 0;
  private static final byte SRV_QUEUE     = 1;
  private static final byte MSG_LOGIN     = 2;
  private static final byte TEST_PREPARE  = 3;
  private static final byte TEST_START    = 4;
  private static final byte TEST_MSG      = 5;
  private static final byte TEST_FINALIZE = 6;
  private static final byte MSG_ERROR     = 7;
  private static final byte MSG_RESULTS   = 8;
  private static final byte MSG_LOGOUT    = 9;
  private static final byte MSG_WAITING   = 10;

  private static final int SFW_NOTTESTED  = 0;
  private static final int SFW_NOFIREWALL = 1;
  private static final int SFW_UNKNOWN    = 2;
  private static final int SFW_POSSIBLE   = 3;

  private static final double VIEW_DIFF = 0.1;

  private static final int CONTROL_PORT = 3001;

  private TextOutputAdapter diagnosis, statistics, results;

  String errmsg;
  boolean failed;
  URL location;
  String s;
  double t;
  int ECNEnabled, NagleEnabled, MSSSent, MSSRcvd;
  int SACKEnabled, TimestampsEnabled, WinScaleRcvd, WinScaleSent;
  int FastRetran, AckPktsOut, SmoothedRTT, CurrentCwnd, MaxCwnd;
  int SndLimTimeRwin, SndLimTimeCwnd, SndLimTimeSender;
  int SndLimTransRwin, SndLimTransCwnd, SndLimTransSender, MaxSsthresh;
  int SumRTT, CountRTT, CurrentMSS, Timeouts, PktsRetrans;
  int SACKsRcvd, DupAcksIn, MaxRwinRcvd, MaxRwinSent;
  int DataPktsOut, Rcvbuf, Sndbuf, AckPktsIn, DataBytesOut;
  int PktsOut, CongestionSignals, RcvWinScale;
  int pkts, lth=8192, CurrentRTO;
  int c2sData, c2sAck, s2cData, s2cAck;

  String emailText;
  double s2cspd, c2sspd, sc2sspd, ss2cspd;
  int ssndqueue;
  double sbytes;

  int half_duplex, congestion, bad_cable, mismatch;
  double mylink;
  double loss, estimate, avgrtt, spd, waitsec, timesec, rttsec;
  double order, rwintime, sendtime, cwndtime, rwin, swin, cwin;
  double aspd;

  String tmpstr, tmpstr2;
  byte tests = TEST_MID | TEST_C2S | TEST_S2C | TEST_SFW | TEST_STATUS;
  int c2sResult = SFW_NOTTESTED;
  int s2cResult = SFW_NOTTESTED;

  private ResourceBundle messages;

  private final String host;
  private final UiServices uiServices;
  private final String networkType;

  /*
   * Initializes the network test thread.
   *
   * @param host hostname of the test server
   * @param uiServices object for UI interaction
   * @param networkType indicates the type of network, e.g. 3G, Wifi, Wired, etc.
   */
  public NdtTests(String host, UiServices uiServices, String networkType) {
    this.host = host;
    this.uiServices = uiServices;
    this.networkType = networkType;
    diagnosis = new TextOutputAdapter(uiServices, UiServices.DIAG_VIEW);
    statistics = new TextOutputAdapter(uiServices, UiServices.STAT_VIEW);
    results = new TextOutputAdapter(uiServices, UiServices.MAIN_VIEW);
    try {
      messages = ResourceBundle.getBundle("Tcpbw100_msgs", Locale.getDefault());
    } catch (MissingResourceException e) {
      // Fall back to US English if the locale we want is missing
      messages = ResourceBundle.getBundle("Tcpbw100_msgs", new Locale("en", "US"));
    }
  }

  public void run() {
    uiServices.onBeginTest();
    try {
      dottcp();
    } catch (IOException e) {
      uiServices.logError("Error running test: " + e);
      failed = true;
      errmsg = messages.getString("serverBusy30s") + "\n";
    }
    if (failed) {
      uiServices.logError(errmsg);
      uiServices.onFailure(errmsg);
    }
    // Finish the Test
    uiServices.onEndTest();
    return;
  }

  class Message {
    byte type;
    byte[] body;
  }
  
  class Protocol {
    private InputStream _ctlin;
    private OutputStream _ctlout;

    public Protocol(Socket ctlSocket) throws IOException
    {
      _ctlin = ctlSocket.getInputStream();
      _ctlout = ctlSocket.getOutputStream();
    }
    
    public void send_msg(byte type, byte toSend) throws IOException
    {
      byte[] tab = new byte[] { toSend };
      send_msg(type, tab);
    }

    public void send_msg(byte type, byte[] tab) throws IOException
    {
      byte[] header = new byte[3];
      header[0] = type;
      header[1] = (byte) (tab.length >> 8);
      header[2] = (byte) tab.length;
      
      _ctlout.write(header);
      _ctlout.write(tab);
    }

    public int readn(Message msg, int amount) throws IOException
    {
      int read = 0; 
      int tmp;
      msg.body = new byte[amount];
      while (read != amount) {
        tmp = _ctlin.read(msg.body, read, amount - read);
        if (tmp <= 0) {
          return read;
        }
        read += tmp;
      }
      return read;
    }
    
    public int recv_msg(Message msg) throws IOException
    {
      int length;
      if (readn(msg, 3) != 3) {
        return 1;
      }
      msg.type = msg.body[0];
      length = ((int) msg.body[1] & 0xFF) << 8;
      length += (int) msg.body[2] & 0xFF; 
      if (readn(msg, length) != length) {
        return 3;
      }
      return 0;
    }

    public void close()
    {
      try {
        _ctlin.close();
        _ctlout.close();
      }
      catch (IOException e) {
        e.printStackTrace();
      }
    }
  }

  public boolean test_mid(Protocol ctl) throws IOException
  {
		byte buff[] = new byte[8192];
    Message msg = new Message();
    if ((tests & TEST_MID) == TEST_MID) {
      /* now look for middleboxes (firewalls, NATs, and other boxes that
       * muck with TCP's end-to-end priciples
       */
      showStatus(messages.getString("middleboxTest"));
      results.append(messages.getString("checkingMiddleboxes") + "  ");
      statistics.append(messages.getString("checkingMiddleboxes") + "  ");
      emailText = messages.getString("checkingMiddleboxes") + "  ";

      if (ctl.recv_msg(msg) != 0) {
        errmsg = messages.getString("protocolError") + Integer.parseInt(new String(msg.body), 16) + " instead\n";
        return true;
      }
      if (msg.type != TEST_PREPARE) {
        errmsg = messages.getString("mboxWrongMessage") + "\n";
        if (msg.type == MSG_ERROR) {
            errmsg += "ERROR MSG: " + Integer.parseInt(new String(msg.body), 16) + "\n";
        }
        return true;
      }
      int midport = Integer.parseInt(new String(msg.body));

      Socket in2Socket = null;
      try {
        in2Socket = new Socket(host, midport);
      } catch (UnknownHostException e) {
        uiServices.logError("Don't know about host: " + host);
        errmsg = messages.getString("unknownServer") + "\n" ;
        return true;
      } catch (IOException e) {
        uiServices.logError("Couldn't perform middlebox testing to: " + host);
        errmsg = messages.getString("middleboxFail") + "\n" ;
        return true;
      }

      InputStream srvin2 = in2Socket.getInputStream();
      OutputStream srvout2 = in2Socket.getOutputStream();

      int largewin = 128*1024;

      in2Socket.setSoTimeout(6500);
      int bytes = 0;
      int inlth;
      t = System.currentTimeMillis();
	  uiServices.setVariable("pub_TimeStamp", new Date());

      try {  
        while ((inlth=srvin2.read(buff,0,buff.length)) > 0) {
          bytes += inlth;
          if ((System.currentTimeMillis() - t) > 5500)
            break;
        }
      } 
      catch (IOException e) {}

      t =  System.currentTimeMillis() - t;
      System.out.println(bytes + " bytes " + (8.0 * bytes)/t + " kb/s " + t/1000 + " secs");
      s2cspd = ((8.0 * bytes) / 1000) / t;

      if (ctl.recv_msg(msg) != 0) {
        errmsg = messages.getString("protocolError") + Integer.parseInt(new String(msg.body), 16) + " instead\n";
        return true;
      }
      if (msg.type != TEST_MSG) {
        errmsg = messages.getString("mboxWrongMessage") + "\n";
        if (msg.type == MSG_ERROR) {
            errmsg += "ERROR MSG: " + Integer.parseInt(new String(msg.body), 16) + "\n";
        }
        return true;
      }
      tmpstr2 = new String(msg.body);

      String tmpstr4 = Double.toString(s2cspd*1000);
      System.out.println("Sending '" + tmpstr4 + "' back to server");
      ctl.send_msg(TEST_MSG, tmpstr4.getBytes());

      try {
        tmpstr2 += in2Socket.getInetAddress() + ";";
      } catch (SecurityException e) {
        uiServices.logError("Unable to obtain Servers IP addresses: using " + host);
        errmsg = "getInetAddress() called failed\n" ;
        tmpstr2 += host + ";";
        results.append(messages.getString("lookupError") + "\n");
      }

      uiServices.logError("calling in2Socket.getLocalAddress()");
      try {
        tmpstr2 += in2Socket.getLocalAddress() + ";";
      } catch (SecurityException e) {
        uiServices.logError("Unable to obtain local IP address: using 127.0.0.1");
        errmsg = "getLocalAddress() call failed\n" ;
        tmpstr2 += "127.0.0.1;";
      }

      srvin2.close();
      srvout2.close();
      in2Socket.close();

      if (ctl.recv_msg(msg) != 0) {
        errmsg = messages.getString("protocolError") + Integer.parseInt(new String(msg.body), 16) + " instead\n";
        return true;
      }
      if (msg.type != TEST_FINALIZE) {
        errmsg = messages.getString("mboxWrongMessage");
        if (msg.type == MSG_ERROR) {
            errmsg += "ERROR MSG: " + Integer.parseInt(new String(msg.body), 16) + "\n";
        }
        return true;
      }
      results.append(messages.getString("done") + "\n");
      statistics.append(messages.getString("done") + "\n");
      emailText += messages.getString("done") + "\n%0A";
    }
    return false;
  }

  public boolean test_sfw(Protocol ctl) throws IOException
  {
    Message msg = new Message();
    if ((tests & TEST_SFW) == TEST_SFW) {
      showStatus(messages.getString("sfwTest"));
      results.append(messages.getString("checkingFirewalls") + "  ");
      statistics.append(messages.getString("checkingFirewalls") + "  ");
      emailText = messages.getString("checkingFirewalls") + "  ";
      
      if (ctl.recv_msg(msg) != 0) {
        errmsg = messages.getString("protocolError") + Integer.parseInt(new String(msg.body), 16) + " instead\n";
        return true;
      }
      if (msg.type != TEST_PREPARE) {
        errmsg = messages.getString("sfwWrongMessage") + "\n";
        if (msg.type == MSG_ERROR) {
            errmsg += "ERROR MSG: " + Integer.parseInt(new String(msg.body), 16) + "\n";
        }
        return true;
      }

      String message = new String(msg.body);

      int srvPort, testTime;
      try {
        int k = message.indexOf(" ");
        srvPort = Integer.parseInt(message.substring(0,k));
        testTime = Integer.parseInt(message.substring(k+1));
      }
      catch (Exception e) {
        errmsg = messages.getString("sfwWrongMessage") + "\n";
        return true;
      }

      System.out.println("SFW: port=" + srvPort);
      System.out.println("SFW: testTime=" + testTime);

      ServerSocket srvSocket;
      try {
          SecurityManager security = System.getSecurityManager();
          if (security != null) {
              System.out.println("Asking security manager for listen permissions...");
              security.checkListen(0);
          }
          srvSocket = new ServerSocket(0);
      }
      catch (Exception e) {
        e.printStackTrace();
        errmsg = messages.getString("sfwSocketFail") + "\n";
        return true;
      }

      System.out.println("SFW: oport=" + srvSocket.getLocalPort());
      ctl.send_msg(TEST_MSG, Integer.toString(srvSocket.getLocalPort()).getBytes());

      if (ctl.recv_msg(msg) != 0) {
        errmsg = messages.getString("protocolError") + Integer.parseInt(new String(msg.body), 16) + " instead\n";
        return true;
      }
      if (msg.type != TEST_START) {
        errmsg = messages.getString("sfwWrongMessage");
        if (msg.type == MSG_ERROR) {
            errmsg += "ERROR MSG: " + Integer.parseInt(new String(msg.body), 16) + "\n";
        }
        return true;
      }     

      OsfwWorker osfwTest = new OsfwWorker(srvSocket, testTime);
      new Thread(osfwTest).start();

      Socket sfwSocket = new Socket();
      try {
        sfwSocket.connect(new InetSocketAddress(host, srvPort), testTime * 1000);

        Protocol sfwCtl = new Protocol(sfwSocket);
        sfwCtl.send_msg(TEST_MSG, new String("Simple firewall test").getBytes());
      }
      catch (Exception e) {
        e.printStackTrace();
      }

      if (ctl.recv_msg(msg) != 0) {
        errmsg = messages.getString("protocolError") + Integer.parseInt(new String(msg.body), 16) + " instead\n";
        return true;
      }
      if (msg.type != TEST_MSG) {
        errmsg = messages.getString("sfwWrongMessage") + "\n";
        if (msg.type == MSG_ERROR) {
            errmsg += "ERROR MSG: " + Integer.parseInt(new String(msg.body), 16) + "\n";
        }
        return true;
      }
      c2sResult = Integer.parseInt(new String(msg.body));

      osfwTest.finalize();

      if (ctl.recv_msg(msg) != 0) {
        errmsg = messages.getString("protocolError") + Integer.parseInt(new String(msg.body), 16) + " instead\n";
        return true;
      }
      if (msg.type != TEST_FINALIZE) {
        errmsg = messages.getString("sfwWrongMessage") + "\n";
        if (msg.type == MSG_ERROR) {
            errmsg += "ERROR MSG: " + Integer.parseInt(new String(msg.body), 16) + "\n";
        }
        return true;
      }
      results.append(messages.getString("done") + "\n");
      statistics.append(messages.getString("done") + "\n");
      emailText += messages.getString("done") + "\n%0A";
    }
    return false;
  }

  public boolean test_c2s(Protocol ctl) throws IOException
  {
		// byte buff2[] = new byte[8192];
		byte buff2[] = new byte[64*1024];
    Message msg = new Message();
    if ((tests & TEST_C2S) == TEST_C2S) {
      showStatus(messages.getString("outboundTest"));
      results.append(messages.getString("runningOutboundTest") + " ");
      statistics.append(messages.getString("runningOutboundTest") + " ");
      emailText += messages.getString("runningOutboundTest") + " ";
      
      if (ctl.recv_msg(msg) != 0) {
        errmsg = messages.getString("protocolError") + Integer.parseInt(new String(msg.body), 16) + " instead\n";
        return true;
      }
      if (msg.type != TEST_PREPARE) {
        errmsg = messages.getString("outboundWrongMessage") + "\n";
        if (msg.type == MSG_ERROR) {
            errmsg += "ERROR MSG: " + Integer.parseInt(new String(msg.body), 16) + "\n";
        }
        return true;
      }
      int c2sport = Integer.parseInt(new String(msg.body));

      Socket outSocket = null;
      try {
        outSocket = new Socket(host, c2sport);
      } catch (UnknownHostException e) {
        uiServices.logError("Don't know about host: " + host);
        errmsg = messages.getString("unknownServer") + "\n" ;
        return true;
      } catch (IOException e) {
        uiServices.logError("Couldn't get 2nd connection to: " + host);
        errmsg = messages.getString("serverBusy15s") + "\n";
        return true;
      }

	// Get client and server IP addresses from the outSocket.
	uiServices.setVariable("pub_clientIP", outSocket.getLocalAddress().getHostAddress().toString());
	uiServices.setVariable("pub_host", outSocket.getInetAddress().getHostAddress().toString());


      OutputStream out = outSocket.getOutputStream();

      // wait here for signal from server application 
      // This signal tells the client to start pumping out data
      if (ctl.recv_msg(msg) != 0) {
        errmsg = messages.getString("protocolError") + Integer.parseInt(new String(msg.body), 16) + " instead\n";
        return true;
      }
      if (msg.type != TEST_START) {
        errmsg = messages.getString("outboundWrongMessage") + "\n";
        if (msg.type == MSG_ERROR) {
            errmsg += "ERROR MSG: " + Integer.parseInt(new String(msg.body), 16) + "\n";
        }
        return true;
      }

      Random rng = new Random();
      byte c = '0';
      int i;
      for (i=0; i<lth; i++) {
        if (c == 'z')
          c = '0';
        buff2[i] = c++;
      }
      uiServices.logError("Send buffer size =" + i);
      outSocket.setSoTimeout(15000); 
      pkts = 0;
      t = System.currentTimeMillis();
      double stop_time = t + 10000; // ten seconds
      do {
        // if (Randomize) rng.nextBytes(buff2);
        try {
          out.write(buff2, 0, buff2.length);
        }
        catch (SocketException e) {
          System.out.println(e);
          break;
        }
        pkts++;
      } while (System.currentTimeMillis() < stop_time);

      t =  System.currentTimeMillis() - t;
      if (t == 0) {
        t = 1;
      }
      out.close();
      outSocket.close();
      System.out.println((8.0 * pkts * lth) / t + " kb/s outbound");
      c2sspd = ((8.0 * pkts * lth) / 1000) / t;
      /* receive the c2sspd from the server */
      if (ctl.recv_msg(msg) != 0) {
        errmsg = messages.getString("protocolError") + Integer.parseInt(new String(msg.body), 16) + " instead\n";
        return true;
      }
      if (msg.type != TEST_MSG) {
        errmsg = messages.getString("outboundWrongMessage");
        if (msg.type == MSG_ERROR) {
            errmsg += "ERROR MSG: " + Integer.parseInt(new String(msg.body), 16) + "\n";
        }
        return true;
      }
      String tmpstr3 = new String(msg.body);
      sc2sspd = Double.parseDouble(tmpstr3) / 1000.0;

      if (sc2sspd < 1.0) {
        results.append(prtdbl(sc2sspd*1000) + "kb/s\n");
        statistics.append(prtdbl(sc2sspd*1000) + "kb/s\n");
        emailText += prtdbl(sc2sspd*1000) + "kb/s\n%0A";
      } 
      else {
        results.append(prtdbl(sc2sspd) + "Mb/s\n");
        statistics.append(prtdbl(sc2sspd) + "Mb/s\n");
        emailText += prtdbl(sc2sspd) + "Mb/s\n%0A";
      }

		// Expose upload speed to JavaScript clients
           uiServices.setVariable("pub_c2sspd", sc2sspd);
    
      if (ctl.recv_msg(msg) != 0) {
        errmsg = messages.getString("protocolError") + Integer.parseInt(new String(msg.body), 16) + " instead\n";
        return true;
      }
      if (msg.type != TEST_FINALIZE) {
        errmsg = messages.getString("outboundWrongMessage");
        if (msg.type == MSG_ERROR) {
            errmsg += "ERROR MSG: " + Integer.parseInt(new String(msg.body), 16) + "\n";
        }
        return true;
      }
    }
    return false;
  }

  public boolean test_s2c(Protocol ctl, Socket ctlSocket) throws IOException
  {
		byte buff[] = new byte[8192];
    Message msg = new Message();
    if ((tests & TEST_S2C) == TEST_S2C) {
        showStatus(messages.getString("inboundTest"));
        results.append(messages.getString("runningInboundTest") + " ");
        statistics.append(messages.getString("runningInboundTest") + " ");
        emailText += messages.getString("runningInboundTest") + " ";
      
      if (ctl.recv_msg(msg) != 0) {
        errmsg = messages.getString("protocolError") + Integer.parseInt(new String(msg.body), 16) + " instead\n";
        return true;
      }
      if (msg.type != TEST_PREPARE) {
        errmsg = messages.getString("inboundWrongMessage") + "\n";
        if (msg.type == MSG_ERROR) {
            errmsg += "ERROR MSG: " + Integer.parseInt(new String(msg.body), 16) + "\n";
        }
        return true;
      }
      int s2cport = Integer.parseInt(new String(msg.body));

      Socket inSocket;
      try {
        inSocket = new Socket(host, s2cport);
      } 
      catch (UnknownHostException e) {
        uiServices.logError("Don't know about host: " + host);
        errmsg = "unknown server\n" ;
        return true;
      } 
      catch (IOException e) {
        uiServices.logError("Couldn't get 3rd connection to: " + host);
        errmsg = "Server Failed while receiving data\n" ;
        return true;
      }

      InputStream srvin = inSocket.getInputStream();
      int bytes = 0;
      int inlth;

      // wait here for signal from server application 
      if (ctl.recv_msg(msg) != 0) {
        errmsg = messages.getString("unknownServer") + Integer.parseInt(new String(msg.body), 16) + " instead\n";
        return true;
      }
      if (msg.type != TEST_START) {
        errmsg = messages.getString("serverFail") + "\n";
        if (msg.type == MSG_ERROR) {
            errmsg += "ERROR MSG: " + Integer.parseInt(new String(msg.body), 16) + "\n";
        }
        return true;
      }

      inSocket.setSoTimeout(15000);
      t = System.currentTimeMillis();

      try {  
        while ((inlth=srvin.read(buff,0,buff.length)) > 0) {
          bytes += inlth;
          if ((System.currentTimeMillis() - t) > 14500)
            break;
        }
      } 
      catch (IOException e) {}

      t =  System.currentTimeMillis() - t;
      System.out.println(bytes + " bytes " + (8.0 * bytes)/t + " kb/s " + t/1000 + " secs");
      s2cspd = ((8.0 * bytes) / 1000) / t;

      /* receive the s2cspd from the server */
      if (ctl.recv_msg(msg) != 0) {
        errmsg = messages.getString("protocolError") + Integer.parseInt(new String(msg.body), 16) + " instead\n";
        return true;
      }
      if (msg.type != TEST_MSG) {
        errmsg = messages.getString("inboundWrongMessage") + "\n";
        if (msg.type == MSG_ERROR) {
            errmsg += "ERROR MSG: " + Integer.parseInt(new String(msg.body), 16) + "\n";
        }
        return true;
      }
      try {
        String tmpstr3 = new String(msg.body);
        int k1 = tmpstr3.indexOf(" ");
        int k2 = tmpstr3.substring(k1+1).indexOf(" ");
        ss2cspd = Double.parseDouble(tmpstr3.substring(0, k1)) / 1000.0;
        ssndqueue = Integer.parseInt(tmpstr3.substring(k1+1).substring(0, k2));
        sbytes = Double.parseDouble(tmpstr3.substring(k1+1).substring(k2+1));
      }
      catch (Exception e) {
        e.printStackTrace();
        errmsg = messages.getString("inboundWrongMessage") + "\n";
        return true;
      }

      if (s2cspd < 1.0) {
        results.append(prtdbl(s2cspd*1000) + "kb/s\n");
        statistics.append(prtdbl(s2cspd*1000) + "kb/s\n");
        emailText += prtdbl(s2cspd*1000) + "kb/s\n%0A";
      } else {
        results.append(prtdbl(s2cspd) + "Mb/s\n");
        statistics.append(prtdbl(s2cspd) + "Mb/s\n");
        emailText += prtdbl(s2cspd) + "Mb/s\n%0A";
      }


		// Expose download speed to JavaScript clients
           uiServices.setVariable("pub_s2cspd", s2cspd);

      srvin.close();
      inSocket.close();

      buff = Double.toString(s2cspd*1000).getBytes();
      String tmpstr4 = new String(buff, 0, buff.length);
      System.out.println("Sending '" + tmpstr4 + "' back to server");
      ctl.send_msg(TEST_MSG, buff);

      /* get web100 variables from server */
      tmpstr = "";
      int i = 0;

      // Try setting a 5 second timer here to break out if the read fails.
      ctlSocket.setSoTimeout(5000);
      try {  
        for (;;) {
          if (ctl.recv_msg(msg) != 0) {
            errmsg = messages.getString("protocolError") + Integer.parseInt(new String(msg.body), 16) + " instead\n";
            return true;
          }
          if (msg.type == TEST_FINALIZE) {
            break;
          }
          if (msg.type != TEST_MSG) {
            errmsg = messages.getString("inboundWrongMessage") + "\n";
            if (msg.type == MSG_ERROR) {
                errmsg += "ERROR MSG: " + Integer.parseInt(new String(msg.body), 16) + "\n";
            }
            return true;
          }
          tmpstr += new String(msg.body);
          i++;
        }
      } catch (IOException e) {}
      ctlSocket.setSoTimeout(0);
    }
    return false;
  }

  private void dottcp() throws IOException {
      Socket ctlSocket = null;
      int ctlport = CONTROL_PORT;
      	double wait2;
      	int sbuf, rbuf;
      	int i, wait, swait=0;

      	failed = false;
          
      try {
		  
		// RAC Debug message
        results.append(messages.getString("connectingTo") + " '" + host + "' [" + InetAddress.getByName(host) + "] " + messages.getString("toRunTest") + "\n");

          ctlSocket = new Socket(host, ctlport);
      } catch (UnknownHostException e) {
          uiServices.logError("Don't know about host: " + host);
          errmsg = messages.getString("unknownServer") + "\n" ;
          failed = true;
          return;
      } catch (IOException e) {
          uiServices.logError("Couldn't get the connection to: " + host + " " +ctlport);
          errmsg = messages.getString("serverNotRunning") + " (" + host + ":" + ctlport + ")\n" ;
          failed = true;
          return;
      }
      Protocol ctl = new Protocol(ctlSocket);
      Message msg = new Message();
      uiServices.incrementProgress();

      /* The beginning of the protocol */

      if (ctlSocket.getInetAddress() instanceof Inet6Address) {
          results.append(messages.getString("connected") + " " + host + messages.getString("usingIpv6") + "\n");
      }
      else {
          results.append(messages.getString("connected") + " " + host + messages.getString("usingIpv4") + "\n");
      }

      /* write our test suite request */
      ctl.send_msg(MSG_LOGIN, tests);
      /* read the specially crafted data that kicks off the old clients */
      if (ctl.readn(msg, 13) != 13) {
          errmsg = messages.getString("unsupportedClient") + "\n";
          failed = true;
          return;
      }

      for (;;) {
          if (ctl.recv_msg(msg) != 0) {
              errmsg = messages.getString("protocolError") + Integer.parseInt(new String(msg.body), 16) + " instead\n";
              failed = true;
              return;
          }
          if (msg.type != SRV_QUEUE) {
              errmsg = messages.getString("loggingWrongMessage") + "\n";
              failed = true;
              return;
          }
          String tmpstr3 = new String(msg.body);
          wait = Integer.parseInt(tmpstr3);
          System.out.println("wait flag received = " + wait);

          if (wait == 0) {
              break;
          }

          if (wait == 9988) {
	    if (swait == 0) {
              errmsg = messages.getString("serverBusy") + "\n";
              failed = true;
              return;
	    } else {
              errmsg = messages.getString("serverFault") + "\n";
              failed = true;
              return;
	    }
          }

          if (wait == 9999) {
              errmsg = messages.getString("serverBusy60s") + "\n";
              failed = true;
              return;
          }

	  if (wait == 9990) {  // signal from the server to see if the client is still alive
      	      ctl.send_msg(MSG_WAITING, tests);
	      continue;
	  }

          // Each test should take less than 30 seconds, so tell them 45 sec * number of 
          // tests in the queue.
          wait = (wait * 45);
          results.append(messages.getString("otherClient") + wait + messages.getString("seconds") +".\n");
	  swait = 1;
      }

      uiServices.onLoginSent();

      if (ctl.recv_msg(msg) != 0) {
          errmsg = messages.getString("protocolError") + Integer.parseInt(new String(msg.body), 16) + " instead\n";
          failed = true;
          return;
      }
      if (msg.type != MSG_LOGIN) {
          errmsg = messages.getString("versionWrongMessage") + "\n";
          failed = true;
          return;
      }

      String vVersion = new String(msg.body);
      if (!vVersion.startsWith("v")) {
          errmsg = messages.getString("incompatibleVersion");
          failed = true;
          return;
      }
      System.out.println("Server version: " + vVersion.substring(1));

      if (ctl.recv_msg(msg) != 0) {
          errmsg = messages.getString("protocolError") + Integer.parseInt(new String(msg.body), 16) + " instead\n";
          failed = true;
          return;
      }
      if (msg.type != MSG_LOGIN) {
          errmsg = messages.getString("testsuiteWrongMessage") + "\n";
          failed = true;
          return;
      }
      uiServices.incrementProgress();
      StringTokenizer tokenizer = new StringTokenizer(new String(msg.body), " ");

      while (tokenizer.hasMoreTokens()) {
          if (uiServices.wantToStop()) {
              ctl.send_msg(MSG_ERROR, "Manually stopped by the user".getBytes());
              ctl.close();
              ctlSocket.close();
              errmsg = "\n" + messages.getString("stopped") + "\n";
              failed = true;
              return;
          }
          int testId = Integer.parseInt(tokenizer.nextToken());
          switch (testId) {
              case TEST_MID:
                  uiServices.updateStatusPanel(messages.getString("middlebox"));
                  if (test_mid(ctl)) {
                      results.append(errmsg);
                      results.append(messages.getString("middleboxFail2") + "\n");
                      tests &= (~TEST_MID);
                  }
                  uiServices.incrementProgress();
                  break;
              case TEST_SFW:
                  uiServices.updateStatusPanel(messages.getString("simpleFirewall"));
                  if (test_sfw(ctl)) {
                      results.append(errmsg);
                      results.append(messages.getString("sfwFail") + "\n");
                      tests &= (~TEST_SFW);
                  }
                  uiServices.incrementProgress();
                  break;
              case TEST_C2S:
                  uiServices.updateStatusPanel(messages.getString("c2sThroughput"));
                  if (test_c2s(ctl)) {
                      results.append(errmsg);
                      results.append(messages.getString("c2sThroughputFailed") + "\n");
                      tests &= (~TEST_C2S);
                  }
                  uiServices.incrementProgress();
                  break;
              case TEST_S2C:
                  uiServices.updateStatusPanel(messages.getString("s2cThroughput"));
                  if (test_s2c(ctl, ctlSocket)) {
                      results.append(errmsg);
                      results.append(messages.getString("s2cThroughputFailed") + "\n");
                      tests &= (~TEST_S2C);
                  }
                  uiServices.incrementProgress();
                  break;
              default:
                  errmsg = messages.getString("unknownID") + "\n";
                  failed = true;
                  return;
          }
      }
      if (uiServices.wantToStop()) {
          ctl.send_msg(MSG_ERROR, "Manually stopped by the user".getBytes());
          ctl.close();
          ctlSocket.close();
          errmsg = messages.getString("stopped") + "\n";
          failed = true;
          return;
      }

      uiServices.updateStatusPanel(messages.getString("receiving"));
      i = 0;

      try {  
          for (;;) {
              if (ctl.recv_msg(msg) != 0) {
                  errmsg = messages.getString("protocolError") + Integer.parseInt(new String(msg.body), 16) + " instead\n";
                  failed = true;
                  return;
              }
              if (msg.type == MSG_LOGOUT) {
                  break;
              }
              if (msg.type != MSG_RESULTS) {
                  errmsg = messages.getString("resultsWrongMessage") + "\n";
                  failed = true;
                  return;
              }
              tmpstr += new String(msg.body);
              i++;
          }
      } catch (IOException e) {}

      if (i == 0) {
          results.append(messages.getString("resultsTimeout") + "\n");
      }
      uiServices.logError("Calling InetAddress.getLocalHost() twice");
      try {
          diagnosis.append(messages.getString("client") + ": " + InetAddress.getLocalHost() + "\n");
      } catch (SecurityException e) {
          diagnosis.append(messages.getString("client") + ": 127.0.0.1\n");
          results.append(messages.getString("unableToObtainIP") + "\n");
          uiServices.logError("Unable to obtain local IP address: using 127.0.0.1");
      }

      try {
          emailText += messages.getString("client") + ": " + InetAddress.getLocalHost() + "\n%0A";
      } catch (SecurityException e) {
          emailText += messages.getString("client") + ": " + "127.0.0.1" + "\n%0A";
      }

      ctl.close();
      ctlSocket.close();

      try {
          testResults(tmpstr);
      }
      catch (Exception ex) {
          results.append(messages.getString("resultsParseError")  + "\n");
          results.append(ex + "\n");
      }
      if ((tests & TEST_MID) == TEST_MID) {
          middleboxResults(tmpstr2);
      }
	  
	  uiServices.setVariable("pub_isReady", "yes");
	  uiServices.setVariable("pub_errmsg", "All tests completed OK.");
      uiServices.incrementProgress();
  }


	class OsfwWorker implements Runnable
  {
    private ServerSocket srvSocket;
    private int testTime;
    private boolean finalized = false;
    
    OsfwWorker(ServerSocket srvSocket, int testTime)
    {
      this.srvSocket = srvSocket;
      this.testTime = testTime;
    }

    public void finalize()
    {
      while (!finalized) {
        try {
          Thread.currentThread().sleep(1000);
        }
        catch (InterruptedException e) {
          // do nothing.
        }
      }
    }
    
    public void run()
    {
      Message msg = new Message();
      Socket sock = null;

      try {
        srvSocket.setSoTimeout(testTime * 1000);
        try {
          sock = srvSocket.accept();
        }
        catch (Exception e) {
          e.printStackTrace();
          s2cResult = SFW_POSSIBLE;
          srvSocket.close();
          finalized = true;
          return;
        }
        Protocol sfwCtl = new Protocol(sock);

        if (sfwCtl.recv_msg(msg) != 0) {
          System.out.println("Simple firewall test: unrecognized message");
          s2cResult = SFW_UNKNOWN;
          sock.close();
          srvSocket.close();
          finalized = true;
          return;
        }
        if (msg.type != TEST_MSG) {
          s2cResult = SFW_UNKNOWN;
          sock.close();
          srvSocket.close();
          finalized = true;
          return;
        }
        if (! new String(msg.body).equals("Simple firewall test")) {
          System.out.println("Simple firewall test: Improper message");
          s2cResult = SFW_UNKNOWN;
          sock.close();
          srvSocket.close();
          finalized = true;
          return;
        }
        s2cResult = SFW_NOFIREWALL;
      }
      catch (IOException ex) {
        s2cResult = SFW_UNKNOWN;
      }
      try {
        sock.close();
        srvSocket.close();
      }
      catch (IOException e) {
        // do nothing
      }
      finalized = true;
    }
  }

	public void testResults(String tmpstr) {
		StringTokenizer tokens;
		int i=0;
		String sysvar, strval;
		int sysval, Zero=0, bwdelay, minwin;
		double sysval2, j;
		String osName, osArch, osVer, javaVer, javaVendor, client;

		tokens = new StringTokenizer(tmpstr);
		sysvar = null;
		strval = null;
		while(tokens.hasMoreTokens()) {
			if(++i%2 == 1) {
				sysvar = tokens.nextToken();
			}
			else {
				strval = tokens.nextToken();
				diagnosis.append(sysvar + " " + strval + "\n");
				emailText += sysvar + " " + strval + "\n%0A";
				if (strval.indexOf(".") == -1) {
					sysval = Integer.parseInt(strval);
					save_int_values(sysvar, sysval);
				}
				else {
					sysval2 = Double.valueOf(strval).doubleValue();
					save_dbl_values(sysvar, sysval2);
				}
			}
		}

		// Grab some client details from the applet environment
		osName = System.getProperty("os.name");
		uiServices.setVariable("pub_osName", osName);
	    
		osArch = System.getProperty("os.arch");
		uiServices.setVariable("pub_osArch", osArch);
		
		osVer = System.getProperty("os.version");
        uiServices.setVariable("pub_osVer", osVer);
		
		javaVer = System.getProperty("java.version");
		uiServices.setVariable("pub_javaVer", javaVer);
		
		javaVendor = System.getProperty("java.vendor");

		if (osArch.startsWith("x86") == true) {
			client = messages.getString("pc");
		}
		else {
			client = messages.getString("workstation");
		}

		// Calculate some variables and determine path conditions
		// Note: calculations now done in server and the results are shipped
		//    back to the client for printing.

		if (CountRTT>0) {
			// Now write some messages to the screen
			if (c2sData < 3) {
				if (c2sData < 0) {
					results.append(messages.getString("unableToDetectBottleneck") + "\n");
					emailText += "Server unable to determine bottleneck link type.\n%0A";
					uiServices.setVariable("pub_AccessTech", "Connection type unknown");
					
				} 
				else {
					results.append(messages.getString("your") + " " + client + " " + messages.getString("connectedTo") + " ");
					emailText += messages.getString("your") + " " + client + " " + messages.getString("connectedTo") + " ";
					if (c2sData == 1) {
						results.append(messages.getString("dialup") + "\n");
						emailText += messages.getString("dialup") +  "\n%0A";
						mylink = .064;
						uiServices.setVariable("pub_AccessTech", "Dial-up Modem");
					} 
					else {
						results.append(messages.getString("cabledsl") + "\n");
						emailText += messages.getString("cabledsl") +  "\n%0A";
						mylink = 3;
						uiServices.setVariable("pub_AccessTech", "Cable/DSL modem");
					}
				}
			} 
			else {
				results.append(messages.getString("theSlowestLink") + " ");
				emailText += messages.getString("theSlowestLink") + " ";
				if (c2sData == 3) {
					results.append(messages.getString("10mbps") + "\n");
					emailText += messages.getString("10mbps") + "\n%0A";
					mylink = 10;
					uiServices.setVariable("pub_AccessTech", "10 Mbps Ethernet");
				} 
				else if (c2sData == 4) {
					results.append(messages.getString("45mbps") + "\n");
					emailText += messages.getString("45mbps") + "\n%0A";
					mylink = 45;
					uiServices.setVariable("pub_AccessTech", "45 Mbps T3/DS3 subnet");
				} 
				else if (c2sData == 5) {
					results.append("100 Mbps ");
					emailText += "100 Mbps ";
					mylink = 100;
					uiServices.setVariable("pub_AccessTech", "100 Mbps Ethernet");
					
					if (half_duplex == 0) {
                        results.append(messages.getString("fullDuplex") + "\n");
                        emailText += messages.getString("fullDuplex") + "\n%0A";
					} 
					else {
                        results.append(messages.getString("halfDuplex") + "\n");
                        emailText += messages.getString("halfDuplex") + "\n%0A";
					}
				} 
				else if (c2sData == 6) {
					results.append(messages.getString("622mbps") + "\n");
					emailText += messages.getString("622mbps") + "\n%0A";
					mylink = 622;
					uiServices.setVariable("pub_AccessTech", "622 Mbps OC-12");
				} 
				else if (c2sData == 7) {
					results.append(messages.getString("1gbps") + "\n");
					emailText += messages.getString("1gbps") + "\n%0A";
					mylink = 1000;
					uiServices.setVariable("pub_AccessTech", "1.0 Gbps Gigabit Ethernet");
				} 
				else if (c2sData == 8) {
					results.append(messages.getString("2.4gbps") + "\n");
					emailText += messages.getString("2.4gbps") + "\n%0A";
					mylink = 2400;
					uiServices.setVariable("pub_AccessTech", "2.4 Gbps OC-48");
				} 
				else if (c2sData == 9) {
					results.append(messages.getString("10gbps") + "\n");
					emailText += messages.getString("10gbps") + "\n%0A";
					mylink = 10000;
					uiServices.setVariable("pub_AccessTech", "10 Gigabit Ethernet/OC-192");
					
				}
			}

 			if (mismatch == 1) {
				results.append(messages.getString("oldDuplexMismatch") + "\n");
                emailText += messages.getString("oldDuplexMismatch") + "\n%0A";
 			}
  			else if (mismatch == 2) {
				results.append(messages.getString("duplexFullHalf") + "\n");
                emailText += messages.getString("duplexFullHalf") + "\n%0A";
 			}
  			else if (mismatch == 4) {
				results.append(messages.getString("possibleDuplexFullHalf") + "\n");
                emailText += messages.getString("possibleDuplexFullHalf") + "\n%0A";
 			}
  			else if (mismatch == 3) {
				results.append(messages.getString("duplexHalfFull") + "\n");
                emailText += messages.getString("duplexHalfFull") + "\n%0A";
 			}
  			else if (mismatch == 5) {
				results.append(messages.getString("possibleDuplexHalfFull") + "\n");
                emailText += messages.getString("possibleDuplexHalfFull") + "\n%0A";
 			}
  			else if (mismatch == 7) {
				results.append(messages.getString("possibleDuplexHalfFullWarning") + "\n");
                emailText += messages.getString("possibleDuplexHalfFullWarning") + "\n%0A";
 			}
 			
			if (mismatch == 0) {
			    if (bad_cable == 1) {
                    results.append(messages.getString("excessiveErrors ") + "\n");
                    emailText += messages.getString("excessiveErrors") + "\n%0A";
			    }
			    if (congestion == 1) {
                    results.append(messages.getString("otherTraffic") + "\n");
                    emailText += messages.getString("otherTraffic") + "\n%0A";
			    }
			    if (((2*rwin)/rttsec) < mylink) {
			        j = (float)((mylink * avgrtt) * 1000) / 8 / 1024;
			        if (j > (float)MaxRwinRcvd) {
                        results.append(messages.getString("receiveBufferShouldBe") + " " + prtdbl(j) + messages.getString("toMaximizeThroughput") + " \n");
                        emailText += messages.getString("receiveBufferShouldBe") + " " + prtdbl(j) + messages.getString("toMaximizeThroughput") + "\n%0A";
			        }
			    }
			}
     
      if ((tests & TEST_C2S) == TEST_C2S) {
        if (sc2sspd < (c2sspd  * (1.0 - VIEW_DIFF))) {
          // TODO:  distinguish the host buffering from the middleboxes buffering
          uiServices.onPacketQueuingDetected();
          results.append(messages.getString("c2sPacketQueuingDetected") + "\n");
        }
      }
      
      if ((tests & TEST_S2C) == TEST_S2C) {
        if (s2cspd < (ss2cspd  * (1.0 - VIEW_DIFF))) {
          // TODO:  distinguish the host buffering from the middleboxes buffering
          uiServices.onPacketQueuingDetected();
          results.append(messages.getString("s2cPacketQueuingDetected") + "\n");
        }
      }

            statistics.append("\n\t------  " + messages.getString("clientInfo") + "------\n");
			statistics.append(messages.getString("osData") + " " + messages.getString("name") + " = " + osName + ", " + messages.getString("architecture") + " = " + osArch);
			statistics.append(", " + messages.getString("version") + " = " + osVer + "\n");
			statistics.append(messages.getString("javaData") + ": " +  messages.getString("vendor") + " = " + javaVendor + ", " + messages.getString("version") + " = " + javaVer + "\n");
			// statistics.append(" java.class.version=" + System.getProperty("java.class.version") + "\n");

			statistics.append("\n\t------  " + messages.getString("web100Details") + "  ------\n");
 			if (c2sData == -2)
				statistics.append(messages.getString("insufficient") + "\n");
 			else if (c2sData == -1)
				statistics.append(messages.getString("ipcFail") + "\n");
 			else if (c2sData == 0)
				statistics.append(messages.getString("rttFail") + "\n");
 			else if (c2sData == 1)
				statistics.append(messages.getString("foundDialup") + "\n");
 			else if (c2sData == 2)
				statistics.append(messages.getString("foundDsl") + "\n");
 			else if (c2sData == 3)
				statistics.append(messages.getString("found10mbps") + "\n");
 			else if (c2sData == 4)
				statistics.append(messages.getString("found45mbps") + "\n");
 			else if (c2sData == 5)
				statistics.append(messages.getString("found100mbps") + "\n");
 			else if (c2sData == 6)
				statistics.append(messages.getString("found622mbps") + "\n");
 			else if (c2sData == 7)
				statistics.append(messages.getString("found1gbps") + "\n");
 			else if (c2sData == 8)
				statistics.append(messages.getString("found2.4gbps") + "\n");
 			else if (c2sData == 9)
				statistics.append(messages.getString("found10gbps") + "\n");
 
 			if (half_duplex == 0)
				statistics.append(messages.getString("linkFullDpx") + "\n");
 			else
				statistics.append(messages.getString("linkHalfDpx") + "\n");
 
 			if (congestion == 0)
				statistics.append(messages.getString("congestNo") + "\n");
 			else
				statistics.append(messages.getString("congestYes") + "\n");
 
 			if (bad_cable == 0)
				statistics.append(messages.getString("cablesOk") + "\n");
 			else
				statistics.append(messages.getString("cablesNok") + "\n");
 
 			if (mismatch == 0)
				statistics.append(messages.getString("duplexOk") + "\n");
 			else if (mismatch == 1) {
				statistics.append(messages.getString("duplexNok") + " ");
				emailText += messages.getString("duplexNok") + " ";
 			}
  			else if (mismatch == 2) {
				statistics.append(messages.getString("duplexFullHalf") + "\n");
				emailText += messages.getString("duplexFullHalf") + "\n%0A ";
 			}
  			else if (mismatch == 3) {
				statistics.append(messages.getString("duplexHalfFull") + "\n");
				emailText += messages.getString("duplexHalfFull") + "\n%0A ";
 			}

			statistics.append("\n" + messages.getString("web100rtt") + " =  " + prtdbl(avgrtt) + " " + "ms" + "; ");
			emailText += "\n%0A" +  messages.getString("web100rtt") + " = " + prtdbl(avgrtt) + " " + "ms" + "; ";

			statistics.append(messages.getString("packetsize") + " = " + CurrentMSS + " " + messages.getString("bytes") + "; " + messages.getString("and") + " \n");
			emailText += messages.getString("packetsize") + " = " + CurrentMSS + " " + messages.getString("bytes") + "; " + messages.getString("and") + " \n%0A";

			if (PktsRetrans > 0) {
                statistics.append(PktsRetrans + " " + messages.getString("pktsRetrans"));
				statistics.append(", " + DupAcksIn + " " + messages.getString("dupAcksIn"));
				statistics.append(", " + messages.getString("and") + " " + SACKsRcvd + " " + messages.getString("sackReceived") + "\n");
				emailText += PktsRetrans + " " + messages.getString("pktsRetrans");
				emailText += ", " + DupAcksIn + " " + messages.getString("dupAcksIn");
				emailText += ", " + messages.getString("and") + " " + SACKsRcvd + " " + messages.getString("sackReceived") + "\n%0A";
				if (Timeouts > 0) {
                    statistics.append(messages.getString("connStalled") + " " + Timeouts + " " + messages.getString("timesPktLoss") + "\n");
				}
	
				statistics.append(messages.getString("connIdle") + " " + prtdbl(waitsec) + " " + messages.getString("seconds") + " (" + prtdbl((waitsec/timesec)*100) + messages.getString("pctOfTime") + ")\n");
				emailText += messages.getString("connStalled") + " " + Timeouts + " " + messages.getString("timesPktLoss") + "\n%0A";
				emailText += messages.getString("connIdle") + " " + prtdbl(waitsec) + " " + messages.getString("seconds") + " (" + prtdbl((waitsec/timesec)*100) + messages.getString("pctOfTime") + ")\n%0A";
			} 
			else if (DupAcksIn > 0) {
				statistics.append(messages.getString("noPktLoss1") + " - ");
				statistics.append(messages.getString("ooOrder") + " " + prtdbl(order*100) + messages.getString("pctOfTime") + "\n");
				emailText += messages.getString("noPktLoss1") + " - ";
				emailText += messages.getString("ooOrder") + " " + prtdbl(order*100) + messages.getString("pctOfTime") + "\n%0A";
			} 
			else {
				statistics.append(messages.getString("noPktLoss2") + ".\n");
				emailText += messages.getString("noPktLoss2") + ".\n%0A";
			}

      if ((tests & TEST_C2S) == TEST_C2S) {
        if (c2sspd > sc2sspd) {
          if (sc2sspd < (c2sspd  * (1.0 - VIEW_DIFF))) {
            statistics.append(messages.getString("c2s") + " " + messages.getString("qSeen") + ": " + prtdbl(100 * (c2sspd - sc2sspd) / c2sspd) + "%\n");
          }
          else {
            statistics.append(messages.getString("c2s") + " " + messages.getString("qSeen") + ": " + prtdbl(100 * (c2sspd - sc2sspd) / c2sspd) + "%\n");
          }
        }
      }
      
      if ((tests & TEST_S2C) == TEST_S2C) {
        if (ss2cspd > s2cspd) {
          if (s2cspd < (ss2cspd  * (1.0 - VIEW_DIFF))) {
            statistics.append(messages.getString("s2c") + " " + messages.getString("qSeen") + ": " + prtdbl(100 * (ss2cspd - s2cspd) / ss2cspd) + "%\n");
          }
          else {
            statistics.append(messages.getString("s2c") + " " + messages.getString("qSeen") + ": " + prtdbl(100 * (ss2cspd - s2cspd) / ss2cspd) + "%\n");
          }
        }
      }
      
			if (rwintime > .015) {
				statistics.append(messages.getString("thisConnIs") + " " + messages.getString("limitRx") + " " + prtdbl(rwintime*100) + messages.getString("pctOfTime") + ".\n");
				emailText += messages.getString("thisConnIs") + " " + messages.getString("limitRx") + " " + prtdbl(rwintime*100) + messages.getString("pctOfTime") + ".\n%0A";
				uiServices.setVariable("pub_pctRcvrLimited", rwintime*100);

			// I think there is a bug here, it sometimes tells you to increase the buffer
			// size, but the new size is smaller than the current.

				if (((2*rwin)/rttsec) < mylink) {
                    statistics.append("  " + messages.getString("incrRxBuf") + " (" + prtdbl(MaxRwinRcvd/1024) + " KB) " + messages.getString("willImprove") + "\n");
				}
			}
			if (sendtime > .015) {
                statistics.append(messages.getString("thisConnIs") + " " + messages.getString("limitTx") + " " + prtdbl(sendtime*100) + messages.getString("pctOfTime") + ".\n");
                emailText += messages.getString("thisConnIs") + " " + messages.getString("limitTx") + " " + prtdbl(sendtime*100) + messages.getString("pctOfTime") + ".\n%0A";
				if ((2*(swin/rttsec)) < mylink) {
                    statistics.append("  " + messages.getString("incrTxBuf") + " (" + prtdbl(Sndbuf/2048) + " KB) " + messages.getString("willImprove") + "\n");
				}
			}
			if (cwndtime > .005) {
				statistics.append(messages.getString("thisConnIs") + " " + messages.getString("limitNet") + " " + prtdbl(cwndtime*100) + messages.getString("pctOfTime") + ".\n");
				emailText += messages.getString("thisConnIs") + " " + messages.getString("limitNet") + " " + prtdbl(cwndtime*100) + messages.getString("pctOfTime") + ".\n%0A";
				// if (cwndtime > .15)
				//	statistics.append("  Contact your local network administrator to report a network problem\n");
				// if (order > .15)
				//	statistics.append("  Contact your local network admin and report excessive packet reordering\n");
			}
			if ((spd < 4) && (loss > .01)) {
				statistics.append(messages.getString("excLoss") + "\n");
			}
			
			statistics.append("\n" + messages.getString("web100tcpOpts") + " \n");
 			statistics.append("RFC 2018 Selective Acknowledgment: ");
 			if(SACKEnabled == Zero)
				statistics.append(messages.getString("off") + "\n");
 			else
				statistics.append(messages.getString("on") + "\n");
 
 			statistics.append("RFC 896 Nagle Algorithm: ");
 			if(NagleEnabled == Zero)
				statistics.append(messages.getString("off") + "\n");
 			else
				statistics.append(messages.getString("on") + "\n");
 
 			statistics.append("RFC 3168 Explicit Congestion Notification: ");
 			if(ECNEnabled == Zero)
				statistics.append(messages.getString("off") + "\n");
 			else
				statistics.append(messages.getString("on") + "\n");
 
 			statistics.append("RFC 1323 Time Stamping: ");
 			if(TimestampsEnabled == 0)
				statistics.append(messages.getString("off") + "\n");
 			else
				statistics.append(messages.getString("on") + "\n");
 
 			statistics.append("RFC 1323 Window Scaling: ");
 			if (MaxRwinRcvd < 65535)
 			    WinScaleRcvd = 0;
 			if((WinScaleRcvd == 0) || (WinScaleRcvd > 20))
				statistics.append(messages.getString("off") + "\n");
 			else
                statistics.append (messages.getString("on") + "; " + messages.getString("scalingFactors") + " -  " + messages.getString("server") + "=" + WinScaleRcvd + ", " + messages.getString("client") + "=" + WinScaleSent + "\n");
 			
      statistics.append("\n");

      if ((tests & TEST_SFW) == TEST_SFW) {
        switch (c2sResult) {
          case SFW_NOFIREWALL:
            statistics.append(messages.getString("server") + " '" + host + "' " + messages.getString("firewallNo") + "\n");
            emailText += messages.getString("server") + " '" + host + "' " + messages.getString("firewallNo") + "\n%0A";
            break;
          case SFW_POSSIBLE:
            statistics.append(messages.getString("server") + " '" + host + "' " + messages.getString("firewallYes") + "\n");
            emailText += messages.getString("server") + " '" + host + "' " + messages.getString("firewallYes") + "\n%0A";
            break;
          case SFW_UNKNOWN:
          case SFW_NOTTESTED:
            break;
        }
        switch (s2cResult) {
          case SFW_NOFIREWALL:
            statistics.append(messages.getString("client2") + " " + messages.getString("firewallNo") + "\n");
            emailText += messages.getString("client2") + " " + messages.getString("firewallNo") + "\n%0A";
            break;
          case SFW_POSSIBLE:
            statistics.append(messages.getString("client2") + " " + messages.getString("firewallYes") + "\n");
            emailText += messages.getString("client2") + " " + messages.getString("firewallYes") + "\n%0A";
            break;
          case SFW_UNKNOWN:
          case SFW_NOTTESTED:
            break;
        }
      }

//			diagnosis.append("\nEstimate = " + prtdbl(estimate) + " based on packet size = "
//				+ (CurrentMSS*8/1024) + "kbits, RTT = " + prtdbl(avgrtt) + "msec, " + "and loss = " + loss + "\n");
      diagnosis.append("\n");

            diagnosis.append(messages.getString("theoreticalLimit") + " " + prtdbl(estimate) + " " + "Mbps\n");
			emailText += messages.getString("theoreticalLimit") + " " + prtdbl(estimate) + " Mbps\n%0A";
 
            diagnosis.append(messages.getString("ndtServerHas") + " " + prtdbl(Sndbuf/2048) + " " + messages.getString("kbyteBufferLimits") + " " + prtdbl(swin/rttsec) + " Mbps\n");
			emailText += messages.getString("ndtServerHas") + " " + prtdbl(Sndbuf/2048) + " " + messages.getString("kbyteBufferLimits") + " " + prtdbl(swin/rttsec) + " Mbps\n%0A";

            diagnosis.append(messages.getString("yourPcHas") + " " + prtdbl(MaxRwinRcvd/1024) + " " + messages.getString("kbyteBufferLimits") + " " + prtdbl(rwin/rttsec) + " Mbps\n");
			emailText += messages.getString("yourPcHas") + " " + prtdbl(MaxRwinRcvd/1024) + " " + messages.getString("kbyteBufferLimits") + " " + prtdbl(rwin/rttsec) + " Mbps\n%0A";

			diagnosis.append(messages.getString("flowControlLimits") + " " +	prtdbl(cwin/rttsec) + " Mbps\n");
			emailText += messages.getString("flowControlLimits") + " " +	prtdbl(cwin/rttsec) + " Mbps\n%0A";

			diagnosis.append("\n" + messages.getString("clientDataReports") + " '" + prttxt(c2sData) + "', " + messages.getString("clientAcksReport") + " '" + prttxt(c2sAck) + "'\n" + messages.getString("serverDataReports") + " '" + prttxt(s2cData) + "', " + messages.getString("serverAcksReport") + " '" + prttxt(s2cAck) + "'\n");


		}
	}  // testResults()
	
	

	/* this routine decodes the middlebox test results.  The data is returned
	* from the server is a specific order.  This routine pulls the string apart
	* and puts the values into the proper variable.  It then compares the values
	* to known values and writes out the specific results.
	*
	* server data is first
	* order is Server IP; Client IP; CurrentMSS; WinScaleRcvd; WinScaleSent;
	* Client then adds
	* Server IP; Client IP.
	*/

	public void middleboxResults(String tmpstr2) {
		StringTokenizer tokens;
		int k;

		// results.append("Mbox stats: ");
		tokens = new StringTokenizer(tmpstr2, ";");
		String ssip = tokens.nextToken();
		String scip = tokens.nextToken();

		// Fix for JS API not reporting NAT'd IPs correctly
		// Assign client and server IP addresses for JA API
		// based on public, not local IP.  
		uiServices.setVariable("pub_clientIP", scip);


		// results.append("ssip=" + ssip + " scip=" + scip + "\n");

		// String mss = tokens.nextToken();
		// String winsrecv = tokens.nextToken();
		// String winssent = tokens.nextToken();
		int mss = Integer.parseInt(tokens.nextToken());
		int winsrecv = Integer.parseInt(tokens.nextToken());
		int winssent = Integer.parseInt(tokens.nextToken());

		String csip = tokens.nextToken();
		k = csip.indexOf("/");
		csip = csip.substring(k+1);

		String ccip = tokens.nextToken();
		k = ccip.indexOf("/");
		ccip = ccip.substring(k+1);

		// results.append("csip=" + csip + " ccip=" + ccip + "\n");
		// results.append("mss=" + mss + " winsrecv=" + winsrecv + " winssent=" +
		// 	winssent + "\n");

		if (mss == 1456)
			statistics.append(messages.getString("packetSizePreserved") + "\n");
		else
			statistics.append(messages.getString("middleboxModifyingMss") + "\n");

		// if ((winsrecv == -1) && (winssent == -1))
		//     statistics.append("Window scaling option is preserved End-to-End\n");
		// else
		//     statistics.append("Information: Network Middlebox is modifying Window scaling option\n");

    boolean preserved = false;
    try {
      preserved = InetAddress.getByName(ssip).equals(InetAddress.getByName(csip));
    }
    catch (UnknownHostException e) {
      preserved = ssip.equals(csip);
    }
		if (preserved) {
			statistics.append(messages.getString("serverIpPreserved") + "\n");
			uiServices.setVariable("pub_natBox", "no");
		}
		else {
			uiServices.setVariable("pub_natBox", "yes");
            statistics.append(messages.getString("serverIpModified") + "\n");
			statistics.append("\t" + messages.getString("serverSays") + " [" + ssip + "], " + messages.getString("clientSays") +" [" + csip + "]\n");
		}

		if (ccip.equals("127.0.0.1")) {
			statistics.append(messages.getString("clientIpNotFound") + "\n");
		}
		else {
      try {
        preserved = InetAddress.getByName(scip).equals(InetAddress.getByName(ccip));
      }
      catch (UnknownHostException e) {
        preserved = scip.equals(ccip);
      }
      catch (SecurityException e ) {
        preserved = scip.equals(ccip);
      }

			if (preserved)
				statistics.append(messages.getString("clientIpPreserved") + "\n");
			else {
                statistics.append(messages.getString("clientIpModified") + "\n");
                statistics.append("\t" + messages.getString("serverSays") + " [" + scip + "], " + messages.getString("clientSays") +" [" + ccip + "]\n");
			}
		}
	} // middleboxResults()



	public String prtdbl(double d) {
		String str = null;
		int i;

		if (d == 0) {
			return ("0");
		}
		str = Double.toString(d);
		i = str.indexOf(".");
		i = i + 3;
		if (i > str.length()) {
			i = i - 1;
		}
		if (i > str.length()) {
			i = i - 1;
		}
		return (str.substring(0, i));
	} // prtdbl()



	public String prttxt(int val) {
		String str = null;

		if (val == -1)
			str = messages.getString("systemFault");
		else if (val == 0)
			str = messages.getString("rtt");
		else if (val == 1)
			str = messages.getString("dialup2");
		else if (val == 2)
			str = "T1";
		else if (val == 3)
			str = "Ethernet";
		else if (val == 4)
			str = "T3";
		else if (val == 5)
			str = "FastE";
		else if (val == 6)
			str = "OC-12";
		else if (val == 7)
			str = "GigE";
		else if (val == 8)
			str = "OC-48";
		else if (val == 9)
			str = "10 Gig";
		return(str);
	} // prttxt()
	
	

	/* This routine saves the specific value into the variable of the
	* same name.  There should probably be an easier way to do this.
	*/
	public void save_dbl_values(String sysvar, double sysval) {
		if(sysvar.equals("bw:")) 
			estimate = sysval;
		else if(sysvar.equals("loss:")) { 
			loss = sysval;
			uiServices.setVariable("pub_loss", loss);
		}
		else if(sysvar.equals("avgrtt:")) {
			avgrtt = sysval;
			uiServices.setVariable("pub_avgrtt", avgrtt);
		}
		else if(sysvar.equals("waitsec:")) 
			waitsec = sysval;
		else if(sysvar.equals("timesec:")) 
			timesec = sysval;
		else if(sysvar.equals("order:")) 
			order = sysval;
		else if(sysvar.equals("rwintime:")) 
			rwintime = sysval;
		else if(sysvar.equals("sendtime:")) 
			sendtime = sysval;
		else if(sysvar.equals("cwndtime:")) {
			cwndtime = sysval;
			uiServices.setVariable("pub_cwndtime", cwndtime);
		}
		else if(sysvar.equals("rttsec:")) 
			rttsec = sysval;
		else if(sysvar.equals("rwin:")) 
			rwin = sysval;
		else if(sysvar.equals("swin:")) 
			swin = sysval;
		else if(sysvar.equals("cwin:")) 
			cwin = sysval;
		else if(sysvar.equals("spd:")) 
			spd = sysval;
		else if(sysvar.equals("aspd:")) 
			aspd = sysval;
	} // save_dbl_values()



	public void save_int_values(String sysvar, int sysval) {
		/*  Values saved for interpretation:
		*	SumRTT 
		*	CountRTT
		*	CurrentMSS
		*	Timeouts
		*	PktsRetrans
		*	SACKsRcvd
		*	DupAcksIn
		*	MaxRwinRcvd
		*	MaxRwinSent
		*	Sndbuf
		*	Rcvbuf
		*	DataPktsOut
		*	SndLimTimeRwin
		*	SndLimTimeCwnd
		*	SndLimTimeSender
		*/   
		if(sysvar.equals("MSSSent:")) 
			MSSSent = sysval;
		else if(sysvar.equals("MSSRcvd:")) 
			MSSRcvd = sysval;
		else if(sysvar.equals("ECNEnabled:")) 
			ECNEnabled = sysval;
		else if(sysvar.equals("NagleEnabled:")) 
			NagleEnabled = sysval;
		else if(sysvar.equals("SACKEnabled:")) 
			SACKEnabled = sysval;
		else if(sysvar.equals("TimestampsEnabled:")) 
			TimestampsEnabled = sysval;
		else if(sysvar.equals("WinScaleRcvd:")) 
			WinScaleRcvd = sysval;
		else if(sysvar.equals("WinScaleSent:")) 
			WinScaleSent = sysval;
		else if(sysvar.equals("SumRTT:")) 
			SumRTT = sysval;
		else if(sysvar.equals("CountRTT:")) 
			CountRTT = sysval;
		else if(sysvar.equals("CurMSS:"))
			CurrentMSS = sysval;
		else if(sysvar.equals("Timeouts:")) 
			Timeouts = sysval;
		else if(sysvar.equals("PktsRetrans:")) 
			PktsRetrans = sysval;
		else if(sysvar.equals("SACKsRcvd:")) {
			SACKsRcvd = sysval;
			uiServices.setVariable("pub_SACKsRcvd", SACKsRcvd);
		}
		else if(sysvar.equals("DupAcksIn:")) 
			DupAcksIn = sysval;
		else if(sysvar.equals("MaxRwinRcvd:")) {
			MaxRwinRcvd = sysval;
			uiServices.setVariable("pub_MaxRwinRcvd", MaxRwinRcvd);
		}
		else if(sysvar.equals("MaxRwinSent:")) 
			MaxRwinSent = sysval;
		else if(sysvar.equals("Sndbuf:")) 
			Sndbuf = sysval;
		else if(sysvar.equals("X_Rcvbuf:")) 
			Rcvbuf = sysval;
		else if(sysvar.equals("DataPktsOut:")) 
			DataPktsOut = sysval;
		else if(sysvar.equals("FastRetran:")) 
			FastRetran = sysval;
		else if(sysvar.equals("AckPktsOut:")) 
			AckPktsOut = sysval;
		else if(sysvar.equals("SmoothedRTT:")) 
			SmoothedRTT = sysval;
		else if(sysvar.equals("CurCwnd:")) 
			CurrentCwnd = sysval;
		else if(sysvar.equals("MaxCwnd:")) 
			MaxCwnd = sysval;
		else if(sysvar.equals("SndLimTimeRwin:")) 
			SndLimTimeRwin = sysval;
		else if(sysvar.equals("SndLimTimeCwnd:")) 
			SndLimTimeCwnd = sysval;
		else if(sysvar.equals("SndLimTimeSender:")) 
			SndLimTimeSender = sysval;
		else if(sysvar.equals("DataBytesOut:")) 
			DataBytesOut = sysval;
		else if(sysvar.equals("AckPktsIn:")) 
			AckPktsIn = sysval;
		else if(sysvar.equals("SndLimTransRwin:"))
			SndLimTransRwin = sysval;
		else if(sysvar.equals("SndLimTransCwnd:"))
			SndLimTransCwnd = sysval;
		else if(sysvar.equals("SndLimTransSender:"))
			SndLimTransSender = sysval;
		else if(sysvar.equals("MaxSsthresh:"))
			MaxSsthresh = sysval;
		else if(sysvar.equals("CurRTO:")) {
			CurrentRTO = sysval;
			uiServices.setVariable("pub_CurRTO", CurrentRTO);
		}
		else if(sysvar.equals("MaxRTO:")) {
			uiServices.setVariable("pub_MaxRTO", sysval);
		}
		else if(sysvar.equals("MinRTO:")) {
			uiServices.setVariable("pub_MinRTO", sysval);
		}
		else if(sysvar.equals("MinRTT:")) {
			uiServices.setVariable("pub_MinRTT", sysval);
		}
		else if(sysvar.equals("MaxRTT:")) {
			uiServices.setVariable("pub_MaxRTT", sysval);
		}
		else if(sysvar.equals("CurRwinRcvd:")) {
			uiServices.setVariable("pub_CurRwinRcvd", sysval);
		}		
		else if(sysvar.equals("Timeouts:")) {
			uiServices.setVariable("pub_Timeouts", sysval);
		}	
		else if(sysvar.equals("c2sData:"))
			c2sData = sysval;
		else if(sysvar.equals("c2sAck:"))
			c2sAck = sysval;
		else if(sysvar.equals("s2cData:"))
			s2cData = sysval;
		else if(sysvar.equals("s2cAck:"))
			s2cAck = sysval;
		else if(sysvar.equals("PktsOut:"))
			PktsOut = sysval;
		else if(sysvar.equals("mismatch:")) {
			mismatch = sysval;
			uiServices.setVariable("pub_mismatch", mismatch);
		}
		else if(sysvar.equals("congestion:")) {
			congestion = sysval;
			uiServices.setVariable("pub_congestion", congestion);
		}
		else if(sysvar.equals("bad_cable:")) {
			bad_cable = sysval;
			uiServices.setVariable("pub_Bad_cable", bad_cable);
		}
		else if(sysvar.equals("half_duplex:"))
			half_duplex = sysval;
		else if(sysvar.equals("CongestionSignals:"))
			CongestionSignals = sysval;
		else if(sysvar.equals("RcvWinScale:")) {
			if (RcvWinScale > 15)
				RcvWinScale = 0;
			else
				RcvWinScale = sysval;
		}
	}  // save_int_values()

  private void showStatus(String s) {
    uiServices.updateStatus(s);
  }

  public String getEmailText() {
    return emailText;
  }

  /**
   * Adapter from JTextArea#append to UiServices#appendString.
   */
  private static class TextOutputAdapter {
    private final int viewId;
    private final UiServices uiServices;

    /**
     * @param viewId UiServices constant to pass to appendString
     */
    public TextOutputAdapter(UiServices uiServices, int viewId) {
      this.viewId = viewId;
      this.uiServices = uiServices;
    }

    public void append(String s) {
      uiServices.appendString(s, viewId);
    }
  }
}
