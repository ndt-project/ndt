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
  {"CurMSS", "The current maximum segment size (MSS), in octets.\n"},
  {"X_Rcvbuf", "The socket receive buffer size in octets.  Note that the\n"
               "meaning of this variable is implementation dependent.  In\n"
               "particular, it may or may not include the reassembly queue.\n"},
  {"X_Sndbuf", "The socket send buffer size in octets.  Note that the\n"
               "meaning of this variable is implementation dependent.\n"
               "Particularly, it may or may not include the retransmit\n"
               "queue.\n"},
  {"AckPktsIn", "The number of valid pure ack packets that have been\n"
                "received on this connection by the Local Host.\n"},
  {"AckPktsOut", "The number of pure ack packets that have been sent\n"
                 "on this connection by the Local Host.\n"},
  {"BytesRetrans", "The number of octets retransmitted.\n"},
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
  {"DataBytesIn", "The number of octets contained in received data segments,\n"
                  "including retransmitted data.  Note that this does not\n"
                  "include TCP headers.\n"},
  {"DataBytesOut", "The number of octets of data contained in transmitted segments,\n"
                   "including retransmitted data.  Note that this does not include\n"
                   "TCP headers.\n"},
  {"DataPktsIn", "The number of segments received containing a positive length\n"
                 "data segment.\n"},
  {"DataPktsOut", "The number of segments sent containing a positive length data\n"
                  "segment.\n"},
  {"DupAcksIn", "The number of duplicate ACKs received.\n"},
  {"ECNEnabled", "Enabled(1) if Explicit Congestion Notification (ECN)\n"
                 "has been negotiated on,\n"
                 "selfDisabled(2) if it is disabled or not implemented\n"
                 "on the local host, or\n"
                 "peerDisabled(3) if not negotiated by the remote hosts.\n"},
  {"FastRetran", "The number of invocations of the Fast Retransmit algorithm.\n"},
  {"MaxCwnd", "The maximum congestion window used during Slow Start, in octets.\n"},
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
  {"MinRwinRcvd", "The minimum window advertisement received, in octets.\n"},
  {"MinRwinSent", "The minimum window advertisement sent, excluding the initial\n"
                  "unscaled window advertised on the SYN, in octets.\n"},
  {"NagleEnabled", "True(1) if the Nagle algorithm is being used, else false(2).\n"},
  {"OtherReductions", "The number of congestion window reductions made as a result\n"
                      "of anything other than AIMD congestion control algorithms.\n"
                      "Examples of non-multiplicative window reductions include\n"
                      "Congestion Window Validation [RFC2861] and experimental\n"
                      "algorithms such as Vegas.\n"},
  {"PktsIn", "The total number of segments received.\n"},
  {"PktsOut", "The total number of segments sent.\n"},
  {"PktsRetrans", "The number of segments transmitted containing at least some\n"
                  "retransmitted data.\n"},
  {"RcvWinScale", "The value of Rcv.Wind.Scale.  Note that\n"
                  "RcvWinScale is either zero or the same as WinScaleSent.\n"},
  {"SACKEnabled", "True(1) if SACK has been negotiated on, else false(2).\n"},
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
  {"SndWinScale", "The value of Snd.Wind.Scale.  Note that\n"
                  "SndWinScale is either zero or the same as WinScaleRcvd.\n"},
  {"SndLimTimeRwin", "The cumulative time spent in the 'Receiver Limited' state.\n"},
  {"SndLimTimeCwnd", "The cumulative time spent in the 'Congestion Limited' state.\n"},
  {"SndLimTimeSender", "The cumulative time spent in the 'Sender Limited' state.\n"},
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
  {"SndLimTransSender", "The number of transitions into the 'Sender Limited' state\n"
                        "from either the 'Receiver Limited' or 'Congestion Limited'\n"
                        "states. This state is entered whenever TCP transmission\n"
                        "stops due to some sender limit such as running out of\n"
                        "application data or other resources and the Karn algorithm.\n"
                        "When TCP stops sending data for any reason which can not be\n"
                        "classified as Receiver Limited or Congestion Limited it\n"
                        "MUST be treated as Sender Limited.\n"},
  {"SndLimBytesRwin", "The cumulative bytes sent while in the 'Receiver Limited'\n"
                      "state, as determined by the Local Host, when the Local Host is\n"
                      "a sender.  This state is entered whenever TCP transmission\n"
                      "stops due to Receiver not being able to receive data.\n"},
  {"SndLimBytesCwnd", "The cumulative bytes sent while in the 'Congestion Limited'\n"
                      "state, as determined by the Local Host, when the Local Host is\n"
                      "a sender.  This state is entered whenever TCP transmission\n"
                      "stops due to congestion.\n"},
  {"SndLimBytesSender", "The cumulative bytes sent while in the 'Sender Limited'\n"
                        "state, as determined by the Local Host, when the Local Host is\n"
                        "a sender.  This state is entered whenever TCP transmission\n"
                        "stops because there is no more data in sender's buffer.\n"},
  {"SubsequentTimeouts", "The number of times the retransmit timeout has expired\n"
                         "after the RTO has been doubled. See section 5.5 in RFC2988.\n"},
  {"SumRTT", "The sum of all sampled round trip times.\n"},
  {"Timeouts", "The number of times the retransmit timeout has expired when\n"
               "the RTO backoff multiplier is equal to one.\n"},
  {"TimestampsEnabled", "Enabled(1) if TCP timestamps have been negotiated on,\n"
                        "selfDisabled(2) if they are disabled or not implemented on\n"
                        "the local host, or peerDisabled(3) if not negotiated by the\n"
                        "remote hosts.\n"},
  {"WinScaleRcvd", "The value of the received window scale option if one was\n"
                   "received; otherwise, a value of -1.\n"},
  {"WinScaleSent", "The value of the transmitted window scale option if one was\n"
                   "sent; otherwise, a value of -1.\n"},
  {"DupAcksOut", "The number of duplicate ACKs sent.\n"},
  {"StartTimeUsec", "The value of sysUpTime at the time this listener was\n"
                    "established.  If the current state was entered prior to\n"
                    "the last re-initialization of the local network management\n"
                    "subsystem, then this object contains a zero value.\n"},
  {"Duration", "The seconds part of the time elapsed between StartTimeStamp\n"
               "and the most recent protocol event (segment sent or received).\n"},
  {NULL, NULL}
};

#endif
