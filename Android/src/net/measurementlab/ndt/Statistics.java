// Copyright 2009 Google Inc. All Rights Reserved.

package net.measurementlab.ndt;

import java.util.Date;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

/**
 * Shows the testing report to the user, email application also could be called here.
 */
public class Statistics extends Activity implements OnClickListener {
  private Button buttonEmail;
  private String statistics;
  private String location;
  private String network;
  private Date date;
  private TextView textViewStatistics;

  /**
   * Gets results from the main activity and shows them to the user.
   */
  @Override
  public void onCreate(Bundle icicle) {
    super.onCreate(icicle);
    setContentView(R.layout.stats);
    Bundle statsInfo = getIntent().getExtras();
    date = new Date();
    buttonEmail = (Button) findViewById(R.id.ButtonEmail);
    buttonEmail.setOnClickListener(this);
    if (statsInfo != null) {
      statistics = statsInfo.getString(Constants.INTENT_STATISTICS);
      location = statsInfo.getString(Constants.INTENT_LOCATION);
      network = statsInfo.getString(Constants.INTENT_NETWORK);
    } else {
      Toast toast =
          Toast.makeText(getApplicationContext(), R.string.intent_error, Toast.LENGTH_SHORT);
      toast.show();
      finish();
    }
    textViewStatistics = (TextView) findViewById(R.id.TextViewStatistics);
    textViewStatistics.setText(statistics);
  }

  /**
   * Invokes the email application. Does not work on the emulator.
   */
  @Override
  public void onClick(View view) {
    Intent intentMail = new Intent(Intent.ACTION_SEND);
    String networkTypeResult = getString(R.string.network_type_indicator, network);
    StringBuilder mail = new StringBuilder()
        .append(statistics)
        .append("\n")
        .append(location)
        .append("\n")
        .append(networkTypeResult);
    intentMail.putExtra(Intent.EXTRA_TEXT, mail.toString());
    String emailTitle = getString(R.string.email_title, date.toString());
    intentMail.putExtra(Intent.EXTRA_SUBJECT, emailTitle);
    intentMail.setType("plain/text");
    startActivity(Intent.createChooser(intentMail, "Choose Email Client"));
  }
}
