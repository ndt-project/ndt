/**
 * This file contains methods to interpret the results sent to the CLT from the server.
 *
 *  Created on: Oct 19, 2011
 *      Author: kkumar@internet2.edu
 *
 */

#include <string.h>
#include <stdio.h>

#include "logging.h"
#include "utils.h"
#include "clt_tests.h"
#include "test_results_clt.h"

#ifndef VIEW_DIFF
#define VIEW_DIFF 0.1
#endif

/** Get link speed based on data sent from server.
 * This data is an integral value from among a set indicative of link speeds.
 * @param c2s_linkspeed_ind
 * @return Current link's speed
 */
double get_linkspeed(int c2s_linkspeed_ind, int half_duplex_ind) {
  double mylinkspeed = 0;
  if (c2s_linkspeed_ind < DATA_RATE_ETHERNET) {
    if (c2s_linkspeed_ind < DATA_RATE_RTT) {
      printf("Server unable to determine bottleneck link type.\n");
    } else {
      printf("Your host is connected to a ");

      if (c2s_linkspeed_ind == DATA_RATE_DIAL_UP) {
        printf("Dial-up Modem\n");
        mylinkspeed = .064;
      } else {
        printf("Cable/DSL modem\n");
        mylinkspeed = 2;
      }
    }
  } else {
    printf("The slowest link in the end-to-end path is a ");

    if (c2s_linkspeed_ind == DATA_RATE_ETHERNET) {
      printf("10 Mbps Ethernet or WiFi 11b subnet\n");
      mylinkspeed = 10;
    } else if (c2s_linkspeed_ind == DATA_RATE_T3) {
      printf("45 Mbps T3/DS3 or WiFi 11 a/g  subnet\n");
      mylinkspeed = 45;
    } else if (c2s_linkspeed_ind == DATA_RATE_FAST_ETHERNET) {
      printf("100 Mbps ");
      mylinkspeed = 100;
      if (half_duplex_ind == NO_HALF_DUPLEX) {
        printf("Full duplex Fast Ethernet subnet\n");
      } else {
        printf("Half duplex Fast Ethernet subnet\n");
      }
    } else if (c2s_linkspeed_ind == DATA_RATE_OC_12) {
      printf("a 622 Mbps OC-12 subnet\n");
      mylinkspeed = 622;
    } else if (c2s_linkspeed_ind == DATA_RATE_GIGABIT_ETHERNET) {
      printf("1.0 Gbps Gigabit Ethernet subnet\n");
      mylinkspeed = 1000;
    } else if (c2s_linkspeed_ind == DATA_RATE_OC_48) {
      printf("2.4 Gbps OC-48 subnet\n");
      mylinkspeed = 2400;
    } else if (c2s_linkspeed_ind == DATA_RATE_10G_ETHERNET) {
      printf("10 Gbps 10 Gigabit Ethernet/OC-192 subnet\n");
      mylinkspeed = 10000;
    }
  }
  return mylinkspeed;
}

/**
 * Print results of the mismatch detection
 * @param mismatch Integer indicating results
 */
void print_results_mismatchcheck(int mismatch) {
  switch (mismatch) {
    case DUPLEX_OLD_ALGO_INDICATOR :
      printf("Warning: Old Duplex-Mismatch condition detected.\n");
      break;

    case DUPLEX_SWITCH_FULL_HOST_HALF :
      printf("Alarm: Duplex Mismatch condition detected. Switch=Full and "
             "Host=Half\n");
      break;
    case DUPLEX_SWITCH_FULL_HOST_HALF_POSS:
      printf("Alarm: Possible Duplex Mismatch condition detected. Switch=Full "
             "and Host=Half\n");
      break;
    case DUPLEX_SWITCH_HALF_HOST_FULL:
      printf("Alarm: Duplex Mismatch condition detected. Switch=Half and "
             "Host=Full\n");
      break;
    case DUPLEX_SWITCH_HALF_HOST_FULL_POSS:
      printf("Alarm: Possible Duplex Mismatch condition detected. Switch=Half "
             "and Host=Full\n");
      break;
    case DUPLEX_SWITCH_HALF_HOST_FULL_WARN:
      printf("Warning: Possible Duplex Mismatch condition detected. "
             "Switch=Half and Host=Full\n");
      break;
  }
}


/**
 * Calculate and display recommended buffer sizes based on
 * receive window size, the max Receive window advertisement
 * received, link speed and round trip time.
 * @param rwin
 * @param rttsec
 * @param avgrtt
 * @param mylink
 * @param max_RwinRcvd
 */
