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

import net.measurementlab.ndt.NdtTests;
import net.measurementlab.ndt.UiServices;
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
import java.lang.reflect.*;
import javax.swing.JLabel;
import javax.swing.JApplet;
import javax.swing.JFrame;
import javax.swing.JTextArea;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.BorderFactory;
import javax.swing.JTextPane;
import javax.swing.text.StyledDocument;
import javax.swing.text.DefaultStyledDocument;
import javax.swing.text.BadLocationException;
import javax.swing.JOptionPane;
import javax.swing.BoxLayout;
import javax.swing.JSpinner;
import javax.swing.SpinnerNumberModel;
import javax.swing.JComboBox;
import javax.swing.JProgressBar;

// Workaround for remote JavaScript start method
import java.security.AccessController;
import java.security.PrivilegedAction;


public class Tcpbw100 extends JApplet implements ActionListener
{
  JTextArea diagnosis, statistics;
  MyTextPane results;
  JButton startTest;
  JButton disMiss, disMiss2;
  JButton copy, copy2;
  JButton deTails;
  JButton sTatistics;
  JButton mailTo;
  JButton options;
  JCheckBox defaultTest, preferIPv6;
  JSpinner numOfTests = new JSpinner();
  String[] delays = { "immediate", "1min","5mins","10mins","30mins","2hours","12hours","1day" };
  JComboBox delay;
  String host;

  // added for mailto url
  protected URL targetURL;
  private String TARGET1 = "U";
  private String TARGET2 = "H";
  String emailText;

  clsFrame f, ff, optionsFrame;

  boolean isApplication = false;
  boolean testInProgress = false;
  boolean cancopy;

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

  public void showStatus(String msg)
  {
    if (!isApplication) {
      super.showStatus(msg);
    }
  }

  public String getParameter(String name)
  {
    if (!isApplication) {
      return super.getParameter(name);
    }
    return null;
  }
  

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

      getContentPane().setLayout(new BorderLayout());
      showStatus(messages.getString("ready"));
      cancopy = false;
      results = new MyTextPane();
      results.append("TCP/Web100 Network Diagnostic Tool v" + NdtTests.VERSION + "\n");
      results.setEditable(false);
      getContentPane().add(new JScrollPane(results));
      results.append(messages.getString("clickStart") + "\n");
      Panel mPanel = new Panel();
      startTest = new JButton(messages.getString("start"));
      startTest.addActionListener(this);
      mPanel.add(startTest);
      sTatistics = new JButton(messages.getString("statistics"));
      sTatistics.addActionListener(this);
      if (getParameter("disableStatistics") == null) {
          mPanel.add(sTatistics);
      }
      sTatistics.setEnabled(false);
      deTails = new JButton(messages.getString("moreDetails"));
      deTails.addActionListener(this);
      if (getParameter("disableDetails") == null) {
          mPanel.add(deTails);
      }
      deTails.setEnabled(false);
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

	//Autorun functionality
    isAutoRun = getParameter("autoRun");
	if ((isAutoRun != null) && isAutoRun.equals("true")) {
		  pub_errmsg = "Test in progress.";
		  runtest();  
  	}

  }



