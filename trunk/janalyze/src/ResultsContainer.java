import java.awt.Component;
import java.awt.Color;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import java.util.Collection;
import java.util.StringTokenizer;
import java.util.Vector;
import java.util.SortedSet;
import java.util.TreeSet;

import java.awt.BorderLayout;
import javax.swing.JFrame;
import javax.swing.JScrollPane;
import javax.swing.JButton;
import javax.swing.JTextArea;
import javax.swing.text.StyleConstants;
import javax.swing.text.SimpleAttributeSet;

import java.sql.ResultSet;
import java.sql.SQLException;

public class ResultsContainer {
    private JAnalyze mainWindow;
    private double[] run_ave = new double[4];
    private double[] runave = new double[4];
    private int[][] links = new int[4][16];
    private int n = 0, m = 0, port;
    private String date, ip_addr, btlneck, cputraceFilename, snaplogFilename,
            c2sSnaplogFilename, ndttraceFilename;
    private int s2c2spd, s2cspd, c2sspd;
    private int timeouts, sumRTT, countRTT, pktsRetrans, fastRetrans,
            dataPktsOut, ackPktsOut, currentMSS, dupAcksIn, ackPktsIn,
            maxRwinRcvd, sndBuf, currentCwnd, sndLimTimeRwin, sndLimTimeCwnd,
            sndLimTimeSender, dataBytesOut, sndLimTransRwin, sndLimTransCwnd,
            sndLimTransSender, maxSsthresh, currentRTO, currentRwinRcvd, link,
            mismatch, bad_cable, half_duplex, c2sdata, c2sack,
            s2cdata, s2cack, pktsOut;
    private int congestionSignals = -1, minRTT = -1, congAvoid = -1,
            congestionOverCount = 0, maxRTT = 0, slowStart = 0, totaltime;
    private int linkcnt, mismatch2, mismatch3;	
    private double idle, loss, loss2, order, bw, bw2;
    private double rwintime, cwndtime, sendtime, timesec;
    private double recvbwd, cwndbwd, sendbwd;		  
    private int congestion2=0;		  
    private double acks, aspeed, avgrtt;
    private double cong, retrn, increase, touts, fr_ratio = 0;
    private double retransec, mmorder;
    private int[] tail = new int[4];
    private int[] head = new int[4];

        /* --- experimental congestion detection code --- */
    double linkType, sspd, mspd;
    double minrtt, ispd, n1, T1, lspd, n2, T2, lT2, uT2, teeth, lTeeth, uTeeth;
    boolean limited, normalOperation;
    private int minPeak = -1, maxPeak = -1, peaks = -1, rawPeaks = -1, realTeeth, ssCurCwnd = -1,
            ssSampleRTT = -1, fSampleRTT = -1, initPeakSpeed = -1;
    private Collection<PeakInfo> peakInfos = new Vector<PeakInfo>();
    private MaxPeakInfo maxPeakSpeed = null;
    private MinPeakInfo minPeakSpeed = null;
    private int max25 = -1, max50 = -1, max75 = -1, maxJitter = -1;
    private int min25 = -1, min50 = -1, min75 = -1, minJitter = -1;
        /* ---------------------------------------------- */

    ResultsContainer(JAnalyze mainWindow) {
        this.mainWindow = mainWindow;
    }

