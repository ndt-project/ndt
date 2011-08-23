package net.measurementlab.ndt;

import java.text.DecimalFormat;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

/**
 * Activity to display the results of the testing, including summary, detailed
 * and expert views.
 * 
 * @author gideon@newamerica.net
 * 
 */
public class ResultsActivity extends Activity {

	/**
	 * {@inheritDoc}
	 */
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.results);

		// surprising that typeface cannot be set in styles.xml or
		// even in the respective layout .xml file
		NdtSupport.applyFont(this, R.id.ResultsHeader);
		NdtSupport.applyFont(this, R.id.UploadSpeedHeader, R.id.UploadSpeed,
				R.id.UploadSpeedMbps);
		NdtSupport.applyFont(this, R.id.DownloadSpeedHeader,
				R.id.DownloadSpeed, R.id.DownloadSpeedMbps);
		NdtSupport.applyFont(this, R.id.DetailedResultsHeader,
				R.id.DetailedResultsInfo);
		NdtSupport.applyFont(this, R.id.AdvancedResultsHeader,
				R.id.AdvancedResultsInfo);
		NdtSupport.applyFont(this, R.id.MoreInfo);

		OnClickListener aboutListener = new OnClickListener() {

			@Override
			public void onClick(View v) {
				Intent intent = new Intent(Intent.ACTION_VIEW, Uri
						.parse("http://measurementlab.net"));
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

	/**
	 * {@inheritDoc}
	 */
	@SuppressWarnings("unchecked")
	@Override
	protected void onResume() {
		super.onResume();
		Intent intent = getIntent();
		Map<String, Object> variables = (Map<String, Object>) intent
				.getSerializableExtra(NdtService.EXTRA_VARS);
		formatSummaryResults(variables);
		formatDetailedResults(variables);
		String diagnosticStatus = intent.getStringExtra(
				NdtService.EXTRA_DIAG_STATUS);
		formatAdvancedResults(diagnosticStatus);
		
		Toast resultsHint = Toast.makeText(getApplicationContext(), getString(R.string.results_swipe_hint), 10);
		resultsHint.show();
	}

	private void formatSummaryResults(Map<String, Object> variables) {
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
		variables.put("pub_jitter", maxRtt - minRtt);

		String latency = getString(R.string.results_latency, Math.round(avgRtt));
		textView = (TextView) findViewById(R.id.Latency);
		textView.setText(latency);

		String jitter = getString(R.string.results_jitter, maxRtt - minRtt);
		textView = (TextView) findViewById(R.id.Jitter);
		textView.setText(jitter);
	}

	private void formatDetailedResults(Map<String, Object> variables) {
		StringBuilder results = new StringBuilder();
		results.append(formatDetailedLine(R.string.results_detailed_system,
				variables, "pub_osName", "pub_osVer", "pub_javaVer",
				"pub_osArch"));
		results.append('\n');

		results.append(formatDetailedLine(R.string.results_detailed_tcp,
				variables, "pub_CurRwinRcvd", "pub_MaxRwinRcvd"));
		results.append(formatDetailedLine(
				R.string.results_detailed_packet_loss, variables, "pub_loss"));
		results.append(formatDetailedLine(R.string.results_detailed_rtt,
				variables, "pub_MinRTT", "pub_MaxRTT", "pub_avgrtt"));
		results.append(formatDetailedLine(R.string.results_detailed_jitter,
				variables, "pub_jitter"));
		Integer curRTO = (Integer) variables.get("pub_CurRTO");
		Integer timeouts = (Integer) variables.get("pub_Timeouts");
		if (null == timeouts) {
			timeouts = 0;
		}
		Integer waitSec = (curRTO * timeouts)/1000;
		variables.put("pub_WaitSec", waitSec);
		results.append(formatDetailedLine(R.string.results_detailed_timeout,
				variables, "pub_WaitSec", "pub_CurRTO"));
		results.append(formatDetailedLine(R.string.results_detailed_ack,
				variables, "pub_SACKsRcvd"));
		
		results.append('\n');
		// TODO finish filling in the rest

//		  if (a.get_mismatch() == "yes") {
//		    d += "A duplex mismatch condition was detected.<br>";
//		  }
//		  else {
//		    d += "No duplex mismatch condition was detected.<br>";
//		  }
//
//		  if (a.get_Bad_cable() == "yes") {
//		    d += "The test detected a cable fault.<br>";
//		  }
//		  else {
//		    d += "The test did not detect a cable fault.<br>";
//		  }
//
//		  if (a.get_congestion() == "yes") {
//		    d += "Network congestion may be limiting the connection.<br>";
//		  }
//		  else {
//		    d += "No network congestion was detected.<br>";
//		  }
//
//		  if (a.get_natStatus() == "yes") {
//		    d += "A network addess translation appliance was detected.<br>";
//		  }
//		  else {
//		    d += "No network addess translation appliance was detected.<br>";
//		  }
//
//		  d += "<br>";
//
//		  d += a.get_cwndtime() + "% of the time was not spent in a receiver limited or sender limited state.<br>";
//		  d += a.get_rcvrLimiting() + "% of the time the connection is limited by the client machine's receive buffer.<br>";
//		  d += "Optimal receive buffer: " + a.get_optimalRcvrBuffer() + " bytes<br>";
//		  d += "Bottleneck link: " + a.get_AccessTech() + "<br>";
//		  d += a.get_DupAcksOut() + " duplicate ACKs set<br>";


		TextView textView = (TextView) findViewById(R.id.DetailedResultsInfo);
		textView.setText(results);
	}

	private void formatAdvancedResults(String diagnosticStatus) {
		String advanced = getString(R.string.results_advanced_web100,
				diagnosticStatus);
		TextView textView = (TextView) findViewById(R.id.AdvancedResultsInfo);
		textView.setText(advanced);
	}

	private String formatDetailedLine(int templateId,
			Map<String, Object> variables, String... paramKeys) {
		List<Object> params = new LinkedList<Object>();
		for (String paramKey : paramKeys) {
			params.add(variables.get(paramKey));
		}
		String message = getString(templateId, params.toArray(new Object[params
				.size()]));
		return message.concat("\n");
	}
}
