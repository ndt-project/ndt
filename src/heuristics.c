/**
 * This file contains algorithms and heuristics used to arrive various results in NDT
 *
 *
 *  Created : Oct 15, 2011
 *      Author: kkumar@internet2.edu
 */
#include <string.h>
#include <math.h>

#include "utils.h"
#include "logging.h"
#include "heuristics.h"

#define log_lvl_heur 2
/**
 * Compute link speed.
 *
 * Throughput is quantized into one of a group
 * of pre-defined bins by incrementing the counter for that bin.
 * Once the test is complete, determine link speed according to the bin
 * with the largest counter value.
 *
 * The bins are defined in mbits/second.
 *
 * Value of bin Index : Value's meaning
 *  0:  0 < inter-packet throughput (mbits/second) <= 0.01 - RTT
 *  1:  0.01 < inter-packet throughput (mbits/second) <= 0.064 - Dial-up Modem
 *  2:  0.064 < inter-packet throughput (mbits/second) <= 1.5 - Cable/DSL modem
 *  3:  1.5 < inter-packet throughput (mbits/second) <= 10 - 10 Mbps Ethernet or WiFi 11b subnet
 *  4:  10 < inter-packet throughput (mbits/second) <= 40 - 45 Mbps T3/DS3 or WiFi 11 a/g subnet
 *  5:  40 < inter-packet throughput (mbits/second) <= 100 - 100 Mbps Fast Ethernet subnet
 *  6:  100 < inter-packet throughput (mbits/second) <= 622 - a 622 Mbps OC-12 subnet
 *  7:  622 < inter-packet throughput (mbits/second) <= 1000 - 1.0 Gbps Gigabit Ethernet subnet
 *  8:  1000 < inter-packet throughput (mbits/second) <= 2400 - 2.4 Gbps OC-48 subnet
 *  9:  2400 < inter-packet throughput (mbits/second) <= 10000 - 10 Gbps 10 Gigabit Ethernet/OC-192 subnet
 * 10: bits cannot be determined - Retransmissions (this bin counts the duplicated or invalid packets and does not denote a real link type)
 *    otherwise - ?
 *
 * @param spds speed string array used as speed counter bins
 * @param spd_index speed index indicating indices of the speed bin array
 * @param c2s_linkspeed_data Data link speed as detected by server data
 * @param c2s_linkspeed_ack Data link speed(type) as detected by server acknowledgments
 * @param s2c_linkspeed_data Data link speed as detected by client data
 * @param s2c_linkspeed_ack Data link speed as detected by client acknowledgments
 * @param runave average run
 * @param dec_cnt number of times window sizes have been decremented
 * @param same_cnt number of times window sizes remained the same
 * @param inc_cnt number of times window sizes have been incremented
 * @param timeout number of times a timeout occurred (based on network packet pair times)
 * @param dupack  number of times duplicate acks were received
 * @param is_c2stest is this a C->S test?
 *
 * */