    public void parseDBRow(ResultSet rs) throws SQLException {
        subParseSpds(rs.getString("spds1"));
        subParseSpds(rs.getString("spds2"));
        subParseSpds(rs.getString("spds3"));
        subParseSpds(rs.getString("spds4"));
        run_ave[0] = rs.getFloat("runave1");
        run_ave[1] = rs.getFloat("runave2");
        run_ave[2] = rs.getFloat("runave3");
        run_ave[3] = rs.getFloat("runave4");
        cputraceFilename = rs.getString("cputimelog");
        if (cputraceFilename.length() == 0)
            cputraceFilename = null;
        snaplogFilename = rs.getString("snaplog");
        if (snaplogFilename.length() == 0)
            snaplogFilename = null;
        c2sSnaplogFilename = rs.getString("c2s_snaplog");
        if (c2sSnaplogFilename.length() == 0)
            c2sSnaplogFilename = null;
        ip_addr = rs.getString("hostName");
        port = rs.getInt("testPort");
        date = rs.getString("date");
        s2c2spd = rs.getInt("s2c2spd");
        s2cspd = rs.getInt("s2cspd");
        c2sspd = rs.getInt("c2sspd");
        timeouts = rs.getInt("Timeouts");
        sumRTT = rs.getInt("SumRTT");
        countRTT = rs.getInt("CountRTT");
        pktsRetrans = rs.getInt("PktsRetrans");
        fastRetrans = rs.getInt("FastRetran");
        dataPktsOut = rs.getInt("DataPktsOut");
        ackPktsOut = rs.getInt("AckPktsOut");
        currentMSS = rs.getInt("CurrentMSS");
        dupAcksIn = rs.getInt("DupAcksIn");
        ackPktsIn = rs.getInt("AckPktsIn");
        maxRwinRcvd = rs.getInt("MaxRwinRcvd");
        sndBuf = rs.getInt("Sndbuf");
        currentCwnd = rs.getInt("MaxCwnd");
        sndLimTimeRwin = rs.getInt("SndLimTimeRwin");
        sndLimTimeCwnd = rs.getInt("SndLimTimeCwnd");
        sndLimTimeSender = rs.getInt("SndLimTimeSender");
        dataBytesOut = rs.getInt("DataBytesOut");
        sndLimTransRwin = rs.getInt("SndLimTransRwin");
        sndLimTransCwnd = rs.getInt("SndLimTransCwnd");
        sndLimTransSender = rs.getInt("SndLimTransSender");
        maxSsthresh = rs.getInt("MaxSsthresh");
        currentRTO = rs.getInt("CurrentRTO");
        currentRwinRcvd = rs.getInt("CurrentRwinRcvd");
        link = rs.getInt("link");
        mismatch = rs.getInt("mismatch");
        bad_cable = rs.getInt("bad_cable");
        half_duplex = rs.getInt("half_duplex");
        c2sdata = rs.getInt("c2sdata");
        c2sack = rs.getInt("c2sack");
        s2cdata = rs.getInt("s2cdata");
        s2cack = rs.getInt("s2cack");
        congestionSignals = rs.getInt("CongestionSignals");
        pktsOut = rs.getInt("PktsOut");
        minRTT = rs.getInt("MinRTT");
        congAvoid = rs.getInt("CongAvoid");
        congestionOverCount = rs.getInt("CongestionOverCount");
        maxRTT = rs.getInt("MaxRTT");
        slowStart = rs.getInt("SlowStart");
        minPeak = rs.getInt("minPeak");
        maxPeak = rs.getInt("maxPeak");
        peaks = rs.getInt("peaks");
        /*ip_addr2 = rs.getString("rmt_host");*/
        /*congestion = rs.getInt("congestion");*/
        /*rcvWinScale = rs.getInt("RcvWinScale");*/
        /*autotune = rs.getInt("autotune");*/
        /*otherReductions = rs.getInt("OtherReductions");*/
        /*curTimeouts = rs.getInt("CurTimeoutCount");*/
        /*abruptTimeouts = rs.getInt("AbruptTimeouts");*/
        /*sendStall = rs.getInt("SendStall");*/
        /*subsequentTimeouts = rs.getInt("SubsequentTimeouts");*/
        /*thruBytesAcked = rs.getInt("ThruBytesAcked");*/
    }