void print_recommend_buffersize(double rwin, double rttsec, double avgrtt,
                                double mylink, int max_RwinRcvd) {
  float j = 0;
  log_print(3, "Is larger buffer recommended?  rwin*2/rttsec (%0.4f) < mylink "
            "(%0.4f) ", ((rwin*2)/rttsec), mylink);
  log_println(3, "AND j (%0.4f) > MaxRwinRcvd (%d)",
              (float)((mylink * avgrtt)*1000)/8, max_RwinRcvd);
  if (((rwin*2)/rttsec) < mylink) {
    j = (float)((mylink * avgrtt)*1000) / 8;
    if ((int)j > max_RwinRcvd) {
      printf("Information: The receive buffer should be %0.0f ", j/1024.0f);
      printf("kbytes to maximize throughput\n");
    }
  }
}

/**
 * Check if faulty hardware was detected, and print results
 * @param is_badcable
 * */
void check_badcable(int is_bad_cable) {
  if (is_bad_cable == POSSIBLE_BAD_CABLE) {
    printf("Alarm: Excessive errors, check network cable(s).\n");
  }
}

/**
 * Check if congestion was detected, and print results
 * @param is_congested
 * */
void check_congestion(int is_congested) {
  if (is_congested == POSSIBLE_CONGESTION) {
    printf("Information: Other network traffic is congesting the link\n");
  }
}

/**
 * Check if packet queuing is a problem at the C->S end.
 * @param c2sthruput The C->S throughput as calculated by the server in the C->S test
 * @param spdout C->S throughput as calculated by the Client during the C->S test
 * @param sndqueue length of send queue of the C->S throughput test
 */
void check_C2Spacketqueuing(double c2sthruput, double spdout, int sndqueue,
                            int pktcount, int buflength) {
  if (c2sthruput < (spdout * (1.0 - VIEW_DIFF))) {
    printf("Information [C2S]: Packet queuing detected: %0.2f%% ",
           100 * (spdout - c2sthruput) / spdout);
    if (sndqueue >
        (0.8 * pktcount * buflength * (spdout - c2sthruput) / spdout)) {
      printf("(local buffers)\n");
    } else {
      printf("(remote buffers)\n");
    }
  }
}


/**
 * Deduce if packet queuing is a problem at the S->C end from values collected
 *  during the S2C tests.
 * @param s2cthroughput The S->C throughput as calculated by the client
 * @param spdin S->C throughput as calculated by the Client during the C->S test
 * @param sndqueue length of send queue of the S->C throughput test
 * @param sbytecount total-sent-byte-count in the S->C throughput test
 */
void check_S2Cpacketqueuing(double s2cthroughput, double spdin,
                            int srvsndqueue, int sbytecount) {
  if (spdin < (s2cthroughput * (1.0 - VIEW_DIFF))) {
    printf("Information [S2C]: Packet queuing detected: %0.2f%% ",
           100 * (s2cthroughput - spdin) / s2cthroughput);
    if (srvsndqueue >
        (0.8 * sbytecount * (s2cthroughput - spdin) / s2cthroughput)) {
      printf("(local buffers)\n");
    } else {
      printf("(remote buffers)\n");
    }
  }
}

/**
 *  Print suggestions based on packet loss/retransmissions details obtained
 *
 * If there were packets retransmitted, print details about :
 * - whether packets arrived out of order
 * -# of times the retransmit timeout expired
 * -total time spent waiting
 *
 * If there were no packet retransmissions, but duplicate
 * acknowledgments were received, print details about:
 * - whether packets arrived out of order
 *
 * @param PktsRetrans Number of packet retransmissions
 * @param DupAcksIn   Number of incoming duplicate acknowledgments
 * @param SACKsRcvd   number of SACK options
 * @param ooorder     out-of-order packets
 * @param Timeouts	  number of times retransmit timer expired
 * @param waitsec     total idle time
 * @param totaltesttime     Total test time
 */