void calc_linkspeed(char spds[4][256], int spd_index, int *c2s_linkspeed_data,
                    int *c2s_linkspeed_ack, int* s2c_linkspeed_data,
                    int *s2c_linkspeed_ack, float runave[4], u_int32_t *dec_cnt,
                    u_int32_t *same_cnt, u_int32_t *inc_cnt, int *timeout,
                    int *dupack, int is_c2stest) {
  int index = 0;  // speed array indices
  int links[16];  // link speed bin values
  int n = 0, j = 0;  // temporary iterator variables
  int max;  // max speed bin counter value
  int total;  // total of the bin counts, used to calculate percentage
  int ifspeedlocal;  // local if-speed indicator

  for (n = 0; n < spd_index; n++) {
    sscanf(spds[n],
           "%d %d %d %d %d %d %d %d %d %d %d %d %f %u %u %u %d %d %d",
           &links[0], &links[1], &links[2], &links[3], &links[4],
           &links[5], &links[6], &links[7], &links[8], &links[9],
           &links[10], &links[11], &runave[n], inc_cnt, dec_cnt,
           same_cnt, timeout, dupack, &ifspeedlocal);
    log_println(log_lvl_heur, " **First ele: spd=%s, runave=%f", spds[n],
                runave[n]);
    max = 0;
    index = 0;
    total = 0;

    if ((ifspeedlocal == -1) || (ifspeedlocal == 0) || (ifspeedlocal > 10))
      ifspeedlocal = 10;  // ifspeed was probably not collected in these cases

    // get the ifspeed bin with the biggest counter value.
    //  NDT determines link speed using this
    for (j = 0; j <= ifspeedlocal; j++) {
      total += links[j];
      if (max < links[j]) {
        max = links[j];
        index = j;
      }
    }

    // speed data was not collected correctly
    if (links[index] == -1)
      index = -1;

    // log
    log_println(0, "spds[%d] = '%s' max=%d [%0.2f%%]\n", n, spds[n], max,
                (float) max * 100 / total);

    // Assign values based on whether C2S or S2C test
    // Note: spd[0] , [1] contains C->S test results
    // spd[2] , spd [3] contains S->C test results
    switch (n + (is_c2stest ? 0 : 2)) {
      case 0:
        *c2s_linkspeed_data = index;
        log_print(log_lvl_heur, "Client --> Server data detects link = ");
        break;
      case 1:
        *c2s_linkspeed_ack = index;
        log_print(log_lvl_heur, "Client <-- Server Ack's detect link = ");
        break;
      case 2:
        *s2c_linkspeed_data = index;
        log_print(log_lvl_heur, "Server --> Client data detects link = ");
        break;
      case 3:
        *s2c_linkspeed_ack = index;
        log_print(1, "Server <-- Client Ack's detect link = ");
        break;
    }

    // classify link speed based on the max ifspeed seen
    log_linkspeed(index);
  }  // end section to determine speed.
}  // end method calc_linkspeed

/**
 * Calculate average round-trip-time in milliseconds.
 * @param sumRTT - The sum of all sampled round trip times
 * @param countRTT - The number of round trip time samples
 * @return average round trip time
 * */

double calc_avg_rtt(tcp_stat_var sumRTT, tcp_stat_var countRTT,
                                                  double *avgRTT) {
  *avgRTT = (double) sumRTT / countRTT;
  log_println(log_lvl_heur,
              "-- Average round trip time= SumRTT (%"VARtype") over "
              "countRTT (%"VARtype")=%f",
              sumRTT, countRTT, (*avgRTT) * .001);
  return ((*avgRTT) * .001);
}

/**
 * Calculate packet loss as the percentage of the lost packets vs total txed segments
 * during the Server-To-Client  throughput test.
 *
 * To avoid possible division by zero, sets the packet loss percentages
 * to certain values when the CongestionSignals is 0:
 *
 *   0.0000000001 - link-type detected by the Bottleneck Link Detection algorithm
 *   		using the C-S data packets' inter-packet arrival times indicates > 100 Mbps Fast Ethernet subnet.
 *   0.000001 - otherwise

 * @param congsnsignals - number of multiplicative
 * 			downward congestion window adjustments due to all
 * 			forms of congestion signals
 * @param pktsout total number Txed segments
 * @param c2sdatalinkspd Integer indicative of link speed, as collected in the C->S test
 * @return packet loss value
 * */
double calc_packetloss(tcp_stat_var congsnsignals, tcp_stat_var pktsout,
                        int c2sdatalinkspd) {
  double packetloss = (double) congsnsignals / pktsout;
  if (packetloss == 0) {
    if (c2sdatalinkspd > 5)
      packetloss = .0000000001;  // set to 10^-10 for links faster than FastE
    else
      packetloss = .000001;  // set to 10^-6 otherwise
  }
  log_println(log_lvl_heur, "--packetloss=%"VARtype" over %"VARtype
              "=%f. Link spd=%d",
              congsnsignals, pktsout, packetloss, c2sdatalinkspd);
  return packetloss;
}

/**
 * Calculate the percentage of packets arriving out of order.
 * This is equal to the ratio of #duplicate acks over
 * 	#actual acks  recieved.
 * 	@param dupackcount number of duplicate ACKs received
 * 	@param actualackcount number of non-duplicate ACKs received
 * 	@return percentage of packets out of order
 * */
double calc_packets_outoforder(tcp_stat_var dupackcount,
                                      tcp_stat_var actualackcount) {
  log_println(log_lvl_heur, "--packets out of order: %f",
              (double) dupackcount / actualackcount);
  return ((double) dupackcount / actualackcount);
}

