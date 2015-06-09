/**
 * This file contains the functions needed to handle the Admin
 * page.  This page allows a remote user to view the usage statistics
 * via a web page.
 *
 * Richard Carlson  3/10/04
 * rcarlson@internet2.edu
 */

#include "web100-admin.h"

#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "heuristics.h"
#include "logging.h"
#include "strlutils.h"
#include "utils.h"
#include "web100srv.h"

/* Initialize the Administrator view.  Process the data in the existing log file to
 * catch up on what's happened before.
 */

int maxc2sspd = 0, minc2sspd = 0, maxs2cspd = 0, mins2cspd = 0;
int count[16], totmismatch, totbad_cable;

float recvbwd, cwndbwd, sendbwd;
char btlneck[64];
char date[32], startdate[32], mindate[32], maxdate[32];
double oo_order;
double bw_theortcl, avgrtt, timesec, loss2;
double bwin, bwout;
char *AdminFileName;

/** Calculate some values for link speeds and make ready for printing
 *
 * @param now  current time/data details
 * @param SumRTT    sum of all sampled round trip times
 * @param CountRTT  number of round trip time samples
 * @param CongestionSignals multiplicative downward congestion window adjustments, web100_var value
 * @param PktsOut  The total number of segments sent
 * @param DupAcksIn number of duplicate acks in
 * @param AckPktsIn number of ack packets in
 * @param CurrentMSS current maximum segment size (MSS), in octets
 * @param SndLimTimeRwin cumulative time spent in 'Receiver Limited' state, web100_var value
 * @param SndLimTimeCwnd cumulative time spent in
 * 			 'Congestion Limited' state, web100_var value
 * @param SndLimTimeSender total time spent in the 'sender limited' state, web100_var value
 * @param MaxRwinRcvd The maximum window advertisement received, web100_var value
 * @param CurrentCwnd current congestion window
 * @param Sndbuf socket send buffer size
 * @param DataBytesOut The number of octets of data contained in transmitted segments, web100_var value
 * @param mismatch mismatch indicator as calculated during tests
 * @param bad_cable bad hardware indicated as calculated during tests
 * @param c2sspd bandwidth as calculated during the C->S test
 * @param s2cspd bandwidth as calculated during the S->C test
 * @param c2s_linkspeed_data Integral indicator of C->S speed range
 * @param s2c_linkspeed_ack S->C Data link speed(type) as detected by server acknowledgments
 * @param view_flag
 * @return count of variable values read
 */
