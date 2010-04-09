#include "../config.h"

#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>

#include "network.h"
#include "usage.h"
#include "logging.h"
#include "varinfo.h"
#include "utils.h"
#include "protocol.h"
#include "test_sfw.h"
#include "clt_tests.h"

#ifndef VIEW_DIFF
#define VIEW_DIFF 0.1
#endif

extern int h_errno;

int Randomize, failed, cancopy;
int ECNEnabled, NagleEnabled, MSSSent, MSSRcvd;
int SACKEnabled, TimestampsEnabled, WinScaleRcvd, WinScaleSent;
int FastRetran, AckPktsOut, SmoothedRTT, CurrentCwnd, MaxCwnd;
int SndLimTimeRwin, SndLimTimeCwnd, SndLimTimeSender;
int SndLimTransRwin, SndLimTransCwnd, SndLimTransSender, MaxSsthresh;
int SumRTT, CountRTT, CurrentMSS, Timeouts, PktsRetrans;
int SACKsRcvd, DupAcksIn, MaxRwinRcvd, MaxRwinSent;
int DataPktsOut, Rcvbuf, Sndbuf, AckPktsIn, DataBytesOut;
int PktsOut, CongestionSignals, RcvWinScale;
int pkts, lth=8192, CurrentRTO;
int c2sData, c2sAck, s2cData, s2cAck;
int winssent, winsrecv, msglvl=0;
int sndqueue, ssndqueue, sbytes;
int minPeak, maxPeak, peaks;
double spdin, spdout, c2sspd, s2cspd;
double aspd;

int half_duplex, congestion, bad_cable, mismatch;
double loss, estimate, avgrtt, spd, waitsec, timesec, rttsec;
double order, rwintime, sendtime, cwndtime, rwin, swin, cwin;
double mylink;

static struct option long_options[] = {
  {"name", 1, 0, 'n'},
  {"port", 1, 0, 'p'},
  {"debug", 0, 0, 'd'},
  {"help", 0, 0, 'h'},
  {"msglvl", 0, 0, 'l'},
  {"web100variables", 0, 0, 301},
  {"buffer", 1, 0, 'b'},
  {"disablemid", 0, 0, 302},
  {"disablec2s", 0, 0, 303},
  {"disables2c", 0, 0, 304},
  {"disablesfw", 0, 0, 305},
#ifdef AF_INET6
  {"ipv4", 0, 0, '4'},
  {"ipv6", 0, 0, '6'},
#endif
  {0, 0, 0, 0}
};

void save_int_values(char *sysvar, int sysval);
void save_dbl_values(char *sysvar, float *sysval);
    
void
printVariables(char *tmpstr)
{
  int i, j, k;
  char sysvar[128], sysval[128];

  k = strlen(tmpstr) - 1;
  i = 0;
  for (;;) {
    for (j=0; tmpstr[i]!=' '; j++)
      sysvar[j] = tmpstr[i++];
    sysvar[j] = '\0';
    i++;
    for (j=0; tmpstr[i]!='\n'; j++)
      sysval[j] = tmpstr[i++];
    sysval[j] = '\0';
    i++;
    printf("%s %s\n", sysvar, sysval);
    if (i >= k)
      return;
  }
}

void
printWeb100VarInfo()
{
  int i = 0;
  printf(" --- Detailed description of the Web100 variables ---\n\n");

  while (web100vartable[i][0])
  {
    printf("* %s\n    %s\n", web100vartable[i][0], web100vartable[i][1]);
    ++i;
  }
}

