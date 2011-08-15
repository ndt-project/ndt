// Copyright 2009 Google Inc. All Rights Reserved.

package net.measurementlab.ndt;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Typeface;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;

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
		Typeface typeFace = Typeface.createFromAsset(getAssets(),
				"fonts/League_Gothic.otf");
		TextView textView = (TextView) findViewById(R.id.MLabDesc);
		textView.setTypeface(typeFace);

		Button startButton = (Button) findViewById(R.id.ButtonStart);
		startButton.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				Intent intent = null;

				intent = new Intent(getApplicationContext(),
						TestsActivity.class);
				startActivity(intent);
			}
		});
	}
}
