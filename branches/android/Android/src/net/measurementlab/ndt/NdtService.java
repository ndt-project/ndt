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

/**
 * Background service that drives the NDT test protocol and broadcasts updates
 * to indicated when the @see {@link TestsActivity} should change to reflect
 * progress to the next user visible test.
 * 
 * @author gideon@newamerica.net
 * 
 */
public class NdtService extends Service {
	// log tag so logcat can be filtered to just the variable
	// written out by the core NDT Java code, useful for understanding
	// what is available for display in the test results
	private static final String VARS_TAG = "variables";

	// user visible test phases, more coarse than actual tests
	/**
	 * Includes middle box and SFW testing.
	 */
	static final int STATUS_PREPARING = 0;
	/**
	 * Client to server test.
	 */
	static final int STATUS_UPLOADING = 1;
	/**
	 * Server to client and meta test.
	 */
	static final int STATUS_DOWNLOADING = 2;
	/**
	 * Indicates tests are concluded.
	 */
	static final int STATUS_COMPLETE = 3;
	/**
	 * Indicates the service encountered an error.
	 */
	static final int STATUS_ERROR = 4;

	/**
	 * Intent for sending updates from this service to trigger display changes
	 * in @link {@link TestsActivity}.
	 */
	static final String INTENT_UPDATE_STATUS = "net.measurementlab.ndt.UpdateStatus";
	/**
	 * Intent for @link {@link TestsActivity} to request that this service stop
	 * testing.
	 */
	static final String INTENT_STOP_TESTS = "net.measurementlab.ndt.StopTests";

	/**
	 * Label for extra data in {@link Intent} sent for test status updates.
	 */
	static final String EXTRA_STATUS = "status";

	/**
	 * Label for extra data about network type in {@link Intent} sent from
	 * {@link InitialActivity}.
	 */
	static final String EXTRA_NETWORK_TYPE = "networkType";

	/**
	 * Status line used for the advanced display.
	 */
	static final String EXTRA_DIAG_STATUS = "diagnosticStatus";

	/**
	 * Label for variables captured during testing and used in presentation of
	 * results in {@link ResultsActivity}.
	 */
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

		// this means the service is already started
		if (uiServices != null) {
			return;
		}

		if (null != intent) {
			networkType = intent.getStringExtra(EXTRA_NETWORK_TYPE);
		}

		// listen to a broadcast from TestActivity requesting that
		// tests be stopped
		stopReceiver = createReceiver();
		registerReceiver(stopReceiver, new IntentFilter(
				NdtService.INTENT_STOP_TESTS));