void
testResults(char tests, char *tmpstr, char* host)
{
  int i=0, Zero=0;
  char *sysvar, *sysval;
  float j;

  if (!(tests & TEST_S2C)) {
    if (tests & TEST_C2S) {
      if (c2sspd < (spdout  * (1.0 - VIEW_DIFF))) {
        printf("Information [C2S]: Packet queuing detected: %0.2f%% ", 100 * (spdout - c2sspd) / spdout);
        if (sndqueue > (0.8 * pkts * lth * (spdout - c2sspd) / spdout)) {
          printf("(local buffers)\n");
        }
        else {
          printf("(remote buffers)\n");
        }
      }
    }
    
    results_sfw(tests, host);
    return;
  }

  sysvar = strtok(tmpstr, " ");
  sysval = strtok(NULL, "\n");
  i = atoi(sysval);
  save_int_values(sysvar, i);

  for(;;) {
    sysvar = strtok(NULL, " ");
    if (sysvar == NULL)
      break;
    sysval = strtok(NULL, "\n");
    if (strchr(sysval, '.') == NULL) {
      i = atoi(sysval);
      save_int_values(sysvar, i);
      log_println(7, "Stored %d [%s] in %s", i, sysval, sysvar);
    }
    else {
      j = atof(sysval);
      save_dbl_values(sysvar, &j);
      log_println(7, "Stored %0.2f (%s) in %s", j, sysval, sysvar);
    }
  }

  if(CountRTT > 0) {

    if (tests & TEST_C2S) {
      if (c2sData < 3) {
        if (c2sData < 0) {
          printf("Server unable to determine bottleneck link type.\n");
        } else {
          printf("Your host is connected to a ");
          if (c2sData == 1) {
            printf("Dial-up Modem\n");
            mylink = .064;
          } else {
            printf("Cable/DSL modem\n");
            mylink = 2;
          }
        }
      } else {
        printf("The slowest link in the end-to-end path is a ");
        if (c2sData == 3) {
          printf("10 Mbps Ethernet or WiFi 11b subnet\n");
          mylink = 10;
        } else if (c2sData == 4) {
          printf("45 Mbps T3/DS3 or WiFi 11 a/g  subnet\n");
          mylink = 45;
        } else if (c2sData == 5) {
          printf("100 Mbps ");
          mylink = 100;
          if (half_duplex == 0) {
            printf("Full duplex Fast Ethernet subnet\n");
          } else {
            printf("Half duplex Fast Ethernet subnet\n");
          }
        } else if (c2sData == 6) {
          printf("a 622 Mbps OC-12 subnet\n");
          mylink = 622;
        } else if (c2sData == 7) {
          printf("1.0 Gbps Gigabit Ethernet subnet\n");
          mylink = 1000;
        } else if (c2sData == 8) {
          printf("2.4 Gbps OC-48 subnet\n");
          mylink = 2400;
        } else if (c2sData == 9) {
          printf("10 Gbps 10 Gigabit Ethernet/OC-192 subnet\n");
          mylink = 10000;
        }
      }
    }

    switch(mismatch) {
      case 1 : printf("Warning: Old Duplex-Mismatch condition detected.\n");
               break;

      case 2 : printf("Alarm: Duplex Mismatch condition detected. Switch=Full and Host=Half\n");
               break;
      case 4 : printf("Alarm: Possible Duplex Mismatch condition detected. Switch=Full and Host=Half\n");
               break;

      case 3 : printf("Alarm: Duplex Mismatch condition detected. Switch=Half and Host=Full\n");
               break;
      case 5 : printf("Alarm: Possible Duplex Mismatch condition detected. Switch=Half and Host=Full\n");
               break;
      case 7 : printf("Warning: Possible Duplex Mismatch condition detected. Switch=Half and Host=Full\n");
               break;
    }


    if (mismatch == 0) {
      if (bad_cable == 1) {
        printf("Alarm: Excessive errors, check network cable(s).\n");
      }
      if (congestion == 1) {
        printf("Information: Other network traffic is congesting the link\n");
      }
      log_print(3, "Is larger buffer recommended?  rwin*2/rttsec (%0.4f) < mylink (%0.4f) ", ((rwin*2)/rttsec), mylink);
      log_println(3, "AND j (%0.4f) > MaxRwinRcvd (%d)", (float)((mylink * avgrtt)*1000)/8, MaxRwinRcvd);
      if (((rwin*2)/rttsec) < mylink) {
        j = (float)((mylink * avgrtt)*1000) / 8;
        if ((int)j > MaxRwinRcvd) {
          printf("Information: The receive buffer should be %0.0f ", j/1024);
          printf("kbytes to maximize throughput\n");
        }
      }
    }

    if (tests & TEST_C2S) {
      if (c2sspd < (spdout  * (1.0 - VIEW_DIFF))) {
        printf("Information [C2S]: Packet queuing detected: %0.2f%% ", 100 * (spdout - c2sspd) / spdout);
        if (sndqueue > (0.8 * pkts * lth * (spdout - c2sspd) / spdout)) {
          printf("(local buffers)\n");
        }
        else {
          printf("(remote buffers)\n");
        }
      }
    }
    if (tests & TEST_S2C) {
      if (spdin < (s2cspd  * (1.0 - VIEW_DIFF))) {
        printf("Information [S2C]: Packet queuing detected: %0.2f%% ", 100 * (s2cspd - spdin) / s2cspd);
        if (ssndqueue > (0.8 * sbytes * (s2cspd - spdin) / s2cspd)) {
          printf("(local buffers)\n");
        }
        else {
          printf("(remote buffers)\n");
        }
      }
    }
    
    results_sfw(tests, host);

    if (msglvl > 0) {
      printf("\n\t------  Web100 Detailed Analysis  ------\n");

      printf("\nWeb100 reports the Round trip time = %0.2f msec;", avgrtt);

      printf("the Packet size = %d Bytes; and \n", CurrentMSS);
      if (PktsRetrans > 0) {
        printf("There were %d packets retransmitted", PktsRetrans);
        printf(", %d duplicate acks received", DupAcksIn);
        printf(", and %d SACK blocks received\n", SACKsRcvd);
        if (order > 0)
          printf("Packets arrived out-of-order %0.2f%% of the time.\n", order*100);
        if (Timeouts > 0)
          printf("The connection stalled %d times due to packet loss.\n", Timeouts);
        if (waitsec > 0)
          printf("The connection was idle %0.2f seconds (%0.2f%%) of the time.\n",
              waitsec, (100*waitsec/timesec));
      } else if (DupAcksIn > 0) {
        printf("No packet loss - ");
        if (order > 0)
          printf("but packets arrived out-of-order %0.2f%% of the time.\n", order*100);
        else
          printf("\n");
      } else {
        printf("No packet loss was observed.\n");
      }

      if (rwintime > .015) {
        printf("This connection is receiver limited %0.2f%% of the time.\n", rwintime*100);
        if ((2*(rwin/rttsec)) < mylink)
          printf("  Increasing the current receive buffer (%0.2f KB) will improve performance\n",
              (float) MaxRwinRcvd/1024);
      }
      if (sendtime > .015) {
        printf("This connection is sender limited %0.2f%% of the time.\n", sendtime*100);
        if ((2*(swin/rttsec)) < mylink)
          printf("  Increasing the current send buffer (%0.2f KB) will improve performance\n",
              (float) Sndbuf/1024);
      }
      if (cwndtime > .005) {
        printf("This connection is network limited %0.2f%% of the time.\n", cwndtime*100);
      }
      if ((spd < 4) && (loss > .01)) {
        printf("Excessive packet loss is impacting your performance, check the ");
        printf("auto-negotiate function on your local PC and network switch\n");
      }
      printf("\n    Web100 reports TCP negotiated the optional Performance Settings to: \n");
      printf("RFC 2018 Selective Acknowledgment: ");
      if(SACKEnabled == Zero)
        printf("OFF\n");
      else
        printf("ON\n");

      printf("RFC 896 Nagle Algorithm: ");
      if(NagleEnabled == Zero)
        printf("OFF\n");
      else
        printf("ON\n");

      printf("RFC 3168 Explicit Congestion Notification: ");
      if(ECNEnabled == Zero)
        printf("OFF\n");
      else
        printf("ON\n");

      printf("RFC 1323 Time Stamping: ");
      if(TimestampsEnabled == 0)
        printf("OFF\n");
      else
        printf("ON\n");

      printf("RFC 1323 Window Scaling: ");
      if (MaxRwinRcvd < 65535)
        WinScaleRcvd = 0;

      if((WinScaleRcvd == 0) || (WinScaleRcvd > 20))
        printf("OFF\n");
      else
        printf("ON; Scaling Factors - Server=%d, Client=%d\n",
            WinScaleRcvd, WinScaleSent);

      if ((RcvWinScale == 0) && (Sndbuf > 65535))
        Sndbuf = 65535;

      printf("The theoretical network limit is %0.2f Mbps\n", estimate);

      printf("The NDT server has a %0.0f KByte buffer which limits the throughput to %0.2f Mbps\n",
          (float)Sndbuf/1024, (float)swin/rttsec);

      printf("Your PC/Workstation has a %0.0f KByte buffer which limits the throughput to %0.2f Mbps\n",
          (float)MaxRwinRcvd/1024, (float)rwin/rttsec);

      printf("The network based flow control limits the throughput to %0.2f Mbps\n",
          (float)cwin/rttsec);

      if (tests & TEST_C2S) {
        printf("\nClient Data reports link is '%3d', Client Acks report link is '%3d'\n",
            c2sData, c2sAck);
      }
      printf("Server Data reports link is '%3d', Server Acks report link is '%3d'\n", 
          s2cData, s2cAck);
    }
  } else {
    printf("No Web100 data collected!  Possible Duplex Mismatch condition caused ");
    printf("Server to client test to run long.\nCheck for host=Full and switch=Half ");
    printf("mismatch condition\n");
  }
}

