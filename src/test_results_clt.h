/*
 * test_results_clt.h
 *
 *  Created on: Oct 19, 2011
 *      Author: kkumar
 */

#ifndef SRC_TEST_RESULTS_CLT_H_
#define SRC_TEST_RESULTS_CLT_H_

// determine linkspeed based on link speed indicator
double get_linkspeed(int c2s_linkspeed_ind, int half_duplex_ind);

// Check if faulty hardware was detected
void check_badcable(int is_bad_able);

// Check if congestion was detected, and print results
void check_congestion(int is_congested);

// Print results of the mismatch detection
void print_results_mismatchcheck(int mismatch);

// Calculate and display recommended buffer sizes
void print_recommend_buffersize(double rwin, double rttsec, double avgrtt,
                                double mylink, int max_RwinRcvd);

// Check if faulty hardware was detected, and print results
void check_badcable(int is_bad_cable);

// Check if congestion was detected, and print results
void check_congestion(int is_congested);

// Check if packet queuing is a problem at the C->S end.
void check_C2Spacketqueuing(double c2sthruput, double spdout, int sndqueue,
                            int pktcount, int buflength);

//  Deduce if packet queuing is a problem at the S->C end from values collected
//  .. during the S2C tests.
void check_S2Cpacketqueuing(double s2cthroughput, double spdin, int srvsndqueue,
                            int sbytecount);

// Print suggestions based on packet loss/retransmissions details obtained
void print_packetloss_statistics(int PktsRetrans, int DupAcksIn, int SACKsRcvd,
                                 double ooorder, int Timeouts, double waitsec,
                                 double totaltesttime);

// Print details about being receiver, sender or congestion window limited.
void print_limitedtime_ratio(double rwintime, double rwin, double sendtime,
                             double swin, double cwndtime, double rttsec,
                             double mylinkspeed, int sndbuf, int max_rwinrcvd);

// Check if excessive packet loss is affecting performance
void print_packetloss_excess(double spd, double loss);

// Print if TCP Selective Acknowledgment Options based on RFC 2018 is on
void print_SAck_RFC2018(int SACKEnabled);

// Print if Nagle algorithm (rfc 896) is on
void print_Nagle_RFC896(int is_nagleenabled);

//  Print if Explicit congestion notification to IP is on - RFC 3168 related
void print_congestion_RFC3168(int is_ECNenabled);

// Print details of whether time stamping is on - RFC 1323 based.
void print_timestamping_RFC1323(int is_timestampenabled);

// Print window scaling based data
void print_windowscaling(int MaxRwinRcvd, int WinScaleRcvd, int WinScaleSent);

// Print details of throughput limits(thresholds) imposed by the send, receive
// or congestion window values
void print_throughputlimits(int max_rwinrcvd, int RcvWinScale, int *sndbuf,
                            double s_win, double r_win, double c_win,
                            double rttsec, double estimate);

// Print details of link speed as seen by data and ack speed indicators
void print_linkspeed_dataacks(int isC2S_enabled, int c2s_linkspeed_data,
                              int c2s_linkspeed_ack, int s2c_linkspeed_data,
                              int s2c_linkspeed_ack);

void print_throughput_snapshots(struct throughputSnapshot *s2c_ThroughputSnapshots,
                                struct throughputSnapshot *c2s_ThroughputSnapshots);

// Check if a Network Address translation box is modifying IP addresses
// of server or client
void check_NAT(char *ssip, char *csip, char *scip, char *ccip);

// Check packet size preservation
void check_MSS_modification(int is_timestampenabled, int *mssvalue);
#endif  // SRC_TEST_RESULTS_CLT_H_