/**
 * Calculate maximum theoretical throughput in bits using the Mathis equation.
 *
 * The Theoretical Maximum Throughput should be computed to receive
 * Mbps instead of Mibps. This is the only variable in the NDT that is kept in
 * Mibps, so it might lead to the inconsistent results when comparing it with the other values.
 *
 * @param currentMSS current maximum segment size (MSS), in octets
 * @param rttsec average round trip time (Latency/Jitter) in seconds
 * @param packetloss Packet loss
 * @return maximum theoretical bandwidth
 * */
double calc_max_theoretical_throughput(tcp_stat_var currentMSS,
                                double rttsec, double packetloss) {
  double maxthruput;
  maxthruput = (currentMSS / (rttsec * sqrt(packetloss))) * BITS_8 / KILO_BITS
      / KILO_BITS;
  log_println(log_lvl_heur, "--max_theoretical_thruput: %f. From %"
              VARtype",%f,%f",
              maxthruput, currentMSS, rttsec, packetloss);
  return maxthruput;
}

/**
 * Finalize some window sizes based on web100 values obtained
 *
 * @param SndWinScale SendWinScale web100 var value
 * @param SendBuf SendBuf web100 var value
 * @param MaxRwinRcvd MaxRwinRcvd web100 var value
 * @param rwin   Receive window value in bytes
 * @param swin   Send window value in bytes
 * @param cwin   congestion window value in bytes
 *
 * */
void calc_window_sizes(tcp_stat_var *SndWinScale,
                       tcp_stat_var *RcvWinScale, tcp_stat_var SendBuf,
                       tcp_stat_var MaxRwinRcvd, tcp_stat_var MaxCwnd,
                       double *rwin, double *swin, double *cwin) {
  if ((*SndWinScale > WINDOW_SCALE_THRESH) || (SendBuf < MAX_TCP_PORT))
    *SndWinScale = 0;
  if ((*RcvWinScale > WINDOW_SCALE_THRESH) || (MaxRwinRcvd < MAX_TCP_PORT))
    *RcvWinScale = 0;

  *rwin = (double) MaxRwinRcvd * BITS_8 / KILO_BITS / KILO_BITS;
  *swin = (double) SendBuf * BITS_8 / KILO_BITS / KILO_BITS;
  *cwin = (double) MaxCwnd * BITS_8 / KILO_BITS / KILO_BITS;
  log_println(
      log_lvl_heur,
      "--window sizes: SndWinScale= %"VARtype", RcvwinScale=%"VARtype
      ", MaxRwinRcvd=%"VARtype", "
      "maxCwnd=%"VARtype",rwin=%f, swin=%f, cwin=%f",
      *SndWinScale, *RcvWinScale, MaxRwinRcvd, MaxCwnd, *rwin, *swin, *cwin);
}

/**
 * Calculate the fraction of idle time spent waiting for packets to arrive.
 * Current retransmit timeout * timeout count = idle time spent waiting for packets to arrive.
 * When divided by total test time, they indicate fraction of time spent idle due to RTO
 * @param timeouts number of timeouts
 * @param currentRTO current retransmit timeout
 * @param totaltime total test time
 * @return idle time fraction
 * */
double calc_RTOIdle(tcp_stat_var timeouts, tcp_stat_var currentRTO,
                                                  double totaltime) {
  double idlewaitingforpackets = (timeouts * ((double) currentRTO / 1000))
      / totaltime;
  log_println(log_lvl_heur, "--RTOIdle:%f", idlewaitingforpackets);
  return idlewaitingforpackets;
}

/**
 * Get total test time used by the Server-To-Client throughput test.
 *
 * @param SndLimTimeRwin SndLimTimeRwin web100 var value, cumulative time spent
 * 		in the 'Receiver Limited' state during the S->C throughput test
 * @param SndLimTimeCwnd SndLimTimeCwnd web100 var value, cumulative time spent in
 * 			 'Congestion Limited' state
 * @param SndLimTimeSender SndLimTimeSender web100 var value, total time spend in the
 * 			'sender limited' state
 * @return Total test time
 * */