int calculate(char now[32], int SumRTT, int CountRTT, int CongestionSignals,
              int PktsOut, int DupAcksIn, int AckPktsIn, int CurrentMSS,
              int SndLimTimeRwin, int SndLimTimeCwnd, int SndLimTimeSender,
              int MaxRwinRcvd, int CurrentCwnd, int Sndbuf, int DataBytesOut,
              int mismatch, int bad_cable, int c2sspd, int s2cspd,
              int c2s_linkspeed_data, int s2c_linkspeed_ack, int view_flag) {
  int i;
  static int totalcnt;
  double rttsec;
  double rwintime = 0, cwndtime = 0;
  int totaltime = 0;
  char view_string[256], *str, tmpstr[32];
  FILE * fp;

  if (view_flag == 1) {
    fp = fopen("/tmp/view.string", "r");
    if (fgets(view_string, 256, fp) != NULL) {
      log_println(3, "View_string=%s", view_string);
      str = view_string;
      sscanf(str, "%[^,]s", tmpstr);
      maxc2sspd = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      minc2sspd = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      maxs2cspd = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      mins2cspd = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      totalcnt = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      totmismatch = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      totbad_cable = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      count[0] = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      count[1] = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      count[2] = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      count[3] = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      count[4] = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      count[5] = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      count[6] = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      count[7] = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      count[8] = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      count[9] = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      count[10] = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      count[11] = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      count[12] = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      count[13] = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      count[14] = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      count[15] = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      strlcpy(maxdate, tmpstr, sizeof(maxdate));

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      strlcpy(mindate, tmpstr, sizeof(mindate));

      for (i = 0; i < strlen(mindate); i++)
        if (mindate[i] == '\n')
          break;
      mindate[i] = '\0';

      log_println(1, "Updating admin_view variables: Total count = %d",
                  totalcnt);
    }
    fclose(fp);
  }

  // print details about bottleneck link's speed
  switch (c2s_linkspeed_data) {
    case DATA_RATE_INSUFFICIENT_DATA:
      snprintf(btlneck, sizeof(btlneck), "Insufficent Data");
      break;
    case DATA_RATE_SYSTEM_FAULT:
      snprintf(btlneck, sizeof(btlneck), "System Fault");
      break;
    case DATA_RATE_RTT:
      snprintf(btlneck, sizeof(btlneck), "Round Trip Time");
      break;
    case DATA_RATE_DIAL_UP:
      snprintf(btlneck, sizeof(btlneck), "Dial-up modem");
      break;
    case DATA_RATE_T1:
      if (((float) c2sspd / (float) s2cspd > .8) &&
          ((float) c2sspd / (float) s2cspd < 1.2) && (c2sspd > 1000)) {
        snprintf(btlneck, sizeof(btlneck), "T1 subnet");
      } else {
        if (s2c_linkspeed_ack == 3)
          snprintf(btlneck, sizeof(btlneck), "Cable Modem");
        else
          snprintf(btlneck, sizeof(btlneck), "DSL");
      }
      break;
    case DATA_RATE_ETHERNET:
      snprintf(btlneck, sizeof(btlneck), "Ethernet");
      break;
    case DATA_RATE_T3:
      snprintf(btlneck, sizeof(btlneck), "T3/DS-3");
      break;
    case DATA_RATE_FAST_ETHERNET:
      snprintf(btlneck, sizeof(btlneck), "FastEthernet");
      break;
    case DATA_RATE_OC_12:
      snprintf(btlneck, sizeof(btlneck), "OC-12");
      break;
    case DATA_RATE_GIGABIT_ETHERNET:
      snprintf(btlneck, sizeof(btlneck), "Gigabit Ethernet");
      break;
    case DATA_RATE_OC_48:
      snprintf(btlneck, sizeof(btlneck), "OC-48");
      break;
    case DATA_RATE_10G_ETHERNET:
      snprintf(btlneck, sizeof(btlneck), "10 Gigabit Enet");
      break;
    case DATA_RATE_RETRANSMISSIONS:
      snprintf(btlneck, sizeof(btlneck), "Retransmissions");
  }

  /* Calculate some values */
  strlcpy(date, now, sizeof(date));
  // Calculate average round trip time and convert to seconds
  rttsec = calc_avg_rtt(SumRTT, CountRTT, &avgrtt);
  loss2 = (double) CongestionSignals / PktsOut;
  if (loss2 == 0)
    loss2 = .000001; /* set to 10^-6 for now */

  oo_order = calc_packets_outoforder(DupAcksIn, AckPktsIn);

  bw_theortcl = calc_max_theoretical_throughput(CurrentMSS, rttsec, loss2);

  totaltime = calc_totaltesttime(SndLimTimeRwin, SndLimTimeCwnd,
                                 SndLimTimeSender);
  log_println(3, "totaltime=%d, sndLim %d", totaltime, SndLimTimeRwin);
  // time spent being send-limited due to client's recv window
  rwintime = calc_sendlimited_rcvrfault(SndLimTimeRwin, totaltime);
  log_println(3, "rwintime=%f", rwintime);

  log_println(3, "before calling cwndtime cal. %d,%d", SndLimTimeCwnd,
              totaltime);
  // time spent in being send-limited due to congestion window
  cwndtime = calc_sendlimited_cong(SndLimTimeCwnd, totaltime);
  log_println(3, "cwndtime=%f", cwndtime);
  // time spent in being send-limited due to own fault
  //sendtime = calc_sendlimited_sndrfault(SndLimTimeSender, totaltime);
  timesec = totaltime / MEGA;  // total time in microsecs

  // calculate receive buffer delay, send buffer delay and congestion window
  // delays
  recvbwd = ((MaxRwinRcvd * 8) / avgrtt) / 1000;
  cwndbwd = ((CurrentCwnd * 8) / avgrtt) / 1000;
  sendbwd = ((Sndbuf * 8) / avgrtt) / 1000;

  // this is different from congestion window heuristic, but seems unused
  /*if ((cwndtime > .02) && (mismatch == 0) && (cwndbwd < recvbwd))
    congestion2 = 1;*/

  // if no web100 variables were read from the file, then assign values
  if (totalcnt == 0) {
    minc2sspd = c2sspd;
    mins2cspd = s2cspd;
    maxc2sspd = c2sspd;
    maxs2cspd = s2cspd;
    strlcpy(startdate, date, sizeof(startdate));
    strlcpy(maxdate, date, sizeof(maxdate));
    strlcpy(mindate, date, sizeof(mindate));
  }

  // assign max/min values of speeds based on real read values
  if (c2sspd > maxc2sspd) {
    maxc2sspd = c2sspd;
    strlcpy(maxdate, date, sizeof(maxdate));
  }
  if (s2cspd > maxs2cspd) {
    maxs2cspd = s2cspd;
    strlcpy(maxdate, date, sizeof(maxdate));
  }
  if (c2sspd < minc2sspd) {
    minc2sspd = c2sspd;
    strlcpy(mindate, date, sizeof(mindate));
  }
  if (s2cspd < mins2cspd) {
    mins2cspd = s2cspd;
    strlcpy(mindate, date, sizeof(mindate));
  }

  log_println(
      1,
      "Initial counter Values Totalcnt = %d, Total Mismatch = %d, "
      "Total Bad Cables = %d", totalcnt, totmismatch, totbad_cable);
  totalcnt++;
  count[c2s_linkspeed_data + 1]++;
  if (mismatch > 0)
    totmismatch++;
  if (bad_cable == 1)
    totbad_cable++;
  log_println(
      1,
      "Updated counter values Totalcnt = %d, Total Mismatch = %d, "
      "Total Bad Cables = %d", totalcnt, totmismatch, totbad_cable);
  log_println(1, "Individual counts = "
      "[%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d]",
      count[0], count[1], count[2], count[3], count[4], count[5],
      count[6], count[7], count[8], count[9], count[10], count[11],
      count[12], count[13], count[14], count[15]);
  return (totalcnt);
}

