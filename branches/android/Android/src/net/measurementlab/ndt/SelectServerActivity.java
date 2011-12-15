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
	private RadioButton server[] = new RadioButton[Constants.NUMBER_OF_SERVERS];
	private Button buttonOptionSave;
	private RadioGroup radioGroup;

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
		for (int i = 0; i < Constants.NUMBER_OF_SERVERS; i++) {
			server[i] = new RadioButton(this);
			server[i].setText(Constants.SERVER_LIST[i][0]);
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
		for (int i = 0; i < Constants.NUMBER_OF_SERVERS; i++) {
			if (server[i].getId() == checkedId) {
				serverNo = i;
			}
		}
	}
}
