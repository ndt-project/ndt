// Copyright 2009 Google Inc. All Rights Reserved.

package net.measurementlab.ndt;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import java.util.Date;

import net.measurementlab.ndt.NdtTests;
import net.measurementlab.ndt.UiServices;

/**
 * UI Thread and Entry Point of NDT mobile client.
 */
public class AndroidNdt extends Activity {
  private int serverNumber;
  private int progress;
  private String serverName;
  private String serverHost;
  private String mobileInfo;
  private String statistics;
  private Button buttonStart;
  private Button buttonAbout;
  private Button buttonStatistics;
  private Button buttonOption;
  private ProgressBar progressBar;
  private TextView textViewMain;
  private UiHandler uiHandler;
  private PowerManager powerManager;
  private PowerManager.WakeLock wakeLock;
  private NetworkInfo networkInfo;
  private UiServices uiServices;
  private NdtLocation ndtLocation;

  private class StartButtonListener implements OnClickListener {
    public void onClick(View view) {
      statistics = "";
      Date date = new Date();
      StringBuilder stringBuilder = new StringBuilder()
          .append("\n")
          .append(getString(R.string.test_begins_at, date.toString()))
          .append("\n");
      uiServices.appendString(stringBuilder.toString(), UiServices.MAIN_VIEW);
      uiServices.appendString(stringBuilder.toString(), UiServices.STAT_VIEW);
      stringBuilder = new StringBuilder()
          .append("\n")
          .append(getString(R.string.from_header))
          .append(getSystemProperty())
          .append("\n")
          .append(mobileInfo)
          .append("\n\n")
          .append(getString(R.string.to_header))
          .append("\n")
          .append(getString(R.string.server_indicator, serverName))
          .append("\n")
          .append(getString(R.string.host_indicator, serverHost))
          .append("\n");
      uiServices.appendString(stringBuilder.toString(), UiServices.STAT_VIEW);
      Thread netWorker = new Thread(new NdtTests(serverHost, uiServices, getNetworkType()));
      netWorker.start();
    }
  }

  private class StatisticsButtonListener implements OnClickListener {
    public void onClick(View view) {
      StringBuilder locationBuilder = new StringBuilder();
      if (ndtLocation.location != null) {
        locationBuilder
            .append(getString(R.string.latitude_result, ndtLocation.location.getLatitude()))
            .append("\n")
            .append(getString(R.string.longitude_result, ndtLocation.location.getLongitude()));
      }
      Intent intent = new Intent(AndroidNdt.this, Statistics.class);
      intent.putExtra(Constants.INTENT_STATISTICS, statistics);
      intent.putExtra(Constants.INTENT_NETWORK, getNetworkType());
      intent.putExtra(Constants.INTENT_LOCATION, locationBuilder.toString());
      startActivityForResult(intent, Constants.ACTIVITY_STATISTICS);
    }
  }

  private class AboutButtonListener implements OnClickListener {
    Context contextActivity;

    AboutButtonListener(Context context) {
      contextActivity = context;
    }

    public void onClick(View view) {
      AlertDialog.Builder builderAbout = new AlertDialog.Builder(contextActivity);
      builderAbout.setTitle(R.string.label_about_title);
      builderAbout.setPositiveButton(R.string.close, new DialogInterface.OnClickListener() {
        public void onClick(DialogInterface dialog, int which) {
          setResult(RESULT_OK);
        }
      });
      builderAbout.setMessage(R.string.label_about_description);
      builderAbout.create();
      builderAbout.show();
    }
  }

  private class OptionButtonListener implements OnClickListener {
    public void onClick(View view) {
      Intent intent = new Intent(AndroidNdt.this, SelectServer.class);
      intent.putExtra(Constants.INTENT_SERVER_NO, serverNumber);
      startActivityForResult(intent, Constants.ACTIVITY_OPTION);
    }
  }