void print_packetloss_statistics(int PktsRetrans, int DupAcksIn, int SACKsRcvd,
                                 double ooorder, int Timeouts, double waitsec,
                                 double totaltesttime) {
  if (PktsRetrans > 0) {
    printf("There were %d packets retransmitted", PktsRetrans);
    printf(", %d duplicate acks received", DupAcksIn);
    printf(", and %d SACK blocks received\n", SACKsRcvd);
    if (ooorder > 0)
      printf("Packets arrived out-of-order %0.2f%% of the time.\n",
             ooorder * 100);
    if (Timeouts > 0)
      printf("The connection stalled %d times due to packet loss.\n",
             Timeouts);
    if (waitsec > 0)
      printf("The connection was idle %0.2f seconds (%0.2f%%) of the time.\n",
             waitsec, (100 * waitsec / totaltesttime));
  } else if (DupAcksIn > 0) {
    printf("No packet loss - ");
    if (ooorder > 0)
      printf("but packets arrived out-of-order %0.2f%% of the time.\n",
             ooorder * 100);
    else
      printf("\n");
  } else {
    printf("No packet loss was observed.\n");
  }
}



/**
 * Print details about being receiver, sender or congestion window limited.
 * @param rwintime  time spent limited due to receive window limitations
 * @param rwin		receive window size
 * @param sendtime  time spent limited due to send window limitations
 * @param swin		send window size
 * @param cwndtime  time spent limited due to congestion window limitations
 * @param rttsec	round-trip time in sec
 * @param mylinkspeed Link speed
 * @param sndbuf	send buffer size
 * @param max_rwinrcvd MaxRwinRcvd value
 */

void print_limitedtime_ratio(double rwintime, double rwin, double sendtime,
                             double swin, double cwndtime, double rttsec,
                             double mylinkspeed, int sndbuf, int max_rwinrcvd) {
  if (rwintime > .015) {
    printf("This connection is receiver limited %0.2f%% of the time.\n",
           rwintime * 100);
    if ((2 * (rwin / rttsec)) < mylinkspeed)
      printf("  Increasing the current receive buffer (%0.2f KB) will improve "
             "performance\n", (float) max_rwinrcvd / KILO_BITS);
  }
  if (sendtime > .015) {
    printf("This connection is sender limited %0.2f%% of the time.\n",
           sendtime * 100);
    if ((2 * (swin / rttsec)) < mylinkspeed)
      printf("  Increasing the current send buffer (%0.2f KB) will improve "
             "performance\n", (float) sndbuf / KILO_BITS);
  }
  if (cwndtime > .005) {
    printf("This connection is network limited %0.2f%% of the time.\n",
           cwndtime * 100);
  }
}

/**
 * Check if excessive packet loss is affecting performance
 * @param spd throughput indicator
 * @param loss packet loss
 */
void print_packetloss_excess(double spd, double loss) {
  if ((spd < 4) && (loss > .01)) {
    printf("Excessive packet loss is impacting your performance, check the ");
    printf("auto-negotiate function on your local PC and network switch\n");
  }
}

/**
 * Print if TCP Selective Acknowledgment Options based on RFC 2018 is on
 *
 * @param SACKEnabled
 */
void print_SAck_RFC2018(int SACKEnabled) {
  printf("RFC 2018 Selective Acknowledgment: ");
  if (SACKEnabled == 0)
    printf("OFF\n");
  else
    printf("ON\n");
}

/**
 * Print if Nagle algorithm (rfc 896) is on
 * @param is_nagleenabled
 */

void print_Nagle_RFC896(int is_nagleenabled) {
  printf("RFC 896 Nagle Algorithm: ");
  if (is_nagleenabled == 0)
    printf("OFF\n");
  else
    printf("ON\n");
}

/**
 * Print if Explicit congestion notification to IP is on - RFC 3168 related
 * @param is_ECNenabled
 */
void print_congestion_RFC3168(int is_ECNenabled) {
  printf("RFC 3168 Explicit Congestion Notification: ");
  if (is_ECNenabled == 0)
    printf("OFF\n");
  else
    printf("ON\n");
}

/**
 * Print details of whether time stamping is on - RFC 1323 related.
 *
 * @param is_timestampenabled
 */
void print_timestamping_RFC1323(int is_timestampenabled) {
  printf("RFC 1323 Time Stamping: ");
  if (is_timestampenabled == 0)
    printf("OFF\n");
  else
    printf("ON\n");
}

/**
 * Print window scaling based data
 * @param max_rwinrcvd 	MaxRwinRcvd web100 var value
 * @param winscale_rcvd value of the received window scale option
 * @param winscale_sent value of the transmitted window scale option
 */
void print_windowscaling(int max_rwinrcvd, int winscale_rcvd,
                         int winscale_sent) {
  printf("RFC 1323 Window Scaling: ");
  if (max_rwinrcvd < 65535)
    winscale_rcvd = 0;

  if ((winscale_rcvd == 0) || (winscale_rcvd > 20))
    printf("OFF\n");
  else
    printf("ON; Scaling Factors - Server=%d, Client=%d\n",
           winscale_rcvd, winscale_sent);
}

