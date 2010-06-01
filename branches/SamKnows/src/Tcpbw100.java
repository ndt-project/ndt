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
import skin.Interface;
import skin.TextPane;
import skin.control.SkinPanel;

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
import javax.swing.*;
import javax.swing.text.StyledDocument;
import javax.swing.text.DefaultStyledDocument;
import javax.swing.text.BadLocationException;

// Workaround for remote JavaScript start method
import java.security.AccessController;
import java.security.PrivilegedAction;


public class Tcpbw100 extends JApplet implements ActionListener
{
  private static final String VERSION = "3.6.3";
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
  
  JTextArea diagnosis, statistics;
  MyTextPane results;
  String inresult, outresult, errmsg;
  JButton startTest;

  // BEGIN by prv
  JButton stopTest;
  // END by prv

  JButton disMiss, disMiss2;
  JButton copy, copy2;
  JButton deTails;
  JButton sTatistics;
  /* BEGIN by prv
  JButton mailTo;
  END by prv */
  JButton options;
  JCheckBox defaultTest, preferIPv6;
  JSpinner numOfTests = new JSpinner();
  String[] delays = { "immediate", "1min","5mins","10mins","30mins","2hours","12hours","1day" };
  JComboBox delay;

  boolean Randomize, failed, cancopy;
  URL location;
  clsFrame f, ff, optionsFrame;
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
  // added for mailto url
  protected URL targetURL;
  private String TARGET1 = "U";
  private String TARGET2 = "H";
  String emailText;
  double s2cspd, c2sspd, sc2sspd, ss2cspd;
  int ssndqueue;
  double sbytes;

  // BEGIN by prv
  private boolean _stop;
  // END by prv

  /*************************************************************************
   * JavaScript access API extension
   * Added by Seth Peery and Gregory Wilson, Virginia Tech
   * October 28, 2009
   * This section adds classwide variables, written to at runtime, 
   * which are then exposed by public accessor methods that can be called
   * from web applications using NDT as a back-end.  
   */

  private double pub_c2sspd = 0.0;
  private double pub_s2cspd = 0.0;
  private int pub_CurRwinRcvd = 0; // source variable does not exist
  private int pub_MaxRwinRcvd = 0;
  private int  pub_MinRTT = 0; // source variable does not exist
  private int  pub_MaxRTT = 0; // source variable does not exist
  private double pub_loss = 0.0;
  private double pub_avgrtt = 0.0;
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
  private int pub_jitter = 0;
  private int pub_Timeouts = 0;
  private String pub_errmsg = "Test not run.";
  private String isAutoRun;
  
  
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


/**
    End of accessor methods
**/	
  /************************************************************************/

  /**
   * Added by Martin Sandsmark, UNINETT AS
   * Internationalization
   */
  private Locale locale;
  private ResourceBundle messages;
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

  boolean isApplication = false;
  boolean testInProgress = false;
  String host = null;
  String tmpstr, tmpstr2;
  byte tests = TEST_MID | TEST_C2S | TEST_S2C | TEST_SFW | TEST_STATUS;
  int c2sResult = SFW_NOTTESTED;
  int s2cResult = SFW_NOTTESTED;

  // BEGIN by prv
  private SkinPanel skinPanel;
  // END by prv

  public void showStatus(String msg)
  {
    if (!isApplication) {
      super.showStatus(msg);
        // BEGIN by prv
        skinPanel.getStatusLabel().setText(msg);
        // END by prv
    }
  }

  public String getParameter(String name)
  {
    if (!isApplication) {
      return super.getParameter(name);
    }
    return null;
  }
  

  // BEGIN by prv
  public boolean wantToStop() {
    return _stop;
  }
  // END by prv

