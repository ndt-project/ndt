// Copyright 2009 Google Inc. All Rights Reserved.

package net.measurementlab.ndt;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Typeface;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

/**
 * Animated progress while selecting server location.
 */
public class TestsActivity extends Activity {
	private BroadcastReceiver statusReceiver;

	/**
	 * Initializes the activity.
	 */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.tests);
		Log.i("ndt", "Loaded!");
		Typeface typeFace = Typeface.createFromAsset(getAssets(),
				"fonts/League_Gothic.otf");
		TextView textView = (TextView) findViewById(R.id.NdtServerLocationLabel);
		textView.setTypeface(typeFace);
	}

	@Override
	protected void onStart() {
		super.onStart();
		Log.i("ndt", "Tests activity started.");
	}

	@Override
	protected void onResume() {
		super.onResume();

		preparing();
		
		statusReceiver = createReceiver();
		registerReceiver(statusReceiver, new IntentFilter(
				NdtService.INTENT_UPDATE_STATUS));
		
		Intent intent = new Intent(getApplicationContext(), NdtService.class);
		intent.putExtra("networkType", getNetworkType());
		startService(intent);
		
		Log.i("ndt", "Tests activity resumed.");
	}

	@Override
	protected void onPause() {
		super.onPause();
		Intent intent = new Intent(NdtService.INTENT_STOP_TESTS);
		sendBroadcast(intent);
		
		intent = new Intent(getApplicationContext(), NdtService.class);
		stopService(intent);

		unregisterReceiver(statusReceiver);
		statusReceiver = null;

		Log.i("ndt", "Tests activity paused.");
	}

	private void preparing() {
		Log.i("ndt", "Preparing Your Tests...");
		updateHeader(getResources().getText(R.string.tests_preparing_header));
	}

	private BroadcastReceiver createReceiver() {
		BroadcastReceiver receiver = new BroadcastReceiver() {

			@Override
			public void onReceive(Context context, Intent intent) {
				Log.i("ndt", "Status change received.");
				int status = intent.getIntExtra("status", NdtService.PREPARING);
				switch (status) {
				case NdtService.PREPARING:
					preparing();
					break;
				case NdtService.UPLOADING:
					Log.i("ndt", "Testing Upload...");
					updateHeader("Testing upload...");
					break;
				case NdtService.DOWNLOADING:
					Log.i("ndt", "Testing Download...");
					updateHeader("Testing download...");
					break;
				case NdtService.COMPLETE:
					Log.i("ndt", "Testing Complete.");
					updateHeader("Test complete.");
					break;
				default:
					Log.i("ndt", "Test reporter not initialized.");
					break;
				}
			}
		};
		Log.i("ndt", "Status receiver created.");
		return receiver;

	}

	private void updateHeader(CharSequence labelText) {
		TextView textView = (TextView) findViewById(R.id.NdtServerLocationLabel);
		textView.setText(labelText);
	}

	private String getNetworkType() {
		ConnectivityManager connectivityManager = (ConnectivityManager) getSystemService(CONNECTIVITY_SERVICE);
		NetworkInfo networkInfo = connectivityManager.getActiveNetworkInfo();
		if (null == networkInfo) {
			return NdtTests.NETWORK_UNKNOWN;
		}
		switch (networkInfo.getType()) {
		case ConnectivityManager.TYPE_MOBILE:
			return NdtTests.NETWORK_MOBILE;
		case ConnectivityManager.TYPE_WIFI:
			return NdtTests.NETWORK_WIFI;
		default:
			return NdtTests.NETWORK_UNKNOWN;
		}
	}
}
