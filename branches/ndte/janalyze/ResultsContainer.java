import java.awt.Component;
import java.util.StringTokenizer;

import javax.swing.BoxLayout;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;


public class ResultsContainer {
	private double[] run_ave = new double[4];
	private double[] runave = new double[4];
	private int[][] links = new int[4][16];
	private int n = 0, m = 0, port;
	private String date, ip_addr, ip_addr2, btlneck;
	private int s2c2spd, s2cspd, c2sspd;
	private int timeouts, sumRTT, countRTT, pktsRetrans, fastRetrans,
	            dataPktsOut, ackPktsOut, currentMSS, dupAcksIn, ackPktsIn,
	            maxRwinRcvd, sndBuf, currentCwnd, sndLimTimeRwin, sndLimTimeCwnd,
	            sndLimTimeSender, dataBytesOut, sndLimTransRwin, sndLimTransCwnd,
	            sndLimTransSender, maxSsthresh, currentRTO, currentRwinRcvd, link,
	            mismatch, bad_cable, half_duplex, congestion, c2sdata, c2sack,
	            s2cdata, s2cack, pktsOut;
	private int congestionSignals = -1, minRTT = -1, rcvWinScale = -1, autotune = -1,
	            congAvoid = -1, congestionOverCount = 0, maxRTT = 0, otherReductions = 0,
	            curTimeouts = 0, abruptTimeouts = 0, sendStall = 0, slowStart = 0,
	            subsequentTimeouts = 0, thruBytesAcked = 0;
	private int linkcnt, mismatch2, mismatch3;	
	private double idle, cpu_idle1 = -1, cpu_idle2 = -1, loss, loss2, order, bw, bw2;
	private double rwintime, cwndtime, sendtime, timesec;
	private double recvbwd, cwndbwd, sendbwd;		  
	private int congestion2=0;		  
	private double acks, aspeed, pureacks, avgrtt;
	private double cong, retrn, increase, touts, fr_ratio = 0;
	private double retransec, mmorder;
	private int[] tail = new int[4];
	private int[] head = new int[4];
	
	public void parseSpds(String line) {
		String spdsLine = line.substring(line.indexOf("'") + 1, line.lastIndexOf("'"));		
		StringTokenizer st = new StringTokenizer(spdsLine.trim(), " ");
		linkcnt = 0;
		while (st.hasMoreTokens()) {
			String number = st.nextToken();
			try {
			    links[n][linkcnt] = Integer.parseInt(number);
			    linkcnt++;
			}
			catch (NumberFormatException exc) {
				runave[n] = Double.parseDouble(number);
				linkcnt++;
				break;
			}			
		}		
		n++;
	}

	public void parseRunAvg(String line) {
		StringTokenizer st = new StringTokenizer(line.trim(), " ");
		st.nextToken(); st.nextToken(); st.nextToken();
		run_ave[m] = Double.parseDouble(st.nextToken());		
		m++;
	}

	public void parsePort(String line) {		
//		System.out.println("Start of New Packet trace -- " + line);
		String dateAndHost = line.substring(0, line.indexOf("port") - 1);
		date = dateAndHost.substring(0, dateAndHost.lastIndexOf(" ")).trim();
		ip_addr = dateAndHost.substring(dateAndHost.lastIndexOf(" ")).trim();
		port = Integer.parseInt(line.substring(line.indexOf("port") + 5));
	}

