/*
 * This file contains the table with the descriptions of the Web100
 * variables.
 *
 * Jakub S³awiñski 2006-06-20
 * jeremian@poczta.fm
 */

#ifndef _JS_VARINFO_H
#define _JS_VARINFO_H

char* web100vartable[][2] = {
  {"AckPktsIn", "???\nThe number of ACK packets received.\n"},
  {"AckPktsOut", "???\nThe number of ACK packets sent.\n"},
  {"BytesRetrans", "???\nThe number of retransmitted bytes.\n"},
  {"CongAvoid", "The number of times the congestion window has been\n"
                "increased by the Congestion Avoidance algorithm.\n"},
  {"CongestionOverCount", "The number of congestion events which were 'backed out' of\n"
                          "the congestion control state machine such that the\n"
                          "congestion window was restored to a prior value. This can\n"
                          "happen due to the Eifel algorithm [RFC3522] or other\n"
                          "algorithms which can be used to detect and cancel spurious\n"
                          "invocations of the Fast Retransmit Algorithm.\n"},
  {"CongestionSignals", "The number of multiplicative downward congestion window\n"
                        "adjustments due to all forms of congestion signals,\n"
                        "including Fast Retransmit, ECN and timeouts. This object\n"
                        "summarizes all events that invoke the MD portion of AIMD\n"
                        "congestion control, and as such is the best indicator of\n"
                        "how cwnd is being affected by congestion.\n"},
  {"CountRTT", "The number of round trip time samples included in\n"
               "tcpEStatsPathSumRTT and tcpEStatsPathHCSumRTT.\n"},
  {"CurCwnd", "The current congestion window, in octets.\n"},
  {"CurRTO", "The current value of the retransmit timer RTO.\n"},
  {"CurRwinRcvd", "The most recent window advertisement received, in octets.\n"},
  {"CurRwinSent", "The most recent window advertisement sent, in octets.\n"},
  {"CurSsthresh", "The current slow start threshold in octets.\n"},
  {"DSACKDups", "The number of duplicate segments reported to the local host\n"
                "by D-SACK blocks.\n"},
  {"DataBytesIn", "???\nThe number of data bytes received.\n"},
  {"DataBytesOut", "???\nThe number of data bytes sent.\n"},
  {"DataPktsIn", "???\nThe number of data packets received.\n"},
  {"DataPktsOut", "???\nThe number of data packets sent.\n"},
  {"DupAcksIn", "The number of duplicate ACKs received.\n"},
  {"ECNEnabled", "???\nIf the explicit congestion notification is enabled.\n"},
  {"FastRetran", "The number of invocations of the Fast Retransmit algorithm.\n"},
  {"MaxCwnd", "???\nThe maximum congestion window, in octets.\n"},
  {"MaxMSS", "The maximum MSS, in octets.\n"},
  {"MaxRTO", "The maximum value of the retransmit timer RTO.\n"},
  {"MaxRTT", "The maximum sampled round trip time.\n"},
  {"MaxRwinRcvd", "The maximum window advertisement received, in octets.\n"},
  {"MaxRwinSent", "The maximum window advertisement sent, in octets.\n"},
  {"MaxSsthresh", "The maximum slow start threshold, excluding the initial\n"
                  "value.\n"},
  {"MinMSS", "The minimum MSS, in octets.\n"},
  {"MinRTO", "The minimum value of the retransmit timer RTO.\n"},
  {"MinRTT", "The minimum sampled round trip time.\n"},
  {"MinRwinRcvd", "???\nThe minimum window advertisement received, in octets.\n"},
  {"MinRwinSent", "???\nThe minimum window advertisement sent, in octets.\n"},
  {"NagleEnabled", "???\nIf the Nagle algorithm is enabled.\n"},
  {"OtherReductions", "The number of congestion window reductions made as a result\n"
                      "of anything other than AIMD congestion control algorithms.\n"
                      "Examples of non-multiplicative window reductions include\n"
                      "Congestion Window Validation [RFC2861] and experimental\n"
                      "algorithms such as Vegas.\n"},
  {"PktsIn", "???\nThe number of packets received.\n"},
  {"PktsOut", "???\nThe number of packets sent.\n"},
  {"PktsRetrans", "???\nThe number of retransmitted packets.\n"},
  {"RcvWinScale", "!!!\nThe value of the received window scale option if one was\n"
                  "received; otherwise, a value of -1.\n"},
  {"SACKEnabled", "???\nIf the SACK is enabled.\n"},
  {"SACKsRcvd", "The number of SACK options received.\n"},
  {"SendStall", "The number of interface stalls or other sender local\n"
                "resource limitations that are treated as congestion\n"
                "signals.\n"},
  {"SlowStart", "The number of times the congestion window has been\n"
                "increased by the Slow Start algorithm.\n"},
  {"SampleRTT", "The most recent raw round trip time measurement used in\n"
                "calculation of the RTO.\n"},
  {"SmoothedRTT", "The smoothed round trip time used in calculation of the\n"
                  "RTO. See SRTT in [RFC2988].\n"},
  {"SndWinScale", "!!!\nThe value of the transmitted window scale option if one was\n"
                  "sent; otherwise, a value of -1.\n"},
  {"SndLimTimeRwin", "The cumulative time spent in the 'Receiver Limited' state.\n"},
  {"SndLimTimeCwnd", "The cumulative time spent in the 'Congestion Limited' state.\n"},
  {"SndLimTimeSender", "!!!\nThe cumulative time spent in the 'Sender Limited' state.\n"},
  {"SndLimTransRwin", "The number of transitions into the 'Receiver Limited' state\n"
                      "from either the 'Congestion Limited' or 'Sender Limited'\n"
                      "states. This state is entered whenever TCP transmission\n"
                      "stops because the sender has filled the announced receiver\n"
                      "window.\n"},
  {"SndLimTransCwnd", "The number of transitions into the 'Congestion Limited'\n"
                      "state from either the 'Receiver Limited' or 'Sender\n"
                      "Limited' states. This state is entered whenever TCP\n"
                      "transmission stops because the sender has reached some\n"
                      "limit defined by congestion control (e.g. cwnd) or other\n"
                      "algorithms (retransmission timeouts) designed to control\n"
                      "network traffic.\n"},
  {"SndLimTransSender", "!!!\nThe number of transitions into the 'Sender Limited' state\n"
                        "from either the 'Receiver Limited' or 'Congestion Limited'\n"
                        "states. This state is entered whenever TCP transmission\n"
                        "stops due to some sender limit such as running out of\n"
                        "application data or other resources and the Karn algorithm.\n"
                        "When TCP stops sending data for any reason which can not be\n"
                        "classified as Receiver Limited or Congestion Limited it\n"
                        "MUST be treated as Sender Limited.\n"},
  {"SndLimBytesRwin", "???\n"},
  {"SndLimBytesCwnd", "???\n"},
  {"SndLimBytesSender", "???\n"},
  {"SubsequentTimeouts", "The number of times the retransmit timeout has expired\n"
                         "after the RTO has been doubled. See section 5.5 in RFC2988.\n"},
  {"SumRTT", "The sum of all sampled round trip times.\n"},
  {"Timeouts", "The number of times the retransmit timeout has expired when\n"
               "the RTO backoff multiplier is equal to one.\n"},
  {"TimestampsEnabled", "!!!\nEnabled(1) if TCP timestamps have been negotiated on,\n"
                        "selfDisabled(2) if they are disabled or not implemented on\n"
                        "the local host, or peerDisabled(3) if not negotiated by the\n"
                        "remote hosts.\n"},
  {"WinScaleRcvd", "The value of the received window scale option if one was\n"
                   "received; otherwise, a value of -1.\n"},
  {"WinScaleSent", "The value of the transmitted window scale option if one was\n"
                   "sent; otherwise, a value of -1.\n"},
  {"DupAcksOut", "The number of duplicate ACKs sent.\n"},
  {"StartTimeUsec", "!!!\nThe value of sysUpTime at the time this listener was\n"
                    "established.  If the current state was entered prior to\n"
                    "the last re-initialization of the local network management\n"
                    "subsystem, then this object contains a zero value.\n"},
  {"Duration", "???\nThe duration of the connection.\n"},
  {NULL, NULL}
};

#endif
