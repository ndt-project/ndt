package net.measurementlab.ndt;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

public class NdtService extends Service {
	public static final int PREPARING = 0;
	public static final int UPLOADING = 1;
	public static final int DOWNLOADING = 2;
	public static final int COMPLETE = 3;
	
	private TestReporter testReporter = new TestReporter();

	@Override
	public IBinder onBind(Intent intent) {
		Log.i("ndt", "Service bound.");
		return testReporter;
	}

	@Override
	public void onCreate() {
		super.onCreate();
		Log.i("ndt", "Service created.");
	}

	@Override
	public void onStart(Intent intent, int startId) {
		Log.i("ndt", "Starting NDT service.");
		super.onStart(intent, startId);
		if (null == intent) {
			throw new IllegalArgumentException("Intent was null!");
		}
		try {
			new Thread(new NdtTests(
					Constants.SERVER_HOST[Constants.DEFAULT_SERVER],
					new CaptiveUiServices(), intent
							.getStringExtra("networkType"))).start();
			testReporter.setState(COMPLETE);
		} catch (Throwable tr) {
			Log.e("ndt", "Problem running tests.", tr);
		}
	}
	
	@Override
	public void onDestroy() {
		super.onDestroy();
		Log.i("ndt", "Finishing NDT service.");
	}
	
	private class TestReporter extends ITestReporter.Stub {
		private int state = PREPARING;

		@Override
		public int getState() throws RemoteException {
			return this.state;
		}
		
		public void setState(int state) { 
			this.state = state;
		}
	}

	private class CaptiveUiServices implements UiServices {

		@Override
		public void appendString(String str, int viewId) {
			Log.d("ndt", String.format("Appended: (%1$d) %2$s.", viewId, str
					.trim()));

			if (str.contains("client-to-server") && UPLOADING == viewId) {
				Log.i("ndt", "Starting upload test.");
				testReporter.setState(UPLOADING);
			}

			if (str.contains("server-to-client") && DOWNLOADING == viewId) {
				Log.i("ndt", "Starting upload test.");
				testReporter.setState(DOWNLOADING);
			}
		}

		@Override
		public void incrementProgress() {
			Log.d("ndt", "Incremented progress.");
		}

		@Override
		public void logError(String str) {
			Log.e("ndt", String.format("Error: %1$s.", str.trim()));
		}

		@Override
		public void onBeginTest() {
			Log.d("ndt", "Test begun.");
		}

		@Override
		public void onEndTest() {
			Log.d("ndt", "Test ended.");
		}

		@Override
		public void onFailure(String errorMessage) {
			Log.d("ndt", String.format("Failed: %1$s.", errorMessage));
		}

		@Override
		public void onLoginSent() {
			Log.d("ndt", "Login sent.");
		}

		@Override
		public void onPacketQueuingDetected() {
			Log.d("ndt", "Packet queuing detected.");
		}

		@Override
		public void setVariable(String name, int value) {
			Log.d("ndt", String.format(
					"Setting variable, %1$s, to value, %2$d.", name, value));
		}

		@Override
		public void setVariable(String name, double value) {
			Log.d("ndt", String.format(
					"Setting variable, %1$s, to value, %2$f.", name, value));
		}

		@Override
		public void setVariable(String name, Object value) {
			Log.d("ndt", String.format(
					"Setting variable, %1$s, to value, %2$s.", name,
					(null == value) ? "null" : value.toString()));
		}

		@Override
		public void updateStatus(String status) {
			Log.d("ndt", String.format("Updating status: %1$s.", status));
		}

		@Override
		public void updateStatusPanel(String status) {
			Log.d("ndt", String.format("Updating status panel: %1$s.", status));
		}

		@Override
		public boolean wantToStop() {
			return false;
		}

	}
}