	public boolean parseWeb100Var(String line) {
		StringTokenizer st = new StringTokenizer(line.trim(), ",");
		st.nextToken();
		ip_addr2 = st.nextToken();
		if (st.hasMoreTokens() == false) {
			return false;
		}
		try {
			s2c2spd = Integer.parseInt(st.nextToken());
			s2cspd = Integer.parseInt(st.nextToken());
			c2sspd = Integer.parseInt(st.nextToken());
			timeouts = Integer.parseInt(st.nextToken());
			sumRTT = Integer.parseInt(st.nextToken());
			countRTT = Integer.parseInt(st.nextToken());
			pktsRetrans = Integer.parseInt(st.nextToken());
			fastRetrans = Integer.parseInt(st.nextToken());
			dataPktsOut = Integer.parseInt(st.nextToken());
			ackPktsOut = Integer.parseInt(st.nextToken());
			currentMSS = Integer.parseInt(st.nextToken());
			dupAcksIn = Integer.parseInt(st.nextToken());
			ackPktsIn = Integer.parseInt(st.nextToken());
			maxRwinRcvd = Integer.parseInt(st.nextToken());
			sndBuf = Integer.parseInt(st.nextToken());
			currentCwnd = Integer.parseInt(st.nextToken());
			sndLimTimeRwin = Integer.parseInt(st.nextToken());
			sndLimTimeCwnd = Integer.parseInt(st.nextToken());
			sndLimTimeSender = Integer.parseInt(st.nextToken());
			dataBytesOut = Integer.parseInt(st.nextToken());
			sndLimTransRwin = Integer.parseInt(st.nextToken());
			sndLimTransCwnd = Integer.parseInt(st.nextToken());
			sndLimTransSender = Integer.parseInt(st.nextToken());
			maxSsthresh = Integer.parseInt(st.nextToken());
			currentRTO = Integer.parseInt(st.nextToken());
			currentRwinRcvd = Integer.parseInt(st.nextToken());
			link = Integer.parseInt(st.nextToken());
			mismatch = Integer.parseInt(st.nextToken());
			bad_cable = Integer.parseInt(st.nextToken());
			half_duplex = Integer.parseInt(st.nextToken());
			congestion = Integer.parseInt(st.nextToken());
			c2sdata = Integer.parseInt(st.nextToken());
			c2sack = Integer.parseInt(st.nextToken());
			s2cdata = Integer.parseInt(st.nextToken());
			s2cack = Integer.parseInt(st.nextToken());
			congestionSignals = Integer.parseInt(st.nextToken());
			pktsOut = Integer.parseInt(st.nextToken());
			minRTT = Integer.parseInt(st.nextToken());
			rcvWinScale = Integer.parseInt(st.nextToken());
			autotune = Integer.parseInt(st.nextToken());
			congAvoid = Integer.parseInt(st.nextToken());
			congestionOverCount = Integer.parseInt(st.nextToken());
			maxRTT = Integer.parseInt(st.nextToken());
			otherReductions = Integer.parseInt(st.nextToken());
			curTimeouts = Integer.parseInt(st.nextToken());
			abruptTimeouts = Integer.parseInt(st.nextToken());
			sendStall = Integer.parseInt(st.nextToken());
			slowStart = Integer.parseInt(st.nextToken());
			subsequentTimeouts = Integer.parseInt(st.nextToken());
			thruBytesAcked = Integer.parseInt(st.nextToken());
			cpu_idle1 = Integer.parseInt(st.nextToken());
			cpu_idle2 = Integer.parseInt(st.nextToken());
		}
		catch (Exception exc) {
			// do nothing			
		}
		return true;
	}

