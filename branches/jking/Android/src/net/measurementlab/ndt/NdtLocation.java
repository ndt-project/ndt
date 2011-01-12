// Copyright 2009 Google Inc. All Rights Reserved.

package net.measurementlab.ndt;

import android.content.Context;
import android.location.Criteria;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Bundle;

/**
 * Handle the location related functions and listeners.
 */
public class NdtLocation implements LocationListener {
  /**
   * Location variable, publicly accessible to provide access to geographic data.
   */
  public Location location;
  private LocationManager locationManager;
  private Criteria criteria;

  /**
   * Passes context to this class to initialize members.
   * 
   * @param context context which is currently running
   */
  public NdtLocation(Context context) {
    locationManager = (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);
    criteria = new Criteria();
    criteria.setAccuracy(Criteria.ACCURACY_FINE);
    criteria.setCostAllowed(true);
    criteria.setPowerRequirement(Criteria.POWER_LOW);
  }

  public void onLocationChanged(Location location) {
    this.location = location;
  }

  public void onProviderDisabled(String provider) {
    // Don't care when the provider is disabled.
  }

  public void onProviderEnabled(String provider) {
    // Don't care when the provider is enabled.
  }

  public void onStatusChanged(String provider, int status, Bundle extras) {
    // Don't care when the provider is temporarily unavailable.
  }

  /**
   * Stops requesting the location update.
   */
  public void stopListen() {
    locationManager.removeUpdates(this);
  }

  /**
   * Begins to request the location update.
   */
  public void startListen() {
    locationManager.requestLocationUpdates(locationManager.getBestProvider(criteria, true), 0, 0,
        this);
    location =
        locationManager.getLastKnownLocation(locationManager.getBestProvider(criteria, true));
  }
}