int calc_totaltesttime(tcp_stat_var SndLimTimeRwin,
                       tcp_stat_var SndLimTimeCwnd,
                       tcp_stat_var SndLimTimeSender) {
  int totaltime = SndLimTimeRwin + SndLimTimeCwnd + SndLimTimeSender;
  log_println(log_lvl_heur, "--Total test time: %"VARtype"+%"VARtype"+%"
              VARtype"=%d ", SndLimTimeRwin,
              SndLimTimeCwnd, SndLimTimeSender, totaltime);
  return (totaltime);
}

/**
 * get 'Sender Limited' state time ratio
 * @param SndLimTimeSender cumulative time spent in the 'Sender Limited' state due to own fault
 * @param totaltime Total test time
 * @return sender limited time ratio
 *
 */
double calc_sendlimited_sndrfault(tcp_stat_var SndLimTimeSender,
                                  int totaltime) {
  double sendlimitedtime = ((double) SndLimTimeSender) / totaltime;
  log_println(log_lvl_heur, "--Send limited time: %"VARtype" over %d=%f ",
              SndLimTimeSender, totaltime, sendlimitedtime);
  return sendlimitedtime;
}

/**
 * get ratio of time spent in 'send Limited' state due to receiver end limitations
 * @param SndLimTimeRwin cumulative time spent in the 'send Limited' state due
 * 		to the receiver window limits
 * @param totaltime Total test time
 * @return sender limited time ratio
 *
 */
double calc_sendlimited_rcvrfault(tcp_stat_var SndLimTimeRwin,
                                  int totaltime) {
  double sendlimitedtime = ((double) SndLimTimeRwin) / totaltime;
  log_println(log_lvl_heur, "--Send limited time: %"VARtype" over %d=%f ",
              SndLimTimeRwin, totaltime, sendlimitedtime);
  return sendlimitedtime;
}

/**
 * get ratio of time spent in 'Sender Limited' state time due to congestion
 * @param SndLimTimeSender cumulative time spent in the 'Sender Limited' state due to congestion
 * @param totaltime Total test time
 * @return sender limited time ratio
 */
double calc_sendlimited_cong(tcp_stat_var SndLimTimeCwnd,
                              int totaltime) {
  double sendlimitedtime = ((double) SndLimTimeCwnd) / totaltime;
  log_println(log_lvl_heur, "--Send limited time: %"VARtype" over %d=%f ",
              SndLimTimeCwnd, totaltime, sendlimitedtime);
  return sendlimitedtime;
}

/**
 * Calculate actual throughput in Mbps as the ratio of total transmitted bytes
 * over total test time
 *
 * @param DataBytesOut Number of bytes sent out
 * @param totaltime Total test time in microseconds
 * @return Actual throughput
 */
double calc_real_throughput(tcp_stat_var DataBytesOut,
                            tcp_stat_var totaltime) {
  double realthruput = ((double) DataBytesOut / (double) totaltime) * BITS_8;
  log_println(log_lvl_heur, "--Actual observed throughput: %f ", realthruput);
  return realthruput;
}

/**
 * Calculate total time spent waiting for packets to arrive
 *
 * @param CurrentRTO web100 value indicating current value of the retransmit timer RTO.
 * @param Timeouts  web100 value indicating # of times the retransmit timeout has
 * 				 expired when the RTO backoff multiplier is equal to one
 * @return Actual throughput
 */
double cal_totalwaittime(tcp_stat_var currentRTO,
                         tcp_stat_var timeoutcounters) {
  double waitarrivetime = (double) (currentRTO * timeoutcounters) / KILO;
  log_println(log_lvl_heur, "--Total wait time: %f ", waitarrivetime);
  return waitarrivetime;
}

/**
 * Check if the S->C throughput as calculated by the midbox test , with a limited
 * cwnd value, is better than that calculated by the S->C test.
 *
 * @param midboxs2cspd S->C throughput calculated by midbox tests wiht limited cwnd
 * @param s2ctestresult  throughput calculated by the s2c tests
 * @return 1 (true) if midboxs2cspd > s2cspd, else 0.
 */
int is_limited_cwnd_throughput_better(int midboxs2cspd, int s2cspd) {
  int thruputmismatch = 0;
  if (midboxs2cspd > s2cspd)
    thruputmismatch = 1;
  return thruputmismatch;
}