/**
 *
 * Build HTML page with current tests' results and sumamry data
 * @param c2sspd C->S test speed
 * @param s2cspd S->C test speed
 * @param MinRTT minimum sampled round trip time, web_100 var value
 * @param PktsRetrans number of segments transmitted containing at least some
 *		retransmitted data, web_100 var value
 * @param Timeouts # of times the retransmit timeout has expired
 * @param Sndbuf socket send buffer size in octets, web_100 var value
 * @param MaxRwinRcvd , web_100 var value
 * @param CurrentCwnd , web_100 var value
 * @param mismatch mismatch indicator as calculated during tests
 * @param bad_cable bad hardware indicated as calculated during tests
 * @param totalcnt total count of variables to be processed
 * @param refresh time in seconds after which to refresh page
 */
void gen_html(int c2sspd, int s2cspd, int MinRTT, int PktsRetrans, int Timeouts,
              int Sndbuf, int MaxRwinRcvd, int CurrentCwnd, int mismatch,
              int bad_cable, int totalcnt, int refresh) {
  FILE * fp;
  char view_string[256], tmpstr[256];
  struct flock lock;

  fp = fopen(AdminFileName, "w");
  if (fp == NULL) {
    log_println(1, "Cannot open file for the admin web page...");
    return;
  }
  /* generate an HTML page for display.  */
  fprintf(
      fp,
      "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n");
  fprintf(fp, "<html> <head> <title>NDT server Admin Page</title>\n");
  fprintf(fp,
          "<meta http-equiv=\"refresh\" content=\"%d; url="ADMINFILE"\">\n",
          refresh);
  fprintf(fp, "<meta http-equiv=\"Content-Language\" content=\"en\">\n");
  fprintf(fp, "<meta http-equiv=\"Content-Type\" content=\"text/html; "
          "charset=iso-8859-1\">\n");
  fprintf(fp, "</head>\n");

  /* First build current results table */
  fprintf(fp, "<table border>\n  <tr>\n ");
  fprintf(fp, "    <th colspan=8><font size=\"+1\">NDT Usage Statistics -- "
          "Current Test Results\n");
  fprintf(fp, "    </font></tr>\n");
  fprintf(
      fp,
      "  <tr>\n    <th>Date/Time\n    <th>Total Tests\n    <th rowspan=6>\n");
  fprintf(fp, "    <th>Bottleneck Link\n    <th>Theoritical Limit\n    "
          "<th rowspan=6>\n");
  fprintf(fp, "    <th>C2S Speed\n    <th>S2C Speed\n  </tr>\n");
  fprintf(fp, "  <tr>\n    <td align=right>%s\n    <td align=right>%d\n    "
          "<td align=right>%s\n",
      date, totalcnt, btlneck);
  if (bw_theortcl > 1)
    fprintf(fp, "    <td align=right>%0.2f Mbps\n", bw_theortcl);
  else
    fprintf(fp, "    <td align=right>%0.2f kbps\n", bw_theortcl * 1000);
  if (c2sspd > 1000)
    fprintf(
        fp,
        "    <td align=right>%0.2f Mbps\n    <td align=right>%0.2f Mbps\n",
        (float) c2sspd / 1000, (float) s2cspd / 1000);
  else
    fprintf(fp,
            "    <td align=right>%d kbps\n    <td align=right>%d kbps, ",
            c2sspd, s2cspd);
  fprintf(fp, "  </tr>\n  <tr>\n    <th>Packet Loss\n    <th>Average RTT\n");
  fprintf(fp,
          "    <th>Mininum RTT\n    <th>Retrans/sec\n    <th>Timeouts/sec\n");
  fprintf(fp, "    <th>%% Out-of-Order\n  </tr>\n  <tr>\n");
  if (loss2 == .000001)
    fprintf(fp, "    <td align=right>None\n    <td align=right>%0.2f msec\n    "
            "<td align=right>%d msec\n", avgrtt, MinRTT);
  else
    fprintf(fp, "    <td align=right>%0.4f%%\n    <td align=right>%0.2f msec\n"
            "    <td align=right>%d msec\n",
        loss2 * 100, avgrtt, MinRTT);
  fprintf(fp, "    <td align=right>%0.2f\n    <td align=right>%0.2f\n    "
          "<td align=right>%0.2f\n  </tr>\n",
      (float) PktsRetrans / timesec, (float) Timeouts / timesec,
      oo_order * 100);
  fprintf(fp, "  <tr>\n    <th>Send Buffer\n    <th>BW*Delay\n");
  fprintf(fp, "    <th>Receive Buffer\n    <th>BW*Delay\n");
  fprintf(fp, "    <th>Congestion Window\n    <th>BW*Delay\n  </tr>\n");
  fprintf(
      fp,
      "  <tr>\n    <td align=right>%d Bytes\n    <td align=right>%0.2f Mbps\n",
      Sndbuf, sendbwd);
  fprintf(fp,
          "    <td align=right>%d Bytes\n    <td align=right>%0.2f Mbps\n",
          MaxRwinRcvd, recvbwd);
  fprintf(fp,
          "    <td align=right>%d Bytes\n    <td align=right>%0.2f Mbps\n",
          CurrentCwnd, cwndbwd);
  fprintf(fp, "  </tr>\n</table>\n\n");

  fprintf(fp, "<applet code=Admin.class\n  width=600 height=400>\n");
  fprintf(fp, "  <PARAM NAME=\"Fault\" VALUE=\"%d\">\n", count[0]);
  fprintf(fp, "  <PARAM NAME=\"RTT\" VALUE=\"%d\">\n", count[1]);
  fprintf(fp, "  <PARAM NAME=\"Dial-up\" VALUE=\"%d\">\n", count[2]);
  fprintf(fp, "  <PARAM NAME=\"T1\" VALUE=\"%d\">\n", count[3]);
  fprintf(fp, "  <PARAM NAME=\"Enet\" VALUE=\"%d\">\n", count[4]);
  fprintf(fp, "  <PARAM NAME=\"T3\" VALUE=\"%d\">\n", count[5]);
  fprintf(fp, "  <PARAM NAME=\"FastE\" VALUE=\"%d\">\n", count[6]);
  fprintf(fp, "  <PARAM NAME=\"OC-12\" VALUE=\"%d\">\n", count[7]);
  fprintf(fp, "  <PARAM NAME=\"GigE\" VALUE=\"%d\">\n", count[8]);
  fprintf(fp, "  <PARAM NAME=\"OC-48\" VALUE=\"%d\">\n", count[9]);
  fprintf(fp, "  <PARAM NAME=\"tenGE\" VALUE=\"%d\">\n", count[10]);
  fprintf(fp, "  <PARAM NAME=\"Total\" VALUE=\"%d\">\n", totalcnt);
  fprintf(fp, "</applet>\n<br>\n");

  /* Next generate summary table */
  fprintf(fp, "<table border>\n  <tr>\n");
  fprintf(fp, "    <th>\n    <th>Date/Time\n    <th rowspan=5>\n");
  fprintf(fp, "    <th colspan=2>Throughput Summary\n    <th rowspan=5>\n");
  fprintf(fp,
          "    <th colspan=2>Configuration Fault Summary\n  </tr>\n  <tr>\n");
  fprintf(fp, "    <td><b>Log Starts</b>\n    <td align=right>%s\n    "
          "<th>Client-to-Server\n",
      startdate);
  fprintf(fp, "    <th>Server-to-Client\n    <th>Duplex Mismatch\n");
  fprintf(fp, "    <th>Excessive Errors\n  </tr>\n");
  fprintf(fp, "  <tr>\n    <td><b>Current</b>\n    <td align=right>%s\n",
          date);
  if (c2sspd > 1000)
    fprintf(
        fp,
        "    <td align=right>%0.2f Mbps\n    <td align=right>%0.2f Mbps\n",
        (float) c2sspd / 1000, (float) s2cspd / 1000);
  else
    fprintf(fp,
            "    <td align=right>%d kbps\n    <td align=right>%d kbps, ",
            c2sspd, s2cspd);

  fprintf(fp,
          "    <td align=right>%s\n    <td align=right>%s\n  </tr>\n  <tr>\n",
          mismatch == 1 ? "Yes" : "No", bad_cable == 1 ? "Yes" : "No");
  fprintf(fp, "    <td><b>Maximum</b>\n    <td align=right>%s\n", maxdate);
  if (maxc2sspd > 1000)
    fprintf(
        fp,
        "    <td align=right>%0.2f Mbps\n    <td align=right>%0.2f Mbps\n",
        (float) maxc2sspd / 1000, (float) maxs2cspd / 1000);
  else
    fprintf(fp,
            "    <td align=right>%d kbps\n    <td align=right>%d kbps, ",
            maxc2sspd, maxs2cspd);
  fprintf(
      fp,
      "    <td align=right>%d found\n    <td align=right>%d found\n  </tr>\n",
      totmismatch, totbad_cable);
  fprintf(fp, "  <tr>\n    <td><b>Minimum</b>\n    <td align=right>%s\n",
          mindate);
  if (minc2sspd > 1000)
    fprintf(
        fp,
        "    <td align=right>%0.2f Mbps\n    <td align=right>%0.2f Mbps\n",
        (float) minc2sspd / 1000, (float) mins2cspd / 1000);
  else
    fprintf(fp,
            "    <td align=right>%d kbps\n    <td align=right>%d kbps\n",
            minc2sspd, mins2cspd);
  fprintf(fp, "    <td>\n    <td>\n  </tr>\n</table>\n");

  fprintf(fp, "<br>\n<hr width=\"100%%\" noShade size=4>\n");
  fprintf(fp, "<table border>\n  <tr>\n");
  fprintf(fp, "    <th>\n    <th>Filename\n    <th>Size\n  </tr>\n  <tr>\n");
  fprintf(fp, "    <td><b>Log</b>\n    <td> %s    <td>\n", get_logfile());
  {
    struct stat fstats;

    if (stat(get_logfile(), &fstats) == 0) {
      char* names[] = { "B", "KB", "MB", "GB" };
      int idname;
      double size = fstats.st_size;
      for (idname = 0; idname < 3; ++idname) {
        if (size < 1024) {
          break;
        }
        size /= 1024.0;
      }
      fprintf(fp, "%.2f %s\n", size, names[idname]);
    } else {
      fprintf(fp, "???\n");
    }
  }
  fprintf(fp, "  </tr>\n</table>\n");

  /* now pick up the rest of the descriptive text that goes along with this page.
   * instead of burying all the text in this program, just include it from an external
   * file.  For now, use the system command to append the text to the just created file.
   */
  fclose(fp);
  snprintf(tmpstr, sizeof(tmpstr), "/bin/cat %s/admin_description.html >> %s",
           BASEDIR, AdminFileName);
  int ret = system(tmpstr);

  /* Save the current variables into a file for later use.  These
   * variables are updated by each child process at the end of every
   * test.  This data must be shared between multiple children.  If
   * not, then the admin view page doesn't get update after every 
   * test.
   */
  fp = fopen("/tmp/view.string", "w");
  if (fp == NULL) {
    return;
  }
  lock.l_type = F_WRLCK;
  log_println(1, "successfully locked '/tmp/view.string' for updating");
  snprintf(view_string, sizeof(view_string),
      "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,"
      "%s,%s",
      maxc2sspd, minc2sspd, maxs2cspd, mins2cspd, totalcnt, totmismatch,
      totbad_cable, count[0], count[1], count[2], count[3], count[4],
      count[5], count[6], count[7], count[8], count[9], count[10],
      count[11], count[12], count[13], count[14], count[15], maxdate,
      mindate);
  log_println(1, "sending '%s' to tmp file", view_string);
  fprintf(fp, "%s\n", view_string);
  lock.l_type = F_UNLCK;
  fcntl(fileno(fp), F_SETLK, lock);
  fclose(fp);
}