/* this routine decodes the middlebox test results.  The data is returned
 * from the server in a specific order.  This routine pulls the string apart
 * and puts the values into the proper variable.  It then compares the values
 * to known values and writes out the specific results.
 *
 * server data is first
 * order is Server IP; Client IP; CurrentMSS; WinScaleRcvd; WinScaleSent;
 * Client then adds
 * Server IP; Client IP.
 */

void
middleboxResults(char *tmpstr, I2Addr local_addr, I2Addr peer_addr)
{
  char ssip[64], scip[64], *str;
  char csip[64], ccip[64];
  int mss;
  size_t tmpLen;

  str = strtok(tmpstr, ";");
  strcpy(ssip, str);
  str = strtok(NULL, ";");
  strcpy(scip, str);

  str = strtok(NULL, ";");
  mss = atoi(str);
  str = strtok(NULL, ";");
  winsrecv = atoi(str);
  str = strtok(NULL, ";");
  winssent = atoi(str);

  memset(ccip, 0, 64);
  tmpLen = 63;
  I2AddrNodeName(local_addr, ccip, &tmpLen);
  memset(csip, 0, 64);
  tmpLen = 63;
  I2AddrNodeName(peer_addr, csip, &tmpLen);

  if (TimestampsEnabled == 1)
    mss += 12;
  if (mss == 1456)
    printf("Packet size is preserved End-to-End\n");
  else
    printf("Information: Network Middlebox is modifying MSS variable (changed to %d)\n", mss);

  if (strcmp(ssip, csip) == 0)
    printf("Server IP addresses are preserved End-to-End\n");
  else {
    printf("Information: Network Address Translation (NAT) box is "); 
    printf("modifying the Server's IP address\n");
    printf("\tServer says [%s] but Client says [ %s]\n", ssip, csip);
  }

  if (strcmp(scip, ccip) == 0)
    printf("Client IP addresses are preserved End-to-End\n");
  else {
    printf("Information: Network Address Translation (NAT) box is "); 
    printf("modifying the Client's IP address\n");
    printf("\tServer says [%s] but Client says [ %s]\n", scip, ccip);
  }
}

