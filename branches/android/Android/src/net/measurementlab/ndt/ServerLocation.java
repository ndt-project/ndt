// Copyright 2009 Google Inc. All Rights Reserved.

package net.measurementlab.ndt;

import java.util.concurrent.TimeUnit;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.graphics.Typeface;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.widget.TextView;

/**
 * Animated progress while selecting server location.
 */
public class ServerLocation extends Activity {
	private ITestReporter testReporter;
	private boolean bound;
	private boolean running;
	private ServiceConnection connection = new ServiceConnection() {

		@Override
		public void onServiceDisconnected(ComponentName name) {
			Log.i("ndt", "Service disconnected.");
			testReporter = null;
			bound = false;
		}

		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			Log.i("ndt", "Service connected.");
			testReporter = ITestReporter.Stub.asInterface(service);
			bound = true;
		}
	};

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
		
		bindService(new Intent(getApplicationContext(), NdtService.class),
				this.connection, Context.BIND_AUTO_CREATE);
		bound = true;

		running = true;
		new Thread() {
			
			@Override
			public void run() {
				try {
					while (true == running && (null == testReporter || NdtService.COMPLETE != testReporter.getState())) {
						TimeUnit.SECONDS.sleep(1l);
						Log.d("ndt", String.format(
								"Checking test state, %1$d.",
								(null == testReporter) ? -1 : testReporter
										.getState()));
						switch ((null == testReporter) ? 0 : testReporter.getState()) {
						case NdtService.PREPARING:
							updateHeader("Preparing...");
							break;
						case NdtService.UPLOADING:
							updateHeader("Testing upload...");
							break;
						case NdtService.DOWNLOADING:
							updateHeader("Testing download...");
							break;
						}
					}
					updateHeader("Test complete.");
				} catch (RemoteException e) {
					Log.e("ndt", "Error in busy-wait loop.", e);
				} catch (InterruptedException e) {
					Log.e("ndt", "Sleep interrupted.", e);
				}
			}
		}.start();
	}

	@Override
	protected void onResume() {
		super.onStart();
		Log.i("ndt", "Server location resumed.");
		if (!bound) {
			bindService(new Intent(ITestReporter.class.getName()),
					this.connection, Context.BIND_AUTO_CREATE);
		}
	}

	@Override
	protected void onPause() {
		super.onPause();
		Log.i("ndt", "Server location paused.");
		if (bound) {
			bound = false;
			unbindService(this.connection);
		}
		running = false;
	}
	
	@Override
	protected void onDestroy() {
		super.onDestroy();
		running = false;
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
