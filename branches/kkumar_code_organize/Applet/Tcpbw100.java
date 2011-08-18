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
/***
 * 
 * Naming convention used: Hungarian, with the following details
 * _VarName: Class variable (instead of c_VarName for class variables to reduce length)
 * iVarName: Integer variable 
 * sVarName: String variable
 * bVarName: boolean variable
 * dVarName: double varibale
 * _iaVarName: Integer "Array"  variable
 * ...and some other self descriptive examples are..
 * _rscBundleMessages : class scoped ResourceBundle Variable called "Messages"
 * _cmboboxIpV6 : Class scoped combo-box variable to indicate IpV6 choice..
 * 
 * Some variables which were called "pub_xxx" are declared to have "accessor" methods for use by
 * other clients. I have left this untouched. These are class scope variables. Though the type is
 * not evident from the declaration immediately, the "getter/setter" methods for them will immediately
 * indicate their types
 * 
 */
import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Cursor;
import java.awt.Dimension;
import java.awt.Panel;
import java.awt.datatransfer.Clipboard;
import java.awt.datatransfer.StringSelection;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.MalformedURLException;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.net.URL;
import java.net.UnknownHostException;
import java.security.AccessController;
import java.security.PrivilegedAction;
import java.util.Date;
import java.util.Locale;
import java.util.ResourceBundle;
import java.util.StringTokenizer;

import javax.swing.BorderFactory;
import javax.swing.BoxLayout;
import javax.swing.JApplet;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JSpinner;
import javax.swing.JTextArea;
import javax.swing.SpinnerNumberModel;


public class Tcpbw100 extends JApplet implements ActionListener
{
	/**
	 * 
	 */
	private static final long serialVersionUID = -7736502859444730371L;
	/**
	 * 
	 */

	JTextArea _txtDiagnosis, _txtStatistics;
	ResultsTextPane _resultsTxtPane;
	//String inresult, outresult; 
	String _sErrMsg;
	JButton _buttonStartTest;
	//TODO: Use just one button for dismiss and copy
	JButton _buttonDismiss, _buttonStatsDismiss; 
	JButton _buttonCopy, _buttonStatsCopy;
	JButton _buttonDetails;
	JButton _buttonStatistics;
	JButton _buttonMailTo;
	JButton _buttonOptions;
	JCheckBox _chkboxDefaultTest, _chkboxPreferIPv6;
	JSpinner _spinnerTestCount = new JSpinner();
	String[] _saDelays = { "immediate", "1min","5mins","10mins","30mins","2hours","12hours","1day" };
	JComboBox _cmboboxDelay;

	boolean _bRandomize, _bFailed, _bCanCopy;
	//URL location; //unused, hence commenting out
	NewFrame _frameWeb100Vars, _frameDetailedStats, _frameOptions;
	//String s; Unused, commenting out
	double _dTime;
	int _iECNEnabled, _iNagleEnabled, MSSSent, MSSRcvd;
	int _iSACKEnabled, _iTimestampsEnabled, _iWinScaleRcvd, _iWinScaleSent;
	int _iFastRetran, _iAckPktsOut, _iSmoothedRTT, _iCurrentCwnd, _iMaxCwnd;
	int _iSndLimTimeRwin, _iSndLimTimeCwnd, _iSndLimTimeSender;
	int _iSndLimTransRwin, _iSndLimTransCwnd, _iSndLimTransSender, _iMaxSsthresh;
	int _iSumRTT, _iCountRTT, _iCurrentMSS, _iTimeouts, _iPktsRetrans;
	int _iSACKsRcvd, _iDupAcksIn, _iMaxRwinRcvd, _iMaxRwinSent;
	int _iDataPktsOut, _iRcvbuf, _iSndbuf, _iAckPktsIn, _iDataBytesOut;
	int _iPktsOut, _iCongestionSignals, _iRcvWinScale;
	//what length is 8192? TODO
	int _iPkts, _iLength=8192, _iCurrentRTO; 
	int _iC2sData, _iC2sAck, _iS2cData, _iS2cAck;
	// added for mailto url
	protected URL _targetURL;

	String _sEmailText;
	double _dS2cspd, _dC2sspd, _dSc2sspd, _dSs2cspd;
	int _iSsndqueue;
	double _dSbytes;

	/*************************************************************************
	 * JavaScript access API extension
	 * Added by Seth Peery and Gregory Wilson, Virginia Tech
	 * October 28, 2009
	 * This section adds classwide variables, written to at runtime, 
	 * which are then exposed by public accessor methods that can be called
	 * from web applications using NDT as a back-end.  
	 */

    //These variables are accessed by the setter/getter methods. While they do not follow naming convention,
	//  are left this way
	//  pub_c2sspd is assigned the value of _dC2sspd (declared above). the pub_xxx version seems to be used 
	//for making public to javascript. No other details known
	private double pub_c2sspd = 0.0;
	private double pub_s2cspd = 0.0;
	private int pub_CurRwinRcvd = 0; // source variable does not exist
	private int pub_MaxRwinRcvd = 0;
	private int  pub_MinRTT = 0; // source variable does not exist
	private int  pub_MaxRTT = 0; // source variable does not exist
	private double pub_loss = 0.0;
	private double pub_avgrtt = 0.0;
	//TODO:Getter/setter commented out for both below. Why?
	private int pub_MinRTO = 0; // source variable does not exist 
	private int pub_MaxRTO = 0; // source variable does not exist
	private int pub_CurRTO = 0;
	// private String pub_CWNDpeaks = ""; // source variable does not exist
	private int pub_SACKsRcvd = 0;
	private String pub_osVer = "unknown"; 
	private String pub_javaVer = "unknown";
	private String pub_host = "unknown";
	private String pub_osName = "unknown"; 
	private String pub_osArch = "unknown";
	private int pub_mismatch = 0;
	private int pub_Bad_cable = 0;
	private int pub_congestion = 0;
	private double pub_cwndtime = 0.0;
	private double pub_pctRcvrLimited = 0.0;
	private String pub_AccessTech = "unknown";
	private String pub_natBox = "unknown";
	private int pub_DupAcksOut = 0;
	private Date pub_TimeStamp;
	private String pub_isReady = new String("no");
	private String pub_clientIP = "unknown";
	private int pub_jitter = 0; //unused. TODO: find out use
	private int pub_Timeouts = 0;
	private String pub_errmsg = "Test not run.";
	private String pub_diagnosis = "Test not run.";
	private String pub_statistics = "Test not run.";
	private String pub_status = "notStarted";
	private double pub_time = 0.0;
	private int pub_bytes = 0;
	private String _sIsAutoRun;
	private String _sUserAgent = null;

	/**
	 * Accessor methods for public variables
	 **/

	public String get_c2sspd()
	{
		// Expressed as MiB using base 10
		return Double.toString((pub_c2sspd));  
	}

	public String get_s2cspd()
	{
		// Expressed as MiB using base 10
		return Double.toString(pub_s2cspd);
	}

	public String get_CurRwinRcvd()
	{
		return Integer.toString(pub_CurRwinRcvd);
	}

	public String get_MaxRwinRcvd()
	{
		return Integer.toString(pub_MaxRwinRcvd);
	}

	public String get_Ping()
	{
		return Integer.toString(pub_MinRTT);
	} 	

	public String get_MaxRTT()
	{
		return Integer.toString(pub_MaxRTT);
	}  

	public String get_loss()
	{
		return Double.toString(pub_loss);
	}

	public String get_avgrtt()
	{
		return Double.toString(pub_avgrtt); 
	}

	/* public String get_MinRTO()
  {
    return pub_MinRTO;
  }*/

	/* public String get_MaxRTO()
  {
    return pub_MaxRTO;
  }*/

	public String get_CurRTO()
	{
		return Integer.toString(pub_CurRTO);
	}

	/*
  public String get_CWNDpeaks()
  {
    return pub_CWNDpeaks;
  } */

	public String get_SACKsRcvd()
	{
		return Integer.toString(pub_SACKsRcvd);
	}  

	public String get_osVer()
	{
		return pub_osVer;
	}

	public String get_javaVer()
	{
		return pub_javaVer;
	}

	public String get_host()
	{
		return pub_host;
	}

	public String get_osName()
	{
		return pub_osName;
	}

	public String get_osArch()
	{
		return pub_osArch;
	}

	public String get_mismatch()
	{
		String result;
		if (pub_mismatch==0) {
			result = "no";
		} else {
			result = "yes";
		}
		return result;
	}

	public String get_Bad_cable()
	{
		String result;
		if (pub_Bad_cable ==1) {
			result = "yes";
		} else {
			result = "no";
		}
		return result;
	}

	public String get_congestion()
	{
		String result;
		if (pub_congestion == 1) {
			result = "yes";
		} else {
			result = "no";
		}
		return result;
	}

	public String get_cwndtime()
	{
		return Double.toString(pub_cwndtime);
	}

	public String get_AccessTech()
	{
		return pub_AccessTech;
	}

	public String get_rcvrLimiting()
	{
		return Double.toString(pub_pctRcvrLimited);	
	}

	public String get_optimalRcvrBuffer()
	{
		return Integer.toString(pub_MaxRwinRcvd*1024);
	}

	public String get_clientIP()
	{
		return pub_clientIP;
	}

	public String get_natStatus()
	{
		return pub_natBox;
	}

	public String get_DupAcksOut()
	{
		return Integer.toString(pub_DupAcksOut);
	}

	public String get_TimeStamp()
	{
		String result = "unknown";
		if (pub_TimeStamp != null) {
			result =  pub_TimeStamp.toString();	
		}
		return result;
	}

	public String isReady()
	{

		// if ((pub_isReady == null) || (pub_isReady.equals(""))) {
		//	 pub_isReady = "no";
		// }
		// String result = "foo";

		//if (failed) {
		//    pub_isReady = "failed1";
		//}
		//result = pub_isReady;
		//   return result; 
		return pub_isReady;
	}

	public String get_jitter()
	{
		return Integer.toString((pub_MaxRTT - pub_MinRTT));
	}

	public String get_WaitSec()
	{
		return Integer.toString((pub_CurRTO * pub_Timeouts)/1000);	
	}

	public String get_errmsg() 
	{
		//String result = "Test not run";
		//result = pub_errmsg;
		//return result;
		return pub_errmsg;
	}

	public String get_diagnosis()
	{
		return pub_diagnosis;
	} 

	public String get_statistics()
	{
		return pub_statistics;
	} 

	public String get_status()
	{
		return pub_status;
	} 

	public String get_instSpeed()
	{
		return Double.toString((8.0 * pub_bytes) /  (System.currentTimeMillis() - pub_time));
	}

	// "Remote Control" function - invoke NDT' runtest() method from the API
	public void run_test()
	{
		// The Java security model considers calling a method that opens a socket
		// from JavaScript to be a privileged action.  By using 
		// java.security.privilegedAction here, we can grant JavaScript the 
		// same expanded privileges as the signed applet to open a socket.
		AccessController.doPrivileged(new PrivilegedAction() {
			public Object run() {
				pub_errmsg = "Test in progress.";
				runtest();
				return null;
			}
		});
	}

	public String getUserAgent() {
		return _sUserAgent;
	}

	public void setUserAgent(String userAgent) {
		this._sUserAgent = userAgent;
	}

	/**
    End of accessor methods
	 **/	
	/************************************************************************/

	/**
	 * Added by Martin Sandsmark, UNINETT AS
	 * Internationalization
	 */
	private Locale _localeObj;
	private ResourceBundle _resBundleMessages;
	private String lang="en";
	private String country="US";
	//private static String lang="nb";
	//private static String country="NO";
	/***/

	int half_duplex, congestion, bad_cable, mismatch;
	double mylink;
	double loss, estimate, avgrtt, spd, waitsec, timesec, rttsec;
	double order, rwintime, sendtime, cwndtime, rwin, swin, cwin;
	double aspd;

	boolean _bIsApplication = false;
	boolean _bTestInProgress = false;
	String host = null;
	String _sTestResults, _sMidBoxTestResult;
	byte _yTests = NDTConstants.TEST_MID | NDTConstants.TEST_C2S | NDTConstants.TEST_S2C | 
			NDTConstants.TEST_SFW | NDTConstants.TEST_STATUS | NDTConstants.TEST_META;
	int c2sResult = NDTConstants.SFW_NOTTESTED;
	int s2cResult = NDTConstants.SFW_NOTTESTED;


	/* Method to initialize the base NDT window 
	 * The init() method 
	 * @param none
	 * @return none
	 */
	public void init() {
		if (getParameter("country") != null) country = getParameter("country");
		if (getParameter("language") != null) lang = getParameter("language");

		try {
			_localeObj = new Locale(lang, country);
			_resBundleMessages = ResourceBundle.getBundle("Tcpbw100_msgs", _localeObj);

			//Replaced method call to initialize _resBundleMessages for access by class
			//NDTConstants.initConstants(locale);
			NDTConstants.initConstants(lang, country);

		} catch (Exception e) {
			JOptionPane.showMessageDialog(null, "Error while loading language files:\n" + e.getMessage());
			e.printStackTrace();
		}

		//call method to create a window
		createMainWindow();
	
		//TODO :Can we move this to SwiigUtilies? Plan for after documentation - it does'nt help
		//with understanding the functionality itself
		
		/*
		getContentPane().setLayout(new BorderLayout());
		showStatus(_resBundleMessages.getString("ready"));
		_bFailed = false ;
		_bRandomize = false;
		_bCanCopy = false;
		_resultsTxtPane = new ResultsTextPane();
		_resultsTxtPane.append("TCP/Web100 Network Diagnostic Tool v" + NDTConstants.VERSION + "\n");
		_resultsTxtPane.setEditable(false);
		getContentPane().add(new JScrollPane(_resultsTxtPane));
		_resultsTxtPane.append(_resBundleMessages.getString("clickStart") + "\n");
		Panel mPanel = new Panel();
		_buttonStartTest = new JButton(_resBundleMessages.getString("start"));
		_buttonStartTest.addActionListener(this);
		mPanel.add(_buttonStartTest);
		_buttonStatistics = new JButton(_resBundleMessages.getString("statistics"));
		_buttonStatistics.addActionListener(this);
		if (getParameter("disableStatistics") == null) {
			mPanel.add(_buttonStatistics);
		}
		_buttonStatistics.setEnabled(false);
		_buttonDetails = new JButton(_resBundleMessages.getString("moreDetails"));
		_buttonDetails.addActionListener(this);
		if (getParameter("disableDetails") == null) {
			mPanel.add(_buttonDetails);
		}
		_buttonDetails.setEnabled(false);
		_buttonMailTo = new JButton(_resBundleMessages.getString("reportProblem"));
		_buttonMailTo.addActionListener(this);
		if (getParameter("disableMailto") == null) {
			mPanel.add(_buttonMailTo);
		}
		_buttonMailTo.setEnabled(false);
		options = new JButton(_resBundleMessages.getString("options") + "...");
		options.addActionListener(new ActionListener() {

			public void actionPerformed(ActionEvent e) {
				options.setEnabled(false);
				showOptions();
				options.setEnabled(true);
			}

		});
		if (getParameter("disableOptions") == null) {
			mPanel.add(options);
		}
		getContentPane().add(BorderLayout.SOUTH, mPanel);
		_chkboxPreferIPv6 = new JCheckBox(_resBundleMessages.getString("_chkboxPreferIPv6"));
		_chkboxPreferIPv6.setSelected(true);
		_chkboxDefaultTest = new JCheckBox(_resBundleMessages.getString("_chkboxDefaultTests"));
		_chkboxDefaultTest.setSelected(true);
		_chkboxDefaultTest.setEnabled(false);
		SpinnerNumberModel model = new SpinnerNumberModel();
		model.setMinimum(new Integer(0));
		model.setValue(new Integer(1));
		_spinnerTestCount.setModel(model);
		_spinnerTestCount.setPreferredSize(new Dimension(60, 20));
		delay = new JComboBox();
		for (int i = 0; i < _saDelays.length; i++) {
			delay.addItem(_resBundleMessages.getString(_saDelays[i]));
		}
		delay.setSelectedIndex(0);
	    */
		
		
		//Autorun functionality
		_sIsAutoRun = getParameter("autoRun");
		if ((_sIsAutoRun != null) && _sIsAutoRun.equals("true")) {
			pub_errmsg = "Test in progress.";
			runtest();  
		}

	}
	
