// Copyright 2009 Google Inc. All Rights Reserved.

package net.measurementlab.ndt;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.RadioGroup;

/**
 * User select the testing server here and pass the related value back to the main activity.
 */
public class SelectServer extends Activity implements RadioGroup.OnCheckedChangeListener {
  private int serverNo = 0;
  private RadioButton server[] = new RadioButton[Constants.NUMBER_OF_SERVERS];
  private Button buttonOptionSave;
  private RadioGroup radioGroup;

  /**
   * Reads the server list from Constants class. Records the user's choice and
   * send it back to the main activity.
   */
  @Override
  public void onCreate(Bundle icicle) {
    super.onCreate(icicle);
    setContentView(R.layout.servers);
    Bundle serverInfo = getIntent().getExtras();
    if (serverInfo != null) {
      serverNo = serverInfo.getInt(Constants.INTENT_SERVER_NO);
      Log.v("NDT", "Passed Into Option: " + serverNo);
    }
    radioGroup = (RadioGroup) findViewById(R.id.servergroup);
    buttonOptionSave = (Button) findViewById(R.id.ButtonServerSave);
    for (int i = 0; i < Constants.NUMBER_OF_SERVERS; i++) {
      server[i] = new RadioButton(this);
      server[i].setText(Constants.SERVER_NAME[i]);
      radioGroup.addView(server[i]);
    }
    server[serverNo].setChecked(true);
    radioGroup.setOnCheckedChangeListener(this);
    buttonOptionSave.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        Intent intentReturn = new Intent();
        intentReturn.putExtra(Constants.INTENT_SERVER_NO, serverNo);
        setResult(RESULT_OK, intentReturn);
        finish();
      }
    });
  }

  /**
   * Responds to the change in selection.
   */
  @Override
  public void onCheckedChanged(RadioGroup radioGroup, int checkedId) {
    for (int i = 0; i < Constants.NUMBER_OF_SERVERS; i++) {
      if (server[i].getId() == checkedId) {
        serverNo = i;
      }
    }
  }
}
