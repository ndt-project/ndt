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
    date = new Date();
    buttonEmail = null;
    buttonEmail.setOnClickListener(this);
    textViewStatistics = null;
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
