package net.measurementlab.ndt;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.Toast;

/**
 * UI Thread and Entry Point of NDT mobile client.
 */
public class InitialActivity extends Activity {

	private int serverNumber = SelectServerActivity.DEFAULT_SERVER;

	/**
	 * Initializes the activity.
	 */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.initial);
		Log.i("ndt", "Loaded!");
		NdtSupport.applyFont(this, "fonts/League_Gothic.otf", R.id.MLabDesc);

		Button startButton = (Button) findViewById(R.id.ButtonStart);
		startButton.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				Intent intent = new Intent(getApplicationContext(),
						TestsActivity.class);
				intent.putExtra(NdtService.EXTRA_SERVER_HOST,
						SelectServerActivity.SERVER_LIST[serverNumber][1]);
				startActivity(intent);
			}
		});
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.initial, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle item selection
		switch (item.getItemId()) {
		case R.id.MenuSelectServers: {
			Intent intent = new Intent(InitialActivity.this, SelectServerActivity.class);
			intent.putExtra(SelectServerActivity.EXTRA_SERVER_NUMBER, serverNumber);
			startActivityForResult(intent, SelectServerActivity.ACTIVITY_SELECT_SERVER);

			return true;
		}
		case R.id.MenuAbout: {
			Intent intent = new Intent(Intent.ACTION_VIEW, Uri
					.parse("http://measurementlab.net"));
			startActivity(intent);
			return true;
		}
		default:
			return super.onOptionsItemSelected(item);
		}
	}

	/**
	 * {@inheritDoc}
	 */
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		if (resultCode == RESULT_OK
				|| requestCode == SelectServerActivity.ACTIVITY_SELECT_SERVER) {
			if (data != null) {
				serverNumber = data.getExtras().getInt(
						SelectServerActivity.EXTRA_SERVER_NUMBER);
				Toast serverSelected = Toast.makeText(getApplicationContext(),
						"Selected " + SelectServerActivity.SERVER_LIST[serverNumber][0],
						10);
				serverSelected.show();
			}
		}
	}

}
