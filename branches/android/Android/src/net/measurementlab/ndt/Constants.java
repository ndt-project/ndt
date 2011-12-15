package net.measurementlab.ndt;

/**
 * Definition for constant values used in NDT mobile.
 */
interface Constants {

	/**
	 * TAG constant for logging.
	 */
	static final String LOG_TAG = "NDT";

	// Options
	// See newest server list here:
	// http://www.measurementlab.net/measurement-lab-tools#ndt
	// All the data here should be kept sync with the published list
	// In future server list should be downloaded from some list servers
	// dynamically.
	public static final int DEFAULT_SERVER = 0;
	public static final String SERVER_LIST[][] = {
		{"Closest Server (DONAR)", "ndt.iupui.donar.measurement-lab.org"}, 
		{"Amsterdam, The Netherlands", "ndt.iupui.ams.donar.measurement-lab.org"},
		{"Athens, Greece", "ndt.iupui.ath.donar.measurement-lab.org"},
		{"Atlanta, Georgia", "ndt.iupui.atl.donar.measurement-lab.org"},
		{"Chicago, Illinois", "ndt.iupui.ord.donar.measurement-lab.org"},
		{"Dallas, Texas", "ndt.iupui.dfw.donar.measurement-lab.org"},
		{"Dulles, Virginia", "ndt.iupui.iad.donar.measurement-lab.org"},
		{"Los Angeles, California", "ndt.iupui.lax.donar.measurement-lab.org"},
		{"London, United Kingdom", "ndt.iupui.lhr.donar.measurement-lab.org"},
		{"Miami, Florida", "ndt.iupui.mia.donar.measurement-lab.org"},
		{"New York City, New York", "ndt.iupui.lga.donar.measurement-lab.org"},
		{"Mountain View, California", "ndt.iupui.nuq.donar.measurement-lab.org"},
		{"Paris, France", "ndt.iupui.par.donar.measurement-lab.org"},
		{"Seattle, Washington", "ndt.iupui.sea.donar.measurement-lab.org"},
		{"Sydney, Australia", "ndt.iupui.syd.donar.measurement-lab.org"},
		{"Tokyo, Japan", "ndt.iupui.hnd.donar.measurement-lab.org"},
		{"Wellington, New Zealand", "ndt.iupui.wlg.donar.measurement-lab.org"}
		};

	/**
	 * Number of servers. All the arrays should have the same length.
	 */
	public static final int NUMBER_OF_SERVERS = SERVER_LIST.length;
}