/**
 * Check if there is a possible duplex mismatch condition. This is done by checking
 * if the S->C throughput as calculated by the C->S throughput tests is greater than
 * that calculated by the S->C test.
 *
 * @param c2stestresult throughput calculated by the c2s tests
 * @param s2ctestresult  throughput calculated by the c2s tests
 * @return 1 (true) if c2stestresult > s2ctestresult, else 0.
 */
int is_c2s_throughputbetter(int c2stestresult, int s2ctestresult) {
  int c2sbetter_yes = 0;
  if (c2stestresult > s2ctestresult)
    c2sbetter_yes = 1;
  return c2sbetter_yes;
}

/**
 * Check if Multiple test mode is enabled.
 * @param multiple value of the "multiple test mode" setting
 * @return 1(true) if multiple test mode is on, else 0
 */
int isNotMultipleTestMode(int multiple) {
  log_println(log_lvl_heur, "--multiple: %f ", multiple);
  return (multiple == 0);
}

/**
 * Calculate duplex mismatch in network link. The conditions to determine this:
 *
 *  -'Congestion Limited' state time share > 90%
 *  -Theoretical Maximum Throughput > 2Mibps
 *  - # of segments transmitted containing some retransmitted data > 2 per second
 *  - Maximum slow start threshold > 0
 *  - Cumulative expired retransmit timeouts RTO > 1% of the total test time
 *  - link type detected is not wireless link
 *  - throughput measured during Middlebox test (with a limited CWND) >
 *  			throughput measured during S->C test
 *  - throughput measured during C->S test > throughput measured during the S2C test
 * @param cwndtime     Time spent send limited due to congestion
 * @param bwtheoretcl  theoreticallly calculated bandwidth
 * @param pktsretxed   # of segments with retransmitted packets
 * @param maxsstartthresh Maximum slow start threshold
 * @param idleRTO      Cumulative expired retransmit timeouts RTO
 * @param int s2cspd   Throughput calculated during S->C tests
 * @param midboxspd    Throughput calculated during middlebox tests
 * @param multiple     is multiple test mode on?
 * @return int 1 if duplex mismatch is found, 0 if not.
 * */
int detect_duplexmismatch(double cwndtime, double bwtheoretcl,
                          tcp_stat_var pktsretxed, double timesec,
                          tcp_stat_var maxsstartthresh, double idleRTO,
                          int link, int s2cspd, int midboxspd, int multiple) {
  int duplex_mismatch_yes = 0;
  if ((cwndtime > .9)  // more than 90% time spent being receiver window limited
      && (bwtheoretcl > 2)  // theoretical max goodput > 2mbps
      && (pktsretxed / timesec > 2)
      // #of segments with pkt-retransmissions> 2
      && (maxsstartthresh > 0)  // max slow start threshold > 0
      && (idleRTO > .01)  // cumulative RTO time > 1% test duration
      && (link > 2)  // not wireless link
      // S->C throughput calculated
      && is_limited_cwnd_throughput_better(midboxspd, s2cspd)
      // by server < client value
      && isNotMultipleTestMode(multiple)) {
    duplex_mismatch_yes = 1;
  }  // end if
  log_println(log_lvl_heur, "--duplexmismatch?: %d ", duplex_mismatch_yes);
  return duplex_mismatch_yes;
}

/** Check if internal link is duplex mismatched
 * @param s2cspd Throughput calculated during the S->C test
 * @param realthruput  actual observed throuput
 * @param rwintime 	  time spend limited sending due to receive window limitations
 * @param packetloss  packet loss during S->C tests
 * return 1 if internal duplex mismatch is found, else 0
 * */
int detect_internal_duplexmismatch(double s2cspd, double realthruput,
                                   double rwintime, double packetloss) {
  int duplex_mismatch_yes = 0;
  if ((s2cspd > 50)  // S->C goodput > 50 Mbps
      && (realthruput < 5)  // actual send throughput < 5 Mbps
      && (rwintime > .9)  // receive window limited for >90% of the time
      && (packetloss < .01)) {
    duplex_mismatch_yes = 1;
  }
  log_println(log_lvl_heur, "--internal duplexmismatch?: %d ",
              duplex_mismatch_yes);
  return duplex_mismatch_yes;
}