void
save_int_values(char *sysvar, int sysval)
{
  /*  Values saved for interpretation:
   *	SumRTT 
   *	CountRTT
   *	CurrentMSS
   *	Timeouts
   *	PktsRetrans
   *	SACKsRcvd
   *	DupAcksIn
   *	MaxRwinRcvd
   *	MaxRwinSent
   *	Sndbuf
   *	Rcvbuf
   *	DataPktsOut
   *	SndLimTimeRwin
   *	SndLimTimeCwnd
   *	SndLimTimeSender
   */   

  log_println(6, "save_int_values(%s, %d)", sysvar, sysval);

  if(strcmp("MSSSent:", sysvar) == 0) 
    MSSSent = sysval;
  else if(strcmp("MSSRcvd:", sysvar) == 0) 
    MSSRcvd = sysval;
  else if(strcmp("ECNEnabled:", sysvar) == 0) 
    ECNEnabled = sysval;
  else if(strcmp("NagleEnabled:", sysvar) == 0) 
    NagleEnabled = sysval;
  else if(strcmp("SACKEnabled:", sysvar) == 0) 
    SACKEnabled = sysval;
  else if(strcmp("TimestampsEnabled:", sysvar) == 0) 
    TimestampsEnabled = sysval;
  else if(strcmp("WinScaleRcvd:", sysvar) == 0) 
    WinScaleRcvd = sysval;
  else if(strcmp("WinScaleSent:", sysvar) == 0) 
    WinScaleSent = sysval;
  else if(strcmp("SumRTT:", sysvar) == 0) 
    SumRTT = sysval;
  else if(strcmp("CountRTT:", sysvar) == 0) 
    CountRTT = sysval;
  else if(strcmp("CurMSS:", sysvar) == 0)
    CurrentMSS = sysval;
  else if(strcmp("Timeouts:", sysvar) == 0) 
    Timeouts = sysval;
  else if(strcmp("PktsRetrans:", sysvar) == 0) 
    PktsRetrans = sysval;
  else if(strcmp("SACKsRcvd:", sysvar) == 0) 
    SACKsRcvd = sysval;
  else if(strcmp("DupAcksIn:", sysvar) == 0) 
    DupAcksIn = sysval;
  else if(strcmp("MaxRwinRcvd:", sysvar) == 0) 
    MaxRwinRcvd = sysval;
  else if(strcmp("MaxRwinSent:", sysvar) == 0) 
    MaxRwinSent = sysval;
  else if(strcmp("Sndbuf:", sysvar) == 0) 
    Sndbuf = sysval;
  else if(strcmp("X_Rcvbuf:", sysvar) == 0) 
    Rcvbuf = sysval;
  else if(strcmp("DataPktsOut:", sysvar) == 0) 
    DataPktsOut = sysval;
  else if(strcmp("FastRetran:", sysvar) == 0) 
    FastRetran = sysval;
  else if(strcmp("AckPktsOut:", sysvar) == 0) 
    AckPktsOut = sysval;
  else if(strcmp("SmoothedRTT:", sysvar) == 0) 
    SmoothedRTT = sysval;
  else if(strcmp("CurCwnd:", sysvar) == 0) 
    CurrentCwnd = sysval;
  else if(strcmp("MaxCwnd:", sysvar) == 0) 
    MaxCwnd = sysval;
  else if(strcmp("SndLimTimeRwin:", sysvar) == 0) 
    SndLimTimeRwin = sysval;
  else if(strcmp("SndLimTimeCwnd:", sysvar) == 0) 
    SndLimTimeCwnd = sysval;
  else if(strcmp("SndLimTimeSender:", sysvar) == 0) 
    SndLimTimeSender = sysval;
  else if(strcmp("DataBytesOut:", sysvar) == 0) 
    DataBytesOut = sysval;
  else if(strcmp("AckPktsIn:", sysvar) == 0) 
    AckPktsIn = sysval;
  else if(strcmp("SndLimTransRwin:", sysvar) == 0)
    SndLimTransRwin = sysval;
  else if(strcmp("SndLimTransCwnd:", sysvar) == 0)
    SndLimTransCwnd = sysval;
  else if(strcmp("SndLimTransSender:", sysvar) == 0)
    SndLimTransSender = sysval;
  else if(strcmp("MaxSsthresh:", sysvar) == 0)
    MaxSsthresh = sysval;
  else if(strcmp("CurRTO:", sysvar) == 0)
    CurrentRTO = sysval;
  else if(strcmp("c2sData:", sysvar) == 0)
    c2sData = sysval;
  else if(strcmp("c2sAck:", sysvar) == 0)
    c2sAck = sysval;
  else if(strcmp("s2cData:", sysvar) == 0)
    s2cData = sysval;
  else if(strcmp("s2cAck:", sysvar) == 0)
    s2cAck = sysval;
  else if(strcmp("PktsOut:", sysvar) == 0)
    PktsOut = sysval;
  else if(strcmp("mismatch:", sysvar) == 0)
    mismatch = sysval;
  else if(strcmp("bad_cable:", sysvar) == 0)
    bad_cable = sysval;
  else if(strcmp("congestion:", sysvar) == 0)
    congestion = sysval;
  else if(strcmp("half_duplex:", sysvar) == 0)
    half_duplex = sysval;
  else if(strcmp("CongestionSignals:", sysvar) == 0)
    CongestionSignals = sysval;
  else if(strcmp("RcvWinScale:", sysvar) == 0) {
    if (sysval > 15)
      RcvWinScale = 0;
    else
      RcvWinScale = sysval;
  }
  else if(strcmp("minCWNDpeak:", sysvar) == 0)
      minPeak = sysval;
  else if(strcmp("maxCWNDpeak:", sysvar) == 0)
      maxPeak = sysval;
  else if(strcmp("CWNDpeaks:", sysvar) == 0)
      peaks = sysval;
}