		// this interface is how the service is able to observe the
		// tests as run by the core Java client code
		uiServices = new CaptiveUiServices();
		try {
			new Thread(new NdtTests(
					Constants.SERVER_HOST[Constants.DEFAULT_SERVER],
					uiServices, networkType)).start();
		} catch (Throwable tr) {
			Log.e(LOG_TAG, "Problem running tests.", tr);
			intent.putExtra(EXTRA_STATUS, STATUS_ERROR);
		}
	}

	@Override
	public void onDestroy() {
		super.onDestroy();
		if (STATUS_COMPLETE != uiServices.status
				&& STATUS_ERROR != uiServices.status) {
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
		private final Map<Integer, StringBuilder> statusBuffers = new HashMap<Integer, StringBuilder>();

		// sub-string that identifies the message marking start of upload
		// testing
		private static final String S2C_MSG_FRAGMENT = "server-to-client";

		// sub-string that identifies the message marking start of download
		// testing
		private static final String C2S_MSG_FRAGMENT = "client-to-server";

		private boolean wantToStop = false;

		int status = STATUS_PREPARING;

		private Map<String, Object> variables = new HashMap<String, Object>();

		CaptiveUiServices() {
			// not needed now but may be useful in the future
//			statusBuffers.put(UiServices.MAIN_VIEW, new StringBuilder());
//			statusBuffers.put(UiServices.STAT_VIEW, new StringBuilder());
			statusBuffers.put(UiServices.DIAG_VIEW, new StringBuilder());
		}

		/**
		 * {@inheritDoc}
		 */
		@Override
		public void appendString(String message, int viewId) {
			Log.d(LOG_TAG, String.format("Appended: (%1$d) %2$s.", viewId,
					message.trim()));

			if (statusBuffers.containsKey(viewId)) {
				statusBuffers.get(viewId).append(message).append('\n');
			}

			if (message.contains(C2S_MSG_FRAGMENT)
					&& UiServices.MAIN_VIEW == viewId) {
				Log.i(LOG_TAG, "Starting upload test.");
				intent.putExtra(EXTRA_STATUS, STATUS_UPLOADING);
				sendBroadcast(intent);
				status = STATUS_UPLOADING;
				Log.i(LOG_TAG, "Broadcast status change.");
			}

			if (message.contains(S2C_MSG_FRAGMENT)
					&& UiServices.MAIN_VIEW == viewId) {
				Log.i(LOG_TAG, "Starting download test.");
				intent.putExtra(EXTRA_STATUS, STATUS_DOWNLOADING);
				sendBroadcast(intent);
				status = STATUS_DOWNLOADING;
				Log.i(LOG_TAG, "Broadcast status change.");
			}
		}

		/**
		 * {@inheritDoc}
		 */
		@Override
		public void incrementProgress() {
			Log.d(LOG_TAG, "Incremented progress.");
		}

		/**
		 * {@inheritDoc}
		 */
		@Override
		public void logError(String str) {
			Log.e(LOG_TAG, String.format("Error: %1$s.", str.trim()));
		}

		/**
		 * {@inheritDoc}
		 */
		@Override
		public void onBeginTest() {
			Log.d(LOG_TAG, "Test begun.");
			wantToStop = false;
		}

		/**
		 * {@inheritDoc}
		 */
		@Override
		public void onEndTest() {
			Log.d(LOG_TAG, "Test ended.");
			intent.putExtra(EXTRA_STATUS, STATUS_COMPLETE);
			intent.putExtra(EXTRA_DIAG_STATUS, statusBuffers.get(
					UiServices.DIAG_VIEW).toString());
			intent.putExtra(EXTRA_VARS, (Serializable) variables);
			sendBroadcast(intent);
			status = STATUS_COMPLETE;
			wantToStop = false;
			Log.i(LOG_TAG, "Broadcast status change.");
		}

		/**
		 * {@inheritDoc}
		 */
		@Override
		public void onFailure(String errorMessage) {
			Log.d(LOG_TAG, String.format("Failed: %1$s.", errorMessage));
			intent.putExtra(EXTRA_STATUS, STATUS_ERROR);
			sendBroadcast(intent);
			status = STATUS_ERROR;
			wantToStop = false;
		}

		/**
		 * {@inheritDoc}
		 */
		@Override
		public void onLoginSent() {
			Log.d(LOG_TAG, "Login sent.");
		}

		/**
		 * {@inheritDoc}
		 */
		@Override
		public void onPacketQueuingDetected() {
			Log.d(LOG_TAG, "Packet queuing detected.");
		}

		/**
		 * {@inheritDoc}
		 */
		@Override
		public void setVariable(String name, int value) {
			Log.d(VARS_TAG, String.format(
					"Setting variable, %1$s, to value, %2$d.", name, value));
			variables.put(name, value);
		}

		/**
		 * {@inheritDoc}
		 */
		@Override
		public void setVariable(String name, double value) {
			Log.d(VARS_TAG, String.format(
					"Setting variable, %1$s, to value, %2$f.", name, value));
			variables.put(name, value);
		}

		/**
		 * {@inheritDoc}
		 */
		@Override
		public void setVariable(String name, Object value) {
			Log.d(VARS_TAG, String.format(
					"Setting variable, %1$s, to value, %2$s.", name,
					(null == value) ? "null" : value.toString()));
			variables.put(name, value);
		}

		/**
		 * {@inheritDoc}
		 */
		@Override
		public void updateStatus(String status) {
			Log.d(LOG_TAG, String.format("Updating status: %1$s.", status));
		}

		/**
		 * {@inheritDoc}
		 */
		@Override
		public void updateStatusPanel(String status) {
			Log.d(LOG_TAG, String
					.format("Updating status panel: %1$s.", status));
		}

		/**
		 * {@inheritDoc}
		 */
		@Override
		public boolean wantToStop() {
			return wantToStop;
		}

		/**
		 * Allows the containing service instance to request the blackbox from
		 * the core NDT Java code stop testing.
		 */
		void requestStop() {
			wantToStop = true;
		}
	}
}