/**
 * Determine whether faulty hardware is affecting performance by observing some
 * threshold values. Particularly, see if
 * - connection is losing more than 15 packets/second
 * - connections spends > 60% in congestion window limited state
 * - packet loss < 1% of packets txed
 * - connection entered TCP slow start stage
 *
 * This heuristic seems to in error:
 * 1) Instead of obtaining
 * 		packet per second loss rate = count_lost_packets / test_duration, the loss rate is multiplied times 100.
 * 		 If "packet loss" is less than 1% (3rd condition), then the packet loss multiplied by 100 and divided by the
 * 		 total test time in seconds will be less than 1 anyway.
 * 2) 'Congestion Limited' state time ratio need not be divided by the total_test
 * 		   time_in_seconds , but be directly compared to "0.6".
 * @param packetloss packetloss calculated
 * @param cwndtime   congestion window limited time
 * @param timesec    total test time
 * @param maxslowstartthresh  max slow start threshold value
 * @return 1 is fault hardware suspected, 0 otherwise
 * */
int detect_faultyhardwarelink(double packetloss, double cwndtime,
                              double timesec, tcp_stat_var maxslowstartthresh) {
  int faultyhw_found = 0;
  if (((packetloss * 100) / timesec > 15) && (cwndtime / timesec > .6)
      && (packetloss < .01) && (maxslowstartthresh > 0)) {
    faultyhw_found = 1;
  }
  log_println(log_lvl_heur, "--faulty hardware?: %d ", faultyhw_found);
  return faultyhw_found;
}

/**
 * Determine whether link is an ethernet link:
 * the WiFi and DSL/Cable modem are not detected,
 * the total Send Throughput is in the range [3 , 9.5] (not inclusive)
 * and the connection is very stable.
 *
 * This specific conditions are:
 * - actual measured throughput < 9.5 Mbps, but > 3 Mbps
 * - S->C throughput < 9.5 Mbps
 * - packet loss < 1%
 * - out of order packets proportion < .35
 * - the heuristics for WiFi and DSL/Cable modem links give negative results
 * @param realthruput Actual calculated throughput
 * @param s2cspd  S->C throuhput
 * @param packetloss packet loss seen
 * @param oo_order  out-pf-order packet ration
 * @param link integer indicative of link type
 * @return 1 if ethernet link, 0 otherwise
 * */
int detect_ethernetlink(double realthruput, double s2cspd, double packetloss,
                        double oo_order, int link) {
  int is_ethernet = 0;
  if ((realthruput < 9.5) && (realthruput > 3.0) && ((s2cspd / 1000) < 9.5)
      && (packetloss < .01) && (oo_order < .035) && (link > 0)) {
    is_ethernet = 1;
  }
  log_println(log_lvl_heur, "--Is ethernet?: %d ", is_ethernet);
  return is_ethernet;
}

/**
 * Determine whether link is a wireless link:
 * DSL/Cable modem are not detected, the NDT Client
 * is a bottleneck and actual observed Throughput < 5 Mbps,
 * but the Theoretical Maximum Throughput is > 50 Mibps.
 *
 * This specific conditions are:
 * - The cumulative time spent in the 'Sender Limited' state is 0 ms
 * - The actual (real, measured) throughput < 5 Mbps
 * - theoretical bw calculated > 50 Mibps
 * - # of transitions into 'Receiver Limited' state == # of transitions
 * 						into'Congestion Limited' state
 * - 'Receiver Limited' state time ratio is greater than 90%
 * - the heuristics for WiFi and DSL/Cable modem links give negative results
 * @param sendtime cumulative time spent in "sender limited" state
 * @param realthruput Actual calculated throughput
 * @param bw_theortcl Theoretical bandwidth
 * @param sndlimtrans_rwin # of transitions into rcvr-limited state
 * @param sndlimtrans_cwnd # of transitions into cwnd-limited state
 * @param double rwindowtime "receive window limited" cumulative time
 * @param link integer indicative of link type
 * @return 1 if wireless link, 0 otherwise
 * */
int detect_wirelesslink(double sendtime, double realthruput, double bw_theortcl,
                        tcp_stat_var sndlimtrans_rwin,
                        tcp_stat_var sndlimtrans_cwnd,
                        double rwindowtime, int link) {
  int is_wireless = 0;
  if ((sendtime == 0) && (realthruput < 5) && (bw_theortcl > 50)
      && ((sndlimtrans_rwin / sndlimtrans_cwnd) == 1)
      && (rwindowtime > .90) && (link > 0)) {
    is_wireless = 1;
  }
  log_println(log_lvl_heur, "--Is wireless?: %d ", is_wireless);
  return is_wireless;
}