    private void subParseSpds(String line) {
        StringTokenizer st = new StringTokenizer(line, " ");
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

    public void parseSpds(String line) {
        String spdsLine = line.substring(line.indexOf("'") + 1, line.lastIndexOf("'"));		
        subParseSpds(spdsLine.trim());
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

    public void parseSnaplogFilename(String line) {
        snaplogFilename = line.substring(14);
    }

    public void parseC2sSnaplogFilename(String line) {
        c2sSnaplogFilename = line.substring(18);
    }

    public void parseCputimeFilename(String line) {
        cputraceFilename = line.substring(20);
    }

    public boolean parseWeb100Var(String line) {
        StringTokenizer st = new StringTokenizer(line.trim(), ",");
        st.nextToken();
        /* ip_addr2 */ st.nextToken();
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
            /* congestion */ Integer.parseInt(st.nextToken());
            c2sdata = Integer.parseInt(st.nextToken());
            c2sack = Integer.parseInt(st.nextToken());
            s2cdata = Integer.parseInt(st.nextToken());
            s2cack = Integer.parseInt(st.nextToken());
            congestionSignals = Integer.parseInt(st.nextToken());
            pktsOut = Integer.parseInt(st.nextToken());
            minRTT = Integer.parseInt(st.nextToken());
            /* rcvWinScale */ Integer.parseInt(st.nextToken());
            /* autotune */ Integer.parseInt(st.nextToken());
            congAvoid = Integer.parseInt(st.nextToken());
            congestionOverCount = Integer.parseInt(st.nextToken());
            maxRTT = Integer.parseInt(st.nextToken());
            /* otherReductions */ Integer.parseInt(st.nextToken());
            /* curTimeouts */ Integer.parseInt(st.nextToken());
            /* abruptTimeouts */ Integer.parseInt(st.nextToken());
            /* sendStall */ Integer.parseInt(st.nextToken());
            slowStart = Integer.parseInt(st.nextToken());
            /* subsequentTimeouts */ Integer.parseInt(st.nextToken());
            /* thruBytesAcked */ Integer.parseInt(st.nextToken());
            minPeak = Integer.parseInt(st.nextToken());
            maxPeak = Integer.parseInt(st.nextToken());
            peaks = Integer.parseInt(st.nextToken());
        }
        catch (Exception exc) {
            // do nothing			
        }
        return true;
    }

    public void calculate() {		
        int i, j;
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
        if (s2cspd < c2sspd)
            aspeed = (double)c2sspd / (double)s2cspd; 
        else
            aspeed = (double)s2cspd / (double)c2sspd;

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

        /* --- experimental congestion detection code --- */

        if (ssCurCwnd == -1) {
            String snaplogData = mainWindow.getSnaplogData(snaplogFilename, "CurCwnd,SampleRTT", 0, true);
            StringTokenizer st = new StringTokenizer(snaplogData);
            boolean slowStart = true;
            boolean decreasing = false;
            int prevCWNDval = -1;
            int prevSampleRTT = -1;
            int CurCwnd = -1;
            int CurSampleRTT = - 1;
            try {
                while (st.hasMoreTokens()) {
                    st.nextToken(); st.nextToken();
                    CurCwnd = Integer.parseInt(st.nextToken());
                    CurSampleRTT = Integer.parseInt(st.nextToken());
                    if (slowStart) {
                        if (CurCwnd < prevCWNDval) {
                            slowStart = false;
                            maxPeak = prevCWNDval;
                            minPeak = -1;
                            peaks = 1;
                            rawPeaks = 1;
                            decreasing = true;
                            peakInfos.add(new MaxPeakInfo(ssCurCwnd, ssSampleRTT == 0 ? 1 : ssSampleRTT));
                        }
                        else {
                            ssCurCwnd = CurCwnd;
                            ssSampleRTT = CurSampleRTT;
                            if (ssSampleRTT > 0 && fSampleRTT == -1) {
                                fSampleRTT = ssSampleRTT;
                            }
                        }
                    }
                    else {
                        if (CurCwnd < prevCWNDval) {
                            if (prevCWNDval > maxPeak) {
                                maxPeak = prevCWNDval;
                            }
                            rawPeaks += 1;
                            if (!decreasing) {
                                // the max peak
                                peakInfos.add(new MaxPeakInfo(prevCWNDval, prevSampleRTT == 0 ? 1 : prevSampleRTT));
                                peaks += 1;
                            }
                            decreasing = true;
                        }
                        else if (CurCwnd > prevCWNDval) {
                            if ((minPeak == -1) || (prevCWNDval < minPeak)) {
                                minPeak = prevCWNDval;
                            }
                            if (decreasing) {
                                // the min peak
                                peakInfos.add(new MinPeakInfo(prevCWNDval, prevSampleRTT == 0 ? 1 : prevSampleRTT));
                            }
                            decreasing = false;
                        }
                    }
                    prevCWNDval = CurCwnd;
                    prevSampleRTT = CurSampleRTT;
                }
            }
            catch (Exception e) {
                // do nothing
            }
        }

        // 1) extract the bottleneck link type and the average & minimum RTT
        // values.  (If min == 0 then set min = 1msec)

        linkType = decodeLinkSpeed(c2sdata) < 0 ? 1 : decodeLinkSpeed(c2sdata);
        minrtt = minRTT <= 0 ? 1 : minRTT;

        // 2) find the time required to overshoot link type

        sspd = linkType;
        ispd = (currentMSS * 8 / 1024.0) / avgrtt;
        n1 = Math.log(sspd/ispd) / Math.log(2);
        T1 = n1 * avgrtt;

        // 3) find the number of teeth (assume single loss on overshoot)

        mspd = linkType;
        lspd = mspd / 2.0;
        // ispd = currentMSS * 8 / avgrtt;
        n2 = lspd / ispd;
        T2 = n2 * avgrtt;
        lT2 = n2 * maxRTT;
        uT2 = n2 * fSampleRTT;
        teeth = (((totaltime / 1000) - T1) / T2) + 1;
        lTeeth = (((totaltime / 1000) - T1) / lT2) + 1;
        uTeeth = (((totaltime / 1000) - T1) / uT2) + 1;

        // 4) find out if the connection is buffer limited

        limited = false;
        if (currentRwinRcvd * 8 / avgrtt < linkType)
            limited = true;

        // 5)

        if (peaks != -1) {
            realTeeth = peaks;
        }
        else {
            realTeeth = congestionSignals;
        }
        if (Math.abs((realTeeth/teeth) - 1.0) < (0.05 + (1.0/teeth)) && !limited) {
            normalOperation = true;
        }
        else {
            normalOperation = false;
        }

        if (ssCurCwnd != -1) {
            initPeakSpeed = encodeLinkSpeed(PeakInfo.getPeakSpeedInMbps(ssCurCwnd, ssSampleRTT));
        }

        /* Statistical analysis */

        SortedSet<Integer> curMaxCwnds = new TreeSet<Integer>();
        SortedSet<Integer> curMinCwnds = new TreeSet<Integer>();

        for (PeakInfo peak : peakInfos) {
            if (peak instanceof MaxPeakInfo) {
                curMaxCwnds.add(peak.getCwnd());
                if (maxPeakSpeed == null ||
                        PeakInfo.getPeakSpeedInMbps(peak.getCwnd(), peak.getSampleRTT()) >
                        PeakInfo.getPeakSpeedInMbps(maxPeakSpeed.getCwnd(), maxPeakSpeed.getSampleRTT())) {
                    maxPeakSpeed = (MaxPeakInfo) peak;
                        }
            }
            else {
                curMinCwnds.add(peak.getCwnd());
                if (minPeakSpeed == null ||
                        PeakInfo.getPeakSpeedInMbps(peak.getCwnd(), peak.getSampleRTT()) <
                        PeakInfo.getPeakSpeedInMbps(minPeakSpeed.getCwnd(), minPeakSpeed.getSampleRTT())) {
                    minPeakSpeed = (MinPeakInfo) peak;
                        }
            }
        }

        // fetch percentiles
        if (curMaxCwnds.size() > 0) {
            max25 = curMaxCwnds.toArray(new Integer[]{})[curMaxCwnds.size() * 1 / 4];
            max50 = curMaxCwnds.toArray(new Integer[]{})[curMaxCwnds.size() * 2 / 4];
            max75 = curMaxCwnds.toArray(new Integer[]{})[curMaxCwnds.size() * 3 / 4];
            maxJitter = max75 - max25;
        }
        if (curMinCwnds.size() > 0) {
            min25 = curMinCwnds.toArray(new Integer[]{})[curMinCwnds.size() * 1 / 4];
            min50 = curMinCwnds.toArray(new Integer[]{})[curMinCwnds.size() * 2 / 4];
            min75 = curMinCwnds.toArray(new Integer[]{})[curMinCwnds.size() * 3 / 4];
            minJitter = min75 - min25;
        }
        

        /* ---------------------------------------------- */

        if (snaplogFilename != null) {
            ndttraceFilename = "ndttrace." + snaplogFilename.substring(snaplogFilename.indexOf("-")+1);
            if (!mainWindow.checkTcpDump(ndttraceFilename)) {
                ndttraceFilename = null;
            }
        }
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

    private double decodeLinkSpeed(int num) {
        switch (num) {
            case 1: return .064;
            case 0:
            case 2: return 3;
            case 3: return 10;
            case 4: return 45;
            case 5: return 100;
            case 6: return 622;
            case 7: return 1000;
            case 8: return 2400;
            case 9: return 10000;
        }
        return -1;
    }

    private int encodeLinkSpeed(double linkSpeed) {
        if (linkSpeed < .128) {
            return 1;
        }
        if (linkSpeed >= 3 && linkSpeed <= 6) {
            return 2;
        }
        if (linkSpeed >= 10 && linkSpeed <= 20) {
            return 3;
        }
        if (linkSpeed >= 45 && linkSpeed <= 90) {
            return 4;
        }
        if (linkSpeed >= 100 && linkSpeed <= 200) {
            return 5;
        }
        if (linkSpeed >= 622 && linkSpeed < 1000) {
            return 6;
        }
        if (linkSpeed >= 1000 && linkSpeed <= 2000) {
            return 7;
        }
        if (linkSpeed >= 2400 && linkSpeed <= 4800) {
            return 8;
        }
        if (linkSpeed >= 10000 && linkSpeed <= 20000) {
            return 9;
        }
        return -1;
    }

    public String toString() {
        return date + " " + ip_addr + " " + port;
    }

    public String getIP() {
        return ip_addr;
    }

    public int getCongestion() {
        return congestion2;
    }

    public int getMismatch() {
        return mismatch;
    }

    public int getDuplex() {
        return half_duplex;
    }

    public int getCable() {
        return bad_cable;
    }

    public int getNewCongestion() {
        return normalOperation ? 0 : 1;
    }

    public int getInitialPeakSpeedEquality() {
        if (initPeakSpeed == -1) {
            return 0;
        }
        if (initPeakSpeed == linkType) {
            return 1;
        }
        else if (initPeakSpeed > linkType) {
            return 2;
        }
        else {
            return 3;
        }
    }

    public Component getInfoPane() {
        final SimpleTextPane panel = new SimpleTextPane();
        panel.append("Client --> Server data detects link = " + describeDetLink(c2sdata) + "\n");
        panel.append("Client <-- Server Ack's detect link = " + describeDetLink(c2sack) + "\n");
        panel.append("Server --> Client data detects link = " + describeDetLink(s2cdata) + "\n");
        panel.append("Server <-- Client Ack's detect link = " + describeDetLink(s2cack) + "\n\n");
        panel.append("Acks = " + Helpers.formatDouble(acks, 4) + ", async speed = " +
                Helpers.formatDouble(aspeed, 4) + ", mismatch3 = " + Helpers.formatDouble(cong, 4) +
                ", CongOver = " + congestionOverCount + "\n");
        panel.append(("idle = " + Helpers.formatDouble(idle, 4) + ", timeout/pkts = " +
                    Helpers.formatDouble(touts, 2) + ", %retranmissions = " +
                    Helpers.formatDouble(retrn*100, 2) + ", %increase = " +
                    Helpers.formatDouble(increase*100, 2)) + "\n");
        panel.append("FastRetrans/Total = " + Helpers.formatDouble(retrn, 4) + ", Fast/Retrans = " +
                    Helpers.formatDouble(fr_ratio, 4) + ", Retrans/sec = " +
                    Helpers.formatDouble(retransec, 4) + "\n\n");
        panel.append("Throughput to host [" + ip_addr + "] is limited by " + btlneck + "\n\n");
        panel.append("\tWeb100 says link = " + link + ", speed-chk says link = " + c2sdata + "\n");
        panel.append("\tSpeed-chk says {" + c2sdata + ", " + c2sack + ", " + s2cdata + ", " + s2cack +
                    "}, Running average = {" + Helpers.formatDouble(runave[0], 1) + ", " +
                    Helpers.formatDouble(runave[1], 1) + ", " + Helpers.formatDouble(runave[2], 1) + ", " +
                    Helpers.formatDouble(runave[3], 1) + "}\n");
        if ((c2sspd > 1000) && (bw > 1)) {
            panel.append("\tC2Sspeed = " + Helpers.formatDouble(c2sspd/1000.0, 2) + " Mbps, S2Cspeed = " +
                        Helpers.formatDouble(s2cspd/1000.0, 2) + " Mbps\n");
            panel.append("CWND-Limited = " + Helpers.formatDouble(s2c2spd/1000.0, 2) + " Mbps, " +
                        "Estimate = " + Helpers.formatDouble(bw, 2) + " Mbps (" +
                        Helpers.formatDouble(bw2, 2) + " Mbps)\n");
        }
        else if (c2sspd > 1000) {
            panel.append("\tC2Sspeed = " + Helpers.formatDouble(c2sspd/1000.0, 2) + " Mbps, S2Cspeed = " +
                        Helpers.formatDouble(s2cspd/1000.0, 2) + " Mbps\n");
            panel.append("CWND-Limited = " + Helpers.formatDouble(s2c2spd/1000.0, 2) + " Mbps, " +
                        "Estimate = " + Helpers.formatDouble(bw*1000, 2) + " kbps (" +
                        Helpers.formatDouble(bw2*1000, 2) + " kbps)\n");
        }
        else if (bw > 1) {
            panel.append("\tC2Sspeed = " + c2sspd + " kbps, S2Cspeed = " + s2cspd + " kbps\n");
            panel.append("CWND-Limited: " + s2c2spd + " kbps, Estimate = " +
                    Helpers.formatDouble(bw, 2) + " Mbps (" + Helpers.formatDouble(bw2, 2) + " Mbps)\n");
        }
        else {
            panel.append("\tC2Sspeed = " + c2sspd + " kbps, S2Cspeed = " + s2cspd + " kbps\n");
            panel.append("CWND-Limited: " + s2c2spd + " kbps, " +
                        "Estimate = " + Helpers.formatDouble(bw*1000, 2) + " kbps (" +
                        Helpers.formatDouble(bw2*1000, 2) + " kbps)\n");
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
        panel.append(estimates + "\n");
        panel.append("\tLoss = " + Helpers.formatDouble(loss*100, 2) + "% (" +
                Helpers.formatDouble(loss2*100, 2) + "%), Out-of-Order = " +
                Helpers.formatDouble(order*100, 2) + "%, Long tail = {" + tail[0] +
                ", " + tail[1] + ", " + tail[2] + ", " + tail[3] + "}\n");
        panel.append("\tDistribution = {" + head[0] + ", " + head[1] + ", " + head[2] + ", " + head[3] +
                "}, time spent {r=" + Helpers.formatDouble(rwintime*100, 1) + "% c=" +
                Helpers.formatDouble(cwndtime*100, 1) + "% s=" + Helpers.formatDouble(sendtime*100, 1)+"%}\n");
        panel.append("\tAve(min) RTT = " + Helpers.formatDouble(avgrtt, 2) + " (" + minRTT +
                    ") msec, Buffers = {r=" + maxRwinRcvd + ", c=" + currentCwnd + ", s=" + sndBuf/2 + "}\n");
        panel.append("\tbw*delay = {r=" + Helpers.formatDouble(recvbwd, 2) + ", c=" +
                    Helpers.formatDouble(cwndbwd, 2) + ", s=" + Helpers.formatDouble(sendbwd, 2) +
                    "}, Transitions/sec = {r=" + Helpers.formatDouble(sndLimTransRwin/timesec, 1) + ", c=" +
                    Helpers.formatDouble(sndLimTransCwnd/timesec, 1) + ", s=" +
                    Helpers.formatDouble(sndLimTransSender/timesec, 1) + "}\n");
        panel.append("\tRetransmissions/sec = " + Helpers.formatDouble(pktsRetrans/timesec, 1) +
                ", Timeouts/sec = " + Helpers.formatDouble(timeouts/timesec, 1) + ", SSThreshold = " +
                maxSsthresh + "\n");
        String mismatchText = "\tMismatch = " + mismatch + " (" + mismatch2 + ":" + mismatch3 + "[" +
            Helpers.formatDouble(mmorder, 2) + "])";
        if (mismatch3 == 1)
            mismatchText += " [H=F, S=H]";
        if (mismatch2 == 1)
            mismatchText += " [H=H, S=F]";

        SimpleAttributeSet green = new SimpleAttributeSet();
        green.addAttribute(StyleConstants.ColorConstants.Foreground, Color.GREEN);

        SimpleAttributeSet red = new SimpleAttributeSet();
        red.addAttribute(StyleConstants.ColorConstants.Foreground, Color.RED);

        panel.appendColored(mismatchText + "\n", mismatch == 0 ? green : red);

        panel.appendColored("Cable fault = " + bad_cable + "\n", bad_cable == 0 ? green : red);

        panel.appendColored("Congestion = " + congestion2 + "\n", congestion2 == 0 ? green : red);

        panel.appendColored("Duplex = " + half_duplex + "\n\n", half_duplex == 0 ? green : red);

        panel.append("S2C Snaplog file: ");
        if (snaplogFilename != null) {
            if (ssCurCwnd == -1) {
                panel.appendColored(snaplogFilename + "\n", red);
            }
            else {
                panel.append(snaplogFilename + " ");
                JButton viewButton = new JButton("View");
                viewButton.addActionListener(new ActionListener() {
                    public void actionPerformed(ActionEvent e) {
                        String snaplogData = mainWindow.getSnaplogData(snaplogFilename, null, 0, false);
                        JTextArea area = new JTextArea(snaplogData);
                        area.setEditable(false);
                        JFrame frame = new JFrame("Snaplog variables");
                        frame.getContentPane().add(new JScrollPane(area), BorderLayout.CENTER);
                        frame.setSize(800, 600);
                        frame.setVisible(true);
                    }
                });
                panel.insertComponent(viewButton);
                JButton plotButton = new JButton("Plot");
                plotButton.addActionListener(new ActionListener() {
                    public void actionPerformed(ActionEvent e) {
                        mainWindow.plotSnaplog(snaplogFilename);
                    }
                });
                panel.insertComponent(plotButton);
                JButton cPlotButton = new JButton("Plot CWND");
                cPlotButton.addActionListener(new ActionListener() {
                    public void actionPerformed(ActionEvent e) {
                        mainWindow.plotSnaplogCWND(snaplogFilename);
                    }
                });
                panel.insertComponent(cPlotButton);
            }
        }
        else {
            panel.appendColored("N/A", red);
        }

        panel.append("\nC2S Snaplog file: ");
        if (c2sSnaplogFilename != null) {
            if (ssCurCwnd == -1) {
                panel.appendColored(c2sSnaplogFilename + "\n", red);
            }
            else {
                panel.append(c2sSnaplogFilename + " ");
                JButton viewButton = new JButton("View");
                viewButton.addActionListener(new ActionListener() {
                    public void actionPerformed(ActionEvent e) {
                        String snaplogData = mainWindow.getSnaplogData(c2sSnaplogFilename, null, 0, false);
                        JTextArea area = new JTextArea(snaplogData);
                        area.setEditable(false);
                        JFrame frame = new JFrame("Snaplog variables");
                        frame.getContentPane().add(new JScrollPane(area), BorderLayout.CENTER);
                        frame.setSize(800, 600);
                        frame.setVisible(true);
                    }
                });
                panel.insertComponent(viewButton);
                JButton plotButton = new JButton("Plot");
                plotButton.addActionListener(new ActionListener() {
                    public void actionPerformed(ActionEvent e) {
                        mainWindow.plotSnaplog(c2sSnaplogFilename);
                    }
                });
                panel.insertComponent(plotButton);
                JButton cPlotButton = new JButton("Plot CWND");
                cPlotButton.addActionListener(new ActionListener() {
                    public void actionPerformed(ActionEvent e) {
                        mainWindow.plotSnaplogCWND(c2sSnaplogFilename);
                    }
                });
                panel.insertComponent(cPlotButton);
            }
        }
        else {
            panel.appendColored("N/A", red);
        }

        panel.append("\nTcpdump trace file: ");
        if (ndttraceFilename != null) {
            panel.append(ndttraceFilename + " ");

            JButton viewButton = new JButton("View");
            viewButton.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    String tcpdumpData = mainWindow.getTcpdumpData(ndttraceFilename);
                    JTextArea area = new JTextArea(tcpdumpData);
                    area.setEditable(false);
                    JFrame frame = new JFrame("tcpdump trace");
                    frame.getContentPane().add(new JScrollPane(area), BorderLayout.CENTER);
                    frame.setSize(800, 600);
                    frame.setVisible(true);
                }
            });
            panel.insertComponent(viewButton);
            JButton plotButton = new JButton("Plot");
            plotButton.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    mainWindow.plotTcpdumpS(ndttraceFilename);
                }
            });
            panel.insertComponent(plotButton);
        }
        else {
            panel.appendColored("N/A", red);
        }

        panel.append("\nCputime trace file: ");
        if (cputraceFilename != null) {
            panel.append(cputraceFilename + " ");
            JButton viewButton = new JButton("View");
            viewButton.addActionListener(new ActionListener()
                    {
                        public void actionPerformed(ActionEvent e) {
                            mainWindow.plotCputime(cputraceFilename);
                        }
                    });
            panel.insertComponent(viewButton);
        }
        else {
            panel.appendColored("N/A", red);
        }

        /* --- experimental congestion detection code --- */

        panel.append("\n\n--- experimental congestion detection code ---\n");

        // 1) extract the bottleneck link type and the average & minimum RTT
        // values.  (If min == 0 then set min = 1msec)
        panel.append("1) Bottleneck link type = " + linkType +
                    ", minimum RTT = " + Helpers.formatDouble(minrtt, 2) +
                    ", average RTT = " + Helpers.formatDouble(avgrtt, 2) + "\n");

        // 2) find the time required to overshoot link type
        panel.append("2) sspd = " + sspd + ", ispd = " + Helpers.formatDouble(ispd, 2) +
                    ", n1 = " + Helpers.formatDouble(n1, 2) + ", T1 = " + Helpers.formatDouble(T1, 2) + "\n");

        // 3) find the number of teeth (assume single loss on overshoot)
        panel.append("3) mspd = " + mspd + ", lspd = " + Helpers.formatDouble(lspd, 2) +
                    ", ispd = " + Helpers.formatDouble(ispd, 2) +
                    ", n2 = " + Helpers.formatDouble(n2, 2) + ", T2 = " + Helpers.formatDouble(T2, 2) + "\n");
        panel.append("   teeth = " + Helpers.formatDouble(teeth, 2) +
                    ", CongestionSignals = " + congestionSignals + "\n");
        panel.append("   lower teeth = " + Helpers.formatDouble(lTeeth, 2) +
                    ", lT2 = " + Helpers.formatDouble(lT2, 2) +
                    ", maxRTT = " + maxRTT + "\n");
        if (fSampleRTT != -1) {
            panel.append("   upper teeth = " + Helpers.formatDouble(uTeeth, 2) +
                        ", uT2 = " + Helpers.formatDouble(uT2, 2) +
                        ", fSampleRTT = " + fSampleRTT + "\n");
        }
        if (peaks != -1) {
            panel.append("   minPeak = " + minPeak + ", maxPeak = " + maxPeak + ", peaks = " + peaks +
                        (rawPeaks == -1 ? "" : " [" + rawPeaks + "]") + "\n");
        }

        if (ssCurCwnd != -1) {
            panel.append("   " + PeakInfo.getPeakSpeed(ssCurCwnd, ssSampleRTT) + "\n");
            if (initPeakSpeed == -1) {
                panel.append("   initial peak speed is not available\n");
            }
            else {
                panel.append("   initial peak speed says link = " + initPeakSpeed + " ");
                if (initPeakSpeed == linkType) {
                    panel.append("EQUAL\n");
                }
                else if (initPeakSpeed > linkType) {
                    panel.append("GREATER\n");
                }
                else {
                    panel.append("LESS\n");
                }
            }
        }

        if (max25 > -1) {
            panel.append("   peaks: 25th: " + max25 + ", 50th: " + max50 + ", 75th: " + max75 +
                    "\n            jitter: " + maxJitter + ", max " +
                    PeakInfo.getPeakSpeed(maxPeakSpeed.getCwnd(), maxPeakSpeed.getSampleRTT()) + "\n");
        }

        if (min25 > -1) {
            panel.append("   valleys: 25th: " + min25 + ", 50th: " + min50 + ", 75th: " + min75 +
                    "\n            jitter: " + minJitter + ", min " +
                    PeakInfo.getPeakSpeed(minPeakSpeed.getCwnd(), minPeakSpeed.getSampleRTT()) + "\n");
        }

        if (peakInfos.size() > 0) {
            int counter = 0;
            panel.append("Peaks:\n");
            for (PeakInfo peakInfo: peakInfos) {
                panel.append(((counter/2)+1) + ". " + peakInfo.toString() + "\n");
                counter += 1;
            }
            panel.append("---\n");
        }


        // 4) find out if the connection is buffer limited
        panel.append("4) limited = " + limited + "\n");

        // 5
        if (normalOperation) {
            panel.appendColored("RESULT: normal operation", green);
        }
        else {
            panel.appendColored("RESULT: something went wrong", red);
        }

        /* ---------------------------------------------- */
        return new JScrollPane(panel);
    }
}