/**
 * Print details of throughput limits(thresholds) imposed by
 * 		the send, receive or congestion window values
 * @param max_rwinrcvd 	MaxRwinRcvd web100 var value
 * @param rcvwinscale Value of receive window scale
 * @param sndbuf send buffer size
 * @param rwin   Receive window value in bytes
 * @param swin   Send window value in bytes
 * @param cwin   congestion window value in bytes
 * @param rttsec total round-trip time
 * @param estimate Estimated theoretical throughput
 */
void print_throughputlimits(int max_rwinrcvd, int rcvwinscale, int *sndbuf,
                            double s_win, double r_win, double c_win,
                            double rttsec, double estimate) {
  int tempsendbuf = *sndbuf;
  if ((rcvwinscale == 0) && (tempsendbuf > 65535))
    tempsendbuf = 65535;

  printf("The theoretical network limit is %0.2f Mbps\n", estimate);

  printf("The NDT server has a %0.0f KByte buffer which limits the throughput "
         "to %0.2f Mbps\n", (float) tempsendbuf / KILO_BITS,
         (float) s_win / rttsec);

  printf("Your PC/Workstation has a %0.0f KByte buffer which limits the "
         "throughput to %0.2f Mbps\n", (float) max_rwinrcvd / KILO_BITS,
         (float) r_win / rttsec);

  printf("The network based flow control limits the throughput to %0.2f Mbps\n",
         (float) c_win / rttsec);
}


/**
 * Print details of link speed as seen by data and ack speed indicators
 *
 * @param isC2S_enabled Has C2S test been performed?
 * @param c2s_linkspeed_data Data link speed as detected by server data
 * @param c2s_linkspeed_ack Data link speed(type) as detected by server acknowledgments
 * @param s2c_linkspeed_data Data link speed as detected by client data
 * @param s2c_linkspeed_ack Data link speed as detected by client acknowledgments
 */
void print_linkspeed_dataacks(int isC2S_enabled, int c2s_linkspeed_data,
                              int c2s_linkspeed_ack, int s2c_linkspeed_data,
                              int s2c_linkspeed_ack) {
  if (isC2S_enabled) {
    printf("\nClient Data reports link is '%3d', Client Acks report link is "
           "'%3d'\n", c2s_linkspeed_data, c2s_linkspeed_ack);
  }
  printf("Server Data reports link is '%3d', Server Acks report link is "
         "'%3d'\n", s2c_linkspeed_data, s2c_linkspeed_ack);
}

/**
 * A Network Address Translation (NAT) box is detected by
 *  comparing the client/server IP addresses as seen from the server and the client boxes.
 * @param ssip Server side IP address string
 * @param csip Client side sersver IP address string
 * @param scip Server side client IP address string
 * @param ccip Client side client IP address string
 */
void check_NAT(char *ssip, char *csip, char *scip, char *ccip) {
  // If server and client both see similar server IP addresses,
  // no NAT happening
  if (strcmp(ssip, csip) == 0) {
    printf("Server IP addresses are preserved End-to-End\n");
  } else {
    printf("Information: Network Address Translation (NAT) box is ");
    printf("modifying the Server's IP address\n");
    printf("\tServer says [%s] but Client says [ %s]\n", ssip, csip);
  }

  // If server and client both see similar client IP addresses,
  // no NAT happening
  if (strcmp(scip, ccip) == 0) {
    printf("Client IP addresses are preserved End-to-End\n");
  } else {
    printf("Information: Network Address Translation (NAT) box is ");
    printf("modifying the Client's IP address\n");
    printf("\tServer says [%s] but Client says [ %s]\n", scip, ccip);
  }
}

/**
 * Check packet size preservation by comparing the final value
 *  of the MSS variable in the Middlebox test.
 *
 * Note that the server sets the MSS value to 1456 on the listening socket
 *   before the NDT Client connects to it; the final value of the MSS
 *   variable is read after the NDT Client connects.
 * @param is_timestampenabled
 * @param mssvalue
 */
void check_MSS_modification(int is_timestampenabled, int *mssvalue) {
  if (is_timestampenabled == 1)
    *mssvalue += 12;
  if (*mssvalue == 1456)
    printf("Packet size is preserved End-to-End\n");
  else
    printf("Information: Network Middlebox is modifying MSS variable "
           "(changed to %d)\n", *mssvalue);
}