  /**
   * Initializes the activity.
   */
  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.main);
    checkServerListValid();
    // Set the default server
    serverNumber = Constants.DEFAULT_SERVER;
    ndtLocation = new NdtLocation(this);
    ndtLocation.startListen();
    powerManager = (PowerManager) getSystemService(Context.POWER_SERVICE);
    ConnectivityManager connectivityManager =
        (ConnectivityManager) getSystemService(CONNECTIVITY_SERVICE);
    networkInfo = connectivityManager.getActiveNetworkInfo();
    wakeLock = powerManager.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, "Network Testing");
    serverName = Constants.SERVER_NAME[serverNumber];
    serverHost = Constants.SERVER_HOST[serverNumber];
    uiHandler = new UiHandler(Looper.myLooper());
    uiServices = new AndroidUiServices(this, uiHandler);
    textViewMain = (TextView) findViewById(R.id.TextViewMain);
    textViewMain.setMovementMethod(ScrollingMovementMethod.getInstance());
    textViewMain.setClickable(false);
    textViewMain.setLongClickable(false);
    textViewMain.append("\n" + getString(R.string.version_indicator, NdtTests.VERSION));
    textViewMain.append("\n" + getString(R.string.default_server_indicator, serverName));
    textViewMain.append("\n");
    mobileInfo = getMobileProperty();
    textViewMain.append(mobileInfo);
    textViewMain.append("\n");
    initComponents();
  }

  /**
   * {@inheritDoc}
   */
  @Override
  protected void onActivityResult(int requestCode, int resultCode, Intent data) {
    if (resultCode == RESULT_OK || requestCode == Constants.ACTIVITY_OPTION) {
      if (data != null) {
        serverNumber = data.getExtras().getInt(Constants.INTENT_SERVER_NO);
        textViewMain.append("\n"
            + getString(R.string.changed_server, Constants.SERVER_NAME[serverNumber]) + "\n");
        serverName = Constants.SERVER_NAME[serverNumber];
        serverHost = Constants.SERVER_HOST[serverNumber];
      }
    }
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

  /**
   * Gets the system related properties.
   * 
   * @return a string describing the OS and Java environment
   */
  private String getSystemProperty() {
    String osName, osArch, osVer, javaVer, javaVendor;
    osName = System.getProperty("os.name");
    osArch = System.getProperty("os.arch");
    osVer = System.getProperty("os.version");
    javaVer = System.getProperty("java.version");
    javaVendor = System.getProperty("java.vendor");
    StringBuilder sb = new StringBuilder()
        .append("\n")
        .append(getString(R.string.os_line, osName, osArch, osVer))
        .append("\n")
        .append(getString(R.string.java_line, javaVer, javaVendor));
    return sb.toString();
  }

  /**
   * Gets the mobile device related properties.
   * 
   * @return a string about location, network type (MOBILE or WIFI)
   */
  private String getMobileProperty() {
    StringBuilder sb = new StringBuilder();
    if (ndtLocation.location != null) {
      Log.v(Constants.LOG_TAG, ndtLocation.location.toString());
      sb.append(getString(R.string.latitude_result, ndtLocation.location.getLatitude()))
          .append("\n")
          .append(getString(R.string.longitude_result, ndtLocation.location.getLongitude()));
    }
    if (networkInfo != null) {
      Log.v(Constants.LOG_TAG, networkInfo.toString());
      sb.append("\n")
          .append(getString(R.string.network_type_indicator, networkInfo.getTypeName()));
    }
    return sb.toString();
  }

  /**
   * Initializes the components on main view.
   */
  private void initComponents() {
    buttonStart = (Button) findViewById(R.id.ButtonStart);
    buttonStart.setOnClickListener(new StartButtonListener());
    buttonAbout = (Button) findViewById(R.id.ButtonAbout);
    buttonAbout.setOnClickListener(new AboutButtonListener(this));
    buttonOption = (Button) findViewById(R.id.ButtonOption);
    buttonOption.setOnClickListener(new OptionButtonListener());
    buttonStatistics = (Button) findViewById(R.id.ButtonStatistics);
    buttonStatistics.setOnClickListener(new StatisticsButtonListener());
    buttonStatistics.setEnabled(false);
    progressBar = (ProgressBar) findViewById(R.id.ProgressBar);
    progressBar.setVisibility(View.INVISIBLE);
    progressBar.setIndeterminate(false);
  }
  
  /**
   * Gets the type of the active network, networkInfo should be initialized before called this function.
   * 
   * @return a string to describe user's network
   * @see NdtTests#NETWORK_WIFI
   * @see NdtTests#NETWORK_MOBILE
   * @see NdtTests#NETWORK_WIRED
   * @see NdtTests#NETWORK_UNKNOWN
   */
  private String getNetworkType() {
    if (networkInfo != null) {
      int networkType = networkInfo.getType();
      switch (networkType) {
        case ConnectivityManager.TYPE_MOBILE:
          return NdtTests.NETWORK_MOBILE;
        case ConnectivityManager.TYPE_WIFI:
          return NdtTests.NETWORK_WIFI;
        default:
          return NdtTests.NETWORK_UNKNOWN;
      }
    } else {
      return NdtTests.NETWORK_UNKNOWN;
    }
  }

  public class UiHandler extends Handler {
    public UiHandler(Looper looper) {
      super(looper);
    }

    @Override
    public void handleMessage(Message message) {
      switch (message.what) {
        case Constants.THREAD_MAIN_APPEND:
          textViewMain.append(message.obj.toString());
          break;
        case Constants.THREAD_STAT_APPEND:
          statistics += message.obj.toString();
          break;
        case Constants.THREAD_BEGIN_TEST:
          Log.v(Constants.LOG_TAG, "Begin the test");
          progress = 0;
          buttonStart.setEnabled(false);
          buttonAbout.setEnabled(false);
          buttonStatistics.setEnabled(false);
          buttonOption.setEnabled(false);
          progressBar.setVisibility(View.VISIBLE);
          progressBar.setProgress(progress);
          progressBar.setMax(UiServices.TEST_STEPS);
          if (wakeLock.isHeld() == false) {
            wakeLock.acquire();
            Log.v(Constants.LOG_TAG, "wakeLock acquired");
          }
          break;
        case Constants.THREAD_END_TEST:
          Log.v(Constants.LOG_TAG, "End the test");
          textViewMain.append("\n" + getString(R.string.test_again) + "\n");
          statistics += "\n";
          buttonStart.setEnabled(true);
          buttonAbout.setEnabled(true);
          buttonStatistics.setEnabled(true);
          buttonOption.setEnabled(true);
          progressBar.setVisibility(View.INVISIBLE);
          if (wakeLock.isHeld()) {
            wakeLock.release();
            Log.v(Constants.LOG_TAG, "wakeLock released");
          }
          break;
        case Constants.THREAD_ADD_PROGRESS:
          progressBar.setProgress(progressBar.getProgress() + 1);
          break;
      }
    }
  }
}
