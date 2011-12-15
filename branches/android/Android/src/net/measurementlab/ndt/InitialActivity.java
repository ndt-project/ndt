package net.measurementlab.ndt;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

/**
 * UI Thread and Entry Point of NDT mobile client.
 */
public class InitialActivity extends Activity {

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
				startActivity(intent);
			}
		});

		Button aboutButton = (Button) findViewById(R.id.ButtonAbout);
		aboutButton.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("http://measurementlab.net"));
				startActivity(intent);
			}
		});
	}
}
