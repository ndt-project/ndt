/**
 * This file contains function declarations for heuristics and algorithms.
 *
 *  Created : Oct 2011
 *      Author: kkumar@internet2.edu
 */

#ifndef SRC_HEURISTICS_H_
#define SRC_HEURISTICS_H_

#include "web100srv.h"

// link speed algorithms
void calc_linkspeed(char spds[4][256], int spd_index, int *c2sdata, int *c2sack,
                    int* s2cdata, int *s2cack, float runave[4],
                    u_int32_t *dec_cnt, u_int32_t *same_cnt, u_int32_t *inc_cnt,
                    int *timeout, int *dupack, int isc2stest);

// calculate average round trip time
double calc_avg_rtt(tcp_stat_var sumRTT, tcp_stat_var countRTT,
                                                  double *avgRTT);

// calculate packet loss percentage
double calc_packetloss(tcp_stat_var congsnsignals, tcp_stat_var pktsout,
                        int c2sdatalinkspd);

// Calculate the percentage of packets arriving out of order
double calc_packets_outoforder(tcp_stat_var dupackcount,
                                      tcp_stat_var actualackcount);

// calculate theoretical maximum goodput in bits
double calc_max_theoretical_throughput(tcp_stat_var currentMSS,
                                double rttsec, double packetloss);

// finalize some window sizes
void calc_window_sizes(tcp_stat_var *SndWinScale,
                       tcp_stat_var *RcvWinScale, tcp_stat_var SendBuf,
                       tcp_stat_var MaxRwinRcvd, tcp_stat_var MaxCwnd,
                       double *rwin, double *swin, double *cwin);

// calculate RTO Idle time
double calc_RTOIdle(tcp_stat_var timeouts, tcp_stat_var currentRTO,
                                                  double totaltime);

// calculate total test time for S_C test
int calc_totaltesttime(tcp_stat_var SndLimTimeRwin,
                       tcp_stat_var SndLimTimeCwnd,
                       tcp_stat_var SndLimTimeSender);

// get time ratio of 'Sender Limited' time due to congestion
double calc_sendlimited_cong(tcp_stat_var SndLimTimeCwnd,
                              int totaltime);

// get time ratio of 'Sender Limited' time due to receivers fault
double calc_sendlimited_rcvrfault(tcp_stat_var SndLimTimeRwin,
                                  int totaltime);

// get time ratio of 'Sender Limited' time due to sender's fault
double calc_sendlimited_sndrfault(tcp_stat_var SndLimTimeSender,
                                  int totaltime);

// Calculate actual throughput in Mbps
double calc_real_throughput(tcp_stat_var DataBytesOut,
                            tcp_stat_var totaltime);

// Calculate total time spent waiting for packets to arrive
double cal_totalwaittime(tcp_stat_var currentRTO,
                         tcp_stat_var timeoutcounters);

// Is throughput measured greater in value during the C->S than S->C test?
int is_c2s_throughputbetter(int c2stestresult, int s2ctestresult);

// Check if the S->C throughput computed with a limited
// cwnd value throughput test is better than that calculated by the S->C test.
int is_limited_cwnd_throughput_better(int midboxs2cspd, int s2cspd);

// Is Multiple test mode enabled?
int isNotMultipleTestMode(int multiple);

// detect duplex mismatch mode
int detect_duplexmismatch(double cwndtime, double bwtheoretcl,
                          tcp_stat_var pktsretxed, double timesec,
                          tcp_stat_var maxsstartthresh, double idleRTO,
                          int link, int s2cspd, int midboxspd, int multiple);

// detect if faulty hardware may exist
int detect_faultyhardwarelink(double packetloss, double cwndtime,
                              double timesec, tcp_stat_var maxslowstartthresh);

// Is this an ethernet link?
int detect_ethernetlink(double realthruput, double s2cspd, double packetloss,
                        double oo_order, int link);

// Is wireless link?
int detect_wirelesslink(double sendtime, double realthruput, double bw_theortcl,
                        tcp_stat_var sndlimtrans_rwin,
                        tcp_stat_var sndlimtrans_cwnd,
                        double rwindowtime, int link);

// Is DSL/Cable modem?
int detect_DSLCablelink(tcp_stat_var sndlim_timesender,
                        tcp_stat_var sndlim_transsender,
                        double realthruput, double bw_theoretical, int link);

// Is a half_duplex link present?
int detect_halfduplex(double rwintime, tcp_stat_var sndlim_transrwin,
                      tcp_stat_var sndlim_transsender, double totaltesttime);

// Is congestion detected?
int detect_congestionwindow(double cwndtime, int mismatch, double cwin,
                            double rwin, double rttsec);

// Is internal network link duplex mismatch detected?
int detect_internal_duplexmismatch(double s2cspd, double realthruput,
                                   double rwintime, double packetloss);
#endif  // SRC_HEURISTICS_H_
