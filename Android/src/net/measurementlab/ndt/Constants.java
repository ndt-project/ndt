// Copyright 2009 Google Inc. All Rights Reserved.

package net.measurementlab.ndt;

/**
 * Definition for constant values used in ndt mobile.
 */
public class Constants {
  /**
   * Maximum test steps for ProgressBar setting.
   */
  public static final int TEST_STEPS = 7;

  /**
   * TAG constant for logging.
   */
  public static final String LOG_TAG = "NDT";

  // Messages submitted from working thread to UI
  public static final int THREAD_MAIN_APPEND = 0;
  public static final int THREAD_STAT_APPEND = 1;
  public static final int THREAD_BEGIN_TEST = 2;
  public static final int THREAD_END_TEST = 3;
  public static final int THREAD_ADD_PROGRESS = 4;

  // ID of Activities
  public static final int ACTIVITY_OPTION = 0;
  public static final int ACTIVITY_STATISTICS = 1;
  
  // Intents ID
  public static final String INTENT_SERVER_NO = "serverno";
  public static final String INTENT_STATISTICS = "statistics";
  public static final String INTENT_LOCATION = "location";
  public static final String INTENT_NETWORK = "network";
  
  // Options
  // See newest server list here: http://www.measurementlab.net/measurement-lab-tools#ndt
  // All the data here should be kept sync with the published list
  // In future server list should be downloaded from some list servers dynamically.
  public static final int DEFAULT_SERVER = 0;
  public static final String SERVER_NAME[] =
      {"Closest Server (DONAR)", "Mountain View, California", "Los Angeles, California", "Seattle, Washington",
          "Dallas, Texas", "Chicago, Illinois", "Atlanta, Georgia", "Miami, Florida",
          "New York City, New York #1", "New York City, New York #2", "London, United Kingdom",
          "Paris, France", "Amsterdam, The Netherlands #1", "Athens, Greece", "Sydney, Australia"};
  public static final String SERVER_HOST[] =
      {"ndt.iupui.donar.measurement-lab.org", 
          "ndt.iupui.nuq01.measurement-lab.org", "ndt.iupui.lax01.measurement-lab.org",
          "ndt.iupui.sea01.measurement-lab.org", "ndt.iupui.dfw01.measurement-lab.org",
          "ndt.iupui.ord01.measurement-lab.org", "ndt.iupui.atl01.measurement-lab.org",
          "ndt.iupui.mia01.measurement-lab.org", "ndt.iupui.lga01.measurement-lab.org",
          "ndt.iupui.lga02.measurement-lab.org", "ndt.iupui.lhr01.measurement-lab.org",
          "ndt.iupui.par01.measurement-lab.org", "ndt.iupui.ams01.measurement-lab.org",
          "ndt.iupui.ath01.measurement-lab.org", "ndt.iupui.syd01.measurement-lab.org"};

  /**
   * Number of servers. All the arrays should have the same length.
   */
  public static final int NUMBER_OF_SERVERS = SERVER_NAME.length;

  private Constants() {
    // private constructor to make sure it can't be instantiated
  }
}
