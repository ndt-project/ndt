// Copyright 2009 Google Inc. All Rights Reserved.

package net.measurementlab.ndt;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.os.PowerManager;
import android.util.Log;
import android.widget.Toast;

/**
 * UI Thread and Entry Point of NDT mobile client.
 */
public class AndroidNdt extends Activity {
  private PowerManager powerManager;
  private PowerManager.WakeLock wakeLock;
  private NdtLocation ndtLocation;

  /**
   * Initializes the activity.
   */
  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    checkServerListValid();
    // Set the default server
    ndtLocation = new NdtLocation(this);
    ndtLocation.startListen();
    powerManager = (PowerManager) getSystemService(Context.POWER_SERVICE);
    wakeLock = powerManager.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, "Network Testing");
  }

  /**
   * {@inheritDoc}
   */
  @Override
  protected void onDestroy() {
    Log.v(Constants.LOG_TAG, "onDestory");
    if (wakeLock != null && wakeLock.isHeld()) {
      wakeLock.release();
      Log.v(Constants.LOG_TAG, "Release Wake Lock onDestroy");
    }
    super.onDestroy();
  }

  /**
   * {@inheritDoc}
   */
  @Override
  protected void onStart() {
    Log.v(Constants.LOG_TAG, "onStart");
    ndtLocation.startListen();
    super.onStart();
  }

  /**
   * {@inheritDoc}
   */
  @Override
  protected void onStop() {
    Log.v(Constants.LOG_TAG, "onStop");
    ndtLocation.stopListen();
    super.onStop();
  }

  /**
   * Called when server length is invalid.
   * 
   * @see Constants#NUMBER_OF_SERVERS
   */
  private void checkServerListValid() {
    if ((Constants.SERVER_NAME.length != Constants.SERVER_HOST.length)
        || (Constants.NUMBER_OF_SERVERS == 0)) {
      Log.v(Constants.LOG_TAG, "Server list corrupted.");
      Toast toast =
          Toast.makeText(getApplicationContext(), R.string.serverlist_corrupted_tip,
              Toast.LENGTH_SHORT);
      toast.show();
      finish();
    }
  }
}
