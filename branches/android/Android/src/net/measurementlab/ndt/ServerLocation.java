// Copyright 2009 Google Inc. All Rights Reserved.

package net.measurementlab.ndt;

import android.app.Activity;
import android.graphics.Typeface;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

/**
 * Animated progress while selecting server location.
 */
public class ServerLocation extends Activity {
	
	/**
	 * Initializes the activity.
	 */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.server_location);
		Log.i("ndt", "Loaded!");
        Typeface typeFace = Typeface.createFromAsset(getAssets(),
        "fonts/League_Gothic.otf");
        TextView textView = (TextView) findViewById(R.id.NdtServerLocationLabel);
        textView.setTypeface(typeFace);
	}
}