	/* Method that initializes the "main" window
	 * The main window is composed of
	 * 1. The results pane, which describes the process and displays their results
	 * 2. The buttons pane, which houses all the buttons for various options
	 * @param none
	 * @return none */	
	private void createMainWindow () {
		//set content manager 
		getContentPane().setLayout(new BorderLayout());
		
		//start with status set to "Ready" to perform tests
		showStatus(_resBundleMessages.getString("ready"));
		
		//initialize 
		_bFailed = false ;
		_bRandomize = false; //what is this used for? Seems unused in code
		_bCanCopy = false;
		
		//Results panel
		_resultsTxtPane = new ResultsTextPane();
		_resultsTxtPane.append("TCP/Web100 Network Diagnostic Tool v" + NDTConstants.VERSION + "\n");
		_resultsTxtPane.setEditable(false);
		getContentPane().add(new JScrollPane(_resultsTxtPane));
		_resultsTxtPane.append(_resBundleMessages.getString("clickStart") + "\n");
		
		//Panel too add all buttons
		Panel buttonsPanel = new Panel();
		
		//Add "start" button
		_buttonStartTest = new JButton(_resBundleMessages.getString("start"));
		_buttonStartTest.addActionListener(this);
		buttonsPanel.add(_buttonStartTest);
		
		//Add "statistics" button
		_buttonStatistics = new JButton(_resBundleMessages.getString("statistics"));
		_buttonStatistics.addActionListener(this);
		if (getParameter("disableStatistics") == null) {
			buttonsPanel.add(_buttonStatistics);
		}
		_buttonStatistics.setEnabled(false);
		
		//Add "Details" button
		_buttonDetails = new JButton(_resBundleMessages.getString("moreDetails"));
		_buttonDetails.addActionListener(this);
		if (getParameter("disableDetails") == null) {
			buttonsPanel.add(_buttonDetails);
		}
		_buttonDetails.setEnabled(false);
		
		//Add "Report problem" button 
		_buttonMailTo = new JButton(_resBundleMessages.getString("reportProblem"));
		_buttonMailTo.addActionListener(this);
		if (getParameter("disableMailto") == null) {
			buttonsPanel.add(_buttonMailTo);
		}
		_buttonMailTo.setEnabled(false);
		
		//Add "Options" button
		_buttonOptions = new JButton(_resBundleMessages.getString("options") + "...");
		_buttonOptions.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				_buttonOptions.setEnabled(false);
				//call the Options-window create code
				createOptionsWindow();
				_buttonOptions.setEnabled(true);
			}
		});
		//If disableOptions is not set, then add button
		if (getParameter("disableOptions") == null) {
			buttonsPanel.add(_buttonOptions);
		}
		
		//add buttons panel to the main window
		getContentPane().add(BorderLayout.SOUTH, buttonsPanel);
		
		//"Options" panel components
		//1. Is IPv6 preferred?	
		_chkboxPreferIPv6 = new JCheckBox(_resBundleMessages.getString("preferIPv6"));
		_chkboxPreferIPv6.setSelected(true);
		//2. Conduct default tests?
		_chkboxDefaultTest = new JCheckBox(_resBundleMessages.getString("defaultTests"));
		_chkboxDefaultTest.setSelected(true);
		_chkboxDefaultTest.setEnabled(false);
		//3. configure number of tests
		SpinnerNumberModel model = new SpinnerNumberModel();
		model.setMinimum(new Integer(0));
		model.setValue(new Integer(1));
		_spinnerTestCount.setModel(model);
		_spinnerTestCount.setPreferredSize(new Dimension(60, 20));
		_cmboboxDelay = new JComboBox();
		for (int i = 0; i < _saDelays.length; i++) {
			_cmboboxDelay.addItem(_resBundleMessages.getString(_saDelays[i]));
		}
		_cmboboxDelay.setSelectedIndex(0);
		
		
	}
	
	/* Method that creates the "More details" window
	 * @param none
	 * @return none */	
	public void createDiagnoseWindow() {
		showStatus(_resBundleMessages.getString("getWeb100Var"));

		if (_frameWeb100Vars == null) {
			_frameWeb100Vars = new NewFrame();
		}
		_frameWeb100Vars.setTitle(_resBundleMessages.getString("web100Var"));
		Panel buttons = new Panel();
		_frameWeb100Vars.getContentPane().add("South", buttons);

		_buttonDismiss = new JButton(_resBundleMessages.getString("close"));
		_buttonDismiss.addActionListener(this);

		_buttonCopy = new JButton(_resBundleMessages.getString("copy"));
		_buttonCopy.addActionListener(this);

		_txtDiagnosis = new JTextArea(_resBundleMessages.getString("web100KernelVar") + ":\n", 15,30);
		_txtDiagnosis.setEditable(true);
		_buttonDismiss.setEnabled(true);
		_buttonCopy.setEnabled(_bCanCopy);

		buttons.add("West", _buttonDismiss);
		buttons.add("East", _buttonCopy);
		_frameWeb100Vars.getContentPane().add(new JScrollPane(_txtDiagnosis));
		_frameWeb100Vars.pack();
	}  // diagnose()


	/* Method that creates the "Statistics" window
	 * @param none
	 * @return none */	
	public void createStatsWindow() {
		showStatus(_resBundleMessages.getString("printDetailedStats"));

		if (_frameDetailedStats  == null) {
			_frameDetailedStats = new NewFrame();
		}
		_frameDetailedStats.setTitle(_resBundleMessages.getString("detailedStats"));
		Panel buttons = new Panel();
		_frameDetailedStats.getContentPane().add("South", buttons);

		_buttonStatsDismiss = new JButton(_resBundleMessages.getString("close"));
		_buttonStatsDismiss.addActionListener(this);

		_buttonStatsCopy = new JButton(_resBundleMessages.getString("copy"));
		_buttonStatsCopy.addActionListener(this);

		_txtStatistics = new JTextArea(_resBundleMessages.getString("web100Stats") + ":\n", 25,70);
		_txtStatistics.setEditable(true);
		_buttonStatsDismiss.setEnabled(true);
		_buttonStatsCopy.setEnabled(_bCanCopy);

		buttons.add("West", _buttonStatsDismiss);
		buttons.add("East", _buttonStatsCopy);
		_frameDetailedStats.getContentPane().add(new JScrollPane(_txtStatistics));
		_frameDetailedStats.pack();
	}  // statistics()

	
	/* Method that creates the "Options" window
	 * The options displayed to the user are:
	 * 1. Perform default tests?
	 * 2. Prefer IPV6?
	 * 3. Select the number of times the test-suite is to be run
	 * @param none
	 * @return none */	
	public void createOptionsWindow() {
		showStatus(_resBundleMessages.getString("showOptions"));

		if (_frameOptions == null) {
			_frameOptions = new NewFrame();
			_frameOptions.setTitle(_resBundleMessages.getString("options"));

			//main panel
			JPanel optionsPanel = new JPanel();
			optionsPanel.setLayout(new BoxLayout(optionsPanel, BoxLayout.Y_AXIS));

			//Panel for displaying the "default tests" option
			JPanel testsPanel = new JPanel();
			testsPanel.setBorder(BorderFactory.createTitledBorder(_resBundleMessages.getString("performedTests")));
			testsPanel.add(_chkboxDefaultTest);
			optionsPanel.add(testsPanel);

			//Panel for displaying the IPV6 option
			JPanel protocolPanel = new JPanel();
			protocolPanel.setBorder(BorderFactory.createTitledBorder(_resBundleMessages.getString("ipProtocol")));
			protocolPanel.add(_chkboxPreferIPv6);
			optionsPanel.add(protocolPanel);

			/*
			 * If user has enabled multiple tests, then build the panel, and add it
			 * to the parent( options ) panel
			*/
			if (getParameter("enableMultipleTests") != null) {
				JPanel generalPanel = new JPanel();
				generalPanel.setLayout(new BoxLayout(generalPanel, BoxLayout.Y_AXIS));
				generalPanel.setBorder(BorderFactory.createTitledBorder(_resBundleMessages.getString("general")));
				JPanel tmpPanel = new JPanel();
				tmpPanel.add(new JLabel(_resBundleMessages.getString("numberOfTests") + ":"));
				tmpPanel.add(_spinnerTestCount);
				generalPanel.add(tmpPanel);
				tmpPanel = new JPanel();
				tmpPanel.add(new JLabel(_resBundleMessages.getString("delayBetweenTests") + ":"));
				tmpPanel.add(_cmboboxDelay);
				generalPanel.add(tmpPanel);

				optionsPanel.add(generalPanel);
			}

			//Add options to the parent frame
			_frameOptions.getContentPane().add(optionsPanel);    
			
			//create and add panel containing buttons
			Panel buttonsPanel = new Panel();
			_frameOptions.getContentPane().add("South", buttonsPanel);

			JButton okButton= new JButton(_resBundleMessages.getString("ok"));
			okButton.addActionListener(new ActionListener() {

				public void actionPerformed(ActionEvent e) {
					_frameOptions.toBack();
					_frameOptions.dispose();
				}

			});

			buttonsPanel.add("West", okButton);

			_frameOptions.pack();
		}
		_frameOptions.setResizable(false);
		_frameOptions.setVisible(true);
	}


	/*Method to run the Thread that calls the "dottcp" method to run tests
	 * This method is called by the Applet's init method if user selected an
	 * "autorun" option,  is run individually if user presses the "start button",
	 * and is internally run by API call */
	synchronized public void runtest() {
		pub_status = "notStarted";
		new Thread(new TestWorker()).start();
	}

	/*Class that starts the tests in a thread
	 * Starts by disabling all buttons
	 * Calls the dottcp() method 
	 * This thread is stopped when the number of tests that was configured to be
	 *  run have all completed, or if the user stops it by interrupting from the GUI
	 *  Once the tests have been run, the buttons are enabled*/
	class TestWorker implements Runnable {
		public void run() {
			if (!_bTestInProgress) {
				int testNo = 1;
				int testsNum = ((Integer)_spinnerTestCount.getValue()).intValue();
				_bTestInProgress = true;
				createDiagnoseWindow();
				createStatsWindow();
				_buttonStartTest.setEnabled(false);
				_buttonDetails.setEnabled(false);
				_buttonStatistics.setEnabled(false);
				_buttonMailTo.setEnabled(false);
				_buttonOptions.setEnabled(false);
				_spinnerTestCount.setEnabled(false);

				//StatusPanel sPanel = new StatusPanel(testsNum);
				//re-arch. Replaced above by the line below
				String sTempEnable = getParameter("enableMultipleTests");
				
				/*create status panel based on whether multiple tests are enabled
				If not, then the progress bar displays just the specifi test (middlebox, C2S, firewall etc)
				If yes, then the progress bar also shows the progress on the number of tests
				*/
				StatusPanel sPanel = new StatusPanel(testsNum, sTempEnable);
				getContentPane().add(BorderLayout.NORTH, sPanel);
				getContentPane().validate();
				getContentPane().repaint();

				try {
					while (true) {
						if (sPanel.wantToStop()) {
							break;
						}
						if (testsNum == 0) {
							_resultsTxtPane.append("\n** " + _resBundleMessages.getString("startingTest") + " " + testNo + " **\n");
						}
						else {
							_resultsTxtPane.append("\n** " + _resBundleMessages.getString("startingTest") + " " + testNo + " " + _resBundleMessages.getString("of") + " " + testsNum + " **\n");
						}
						dottcp(sPanel);
						//If test count scheduled is complete, quit
						if (testNo == testsNum) {
							break;
						}
						//If user stops the test, quit
						if (sPanel.wantToStop()) {
							break;
						}
						sPanel.setText("");
						sPanel.endTest();
						//increment test count
						testNo += 1;
						
						/* This iteration of tests is now complete. Enable all buttons and output
						so that user can view details of results */
						_buttonDetails.setEnabled(true);
						_buttonStatistics.setEnabled(true);
						_buttonMailTo.setEnabled(true);
						_buttonOptions.setEnabled(true);
						_txtStatistics.append("\n** " + _resBundleMessages.getString("test") + " " + testNo + " **\n");
						_txtDiagnosis.append("\n** " + _resBundleMessages.getString("test") + " " + testNo + " **\n");
						
						//Now, sleep for some time based on user's choice before running the next iteration of the test suite
						try {
							switch (_cmboboxDelay.getSelectedIndex()) {
							case 1:
								_resultsTxtPane.append("\n** " + _resBundleMessages.getString("sleep1m") + " **\n");
								Thread.sleep(1000 * 60);
								break;
							case 2:
								_resultsTxtPane.append("\n** " + _resBundleMessages.getString("sleep5m") + " **\n");
								Thread.sleep(1000 * 60 * 5);
								break;
							case 3:
								_resultsTxtPane.append("\n** " + _resBundleMessages.getString("sleep10m") + " **\n");
								Thread.sleep(1000 * 60 * 10);
								break;
							case 4:
								_resultsTxtPane.append("\n** " + _resBundleMessages.getString("sleep30m") + " **\n");
								Thread.sleep(1000 * 60 * 30);
								break;
							case 5:
								_resultsTxtPane.append("\n** " + _resBundleMessages.getString("sleep2h") + " **\n");
								Thread.sleep(1000 * 60 * 120);
								break;
							case 6:
								_resultsTxtPane.append("\n** " + _resBundleMessages.getString("sleep12h") + " **\n");
								Thread.sleep(1000 * 60 * 720);
								break;
							case 7:
								_resultsTxtPane.append("\n** " + _resBundleMessages.getString("sleep1d") + " **\n");
								Thread.sleep(1000 * 60 * 1440);
								break;
							}
						}
						catch (InterruptedException e) {
							// do nothing.
						}
					}
				} catch(IOException e) {
					e.printStackTrace();
					_bFailed=true;
					_sErrMsg = _resBundleMessages.getString("serverBusy30s") + "\n";
				}

				/* If test failed due to any reason, mark failure reason too*/
				if (_bFailed) {
					_resultsTxtPane.append(_sErrMsg);

					pub_isReady = "failed";
					pub_errmsg = _sErrMsg;
				}

				//Enable all buttons. Continue activities to mark status as complete
				_buttonDetails.setEnabled(true);
				_buttonStatistics.setEnabled(true);
				_buttonMailTo.setEnabled(true);
				_buttonOptions.setEnabled(true);
				_spinnerTestCount.setEnabled(true);
				showStatus(_resBundleMessages.getString("done2"));
				_resultsTxtPane.append("\n" + _resBundleMessages.getString("clickStart2") + "\n");
				_buttonStartTest.setEnabled(true);
				_bTestInProgress = false;
				getContentPane().remove(sPanel);
				getContentPane().validate();
				getContentPane().repaint();
			}
		}
	}

	public void actionPerformed(ActionEvent event) {
		Object source = event.getSource();
		// System.err.println("Processing WINDOW event #" +event.getID());
		// System.err.println("Processing event " + source);

		if (source == _buttonStartTest) {
			if(_frameWeb100Vars != null) {
				_frameWeb100Vars.toBack();
				_frameWeb100Vars.dispose();
				_frameWeb100Vars = null;
			}

			if(_frameDetailedStats != null) {
				_frameDetailedStats.toBack();
				_frameDetailedStats.dispose();
				_frameDetailedStats = null;
			}

			pub_errmsg = "Test in progress.";
			runtest();
		}

		else if (source == _buttonDetails) {
			_buttonDetails.setEnabled(false);
			_frameWeb100Vars.setResizable(true);
			_frameWeb100Vars.setVisible(true);
			_buttonDetails.setEnabled(true);
		}

		else if (source == _buttonDismiss) {
			_frameWeb100Vars.toBack();
			_frameWeb100Vars.dispose();
		}

		else if (source == _buttonStatsDismiss) {
			_frameDetailedStats.toBack();
			_frameDetailedStats.dispose();
		}

		else if (source == _buttonCopy) {
			try {
				Clipboard clipbd = getToolkit().getSystemClipboard();
				_bCanCopy = true;
				String s = _txtDiagnosis.getText();
				StringSelection ss = new StringSelection(s);
				clipbd.setContents(ss, ss);
				_txtDiagnosis.selectAll();
			} catch (SecurityException e) {
				_bCanCopy = false;
			}
		}

		else if (source == _buttonStatsCopy) {
			Clipboard clipbd = getToolkit().getSystemClipboard();
			String sTemp = _txtStatistics.getText();
			StringSelection ssTemp = new StringSelection(sTemp);
			clipbd.setContents(ssTemp, ssTemp);
			_txtStatistics.selectAll();
		}

		else if (source == _buttonStatistics) {
			_buttonStatistics.setEnabled(false);
			_frameDetailedStats.setResizable(true);
			_frameDetailedStats.setVisible(true);
			_buttonStatistics.setEnabled(true);
		}

		else if (source == _buttonMailTo) {
			//int i; //did'nt need it
			//char key; comment out. seems unused
			String to[], from[], comments[];
			String sName, sHost;

			_buttonMailTo.setEnabled(false);
			// envoke mailto: function
			showStatus(_resBundleMessages.getString("invokingMailtoFunction") + "...");

			_resultsTxtPane.append(_resBundleMessages.getString("generatingReport") + "\n");
			try {
				//TODO exception here occasionaly. Check out if my changes introduced these
				if ((sName = getParameter(NDTConstants.TARGET1)) == null) {
					throw new IllegalArgumentException("U parameter Required:");
				}
				if ((sHost = getParameter(NDTConstants.TARGET2)) == null) {
					throw new IllegalArgumentException("H parameter Required:");
				}

				String theURL = "mailto:" + sName + "@" + sHost;
				String subject = getParameter("subject");

				if (subject == null) {
					subject = _resBundleMessages.getString("troubleReportFrom") + " " + getCodeBase().getHost();
				}
				theURL += "?subject=" + subject;
				theURL += "&body=" + _resBundleMessages.getString("comments") + ":%0A%0A" + _sEmailText + " " + _resBundleMessages.getString("endOfEmail") + "\n%0A";
				// System.out.println("Message body is '" + emailText + "'\n");
				_targetURL = new URL(theURL);

			} catch (MalformedURLException rsi) {
				throw new IllegalArgumentException("Can't create mailto: URL" + rsi.getMessage());
			}

			getAppletContext().showDocument(_targetURL);
		}
	}  // actionPerformed()

	/* Method to display current status in Applet window
	 * @param String value of status
	 * @return void */
	public void showStatus(String msg) {
		if (!_bIsApplication) {
			super.showStatus(msg);
		}
	}
	
	
	/* MiddleBox testing method
	 * @param Protocol Object
	 * @return boolean value indicating test failure status
	 * 	   true if test failed
	 *     false if test passed
	 * @ */
	
	public boolean test_mid(Protocol ctl) throws IOException {

		//byte buff[] = new byte[8192];
		//
		byte buff[] = new byte[NDTConstants.MIDDLEBOX_PREDEFINED_MSS];
		
		Message msg = new Message();
		if ((_yTests & NDTConstants.TEST_MID) == NDTConstants.TEST_MID) {
			/* now look for middleboxes (firewalls, NATs, and other boxes that
			 * muck with TCP's end-to-end principles
			 */
			showStatus(_resBundleMessages.getString("middleboxTest"));
			_resultsTxtPane.append(_resBundleMessages.getString("checkingMiddleboxes") + "  ");
			_txtStatistics.append(_resBundleMessages.getString("checkingMiddleboxes") + "  ");
			_sEmailText = _resBundleMessages.getString("checkingMiddleboxes") + "  ";
			pub_status = "checkingMiddleboxes";

			//If reading socket was not successful, then declare middlebox test as a failure
			if (ctl.recv_msg(msg) != 0) {
				_sErrMsg = _resBundleMessages.getString("protocolError") + Integer.parseInt(new String(msg.getBody()), 16) + " instead\n";
				return true;
			}
			
			//Initially, expecting a TEST_PREPARE message
			if (msg.getType() != MessageType.TEST_PREPARE) {
				_sErrMsg = _resBundleMessages.getString("mboxWrongMessage") + "\n";
				if (msg.getType() == MessageType.MSG_ERROR) {
					_sErrMsg += "ERROR MSG: " + Integer.parseInt(new String(msg.getBody()), 16) + "\n";
				}
				return true;
			}
			//get port number that server wants client to bind to for this test
			int midport = Integer.parseInt(new String(msg.getBody()));

			//connect to server using port obtained above
			Socket midSrvrSockObj = null;
			try {
				midSrvrSockObj = new Socket(host, midport);
			} catch (UnknownHostException e) {
				System.err.println("Don't know about host: " + host);
				_sErrMsg = _resBundleMessages.getString("unknownServer") + "\n" ;
				return true;
			} catch (IOException e) {
				System.err.println("Couldn't perform middlebox testing to: " + host);
				_sErrMsg = _resBundleMessages.getString("middleboxFail") + "\n" ;
				return true;
			}

			InputStream srvin2 = midSrvrSockObj.getInputStream();
			OutputStream srvout2 = midSrvrSockObj.getOutputStream();

			//int largewin = 128*1024; //Unused variable, commenting out until needed

			//Time out the socket after 6.5 seconds
			midSrvrSockObj.setSoTimeout(6500);
			int bytes = 0;
			int inlth;
			_dTime = System.currentTimeMillis();
			pub_TimeStamp = new Date();

			/*Read server packets into the pre-initialized buffer.
			 * Record number of packets read as reading completes
			*/
			try {  
				while ((inlth=srvin2.read(buff,0,buff.length)) > 0) {
					bytes += inlth;
					pub_bytes = bytes;
					//If more than 5.5 seconds have passed by, stop reading socket input
					if ((System.currentTimeMillis() - _dTime) > 5500)
						break;
				}
			} 
			catch (IOException e) {} //TODO Question for a later fix. Why no exception handling code?

			//Record test duration seconds
			_dTime =  System.currentTimeMillis() - _dTime;
			System.out.println(bytes + " bytes " + (8.0 * bytes)/_dTime + " kb/s " + _dTime/1000 + " secs");
			//Calculate throughput in Kbps. 
			_dS2cspd = ((8.0 * bytes) / 1000) / _dTime;

			//Test is complete. Now, get results from server 
			if (ctl.recv_msg(msg) != 0) { //msg not received correctly
				_sErrMsg = _resBundleMessages.getString("protocolError") + Integer.parseInt(new String(msg.getBody()), 16) + " instead\n";
				return true;
			}
			
			//Results are sent from server in the form of a TEST_MSG object
			if (msg.getType() != MessageType.TEST_MSG) { //If not TEST_MSG, then test results not obtained
				_sErrMsg = _resBundleMessages.getString("mboxWrongMessage") + "\n";
				//get error code
				if (msg.getType() == MessageType.MSG_ERROR) {
					_sErrMsg += "ERROR MSG: " + Integer.parseInt(new String(msg.getBody()), 16) + "\n";
				}
				return true;
			}
			
			//Get Test results 
			_sMidBoxTestResult = new String(msg.getBody());

			//client now sends throughput as calculated above, in bytes
			String tmpstr4 = Double.toString(_dS2cspd*1000);
			System.out.println("Sending '" + tmpstr4 + "' back to server");
			ctl.send_msg(MessageType.TEST_MSG, tmpstr4.getBytes());

			//append local address to the Test Results obtained from server
			try {
				_sMidBoxTestResult += midSrvrSockObj.getInetAddress() + ";";
			} catch (SecurityException e) {
				System.err.println("Unable to obtain Servers IP addresses: using " + host);
				_sErrMsg = "getInetAddress() called failed\n" ;
				_sMidBoxTestResult += host + ";";
				_resultsTxtPane.append(_resBundleMessages.getString("lookupError") + "\n");
			}

			System.err.println("calling in2Socket.getLocalAddress()");
			try {
				_sMidBoxTestResult += midSrvrSockObj.getLocalAddress() + ";";
			} catch (SecurityException e) {
				System.err.println("Unable to obtain local IP address: using 127.0.0.1");
				_sErrMsg = "getLocalAddress() call failed\n" ;
				_sMidBoxTestResult += "127.0.0.1;";
			}
			
			//wrap up test set up
			srvin2.close();
			srvout2.close();
			midSrvrSockObj.close();

			//Expect TEST_FINALIZE message from server
			if (ctl.recv_msg(msg) != 0) {
				_sErrMsg = _resBundleMessages.getString("protocolError") + Integer.parseInt(new String(msg.getBody()), 16) + " instead\n";
				return true;
			}
			
			if (msg.getType() != MessageType.TEST_FINALIZE) { //report unexpected message reception
				_sErrMsg = _resBundleMessages.getString("mboxWrongMessage");
				if (msg.getType() == MessageType.MSG_ERROR) {
					_sErrMsg += "ERROR MSG: " + Integer.parseInt(new String(msg.getBody()), 16) + "\n";
				}
				return true;
			}
			
			//Report status as "complete"
			_resultsTxtPane.append(_resBundleMessages.getString("done") + "\n");
			_txtStatistics.append(_resBundleMessages.getString("done") + "\n");
			_sEmailText += _resBundleMessages.getString("done") + "\n%0A";
		}
		return false;
	}

	/* Fireiwall tests aiming to find out if one exists between Client and server 
	 * Tests performed in both directions
	 * @param Protocol Object
	 * @return boolean 
	 * 	   true if test failed
	 *     false if test passed */
	public boolean test_sfw(Protocol protocolObj) throws IOException
	{
		Message msg = new Message();
		//start test
		if ((_yTests & NDTConstants.TEST_SFW) == NDTConstants.TEST_SFW) {
			//show status of test as started
			showStatus(_resBundleMessages.getString("sfwTest"));
			_resultsTxtPane.append(_resBundleMessages.getString("checkingFirewalls") + "  ");
			_txtStatistics.append(_resBundleMessages.getString("checkingFirewalls") + "  ");
			_sEmailText = _resBundleMessages.getString("checkingFirewalls") + "  ";
			pub_status = "checkingFirewalls";

			//Message received in error?
			if (protocolObj.recv_msg(msg) != 0) {
				_sErrMsg = _resBundleMessages.getString("protocolError") + Integer.parseInt(new String(msg.getBody()), 16) + " instead\n";
				return true;
			}
			
			//TEST_PREPARE is the first message expected from the server
			if (msg.getType() != MessageType.TEST_PREPARE) { //oops, unexpected message received
				_sErrMsg = _resBundleMessages.getString("sfwWrongMessage") + "\n";
				if (msg.getType() == MessageType.MSG_ERROR) {
					_sErrMsg += "ERROR MSG: " + Integer.parseInt(new String(msg.getBody()), 16) + "\n";
				}
				return true;
			}

			//Get message body from server packet
			String sMsgBody = new String(msg.getBody());

			/* 
			 * This message body contains ephemeral port # and testTime separated by a 
			 * single space
			 */
			int srvPort, testTime;
			try {
				int k = sMsgBody.indexOf(" ");
				srvPort = Integer.parseInt(sMsgBody.substring(0,k));
				testTime = Integer.parseInt(sMsgBody.substring(k+1));
			}
			catch (Exception e) {
				_sErrMsg = _resBundleMessages.getString("sfwWrongMessage") + "\n";
				return true;
			}

			System.out.println("SFW: port=" + srvPort);
			System.out.println("SFW: testTime=" + testTime);

			//Clients creates a server socket to send TEST_MSG
			ServerSocket srvSocket;
			try {
				SecurityManager security = System.getSecurityManager();
				if (security != null) {
					System.out.println("Asking security manager for listen permissions...");
					security.checkListen(0);
				}
				//0 to use any free port
				srvSocket = new ServerSocket(0);
			}
			catch (Exception e) {
				e.printStackTrace();
				_sErrMsg = _resBundleMessages.getString("sfwSocketFail") + "\n";
				return true;
			}

			System.out.println("SFW: oport=" + srvSocket.getLocalPort());
			//Send TEST_MSG
			protocolObj.send_msg(MessageType.TEST_MSG, Integer.toString(srvSocket.getLocalPort()).getBytes());

			//Expect a TEST_START message from the server
			if (protocolObj.recv_msg(msg) != 0) { //oops, error receiving message
				_sErrMsg = _resBundleMessages.getString("protocolError") + Integer.parseInt(new String(msg.getBody()), 16) + " instead\n";
				return true;
			}
			//Only TEST_START message expected at this point
			if (msg.getType() != MessageType.TEST_START) {
				_sErrMsg = _resBundleMessages.getString("sfwWrongMessage");
				if (msg.getType() == MessageType.MSG_ERROR) {
					_sErrMsg += "ERROR MSG: " + Integer.parseInt(new String(msg.getBody()), 16) + "\n";
				}
				return true;
			}     

			//re-arch new code
			//OsfwWorker osfwTest = new OsfwWorker(srvSocket, testTime);
			OsfwWorker osfwTest = new OsfwWorker(srvSocket, testTime, this);
			new Thread(osfwTest).start();

			//Now, run test trying to connect to ephemeral port 
			Socket sfwSocket = new Socket();
			try {
				//create socket to ephemeral port. testTime now specified in mS
				sfwSocket.connect(new InetSocketAddress(host, srvPort), testTime * 1000);

				Protocol sfwCtl = new Protocol(sfwSocket);
				
				//send a simple string message over this socket
				sfwCtl.send_msg(MessageType.TEST_MSG, new String("Simple firewall test").getBytes());
			}
			catch (Exception e) {
				e.printStackTrace();
			}

			//Server expected to respond back with a TEST_MSG too
			if (protocolObj.recv_msg(msg) != 0) { //oops, error reading Protocol
				_sErrMsg = _resBundleMessages.getString("protocolError") + Integer.parseInt(new String(msg.getBody()), 16) + " instead\n";
				return true;
			}
			
			//Only TEST_MSG type expected at this point
			if (msg.getType() != MessageType.TEST_MSG) {
				_sErrMsg = _resBundleMessages.getString("sfwWrongMessage") + "\n";
				if (msg.getType() == MessageType.MSG_ERROR) {
					_sErrMsg += "ERROR MSG: " + Integer.parseInt(new String(msg.getBody()), 16) + "\n";
				}
				return true;
			}
			
			//Todo: Could not determine where this is used yet
			//But this is an integer with value 0/1/2/3 indicating connection status
			c2sResult = Integer.parseInt(new String(msg.getBody()));

			//Sleep for some time
			osfwTest.finalize();

			//Server closes the SFW test session by sending TEST_FINALIZE
			if (protocolObj.recv_msg(msg) != 0) { //oops, error reading message
				_sErrMsg = _resBundleMessages.getString("protocolError") + Integer.parseInt(new String(msg.getBody()), 16) + " instead\n";
				return true;
			}
			
			//ONLY TEST_FINALIZE type of message expected here
			if (msg.getType() != MessageType.TEST_FINALIZE) {
				_sErrMsg = _resBundleMessages.getString("sfwWrongMessage") + "\n";
				if (msg.getType() == MessageType.MSG_ERROR) {
					_sErrMsg += "ERROR MSG: " + Integer.parseInt(new String(msg.getBody()), 16) + "\n";
				}
				return true;
			}
			
			//Conclude by updatng status as "complete" on GUI window
			_resultsTxtPane.append(_resBundleMessages.getString("done") + "\n");
			_txtStatistics.append(_resBundleMessages.getString("done") + "\n");
			_sEmailText += _resBundleMessages.getString("done") + "\n%0A";
		}
		
		//successfully finished the SFW test, hence return testfailure=false
		return false;
	}

	public boolean test_c2s(Protocol ctl) throws IOException
	{
		// byte buff2[] = new byte[8192];
		byte buff2[] = new byte[64*1024];
		Message msg = new Message();
		if ((_yTests & NDTConstants.TEST_C2S) == NDTConstants.TEST_C2S) {
			showStatus(_resBundleMessages.getString("outboundTest"));
			_resultsTxtPane.append(_resBundleMessages.getString("runningOutboundTest") + " ");
			_txtStatistics.append(_resBundleMessages.getString("runningOutboundTest") + " ");
			_sEmailText += _resBundleMessages.getString("runningOutboundTest") + " ";
			pub_status = "runningOutboundTest";

			if (ctl.recv_msg(msg) != 0) {
				_sErrMsg = _resBundleMessages.getString("protocolError") + Integer.parseInt(new String(msg.getBody()), 16) + " instead\n";
				return true;
			}
			if (msg.getType() != MessageType.TEST_PREPARE) {
				_sErrMsg = _resBundleMessages.getString("outboundWrongMessage") + "\n";
				if (msg.getType() == MessageType.MSG_ERROR) {
					_sErrMsg += "ERROR MSG: " + Integer.parseInt(new String(msg.getBody()), 16) + "\n";
				}
				return true;
			}
			int c2sport = Integer.parseInt(new String(msg.getBody()));

			final Socket outSocket;
			try {
				outSocket = new Socket(host, c2sport);
			} catch (UnknownHostException e) {
				System.err.println("Don't know about host: " + host);
				_sErrMsg = _resBundleMessages.getString("unknownServer") + "\n" ;
				return true;
			} catch (IOException e) {
				System.err.println("Couldn't get 2nd connection to: " + host);
				_sErrMsg = _resBundleMessages.getString("serverBusy15s") + "\n";
				return true;
			}

			// Get server IP address from the outSocket.
			pub_host	 = outSocket.getInetAddress().getHostAddress().toString();


			final OutputStream out = outSocket.getOutputStream();

			// wait here for signal from server application 
			// This signal tells the client to start pumping out data
			if (ctl.recv_msg(msg) != 0) {
				_sErrMsg = _resBundleMessages.getString("protocolError") + Integer.parseInt(new String(msg.getBody()), 16) + " instead\n";
				return true;
			}
			if (msg.getType() != MessageType.TEST_START) {
				_sErrMsg = _resBundleMessages.getString("outboundWrongMessage") + "\n";
				if (msg.getType() == MessageType.MSG_ERROR) {
					_sErrMsg += "ERROR MSG: " + Integer.parseInt(new String(msg.getBody()), 16) + "\n";
				}
				return true;
			}

			byte c = '0';
			int i;
			for (i=0; i<_iLength; i++) {
				if (c == 'z')
					c = '0';
				buff2[i] = c++;
			}
			System.err.println("Send buffer size =" + i);
			_iPkts = 0;
			_dTime = System.currentTimeMillis();
			pub_time = _dTime;
			new Thread() {

				public void run() {
					try {
						Thread.sleep(10000);
					} catch (InterruptedException e) {
						System.out.println(e);
					}
					try {
						out.close();
						outSocket.close();
					} catch (IOException e) {
						System.out.println(e);
					}
				}
			}.start();
			while (true) {
				// System.err.println("Send pkt = " + pkts + "; at " + System.currentTimeMillis());
				try {
					out.write(buff2, 0, buff2.length);
				}
				catch (SocketException e) {
					System.out.println(e);
					break;
				}
				// catch (InterruptedIOException iioe) {
				catch (IOException ioe) {
					System.out.println("Client socket timed out");
					break;
				}
				_iPkts++;
				pub_bytes = (_iPkts * _iLength);
			}

			_dTime =  System.currentTimeMillis() - _dTime;
			System.err.println(_dTime + "sec test completed");
			if (_dTime == 0) {
				_dTime = 1;
			}
			System.out.println((8.0 * _iPkts * buff2.length) / _dTime + " kb/s outbound");
			_dC2sspd = ((8.0 * _iPkts * buff2.length) / 1000) / _dTime;
			/* receive the c2sspd from the server */
			if (ctl.recv_msg(msg) != 0) {
				_sErrMsg = _resBundleMessages.getString("protocolError") + Integer.parseInt(new String(msg.getBody()), 16) + " instead\n";
				return true;
			}
			if (msg.getType() != MessageType.TEST_MSG) {
				_sErrMsg = _resBundleMessages.getString("outboundWrongMessage");
				if (msg.getType() == MessageType.MSG_ERROR) {
					_sErrMsg += "ERROR MSG: " + Integer.parseInt(new String(msg.getBody()), 16) + "\n";
				}
				return true;
			}
			String tmpstr3 = new String(msg.getBody());
			_dSc2sspd = Double.parseDouble(tmpstr3) / 1000.0;

			if (_dSc2sspd < 1.0) {
				_resultsTxtPane.append(prtdbl(_dSc2sspd*1000) + "kb/s\n");
				_txtStatistics.append(prtdbl(_dSc2sspd*1000) + "kb/s\n");
				_sEmailText += prtdbl(_dSc2sspd*1000) + "kb/s\n%0A";
			} 
			else {
				_resultsTxtPane.append(prtdbl(_dSc2sspd) + "Mb/s\n");
				_txtStatistics.append(prtdbl(_dSc2sspd) + "Mb/s\n");
				_sEmailText += prtdbl(_dSc2sspd) + "Mb/s\n%0A";
			}

			// Expose upload speed to JavaScript clients
			pub_c2sspd = _dSc2sspd;

			if (ctl.recv_msg(msg) != 0) {
				_sErrMsg = _resBundleMessages.getString("protocolError") + Integer.parseInt(new String(msg.getBody()), 16) + " instead\n";
				return true;
			}
			if (msg.getType() != MessageType.TEST_FINALIZE) {
				_sErrMsg = _resBundleMessages.getString("outboundWrongMessage");
				if (msg.getType() == MessageType.MSG_ERROR) {
					_sErrMsg += "ERROR MSG: " + Integer.parseInt(new String(msg.getBody()), 16) + "\n";
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
		if ((_yTests & NDTConstants.TEST_S2C) == NDTConstants.TEST_S2C) {
			showStatus(_resBundleMessages.getString("inboundTest"));
			_resultsTxtPane.append(_resBundleMessages.getString("runningInboundTest") + " ");
			_txtStatistics.append(_resBundleMessages.getString("runningInboundTest") + " ");
			_sEmailText += _resBundleMessages.getString("runningInboundTest") + " ";
			pub_status = "runningInboundTest";

			if (ctl.recv_msg(msg) != 0) {
				_sErrMsg = _resBundleMessages.getString("protocolError") + Integer.parseInt(new String(msg.getBody()), 16) + " instead\n";
				return true;
			}
			if (msg.getType() != MessageType.TEST_PREPARE) {
				_sErrMsg = _resBundleMessages.getString("inboundWrongMessage") + "\n";
				if (msg.getType() == MessageType.MSG_ERROR) {
					_sErrMsg += "ERROR MSG: " + Integer.parseInt(new String(msg.getBody()), 16) + "\n";
				}
				return true;
			}
			int s2cport = Integer.parseInt(new String(msg.getBody()));

			Socket inSocket;
			try {
				inSocket = new Socket(host, s2cport);
			} 
			catch (UnknownHostException e) {
				System.err.println("Don't know about host: " + host);
				_sErrMsg = "unknown server\n" ;
				return true;
			} 
			catch (IOException e) {
				System.err.println("Couldn't get 3rd connection to: " + host);
				_sErrMsg = "Server Failed while receiving data\n" ;
				return true;
			}

			InputStream srvin = inSocket.getInputStream();
			int bytes = 0;
			int inlth;

			// wait here for signal from server application 
			if (ctl.recv_msg(msg) != 0) {
				_sErrMsg = _resBundleMessages.getString("unknownServer") + Integer.parseInt(new String(msg.getBody()), 16) + " instead\n";
				return true;
			}
			if (msg.getType() != MessageType.TEST_START) {
				_sErrMsg = _resBundleMessages.getString("serverFail") + "\n";
				if (msg.getType() == MessageType.MSG_ERROR) {
					_sErrMsg += "ERROR MSG: " + Integer.parseInt(new String(msg.getBody()), 16) + "\n";
				}
				return true;
			}

			inSocket.setSoTimeout(15000);
			_dTime = System.currentTimeMillis();
			pub_time = _dTime;

			try {  
				while ((inlth=srvin.read(buff,0,buff.length)) > 0) {
					bytes += inlth;
					pub_bytes = bytes;
					if ((System.currentTimeMillis() - _dTime) > 14500)
						break;
				}
			} 
			catch (IOException e) {}

			_dTime =  System.currentTimeMillis() - _dTime;
			System.out.println(bytes + " bytes " + (8.0 * bytes)/_dTime + " kb/s " + _dTime/1000 + " secs");
			_dS2cspd = ((8.0 * bytes) / 1000) / _dTime;

			/* receive the s2cspd from the server */
			if (ctl.recv_msg(msg) != 0) {
				_sErrMsg = _resBundleMessages.getString("protocolError") + Integer.parseInt(new String(msg.getBody()), 16) + " instead\n";
				return true;
			}
			if (msg.getType() != MessageType.TEST_MSG) {
				_sErrMsg = _resBundleMessages.getString("inboundWrongMessage") + "\n";
				if (msg.getType() == MessageType.MSG_ERROR) {
					_sErrMsg += "ERROR MSG: " + Integer.parseInt(new String(msg.getBody()), 16) + "\n";
				}
				return true;
			}
			try {
				String tmpstr3 = new String(msg.getBody());
				int k1 = tmpstr3.indexOf(" ");
				int k2 = tmpstr3.substring(k1+1).indexOf(" ");
				_dSs2cspd = Double.parseDouble(tmpstr3.substring(0, k1)) / 1000.0;
				_iSsndqueue = Integer.parseInt(tmpstr3.substring(k1+1).substring(0, k2));
				_dSbytes = Double.parseDouble(tmpstr3.substring(k1+1).substring(k2+1));
			}
			catch (Exception e) {
				e.printStackTrace();
				_sErrMsg = _resBundleMessages.getString("inboundWrongMessage") + "\n";
				return true;
			}

			if (_dS2cspd < 1.0) {
				_resultsTxtPane.append(prtdbl(_dS2cspd*1000) + "kb/s\n");
				_txtStatistics.append(prtdbl(_dS2cspd*1000) + "kb/s\n");
				_sEmailText += prtdbl(_dS2cspd*1000) + "kb/s\n%0A";
			} else {
				_resultsTxtPane.append(prtdbl(_dS2cspd) + "Mb/s\n");
				_txtStatistics.append(prtdbl(_dS2cspd) + "Mb/s\n");
				_sEmailText += prtdbl(_dS2cspd) + "Mb/s\n%0A";
			}


			// Expose download speed to JavaScript clients
			pub_s2cspd = _dS2cspd;
			pub_status = "done";

			srvin.close();
			inSocket.close();

			buff = Double.toString(_dS2cspd*1000).getBytes();
			String tmpstr4 = new String(buff, 0, buff.length);
			System.out.println("Sending '" + tmpstr4 + "' back to server");
			ctl.send_msg(MessageType.TEST_MSG, buff);

			/* get web100 variables from server */
			_sTestResults = "";
			int i = 0;

			// Try setting a 5 second timer here to break out if the read fails.
			ctlSocket.setSoTimeout(5000);
			try {  
				for (;;) {
					if (ctl.recv_msg(msg) != 0) {
						_sErrMsg = _resBundleMessages.getString("protocolError") + Integer.parseInt(new String(msg.getBody()), 16) + " instead\n";
						return true;
					}
					if (msg.getType() == MessageType.TEST_FINALIZE) {
						break;
					}
					if (msg.getType() != MessageType.TEST_MSG) {
						_sErrMsg = _resBundleMessages.getString("inboundWrongMessage") + "\n";
						if (msg.getType() == MessageType.MSG_ERROR) {
							_sErrMsg += "ERROR MSG: " + Integer.parseInt(new String(msg.getBody()), 16) + "\n";
						}
						return true;
					}
					_sTestResults += new String(msg.getBody());
					i++;
				}
			} catch (IOException e) {}
			ctlSocket.setSoTimeout(0);
		}
		pub_status = "done";
		return false;
	}

	/* Meta tests method 
	 * The META test allows the Client to send an additional information to the Server
	 * that basically gets included along with the overall set of test results. 
	 *  
	 * @param Protocol Object
	 * @return booleanturn boolean 
	 * 	   true if test failed
	 *     false if test passed
	 * @throws IOException when sending/receiving messages from server fails
	 * @see Protocol#recv_msg(Message msgParam)
	 * @see Protocol#send_msg(byte bParamType, byte[] baParamTab)
	 *    These methods indicate more information about IOException
	 * */
	public boolean test_meta(Protocol protocolObj) throws IOException
	{
		Message msg = new Message();
		//Start META tests
		if ((_yTests & NDTConstants.TEST_META) == NDTConstants.TEST_META) {
			showStatus(_resBundleMessages.getString("metaTest"));
			_resultsTxtPane.append(_resBundleMessages.getString("sendingMetaInformation") + " ");
			_txtStatistics.append(_resBundleMessages.getString("sendingMetaInformation") + " ");
			_sEmailText += _resBundleMessages.getString("sendingMetaInformation") + " ";
			pub_status = "sendingMetaInformation";

			//Server starts with a TEST_PREPARE message. 
			if (protocolObj.recv_msg(msg) != 0) { //error, message not received correctly
				_sErrMsg = _resBundleMessages.getString("protocolError") + Integer.parseInt(new String(msg.getBody()), 16) + " instead\n";
				return true;
			}
			if (msg.getType() != MessageType.TEST_PREPARE) { 
				//only TEST_PREPARE message expected at this point
				_sErrMsg = _resBundleMessages.getString("metaWrongMessage") + "\n";
				if (msg.getType() == MessageType.MSG_ERROR) {
					_sErrMsg += "ERROR MSG: " + Integer.parseInt(new String(msg.getBody()), 16) + "\n";
				}
				return true;
			}

			//Server now send a TEST_START message 
			if (protocolObj.recv_msg(msg) != 0) { //error, message not read/received correctly
				_sErrMsg = _resBundleMessages.getString("protocolError") + Integer.parseInt(new String(msg.getBody()), 16) + " instead\n";
				return true;
			}
			
			//Only TEST_START message expected here. Everything else is unacceptable
			if (msg.getType() != MessageType.TEST_START) {
				_sErrMsg = _resBundleMessages.getString("metaWrongMessage") + "\n";
				
				if (msg.getType() == MessageType.MSG_ERROR) {
					_sErrMsg += "ERROR MSG: " + Integer.parseInt(new String(msg.getBody()), 16) + "\n";
				}
				return true;
			}

			/* As a response to the Server's TEST_START message, client responds with TEST_MSG. These messages may be used, as below
			 to send configuration data name-value pairs.
			 Note that there are length constraints to keys- values: 64/256 characters respectively*/
			protocolObj.send_msg(MessageType.TEST_MSG, (NDTConstants.META_CLIENT_OS + ":" + System.getProperty("os.name")).getBytes());
			protocolObj.send_msg(MessageType.TEST_MSG, (NDTConstants.META_BROWSER_OS + ":" + UserAgentTools.getBrowser(getUserAgent())[2]).getBytes());
			protocolObj.send_msg(MessageType.TEST_MSG, (NDTConstants.META_CLIENT_KERNEL_VERSION + ":" + System.getProperty("os.version")).getBytes());
			protocolObj.send_msg(MessageType.TEST_MSG, (NDTConstants.META_CLIENT_VERSION + ":" + NDTConstants.VERSION).getBytes());

			/* Client can send any number of such meta data in a TEST_MSG format, and signal
			the end of the transmission using an empty TEST_MSG */
			protocolObj.send_msg(MessageType.TEST_MSG, new byte[0]);

			//The server now closes the META test session by sending a TEST_FINALIZE message
			if (protocolObj.recv_msg(msg) != 0) { //error, message cannot be read/received properly
				_sErrMsg = _resBundleMessages.getString("protocolError") + Integer.parseInt(new String(msg.getBody()), 16) + " instead\n";
				return true;
			}
			if (msg.getType() != MessageType.TEST_FINALIZE) { //Only this message type is expected
				_sErrMsg = _resBundleMessages.getString("metaWrongMessage");
				if (msg.getType() == MessageType.MSG_ERROR) {
					_sErrMsg += "ERROR MSG: " + Integer.parseInt(new String(msg.getBody()), 16) + "\n";
				}
				return true;
			}
			//Display status as "complete"
			_resultsTxtPane.append(_resBundleMessages.getString("done") + "\n");
			_txtStatistics.append(_resBundleMessages.getString("done") + "\n");
			_sEmailText += _resBundleMessages.getString("done") + "\n%0A";
		}

		//completed tests
		pub_status = "done";
		//status is false indicating test-failure=false
		return false;
	}

	/* Method to run tests and interpret the results sent by the server
	 * @param StatusPanel object to describe status of tests
	 * @return none
	 */
	public void dottcp(StatusPanel sPanel) throws IOException {
		Socket ctlSocket = null;
		if (!_bIsApplication) {

			/*************************************************************************
			 * Enable NDT to test against a web100srv instance on a remote server.
			 * Instead of using the getCodeBase().getHost() value for the testing server,
			 * which assumes this applet is being served from the web100srv server,
			 * use a parameter provided in the APPLET tag.
			 * Note that for this to work the applet must be signed because you are
			 * potentially accessing a server outside the source domain.
			 */
			host = getParameter("testingServer");
			/************************************************************************/
			/* fall back to the old behaviour if the APPLET tag is not set */
			if (host == null) {
				host = getCodeBase().getHost();
			}

			pub_host = host;
		}

		/* The default control port used for the NDT tests session. NDT server listens 
		 to this port */
		int ctlport = NDTConstants.CONTROL_PORT_DEFAULT;
		
		//Commenting these 2 variables - seem unused
		//double wait2;
		//int sbuf, rbuf;
		int i, wait;
		int iServerWaitFlag=0; //flag indicating whether a wait message was already received once

		//Assign false to test result status initially
		_bFailed = false;

		try {

			// RAC Debug message
			_resultsTxtPane.append(_resBundleMessages.getString("connectingTo") + " '" + host + "' [" + InetAddress.getByName(host) + "] " + _resBundleMessages.getString("toRunTest") + "\n");
			//If IPv6 is preferred by Applet user, set property for any further use
			if (_chkboxPreferIPv6.isSelected()) {
				try {
					System.setProperty("java.net._chkboxPreferIPv6Addresses", "true");
				}
				catch (SecurityException e) {
					System.err.println("Couldn't set system property. Check your security settings.");
				}
			}
			_chkboxPreferIPv6.setEnabled(false);
			//create socket to host specified by user and the default port
			ctlSocket = new Socket(host, ctlport);
		} catch (UnknownHostException e) {
			System.err.println("Don't know about host: " + host);
			_sErrMsg = _resBundleMessages.getString("unknownServer") + "\n" ;
			_bFailed = true;
			return;
		} catch (IOException e) {
			System.err.println("Couldn't get the connection to: " + host + " " +ctlport);
			_sErrMsg = _resBundleMessages.getString("serverNotRunning") + " (" + host + ":" + ctlport + ")\n" ;
			_bFailed = true;
			return;
		}
		
		
		Protocol protocolObj = new Protocol(ctlSocket);
		Message msg = new Message();

		/* The beginning of the protocol */
		
		//Determine, and indicate to client about Inet6/4 address being used
		if (ctlSocket.getInetAddress() instanceof Inet6Address) {
			_resultsTxtPane.append(_resBundleMessages.getString("connected") + " " + host + _resBundleMessages.getString("usingIpv6") + "\n");
		}
		else {
			_resultsTxtPane.append(_resBundleMessages.getString("connected") + " " + host + _resBundleMessages.getString("usingIpv4") + "\n");
		}

		/* write our test suite request by sending a login message
		_yTests indicates the requested test-suite
		*/
		protocolObj.send_msg(MessageType.MSG_LOGIN, _yTests);
		/* read the specially crafted data that kicks off the old clients */
		if (protocolObj.readn(msg, 13) != 13) {
			_sErrMsg = _resBundleMessages.getString("unsupportedClient") + "\n";
			_bFailed = true;
			return;
		}

		for (;;) {
			//If SRV_QUEUE message sent by NDT server does not indicate that the test session starts now, return
			//if (protocolObj.recv_msg(msg) != 0) { //comment to replace with constant
			if (protocolObj.recv_msg(msg) != NDTConstants.SRV_QUEUE_TEST_STARTS_NOW) {
				_sErrMsg = _resBundleMessages.getString("protocolError") + 
						Integer.parseInt(new String(msg.getBody()), 16) + " instead\n";
				_bFailed = true;
				return;
			}
			
			//SRV_QUEUE messages are only sent to queued clients with if the test can be started immediately. 
			//Any other type of message at this stage is incorrect			
			if (msg.getType() != MessageType.SRV_QUEUE) {
				_sErrMsg = _resBundleMessages.getString("loggingWrongMessage") + "\n";
				_bFailed = true;
				return;
			}	
			
			String tmpstr3 = new String(msg.getBody());
			wait = Integer.parseInt(tmpstr3);
			System.out.println("wait flag received = " + wait);

			
			if (wait == 0) { /* SRV_QUEUE message received indicating "ready to start tests" status,
								proceed to running tests */			 
				break;
			}

			//if (wait == 9988) {
			if (wait == NDTConstants.SRV_QUEUE_SERVER_BUSY) {
				if (iServerWaitFlag == 0) { //First message from server, indicating server is busy. Quit
					_sErrMsg = _resBundleMessages.getString("serverBusy") + "\n";
					_bFailed = true;
					return;
				} else {    //Server fault, quit qithout further ado
					_sErrMsg = _resBundleMessages.getString("serverFault") + "\n";
					_bFailed = true;
					return;
				}
			}

			//if (wait == 9999) {
			//server busy, wait 60 s for previous test to finish
			if ( wait == NDTConstants.SRV_QUEUE_SERVER_BUSY_60s ) {
				_sErrMsg = _resBundleMessages.getString("serverBusy60s") + "\n";
				_bFailed = true;
				return;
			}

			//if (wait == 9990) {  // signal from the server to see if the client is still alive
			if (wait == NDTConstants.SRV_QUEUE_HEARTBEAT) {  // signal from the server to see if the client is still alive
				// Client has to respond with a "MSG_WAITING" to such heart-beat messages from server
				protocolObj.send_msg(MessageType.MSG_WAITING, _yTests);
				continue;
			}

			// Each test should take less than 30 seconds, so tell them 45 sec * number of 
			// tests in the queue.
			wait = (wait * 45);
			_resultsTxtPane.append(_resBundleMessages.getString("otherClient") + wait + _resBundleMessages.getString("seconds") +".\n");
			iServerWaitFlag = 1; //first message from server already encountered
		} //end waiting 

		_frameWeb100Vars.toBack();
		_frameDetailedStats.toBack();
		
		/*Tests can be started. Read server response again.
		 * The server must send a message to verify version, and this is
		 * a MSG_LOGIN type of message
		*/
		if (protocolObj.recv_msg(msg) != 0) { //it not read correctly, it is protocol error
			_sErrMsg = _resBundleMessages.getString("protocolError") + Integer.parseInt(new String(msg.getBody()), 16) + " instead\n";
			_bFailed = true;
			return;
		}
		if (msg.getType() != MessageType.MSG_LOGIN) { //Only this type of message is expected at this stage
													  //...every othr message type is "wrong"
			_sErrMsg = _resBundleMessages.getString("versionWrongMessage") + "\n";
			_bFailed = true;
			return;
		}

		//Version compatibility between server-client must be verified
		String vVersion = new String(msg.getBody());
		if (!vVersion.startsWith("v")) {
			_sErrMsg = _resBundleMessages.getString("incompatibleVersion");
			_bFailed = true;
			return;
		}
		System.out.println("Server version: " + vVersion.substring(1));

		/* Read server message again. Server must send a message to negotiate the test suite, and this is
		 * a MSG_LOGIN type of message which indicates the same set of tests as requested by the client earlier */
		if (protocolObj.recv_msg(msg) != 0) {
			_sErrMsg = _resBundleMessages.getString("protocolError") + Integer.parseInt(new String(msg.getBody()), 16) + " instead\n";
			_bFailed = true;
			return;
		}
		if (msg.getType() != MessageType.MSG_LOGIN) { // Only tests-negotiation message expected at this stage.
			  										  //    ..Every other message type is "wrong"
			_sErrMsg = _resBundleMessages.getString("testsuiteWrongMessage") + "\n";
			_bFailed = true;
			return;
		}
		
		//get ids of tests to be run now
		StringTokenizer tokenizer = new StringTokenizer(new String(msg.getBody()), " ");
		
		/* Run all tests requested, based on the ID. In each case, if tests cannot be successfully run,
		indicate reason */
		while (tokenizer.hasMoreTokens()) {
			if (sPanel.wantToStop()) { //user has indicated decision to stop tests from GUI
				protocolObj.send_msg(MessageType.MSG_ERROR, "Manually stopped by the user".getBytes());
				protocolObj.close();
				ctlSocket.close();
				_sErrMsg = "\n" + _resBundleMessages.getString("stopped") + "\n";
				_bFailed = true;
				return;
			}
			int testId = Integer.parseInt(tokenizer.nextToken());
			switch (testId) {
			case NDTConstants.TEST_MID:
				sPanel.setText(_resBundleMessages.getString("middlebox"));
				if (test_mid(protocolObj)) {
					_resultsTxtPane.append(_sErrMsg);
					_resultsTxtPane.append(_resBundleMessages.getString("middleboxFail2") + "\n");
					_yTests &= (~NDTConstants.TEST_MID);
				}
				break;
			case NDTConstants.TEST_SFW:
				sPanel.setText(_resBundleMessages.getString("simpleFirewall"));
				if (test_sfw(protocolObj)) {
					_resultsTxtPane.append(_sErrMsg);
					_resultsTxtPane.append(_resBundleMessages.getString("sfwFail") + "\n");
					_yTests &= (~NDTConstants.TEST_SFW);
				}
				break;
			case NDTConstants.TEST_C2S:
				sPanel.setText(_resBundleMessages.getString("c2sThroughput"));
				if (test_c2s(protocolObj)) {
					_resultsTxtPane.append(_sErrMsg);
					_resultsTxtPane.append(_resBundleMessages.getString("c2sThroughputFailed") + "\n");
					_yTests &= (~NDTConstants.TEST_C2S);
				}
				break;
			case NDTConstants.TEST_S2C:
				sPanel.setText(_resBundleMessages.getString("s2cThroughput"));
				if (test_s2c(protocolObj, ctlSocket)) {
					_resultsTxtPane.append(_sErrMsg);
					_resultsTxtPane.append(_resBundleMessages.getString("s2cThroughputFailed") + "\n");
					_yTests &= (~NDTConstants.TEST_S2C);
				}
				break;
			case NDTConstants.TEST_META:
				sPanel.setText(_resBundleMessages.getString("meta"));
				if (test_meta(protocolObj)) {
					_resultsTxtPane.append(_sErrMsg);
					_resultsTxtPane.append(_resBundleMessages.getString("metaFailed") + "\n");
					_yTests &= (~NDTConstants.TEST_META);
				}
				break;
			default:
				_sErrMsg = _resBundleMessages.getString("unknownID") + "\n";
				_bFailed = true;
				return;
			}
		}
		
		if (sPanel.wantToStop()) { //user has indicated decision to stop tests from GUI
			protocolObj.send_msg(MessageType.MSG_ERROR, "Manually stopped by the user".getBytes());
			protocolObj.close();
			ctlSocket.close();
			_sErrMsg = _resBundleMessages.getString("stopped") + "\n";
			_bFailed = true;
			return;
		}

		sPanel.setText(_resBundleMessages.getString("receiving"));
		
		//Get results of tests
		i = 0;
		try {  
			for (;;) {
				if (protocolObj.recv_msg(msg) != 0) {
					_sErrMsg = _resBundleMessages.getString("protocolError") + Integer.parseInt(new String(msg.getBody()), 16) + " instead\n";
					_bFailed = true;
					return;
				}
				
				//results obtained. Log out message received now
				if (msg.getType() == MessageType.MSG_LOGOUT) {
					break;
				}
				
				//get results in the form of a human-readable string
				if (msg.getType() != MessageType.MSG_RESULTS) {
					_sErrMsg = _resBundleMessages.getString("resultsWrongMessage") + "\n";
					_bFailed = true;
					return;
				}
				_sTestResults += new String(msg.getBody());
				i++;
			}
		} catch (IOException e) {}

		//Timed-out while waiting for results
		if (i == 0) {
			_resultsTxtPane.append(_resBundleMessages.getString("resultsTimeout") + "\n");
		}
		System.err.println("Calling InetAddress.getLocalHost() twice");
		try {
			_txtDiagnosis.append(_resBundleMessages.getString("client") + ": " + InetAddress.getLocalHost() + "\n");
		} catch (SecurityException e) {
			_txtDiagnosis.append(_resBundleMessages.getString("client") + ": 127.0.0.1\n");
			_resultsTxtPane.append(_resBundleMessages.getString("unableToObtainIP") + "\n");
			System.err.println("Unable to obtain local IP address: using 127.0.0.1");
		}

		try {
			_sEmailText += _resBundleMessages.getString("client") + ": " + InetAddress.getLocalHost() + "\n%0A";
		} catch (SecurityException e) {
			_sEmailText += _resBundleMessages.getString("client") + ": " + "127.0.0.1" + "\n%0A";
		}

		//Final cleanup steps after completion of tests
		protocolObj.close();
		ctlSocket.close();
		//call testResults method
		try {
			testResults(_sTestResults);
		}
		catch (Exception ex) {
			_resultsTxtPane.append(_resBundleMessages.getString("resultsParseError")  + "\n");
			_resultsTxtPane.append(ex + "\n");
		}
		if ((_yTests & NDTConstants.TEST_MID) == NDTConstants.TEST_MID) {
			middleboxResults(_sMidBoxTestResult);
		}

		pub_isReady="yes";
		pub_errmsg ="All tests completed OK.";
		pub_status = "done";
	}

	
	
    /* 
     * Method that interprets test results
     * @param String containing all results
     * @return none
     * Done: SFW and some web100 variables
     * */
	public void testResults(String sTestResParam) {
		StringTokenizer tokens;
		int i=0;
		String sysvar, strval;
		int sysval, Zero=0, bwdelay, minwin;
		double sysval2, j;
		String osName, osArch, osVer, javaVer, javaVendor, client;

		tokens = new StringTokenizer(sTestResParam);
		sysvar = null;
		strval = null;
		while(tokens.hasMoreTokens()) {
			if(++i%2 == 1) {
				sysvar = tokens.nextToken();
			}
			else {
				strval = tokens.nextToken();
				_txtDiagnosis.append(sysvar + " " + strval + "\n");
				_sEmailText += sysvar + " " + strval + "\n%0A";
				if (strval.indexOf(".") == -1) { //no decimal point, hence integer
					sysval = Integer.parseInt(strval);
					//save value into a key value expected by us
					save_int_values(sysvar, sysval);
				}
				else { //if not integer, save as double
					sysval2 = Double.valueOf(strval).doubleValue();
					save_dbl_values(sysvar, sysval2);
				}
			}
		}

		// Grab some client details from the Applet environment
		osName = System.getProperty("os.name");
		pub_osName = osName;

		osArch = System.getProperty("os.arch");
		pub_osArch = osArch;

		osVer = System.getProperty("os.version");
		pub_osVer = osVer;

		javaVer = System.getProperty("java.version");
		pub_javaVer = javaVer;

		javaVendor = System.getProperty("java.vendor");

		if (osArch.startsWith("x86") == true) {
			client = _resBundleMessages.getString("pc");
		}
		else {
			client = _resBundleMessages.getString("workstation");
		}

		// Calculate some variables and determine path conditions
		// Note: calculations now done in server and the results are shipped
		//    back to the client for printing.

		if (_iCountRTT>0) {
			// Now write some _resBundleMessages to the screen
			if (_iC2sData < 3) {
				if (_iC2sData < 0) {
					_resultsTxtPane.append(_resBundleMessages.getString("unableToDetectBottleneck") + "\n");
					_sEmailText += "Server unable to determine bottleneck link type.\n%0A";
					pub_AccessTech = "Connection type unknown";

				} 
				else {
					_resultsTxtPane.append(_resBundleMessages.getString("your") + " " + client + " " + _resBundleMessages.getString("connectedTo") + " ");
					_sEmailText += _resBundleMessages.getString("your") + " " + client + " " + _resBundleMessages.getString("connectedTo") + " ";
					if (_iC2sData == 1) {
						_resultsTxtPane.append(_resBundleMessages.getString("dialup") + "\n");
						_sEmailText += _resBundleMessages.getString("dialup") +  "\n%0A";
						mylink = .064;
						pub_AccessTech = "Dial-up Modem";
					} 
					else {
						_resultsTxtPane.append(_resBundleMessages.getString("cabledsl") + "\n");
						_sEmailText += _resBundleMessages.getString("cabledsl") +  "\n%0A";
						mylink = 3;
						pub_AccessTech = "Cable/DSL modem";
					}
				}
			} 
			else {
				_resultsTxtPane.append(_resBundleMessages.getString("theSlowestLink") + " ");
				_sEmailText += _resBundleMessages.getString("theSlowestLink") + " ";
				if (_iC2sData == 3) {
					_resultsTxtPane.append(_resBundleMessages.getString("10mbps") + "\n");
					_sEmailText += _resBundleMessages.getString("10mbps") + "\n%0A";
					mylink = 10;
					pub_AccessTech = "10 Mbps Ethernet";
				} 
				else if (_iC2sData == 4) {
					_resultsTxtPane.append(_resBundleMessages.getString("45mbps") + "\n");
					_sEmailText += _resBundleMessages.getString("45mbps") + "\n%0A";
					mylink = 45;
					pub_AccessTech = "45 Mbps T3/DS3 subnet";
				} 
				else if (_iC2sData == 5) {
					_resultsTxtPane.append("100 Mbps ");
					_sEmailText += "100 Mbps ";
					mylink = 100;
					pub_AccessTech = "100 Mbps Ethernet";

					if (half_duplex == 0) {
						_resultsTxtPane.append(_resBundleMessages.getString("fullDuplex") + "\n");
						_sEmailText += _resBundleMessages.getString("fullDuplex") + "\n%0A";
					} 
					else {
						_resultsTxtPane.append(_resBundleMessages.getString("halfDuplex") + "\n");
						_sEmailText += _resBundleMessages.getString("halfDuplex") + "\n%0A";
					}
				} 
				else if (_iC2sData == 6) {
					_resultsTxtPane.append(_resBundleMessages.getString("622mbps") + "\n");
					_sEmailText += _resBundleMessages.getString("622mbps") + "\n%0A";
					mylink = 622;
					pub_AccessTech = "622 Mbps OC-12";
				} 
				else if (_iC2sData == 7) {
					_resultsTxtPane.append(_resBundleMessages.getString("1gbps") + "\n");
					_sEmailText += _resBundleMessages.getString("1gbps") + "\n%0A";
					mylink = 1000;
					pub_AccessTech = "1.0 Gbps Gigabit Ethernet";
				} 
				else if (_iC2sData == 8) {
					_resultsTxtPane.append(_resBundleMessages.getString("2.4gbps") + "\n");
					_sEmailText += _resBundleMessages.getString("2.4gbps") + "\n%0A";
					mylink = 2400;
					pub_AccessTech = "2.4 Gbps OC-48";
				} 
				else if (_iC2sData == 9) {
					_resultsTxtPane.append(_resBundleMessages.getString("10gbps") + "\n");
					_sEmailText += _resBundleMessages.getString("10gbps") + "\n%0A";
					mylink = 10000;
					pub_AccessTech = "10 Gigabit Ethernet/OC-192";

				}
			}

			if (mismatch == 1) {
				_resultsTxtPane.append(_resBundleMessages.getString("oldDuplexMismatch") + "\n");
				_sEmailText += _resBundleMessages.getString("oldDuplexMismatch") + "\n%0A";
			}
			else if (mismatch == 2) {
				_resultsTxtPane.append(_resBundleMessages.getString("duplexFullHalf") + "\n");
				_sEmailText += _resBundleMessages.getString("duplexFullHalf") + "\n%0A";
			}
			else if (mismatch == 4) {
				_resultsTxtPane.append(_resBundleMessages.getString("possibleDuplexFullHalf") + "\n");
				_sEmailText += _resBundleMessages.getString("possibleDuplexFullHalf") + "\n%0A";
			}
			else if (mismatch == 3) {
				_resultsTxtPane.append(_resBundleMessages.getString("duplexHalfFull") + "\n");
				_sEmailText += _resBundleMessages.getString("duplexHalfFull") + "\n%0A";
			}
			else if (mismatch == 5) {
				_resultsTxtPane.append(_resBundleMessages.getString("possibleDuplexHalfFull") + "\n");
				_sEmailText += _resBundleMessages.getString("possibleDuplexHalfFull") + "\n%0A";
			}
			else if (mismatch == 7) {
				_resultsTxtPane.append(_resBundleMessages.getString("possibleDuplexHalfFullWarning") + "\n");
				_sEmailText += _resBundleMessages.getString("possibleDuplexHalfFullWarning") + "\n%0A";
			}

			if (mismatch == 0) {
				if (bad_cable == 1) {
					_resultsTxtPane.append(_resBundleMessages.getString("excessiveErrors ") + "\n");
					_sEmailText += _resBundleMessages.getString("excessiveErrors") + "\n%0A";
				}
				if (congestion == 1) {
					_resultsTxtPane.append(_resBundleMessages.getString("otherTraffic") + "\n");
					_sEmailText += _resBundleMessages.getString("otherTraffic") + "\n%0A";
				}
				if (((2*rwin)/rttsec) < mylink) {
					j = (float)((mylink * avgrtt) * 1000) / 8 / 1024;
					if (j > (float)_iMaxRwinRcvd) {
						_resultsTxtPane.append(_resBundleMessages.getString("receiveBufferShouldBe") + " " + prtdbl(j) + _resBundleMessages.getString("toMaximizeThroughput") + " \n");
						_sEmailText += _resBundleMessages.getString("receiveBufferShouldBe") + " " + prtdbl(j) + _resBundleMessages.getString("toMaximizeThroughput") + "\n%0A";
					}
				}
			}

			if ((_yTests & NDTConstants.TEST_C2S) == NDTConstants.TEST_C2S) {
				if (_dSc2sspd < (_dC2sspd  * (1.0 - NDTConstants.VIEW_DIFF))) {
					// TODO:  distinguish the host buffering from the middleboxes buffering
					JLabel info = new JLabel(_resBundleMessages.getString("information"));
					info.addMouseListener(new MouseAdapter()
					{
						public void mouseClicked(MouseEvent e) {
							showBufferedBytesInfo();
						}
					});
					info.setForeground(Color.BLUE);
					info.setCursor(new Cursor(Cursor.HAND_CURSOR));
					info.setAlignmentY((float) 0.8);
					_resultsTxtPane.insertComponent(info);
					_resultsTxtPane.append(_resBundleMessages.getString("c2sPacketQueuingDetected") + "\n");
				}
			}

			if ((_yTests & NDTConstants.TEST_S2C) == NDTConstants.TEST_S2C) {
				if (_dS2cspd < (_dSs2cspd  * (1.0 - NDTConstants.VIEW_DIFF))) {
					// TODO:  distinguish the host buffering from the middleboxes buffering
					JLabel info = new JLabel(_resBundleMessages.getString("information"));
					info.addMouseListener(new MouseAdapter()
					{

						public void mouseClicked(MouseEvent e) {
							showBufferedBytesInfo();
						}

					});
					info.setForeground(Color.BLUE);
					info.setCursor(new Cursor(Cursor.HAND_CURSOR));
					info.setAlignmentY((float) 0.8);
					_resultsTxtPane.insertComponent(info);
					_resultsTxtPane.append(_resBundleMessages.getString("s2cPacketQueuingDetected") + "\n");
				}
			}

			//Get client information obtained from META tests
			_txtStatistics.append("\n\t------  " + _resBundleMessages.getString("clientInfo") + "------\n");
			_txtStatistics.append(_resBundleMessages.getString("osData") + " " + _resBundleMessages.getString("name") + " = " + osName + ", " + _resBundleMessages.getString("architecture") + " = " + osArch);
			_txtStatistics.append(", " + _resBundleMessages.getString("version") + " = " + osVer + "\n");
			_txtStatistics.append(_resBundleMessages.getString("javaData") + ": " +  _resBundleMessages.getString("vendor") + " = " + javaVendor + ", " + _resBundleMessages.getString("version") + " = " + javaVer + "\n");
			// statistics.append(" java.class.version=" + System.getProperty("java.class.version") + "\n");

			_txtStatistics.append("\n\t------  " + _resBundleMessages.getString("web100Details") + "  ------\n");
			/*
			if (c2sData == -2)
				_txtStatistics.append(_resBundleMessages.getString("insufficient") + "\n");
			else if (c2sData == -1)
				_txtStatistics.append(_resBundleMessages.getString("ipcFail") + "\n");
			else if (c2sData == 0)
				_txtStatistics.append(_resBundleMessages.getString("rttFail") + "\n");
			else if (c2sData == 1)
				_txtStatistics.append(_resBundleMessages.getString("foundDialup") + "\n");
			else if (c2sData == 2)
				_txtStatistics.append(_resBundleMessages.getString("foundDsl") + "\n");
			else if (c2sData == 3)
				_txtStatistics.append(_resBundleMessages.getString("found10mbps") + "\n");
			else if (c2sData == 4)
				_txtStatistics.append(_resBundleMessages.getString("found45mbps") + "\n");
			else if (c2sData == 5)
				_txtStatistics.append(_resBundleMessages.getString("found100mbps") + "\n");
			else if (c2sData == 6)
				_txtStatistics.append(_resBundleMessages.getString("found622mbps") + "\n");
			else if (c2sData == 7)
				_txtStatistics.append(_resBundleMessages.getString("found1gbps") + "\n");
			else if (c2sData == 8)
				_txtStatistics.append(_resBundleMessages.getString("found2.4gbps") + "\n");
			else if (c2sData == 9)
				_txtStatistics.append(_resBundleMessages.getString("found10gbps") + "\n");
			*/
			//replace by switch
			//TODO define names for these constants. What is 6/8/9 and so on
			switch (_iC2sData) {
			case -2:
				_txtStatistics.append(_resBundleMessages.getString("insufficient") + "\n");
				break;
			case -1:
				_txtStatistics.append(_resBundleMessages.getString("ipcFail") + "\n");	
				break;
			case 0:
				_txtStatistics.append(_resBundleMessages.getString("rttFail") + "\n");
				break;
			case 1:
				_txtStatistics.append(_resBundleMessages.getString("foundDialup") + "\n");
				break;
			case 2:
				_txtStatistics.append(_resBundleMessages.getString("foundDsl") + "\n");
				break;
			case 3:
				_txtStatistics.append(_resBundleMessages.getString("found10mbps") + "\n");
				break;
			case 4:
				_txtStatistics.append(_resBundleMessages.getString("found45mbps") + "\n");
				break;
			case 5:
				_txtStatistics.append(_resBundleMessages.getString("found100mbps") + "\n");
				break;
			case 6:
				_txtStatistics.append(_resBundleMessages.getString("found622mbps") + "\n");
				break;
			case 7:			
				_txtStatistics.append(_resBundleMessages.getString("found1gbps") + "\n");		
				break;
			case 8:
				_txtStatistics.append(_resBundleMessages.getString("found2.4gbps") + "\n");		
				break;
			case 9:
				_txtStatistics.append(_resBundleMessages.getString("found10gbps") + "\n");
				break;
			}

			if (half_duplex == 0)
				_txtStatistics.append(_resBundleMessages.getString("linkFullDpx") + "\n");
			else
				_txtStatistics.append(_resBundleMessages.getString("linkHalfDpx") + "\n");

			if (congestion == 0)
				_txtStatistics.append(_resBundleMessages.getString("congestNo") + "\n");
			else
				_txtStatistics.append(_resBundleMessages.getString("congestYes") + "\n");

			if (bad_cable == 0)
				_txtStatistics.append(_resBundleMessages.getString("cablesOk") + "\n");
			else
				_txtStatistics.append(_resBundleMessages.getString("cablesNok") + "\n");

			if (mismatch == 0)
				_txtStatistics.append(_resBundleMessages.getString("duplexOk") + "\n");
			else if (mismatch == 1) {
				_txtStatistics.append(_resBundleMessages.getString("duplexNok") + " ");
				_sEmailText += _resBundleMessages.getString("duplexNok") + " ";
			}
			else if (mismatch == 2) {
				_txtStatistics.append(_resBundleMessages.getString("duplexFullHalf") + "\n");
				_sEmailText += _resBundleMessages.getString("duplexFullHalf") + "\n%0A ";
			}
			else if (mismatch == 3) {
				_txtStatistics.append(_resBundleMessages.getString("duplexHalfFull") + "\n");
				_sEmailText += _resBundleMessages.getString("duplexHalfFull") + "\n%0A ";
			}

			_txtStatistics.append("\n" + _resBundleMessages.getString("web100rtt") + " =  " + prtdbl(avgrtt) + " " + "ms" + "; ");
			_sEmailText += "\n%0A" +  _resBundleMessages.getString("web100rtt") + " = " + prtdbl(avgrtt) + " " + "ms" + "; ";

			_txtStatistics.append(_resBundleMessages.getString("packetsize") + " = " + _iCurrentMSS + " " + _resBundleMessages.getString("bytes") + "; " + _resBundleMessages.getString("and") + " \n");
			_sEmailText += _resBundleMessages.getString("packetsize") + " = " + _iCurrentMSS + " " + _resBundleMessages.getString("bytes") + "; " + _resBundleMessages.getString("and") + " \n%0A";

			if (_iPktsRetrans > 0) {
				_txtStatistics.append(_iPktsRetrans + " " + _resBundleMessages.getString("pktsRetrans"));
				_txtStatistics.append(", " + _iDupAcksIn + " " + _resBundleMessages.getString("dupAcksIn"));
				_txtStatistics.append(", " + _resBundleMessages.getString("and") + " " + _iSACKsRcvd + " " + _resBundleMessages.getString("sackReceived") + "\n");
				_sEmailText += _iPktsRetrans + " " + _resBundleMessages.getString("pktsRetrans");
				_sEmailText += ", " + _iDupAcksIn + " " + _resBundleMessages.getString("dupAcksIn");
				_sEmailText += ", " + _resBundleMessages.getString("and") + " " + _iSACKsRcvd + " " + _resBundleMessages.getString("sackReceived") + "\n%0A";
				if (_iTimeouts > 0) {
					_txtStatistics.append(_resBundleMessages.getString("connStalled") + " " + _iTimeouts + " " + _resBundleMessages.getString("timesPktLoss") + "\n");
				}

				_txtStatistics.append(_resBundleMessages.getString("connIdle") + " " + prtdbl(waitsec) + " " + _resBundleMessages.getString("seconds") + " (" + prtdbl((waitsec/timesec)*100) + _resBundleMessages.getString("pctOfTime") + ")\n");
				_sEmailText += _resBundleMessages.getString("connStalled") + " " + _iTimeouts + " " + _resBundleMessages.getString("timesPktLoss") + "\n%0A";
				_sEmailText += _resBundleMessages.getString("connIdle") + " " + prtdbl(waitsec) + " " + _resBundleMessages.getString("seconds") + " (" + prtdbl((waitsec/timesec)*100) + _resBundleMessages.getString("pctOfTime") + ")\n%0A";
			} 
			else if (_iDupAcksIn > 0) {
				_txtStatistics.append(_resBundleMessages.getString("noPktLoss1") + " - ");
				_txtStatistics.append(_resBundleMessages.getString("ooOrder") + " " + prtdbl(order*100) + _resBundleMessages.getString("pctOfTime") + "\n");
				_sEmailText += _resBundleMessages.getString("noPktLoss1") + " - ";
				_sEmailText += _resBundleMessages.getString("ooOrder") + " " + prtdbl(order*100) + _resBundleMessages.getString("pctOfTime") + "\n%0A";
			} 
			else {
				_txtStatistics.append(_resBundleMessages.getString("noPktLoss2") + ".\n");
				_sEmailText += _resBundleMessages.getString("noPktLoss2") + ".\n%0A";
			}

			if ((_yTests & NDTConstants.TEST_C2S) == NDTConstants.TEST_C2S) {
				if (_dC2sspd > _dSc2sspd) {
					if (_dSc2sspd < (_dC2sspd  * (1.0 - NDTConstants.VIEW_DIFF))) {
						_txtStatistics.append(_resBundleMessages.getString("c2s") + " " + _resBundleMessages.getString("qSeen") + ": " + prtdbl(100 * (_dC2sspd - _dSc2sspd) / _dC2sspd) + "%\n");
					}
					else {
						_txtStatistics.append(_resBundleMessages.getString("c2s") + " " + _resBundleMessages.getString("qSeen") + ": " + prtdbl(100 * (_dC2sspd - _dSc2sspd) / _dC2sspd) + "%\n");
					}
				}
			}

			if ((_yTests & NDTConstants.TEST_S2C) == NDTConstants.TEST_S2C) {
				if (_dSs2cspd > _dS2cspd) {
					if (_dSs2cspd < (_dSs2cspd  * (1.0 - NDTConstants.VIEW_DIFF))) {
						_txtStatistics.append(_resBundleMessages.getString("s2c") + " " + _resBundleMessages.getString("qSeen") + ": " + prtdbl(100 * (_dSs2cspd - _dS2cspd) / _dSs2cspd) + "%\n");
					}
					else {
						_txtStatistics.append(_resBundleMessages.getString("s2c") + " " + _resBundleMessages.getString("qSeen") + ": " + prtdbl(100 * (_dSs2cspd - _dS2cspd) / _dSs2cspd) + "%\n");
					}
				}
			}

			if (rwintime > .015) {
				_txtStatistics.append(_resBundleMessages.getString("thisConnIs") + " " + _resBundleMessages.getString("limitRx") + " " + prtdbl(rwintime*100) + _resBundleMessages.getString("pctOfTime") + ".\n");
				_sEmailText += _resBundleMessages.getString("thisConnIs") + " " + _resBundleMessages.getString("limitRx") + " " + prtdbl(rwintime*100) + _resBundleMessages.getString("pctOfTime") + ".\n%0A";
				pub_pctRcvrLimited = rwintime*100;

				// I think there is a bug here, it sometimes tells you to increase the buffer
				// size, but the new size is smaller than the current.

				if (((2*rwin)/rttsec) < mylink) {
					_txtStatistics.append("  " + _resBundleMessages.getString("incrRxBuf") + " (" + prtdbl(_iMaxRwinRcvd/1024) + " KB) " + _resBundleMessages.getString("willImprove") + "\n");
				}
			}
			if (sendtime > .015) {
				_txtStatistics.append(_resBundleMessages.getString("thisConnIs") + " " + _resBundleMessages.getString("limitTx") + " " + prtdbl(sendtime*100) + _resBundleMessages.getString("pctOfTime") + ".\n");
				_sEmailText += _resBundleMessages.getString("thisConnIs") + " " + _resBundleMessages.getString("limitTx") + " " + prtdbl(sendtime*100) + _resBundleMessages.getString("pctOfTime") + ".\n%0A";
				if ((2*(swin/rttsec)) < mylink) {
					_txtStatistics.append("  " + _resBundleMessages.getString("incrTxBuf") + " (" + prtdbl(_iSndbuf/2048) + " KB) " + _resBundleMessages.getString("willImprove") + "\n");
				}
			}
			if (cwndtime > .005) {
				_txtStatistics.append(_resBundleMessages.getString("thisConnIs") + " " + _resBundleMessages.getString("limitNet") + " " + prtdbl(cwndtime*100) + _resBundleMessages.getString("pctOfTime") + ".\n");
				_sEmailText += _resBundleMessages.getString("thisConnIs") + " " + _resBundleMessages.getString("limitNet") + " " + prtdbl(cwndtime*100) + _resBundleMessages.getString("pctOfTime") + ".\n%0A";
				// if (cwndtime > .15)
				//	statistics.append("  Contact your local network administrator to report a network problem\n");
				// if (order > .15)
				//	statistics.append("  Contact your local network admin and report excessive packet reordering\n");
			}
			if ((spd < 4) && (loss > .01)) {
				_txtStatistics.append(_resBundleMessages.getString("excLoss") + "\n");
			}

			_txtStatistics.append("\n" + _resBundleMessages.getString("web100tcpOpts") + " \n");
			_txtStatistics.append("RFC 2018 Selective Acknowledgment: ");
			if(_iSACKEnabled == Zero)
				_txtStatistics.append(_resBundleMessages.getString("off") + "\n");
			else
				_txtStatistics.append(_resBundleMessages.getString("on") + "\n");

			_txtStatistics.append("RFC 896 Nagle Algorithm: ");
			if(_iNagleEnabled == Zero)
				_txtStatistics.append(_resBundleMessages.getString("off") + "\n");
			else
				_txtStatistics.append(_resBundleMessages.getString("on") + "\n");

			_txtStatistics.append("RFC 3168 Explicit Congestion Notification: ");
			if(_iECNEnabled == Zero)
				_txtStatistics.append(_resBundleMessages.getString("off") + "\n");
			else
				_txtStatistics.append(_resBundleMessages.getString("on") + "\n");

			_txtStatistics.append("RFC 1323 Time Stamping: ");
			if(_iTimestampsEnabled == 0)
				_txtStatistics.append(_resBundleMessages.getString("off") + "\n");
			else
				_txtStatistics.append(_resBundleMessages.getString("on") + "\n");

			_txtStatistics.append("RFC 1323 Window Scaling: ");
			if (_iMaxRwinRcvd < 65535)
				_iWinScaleRcvd = 0;
			if((_iWinScaleRcvd == 0) || (_iWinScaleRcvd > 20))
				_txtStatistics.append(_resBundleMessages.getString("off") + "\n");
			else
				_txtStatistics.append (_resBundleMessages.getString("on") + "; " + _resBundleMessages.getString("scalingFactors") + " -  " + _resBundleMessages.getString("server") + "=" + _iWinScaleRcvd + ", " + _resBundleMessages.getString("client") + "=" + _iWinScaleSent + "\n");

			_txtStatistics.append("\n");

			//SFW test results
			if ((_yTests & NDTConstants.TEST_SFW) == NDTConstants.TEST_SFW) {
				//Results in the direction of Client to server
				switch (c2sResult) {
					//Update Statistics  and email-text based on results 
				case NDTConstants.SFW_NOFIREWALL:
					_txtStatistics.append(_resBundleMessages.getString("server") + " '" + host + "' " + _resBundleMessages.getString("firewallNo") + "\n");
					_sEmailText += _resBundleMessages.getString("server") + " '" + host + "' " + _resBundleMessages.getString("firewallNo") + "\n%0A";
					break;
				case NDTConstants.SFW_POSSIBLE:
					_txtStatistics.append(_resBundleMessages.getString("server") + " '" + host + "' " + _resBundleMessages.getString("firewallYes") + "\n");
					_sEmailText += _resBundleMessages.getString("server") + " '" + host + "' " + _resBundleMessages.getString("firewallYes") + "\n%0A";
					break;
				case NDTConstants.SFW_UNKNOWN:
				case NDTConstants.SFW_NOTTESTED:
					break;
				}
				//Results in the direction of Server to client
				switch (s2cResult) {
					//Update Statistics  and email-text based on results
				case NDTConstants.SFW_NOFIREWALL:
					_txtStatistics.append(_resBundleMessages.getString("client2") + " " + _resBundleMessages.getString("firewallNo") + "\n");
					_sEmailText += _resBundleMessages.getString("client2") + " " + _resBundleMessages.getString("firewallNo") + "\n%0A";
					break;
				case NDTConstants.SFW_POSSIBLE:
					_txtStatistics.append(_resBundleMessages.getString("client2") + " " + _resBundleMessages.getString("firewallYes") + "\n");
					_sEmailText += _resBundleMessages.getString("client2") + " " + _resBundleMessages.getString("firewallYes") + "\n%0A";
					break;
				case NDTConstants.SFW_UNKNOWN:
				case NDTConstants.SFW_NOTTESTED:
					break;
				}
			}

			//			diagnosis.append("\nEstimate = " + prtdbl(estimate) + " based on packet size = "
			//				+ (CurrentMSS*8/1024) + "kbits, RTT = " + prtdbl(avgrtt) + "msec, " + "and loss = " + loss + "\n");
			_txtDiagnosis.append("\n");

			//Output relevant to the "More Details" tab, related to factors influencing throughput
			//Theoretical network limit
			_txtDiagnosis.append(_resBundleMessages.getString("theoreticalLimit") + " " + prtdbl(estimate) + " " + "Mbps\n");
			_sEmailText += _resBundleMessages.getString("theoreticalLimit") + " " + prtdbl(estimate) + " Mbps\n%0A";

			//NDT server buffer imposed limit
			_txtDiagnosis.append(_resBundleMessages.getString("ndtServerHas") + " " + prtdbl(_iSndbuf/2048) + " " + _resBundleMessages.getString("kbyteBufferLimits") + " " + prtdbl(swin/rttsec) + " Mbps\n");
			_sEmailText += _resBundleMessages.getString("ndtServerHas") + " " + prtdbl(_iSndbuf/2048) + " " + _resBundleMessages.getString("kbyteBufferLimits") + " " + prtdbl(swin/rttsec) + " Mbps\n%0A";

			//PC buffer imposed throughput limit
			_txtDiagnosis.append(_resBundleMessages.getString("yourPcHas") + " " + prtdbl(_iMaxRwinRcvd/1024) + " " + _resBundleMessages.getString("kbyteBufferLimits") + " " + prtdbl(rwin/rttsec) + " Mbps\n");
			_sEmailText += _resBundleMessages.getString("yourPcHas") + " " + prtdbl(_iMaxRwinRcvd/1024) + " " + _resBundleMessages.getString("kbyteBufferLimits") + " " + prtdbl(rwin/rttsec) + " Mbps\n%0A";

			//Network based flow control limit imposed throughput limit
			_txtDiagnosis.append(_resBundleMessages.getString("flowControlLimits") + " " +	prtdbl(cwin/rttsec) + " Mbps\n");
			_sEmailText += _resBundleMessages.getString("flowControlLimits") + " " +	prtdbl(cwin/rttsec) + " Mbps\n%0A";

			//Client, Server data reports on link capacity
			_txtDiagnosis.append("\n" + _resBundleMessages.getString("clientDataReports") + " '" + prttxt(_iC2sData) + "', " + _resBundleMessages.getString("clientAcksReport") + " '" + prttxt(_iC2sAck) + "'\n" + _resBundleMessages.getString("serverDataReports") + " '" + prttxt(_iS2cData) + "', " + _resBundleMessages.getString("serverAcksReport") + " '" + prttxt(_iS2cAck) + "'\n");
			pub_diagnosis = _txtDiagnosis.getText();


		}
	}  // testResults()



	/* This routine decodes the middlebox test results.  The data is returned
	 * from the server is a specific order.  This routine pulls the string apart
	 * and puts the values into the proper variable.  It then compares the values
	 * to known values and writes out the specific results.
	 *
	 * server data is first
	 * order is Server IP; Client IP; CurrentMSS; WinScaleRcvd; WinScaleSent;
	 * Client then adds
	 * Server IP; Client IP.
	 */

	public void middleboxResults(String sMidBoxTestResParam) {
		StringTokenizer tokens;
		int k;

		// results.append("Mbox stats: ");
		tokens = new StringTokenizer(sMidBoxTestResParam, ";");
		String ssip = tokens.nextToken();
		String scip = tokens.nextToken();

		// Fix for JS API not reporting NAT'd IPs correctly
		// Assign client and server IP addresses for JA API
		// based on public, not local IP.  
		pub_clientIP = scip;


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
			_txtStatistics.append(_resBundleMessages.getString("packetSizePreserved") + "\n");
		else
			_txtStatistics.append(_resBundleMessages.getString("middleboxModifyingMss") + "\n");

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
			_txtStatistics.append(_resBundleMessages.getString("serverIpPreserved") + "\n");
			pub_natBox = "no";
		}
		else {
			pub_natBox = "yes";
			_txtStatistics.append(_resBundleMessages.getString("serverIpModified") + "\n");
			_txtStatistics.append("\t" + _resBundleMessages.getString("serverSays") + " [" + ssip + "], " + _resBundleMessages.getString("clientSays") +" [" + csip + "]\n");
		}

		if (ccip.equals("127.0.0.1")) {
			_txtStatistics.append(_resBundleMessages.getString("clientIpNotFound") + "\n");
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
				_txtStatistics.append(_resBundleMessages.getString("clientIpPreserved") + "\n");
			else {
				_txtStatistics.append(_resBundleMessages.getString("clientIpModified") + "\n");
				_txtStatistics.append("\t" + _resBundleMessages.getString("serverSays") + " [" + scip + "], " + _resBundleMessages.getString("clientSays") +" [" + ccip + "]\n");
			}
		}
		pub_statistics = _txtStatistics.getText();
	} // middleboxResults()


	public void showBufferedBytesInfo() {
		JOptionPane.showMessageDialog(null, _resBundleMessages.getString("packetQueuingInfo"), _resBundleMessages.getString("packetQueuing"),
				JOptionPane.INFORMATION_MESSAGE);
	}
	
	
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
			str = _resBundleMessages.getString("systemFault");
		else if (val == 0)
			str = _resBundleMessages.getString("rtt");
		else if (val == 1)
			str = _resBundleMessages.getString("dialup2");
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
	/* Method to save double values of various "keys" from the 
	 * the test results string into corresponding double datatypes
	 * @param sysvar key name string
	 * @param sysval Value for this key name
	 * @returns none */
	public void save_dbl_values(String sysvar, double sysval) {
		if(sysvar.equals("bw:")) 
			estimate = sysval;
		else if(sysvar.equals("loss:")) { 
			loss = sysval;
			pub_loss = loss;
		}
		else if(sysvar.equals("avgrtt:")) {
			avgrtt = sysval;
			pub_avgrtt = avgrtt;
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
			pub_cwndtime=cwndtime;
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


	/* Method to save integer values of various "keys" from the 
	 * the test results string into corresponding integer datatypes
	 * @param sysvar String key name
	 * @param sysval Value for this key name
	 * @returns none */
	
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
			_iECNEnabled = sysval;
		else if(sysvar.equals("NagleEnabled:")) 
			_iNagleEnabled = sysval;
		else if(sysvar.equals("SACKEnabled:")) 
			_iSACKEnabled = sysval;
		else if(sysvar.equals("TimestampsEnabled:")) 
			_iTimestampsEnabled = sysval;
		else if(sysvar.equals("WinScaleRcvd:")) 
			_iWinScaleRcvd = sysval;
		else if(sysvar.equals("WinScaleSent:")) 
			_iWinScaleSent = sysval;
		else if(sysvar.equals("SumRTT:")) 
			_iSumRTT = sysval;
		else if(sysvar.equals("CountRTT:")) 
			_iCountRTT = sysval;
		else if(sysvar.equals("CurMSS:"))
			_iCurrentMSS = sysval;
		else if(sysvar.equals("Timeouts:")) 
			_iTimeouts = sysval;
		else if(sysvar.equals("PktsRetrans:")) 
			_iPktsRetrans = sysval;
		else if(sysvar.equals("SACKsRcvd:")) {
			_iSACKsRcvd = sysval;
			pub_SACKsRcvd = _iSACKsRcvd;
		}
		else if(sysvar.equals("DupAcksIn:")) 
			_iDupAcksIn = sysval;
		else if(sysvar.equals("MaxRwinRcvd:")) {
			_iMaxRwinRcvd = sysval;
			pub_MaxRwinRcvd=_iMaxRwinRcvd;
		}
		else if(sysvar.equals("MaxRwinSent:")) 
			_iMaxRwinSent = sysval;
		else if(sysvar.equals("Sndbuf:")) 
			_iSndbuf = sysval;
		else if(sysvar.equals("X_Rcvbuf:")) 
			_iRcvbuf = sysval;
		else if(sysvar.equals("DataPktsOut:")) 
			_iDataPktsOut = sysval;
		else if(sysvar.equals("FastRetran:")) 
			_iFastRetran = sysval;
		else if(sysvar.equals("AckPktsOut:")) 
			_iAckPktsOut = sysval;
		else if(sysvar.equals("SmoothedRTT:")) 
			_iSmoothedRTT = sysval;
		else if(sysvar.equals("CurCwnd:")) 
			_iCurrentCwnd = sysval;
		else if(sysvar.equals("MaxCwnd:")) 
			_iMaxCwnd = sysval;
		else if(sysvar.equals("SndLimTimeRwin:")) 
			_iSndLimTimeRwin = sysval;
		else if(sysvar.equals("SndLimTimeCwnd:")) 
			_iSndLimTimeCwnd = sysval;
		else if(sysvar.equals("SndLimTimeSender:")) 
			_iSndLimTimeSender = sysval;
		else if(sysvar.equals("DataBytesOut:")) 
			_iDataBytesOut = sysval;
		else if(sysvar.equals("AckPktsIn:")) 
			_iAckPktsIn = sysval;
		else if(sysvar.equals("SndLimTransRwin:"))
			_iSndLimTransRwin = sysval;
		else if(sysvar.equals("SndLimTransCwnd:"))
			_iSndLimTransCwnd = sysval;
		else if(sysvar.equals("SndLimTransSender:"))
			_iSndLimTransSender = sysval;
		else if(sysvar.equals("MaxSsthresh:"))
			_iMaxSsthresh = sysval;
		else if(sysvar.equals("CurRTO:")) {
			_iCurrentRTO = sysval;
			pub_CurRTO = _iCurrentRTO;
		}
		else if(sysvar.equals("MaxRTO:")) {
			pub_MaxRTO = sysval;
		}
		else if(sysvar.equals("MinRTO:")) {
			pub_MinRTO = sysval;
		}
		else if(sysvar.equals("MinRTT:")) {
			pub_MinRTT = sysval;
		}
		else if(sysvar.equals("MaxRTT:")) {
			pub_MaxRTT = sysval;
		}
		else if(sysvar.equals("CurRwinRcvd:")) {
			pub_CurRwinRcvd = sysval;
		}		
		else if(sysvar.equals("Timeouts:")) {
			pub_Timeouts = sysval;
		}	
		else if(sysvar.equals("c2sData:"))
			_iC2sData = sysval;
		else if(sysvar.equals("c2sAck:"))
			_iC2sAck = sysval;
		else if(sysvar.equals("s2cData:"))
			_iS2cData = sysval;
		else if(sysvar.equals("s2cAck:"))
			_iS2cAck = sysval;
		else if(sysvar.equals("PktsOut:"))
			_iPktsOut = sysval;
		else if(sysvar.equals("mismatch:")) {
			mismatch = sysval;
			pub_mismatch=mismatch;
		}
		else if(sysvar.equals("congestion:")) {
			congestion = sysval;
			pub_congestion = congestion;
		}
		else if(sysvar.equals("bad_cable:")) {
			bad_cable = sysval;
			pub_Bad_cable = bad_cable;
		}
		else if(sysvar.equals("half_duplex:"))
			half_duplex = sysval;
		else if(sysvar.equals("CongestionSignals:"))
			_iCongestionSignals = sysval;
		else if(sysvar.equals("RcvWinScale:")) {
			if (_iRcvWinScale > 15)
				_iRcvWinScale = 0;
			else
				_iRcvWinScale = sysval;
		}
	}  // save_int_values()

	
	
    /*psvm for running as an Applicaion*/
	public static void main(String[] args)
	{
		JFrame frame = new JFrame("ANL/Internet2 NDT (applet)");
		if (args.length != 1) {
			System.out.println("Usage: java -jar Tcpbw100.jar " + "HOST");
			System.exit(0);
		}
		final Tcpbw100 applet = new Tcpbw100();
		frame.addWindowListener(new WindowAdapter() {
			public void windowClosing(WindowEvent e) {
				applet.destroy();
				System.exit(0);
			}
		});
		applet._bIsApplication = true;
		applet.host = args[0];
		frame.getContentPane().add(applet);
		frame.setSize(700, 320);
		applet.init();
		applet.start();
		frame.setVisible(true);
	}
		

	/* Utility method to get parameter value
	 * @param Key String whose value has to be found 
	 * @return Value of key requested for */
	public String getParameter(String name) {
		if (!_bIsApplication) {
			return super.getParameter(name);
		}
		return null;
	}

	/* Class variable getter/setter methods */
	public int getC2STestResults() {
		return this.c2sResult;
	}
	
	public void setC2STestResults(int iParamC2SRes) {
		this.c2sResult = iParamC2SRes;
	}
	
	public int getS2CTestResults() {
		return this.s2cResult;
	}
	
	public void setS2CTestResults(int iParamS2CRes) {
		this.s2cResult = iParamS2CRes;
	}


} // class: Tcpbw100






