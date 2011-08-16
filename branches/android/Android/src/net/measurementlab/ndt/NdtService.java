package net.measurementlab.ndt;

import static net.measurementlab.ndt.Constants.LOG_TAG;

import java.io.Serializable;
import java.util.HashMap;
import java.util.Map;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.IBinder;
import android.util.Log;

public class NdtService extends Service {
	private static final String VARS_TAG = "variables";
	
	static final int PREPARING = 0;
	static final int UPLOADING = 1;
	static final int DOWNLOADING = 2;
	static final int COMPLETE = 3;
	
	static final String INTENT_UPDATE_STATUS = "net.measurementlab.ndt.UpdateStatus";
	static final String INTENT_STOP_TESTS = "net.measurementlab.ndt.StopTests";
	
	static final String EXTRA_STATUS = "status";
	static final String EXTRA_VARS = VARS_TAG;
	
	private Intent intent;
	
	private String networkType;
	
	private CaptiveUiServices uiServices;
	
	private BroadcastReceiver stopReceiver;

	@Override
	public IBinder onBind(Intent intent) {
		return null;
	}

	@Override
	public void onCreate() {
		super.onCreate();
		Log.i(LOG_TAG, "Service created.");
		intent = new Intent(INTENT_UPDATE_STATUS);
	}

	@Override
	public void onStart(Intent intent, int startId) {
		Log.i(LOG_TAG, "Starting NDT service.");
		super.onStart(intent, startId);
		if (uiServices != null) {
			return;
		}
		if (null != intent) {
			networkType = intent.getStringExtra("networkType");
		}
		stopReceiver = createReceiver();
		registerReceiver(stopReceiver, new IntentFilter(
				NdtService.INTENT_STOP_TESTS));
		uiServices = new CaptiveUiServices();
		try {
			new Thread(new NdtTests(
					Constants.SERVER_HOST[Constants.DEFAULT_SERVER],
					uiServices, networkType)).start();
		} catch (Throwable tr) {
			Log.e(LOG_TAG, "Problem running tests.", tr);
		}
	}
	
	@Override
	public void onDestroy() {
		super.onDestroy();
		if (COMPLETE != uiServices.status) {
			uiServices.requestStop();
		}
		uiServices = null;
		unregisterReceiver(stopReceiver);
		Log.i(LOG_TAG, "Finishing NDT service.");
	}

	private BroadcastReceiver createReceiver() {
		BroadcastReceiver receiver = new BroadcastReceiver() {

			@Override
			public void onReceive(Context context, Intent intent) {
				Log.i(LOG_TAG, "Stop request received.");
				uiServices.requestStop();
			}
		};
		Log.i("ndt", "Stop receiver created.");
		return receiver;

	}

	private class CaptiveUiServices implements UiServices {
		private boolean wantToStop = false;
		
		int status = PREPARING;
		
		private Map<String, Object> variables = new HashMap<String, Object>();
		
		@Override
		public void appendString(String str, int viewId) {
			Log.d(LOG_TAG, String.format("Appended: (%1$d) %2$s.", viewId, str
					.trim()));
			
			if (str.contains("client-to-server") && 0 == viewId) {
				Log.i(LOG_TAG, "Starting upload test.");
				intent.putExtra(EXTRA_STATUS, UPLOADING);
				sendBroadcast(intent);
				status = UPLOADING;
				Log.i(LOG_TAG, "Broadcast status change.");
			}

			if (str.contains("server-to-client") && 0 == viewId) {
				Log.i(LOG_TAG, "Starting download test.");
				intent.putExtra(EXTRA_STATUS, DOWNLOADING);
				sendBroadcast(intent);
				status = DOWNLOADING;
				Log.i(LOG_TAG, "Broadcast status change.");
			}
		}

		@Override
		public void incrementProgress() {
			Log.d(LOG_TAG, "Incremented progress.");
		}

		@Override
		public void logError(String str) {
			Log.e(LOG_TAG, String.format("Error: %1$s.", str.trim()));
		}

		@Override
		public void onBeginTest() {
			Log.d(LOG_TAG, "Test begun.");
			wantToStop = false;
		}

		@Override
		public void onEndTest() {
			Log.d(LOG_TAG, "Test ended.");
			intent.putExtra(EXTRA_STATUS, COMPLETE);
			intent.putExtra(EXTRA_VARS, (Serializable) variables);
			sendBroadcast(intent);
			wantToStop = false;
			status = COMPLETE;
			Log.i(LOG_TAG, "Broadcast status change.");
		}

		@Override
		public void onFailure(String errorMessage) {
			Log.d(LOG_TAG, String.format("Failed: %1$s.", errorMessage));
			wantToStop = false;
		}

		@Override
		public void onLoginSent() {
			Log.d(LOG_TAG, "Login sent.");
		}

		@Override
		public void onPacketQueuingDetected() {
			Log.d(LOG_TAG, "Packet queuing detected.");
		}

		@Override
		public void setVariable(String name, int value) {
			Log.d(VARS_TAG, String.format(
					"Setting variable, %1$s, to value, %2$d.", name, value));
			variables.put(name, value);
		}

		@Override
		public void setVariable(String name, double value) {
			Log.d(VARS_TAG, String.format(
					"Setting variable, %1$s, to value, %2$f.", name, value));
			variables.put(name, value);
		}

		@Override
		public void setVariable(String name, Object value) {
			Log.d(VARS_TAG, String.format(
					"Setting variable, %1$s, to value, %2$s.", name,
					(null == value) ? "null" : value.toString()));
			variables.put(name, value);
		}

		@Override
		public void updateStatus(String status) {
			Log.d(LOG_TAG, String.format("Updating status: %1$s.", status));
		}

		@Override
		public void updateStatusPanel(String status) {
			Log.d(LOG_TAG, String.format("Updating status panel: %1$s.", status));
		}

		@Override
		public boolean wantToStop() {
			return wantToStop;
		}

		void requestStop() {
			wantToStop = true;
		}
	}
}