  public void init() {
      if (getParameter("country") != null) country = getParameter("country");
      if (getParameter("language") != null) lang = getParameter("language");

      try {
          locale = new Locale(lang, country);
          messages = ResourceBundle.getBundle("Tcpbw100_msgs", locale);
      } catch (Exception e) {
          JOptionPane.showMessageDialog(null, "Error while loading language files:\n" + e.getMessage());
          e.printStackTrace();
      }

      /* BEGIN by prv
      getContentPane().setLayout(new BorderLayout());
      showStatus(messages.getString("ready"));
      END by prv */
      failed = false ;
      Randomize = false;
      cancopy = false;
      /* BEGIN by prv
      results = new MyTextPane();
      END by prv */
      // BEGIN by prv
      Interface skin = Interface.getInstance();

      skinPanel = new SkinPanel(skin);
      Insets insets = skinPanel.getInsets();
      results = new MyTextPane(skin.getTextPane(), insets);
      // END by prv

      // BEGIN by prv
      startTest = skinPanel.getStartButton();
      startTest.addActionListener(this);
      stopTest = skinPanel.getStopButton();
      stopTest.setEnabled(false);

      stopTest.addActionListener(new ActionListener() {

          public void actionPerformed(ActionEvent e) {
              if (!_stop && JOptionPane.showConfirmDialog(null, "Are you sure?", "Stop test", JOptionPane.YES_NO_OPTION, JOptionPane.QUESTION_MESSAGE) == JOptionPane.YES_OPTION) {
                  stopTest.setEnabled(false);
                  _stop = true;
              }

          }

      });


      options = skinPanel.getAdvancedButton();
      options.addActionListener(new ActionListener() {

         public void actionPerformed(ActionEvent e) {
            options.setEnabled(false);
            showAdvancedPanel();

         }

      });

      setContentPane(skinPanel);
      // END by prv

      results.append("TCP/Web100 Network Diagnostic Tool v" + VERSION + "\n");
      /* BEGIN by prv
      results.setEditable(false);
      getContentPane().add(new JScrollPane(results));
      END by prv */
      results.append(messages.getString("clickStart") + "\n");
      /* BEGIN by prv
      Panel mPanel = new Panel();
      startTest = new JButton(messages.getString("start"));
      startTest.addActionListener(this);
      mPanel.add(startTest);
      END by prv */
      sTatistics = new JButton(messages.getString("statistics"));
      /* BEGIN by prv
      sTatistics.addActionListener(this);
      if (getParameter("disableStatistics") == null) {
          mPanel.add(sTatistics);
      }
      sTatistics.setEnabled(false);
      END by prv */
      deTails = new JButton(messages.getString("moreDetails"));
      /* BEGIN by prv
      deTails.addActionListener(this);
      if (getParameter("disableDetails") == null) {
          mPanel.add(deTails);
      }
      END by prv */
      deTails.setEnabled(false);
      /* BEGIN by prv
      mailTo = new JButton(messages.getString("reportProblem"));
      mailTo.addActionListener(this);
      if (getParameter("disableMailto") == null) {
          mPanel.add(mailTo);
      }
      mailTo.setEnabled(false);
      options = new JButton(messages.getString("options") + "...");
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
      END by prv */
      preferIPv6 = new JCheckBox(messages.getString("preferIPv6"));
      preferIPv6.setSelected(true);
      defaultTest = new JCheckBox(messages.getString("defaultTests"));
      defaultTest.setSelected(true);
      defaultTest.setEnabled(false);
      SpinnerNumberModel model = new SpinnerNumberModel();
      model.setMinimum(new Integer(0));
      model.setValue(new Integer(1));
      numOfTests.setModel(model);
      numOfTests.setPreferredSize(new Dimension(60, 20));
      delay = new JComboBox();
      for (int i = 0; i < delays.length; i++) {
              delay.addItem(messages.getString(delays[i]));
      }
      delay.setSelectedIndex(0);

	//Autorun functionality
    isAutoRun = getParameter("autoRun");
	if ((isAutoRun != null) && isAutoRun.equals("true")) {
		  pub_errmsg = "Test in progress.";
		  runtest();  
  	}

  }



class MyTextPane extends JTextPane
  {
    public MyTextPane(final TextPane skin, final Insets insets) {

        setEditable(false);

        final Dimension size = new Dimension(skin.getW(), skin.getH());
        setPreferredSize(size);
        setBounds(skin.getX() + insets.left, skin.getY() + insets.top,
                size.width, size.height);
    }

    public void append(String text)
    {
      try {
        getStyledDocument().insertString(getStyledDocument().getLength(), text, null);
      }
      catch (BadLocationException e) {
        System.out.println("WARNING: failed to append text to the text pane! [" + text + "]");
      }
    }

    public void insertComponent(Component c)
    {
      setSelectionStart(results.getStyledDocument().getLength());
      setSelectionEnd(results.getStyledDocument().getLength());
      super.insertComponent(c);
    }
  }

