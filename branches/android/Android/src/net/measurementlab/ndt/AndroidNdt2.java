// Copyright 2009 Google Inc. All Rights Reserved.

package net.measurementlab.ndt;

import android.app.Activity;
import android.graphics.Typeface;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

/**
 * UI Thread and Entry Point of NDT mobile client.
 */
public class AndroidNdt2 extends Activity {

	/**
	 * Initializes the activity.
	 */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.start);
		Log.i("ndt", "Loaded!");
        Typeface typeFace = Typeface.createFromAsset(getAssets(),
        "fonts/League_Gothic.otf");
        TextView textView = (TextView) findViewById(R.id.MLabDesc);
        textView.setTypeface(typeFace);
	}
}
