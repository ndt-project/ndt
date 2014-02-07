/*
Copyright © 2003 University of Chicago.  All rights reserved.
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
import java.io.*;
import java.net.*;
import java.net.Socket;
import java.awt.*;
import java.awt.event.*;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import java.awt.datatransfer.*;
import java.applet.*;
import java.util.*;
import java.text.*;
import java.lang.*;
import java.applet.Applet;

public class Tcpbw100 extends Applet implements ActionListener
{
	TextArea results, diagnosis, statistics, reports;
	String inresult, outresult, errmsg;
	Button startTest;
	Button disMiss, disMiss2;
	Button copy, copy2;
	Button deTails;
	Button sTatistics;
	Button mailTo;
	Label lab1, lab2, lab3, lab4;
	boolean Randomize, failed, cancopy;
	URL location;
	clsFrame f, ff;
	String s;
	Frame frame;
	double t;
	int ECNEnabled, NagleEnabled, MSSSent, MSSRcvd;
	int SACKEnabled, TimestampsEnabled, WinScaleRcvd;
	int FastRetran, AckPktsOut, SmoothedRTT, CurrentCwnd, MaxCwnd;
	int SndLimTimeRwin, SndLimTimeCwnd, SndLimTimeSender;
	int SndLimTransRwin, SndLimTransCwnd, SndLimTransSender, MaxSsthresh;
	int SumRTT, CountRTT, CurrentMSS, Timeouts, PktsRetrans;
	int SACKsRcvd, DupAcksIn, MaxRwinRcvd, MaxRwinSent;
	int DataPktsOut, Rcvbuf, Sndbuf, AckPktsIn, DataBytesOut;
	int PktsOut, CongestionSignals, RcvWinScale;
	int pkts, lth=8192, CurrentRTO;
	int c2sData, c2sAck, s2cData, s2cAck;
	// added for mailto url
	protected URL targetURL;
	private String TARGET1 = "U";
	private String TARGET2 = "H";
	String emailText;
	double s2cspd, c2sspd;

/*************************************************************************
 * Added by Seth Peery on 2006-08-04
 * Adds public variables to store upstream and downstream speeds in kbps
 */
        public String downstream = "No data";
        public String upstream = "No data";
        public String host = "127.0.0.1";
