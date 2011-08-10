import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JProgressBar;

/* Class that displays status of tests being run Status 
 * TODO: Can this be named better? */

public class StatusPanel extends JPanel
{
	private int _testNo;
	private int _testsNum;
	private boolean _stop = false;

	private JLabel testNoLabel = new JLabel();
	private JButton stopButton;
	private JProgressBar progressBar = new JProgressBar();

	/* Constructor 
	 * @param testsNum Total number of tests scheduled to be run
	 * @param sParamaEnableMultiple Are multiple tests scheduled?*/
	StatusPanel(int testsNum, String sParamEnableMultiple) {
		this._testNo = 1;
		this._testsNum = testsNum;

		setTestNoLabelText();
		//re-arch
		//If multiple tests are enabled to be run, then add information about the
		//test number being run
		if ( sParamEnableMultiple != null) {
			add(testNoLabel);
		}
		/*
        if ( getParameter("enableMultipleTests") != null ) {
            add(testNoLabel);
        }*/
		progressBar.setMinimum(0);
		progressBar.setMaximum(_testsNum);
		progressBar.setValue(0);
		progressBar.setStringPainted(true);
		if (_testsNum == 0) {
			progressBar.setString("");
			progressBar.setIndeterminate(true);
		}
		else {
			progressBar.setString(NDTConstants.getMessageString("initialization"));
		}
		add(progressBar);
		stopButton= new JButton(NDTConstants.getMessageString("stop"));
		stopButton.addActionListener(new ActionListener() {

			public void actionPerformed(ActionEvent e) {
				_stop = true;
				stopButton.setEnabled(false);
				StatusPanel.this.setText(NDTConstants.getMessageString("stopping"));
			}

		});
		/*
        if ( getParameter("enableMultipleTests") != null ) {
            add(stopButton);
        }*/
		//If multiple tests are enabled to be run, provide user option to 
		//	stop the one currently running
		if (sParamEnableMultiple != null) {
			add(stopButton);
		}
	}

	/*Set Test number being run*/
	private void setTestNoLabelText() {
		testNoLabel.setText(NDTConstants.getMessageString("test") + " " + _testNo + " " + NDTConstants.getMessageString("of") + " " +_testsNum);
	}

	/*record intention to stop tests */
	public boolean wantToStop() {
		return _stop;
	}

	/*end the currently runnig test */
	public void endTest() {
		progressBar.setValue(_testNo);
		_testNo++;
		setTestNoLabelText();
	}

	/* Set progress text */
	public void setText(String text) {
		if (!progressBar.isIndeterminate()) {
			progressBar.setString(text);
		}
	}
} //end class StatusPanel