void
save_dbl_values(char *sysvar, float *sysval)
{
  if (strcmp(sysvar, "bw:") == 0)
    estimate = *sysval;
  else if (strcmp(sysvar, "loss:") == 0)
    loss = *sysval;
  else if (strcmp(sysvar, "avgrtt:") == 0)
    avgrtt = *sysval;
  else if (strcmp(sysvar, "waitsec:") == 0)
    waitsec = *sysval;
  else if (strcmp(sysvar, "timesec:") == 0)
    timesec = *sysval;
  else if (strcmp(sysvar, "order:") == 0)
    order = *sysval;
  else if (strcmp(sysvar, "rwintime:") == 0)
    rwintime = *sysval;
  else if (strcmp(sysvar, "sendtime:") == 0)
    sendtime = *sysval;
  else if (strcmp(sysvar, "cwndtime:") == 0)
    cwndtime = *sysval;
  else if (strcmp(sysvar, "rttsec:") == 0)
    rttsec = *sysval;
  else if (strcmp(sysvar, "rwin:") == 0)
    rwin = *sysval;
  else if (strcmp(sysvar, "swin:") == 0)
    swin = *sysval;
  else if (strcmp(sysvar, "cwin:") == 0)
    cwin = *sysval;
  else if (strcmp(sysvar, "spd:") == 0)
    spd = *sysval;
  else if (strcmp(sysvar, "aspd:") == 0)
    aspd = *sysval;
}