	public void calculate() {		
		int i, j, totaltime;
		double k, rttsec, spd;

		tail[0] = tail[1] = tail[2] = tail[3] = 0;
		head[0] = head[1] = head[2] = head[3] = 0;

		for (i=0; i<4; i++) {
			int max = 0;
			int indx = 0;
			int total = 0;
			for (j=0; j<linkcnt-1; j++) {
				total += links[i][j];
				if (max < links[i][j]) {
					max = links[i][j];
					indx = j;
				}
			}
			for (j=indx+1; j<10; j++) {
				k = (float) links[i][j] / max;
				if (k > .1)
					tail[i]++;
			}
			for (j=0; j<indx; j++) {
				k = (float) links[i][j] / max;
				if (k > .1)
					head[i]++;
			}
			if (links[i][indx] == -1)
				indx = -1;
			if ((total < 20) && (indx != -1))
				indx = -2;
			switch (i) {
			case 0: c2sdata = indx;
//			System.out.println("Client --> Server data detects link = " + describeDetLink(indx));
			break;
			case 1: c2sack = indx;
//			System.out.println("Client <-- Server Ack's detect link = " + describeDetLink(indx));
			break;
			case 2: s2cdata = indx;
//			System.out.println("Server --> Client data detects link = " + describeDetLink(indx));
			break;
			case 3: s2cack = indx;
//			System.out.println("Server <-- Client Ack's detect link = " + describeDetLink(indx));
			break;		              
			}
		}
		switch (c2sdata) {
		case -2: btlneck = "Insufficent Data"; break;
		case -1: btlneck = "a System Fault"; break;
		case 0: btlneck = "the Round Trip Time"; break;
		case 1: btlneck = "a 'Dial-up modem' connection"; break;
		case 2:
			if ((c2sspd/s2cspd > .8) && (c2sspd/s2cspd < 1.2) && (c2sspd > 1000))
				btlneck = "a 'T1' subnet";
			else {
				if ((tail[3] > 1) || (s2cack == 3))
					btlneck = "a 'Cable Modem' connection";
				else
					btlneck = "a 'DSL' connection";
			}
			break;
		case 3:  if (linkcnt == 16)
			btlneck = "a T1 + subnet";
		else
			btlneck = "an 'Ethernet' subnet";
		break;
		case 4:  if (linkcnt == 16)
			btlneck = "a IEEE 802.11b Wifi subnet";
		else
			btlneck = "a 'T3/DS-3' subnet";
		break;
		case 5:  if (linkcnt == 16)
			btlneck = "a Wifi + subnet";
		else
			btlneck = "a 'FastEthernet' subnet";
		break;
		case 6:  if (linkcnt == 16)
			btlneck = "a Ethernet subnet";
		else
			btlneck = "an 'OC-12' subnet";
		break;
		case 7:  if (linkcnt == 16)
			btlneck = "a T3/DS3 subnet";
		else
			btlneck = "a 'Gigabit Ethernet' subnet";
		break;
		case 8:  if (linkcnt == 16)
			btlneck = "a FastEthernet subnet";
		else
			btlneck = "an 'OC-48' subnet";
		break;
		case 9:  if (linkcnt == 16)
			btlneck = "a OC-12 subnet";
		else
			btlneck = "a '10 Gigabit Enet' subnet";
		break;
		case 10: if (linkcnt == 16)
			btlneck = "a Gigabit Ethernet subnet";
		else
			btlneck = "Retransmissions";
		case 11: btlneck = "an 'OC-48' subnet"; break;
		case 12: btlneck = "a '10 Gigabit Enet' subnet"; break;
		case 13: btlneck = "Retransmissions"; break;
		}
		/* Calculate some values */
		avgrtt = (double) sumRTT/countRTT;
		rttsec = avgrtt * .001;
		loss = (double)(pktsRetrans- fastRetrans)/(double)(dataPktsOut-ackPktsOut);
		loss2 = (double)congestionSignals/pktsOut;
		if (loss == 0)
			loss = .0000000001;  /* set to 10^-6 for now */
		if (loss2 == 0)
			loss2 = .0000000001;  /* set to 10^-6 for now */

		order = (double)dupAcksIn/ackPktsIn;
		bw = (currentMSS / (rttsec * Math.sqrt(loss))) * 8 / 1024 / 1024;
		bw2 = (currentMSS / (rttsec * Math.sqrt(loss2))) * 8 / 1024 / 1024;
		totaltime = sndLimTimeRwin + sndLimTimeCwnd + sndLimTimeSender;
		rwintime = (double)sndLimTimeRwin/totaltime;
		cwndtime = (double)sndLimTimeCwnd/totaltime;
		sendtime = (double)sndLimTimeSender/totaltime;
		timesec = totaltime/1000000;
		idle = (timeouts * ((double)currentRTO/1000))/timesec;
		retrn = (float)pktsRetrans / pktsOut;
		increase = (float)congAvoid / pktsOut;

		recvbwd = ((maxRwinRcvd*8)/avgrtt)/1000;
		cwndbwd = ((currentCwnd*8)/avgrtt)/1000;
		sendbwd = ((sndBuf*8)/avgrtt)/1000;

		spd = ((double)dataBytesOut / (double)totaltime) * 8;

		mismatch2 = 0;
		mismatch3 = 0;
		mmorder = (float)(dataPktsOut - pktsRetrans - fastRetrans) / dataPktsOut;
		cong = (float)(congestionSignals - congestionOverCount) / pktsOut;
		touts = (float)timeouts / pktsOut;
		if (pktsRetrans > 0)
			fr_ratio = fastRetrans / pktsRetrans;
		retransec = pktsRetrans / timesec;

		/* new test based on analysis of TCP behavior in duplex mismatch condition. */

		acks = (double) ackPktsIn / (double) dataPktsOut;
		pureacks = (double) (ackPktsIn - dupAcksIn) / (double) dataPktsOut;
		if (s2cspd < c2sspd)
			aspeed = (double)c2sspd / (double)s2cspd; 
		else
			aspeed = (double)s2cspd / (double)c2sspd;
//		System.out.println("Acks = " + Helpers.formatDouble(acks, 4) + ", async speed = " + Helpers.formatDouble(aspeed, 4) +
//				", mismatch3 = " + Helpers.formatDouble(cong, 4) + ", CongOver = " + congestionOverCount);
//		System.out.println("idle = " + Helpers.formatDouble(idle, 4) + ", timeout/pkts = " + Helpers.formatDouble(touts, 2) +
//				", %retranmissions = " + Helpers.formatDouble(retrn*100, 2) + ", %increase = " + Helpers.formatDouble(increase*100, 2));
//		System.out.println("FastRetrans/Total = " + Helpers.formatDouble(retrn, 4) + ", Fast/Retrans = " +
//				Helpers.formatDouble(fr_ratio, 4) + ", Retrans/sec = " + Helpers.formatDouble(retransec, 4));
		if (((acks > 0.7) || (acks < 0.3)) && (retrn > 0.03) && (congAvoid > slowStart)) {
			if ((2*currentMSS) == maxSsthresh) {
				mismatch2 = 1;
				mismatch3 = 0;
			} else if (aspeed > 15){
				mismatch2 = 2;
			}
		}
		if ((idle > 0.65) && (touts < 0.4)) {
			if (maxSsthresh == (2*currentMSS)) {
				mismatch2 = 0;
				mismatch3 = 1;
			} else {
				mismatch3 = 2;
			}
		}
		/* estimate is less than throughput, something is wrong */
		if (bw < spd)
			link = 0;

		if (((loss*100)/timesec > 15) && (cwndtime/timesec > .6) &&
				(loss < .01) && (maxSsthresh > 0))
			bad_cable = 1;

		/* test for Ethernet link (assume Fast E.) */
		if ((spd < 9.5) && (spd > 3.0) && (loss < .01) && (order < .035) && (link > 0))
			link = 10;

		/* test for DSL/Cable modem link */
		if ((sndLimTimeSender < 15000) && (spd < 2) && (spd < bw) && (link > 0))
			link = 2;

		if (((rwintime > .95) && (sndLimTransRwin/timesec > 30) &&
				(sndLimTransSender/timesec > 30)) || (link <= 10))
			half_duplex = 1;

		if ((cwndtime > .02) && (mismatch2 == 0) && (cwndbwd < recvbwd))
			congestion2 = 1;

//		System.out.println("Throughput to host [" + ip_addr + "] is limited by " + btlneck);

//		System.out.println("\tWeb100 says link = " + link + ", speed-chk says link = " + c2sdata);
//		System.out.println("\tSpeed-chk says {" + c2sdata + ", " + c2sack + ", " + s2cdata + ", " + s2cack +
//				"}, Running average = {" + Helpers.formatDouble(runave[0], 1) + ", " + Helpers.formatDouble(runave[1], 1) +
//				", " + Helpers.formatDouble(runave[2], 1) + ", " + Helpers.formatDouble(runave[3], 1) + "}");		      
//		if (c2sspd > 1000)
//			System.out.print("\tC2Sspeed = " + Helpers.formatDouble(c2sspd/1000.0, 2) + " Mbps, S2Cspeed = " +
//					Helpers.formatDouble(s2cspd/1000.0, 2) + " Mbps,\nCWND-Limited = " + Helpers.formatDouble(s2c2spd/1000.0, 2) + " Mbps, ");		        
//		else
//			System.out.print("\tC2Sspeed = " + c2sspd + " kbps, S2Cspeed = " + s2cspd + " kbps,\nCWND-Limited: " + s2c2spd + " kbps, ");
//		if (bw > 1)
//			System.out.println("Estimate = " + Helpers.formatDouble(bw, 2) + " Mbps (" + Helpers.formatDouble(bw2, 2) + " Mbps)");
//		else
//			System.out.println("Estimate = " + Helpers.formatDouble(bw*1000, 2) + " kbps (" +
//					Helpers.formatDouble(bw2*1000, 2) + " kbps)");
//
//		if ((bw*1000) > s2cspd)
//			System.out.print("\tOld estimate is greater than measured; ");
//		else
//			System.out.print("\tOld estimate is less than measured; ");
//
//		if (congestionSignals == -1)
//			System.out.println("No data collected to calculage new estimate");
//		else {
//			if ((bw2*1000) > s2cspd)
//				System.out.println("New estimate is greater than measured");
//			else
//				System.out.println("New estimate is less than measured");
//		}
//		System.out.println("\tLoss = " + Helpers.formatDouble(loss*100, 2) + "% (" + Helpers.formatDouble(loss2*100, 2) +
//				"%), Out-of-Order = " + Helpers.formatDouble(order*100, 2) +
//				"%, Long tail = {" + tail[0] + ", " + tail[1] + ", " + tail[2] + ", " + tail[3] + "}");				
//		System.out.println("\tDistribution = {" + head[0] + ", " + head[1] + ", " + head[2] + ", " + head[3] +
//				"}, time spent {r=" + Helpers.formatDouble(rwintime*100, 1) + "% c=" + Helpers.formatDouble(cwndtime*100, 1) +
//				"% s=" + Helpers.formatDouble(sendtime*100, 1) + "%}");				
//		System.out.println("\tAve(min) RTT = " + Helpers.formatDouble(avgrtt, 2) + " (" + minRTT +
//				") msec, Buffers = {r=" + maxRwinRcvd + ", c=" + currentCwnd + ", s=" + sndBuf/2 + "}");				
//		System.out.println("\tbw*delay = {r=" + Helpers.formatDouble(recvbwd, 2) + ", c=" +
//				Helpers.formatDouble(cwndbwd, 2) + ", s=" + Helpers.formatDouble(sendbwd, 2) +
//				"}, Transitions/sec = {r=" + Helpers.formatDouble(sndLimTransRwin/timesec, 1) + ", c=" +
//				Helpers.formatDouble(sndLimTransCwnd/timesec, 1) + ", s=" +
//				Helpers.formatDouble(sndLimTransSender/timesec, 1) + "}");								
//		System.out.println("\tRetransmissions/sec = " + Helpers.formatDouble(pktsRetrans/timesec, 1) + ", Timeouts/sec = " + Helpers.formatDouble(timeouts/timesec, 1) + ", SSThreshold = " + maxSsthresh);
//		System.out.print("\tMismatch = " + mismatch + " (" + mismatch2 + ":" + mismatch3 + "[" + Helpers.formatDouble(mmorder, 2) + "])");
//		if (mismatch3 == 1)
//			System.out.print(" [H=F, S=H]");
//		if (mismatch2 == 1)
//			System.out.print(" [H=H, S=F]");
//		System.out.println(", Cable fault = " + bad_cable + ", Congestion = " + congestion2 +
//				", Duplex = " + half_duplex + "\n");			      
	}
	
