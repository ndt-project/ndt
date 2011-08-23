// Copyright 2009 Google Inc. All Rights Reserved.

package net.measurementlab.ndt;

import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.Enumeration;

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
import android.widget.ImageView;
import android.widget.TextView;

import static net.measurementlab.ndt.Constants.LOG_TAG;

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
		Log.i(LOG_TAG, "Loaded!");
		Typeface typeFace = Typeface.createFromAsset(getAssets(),
				"fonts/League_Gothic.otf");
		TextView textView = (TextView) findViewById(R.id.NdtTestsHeader);
		textView.setTypeface(typeFace);
		textView = (TextView) findViewById(R.id.NdtTestsInfo);
		textView.setTypeface(typeFace);
		textView = (TextView) findViewById(R.id.TestServerLabel);
		textView.setTypeface(typeFace);
		textView = (TextView) findViewById(R.id.TestServerValue);
		textView.setTypeface(typeFace);
		textView = (TextView) findViewById(R.id.TestClientLabel);
		textView.setTypeface(typeFace);
		textView = (TextView) findViewById(R.id.TestClientValue);
		textView.setTypeface(typeFace);
	}

	@Override
	protected void onStart() {
		super.onStart();
		Log.i(LOG_TAG, "Tests activity started.");
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
		
		Log.i(LOG_TAG, "Tests activity resumed.");
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
		Log.i(LOG_TAG, "Preparing Your Tests...");
		updateHeader(R.string.tests_preparing_header);
		
		String localAddress = getLocalAddress();
		if (null != localAddress) {
			TextView textView = (TextView) findViewById(R.id.TestClientValue);
			textView.setText(localAddress);
		}
		String serverAddress = getServerAddress();
		if (null != serverAddress) {
			TextView textView = (TextView) findViewById(R.id.TestServerValue);
			textView.setText(serverAddress);
		}
		ImageView imageView = (ImageView) findViewById(R.id.NdtTestsProgress);
		imageView.setImageDrawable(getResources().getDrawable(R.drawable.progress_bar));
		// TODO show preparation animation
	}
	
	private void uploading() {
		Log.i(LOG_TAG, "Testing Upload...");
		updateHeader(R.string.tests_both_header, R.string.tests_upload_info);
		ImageView imageView = (ImageView) findViewById(R.id.NdtTestsProgress);
		imageView.setImageDrawable(getResources().getDrawable(R.drawable.progress_bar_left));
		// TODO show upload animation
	}
	
	private void downloading() {
		Log.i(LOG_TAG, "Testing Download...");
		updateHeader(R.string.tests_both_header, R.string.tests_download_info);
		ImageView imageView = (ImageView) findViewById(R.id.NdtTestsProgress);
		imageView.setImageDrawable(getResources().getDrawable(R.drawable.progress_bar_right));
		// TODO show download animation
	}
	
	private void complete(Intent status) {
		Log.i(LOG_TAG, "Testing Complete.");
		updateHeader(R.string.tests_complete_header);
		ImageView imageView = (ImageView) findViewById(R.id.NdtTestsProgress);
		imageView.setImageDrawable(null);
		Intent intent = new Intent(getApplicationContext(), ResultsActivity.class);
		intent.putExtra(NdtService.EXTRA_VARS, status.getSerializableExtra(NdtService.EXTRA_VARS));
		intent.putExtra(NdtService.EXTRA_DIAG_STATUS, status.getStringExtra(NdtService.EXTRA_DIAG_STATUS));
		startActivity(intent);
	}

	private BroadcastReceiver createReceiver() {
		BroadcastReceiver receiver = new BroadcastReceiver() {

			@Override
			public void onReceive(Context context, Intent intent) {
				Log.i(LOG_TAG, "Status change received.");
				int status = intent.getIntExtra(NdtService.EXTRA_STATUS, NdtService.STATUS_PREPARING);
				switch (status) {
				case NdtService.STATUS_PREPARING:
					preparing();
					break;
				case NdtService.STATUS_UPLOADING:
					uploading();
					break;
				case NdtService.STATUS_DOWNLOADING:
					downloading();
					break;
				case NdtService.STATUS_COMPLETE:
					complete(intent);
					break;
				default:
					Log.i(LOG_TAG, "Test reporter not initialized.");
					break;
				}
			}
		};
		Log.i(LOG_TAG, "Status receiver created.");
		return receiver;

	}

	private void updateHeader(int headerResource) {
		TextView textView = (TextView) findViewById(R.id.NdtTestsHeader);
		textView.setText(getResources().getText(headerResource));
		textView = (TextView) findViewById(R.id.NdtTestsInfo);
		textView.setText("");
	}

	private void updateHeader(int headerResource, int infoResource) {
		updateHeader(headerResource);
		TextView textView = (TextView) findViewById(R.id.NdtTestsInfo);
		textView.setText(getResources().getText(infoResource));
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

	private String getLocalAddress() {
	    try {
	        for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements();) {
	            NetworkInterface intf = en.nextElement();
	            for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses(); enumIpAddr.hasMoreElements();) {
	                InetAddress inetAddress = enumIpAddr.nextElement();
	                if (!inetAddress.isLoopbackAddress()) {
	                    return inetAddress.getHostAddress().toString();
	                }
	            }
	        }
	    } catch (SocketException ex) {
	        Log.e(LOG_TAG, ex.toString());
	    }
	    return null;
	}
	
	private String getServerAddress() {
		try {
			InetAddress server = InetAddress.getByName(Constants.SERVER_HOST[Constants.DEFAULT_SERVER]);
			return server.getHostAddress();
		} catch (UnknownHostException e) {
			Log.e(LOG_TAG, "Error resolving server hosts.", e);
		}
		return null;
	}
}