int
main(int argc, char *argv[])
{
  int c, swait;
  char tmpstr2[512], tmpstr[16384], varstr[16384];
  unsigned char tests = TEST_MID | TEST_C2S | TEST_S2C | TEST_SFW | TEST_STATUS;
  int ctlSocket;
  int ctlport = 3001;
  int ret, xwait;
  char buff[8192];
  char* strtokbuf;
  char *host = NULL;
  int buf_size=0;
  int msgLen, msgType;
  int conn_options = 0, debug = 0, testId;
  I2Addr server_addr = NULL;
  I2Addr local_addr = NULL, remote_addr = NULL;
  char* ptr;

#ifdef AF_INET6
#define GETOPT_LONG_INET6(x) "46"x
#else
#define GETOPT_LONG_INET6(x) x
#endif
  
  while ((c = getopt_long(argc, argv,
          GETOPT_LONG_INET6("n:p:dhlvb:"), long_options, 0)) != -1) {
    switch (c) {
      case '4':
        conn_options |= OPT_IPV4_ONLY;
        break;
      case '6':
        conn_options |= OPT_IPV6_ONLY;
        break;
      case 'h':
        clt_long_usage("ANL/Internet2 NDT version " VERSION " (client)");
        break;
      case 'v':
        printf("ANL/Internet2 NDT version %s (client)\n", VERSION);
        exit(0);
        break;
      case 'b':
        buf_size = atoi(optarg);
        break;
      case 'd':
        debug++;
        break;
      case 'l':
        msglvl++;
        break;
      case 'p':
        ctlport = atoi(optarg);
        break;
      case 'n':
        host = optarg;
        break;
      case 301:
        printWeb100VarInfo();
        exit(0);
        break;
      case 302:
        tests &= (~TEST_MID);
        break;
      case 303:
        tests &= (~TEST_C2S);
        break;
      case 304:
        tests &= (~TEST_S2C);
        break;
      case 305:
        tests &= (~TEST_SFW);
        break;
      case '?':
        short_usage(argv[0], "");
        break;
    }
  }
  
  if (optind < argc) {
    short_usage(argv[0], "Unrecognized non-option elements");
  }
  
  log_init(argv[0], debug);
  
  failed = 0;
  
  if (!(tests & (TEST_MID | TEST_C2S | TEST_S2C | TEST_SFW))) {
    short_usage(argv[0], "Cannot perform empty test suites");
  }

  if (host == NULL) {
    short_usage(argv[0], "Name of the server is required");
  }

  printf("Testing network path for configuration and performance problems  --  ");
  fflush(stdout);

  if ((server_addr = I2AddrByNode(get_errhandle(), host)) == NULL) {
    printf("Unable to resolve server address\n");
    exit(-3);
  }
  I2AddrSetPort(server_addr, ctlport);

  if ((ret = CreateConnectSocket(&ctlSocket, NULL, server_addr, conn_options, 0))) {
    printf("Connect() for control socket failed\n");
    exit(-4);
  }

  if (I2AddrSAddr(server_addr, 0)->sa_family == AF_INET) {
    printf("Using IPv4 address\n");
  }
  else {
    printf("Using IPv6 address\n");
  }

  log_println(1, "Requesting test suite:");
  if (tests & TEST_MID) {
    log_println(1, " > Middlebox test");
  }
  if (tests & TEST_SFW) {
    log_println(1, " > Simple firewall test");
  }
  if (tests & TEST_C2S) {
    log_println(1, " > C2S throughput test");
  }
  if (tests & TEST_S2C) {
    log_println(1, " > S2C throughput test");
  }

  /* set the TEST_STATUS flag so the server knows this client will respond to status requests.
   * this will let the server kill off zombie clients from the queue
   * RAC 7/7/09
   */
  if (tests & TEST_STATUS) {
    log_println(1, " > New Client, impliments queuing feedback");
  }
  
  /* The beginning of the protocol */

  /* write our test suite request */
  send_msg(ctlSocket, MSG_LOGIN, &tests, 1);
  /* read the specially crafted data that kicks off the old clients */
  if (readn(ctlSocket, buff, 13) != 13) {
    printf("Information: The server '%s' does not support this command line client\n",
        host);
    exit(0);
  }

  swait = 0;
  for (;;) {
    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - expected hello  message!");
      exit(1);
    }
    if (check_msg_type("Logging to server", SRV_QUEUE, msgType, buff, msgLen)) {
      exit(2);
    }
    if (msgLen <= 0) {
      log_println(0, "Improper message");
      exit(3);
    }
    buff[msgLen] = 0;
    if (check_int(buff, &xwait)) {
      log_println(0, "Invalid queue indicator");
      exit(4);
    }
    if (xwait == 0)	/* signal from ver 3.0.x NDT servers */
      break;
    if (xwait == 9977) {
      fprintf(stderr, "Server Fault: Test terminated for unknown reason, plase try again later.\n");
      exit(0);
    }
    if (xwait == 9988) {
      if (swait == 0)
      	fprintf(stderr, "Server Busy: Too many clients waiting in queue, plase try again later.\n");
      else
        fprintf(stderr, "Server Fault: Test terminated for unknown reason, plase try again later.\n");
      exit(0);
    }
    if (xwait == 9999) {
      fprintf(stderr, "Server Busy: Please wait 60 seconds for the current test to finish.\n");
      exit(0);
    }
    if (xwait == 9990) {	/* Signal from the server to see if the client is still alive */
      send_msg(ctlSocket, MSG_WAITING, &tests, 1);
      continue;
    }

    /* Each test should take less than 30 seconds, so tell them 45 sec * number of 
     * tests in the queue.
     */
    xwait = (xwait * 45);
    log_print(0, "Another client is currently begin served, your test will ");
    log_println(0,  "begin within %d seconds", xwait);
    swait = 1;
  }

  /* add alarm() signal to kill off client if the server never finishes the tests
   * RAC 7/13/09
   */
  alarm(90);

  msgLen = sizeof(buff);
  if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
    log_println(0, "Protocol error - expected version message!");
    exit(1);
  }
  if (check_msg_type("Negotiating NDT version", MSG_LOGIN, msgType, buff, msgLen)) {
    exit(2);
  }
  if (msgLen <= 0) {
    log_println(0, "Improper message");
    exit(3);
  }
  buff[msgLen] = 0;
  if (buff[0] != 'v') {
    log_println(0, "Incompatible version number");
    exit(4);
  }
  log_println(5, "Server version: %s", &buff[1]);
  if (strcmp(&buff[1], VERSION)) {
    log_println(1, "WARNING: NDT server has different version number (%s)", &buff[1]);
  }
  
  msgLen = sizeof(buff);
  if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
    log_println(0, "Protocol error - expected test suite message!");
    exit(1);
  }
  if (check_msg_type("Negotiating test suite", MSG_LOGIN, msgType, buff, msgLen)) {
    exit(2);
  }
  if (msgLen <= 0) {
    log_println(0, "Improper message");
    exit(3);
  }
  buff[msgLen] = 0;
  log_println(5, "Received tests sequence: '%s'", buff);
  if ((strtokbuf = malloc(1024)) == NULL) {
    log_println(0, "Malloc failed!");
    exit(6);
  }
  ptr = strtok_r(buff, " ", &strtokbuf);

  while (ptr) {
    if (check_int(ptr, &testId)) {
      log_println(0, "Invalid test ID");
      exit(4);
    }
    switch (testId) {
      case TEST_MID:
        if (test_mid_clt(ctlSocket, tests, host, conn_options, buf_size, tmpstr2)) {
          log_println(0, "Middlebox test FAILED!");
          tests &= (~TEST_MID);
        }
        break;
      case TEST_C2S:
        if (test_c2s_clt(ctlSocket, tests, host, conn_options, buf_size)) {
          log_println(0, "C2S throughput test FAILED!");
          tests &= (~TEST_C2S);
        }
        break;
      case TEST_S2C:
        if (test_s2c_clt(ctlSocket, tests, host, conn_options, buf_size, tmpstr)) {
          log_println(0, "S2C throughput test FAILED!");
          tests &= (~TEST_S2C);
        }
        break;
      case TEST_SFW:
        if (test_sfw_clt(ctlSocket, tests, host, conn_options)) {
          log_println(0, "Simple firewall test FAILED!");
          tests &= (~TEST_SFW);
        }
        break;
      default:
        log_println(0, "Unknown test ID");
        exit(5);
    }
    ptr = strtok_r(NULL, " ", &strtokbuf);
  }
  
  /* get the final results from server
   *
   * The results are encapsulated by the MSG_RESULTS messages. The last
   * message is MSG_LOGOUT.
   */
  for (;;) {
    msgLen = sizeof(buff);
    if (ret = (recv_msg(ctlSocket, &msgType, buff, &msgLen))) {
      if (errno == ECONNRESET)
	log_println(0, "Connection closed by server, No test performed.");
      else
        log_println(0, "Protocol error - expected results!  got '%s', msgtype=%d", buff, msgType);
      exit(1);
    }
    if (msgType == MSG_LOGOUT) {
      break;
    }
    if (check_msg_type("Tests results", MSG_RESULTS, msgType, buff, msgLen)) {
      exit(2);
    }
    strncat(tmpstr, buff, msgLen);
    log_println(6, "tmpstr = '%s'", tmpstr);
  }

  local_addr = I2AddrByLocalSockFD(get_errhandle(), ctlSocket, False);
  remote_addr = I2AddrBySockFD(get_errhandle(), ctlSocket, False);
  I2AddrFree(server_addr);
  strcpy(varstr, tmpstr);
  testResults(tests, tmpstr, host);
  if (tests & TEST_MID) {
    middleboxResults(tmpstr2, local_addr, remote_addr);
  }
  if ((tests & TEST_S2C) && (msglvl > 1))
    printVariables(varstr);
  return 0;
}
