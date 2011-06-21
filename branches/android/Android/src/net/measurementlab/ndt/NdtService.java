package net.measurementlab.ndt;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

public class NdtService extends Service {

	@Override
	public IBinder onBind(Intent intent) {
		return null;
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
		try {
			NdtTests tests = new NdtTests(
					Constants.SERVER_HOST[Constants.DEFAULT_SERVER],
					new Android2UiServices(), intent
							.getStringExtra("networkType"));
			tests.run();
		} catch (Throwable tr) {
			Log.e("ndt", "Problem running tests.", tr);
		}
		Log.i("ndt", "Finishing NDT service.");
	}
}
