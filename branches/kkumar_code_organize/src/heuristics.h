/**
 * This file contains function declarations for heuristics and algorithms.
 *
 *  Created : Oct 2011
 *      Author: kkumar@internet2.edu
 */

#ifndef HEURISTICS_H_
#define HEURISTICS_H_

// link speed algorithms
void calc_linkspeed(char *spds, int spd_index, int *c2sdata, int *c2sack,
		int* s2cdata, int *s2cack, float runave[4], u_int32_t *dec_cnt,
		u_int32_t *same_cnt, u_int32_t *inc_cnt, int *timeout, int *dupack,
		int isc2stest);

// calculate average round trip time
double calc_avg_rtt(int sumRTT, int countRTT, double * avgRTT);

// calculate packet loss percentage
double calc_packetloss(int congsnsignals, int pktsout, int c2sdatalinkspd);

// Calculate the percentage of packets arriving out of order
double calc_packets_outoforder(int dupackcount, int actualackcount);

// calculate theoretical maximum goodput in bits
double calc_max_theoretical_throughput(int currentMSS, double rttsec,
		double packetloss);

// finalize some window sizes
void calc_window_sizes(int *SndWinScale, int *RcvWinScale, int SendBuf,
		int MaxRwinRcvd, int MaxCwnd, double *rwin, double *swin, double *cwin);

// calculate RTO Idle time
double calc_RTOIdle(int timeouts, int CurrentRTO, double totaltime);

// calculate total test time for S_C test
int calc_totaltesttime(int SndLimTimeRwin, int SndLimTimeCwnd,
		int SndLimTimeSender);

// get time ratio of 'Sender Limited' time due to congestion
double calc_sendlimited_cong(int SndLimTimeCwnd, int totaltime);

// get time ratio of 'Sender Limited' time due to receivers fault
double calc_sendlimited_rcvrfault(int SndLimTimeRwin, int totaltime);

// get time ratio of 'Sender Limited' time due to sender's fault
double calc_sendlimited_sndrfault(int SndLimTimeSender, int totaltime);

// Calculate actual throughput in Mbps
double calc_real_throughput(int DataBytesOut, int totaltime);

// Calculate total time spent waiting for packets to arrive
double cal_totalwaittime(int currentRTO, int timeoutcounters);

// Is throughput measured greater in value during the C->S than S->C test?
int is_c2s_throughputbetter(int c2stestresult, int s2ctestresult);

// Check if the S->C throughput computed with a limited
// cwnd value throughput test is better than that calculated by the S->C test.
int is_limited_cwnd_throughput_better(int midboxs2cspd, int s2cspd);

// Is Multiple test mode enabled?
int isNotMultipleTestMode(int multiple);

// detect duplex mismatch mode
int detect_duplexmismatch(double cwndtime, double bwtheoretcl, int pktsretxed,
		double timesec, int maxsstartthresh, double idleRTO, int link,
		int s2cspd, int midboxspd, int multiple);

// detect if faulty hardware may exist
int detect_faultyhardwarelink(double packetloss, double cwndtime,
		double timesec, int maxslowstartthresh);

// Is this an ethernet link?
int detect_ethernetlink(double realthruput, double s2cspd, double packetloss,
		double oo_order, int link);

// Is wireless link?
int detect_wirelesslink(double sendtime, double realthruput, double bw_theortcl,
		int sndlimtrans_rwin, int sndlimtrans_cwnd, double rwindowtime,
		int link);

// Is DSL/Cable modem?
int detect_DSLCablelink(int sndlim_timesender, int sndlim_transsender,
		double realthruput, double bw_theoretical, int link);

// Is a half_duplex link present?
int detect_halfduplex(double rwintime, int sndlim_transrwin,
		int sndlim_transsender, double totaltesttime);

// Is congestion detected?
int detect_congestionwindow(double cwndtime, int mismatch, double cwin,
		double rwin, double rttsec);
#endif /* HEURISTICS_H_ */