/************************************************************************/



	int half_duplex, congestion, bad_cable, mismatch;
	double mylink;
	double loss, estimate, avgrtt, spd, waitsec, timesec, rttsec;
	double order, rwintime, sendtime, cwndtime, rwin, swin, cwin;
	double aspd;



	public void init() {
		setLayout(new FlowLayout(FlowLayout.CENTER, 5, 5));
		showStatus("Tcpbw100 ready");
		failed = false ;
		Randomize = false;
		cancopy = false;

/*************************************************************************
 * Changed by Seth Peery on 2006-08-07
 * Added the '3' parameter in the TextArea() constructor call to 
 * enable text wrapping.
*/
		results = new TextArea("TCP/Web100 Network Diagnostic Tool v5.3.5a\n",15,70,3);
/************************************************************************/


		results.setEditable(false);
		add(results);


/*************************************************************************
 * Added by Seth Peery on 2006-08-04
 * Adds our custom message to the applet window 
 */
results.append("\nThis is a modified version of the original NDT applet, incorporating minor changes made by the Virginia Tech eCorridors Program to integrate its output with the Community Broadband Map.\nNOTE: results for dial-up connections may be inaccurate.\n");
/************************************************************************/

		results.append("click START to begin\n");
		startTest = new Button("START");
		startTest.addActionListener(this);
		add("West", startTest);
		Panel mPanel = new Panel();
		sTatistics = new Button("Statistics");
		sTatistics.addActionListener(this);
		sTatistics.setEnabled(false);
		deTails = new Button("More Details...");
		deTails.addActionListener(this);
		deTails.setEnabled(false);
		mailTo = new Button("Report Problem");
		mailTo.addActionListener(this);
		mailTo.setEnabled(false);

/*************************************************************************
* Changed by Seth Peery on 2006-08-04
* Change values here to remove buttons from the applet.
* Note that these lines are relocated from above. 
*/
        //      mPanel.add(mailTo);
                mPanel.add(deTails);
                mPanel.add(sTatistics);
/************************************************************************/

		add("South", mPanel);
	}

	

	public void runtest() {
		diagnose();
		statistics();
		startTest.setEnabled(false);
		deTails.setEnabled(false);
		sTatistics.setEnabled(false);
		mailTo.setEnabled(false);
		
		try {
			dottcp();
		} catch(IOException e) {
			System.out.println (e);
			failed=true;
			errmsg = "Server busy: Please wait 30 seconds for previous test to finish\n";
		}
		
		if (failed) {
			results.append(errmsg);
		}
		
		deTails.setEnabled(true);
		sTatistics.setEnabled(true);
		mailTo.setEnabled(true);
		showStatus("Tcpbw100 done");
		results.append("\nclick START to re-test\n");
		startTest.setEnabled(true);
	}



	public void dottcp() throws IOException {
		Socket ctlSocket = null;
		Socket outSocket = null;
		Socket inSocket = null;
		Socket in2Socket = null;



/*************************************************************************
 * Added by Seth Peery, 2006-05-17
 * Instead of using the getCodeBase().getHost() value for the testing server,
 * which assumes this applet is being served from the web100srv server,
 * use a parameter provided in the APPLET tag.
 * Note that for this to work the applet must be signed because you are
 * potentially accessing a server outside the source domain.
*/
                //String host = getCodeBase().getHost();

		// Previously, we used hardcoded server names; 
		// we have since migrated to APPLET parameter tags.
		//String host = "miranda.ctd.anl.gov";

                host = getParameter("testingServer");
                results.append("Running speed test from host: "+host+"\n");
/************************************************************************/


		int ctlport = 3001,  outport, inport, inlth, bytes;
		int midport = 3004;
		byte buff[] = new byte[8192];
		byte buff2[] = new byte[8192];
		double stop_time, wait2;
		int sbuf, rbuf;
		int i, wait;

		failed = false;
		try {
			ctlSocket = new Socket(host, ctlport);
		} catch (UnknownHostException e) {
			System.err.println("Don't know about host: " + host);
			errmsg = "unknown server\n" ;
			failed = true;
			return;
		} catch (IOException e) {
			System.err.println("Couldn't get the connection to: " + host + " " +ctlport);
			errmsg = "Server process not running: start web100srv process on remote server\n" ;
			failed = true;
			return;
		}

		/* This is part of the server queuing process.  The server will now send
		* a integer value over to the client before testing will begin.  If queuing
		* is enabled, the server will send a positive value.  Zero indicated that
		* testing can begin, and -1 indicates that queuing is disabled and the 
		* user should try again later.
		*/

		InputStream ctlin = ctlSocket.getInputStream();
		OutputStream ctlout = ctlSocket.getOutputStream();

		try {  
			while ((inlth = ctlin.read(buff,0,buff.length)) > 0) {
				String tmpstr3 = new String(buff, 0, inlth);
				wait = Integer.parseInt(tmpstr3);
				System.out.println("wait flag received = " + wait);

				if (wait == 0)
					break;

				if (wait == 9999) {
					errmsg = "Server Busy: Please wait 60 seconds for the current test to finish\n";
					failed = true;
					return;
				}
				
				// Each test should take less than 30 seconds, so tell them 45 sec * numer of 
				// tests in the queue.
				wait = (wait * 45);
				results.append("Another client is currently being served, your test will " +
					"begin within " + wait + " seconds\n");
			}
		} catch (IOException e) {}

		inlth = ctlin.read(buff,0,buff.length); 

		if (inlth <= 0) {  
			System.err.println("control port read failed");
			errmsg = "Server Busy: Please wait 60 seconds for previous test to finish\n" ;
			failed = true;
			return;
		}

		String tmpstr = new String(buff,0,inlth);
		System.out.println("server ports " + tmpstr);
		int k = tmpstr.indexOf(" ");
		outport = Integer.parseInt(tmpstr.substring(0,k));
		inport = Integer.parseInt(tmpstr.substring(k+1));

		f.toBack();
		ff.toBack();

		/* now look for middleboxes (firewalls, NATs, and other boxes that
		* muck with TCP's end-to-end priciples
		*/
		showStatus("Tcpbw100 Middlebox test...");
		// results.append("Trying to open new connection to server for middlebox testing\n");

		try {
			in2Socket = new Socket(host, inport);
			// in2Socket = new Socket(host, midport);
		} catch (UnknownHostException e) {
			System.err.println("Don't know about host: " + host);
			errmsg = "unknown server\n" ;
			failed = true;
			return;
		} catch (IOException e) {
			System.err.println("Couldn't perform middlebox testing to: " + host);
			errmsg = "Server Failed while middlebox testing\n" ;
			failed = true;
			return;
		}
		results.append("Checking for Middleboxes . . . . . . . . . . . . . . . . . .  ");
		statistics.append("Checking for Middleboxes . . . . . . . . . . . . . . . . . .  ");
		emailText = "Checking for Middleboxes . . . . . . . . . . . . . . . . . .  ";

		InputStream srvin2 = in2Socket.getInputStream();
		OutputStream srvout2 = in2Socket.getOutputStream();

		int largewin = 128*1024;

		in2Socket.setSoTimeout(6500);
		bytes = 0;
		t = System.currentTimeMillis();

		try {  
			while ((inlth=srvin2.read(buff,0,buff.length)) > 0) {
   			    bytes += inlth;
			    if ((System.currentTimeMillis() - t) > 5500)
				break;
			}
		} 
		catch (IOException e) {}

		t =  System.currentTimeMillis() - t;
		System.out.println(bytes + " bytes " + 8.0 * bytes/t + " Kb/s " + t/1000 + " secs");
		s2cspd = ((8.0 * bytes) / 1000) / t;

		buff = Double.toString(s2cspd*1000).getBytes();
		String tmpstr4 = new String(buff, 0, buff.length);
		System.out.println("Sending '" + tmpstr4 + "' back to server");
		ctlout.write(buff, 0, buff.length);

		String tmpstr2 = new String(buff2, 0, 512);
		tmpstr2 = "";
		inlth = ctlin.read(buff2, 0, 512);
		tmpstr2 += new String(buff2, 0, inlth);
			System.out.println("read tmpstr2 = '" + tmpstr2 + "' from remote server");
		
		results.append("Done\n");
		statistics.append("Done\n");
		emailText += "Done\n%0A";
		try {
			tmpstr2 += in2Socket.getInetAddress() + ";";
		} catch (SecurityException e) {
			System.err.println("Unable to obtain Servers IP addresses: using " + host);
			errmsg = "getInetAddress() called failed\n" ;
			tmpstr2 += host + ";";
			results.append("Unable to obtain remote IP address\n");
		}

		System.err.println("calling in2Socket.getLocalAddress()");
		try {
			tmpstr2 += in2Socket.getLocalAddress() + ";";
		} catch (SecurityException e) {
			System.err.println("Unable to obtain local IP address: using 127.0.0.1");
			errmsg = "getLocalAddress() call failed\n" ;
			tmpstr2 += "127.0.0.1;";
			// results.append("Unable to obtain local IP address: Using 127.0.0.1 instead\n");
		}

		srvin2.close();
		srvout2.close();
		in2Socket.close();

		inlth = ctlin.read(buff2,0,buff2.length); 
		if (inlth <= 0) {  
			System.err.println("read failed read 'C2S Open-Connection' flag");
			errmsg = "Server failed: 'C2S Open-Connection' flag not received\n" ;
			failed = true;
			return;
		}
		String tmpstr5 = new String(buff2, 0, inlth);
		System.err.println("read string'" + tmpstr5 +"' from server, C2S Open-Connection message");

		////
		//   OUTBOUND
		////
		showStatus("Tcpbw100 outbound test...");
		try {
			outSocket = new Socket(host, outport);
		} catch (UnknownHostException e) {
			System.err.println("Don't know about host: " + host);
			errmsg = "unknown server\n" ;
			failed = true;
			return;
		} catch (IOException e) {
			System.err.println("Couldn't get 2nd connection to: " + host);
			errmsg = "Server Busy: Please wait 15 seconds for previous test to finish\n" ;
			failed = true;
			return;
		}
		results.append("running 10s outbound test (client to server) . . . . . ");
		statistics.append("running 10s outbound test (client to server) . . . . . ");
		emailText += "running 10s outbound test (client to server) . . . . . ";

		OutputStream out = outSocket.getOutputStream();

		// wait here for signal from server application 
		// This signal tells the client to start pumping out data
		inlth = ctlin.read(buff,0,buff.length); 
		if (inlth <= 0) {  
			System.err.println("read failed read 'Start' flag");
			errmsg = "Server failed: 'Start' flag not received\n" ;
			failed = true;
			return;
		}
		tmpstr5 = new String(buff, 0, inlth);
		System.err.println("read string '" + tmpstr5 + "' from server, starting C2S test");

		Random rng = new Random();
		byte c = '0';
		for (i=0; i<lth; i++) {
			if (c == 'z')
			    c = '0';
			buff2[i] = c++;
		}
		System.err.println("Send buffer size =" + i);
		outSocket.setSoTimeout(15000); 
		pkts = 0;
		t = System.currentTimeMillis();
		stop_time = t + 10000; // ten seconds
		do {
			// if (Randomize) rng.nextBytes(buff2);
			out.write(buff2, 0, buff2.length);
			pkts++;
		} while (System.currentTimeMillis() < stop_time);
		
		t =  System.currentTimeMillis() - t;
		out.close();
		outSocket.close();
			System.out.println((8.0 * pkts * lth) / t + " Kb/s outbound");
		c2sspd = ((8.0 * pkts * lth) / 1000) / t;

		String srvresult = new String(buff,0,inlth);
		System.out.println(srvresult + " got " + inlth );
		if (c2sspd < 1.0) {
			results.append(prtdbl(c2sspd*1000) + "Kb/s\n");
			statistics.append(prtdbl(c2sspd*1000) + "Kb/s\n");
			emailText += prtdbl(c2sspd*1000) + "Kb/s\n%0A";
		} 
		else {
			results.append(prtdbl(c2sspd) + "Mb/s\n");
			statistics.append(prtdbl(c2sspd) + "Mb/s\n");
			emailText += prtdbl(c2sspd) + "Mb/s\n%0A";
		}


/*************************************************************************
 * Added by Seth Peery, 2006-08-04
 * Write string representation of upstream speed to public String upstream
 */
                upstream = ""+(prtdbl(c2sspd*1000));
/************************************************************************/





		//// 
		//   INBOUND 
		////
		showStatus("Tcpbw100 inbound test...");

		inlth= ctlin.read(buff2, 0, buff2.length); 
		if (inlth <= 0) {  
			System.err.println("2nd connection failed");
			errmsg = "Server Failed while sending data\n" ;
			failed = true;
			return;
		}
		tmpstr5 = new String(buff2, 0, inlth);
		System.err.println("read string '" + tmpstr5 + "' from server, open new connecion");


		try {
			inSocket = new Socket(host, inport);
		} 
		catch (UnknownHostException e) {
			System.err.println("Don't know about host: " + host);
			errmsg = "unknown server\n" ;
			failed = true;
			return;
		} 
		catch (IOException e) {
			System.err.println("Couldn't get 3rd connection to: " + host);
			errmsg = "Server Failed while receiving data\n" ;
			failed = true;
			return;
		}

		InputStream srvin = inSocket.getInputStream();
		bytes = 0;

		results.append("running 10s inbound test (server to client) . . . . . . ");
		statistics.append("running 10s inbound test (server to client) . . . . . . ");
		emailText += "running 10s inbound test (server to client) . . . . . . ";

		// wait here for signal from server application 
		inlth = ctlin.read(buff,0,buff.length); 
		if (inlth <= 0) {  
			System.err.println("read failed read 'Go' flag");
			errmsg = "Server failed: 'Go' flag not received\n" ;
			failed = true;
			return;
		}
		tmpstr5 = new String(buff, 0, inlth);
		System.err.println("read string '" + tmpstr5 + "' from server, starting next test");

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
		System.out.println(bytes + " bytes " + 8.0 * bytes/t + " Kb/s " + t/1000 + " secs");
		s2cspd = ((8.0 * bytes) / 1000) / t;
		if (s2cspd < 1.0) {
			results.append(prtdbl(s2cspd*1000) + "kb/s\n");
			statistics.append(prtdbl(s2cspd*1000) + "kb/s\n");
			emailText += prtdbl(s2cspd*1000) + "kb/s\n%0A";
		} else {
			results.append(prtdbl(s2cspd) + "Mb/s\n");
			statistics.append(prtdbl(s2cspd) + "Mb/s\n");
			emailText += prtdbl(s2cspd) + "Mb/s\n%0A";
		}


/*************************************************************************
 * Added by Seth Peery, 2006-08-04
 * Write string representation of downstream speed to public String upstream
 */
                downstream = ""+(prtdbl(s2cspd*1000));
/************************************************************************/


		srvin.close();
		inSocket.close();

		buff = Double.toString(s2cspd*1000).getBytes();
		tmpstr4 = new String(buff, 0, buff.length);
		System.out.println("Sending '" + tmpstr4 + "' back to server");
		ctlout.write(buff, 0, buff.length);

		/* get web100 variables from server */
		tmpstr = "";
		i = 0;

		// Try setting a 5 second timer here to break out if the read fails.
		ctlSocket.setSoTimeout(5000);
		try {  
			for (;;) {
				inlth = ctlin.read(buff, 0, buff.length);
				//results.append("Read " + inlth + " bytes from ctl socket\n");
				if (inlth < 0) {
					//results.append("Finished reading data from ctl socket\n");
					break;
				}
				tmpstr += new String(buff, 0, inlth);
				i++;
			}
		} catch (IOException e) {}

		if (i == 0)
		    results.append("Warning! Client time-out while reading data, possible duplex mismatch exists\n");
		
		System.err.println("Calling InetAddress.getLocalHost() twice");
		try {
			diagnosis.append("Client: " + InetAddress.getLocalHost() + "\n");
		} catch (SecurityException e) {
			diagnosis.append("Client: 127.0.0.1\n");
			results.append("Unable to obtain local IP address\n");
			System.err.println("Unable to obtain local IP address: using 127.0.0.1");
		}
		
		try {
			emailText += "Client: " + InetAddress.getLocalHost() + "\n%0A";
		} catch (SecurityException e) {
			emailText += "Client: 127.0.0.1\n%0A";
		}

		
		/// XXX TODO -- this code doesn't do anything  dmr
	// 	try {  
	// 		for (;;) {
	// 			inlth = ctlin.read(buff, 0, buff.length);
	// 			//results.append("Read " + inlth + " bytes from ctl socket\n");
	// 			if (inlth < 0) {
	// 				// System.err.println("Finished reading calculated data");
	// 				break;
	// 			}
	// 			tmpstr += new String(buff, 0, inlth);
	// 		}
	// 	} catch (IOException e) {}

		ctlin.close();
		ctlout.close();
		ctlSocket.close();

		// System.err.println("tmpstr2 is '" + tmpstr2 + "'\n");
		// System.err.println("tmpstr is '" + tmpstr + "'\n");
		testResults(tmpstr);
		middleboxResults(tmpstr2);
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
				// results.append(sysvar + " " + strval + "\n");
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

		// Grap some client details from the applet environment
		osName = System.getProperty("os.name");
		osArch = System.getProperty("os.arch");
		osVer = System.getProperty("os.version");
		javaVer = System.getProperty("java.version");
		javaVendor = System.getProperty("java.vendor");

		if (osArch.startsWith("x86") == true) {
			client = "PC";
		}
		else {
			client = "Workstation";
		}

		// Calculate some variables and determine path conditions
		// Note: calculations now done in server and the results are shipped
		//    back to the client for printing.

		if (CountRTT>0) {
			// Now write some messages to the screen
			if (c2sData < 3) {
				if (c2sData < 0) {
					results.append("Server unable to determine bottleneck link type.\n");
					emailText += "Server unable to determine bottleneck link type.\n%0A";
				} 
				else {
					results.append("Your " + client + " is connected to a ");
					emailText += "Your " + client + " is connected to a ";
					if (c2sData == 1) {
						results.append("Dial-up Modem\n");
						emailText += "Dial-up Modem\n%0A";
						mylink = .064;
					} 
					else {
						results.append("Cable/DSL modem\n");
						emailText += "Cable/DSL modem\n%0A";
						mylink = 3;
					}
				}
			} 
			else {
				results.append("The slowest link in the end-to-end path is a ");
				emailText += "The slowest link in the end-to-end path is a ";
				if (c2sData == 3) {
					results.append("10 Mbps Ethernet subnet\n");
					emailText += "10 Mbps Ethernet subnet\n%0A";
					mylink = 10;
				} 
				else if (c2sData == 4) {
					results.append("45 Mbps T3/DS3 subnet\n");
					emailText += "45 Mbps T3/DS3 subnet\n%0A";
					mylink = 45;
				} 
				else if (c2sData == 5) {
					results.append("100 Mbps ");
					emailText += "100 Mbps ";
					mylink = 100;
					if (half_duplex == 0) {
						results.append("Full duplex Fast Ethernet subnet\n");
						emailText += "Full duplex Fast Ethernet subnet\n%0A";
					} 
					else {
						results.append("Half duplex Fast Ethernet subnet\n");
						emailText += "Half duplex Fast Ethernet subnet\n%0A";
					}
				} 
				else if (c2sData == 6) {
					results.append("a 622 Mbps OC-12 subnet\n");
					emailText += "a 622 Mbps OC-12 subnet\n%0A";
					mylink = 622;
				} 
				else if (c2sData == 7) {
					results.append("1.0 Gbps Gigabit Ethernet subnet\n");
					emailText += "1.0 Gbps Gigabit Ethernet subnet\n%0A";
					mylink = 1000;
				} 
				else if (c2sData == 8) {
					results.append("2.4 Gbps OC-48 subnet\n");
					emailText += "2.4 Gbps OC-48 subnet\n%0A";
					mylink = 2400;
				} 
				else if (c2sData == 9) {
					results.append("10 Gbps 10 Gigabit Ethernet/OC-192 subnet\n");
					emailText += "10 Gbps 10 Gigabit Ethernet/OC-192 subnet\n%0A";
					mylink = 10000;
				}
			}

			if (mismatch == 1) {
				results.append("Warning: Old Duplex mismatch condition detected: ");
				emailText += "Warning: Old Duplex mismatch condition detected: ";
			}
 			else if (mismatch == 2) {
				results.append("Alarm: Duplex Mismatch condition detected Switch=Full and Host=half\n ");
				emailText += "Alarm: Duplex Mismatch condition detected Switch=Full and Host=half\n%0A ";
			}
 			else if (mismatch == 4) {
				results.append("Alarm: Possible Duplex Mismatch condition detected Switch=Full and Host=half\n ");
				emailText += "Alarm: Possible Duplex Mismatch condition detected Switch=Full and Host=half\n%0A ";
			}
 			else if (mismatch == 3) {
				results.append("Alarm: Duplex Mismatch condition detected Switch=half and Host=full\n ");
				emailText += "Alarm: Duplex Mismatch condition detected Switch=half and Host=full\n%0A ";
			}
 			else if (mismatch == 5) {
				results.append("Alarm: Possible Duplex Mismatch condition detected Switch=half and Host=full\n ");
				emailText += "Alarm: Possible Duplex Mismatch condition detected Switch=half and Host=full\n%0A ";
			}
 			else if (mismatch == 7) {
				results.append("Warning: Possible Duplex Mismatch condition detected Switch=half and Host=full\n ");
				emailText += "Warning: Possible Duplex Mismatch condition detected Switch=half and Host=full\n%0A ";
			}

			if (mismatch == 0) {
			    if (bad_cable == 1) {
				results.append("Alarm: Excessive errors, check network cable(s).\n");
				emailText += "Alarm: Excessive errors, check network cable(s).\n%0A";
			    }
			    if (congestion == 1) {
				results.append("Information: Other network traffic is congesting the link\n");
				emailText += "Information: Other network traffic is congesting the link\n%0A";
			    }
			    if (((2*rwin)/rttsec) < mylink) {
			        j = (float)((mylink * avgrtt) * 1000) / 8 / 1024;
			        if (j > (float)MaxRwinRcvd) {
				    results.append("Information: The receive buffer should be " +
				    prtdbl(j) + " Kbytes to maximize throughput\n");
				    emailText += "Information: The receive buffer should be " +
				    prtdbl(j) + " Kbytes to maximize throughput\n";
			        }
			    }
			}

			statistics.append("\n\t------  Client System Details  ------\n");
			statistics.append("OS data: Name = " + osName + ", Architecture = " + osArch);
			statistics.append(", Version = " + osVer + "\n");
			statistics.append("Java data: Vendor = " + javaVendor + ", Version = " + javaVer + "\n");
			// statistics.append(" java.class.version=" + System.getProperty("java.class.version") + "\n");

			statistics.append("\n\t------  Web100 Detailed Analysis  ------\n");
			if (c2sData == -2)
				statistics.append("Insufficent data collected to determine link type.\n");
			else if (c2sData == -1)
				statistics.append("Interprocess communications failed, unknown link type.\n");
			else if (c2sData == 0)
				statistics.append("Link detection algorithm failed due to excessive Round Trip Times.\n");
			else if (c2sData == 1)
				statistics.append("Dial-up modem link found.\n");
			else if (c2sData == 2)
				statistics.append("Cable modem/DSL/T1 link found.\n");
			else if (c2sData == 3)
				statistics.append("10 Mbps Ethernet link found.\n");
			else if (c2sData == 4)
				statistics.append("45 Mbps T3/DS3 link found.\n");
			else if (c2sData == 5)
				statistics.append("100 Mbps FastEthernet link found.\n");
			else if (c2sData == 6)
				statistics.append("622 Mbps OC-12 link found.\n");
			else if (c2sData == 7)
				statistics.append("1 Gbps GigabitEthernet link found.\n");
			else if (c2sData == 8)
				statistics.append("2.4 Gbps OC-48 link found.\n");
			else if (c2sData == 9)
				statistics.append("10 Gbps 10 GigEthernet/OC-192 link found.\n");

			if (half_duplex == 0)
				statistics.append("Link set to Full Duplex mode\n");
			else
				statistics.append("Link set to Half Duplex mode\n");

			if (congestion == 0)
				statistics.append("No network congestion discovered.\n");
			else
				statistics.append("Information: throughput is limited by other network traffic.\n");

			if (bad_cable == 0)
				statistics.append("Good network cable(s) found\n");
			else
				statistics.append("Warning: excessive network errors, check network cable(s)\n");

			if (mismatch == 0)
				statistics.append("Normal duplex operation found.\n");
			else if (mismatch == 1) {
				statistics.append("Warning: Old Duplex mismatch condition detected: ");
				emailText += "Warning: Old Duplex mismatch condition detected: ";
			}
 			else if (mismatch == 2) {
				statistics.append("Alarm: Duplex Mismatch condition detected Switch=Full and Host=half\n ");
				emailText += "Alarm: Duplex Mismatch condition detected Switch=Full and Host=half\n%0A ";
			}
 			else if (mismatch == 3) {
				statistics.append("Alarm: Duplex Mismatch condition detected Switch=half and Host=full\n ");
				emailText += "Alarm: Duplex Mismatch condition detected Switch=half and Host=full\n%0A ";
			}

			statistics.append("\nWeb100 reports the Round trip time = " + prtdbl(avgrtt) + " msec; ");
			emailText += "\n%0AWeb100 reports the Round trip time = " + prtdbl(avgrtt)	+ " msec; ";

			statistics.append("the Packet size = " + CurrentMSS + " Bytes; and \n");
			emailText += "the Packet size = " + CurrentMSS + " Bytes; and \n%0A";
			if (PktsRetrans > 0) {
				statistics.append("There were " + PktsRetrans + " packets retransmitted");
				statistics.append(", " + DupAcksIn + " duplicate acks received");
				statistics.append(", and " + SACKsRcvd + " SACK blocks received\n");
				emailText += "There were " + PktsRetrans + " packets retransmitted";
				emailText += ", " + DupAcksIn + " duplicate acks received";
				emailText += ", and " + SACKsRcvd + " SACK blocks received\n%0A";
				if (Timeouts > 0) {
					statistics.append("The connection stalled " + Timeouts + " times due to packet loss\n");
				}
	
				statistics.append("The connection was idle " + prtdbl(waitsec) + " seconds (" + 
					prtdbl((waitsec/timesec)*100) + "%) of the time\n");
				emailText += "The connection stalled " + Timeouts + " times due to packet loss\n%0A";
				emailText += "The connection was idle " + prtdbl(waitsec) + " seconds (" + 
				prtdbl((waitsec/timesec)*100) + "%) of the time\n%0A";
			} 
			else if (DupAcksIn > 0) {
				statistics.append("No packet loss - ");
				statistics.append("but packets arrived out-of-order " + prtdbl(order*100) +
				"% of the time\n");
				emailText += "No packet loss - ";
				emailText += "but packets arrived out-of-order " + prtdbl(order*100) +
				"% of the time\n%0A";
			} 
			else {
				statistics.append("No packet loss was observed.\n");
				emailText += "No packet loss was observed.\n%0A";
			}

			if (rwintime > .015) {
				statistics.append("This connection is receiver limited " + prtdbl(rwintime*100) +
				"% of the time.\n");
				emailText += "This connection is receiver limited " + prtdbl(rwintime*100) +
				"% of the time.\n%0A";

			// I think there is a bug here, it sometimes tells you to increase the buffer
			// size, but the new size is smaller than the current.

				if (((2*rwin)/rttsec) < mylink) {
					statistics.append("  Increasing the the client's receive buffer (" + prtdbl(MaxRwinRcvd/1024) +
					" KB) will improve performance\n");
				}
			}
			if (sendtime > .015) {
				statistics.append("This connection is sender limited " + prtdbl(sendtime*100) +
				"% of the time.\n");
				emailText += "This connection is sender limited " + prtdbl(sendtime*100) +
				"% of the time.\n%0A";
				if ((2*(swin/rttsec)) < mylink) {
					statistics.append("  Increasing the NDT server's send buffer (" + prtdbl(Sndbuf/2048) +
					" KB) will improve performance\n");
				}
			}
			if (cwndtime > .005) {
				statistics.append("This connection is network limited " + prtdbl(cwndtime*100) + "% of the time.\n");
				emailText += "This connection is network limited " + prtdbl(cwndtime*100) + "% of the time.\n%0A";
				// if (cwndtime > .15)
				//	statistics.append("  Contact your local network administrator to report a network problem\n");
				// if (order > .15)
				//	statistics.append("  Contact your local network admin and report excessive packet reordering\n");
			}
			if ((spd < 4) && (loss > .01)) {
				statistics.append("Excessive packet loss is impacting your performance, check the ");
				statistics.append("auto-negotiate function on your local PC and network switch\n");
			}
			
			statistics.append("\nWeb100 reports TCP negotiated the optional Performance Settings to: \n");
			statistics.append("RFC 2018 Selective Acknowledgment: ");
			if(SACKEnabled == Zero)
				statistics.append ("OFF\n");
			else
				statistics.append ("ON\n");

			statistics.append("RFC 896 Nagle Algorithm: ");
			if(NagleEnabled == Zero)
				statistics.append ("OFF\n");
			else
				statistics.append ("ON\n");

			statistics.append("RFC 3168 Explicit Congestion Notification: ");
			if(ECNEnabled == Zero)
				statistics.append ("OFF\n");
			else
				statistics.append ("ON\n");

			statistics.append("RFC 1323 Time Stamping: ");
			if(TimestampsEnabled == 0)
				statistics.append ("OFF\n");
			else
				statistics.append ("ON\n");

			statistics.append("RFC 1323 Window Scaling: ");
			if (MaxRwinRcvd < 65535)
			    WinScaleRcvd = 0;
			if((WinScaleRcvd == 0) || (WinScaleRcvd > 20))
				statistics.append ("OFF\n");
			else
				statistics.append ("ON\n");
			diagnosis.append("\n");


			// diagnosis.append("Checking for mismatch condition\n\t(cwndtime > .3) " +
			//	"[" + prtdbl(cwndtime) + ">.3], (MaxSsthresh > 0) [" + MaxSsthresh +
			//	">0],\n\t (PktsRetrans/sec > 2) [" + prtdbl(PktsRetrans/timesec) + ">2], " +
			//	"(estimate > 2) [" + prtdbl(estimate) + ">2]\n");

			diagnosis.append("Checking for mismatch on uplink\n\t(speed > 50 [" +
				prtdbl(spd) + ">50], (xmitspeed < 5) [" + prtdbl(c2sspd) +
				"<5]\n\t(rwintime > .9) [" + prtdbl(rwintime) + ">.9], (loss < .01) [" +
				prtdbl(loss) + "<.01]\n");

			diagnosis.append("Checking for excessive errors condition\n\t" +
				"(loss/sec > .15) [" + prtdbl(loss/timesec) + ">.15], " +
				"(cwndtime > .6) [" + prtdbl(cwndtime) + ">.6], \n\t" +
				"(loss < .01) [" + prtdbl(loss) + "<.01], " +
				"(MaxSsthresh > 0) [" + MaxSsthresh + ">0]\n");

			diagnosis.append("Checking for 10 Mbps link\n\t(speed < 9.5) [" +
				prtdbl(spd) + "<9.5], (speed > 3.0) [" + prtdbl(spd) + ">3.0]\n\t" +
				"(xmitspeed < 9.5) [" + prtdbl(c2sspd) + "<9.5] " +
				"(loss < .01) [" + prtdbl(loss) + "<.01], (mylink > 0) [" + mylink + ">0]\n");

			diagnosis.append("Checking for Wireless link\n\t(sendtime = 0) [" +
				prtdbl(sendtime) + "=0], (speed < 5) [" + prtdbl(spd) + "<5]\n\t" +
				"(Estimate > 50 [" + prtdbl(estimate) + ">50], (Rwintime > 90) [" + 
				prtdbl(rwintime) + ">.90]\n\t (RwinTrans/CwndTrans = 1) [" + SndLimTransRwin +
				"/" + SndLimTransCwnd + "=1], (mylink > 0) [" + mylink + ">0]\n");

			diagnosis.append("Checking for DSL/Cable Modem link\n\t(speed < 2) [" +
				prtdbl(spd) + "<2], " +
				"(SndLimTransSender = 0) [" + SndLimTransSender + "=0]\n\t " +
				"(SendTime = 0) [" + sendtime + "=0], (mylink > 0) [" + mylink + ">0]\n");

			diagnosis.append("Checking for half-duplex condition\n\t(rwintime > .95) [" +
				prtdbl(rwintime) + ">.95], (RwinTrans/sec > 30) [" +
				prtdbl(SndLimTransRwin/timesec) + ">30],\n\t (SenderTrans/sec > 30) [" +
				prtdbl(SndLimTransSender/timesec) + ">30], OR (mylink <= 10) [" + mylink +	"<=10]\n");

			diagnosis.append("Checking for congestion\n\t(cwndtime > .02) [" +
				prtdbl(cwndtime) + ">.02], (mismatch = 0) [" + mismatch + "=0]\n\t" +
				"(MaxSsthresh > 0) [" + MaxSsthresh + ">0]\n");

			diagnosis.append("\nestimate = " + prtdbl(estimate) + " based on packet size = "
				+ (CurrentMSS*8/1024) + "Kbits, RTT = " + prtdbl(avgrtt) + "msec, " + "and loss = " + loss + "\n");

			diagnosis.append("The theoretical network limit is " + prtdbl(estimate) + " Mbps\n");
			emailText += "The theoretical network limit is " + prtdbl(estimate) + " Mbps\n%0A";

			diagnosis.append("The NDT server has a " + prtdbl(Sndbuf/2048) + 
				" KByte buffer which limits the throughput to " + prtdbl(swin/rttsec) + " Mbps\n");
			emailText += "The NDT server has a " + prtdbl(Sndbuf/2048) + 
				" KByte buffer which limits the throughput to " + prtdbl(swin/rttsec) + " Mbps\n%0A";

			diagnosis.append("Your PC/Workstation has a " + prtdbl(MaxRwinRcvd/1024) +
				" KByte buffer which limits the throughput to " + prtdbl(rwin/rttsec) + " Mbps\n");
			emailText += "Your PC/Workstation has a " + prtdbl(MaxRwinRcvd/1024) +
				" KByte buffer which limits the throughput to " + prtdbl(rwin/rttsec) + " Mbps\n%0A";

			diagnosis.append("The network based flow control limits the throughput to " +	prtdbl(cwin/rttsec) + " Mbps\n");
			emailText += "The network based flow control limits the throughput to " +	prtdbl(cwin/rttsec) + " Mbps\n%0A";

			diagnosis.append("\nClient Data reports link is '" + prttxt(c2sData) +
				"', Client Acks report link is '" + prttxt(c2sAck) + "'\n" + 
				"Server Data reports link is '" + prttxt(s2cData) +
				"', Server Acks report link is '" + prttxt(s2cAck) + "'\n");
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
			statistics.append("Packet size is preserved End-to-End\n");
		else
			statistics.append("Information: Network Middlebox is modifying MSS variable\n");

		// if ((winsrecv == -1) && (winssent == -1))
		//     statistics.append("Window scaling option is preserved End-to-End\n");
		// else
		//     statistics.append("Information: Network Middlebox is modifying Window scaling option\n");

		if (ssip.equals(csip)) {
			statistics.append("Server IP addresses are preserved End-to-End\n");
		}
		else {
			statistics.append("Information: Network Address Translation (NAT) box is " +
			"modifying the Server's IP address\n");
			statistics.append("\tServer says [" + ssip + "] but Client says [" + csip + "]\n");
		}

		if (ccip.equals("127.0.0.1")) {
			statistics.append("Client IP address not found.  For IE users, modify the Java parameters\n");
			statistics.append("\tclick Tools - Internet Options - Security - Custom Level, scroll down to\n");
			statistics.append("\tMicrosoft VM - Java permissions and click Custom, click Java Custom Settings\n");
			statistics.append("\tEdit Permissions - Access to all Network Addresses, click Eanble and save changes\n");
		}
		else {
			if (scip.equals(ccip))
				statistics.append("Client IP addresses are preserved End-to-End\n");
			else {
				statistics.append("Information: Network Address Translation (NAT) box is modifying the Client's IP address\n");
				statistics.append("\tServer says [" + scip + "] but Client says [" + ccip + "]\n");
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
			str = "System Fault";
		else if (val == 0)
			str = "RTT";
		else if (val == 1)
			str = "Dial-up";
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
		else if(sysvar.equals("loss:")) 
			loss = sysval;
		else if(sysvar.equals("avgrtt:")) 
			avgrtt = sysval;
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
		else if(sysvar.equals("cwndtime:")) 
			cwndtime = sysval;
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
		else if(sysvar.equals("SACKsRcvd:")) 
			SACKsRcvd = sysval;
		else if(sysvar.equals("DupAcksIn:")) 
			DupAcksIn = sysval;
		else if(sysvar.equals("MaxRwinRcvd:")) 
			MaxRwinRcvd = sysval;
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
		else if(sysvar.equals("CurRTO:"))
			CurrentRTO = sysval;
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
		else if(sysvar.equals("mismatch:"))
			mismatch = sysval;
		else if(sysvar.equals("congestion:"))
			congestion = sysval;
		else if(sysvar.equals("bad_cable:"))
			bad_cable = sysval;
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


	public void diagnose() {
		showStatus("Get WEB100 Variables");
		
		if (f == null) {
			f = new clsFrame();
		}
		f.setTitle("Web100 Variables");
		Panel buttons = new Panel();
		f.add("South", buttons);
		
		disMiss = new Button("Close");
		disMiss.addActionListener(this);
		
		copy = new Button("Copy");
		copy.addActionListener(this);
		
		diagnosis = new TextArea("WEB100 Kernel Variables:\n", 15,30);
		diagnosis.setEditable(true);
		disMiss.setEnabled(true);
		copy.setEnabled(cancopy);
		
		buttons.add("West", disMiss);
		buttons.add("East", copy);
		f.add(diagnosis);
		f.pack();
	}  // diagnose()
	
	

	public void statistics() {
		showStatus("Print Detailed Statistics");

		if (ff == null) {
			ff = new clsFrame();
		}
		ff.setTitle("Detailed Statistics");
		Panel buttons = new Panel();
		ff.add("South", buttons);
		
		disMiss2 = new Button("Close");
		disMiss2.addActionListener(this);
		
		copy2 = new Button("Copy");
		copy2.addActionListener(this);
		
		statistics = new TextArea("WEB100 Enabled Statistics:\n", 25,70);
		statistics.setEditable(true);
		disMiss2.setEnabled(true);
		copy2.setEnabled(cancopy);
		
		buttons.add("West", disMiss2);
		buttons.add("East", copy2);
		ff.add(statistics);
		ff.pack();
	}  // statistics()



	public void actionPerformed(ActionEvent event) {
		Object source = event.getSource();
		// System.err.println("Processing WINDOW event #" +event.getID());
		// System.err.println("Processing event " + source);

		if (source == startTest) {
			if(f != null) {
				f.toBack();
				f.dispose();
				f = null;
			}
			
			if(ff != null) {
				ff.toBack();
				ff.dispose();
				ff = null;
			}
			runtest();
		}

		else if (source == deTails) {
			deTails.setEnabled(false);
			f.setResizable(true);
			f.show();
			deTails.setEnabled(true);
		}

		else if (source == disMiss) {
			f.toBack();
			f.dispose();
		}

		else if (source == disMiss2) {
			ff.toBack();
			ff.dispose();
		}

		else if (source == copy) {
			try {
				Clipboard clipbd = getToolkit().getSystemClipboard();
				cancopy = true;
				String s = diagnosis.getText();
				StringSelection ss = new StringSelection(s);
				clipbd.setContents(ss, ss);
				diagnosis.selectAll();
			} catch (SecurityException e) {
				cancopy = false;
			}
		}

		else if (source == copy2) {
			Clipboard clipbd = getToolkit().getSystemClipboard();
			String s = statistics.getText();
			StringSelection ss = new StringSelection(s);
			clipbd.setContents(ss, ss);
			statistics.selectAll();
		}

		else if (source == sTatistics) {
			sTatistics.setEnabled(false);
			ff.setResizable(true);
			ff.show();
			sTatistics.setEnabled(true);
		}

		else if (source == mailTo) {
			int i;
			char key;
			String to[], from[], comments[];
			String Name, Host;

			mailTo.setEnabled(false);
			// envoke mailto: function
			showStatus("Tcpbw100 Invoking Mailto function...");
			
			results.append("Generating Trouble Report:  This report will be" +
			" emailed to the person you specify\n");
			try {
				if ((Name = getParameter(TARGET1)) == null) {
					throw new IllegalArgumentException("U parameter Required:");
				}
				if ((Host = getParameter(TARGET2)) == null) {
					throw new IllegalArgumentException("H parameter Required:");
				}

				String theURL = "mailto:" + Name + "@" + Host;
				String subject = getParameter("subject");
				
				if (subject == null) {
					subject = "Trouble Report from NDT on " + getCodeBase().getHost();
				}
				theURL += "?subject=" + subject;
				theURL += "&body=Comments:%0A%0A" + emailText + " End Of Email Message\n%0A";
				// System.out.println("Message body is '" + emailText + "'\n");
				targetURL = new URL(theURL);

			} catch (MalformedURLException rsi) {
				throw new IllegalArgumentException("Can't create mailto: URL" + rsi.getMessage());
			}

			getAppletContext().showDocument(targetURL);
		}
	}  // actionPerformed()


	public class clsFrame extends Frame {
		public clsFrame() {
			addWindowListener(new WindowAdapter() {
				public void windowClosing(WindowEvent event) {
					// System.err.println("Handling window closing event");
					dispose();
				}
			});
			// System.err.println("Extended Frame class - RAC9/15/03");
		}
	} // class: clsFrame

} // class: Tcpbw100