    private void showAdvancedPanel() {
        final JDialog dialog = new JDialog();
        dialog.setTitle("Advanced");
        JPanel panel = new JPanel(new BorderLayout());
        panel.add(BorderLayout.CENTER, new JScrollPane(results));
        JPanel buttonsPanel = new JPanel();

        final JButton optionsButton = new JButton("Options");
        optionsButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                showOptions();
            }
        });
        buttonsPanel.add(optionsButton);

        buttonsPanel.add(deTails);

        JButton btn = new JButton("Close");
        btn.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                dialog.dispose();
            }
        });
        buttonsPanel.add(btn);
        panel.add(BorderLayout.SOUTH, buttonsPanel);
        dialog.addWindowListener(new WindowAdapter() {
            @Override
            public void windowClosed(WindowEvent e) {
                options.setEnabled(true);
            }
        });
        dialog.setContentPane(panel);
        dialog.setSize(new Dimension(600, 400));
        dialog.setLocationRelativeTo(null);
        dialog.setDefaultCloseOperation(JDialog.DISPOSE_ON_CLOSE);
        dialog.setVisible(true);
    }

  class StatusPanel extends JPanel
  {
      private int _testNo;
      private int _testsNum;
      private boolean _stop = false;

      private JLabel testNoLabel = new JLabel();
      private JButton stopButton;
      private JProgressBar progressBar = new JProgressBar();

      StatusPanel(int testsNum) {
          this._testNo = 1;
          this._testsNum = testsNum;

          setTestNoLabelText();
          if (getParameter("enableMultipleTests") != null) {
              add(testNoLabel);
          }
          progressBar.setMinimum(0);
          progressBar.setMaximum(_testsNum);
          progressBar.setValue(0);
          progressBar.setStringPainted(true);
          if (_testsNum == 0) {
              progressBar.setString("");
              progressBar.setIndeterminate(true);
          }
          else {
              progressBar.setString(messages.getString("initialization"));
          }
          add(progressBar);
          stopButton= new JButton(messages.getString("stop"));
          stopButton.addActionListener(new ActionListener() {

              public void actionPerformed(ActionEvent e) {
                  _stop = true;
                  stopButton.setEnabled(false);
                  StatusPanel.this.setText(messages.getString("stopping"));
              }

          });
          if (getParameter("enableMultipleTests") != null) {
              add(stopButton);
          }
      }

      private void setTestNoLabelText() {
          testNoLabel.setText(messages.getString("test") + " " + _testNo + " " + messages.getString("of") + " " +_testsNum);
      }

      public boolean wantToStop() {
          return _stop;
      }

      public void endTest() {
          progressBar.setValue(_testNo);
          _testNo++;
          setTestNoLabelText();
      }

      public void setText(String text) {
          if (!progressBar.isIndeterminate()) {
              progressBar.setString(text);
          }
      }
  }

  class TestWorker implements Runnable
  {
    public void run()
    {
      if (!testInProgress) {
        int testNo = 1;
        int testsNum = ((Integer)numOfTests.getValue()).intValue();
        testInProgress = true;
        diagnose();
        statistics();
        startTest.setEnabled(false);
                // BEGIN by prv
                stopTest.setEnabled(true);
                _stop = false;
                // END by prv

        deTails.setEnabled(false);
        sTatistics.setEnabled(false);
          /* BEGIN by prv
        mailTo.setEnabled(false);
        options.setEnabled(false);
                END by prv */
        numOfTests.setEnabled(false);
        StatusPanel sPanel = new StatusPanel(testsNum);
		getContentPane().add(BorderLayout.NORTH, sPanel);
        getContentPane().validate();
        getContentPane().repaint();

        try {
            while (true) {
                /* BEGIN by prv
                if (sPanel.wantToStop()) {
                END by prv */
                if (sPanel.wantToStop() || wantToStop()) {
                    break;
                }
                if (testsNum == 0) {
                    results.append("\n** " + messages.getString("startingTest") + " " + testNo + " **\n");
                }
                else {
                    results.append("\n** " + messages.getString("startingTest") + " " + testNo + " " + messages.getString("of") + " " + testsNum + " **\n");
                }
                dottcp(sPanel);
                if (testNo == testsNum) {
                    break;
                }
                /* BEGIN by prv
                if (sPanel.wantToStop()) {
                END by prv */
                if (sPanel.wantToStop() || wantToStop()) {
                    break;
                }
                sPanel.setText("");
                sPanel.endTest();
                testNo += 1;
                deTails.setEnabled(true);
                sTatistics.setEnabled(true);
                /* BEGIN by prv
                mailTo.setEnabled(true);
                options.setEnabled(true);
                END by prv */
                statistics.append("\n** " + messages.getString("test") + " " + testNo + " **\n");
                diagnosis.append("\n** " + messages.getString("test") + " " + testNo + " **\n");
                try {
                    switch (delay.getSelectedIndex()) {
                        case 1:
                                results.append("\n** " + messages.getString("sleep1m") + " **\n");
                                Thread.sleep(1000 * 60);
                                break;
                        case 2:
                                results.append("\n** " + messages.getString("sleep5m") + " **\n");
                                Thread.sleep(1000 * 60 * 5);
                                break;
                        case 3:
                                results.append("\n** " + messages.getString("sleep10m") + " **\n");
                                Thread.sleep(1000 * 60 * 10);
                                break;
                        case 4:
                                results.append("\n** " + messages.getString("sleep30m") + " **\n");
                                Thread.sleep(1000 * 60 * 30);
                                break;
                        case 5:
                                results.append("\n** " + messages.getString("sleep2h") + " **\n");
                                Thread.sleep(1000 * 60 * 120);
                                break;
                        case 6:
                                results.append("\n** " + messages.getString("sleep12h") + " **\n");
                                Thread.sleep(1000 * 60 * 720);
                                break;
                        case 7:
                                results.append("\n** " + messages.getString("sleep1d") + " **\n");
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
          failed=true;
          errmsg = messages.getString("serverBusy30s") + "\n";
        }

        if (failed) {
          results.append(errmsg);
          // BEGIN by prv
          showStatus(Interface.getInstance().getStopString());
          // END by prv

		  pub_isReady = "failed";
		  pub_errmsg = errmsg;
        }
        // BEGIN by prv
        else {
          showStatus(Interface.getInstance().getCompleteTestString());
        }
        // END by prv

        deTails.setEnabled(true);
        sTatistics.setEnabled(true);
        /* BEGIN by prv
        mailTo.setEnabled(true);
        options.setEnabled(true);
        END by prv */
        // BEGIN by prv
        skinPanel.setValue1_(0);
        skinPanel.setValue2_(0);
        // END by prv
        numOfTests.setEnabled(true);
        /* BEGIN by prv
        showStatus(messages.getString("done2"));
        END by prv */
        results.append("\n" + messages.getString("clickStart2") + "\n");
        startTest.setEnabled(true);
        // BEGIN by prv
        stopTest.setEnabled(false);
        // END by prv
        testInProgress = false;
		getContentPane().remove(sPanel);
        getContentPane().validate();
        getContentPane().repaint();
      }
    }
  }

	synchronized public void runtest() {
    new Thread(new TestWorker()).start();
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
      /* BEGIN by prv
      showStatus(messages.getString("middleboxTest"));
      END by prv */
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
        System.err.println("Don't know about host: " + host);
        errmsg = messages.getString("unknownServer") + "\n" ;
        return true;
      } catch (IOException e) {
        System.err.println("Couldn't perform middlebox testing to: " + host);
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
	  pub_TimeStamp = new Date();

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
        System.err.println("Unable to obtain Servers IP addresses: using " + host);
        errmsg = "getInetAddress() called failed\n" ;
        tmpstr2 += host + ";";
        results.append(messages.getString("lookupError") + "\n");
      }

      System.err.println("calling in2Socket.getLocalAddress()");
      try {
        tmpstr2 += in2Socket.getLocalAddress() + ";";
      } catch (SecurityException e) {
        System.err.println("Unable to obtain local IP address: using 127.0.0.1");
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
      /* BEGIN by prv
      showStatus(messages.getString("sfwTest"));
      END by prv */
      showStatus(Interface.getInstance().getFirewallTestString());
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
      /* BEGIN by prv
      showStatus(messages.getString("outboundTest"));
      END by prv */
      showStatus(Interface.getInstance().getUploadTestString());
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
        System.err.println("Don't know about host: " + host);
        errmsg = messages.getString("unknownServer") + "\n" ;
        return true;
      } catch (IOException e) {
        System.err.println("Couldn't get 2nd connection to: " + host);
        errmsg = messages.getString("serverBusy15s") + "\n";
        return true;
      }

	// Get client and server IP addresses from the outSocket.
	pub_clientIP = outSocket.getLocalAddress().getHostAddress().toString();
	pub_host	 = outSocket.getInetAddress().getHostAddress().toString();


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
      System.err.println("Send buffer size =" + i);
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
        // BEGIN by prv
        if (System.currentTimeMillis() - t > 0) {
          skinPanel.setValue2((int) ((8.0 * pkts * lth) / (System.currentTimeMillis() - t)));
        }
        //END by prv
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

      // BEGIN by prv
      skinPanel.setValue2((int) (sc2sspd * 1000));
      // END by prv
      
		// Expose upload speed to JavaScript clients
           pub_c2sspd = sc2sspd;
    
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
        /* BEGIN by prv
        showStatus(messages.getString("inboundTest"));
        END by prv */
        showStatus(Interface.getInstance().getDownloadTestString());
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
        System.err.println("Don't know about host: " + host);
        errmsg = "unknown server\n" ;
        return true;
      } 
      catch (IOException e) {
        System.err.println("Couldn't get 3rd connection to: " + host);
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
          // BEGIN by prv
          if (System.currentTimeMillis() - t > 0)
          skinPanel.setValue1((int) ((8.0 * bytes) / (System.currentTimeMillis() - t)));
          // END by prv
        }
      } 
      catch (IOException e) {}

      t =  System.currentTimeMillis() - t;
      System.out.println(bytes + " bytes " + (8.0 * bytes)/t + " kb/s " + t/1000 + " secs");
      s2cspd = ((8.0 * bytes) / 1000) / t;

      // BEGIN by prv
      skinPanel.setValue1((int) ((8.0 * bytes) / t));
      // END by prv
      
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
           pub_s2cspd = s2cspd;

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

  public void dottcp(StatusPanel sPanel) throws IOException {
      Socket ctlSocket = null;
      if (!isApplication) {

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




      	int ctlport = 3001;
      	double wait2;
      	int sbuf, rbuf;
      	int i, wait, swait=0;

      	failed = false;
          
      try {
		  
		// RAC Debug message
        results.append(messages.getString("connectingTo") + " '" + host + "' [" + InetAddress.getByName(host) + "] " + messages.getString("toRunTest") + "\n");

		  if (preferIPv6.isSelected()) {
              try {
                  System.setProperty("java.net.preferIPv6Addresses", "true");
              }
              catch (SecurityException e) {
                  System.err.println("Couldn't set system property. Check your security settings.");
              }
          }
          preferIPv6.setEnabled(false);
          ctlSocket = new Socket(host, ctlport);
      } catch (UnknownHostException e) {
          System.err.println("Don't know about host: " + host);
          errmsg = messages.getString("unknownServer") + "\n" ;
          failed = true;
          return;
      } catch (IOException e) {
          System.err.println("Couldn't get the connection to: " + host + " " +ctlport);
          errmsg = messages.getString("serverNotRunning") + " (" + host + ":" + ctlport + ")\n" ;
          failed = true;
          return;
      }
      Protocol ctl = new Protocol(ctlSocket);
      Message msg = new Message();

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
          // BEGIN prv
          showStatus(Interface.getInstance().getWaitString());
          // END prv
	  swait = 1;
      }

      f.toBack();
      ff.toBack();

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
      StringTokenizer tokenizer = new StringTokenizer(new String(msg.body), " ");

      while (tokenizer.hasMoreTokens()) {
          /* BEGIN by prv
          if (sPanel.wantToStop()) {
          END by prv */
          if (sPanel.wantToStop() || wantToStop()) {
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
                  sPanel.setText(messages.getString("middlebox"));
                  if (test_mid(ctl)) {
                      results.append(errmsg);
                      results.append(messages.getString("middleboxFail2") + "\n");
                      tests &= (~TEST_MID);
                  }
                  break;
              case TEST_SFW:
                  sPanel.setText(messages.getString("simpleFirewall"));
                  if (test_sfw(ctl)) {
                      results.append(errmsg);
                      results.append(messages.getString("sfwFail") + "\n");
                      tests &= (~TEST_SFW);
                  }
                  break;
              case TEST_C2S:
                  sPanel.setText(messages.getString("c2sThroughput"));
                  if (test_c2s(ctl)) {
                      results.append(errmsg);
                      results.append(messages.getString("c2sThroughputFailed") + "\n");
                      tests &= (~TEST_C2S);
                  }
                  // BEGIN by prv
                  try {
                      Thread.sleep(3000);
                  } catch (InterruptedException e) {
                  }
                  skinPanel.setValue2_(0);
                  try {
                      Thread.sleep(1000);
                  } catch (InterruptedException e) {
                  }
                  // END by prv
                  break;
              case TEST_S2C:
                  // BEGIN by prv
                  try {
                      Thread.sleep(1000);
                  } catch (InterruptedException e) {
                  }
                  // END by prv
                  sPanel.setText(messages.getString("s2cThroughput"));
                  if (test_s2c(ctl, ctlSocket)) {
                      results.append(errmsg);
                      results.append(messages.getString("s2cThroughputFailed") + "\n");
                      tests &= (~TEST_S2C);
                  }
                  // BEGIN by prv
                  try {
                      Thread.sleep(1000);
                  } catch (InterruptedException e) {
                  }
                  // END by prv
                  break;
              default:
                  errmsg = messages.getString("unknownID") + "\n";
                  failed = true;
                  return;
          }
      }
      /* BEGIN by prv
      if (sPanel.wantToStop()) {
      END by prv */
      if (sPanel.wantToStop() || wantToStop()) {
          ctl.send_msg(MSG_ERROR, "Manually stopped by the user".getBytes());
          ctl.close();
          ctlSocket.close();
          errmsg = messages.getString("stopped") + "\n";
          failed = true;
          return;
      }

      sPanel.setText(messages.getString("receiving"));
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
      System.err.println("Calling InetAddress.getLocalHost() twice");
      try {
          diagnosis.append(messages.getString("client") + ": " + InetAddress.getLocalHost() + "\n");
      } catch (SecurityException e) {
          diagnosis.append(messages.getString("client") + ": 127.0.0.1\n");
          results.append(messages.getString("unableToObtainIP") + "\n");
          System.err.println("Unable to obtain local IP address: using 127.0.0.1");
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
	  
      // BEGIN by prv
      skinPanel.setRtt(avgrtt);
      skinPanel.setLoss(loss);

      StringBuilder sb = new StringBuilder();
      sb.append("sessionid=").append(URLEncoder.encode(getParameter("sessionid"), "UTF-8")).append('&');
      sb.append("avgrtt=").append(URLEncoder.encode(String.valueOf(avgrtt), "UTF-8")).append('&');
      sb.append("loss=").append(URLEncoder.encode(String.valueOf(loss), "UTF-8")).append('&');
      sb.append("upload=").append(URLEncoder.encode(String.valueOf(sc2sspd), "UTF-8")).append('&');
      //        sb.append("upload=").append(URLEncoder.encode(String.valueOf(c2sspd), "UTF-8")).append('&');
      sb.append("download=").append(URLEncoder.encode(String.valueOf(s2cspd), "UTF-8")).append('\n');
      System.out.println("Request: " + sb.toString());
      sendPostRequest(getParameter("resultServer"), sb.toString());
      // END by prv

	  pub_isReady="yes";
	  pub_errmsg ="All tests completed OK.";
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

  public void showBufferedBytesInfo()
  {
      JOptionPane.showMessageDialog(null, messages.getString("packetQueuingInfo"), messages.getString("packetQueuing"),
      JOptionPane.INFORMATION_MESSAGE);
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
		pub_osName = osName;
	    
		osArch = System.getProperty("os.arch");
		pub_osArch = osArch;
		
		osVer = System.getProperty("os.version");
        pub_osVer = osVer;
		
		javaVer = System.getProperty("java.version");
		pub_javaVer = javaVer;
		
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
					pub_AccessTech = "Connection type unknown";
					
				} 
				else {
					results.append(messages.getString("your") + " " + client + " " + messages.getString("connectedTo") + " ");
					emailText += messages.getString("your") + " " + client + " " + messages.getString("connectedTo") + " ";
					if (c2sData == 1) {
						results.append(messages.getString("dialup") + "\n");
						emailText += messages.getString("dialup") +  "\n%0A";
						mylink = .064;
						pub_AccessTech = "Dial-up Modem";
					} 
					else {
						results.append(messages.getString("cabledsl") + "\n");
						emailText += messages.getString("cabledsl") +  "\n%0A";
						mylink = 3;
						pub_AccessTech = "Cable/DSL modem";
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
					pub_AccessTech = "10 Mbps Ethernet";
				} 
				else if (c2sData == 4) {
					results.append(messages.getString("45mbps") + "\n");
					emailText += messages.getString("45mbps") + "\n%0A";
					mylink = 45;
					pub_AccessTech = "45 Mbps T3/DS3 subnet";
				} 
				else if (c2sData == 5) {
					results.append("100 Mbps ");
					emailText += "100 Mbps ";
					mylink = 100;
					pub_AccessTech = "100 Mbps Ethernet";
					
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
					pub_AccessTech = "622 Mbps OC-12";
				} 
				else if (c2sData == 7) {
					results.append(messages.getString("1gbps") + "\n");
					emailText += messages.getString("1gbps") + "\n%0A";
					mylink = 1000;
					pub_AccessTech = "1.0 Gbps Gigabit Ethernet";
				} 
				else if (c2sData == 8) {
					results.append(messages.getString("2.4gbps") + "\n");
					emailText += messages.getString("2.4gbps") + "\n%0A";
					mylink = 2400;
					pub_AccessTech = "2.4 Gbps OC-48";
				} 
				else if (c2sData == 9) {
					results.append(messages.getString("10gbps") + "\n");
					emailText += messages.getString("10gbps") + "\n%0A";
					mylink = 10000;
					pub_AccessTech = "10 Gigabit Ethernet/OC-192";
					
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
          JLabel info = new JLabel(messages.getString("information"));
          info.addMouseListener(new MouseAdapter()
              {

                public void mouseClicked(MouseEvent e) {
                  showBufferedBytesInfo();
                }

              });
          info.setForeground(Color.BLUE);
          info.setCursor(new Cursor(Cursor.HAND_CURSOR));
          info.setAlignmentY((float) 0.8);
          results.insertComponent(info);
          results.append(messages.getString("c2sPacketQueuingDetected") + "\n");
        }
      }
      
      if ((tests & TEST_S2C) == TEST_S2C) {
        if (s2cspd < (ss2cspd  * (1.0 - VIEW_DIFF))) {
          // TODO:  distinguish the host buffering from the middleboxes buffering
          JLabel info = new JLabel(messages.getString("information"));
          info.addMouseListener(new MouseAdapter()
              {

                public void mouseClicked(MouseEvent e) {
                  showBufferedBytesInfo();
                }

              });
          info.setForeground(Color.BLUE);
          info.setCursor(new Cursor(Cursor.HAND_CURSOR));
          info.setAlignmentY((float) 0.8);
          results.insertComponent(info);
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
				pub_pctRcvrLimited = rwintime*100;

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
			pub_natBox = "no";
		}
		else {
			pub_natBox = "yes";
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
			pub_SACKsRcvd = SACKsRcvd;
		}
		else if(sysvar.equals("DupAcksIn:")) 
			DupAcksIn = sysval;
		else if(sysvar.equals("MaxRwinRcvd:")) {
			MaxRwinRcvd = sysval;
			pub_MaxRwinRcvd=MaxRwinRcvd;
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
			pub_CurRTO = CurrentRTO;
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
			CongestionSignals = sysval;
		else if(sysvar.equals("RcvWinScale:")) {
			if (RcvWinScale > 15)
				RcvWinScale = 0;
			else
				RcvWinScale = sysval;
		}
	}  // save_int_values()


	public void diagnose() {
        /* BEGIN by prv
		showStatus(messages.getString("getWeb100Var"));
		END by prv */
		
		if (f == null) {
			f = new clsFrame();
		}
		f.setTitle(messages.getString("web100Var"));
		Panel buttons = new Panel();
		f.getContentPane().add("South", buttons);
		
		disMiss = new JButton(messages.getString("close"));
		disMiss.addActionListener(this);
		
		copy = new JButton(messages.getString("copy"));
		copy.addActionListener(this);
		
		diagnosis = new JTextArea(messages.getString("web100KernelVar") + ":\n", 15,30);
		diagnosis.setEditable(true);
		disMiss.setEnabled(true);
		copy.setEnabled(cancopy);
		
		buttons.add("West", disMiss);
		buttons.add("East", copy);
		f.getContentPane().add(new JScrollPane(diagnosis));
		f.pack();
	}  // diagnose()
	
	

	public void statistics() {
        /* BEGIN by prv
		showStatus(messages.getString("printDetailedStats"));
		END by prv */

		if (ff == null) {
			ff = new clsFrame();
		}
		ff.setTitle(messages.getString("detailedStats"));
		Panel buttons = new Panel();
		ff.getContentPane().add("South", buttons);
		
		disMiss2 = new JButton(messages.getString("close"));
		disMiss2.addActionListener(this);
		
		copy2 = new JButton(messages.getString("copy"));
		copy2.addActionListener(this);
		
		statistics = new JTextArea(messages.getString("web100Stats") + ":\n", 25,70);
		statistics.setEditable(true);
		disMiss2.setEnabled(true);
		copy2.setEnabled(cancopy);
		
		buttons.add("West", disMiss2);
		buttons.add("East", copy2);
		ff.getContentPane().add(new JScrollPane(statistics));
		ff.pack();
	}  // statistics()


  public void showOptions() {
    showStatus(messages.getString("showOptions"));

    if (optionsFrame == null) {
      optionsFrame = new clsFrame();
      optionsFrame.setTitle(messages.getString("options"));

      JPanel optionsPanel = new JPanel();
      optionsPanel.setLayout(new BoxLayout(optionsPanel, BoxLayout.Y_AXIS));

      JPanel testsPanel = new JPanel();
      testsPanel.setBorder(BorderFactory.createTitledBorder(messages.getString("performedTests")));
      testsPanel.add(defaultTest);
      optionsPanel.add(testsPanel);

      JPanel protocolPanel = new JPanel();
      protocolPanel.setBorder(BorderFactory.createTitledBorder(messages.getString("ipProtocol")));
      protocolPanel.add(preferIPv6);
      optionsPanel.add(protocolPanel);

      if (getParameter("enableMultipleTests") != null) {
          JPanel generalPanel = new JPanel();
          generalPanel.setLayout(new BoxLayout(generalPanel, BoxLayout.Y_AXIS));
          generalPanel.setBorder(BorderFactory.createTitledBorder(messages.getString("general")));
          JPanel tmpPanel = new JPanel();
          tmpPanel.add(new JLabel(messages.getString("numberOfTests") + ":"));
          tmpPanel.add(numOfTests);
          generalPanel.add(tmpPanel);
          tmpPanel = new JPanel();
          tmpPanel.add(new JLabel(messages.getString("delayBetweenTests") + ":"));
          tmpPanel.add(delay);
          generalPanel.add(tmpPanel);

          optionsPanel.add(generalPanel);
      }

      optionsFrame.getContentPane().add(optionsPanel);     
      Panel buttons = new Panel();
      optionsFrame.getContentPane().add("South", buttons);

      JButton okButton= new JButton(messages.getString("ok"));
      okButton.addActionListener(new ActionListener() {

        public void actionPerformed(ActionEvent e) {
          optionsFrame.toBack();
          optionsFrame.dispose();
        }
        
      });

      buttons.add("West", okButton);

      optionsFrame.pack();
    }
    optionsFrame.setResizable(false);
    optionsFrame.setVisible(true);
  }

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
            // BEGIN by prv
            _stop = false;
            skinPanel.setValue1(0);
            skinPanel.setValue2(0);
            skinPanel.setRtt(0f);
            skinPanel.setLoss(0f);
            showStatus(Interface.getInstance().getPreparingTestString());
            // END by prv

			pub_errmsg = "Test in progress.";
			runtest();
		}

		else if (source == deTails) {
			deTails.setEnabled(false);
			f.setResizable(true);
      f.setVisible(true);
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
      ff.setVisible(true);
			sTatistics.setEnabled(true);
		}

        /* BEGIN by prv
		else if (source == mailTo) {
			int i;
			char key;
			String to[], from[], comments[];
			String Name, Host;

			mailTo.setEnabled(false);
			// envoke mailto: function
			showStatus(messages.getString("invokingMailtoFunction") + "...");

			results.append(messages.getString("generatingReport") + "\n");
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
					subject = messages.getString("troubleReportFrom") + " " + getCodeBase().getHost();
				}
				theURL += "?subject=" + subject;
				theURL += "&body=" + messages.getString("comments") + ":%0A%0A" + emailText + " " + messages.getString("endOfEmail") + "\n%0A";
				// System.out.println("Message body is '" + emailText + "'\n");
				targetURL = new URL(theURL);

			} catch (MalformedURLException rsi) {
				throw new IllegalArgumentException("Can't create mailto: URL" + rsi.getMessage());
			}

			getAppletContext().showDocument(targetURL);
		}
		END by prv */
	}  // actionPerformed()


	public class clsFrame extends JFrame {
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
    applet.isApplication = true;
    applet.host = args[0];
    frame.getContentPane().add(applet);
    frame.setSize(700, 320);
    applet.init();
    applet.start();
    frame.setVisible(true);
  }

  // BEGIN by prv
  // send a POST request to Web server

  public void sendPostRequest(String url, String message) {
    try {
      System.out.println("Send report to: " + url);
      URLConnection connection = new URL(url).openConnection();
      connection.setRequestProperty("Content-Type", "application/x-www-form-urlencoded;charset=utf-8");
      connection.setDoOutput(true);
      BufferedWriter out = new BufferedWriter(new OutputStreamWriter(connection.getOutputStream()));
      out.write(message);
      out.flush();
      out.close();

      BufferedInputStream in = new BufferedInputStream(connection.getInputStream());
      while (in.read() != -1) ;
      System.out.println();
      in.close();

    } catch (Exception e) {
      e.printStackTrace();
    }
  }
  // END by prv
  
} // class: Tcpbw100
