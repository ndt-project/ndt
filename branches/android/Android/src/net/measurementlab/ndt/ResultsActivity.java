package net.measurementlab.ndt;

import java.text.DecimalFormat;
import java.text.MessageFormat;
import java.util.Map;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;

public class ResultsActivity extends Activity {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.results);

		Typeface typeFace = Typeface.createFromAsset(getAssets(), "fonts/League_Gothic.otf");
		TextView textView = (TextView) findViewById(R.id.ResultsHeader);
		textView.setTypeface(typeFace);
		
		textView = (TextView) findViewById(R.id.UploadSpeedHeader);
		textView.setTypeface(typeFace);
		textView = (TextView) findViewById(R.id.UploadSpeed);
		textView.setTypeface(typeFace);
		textView = (TextView) findViewById(R.id.UploadSpeedMbps);
		textView.setTypeface(typeFace);
		
		textView = (TextView) findViewById(R.id.DownloadSpeedHeader);
		textView.setTypeface(typeFace);
		textView = (TextView) findViewById(R.id.DownloadSpeed);
		textView.setTypeface(typeFace);
		textView = (TextView) findViewById(R.id.DownloadSpeedMbps);
		textView.setTypeface(typeFace);

		textView = (TextView) findViewById(R.id.DetailedResultsHeader);
		textView.setTypeface(typeFace);

		textView = (TextView) findViewById(R.id.AdvancedResultsHeader);
		textView.setTypeface(typeFace);

		textView = (TextView) findViewById(R.id.MoreInfo);
		textView.setTypeface(typeFace);

		OnClickListener aboutListener = new OnClickListener() {

			@Override
			public void onClick(View v) {
				Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("http://measurementlab.net"));
				startActivity(intent);
			}
		};
		View aboutView = findViewById(R.id.MoreInfo);
		aboutView.setOnClickListener(aboutListener);
		aboutView = findViewById(R.id.MLabLogo);
		aboutView.setOnClickListener(aboutListener);
		

		Button startButton = (Button) findViewById(R.id.ButtonStart);
		startButton.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				Intent intent = new Intent(getApplicationContext(),
						TestsActivity.class);
				startActivity(intent);
			}
		});
	}
	
	@SuppressWarnings("unchecked")
	@Override
	protected void onResume() {
		super.onResume();
		Map<String,Object> variables = (Map<String,Object>) getIntent().getSerializableExtra(NdtService.EXTRA_VARS);
		
		DecimalFormat format = (DecimalFormat) DecimalFormat.getInstance();
		format.setMaximumFractionDigits(2);

		Double uploadSpeed = (Double) variables.get("pub_c2sspd");
		TextView textView = (TextView) findViewById(R.id.UploadSpeed);
		textView.setText(format.format(uploadSpeed));

		Double downloadSpeed = (Double) variables.get("pub_s2cspd");
		textView = (TextView) findViewById(R.id.DownloadSpeed);
		textView.setText(format.format(downloadSpeed));
		
		Integer maxRtt = (Integer) variables.get("pub_MaxRTT");
		Integer minRtt = (Integer) variables.get("pub_MinRTT");
		Double avgRtt = (Double) variables.get("pub_avgrtt");
		
		String latency = getResources().getString(R.string.results_latency);
		latency = MessageFormat.format(latency, (int)(double)avgRtt);
		
		textView = (TextView) findViewById(R.id.Latency);
		textView.setText(latency);
		
		String jitter = getResources().getString(R.string.results_jitter);
		jitter = MessageFormat.format(jitter, maxRtt - minRtt);
		
		textView = (TextView) findViewById(R.id.Jitter);
		textView.setText(jitter);
	}
}