/**
 *
 * The link is treated as a DSL/Cable modem when the NDT Server isn't a
 *  bottleneck and the Total Send Throughput is less than 2 Mbps and
 *  less than the Theoretical Maximum Throughput.
 *
 *  The specific conditions used:
 *  - The cumulative time spent in the 'Sender Limited' state is less than 0.6 ms
 *  - The number of transitions into the 'Sender Limited' state is 0
 *  - The Total Send Throughput is less than 2 Mbps
 *  - The Total Send Throughput is less than Theoretical Maximum Throughput
 *
 * @param sndlimtimesender cumulative time spent in the 'Sender Limited' state
 * @param sndlimtranssender # of transitions into sender limited state
 * @param realthruput Actual calculated throughput
 * @param bw_theoretical Theoretical bandwidth
 * @param link integer indicative of link type
 * @return 1 if wireless link, 0 otherwise
 * */
int detect_DSLCablelink(tcp_stat_var sndlim_timesender,
                        tcp_stat_var sndlim_transsender,
                        double realthruput, double bw_theoretical, int link) {
  int is_dslorcable = 0;
  if ((sndlim_timesender < 600) && (sndlim_transsender == 0)
      && (realthruput < 2) && (realthruput < bw_theoretical)
      && (link > 0)) {
    is_dslorcable = 1;
  }
  log_println(log_lvl_heur, "--Is DSL/Cable?: %d ", is_dslorcable);
  return is_dslorcable;
}

/**
 * Determine if there is a half-duplex link in the path by looking at
 *  web100 variables. Look for a connection that toggles rapidly
 *  between the sender buffer limited and receiver buffer limited states, but
 *  does not remain in the sender-buffer-limited-state and instead spends > 95%
 *  time in rcvr-buffer-limited state.
 *
 *   The specific conditions used:
 * - 'Receiver Limited' state time ratio > 95%
 * - # of transitions into the 'Receiver Limited' state > 30 per second
 * - # of transitions into the 'Sender Limited' state > 30 per second
 *
 * @param rwintime 'Receiver Limited' state time ratio
 * @param sndlim_transrwin # of transitions into receiver limited state
 * @param sndlim_transsender # of transitions into sender limited state
 * @param totaltesttime total test time
 * @return 1 if half_duplex link suspected, 0 otherwise
 * */
int detect_halfduplex(double rwintime, tcp_stat_var sndlim_transrwin,
                      tcp_stat_var sndlim_transsender, double totaltesttime) {
  int is_halfduplex = 0;
  if ((rwintime > .95) && (sndlim_transrwin / totaltesttime > 30)
      && (sndlim_transsender / totaltesttime > 30)) {
    is_halfduplex = 1;
  }
  log_println(log_lvl_heur, "--Is Half_duplex detected? %d ", is_halfduplex);
  return is_halfduplex;
}

/**
 * Congestion is detected when the connection is
 *  congestion limited a non-trivial percent of the time,
 *  there isn't a duplex mismatch detected and the NDT
 *   Client's receive window is not the limiting factor.
 *
 *   The specific conditions used:
 * - 'Congestion Limited' state time share is greater than 2%
 * - Duplex mismatch condition heuristic gives negative results
 * - Max window advertisement received > than the maximum
 * 				congestion window used during Slow Start
 *
 * @param cwndtime time spent in being send-limited due to congestion window
 * @param mismatch id duplex_mismatch detected?
 * @param cwin congestion window size
 * @param rwin max window size advertisement rcvd
 * @param rttsec total round-trip time
 * @return 1 if congestion detected, 0 otherwise
 * */
int detect_congestionwindow(double cwndtime, int mismatch, double cwin,
                            double rwin, double rttsec) {
  int is_congested = 0;
  if ((cwndtime > .02) && (mismatch == 0)
      && ((cwin / rttsec) < (rwin / rttsec))) {
    is_congested = 1;
  }
  log_println(log_lvl_heur, "--Is congested? %d ", is_congested);
  return is_congested;
}
