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
public class ServerLocation extends Activity {
	private BroadcastReceiver statusReceiver;

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

		Intent intent = new Intent(getApplicationContext(), NdtService.class);
		intent.putExtra("networkType", getNetworkType());
		startService(intent);

		bindAndRegister();
	}

	@Override
	protected void onStart() {
		super.onStart();
		Log.i("ndt", "Server location started.");
	}

	@Override
	protected void onResume() {
		super.onResume();
		Log.i("ndt", "Server location resumed.");
	}

	@Override
	protected void onPause() {
		super.onPause();
		Log.i("ndt", "Server location paused.");
		unregisterReceiver(statusReceiver);
		// TODO tell NdtService to stop test
		// TODO reset activity state to preparing
	}

	private void bindAndRegister() {
		statusReceiver = createReceiver();
		registerReceiver(statusReceiver, new IntentFilter(
				NdtService.INTENT_UPDATE_STATUS));
		Log.i("ndt", "Status receiver registered.");
	}

	private BroadcastReceiver createReceiver() {
		BroadcastReceiver receiver = new BroadcastReceiver() {

			@Override
			public void onReceive(Context context, Intent intent) {
				Log.i("ndt", "Status change received.");
				int status = intent.getIntExtra("status", NdtService.PREPARING);
				switch (status) {
				case NdtService.PREPARING:
					Log.i("ndt", "Preparing Your Tests...");
					updateHeader("Preparing...");
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

	private void updateHeader(String labelText) {
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