	private String describeDetLink(int num) {		
		switch (num) {
		case -2: return "Insufficent Data";
		case -1: return "System Fault";
		case 0:  return "RTT";
		case 1:  return "Dial-up";
		case 2:  return "T1";
		case 3:  return "Ethernet";
		case 4:  return "T3";
		case 5:  return "FastEthernet";
		case 6:  return "OC-12";
		case 7:  return "Gigabit Ethernet";
		case 8:  return "OC-48";
		case 9:  return "10 Gigabit Enet";
		case 10: return "Retransmissions";
		}
		return "Unknown";
	}
	
	public String toString() {
		return date + " " + ip_addr + " " + port;
	}

	public Component getInfoPane() {
		JPanel panel = new JPanel();
		panel.setLayout(new BoxLayout(panel, BoxLayout.Y_AXIS));
		panel.add(new JLabel("Client --> Server data detects link = " + describeDetLink(c2sdata)));
		panel.add(new JLabel("Client <-- Server Ack's detect link = " + describeDetLink(c2sack)));
		panel.add(new JLabel("Server --> Client data detects link = " + describeDetLink(s2cdata)));
		panel.add(new JLabel("Server <-- Client Ack's detect link = " + describeDetLink(s2cack)));
		panel.add(new JLabel(" "));
		panel.add(new JLabel("Acks = " + Helpers.formatDouble(acks, 4) + ", async speed = " + Helpers.formatDouble(aspeed, 4) +
				", mismatch3 = " + Helpers.formatDouble(cong, 4) + ", CongOver = " + congestionOverCount));
		panel.add(new JLabel("idle = " + Helpers.formatDouble(idle, 4) + ", timeout/pkts = " + Helpers.formatDouble(touts, 2) +
				", %retranmissions = " + Helpers.formatDouble(retrn*100, 2) + ", %increase = " + Helpers.formatDouble(increase*100, 2)));
		panel.add(new JLabel("FastRetrans/Total = " + Helpers.formatDouble(retrn, 4) + ", Fast/Retrans = " +
				Helpers.formatDouble(fr_ratio, 4) + ", Retrans/sec = " + Helpers.formatDouble(retransec, 4)));
		panel.add(new JLabel(" "));
		panel.add(new JLabel("Throughput to host [" + ip_addr + "] is limited by " + btlneck));
		panel.add(new JLabel(" "));
		panel.add(new JLabel("\tWeb100 says link = " + link + ", speed-chk says link = " + c2sdata));
		panel.add(new JLabel("\tSpeed-chk says {" + c2sdata + ", " + c2sack + ", " + s2cdata + ", " + s2cack +
				"}, Running average = {" + Helpers.formatDouble(runave[0], 1) + ", " + Helpers.formatDouble(runave[1], 1) +
				", " + Helpers.formatDouble(runave[2], 1) + ", " + Helpers.formatDouble(runave[3], 1) + "}"));
		if ((c2sspd > 1000) && (bw > 1)) {
			panel.add(new JLabel("\tC2Sspeed = " + Helpers.formatDouble(c2sspd/1000.0, 2) + " Mbps, S2Cspeed = " +
					Helpers.formatDouble(s2cspd/1000.0, 2) + " Mbps"));
			panel.add(new JLabel("CWND-Limited = " + Helpers.formatDouble(s2c2spd/1000.0, 2) + " Mbps, " +
					"Estimate = " + Helpers.formatDouble(bw, 2) + " Mbps (" + Helpers.formatDouble(bw2, 2) + " Mbps)"));
		}
		else if (c2sspd > 1000) {
			panel.add(new JLabel("\tC2Sspeed = " + Helpers.formatDouble(c2sspd/1000.0, 2) + " Mbps, S2Cspeed = " +
					Helpers.formatDouble(s2cspd/1000.0, 2) + " Mbps"));
			panel.add(new JLabel("CWND-Limited = " + Helpers.formatDouble(s2c2spd/1000.0, 2) + " Mbps, " +
					"Estimate = " + Helpers.formatDouble(bw*1000, 2) + " kbps (" +
					Helpers.formatDouble(bw2*1000, 2) + " kbps)"));
		}
		else if (bw > 1) {
			panel.add(new JLabel("\tC2Sspeed = " + c2sspd + " kbps, S2Cspeed = " + s2cspd + " kbps"));
			panel.add(new JLabel("CWND-Limited: " + s2c2spd + " kbps, " +
					"Estimate = " + Helpers.formatDouble(bw, 2) + " Mbps (" + Helpers.formatDouble(bw2, 2) + " Mbps)"));
		}
		else {
			panel.add(new JLabel("\tC2Sspeed = " + c2sspd + " kbps, S2Cspeed = " + s2cspd + " kbps"));
			panel.add(new JLabel("CWND-Limited: " + s2c2spd + " kbps, " +
					"Estimate = " + Helpers.formatDouble(bw*1000, 2) + " kbps (" +
					Helpers.formatDouble(bw2*1000, 2) + " kbps)"));
		}
		String estimates;
		if ((bw*1000) > s2cspd)
			estimates ="\tOld estimate is greater than measured; ";
		else
			estimates = "\tOld estimate is less than measured; ";

		if (congestionSignals == -1)
			estimates += "No data collected to calculage new estimate";
		else {
			if ((bw2*1000) > s2cspd)
				estimates += "New estimate is greater than measured";
			else
				estimates += "New estimate is less than measured";
		}
		panel.add(new JLabel(estimates));
		panel.add(new JLabel("\tLoss = " + Helpers.formatDouble(loss*100, 2) + "% (" + Helpers.formatDouble(loss2*100, 2) +
				"%), Out-of-Order = " + Helpers.formatDouble(order*100, 2) +
				"%, Long tail = {" + tail[0] + ", " + tail[1] + ", " + tail[2] + ", " + tail[3] + "}"));
		panel.add(new JLabel("\tDistribution = {" + head[0] + ", " + head[1] + ", " + head[2] + ", " + head[3] +
				"}, time spent {r=" + Helpers.formatDouble(rwintime*100, 1) + "% c=" + Helpers.formatDouble(cwndtime*100, 1) +
				"% s=" + Helpers.formatDouble(sendtime*100, 1) + "%}"));
		panel.add(new JLabel("\tAve(min) RTT = " + Helpers.formatDouble(avgrtt, 2) + " (" + minRTT +
				") msec, Buffers = {r=" + maxRwinRcvd + ", c=" + currentCwnd + ", s=" + sndBuf/2 + "}"));
		panel.add(new JLabel("\tbw*delay = {r=" + Helpers.formatDouble(recvbwd, 2) + ", c=" +
				Helpers.formatDouble(cwndbwd, 2) + ", s=" + Helpers.formatDouble(sendbwd, 2) +
				"}, Transitions/sec = {r=" + Helpers.formatDouble(sndLimTransRwin/timesec, 1) + ", c=" +
				Helpers.formatDouble(sndLimTransCwnd/timesec, 1) + ", s=" +
				Helpers.formatDouble(sndLimTransSender/timesec, 1) + "}"));
		panel.add(new JLabel("\tRetransmissions/sec = " + Helpers.formatDouble(pktsRetrans/timesec, 1) + ", Timeouts/sec = " + Helpers.formatDouble(timeouts/timesec, 1) + ", SSThreshold = " + maxSsthresh));
		String mismatchText = "\tMismatch = " + mismatch + " (" + mismatch2 + ":" + mismatch3 + "[" + Helpers.formatDouble(mmorder, 2) + "])";
		if (mismatch3 == 1)
			mismatchText += " [H=F, S=H]";
		if (mismatch2 == 1)
			mismatchText += " [H=H, S=F]";
		mismatchText += ", Cable fault = " + bad_cable + ", Congestion = " + congestion2 +
				", Duplex = " + half_duplex + "\n";
		panel.add(new JLabel(mismatchText));
		return new JScrollPane(panel);
	}
}