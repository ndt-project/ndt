package net.measurementlab.ndt;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

public class ResultsActivity extends Activity {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.results);
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		String statistics = getIntent().getStringExtra("statistics");
		TextView textView = (TextView) findViewById(R.id.Statistics);
		textView.setText(statistics);
	}
}
