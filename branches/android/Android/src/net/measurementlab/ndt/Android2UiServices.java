package net.measurementlab.ndt;

import android.util.Log;

public class Android2UiServices implements UiServices {

	@Override
	public void appendString(String str, int viewId) {
		Log.i("ndt", String.format("Appended: (%1$d) %2$s.", viewId, str));
	}

	@Override
	public void incrementProgress() {
		Log.i("ndt", "Incremented progress.");
	}

	@Override
	public void logError(String str) {
		Log.e("ndt", String.format("Error: %1$s.", str));
	}

	@Override
	public void onBeginTest() {
		Log.i("ndt", "Test begun.");
	}

	@Override
	public void onEndTest() {
		Log.i("ndt", "Test ended.");
	}

	@Override
	public void onFailure(String errorMessage) {
		Log.i("ndt", String.format("Failed: %1$s.", errorMessage));
	}

	@Override
	public void onLoginSent() {
		Log.i("ndt", "Login sent.");
	}

	@Override
	public void onPacketQueuingDetected() {
		Log.i("ndt", "Packet queuing detected.");
	}

	@Override
	public void setVariable(String name, int value) {
		Log.i("ndt", String.format("Setting variable, %1$s, to value, %2$d.",
				name, value));
	}

	@Override
	public void setVariable(String name, double value) {
		Log.i("ndt", String.format("Setting variable, %1$s, to value, %2$f.",
				name, value));
	}

	@Override
	public void setVariable(String name, Object value) {
		Log.i("ndt", String.format("Setting variable, %1$s, to value, %2$s.",
				name, (null == value) ? "null" : value.toString()));
	}

	@Override
	public void updateStatus(String status) {
		Log.i("ndt", String.format("Updating status: %1$s.", status));
	}

	@Override
	public void updateStatusPanel(String status) {
		Log.i("ndt", String.format("Updating status panel: %1$s.", status));
	}

	@Override
	public boolean wantToStop() {
		return false;
	}

}
