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
	private int _iTestsCompleted; //variable used to record the count of "finished" tests
	private int _iTestsNum;
	private boolean _bStop = false;

	private JLabel _labelTestNum = new JLabel();
	private JButton _buttonStop;
	private JProgressBar _progressBarObj = new JProgressBar();

	/* Constructor 
	 * @param testsNum Total number of tests scheduled to be run
	 * @param sParamaEnableMultiple Are multiple tests scheduled?*/
	StatusPanel(int iParamTestsNum, String sParamEnableMultiple) {
		this._iTestsCompleted = 1;
		this._iTestsNum = iParamTestsNum;

		setTestNoLabelText();
		//re-arch
		//If multiple tests are enabled to be run, then add information about the
		//test number being run
		if ( sParamEnableMultiple != null) {
			add(_labelTestNum);
		}
		/*
        if ( getParameter("enableMultipleTests") != null ) {
            add(testNoLabel);
        }*/
		_progressBarObj.setMinimum(0);
		_progressBarObj.setMaximum(_iTestsNum);
		_progressBarObj.setValue(0);
		_progressBarObj.setStringPainted(true);
		if (_iTestsNum == 0) {
			_progressBarObj.setString("");
			_progressBarObj.setIndeterminate(true);
		}
		else {
			_progressBarObj.setString(NDTConstants.getMessageString("initialization"));
		}
		add(_progressBarObj);
		_buttonStop= new JButton(NDTConstants.getMessageString("stop"));
		_buttonStop.addActionListener(new ActionListener() {

			public void actionPerformed(ActionEvent e) {
				_bStop = true;
				_buttonStop.setEnabled(false);
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
			add(_buttonStop);
		}
	}

	/*Set Test number being run*/
	private void setTestNoLabelText() {
		_labelTestNum.setText(NDTConstants.getMessageString("test") + " " + _iTestsCompleted + " " + NDTConstants.getMessageString("of") + " " +_iTestsNum);
	}

	/*record intention to stop tests */
	public boolean wantToStop() {
		return _bStop;
	}

	/*end the currently runnig test */
	public void endTest() {
		_progressBarObj.setValue(_iTestsCompleted);
		_iTestsCompleted++;
		setTestNoLabelText();
	}

	/* Set progress text */
	public void setText(String sParamText) {
		if (!_progressBarObj.isIndeterminate()) {
			_progressBarObj.setString(sParamText);
		}
	}
} //end class StatusPanel
