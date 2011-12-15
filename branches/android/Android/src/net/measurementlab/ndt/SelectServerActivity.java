package net.measurementlab.ndt;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.RadioGroup;

/**
 * User select the testing server here and pass the related value back to the
 * main activity.
 */
public class SelectServerActivity extends Activity implements
		RadioGroup.OnCheckedChangeListener {

	public static final String EXTRA_SERVER_NUMBER = "net.measurementlab.ndt.SelectedServer";

	public static final int ACTIVITY_SELECT_SERVER = 1;
	private int serverNo = 0;
	private RadioButton server[] = new RadioButton[SelectServerActivity.NUMBER_OF_SERVERS];
	private Button buttonOptionSave;
	private RadioGroup radioGroup;

	static final String SERVER_LIST[][] = {
	{"Closest Server (DONAR)", "ndt.iupui.donar.measurement-lab.org"}, 
	{"Amsterdam, The Netherlands", "ndt.iupui.ams.donar.measurement-lab.org"},
	{"Athens, Greece", "ndt.iupui.ath.donar.measurement-lab.org"},
	{"Atlanta, Georgia", "ndt.iupui.atl.donar.measurement-lab.org"},
	{"Chicago, Illinois", "ndt.iupui.ord.donar.measurement-lab.org"},
	{"Dallas, Texas", "ndt.iupui.dfw.donar.measurement-lab.org"},
	{"Dulles, Virginia", "ndt.iupui.iad.donar.measurement-lab.org"},
	{"Los Angeles, California", "ndt.iupui.lax.donar.measurement-lab.org"},
	{"London, United Kingdom", "ndt.iupui.lhr.donar.measurement-lab.org"},
	{"Miami, Florida", "ndt.iupui.mia.donar.measurement-lab.org"},
	{"New York City, New York", "ndt.iupui.lga.donar.measurement-lab.org"},
	{"Mountain View, California", "ndt.iupui.nuq.donar.measurement-lab.org"},
	{"Paris, France", "ndt.iupui.par.donar.measurement-lab.org"},
	{"Seattle, Washington", "ndt.iupui.sea.donar.measurement-lab.org"},
	{"Sydney, Australia", "ndt.iupui.syd.donar.measurement-lab.org"},
	{"Tokyo, Japan", "ndt.iupui.hnd.donar.measurement-lab.org"},
	{"Wellington, New Zealand", "ndt.iupui.wlg.donar.measurement-lab.org"}
	};

	/**
	 * Number of servers. All the arrays should have the same length.
	 */
	static final int NUMBER_OF_SERVERS = SERVER_LIST.length;

	// Options
	// See newest server list here:
	// http://www.measurementlab.net/measurement-lab-tools#ndt
	// All the data here should be kept sync with the published list
	// In future server list should be downloaded from some list servers
	// dynamically.
	static final int DEFAULT_SERVER = 0;

	/**
	 * Reads the server list from Constants class. Records the user's choice and
	 * send it back to the main activity.
	 */
	@Override
	public void onCreate(Bundle icicle) {
		super.onCreate(icicle);
		setContentView(R.layout.servers);
		Bundle serverInfo = getIntent().getExtras();
		if (serverInfo != null) {
			serverNo = serverInfo.getInt(EXTRA_SERVER_NUMBER);
			Log.v("NDT", "Passed Into Option: " + serverNo);
		}
		radioGroup = (RadioGroup) findViewById(R.id.servergroup);
		buttonOptionSave = (Button) findViewById(R.id.ButtonServerSave);
		for (int i = 0; i < SelectServerActivity.NUMBER_OF_SERVERS; i++) {
			server[i] = new RadioButton(this);
			server[i].setText(SelectServerActivity.SERVER_LIST[i][0]);
			radioGroup.addView(server[i]);
		}
		server[serverNo].setChecked(true);
		radioGroup.setOnCheckedChangeListener(this);
		buttonOptionSave.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				Intent intentReturn = new Intent();
				intentReturn.putExtra(EXTRA_SERVER_NUMBER, serverNo);
				setResult(RESULT_OK, intentReturn);
				finish();
			}
		});
	}

	/**
	 * Responds to the change in selection.
	 */
	@Override
	public void onCheckedChanged(RadioGroup radioGroup, int checkedId) {
		for (int i = 0; i < SelectServerActivity.NUMBER_OF_SERVERS; i++) {
			if (server[i].getId() == checkedId) {
				serverNo = i;
			}
		}
	}
}