class MyTextPane extends JTextPane
  {
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

  class TestWorker implements Runnable, UiServices
  {
    StatusPanel sPanel;

    public void run()
    {
      if (!testInProgress) {
        int testNo = 1;
        int testsNum = ((Integer)numOfTests.getValue()).intValue();
        testInProgress = true;
        diagnose();
        statistics();
        startTest.setEnabled(false);
        deTails.setEnabled(false);
        sTatistics.setEnabled(false);
        mailTo.setEnabled(false);
        options.setEnabled(false);
        numOfTests.setEnabled(false);
        sPanel = new StatusPanel(testsNum);
		getContentPane().add(BorderLayout.NORTH, sPanel);
        getContentPane().validate();
        getContentPane().repaint();

		if (preferIPv6.isSelected()) {
            try {
                System.setProperty("java.net.preferIPv6Addresses", "true");
            }
            catch (SecurityException e) {
                System.err.println("Couldn't set system property. Check your security settings.");
            }
        }
        preferIPv6.setEnabled(false);
        // TODO: detect network type
        NdtTests tests = new NdtTests(host, this, "unknown");
        while (true) {
            if (sPanel.wantToStop()) {
                break;
            }
            if (testsNum == 0) {
                results.append("\n** " + messages.getString("startingTest") + " " + testNo + " **\n");
            }
            else {
                results.append("\n** " + messages.getString("startingTest") + " " + testNo + " " + messages.getString("of") + " " + testsNum + " **\n");
            }
            tests.run();
            if (testNo == testsNum) {
                break;
            }
            if (sPanel.wantToStop()) {
                break;
            }
            emailText = tests.getEmailText();
            sPanel.setText("");
            sPanel.endTest();
            testNo += 1;
            deTails.setEnabled(true);
            sTatistics.setEnabled(true);
            mailTo.setEnabled(true);
            options.setEnabled(true);
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

        deTails.setEnabled(true);
        sTatistics.setEnabled(true);
        mailTo.setEnabled(true);
        options.setEnabled(true);
        numOfTests.setEnabled(true);
        showStatus(messages.getString("done2"));
        results.append("\n" + messages.getString("clickStart2") + "\n");
        startTest.setEnabled(true);
        testInProgress = false;
		getContentPane().remove(sPanel);
        getContentPane().validate();
        getContentPane().repaint();
      }
    }

    // Implementation of UiServices

    public void appendString(String str, int viewId) {
      switch (viewId) {
        case UiServices.MAIN_VIEW:
          results.append(str);
          break;
        case UiServices.STAT_VIEW:
          statistics.append(str);
          break;
        case UiServices.DIAG_VIEW:
          diagnosis.append(str);
          break;
        default:
          // Do nothing
      }
    }

    public void incrementProgress() {
      // TODO: implement
    }

    public void onBeginTest() {
    }

    public void onEndTest() {
    }

    public void onFailure(String errorMessage) {
      results.append(errorMessage);
      pub_isReady = "failed";
      pub_errmsg = errorMessage;
    }

    public void onPacketQueuingDetected() {
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
    }

    public void onLoginSent() {
      f.toBack();
      ff.toBack();
    }

    public boolean wantToStop() {
      return sPanel.wantToStop();
    }

    public void logError(String str) {
      System.err.println(str);
    }

    public void updateStatus(String status) {
      showStatus(status);
    }

    public void updateStatusPanel(String status) {
      sPanel.setText(status);
    }

    // Allows setting of the 'public' fields for the JavaScript API from the test thread
    public void setVariable(String name, int value) {
      try {
        getPubField(name).setInt(Tcpbw100.this, value);
      } catch (IllegalAccessException e) {
        System.err.println("Failed to set '" + name + "': " + e);
      }
    }

    public void setVariable(String name, double value) {
      try {
        getPubField(name).setDouble(Tcpbw100.this, value);
      } catch (IllegalAccessException e) {
        System.err.println("Failed to set '" + name + "':" + e);
      }
    }

    public void setVariable(String name, Object value) {
      try {
        getPubField(name).set(Tcpbw100.this, value);
      } catch (IllegalAccessException e) {
        System.err.println("Failed to set '" + name + "':" + e);
      }
    }

    private Field getPubField(String name) {
      if (!name.startsWith("pub_")) {
        throw new SecurityException("attempt to set non public variable");
      }
      try {
        return Tcpbw100.class.getField(name);
      } catch (NoSuchFieldException e) {
        throw new RuntimeException("Field '" + name + "' doesn't exist");
      }
    }

  }

	synchronized public void runtest() {
    new Thread(new TestWorker()).start();
	}

  public void showBufferedBytesInfo()
  {
      JOptionPane.showMessageDialog(null, messages.getString("packetQueuingInfo"), messages.getString("packetQueuing"),
      JOptionPane.INFORMATION_MESSAGE);
  }

	public void diagnose() {
		showStatus(messages.getString("getWeb100Var"));
		
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
		showStatus(messages.getString("printDetailedStats"));

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

} // class: Tcpbw100