/**
 * Initialise admin results page with previous values from the log file.
 * Not that these  valeus will be updated after completion of the tests.
 * @param refresh time in seconds after which to refresh page
 */
void view_init(int refresh) {
  int Timeouts = 0, SumRTT, CountRTT, MinRTT = 0, PktsRetrans = 0;
  int CurrentMSS, DupAcksIn, AckPktsIn, MaxRwinRcvd = 0, Sndbuf =
      0;
  // int CurrentCwnd = 0, SndLimTimeRwin, SndLimTimeCwnd, SndLimTimeSender,
  //                   DataBytesOut;
  // commenting out lines above to assign values
  int CurrentCwnd = 0, SndLimTimeRwin = 0, SndLimTimeCwnd = 0,
      SndLimTimeSender = 0, DataBytesOut = 0;

  int CongestionSignals, PktsOut = 0;
  FILE * fp;
  int c2sspd = 0, s2cspd = 0;
  char ip_addr2[64], buff[512], *str, tmpstr[32];
  int mismatch = 0, bad_cable = 0;
  int c2s_linkspeed_data = 0, s2c_linkspeed_ack = 0;
  int totalcnt = 0, view_flag = 0;

  if ((fp = fopen(get_logfile(), "r")) == NULL)
    return;

  while ((fgets(buff, 512, fp)) != NULL) {
    log_println(3, "Reading line: %s", buff);
    if ((str = strchr(buff, ',')) != NULL) {
      sscanf(buff, "%[^,]s", date);
      str++;
      sscanf(str, "%[^,]s", ip_addr2);
      if ((str = strchr(str, ',')) == NULL)
        continue;
      /* skip over s2c cwnd-limited speed value */
      str++;
      sscanf(str, "%[^,]s", tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      c2sspd = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      s2cspd = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      Timeouts = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      SumRTT = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      CountRTT = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      PktsRetrans = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      //FastRetran = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      //DataPktsOut = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      //AckPktsOut = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      CurrentMSS = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      DupAcksIn = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      AckPktsIn = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      MaxRwinRcvd = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      Sndbuf = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      CurrentCwnd = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      SndLimTimeRwin = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      SndLimTimeCwnd = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      SndLimTimeSender = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      DataBytesOut = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      //SndLimTransRwin = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      //SndLimTransCwnd = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      //SndLimTransSender = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      //MaxSsthresh = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      //CurrentRTO = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      //CurrentRwinRcvd = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      //link = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      mismatch = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      bad_cable = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      //half_duplex = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      //congestion = atoi(tmpstr);

      str = strchr(str, ',');
      if (str == NULL) {
        CongestionSignals = -1;
        goto display;
      }
      str += 1;
      sscanf(str, "%[^,]s", tmpstr);
      c2s_linkspeed_data = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      //c2s_linkspeed_ack = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      //s2c_linkspeed_data = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      s2c_linkspeed_ack = atoi(tmpstr);

      str = strchr(str, ',');
      if (str == NULL) {
        CongestionSignals = -1;
        goto display;
      }
      str += 1;
      sscanf(str, "%[^,]s", tmpstr);
      CongestionSignals = atoi(tmpstr);

      str = strchr(str, ',') + 1;
      sscanf(str, "%[^,]s", tmpstr);
      PktsOut = atoi(tmpstr);

      str = strchr(str, ',');
      if (str == NULL) {
        MinRTT = -1;
      } else {
        str += 1;
        sscanf(str, "%[^,]s", tmpstr);
        MinRTT = atoi(tmpstr);
      }

display: log_println(4, TCP_STAT_NAME" variables line received\n");
         totalcnt = calculate(date, SumRTT, CountRTT, CongestionSignals,
                              PktsOut, DupAcksIn, AckPktsIn, CurrentMSS,
                              SndLimTimeRwin, SndLimTimeCwnd, SndLimTimeSender,
                              MaxRwinRcvd, CurrentCwnd, Sndbuf, DataBytesOut,
                              mismatch, bad_cable, c2sspd, s2cspd,
                              c2s_linkspeed_data, s2c_linkspeed_ack, view_flag);
    }
  }
  fclose(fp);
  view_flag = 1;
  gen_html(c2sspd, s2cspd, MinRTT, PktsRetrans, Timeouts, Sndbuf, MaxRwinRcvd,
           CurrentCwnd, mismatch, bad_cable, totalcnt, refresh);
}
