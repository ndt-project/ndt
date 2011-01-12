// Copyright 2009 Google Inc. All Rights Reserved.

package net.measurementlab.ndt;

import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import net.measurementlab.ndt.UiServices;

/**
 * Implementation of UiServices for Android.
 */
public class AndroidUiServices implements UiServices {
  private Message message;
  private Handler handler;
  private Context context;

  /**
   * Constructor to get the handler from Android's UI Thread.
   * 
   * @param handler handler from main activity for dispatching messages
   */
  public AndroidUiServices(Context context, Handler handler) {
    this.handler = handler;
    this.context = context;
  }

  /**
   * {@inheritDoc}
   */
  @Override
  public void appendString(String str, int objectId) {
    switch (objectId) {
      case MAIN_VIEW:
        message = handler.obtainMessage(Constants.THREAD_MAIN_APPEND, str);
        handler.sendMessage(message);
        break;
      case STAT_VIEW:
        message = handler.obtainMessage(Constants.THREAD_STAT_APPEND, str);
        handler.sendMessage(message);
        break;
      case DIAG_VIEW:
        // Diagnosis view is redirected to Statistics view on Android.
        message = handler.obtainMessage(Constants.THREAD_STAT_APPEND, str);
        handler.sendMessage(message);
        break;
      case DEBUG_VIEW:
        // We don't have diagnosis view here, just ignore this action.
      default:
        break;
    }
  }

  /**
   * {@inheritDoc}
   */
  @Override
  public void incrementProgress() {
    message = handler.obtainMessage(Constants.THREAD_ADD_PROGRESS);
    handler.sendMessage(message);
  }

  /**
   * {@inheritDoc}
   */
  @Override
  public void onBeginTest() {
    message = handler.obtainMessage(Constants.THREAD_BEGIN_TEST);
    handler.sendMessage(message);
  }

  /**
   * {@inheritDoc}
   */
  @Override
  public void onEndTest() {
    message = handler.obtainMessage(Constants.THREAD_END_TEST);
    handler.sendMessage(message);
  }

  /**
   * {@inheritDoc}
   */
  @Override
  public void onFailure(String errorMessage) {
    String str = "\n" + context.getString(R.string.fail_tip) + "\n";
    appendString(str, UiServices.MAIN_VIEW);
    appendString(str, STAT_VIEW);
  }

  /**
   * {@inheritDoc}
   */
  @Override
  public void logError(String str) {
    Log.e(Constants.LOG_TAG, str);
  }

  // Unimplemented (and unnecessary Applet-specific) methods below
  /**
   * {@inheritDoc}
   */
  @Override
  public void updateStatus(String status) {
  }

  /**
   * {@inheritDoc}
   */
  @Override
  public void updateStatusPanel(String status) {
  }

  /**
   * {@inheritDoc}
   */
  @Override
  public void onLoginSent() {
  }

  /**
   * {@inheritDoc}
   */
  @Override
  public void onPacketQueuingDetected() {
  }

  /**
   * {@inheritDoc}
   */
  @Override
  public boolean wantToStop() {
    return false;
  }

  @Override
  public void setVariable(String name, int value) {
  }

  @Override
  public void setVariable(String name, double value) {
  }

  @Override
  public void setVariable(String name, Object value) {
  }
}
