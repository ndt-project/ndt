#include "../config.h"

#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "network.h"
#include "usage.h"
#include "logging.h"
#include "varinfo.h"
#include "utils.h"
#include "protocol.h"
#include "test_sfw.h"
#include "test_meta.h"
#include "clt_tests.h"
#include "strlutils.h"
#include "test_results_clt.h"
#include <arpa/inet.h>
#include <assert.h>

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
int pkts, lth = 8192, CurrentRTO;
int c2sData, c2sAck, s2cData, s2cAck;
int winssent, winsrecv, msglvl = 0;
int sndqueue, ssndqueue, sbytes;
int minPeak, maxPeak, peaks;
double spdin, spdout, c2sspd, s2cspd;
double aspd;

int half_duplex, congestion, bad_cable, mismatch;
double loss, estimate, avgrtt, spd, waitsec, timesec, rttsec;
double order, rwintime, sendtime, cwndtime, rwin, swin, cwin;
double mylink;
/* Set to either Web10G or Web100 */
const char *ServerType;

static struct option long_options[] = {
  { "name", 1, 0, 'n' }, { "port", 1, 0, 'p' },
  { "debug", 0, 0, 'd' }, { "help", 0, 0, 'h' },
  { "msglvl", 0, 0, 'l' }, { "web100variables", 0, 0, 301 },
  { "buffer", 1, 0, 'b' }, { "disablemid", 0, 0, 302 },
  { "disablec2s", 0, 0, 303 }, { "disables2c", 0, 0, 304 },
  { "disablesfw", 0, 0, 305 }, { "protocol_log", 1, 0, 'u' },
  { "enableprotolog", 0, 0, 'e' },
#ifdef AF_INET6
  { "ipv4", 0, 0, '4'},
  { "ipv6", 0, 0, '6'},
#endif
  { 0, 0, 0, 0 }
};

void save_int_values(char *sysvar, int sysval);
void save_dbl_values(char *sysvar, float *sysval);

/**
 * Print variables from a string containing
 * key-value pairs, where the key is separated from the value by
 * a whitespace and each key-value pair is delimited by a newline.
 *
 * This function is not currently used outside this file, and hence
 * retaining this here.
 * @param tmpstr Char array containing key-value pairs
 *  */
void printVariables(char *tmpstr) {
  int i, j, k;
  char sysvar[128], sysval[128];

  k = strlen(tmpstr) - 1;
  i = 0;
  for (;;) {
    for (j = 0; tmpstr[i] != ' '; j++)
      sysvar[j] = tmpstr[i++];
    sysvar[j] = '\0';
    i++;
    for (j = 0; tmpstr[i] != '\n'; j++)
      sysval[j] = tmpstr[i++];
    sysval[j] = '\0';
    i++;
    printf("%s %s\n", sysvar, sysval);
    if (i >= k)
      return;
  }
}

/**
 * Print information about web 100 variables
 *  from a pre-defined array.
 *
 * This function is not currently used outside this file, and hence
 * retaining this here.
 */

void printWeb100VarInfo() {
  int i = 0;

  printf(" --- Detailed description of the %s variables ---\n\n", ServerType);

  while (web100vartable[i][0]) {
    printf("* %s\n    %s\n", web100vartable[i][0], web100vartable[i][1]);
    ++i;
  }
}

/**
 * Interpret test results based on values received from the S2C, C2S tests and
 *  SFW tests. Print recommendations based on some calculations, or display
 *  values of settings that have been detected.
 *
 * @param tests  test indicator character
 * @param testresult_str Result string obtained at the end of tests from the server
 * @param host   server host name string
 */
void testResults(char tests, char *testresult_str, char* host) {
  int i = 0;

  char *sysvar, *sysval;
  float j;

  // If the S2C test was not performed, just interpret the C2S and SFW test
  // results
  if (!(tests & TEST_S2C)) {
    if (tests & TEST_C2S) {  // Was C2S test performed?
      check_C2Spacketqueuing(c2sspd, spdout, sndqueue, pkts, lth);
    }

    results_sfw(tests, host);
    return;
  }

  sysvar = strtok(testresult_str, " ");
  sysval = strtok(NULL, "\n");
  i = atoi(sysval);
  save_int_values(sysvar, i);

  for (;;) {
    sysvar = strtok(NULL, " ");
    if (sysvar == NULL)
      break;
    sysval = strtok(NULL, "\n");
    if (strchr(sysval, '.') == NULL) {
      i = atoi(sysval);
      save_int_values(sysvar, i);
      log_println(7, "Stored %d [%s] in %s", i, sysval, sysvar);
    } else {
      j = atof(sysval);
      save_dbl_values(sysvar, &j);
      log_println(7, "Stored %0.2f (%s) in %s", j, sysval, sysvar);
    }
  }

  // CountRTT = 615596;
  if (CountRTT > 0) {  // The number of round trip time samples is finite
    // Get the link speed as determined during the C2S test. if it was performed
    if (tests & TEST_C2S) {
      mylink = get_linkspeed(c2sData, half_duplex);
    }

    print_results_mismatchcheck(mismatch);

    if (mismatch == 0) {
      check_congestion(congestion);
      check_badcable(bad_cable);
      print_recommend_buffersize(rwin, rttsec, avgrtt, mylink,
                                 MaxRwinRcvd);
    }

    if (tests & TEST_C2S) {
      check_C2Spacketqueuing(c2sspd, spdout, sndqueue, pkts, lth);
    }
    if (tests & TEST_S2C) {
      check_S2Cpacketqueuing(s2cspd, spdin, ssndqueue, sbytes);
    }

    results_sfw(tests, host);

    if (msglvl > 0) {
      printf("\n\t------  %s Detailed Analysis  ------\n", ServerType);

      printf("\n%s reports the Round trip time = %0.2f msec;",
             ServerType, avgrtt);

      printf("the Packet size = %d Bytes; and \n", CurrentMSS);

      // print packet loss statistics
      print_packetloss_statistics(PktsRetrans, DupAcksIn, SACKsRcvd,
                                  order, Timeouts, waitsec, timesec);

      // print details of whether connection was send/receive/congestion limited
      print_limitedtime_ratio(rwintime, rwin, sendtime, swin, cwndtime, rttsec,
                              mylink, Sndbuf, MaxRwinRcvd);

      // check to see if excessive packet loss was reported
      print_packetloss_excess(spd, loss);

      // Now print details of optional Performance settings values like the
      // following list:
      printf("\n    %s reports TCP negotiated the optional Performance "
             "Settings to: \n", ServerType);

      // ..Selective ack options
      print_SAck_RFC2018(SACKEnabled);

      // ..Nagle algo details
      print_Nagle_RFC896(NagleEnabled);

      // ..Explicit Congestion Notification
      print_congestion_RFC3168(ECNEnabled);

      // ..RFC 1323 Time Stamping
      print_timestamping_RFC1323(TimestampsEnabled);

      // ..RFC 1323 Window Scaling
      print_windowscaling(MaxRwinRcvd, WinScaleRcvd, WinScaleSent);

      // ..throughput limiting factors details
      print_throughputlimits(MaxRwinRcvd, RcvWinScale, &Sndbuf, swin, rwin,
                             cwin, rttsec, estimate);

      // client and server's views of link speed
      print_linkspeed_dataacks((tests & TEST_C2S), c2sData,
                               c2sAck, s2cData, s2cAck);
    }
  } else {
    printf("No %s data collected!  Possible Duplex Mismatch condition "
           "caused Server to client test to run long.\nCheck for host=Full "
           "and switch=Half mismatch condition\n", ServerType);
  }
}

/**
 * This routine decodes the middlebox test results.  The data is returned
 * from the server in a specific order.  This routine pulls the string apart
 * and puts the values into the proper variable.  It then compares the values
 * to known values and writes out the specific results.
 *
 * Server data is first.
 * order is Server IP; Client IP; CurrentMSS; WinScaleRcvd; WinScaleSent;
 * Client then adds
 * Server IP; Client IP.
 * @param midresult_str  String containing test results
 * @param cltsock Used to get address information
 */

void middleboxResults(char *midresult_str, int cltsock) {
  char ssip[64], scip[64], *str;
  char csip[64], ccip[64];
  struct sockaddr_storage addr;
  socklen_t addr_size;
  int mss;
  size_t tmpLen;

  str = strtok(midresult_str, ";");
  strlcpy(ssip, str, sizeof(ssip));
  str = strtok(NULL, ";");

  strlcpy(scip, str, sizeof(scip));

  str = strtok(NULL, ";");
  mss = atoi(str);
  str = strtok(NULL, ";");
  // changing order to read winsent before winsrecv for issue 61
  winssent = atoi(str);
  str = strtok(NULL, ";");
  winsrecv = atoi(str);

  /* Get the our local IP address */
  addr_size = sizeof(addr);
  memset(ccip, 0, 64);
  tmpLen = 63;
  if (getsockname(cltsock, (struct sockaddr *) &addr, &addr_size) == -1) {
    perror("Middlebox - getsockname() failed");
  } else {
    addr2a(&addr, ccip , tmpLen);
  }

  /* Get the server IP address */
  addr_size = sizeof(addr);
  memset(csip, 0, 64);
  tmpLen = 63;
  if (getpeername(cltsock, (struct sockaddr *) &addr, &addr_size) == -1) {
    perror("Middlebox - getpeername() failed");
  } else {
    addr2a(&addr, csip , tmpLen);
  }

  // Check if MSS modification is happening
  check_MSS_modification(TimestampsEnabled, &mss);

  // detect Network Address translation box
  check_NAT(ssip, csip, scip, ccip);
}

/**
 * Save integer values for web100 variables obtained
 * 	into local variables.
 * @param sysvar Variable's name string
 * @param sysval value of this variable
 */
void save_int_values(char *sysvar, int sysval) {
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

  if (strcmp("MSSSent:", sysvar) == 0)
    MSSSent = sysval;
  else if (strcmp("MSSRcvd:", sysvar) == 0)
    MSSRcvd = sysval;
  else if (strcmp("ECNEnabled:", sysvar) == 0)
    ECNEnabled = sysval;
  else if (strcmp("NagleEnabled:", sysvar) == 0)
    NagleEnabled = sysval;
  else if (strcmp("SACKEnabled:", sysvar) == 0)
    SACKEnabled = sysval;
  else if (strcmp("TimestampsEnabled:", sysvar) == 0)
    TimestampsEnabled = sysval;
  else if (strcmp("WinScaleRcvd:", sysvar) == 0)
    WinScaleRcvd = sysval;
  else if (strcmp("WinScaleSent:", sysvar) == 0)
    WinScaleSent = sysval;
  else if (strcmp("SumRTT:", sysvar) == 0)
    SumRTT = sysval;
  else if (strcmp("CountRTT:", sysvar) == 0)
    CountRTT = sysval;
  else if (strcmp("CurMSS:", sysvar) == 0)
    CurrentMSS = sysval;
  else if (strcmp("Timeouts:", sysvar) == 0)
    Timeouts = sysval;
  else if (strcmp("PktsRetrans:", sysvar) == 0)
    PktsRetrans = sysval;
  else if (strcmp("SACKsRcvd:", sysvar) == 0)
    SACKsRcvd = sysval;
  else if (strcmp("DupAcksIn:", sysvar) == 0)
    DupAcksIn = sysval;
  else if (strcmp("MaxRwinRcvd:", sysvar) == 0)
    MaxRwinRcvd = sysval;
  else if (strcmp("MaxRwinSent:", sysvar) == 0)
    MaxRwinSent = sysval;
  else if (strcmp("Sndbuf:", sysvar) == 0)
    Sndbuf = sysval;
  else if (strcmp("X_Rcvbuf:", sysvar) == 0)
    Rcvbuf = sysval;
  else if (strcmp("DataPktsOut:", sysvar) == 0)
    DataPktsOut = sysval;
  else if (strcmp("FastRetran:", sysvar) == 0)
    FastRetran = sysval;
  else if (strcmp("AckPktsOut:", sysvar) == 0)
    AckPktsOut = sysval;
  else if (strcmp("SmoothedRTT:", sysvar) == 0)
    SmoothedRTT = sysval;
  else if (strcmp("CurCwnd:", sysvar) == 0)
    CurrentCwnd = sysval;
  else if (strcmp("MaxCwnd:", sysvar) == 0)
    MaxCwnd = sysval;
  else if (strcmp("SndLimTimeRwin:", sysvar) == 0)
    SndLimTimeRwin = sysval;
  else if (strcmp("SndLimTimeCwnd:", sysvar) == 0)
    SndLimTimeCwnd = sysval;
  else if (strcmp("SndLimTimeSender:", sysvar) == 0)
    SndLimTimeSender = sysval;
  else if (strcmp("DataBytesOut:", sysvar) == 0)
    DataBytesOut = sysval;
  else if (strcmp("AckPktsIn:", sysvar) == 0)
    AckPktsIn = sysval;
  else if (strcmp("SndLimTransRwin:", sysvar) == 0)
    SndLimTransRwin = sysval;
  else if (strcmp("SndLimTransCwnd:", sysvar) == 0)
    SndLimTransCwnd = sysval;
  else if (strcmp("SndLimTransSender:", sysvar) == 0)
    SndLimTransSender = sysval;
  else if (strcmp("MaxSsthresh:", sysvar) == 0)
    MaxSsthresh = sysval;
  else if (strcmp("CurRTO:", sysvar) == 0)
    CurrentRTO = sysval;
  else if (strcmp("c2sData:", sysvar) == 0)
    c2sData = sysval;
  else if (strcmp("c2sAck:", sysvar) == 0)
    c2sAck = sysval;
  else if (strcmp("s2cData:", sysvar) == 0)
    s2cData = sysval;
  else if (strcmp("s2cAck:", sysvar) == 0)
    s2cAck = sysval;
  else if (strcmp("PktsOut:", sysvar) == 0)
    PktsOut = sysval;
  else if (strcmp("mismatch:", sysvar) == 0)
    mismatch = sysval;
  else if (strcmp("bad_cable:", sysvar) == 0)
    bad_cable = sysval;
  else if (strcmp("congestion:", sysvar) == 0)
    congestion = sysval;
  else if (strcmp("half_duplex:", sysvar) == 0)
    half_duplex = sysval;
  else if (strcmp("CongestionSignals:", sysvar) == 0)
    CongestionSignals = sysval;
  else if (strcmp("RcvWinScale:", sysvar) == 0) {
    if (sysval > 15)
      RcvWinScale = 0;
    else
      RcvWinScale = sysval;
  } else if (strcmp("minCWNDpeak:", sysvar) == 0) {
    minPeak = sysval;
  } else if (strcmp("maxCWNDpeak:", sysvar) == 0) {
    maxPeak = sysval;
  } else if (strcmp("CWNDpeaks:", sysvar) == 0) {
    peaks = sysval;
  }
}

/**
 * Save float values for web100 variables obtained
 * 	into local variables.
 * @param sysvar Variable's name
 * @param sysval variable's value
 */
void save_dbl_values(char *sysvar, float *sysval) {
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

/**
 * Main function of web command line tool
 * @param argc	Number of command line arguments
 * @param argv  Command line arguments
 * @return > 0 if successful, < 0 if not
 */
int main(int argc, char *argv[]) {
  int useroption;  // user options char
  int swait;  // server wait status
  char mid_resultstr[MIDBOX_TEST_RES_SIZE];  // MID test results
  char resultstr[2*BUFFSIZE];  // S2C test results, 16384 = 2 * BUFFSIZE
  char varstr[2*BUFFSIZE];  // temporary storage for S2C test results,
                            // 16384 = 2 * BUFFSIZE
  // which tests have been selectedto be performed?
  unsigned char tests = TEST_MID | TEST_C2S | TEST_S2C | TEST_SFW |
      TEST_STATUS | TEST_META;
  int ctlSocket;  // socket fd
  int ctlport = atoi(PORT);  // default port number
  int retcode;  // return code from protocol operations, mostly
  int xwait;  // wait flag
  char buff[BUFFSIZE];  // buffer used to store protocol message payload
  char* strtokbuf;  // buffer to store string tokens
  char *host = NULL;  // server name to connect to
  int buf_size = 0;  // TCP send/receive window size received from user
  int msgLen, msgType;  // protocol message related variables
  int conn_options = 0;  // connection options received from user
  int debug = 0;  // debug flag
  int testId;  // test ID received from server
  // addresses..
  I2Addr server_addr = NULL;
  char* ptr;
#ifdef AF_INET6
#define GETOPT_LONG_INET6(x) "46"x
#else
#define GETOPT_LONG_INET6(x) x
#endif
  // Read and record various optional values used for the tests/process
  while ((useroption = getopt_long(argc, argv,
                                   GETOPT_LONG_INET6("n:u:e:p:dhlvb:"),
                                   long_options, 0)) != -1) {
    switch (useroption) {
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
      case 'u':
        set_protologdir(strdup(optarg));
        break;
      case 'e':
        enableprotocollogging();
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

  // If protocol log is enabled, create log dir
  create_protolog_dir();

  log_init(argv[0], debug);

  failed = 0;

  // Check if user options do not include any test!
  if (!(tests & (TEST_MID | TEST_C2S | TEST_S2C | TEST_SFW))) {
    short_usage(argv[0], "Cannot perform empty test suites");
  }

  // A server is needed to run the tests against
  if (host == NULL) {
    short_usage(argv[0], "Name of the server is required");
  }

  // Check for initial configuration and performance problems
  printf(
      "Testing network path for configuration and performance problems  --  ");
  fflush(stdout);

  if ((server_addr = I2AddrByNode(get_errhandle(), host)) == NULL) {
    printf("Unable to resolve server address\n");
    exit(-3);
  }
  I2AddrSetPort(server_addr, ctlport);

  if ((retcode = CreateConnectSocket(&ctlSocket, NULL, server_addr,
                                     conn_options, 0))) {
    printf("Connect() for control socket failed\n");
    exit(-4);
  }

  // check and print Address family being used
  if (I2AddrSAddr(server_addr, 0)->sa_family == AF_INET) {
    printf("Using IPv4 address\n");
  } else {
    printf("Using IPv6 address\n");
  }

  // set options for test direction
  enum Tx_DIRECTION currentDirn = C_S;
  setCurrentDirn(currentDirn);
  // end protocol logging

  /* set the TEST_STATUS flag so the server knows this client will respond to status requests.
   * this will let the server kill off zombie clients from the queue
   * RAC 7/7/09
   */
  if (tests & TEST_STATUS) {
    log_println(1, "* New Client, implements queuing feedback");
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
  if (tests & TEST_META) {
    log_println(1, " > META test");
  }

  /* The beginning of the protocol */

  /* write our test suite request by sending a login message */
  send_msg(ctlSocket, MSG_LOGIN, &tests, 1);
  /* read the specially crafted data that kicks off the old clients */
  if (readn(ctlSocket, buff, 13) != 13) {
    printf("Information: The server '%s' does not support this command line "
           "client\n", host);
    exit(0);
  }

  // The server now sends a SRV_QUEUE message to indicate whether/not to wait
  // SRV_QUEUE messages are only sent to queued clients with a message
  // body that indicates one of a few statuses, as will be handled individually
  // below.

  swait = 0;
  for (;;) {
    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - expected hello  message!");
      exit(1);
    }
    if (check_msg_type("Logging to server", SRV_QUEUE, msgType, buff,
                       msgLen)) {
      // Any other type of message at this stage is incorrect
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
    // SRV_QUEUE message received indicating "ready to start tests" .
    // ...proceed to running tests
    if (xwait == SRV_QUEUE_TEST_STARTS_NOW)
      /* signal from ver 3.0.x NDT servers */
      break;

    if (xwait == SRV_QUEUE_SERVER_FAULT) {
      fprintf(stderr,
              "Server Fault: Test terminated for unknown reason, please try "
              "again later.\n");
      exit(0);
    }
    if (xwait == SRV_QUEUE_SERVER_BUSY) {
      if (swait == 0) {
        // First message from server, indicating server is busy. Quit
        fprintf(stderr, "Server Busy: Too many clients waiting in queue, plase "
                "try again later.\n");
      } else {  // Server fault, quit
        fprintf(stderr, "Server Fault: Test terminated for unknown reason, "
                "please try again later.\n");
      }
      exit(0);
    }
    // server busy, wait 60 s for previous test to finish
    if (xwait == SRV_QUEUE_SERVER_BUSY_60s) {
      fprintf(stderr, "Server Busy: Please wait 60 seconds for the current "
              "test to finish.\n");
      exit(0);
    }
    // Signal from the server to see if the client is still alive
    if (xwait == SRV_QUEUE_HEARTBEAT) {
      send_msg(ctlSocket, MSG_WAITING, &tests, 1);
      continue;
    }


    /* Each test should take less than 30 s. So convey 45 sec *
       number of tests-suites waiting in the queue. Note that server sends a
       number equal to the number of clients ==
       number of minutes to wait before starting tests. In other words,
       wait = num of minutes to wait = number of queued clients */

    xwait = (xwait * 45);
    log_print(0,
              "Another client is currently begin served, your test will ");
    log_println(0, "begin within %d seconds", xwait);
    swait = 1;
  }

  /* add alarm() signal to kill off client if the server never finishes the tests
   * RAC 7/13/09
   */
  alarm(90);

  // Tests can be started. Read server response again.
  // The server must send MSG_LOGIN  message to verify version.
  // Any other type of message is unexpected at this point.

  msgLen = sizeof(buff);
  if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
    log_println(0, "Protocol error - expected version message!");
    exit(1);
  }
  if (check_msg_type("Negotiating NDT version", MSG_LOGIN, msgType, buff,
                     msgLen)) {
    exit(2);
  }
  if (msgLen <= 0) {
    log_println(0, "Improper message");
    exit(3);
  }

  // Version compatibility between server-client must be verified

  buff[msgLen] = 0;
  if (buff[0] != 'v') {  // payload doesn't start with a version indicator
    log_println(0, "Incompatible version number");
    exit(4);
  }
  log_println(5, "Server version: %s", &buff[1]);

  if (strcmp(&buff[strlen(buff) - 6], "Web10G") == 0 || strcmp(&buff[strlen(buff) - 6], "web100") == 0) {
    char buffTmp[strlen(buff)+1];
    strncpy(buffTmp, &buff[1], strlen(&buff[1])-7);

    log_println(5, "Compare versions. Server:%s Client:%s Compare result: %i", buffTmp, VERSION, strcmp(buffTmp, VERSION));
    if (strcmp(buffTmp, VERSION) != 0) {
      log_println(1, "WARNING: NDT server has different version number (%s)", &buff[1]);
    }
  }
  else if (strcmp(&buff[1], VERSION)) { //older server did not send type server at the end
    log_println(1, "WARNING: NDT server has different version number (%s)", &buff[1]);
  }

  ServerType = "Web100";
  if (strlen(buff) > 6) {
    if (strcmp(&buff[strlen(buff) - 6], "Web10G") == 0)
      ServerType = "Web10G";
  }

  // Server must send a message to negotiate the test suite, and this is
  // a MSG_LOGIN type of message which indicates the same set of tests as
  // requested by the client earlier
  msgLen = sizeof(buff);
  if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
    log_println(0, "Protocol error - expected test suite message!");
    exit(1);
  }
  if (check_msg_type("Negotiating test suite", MSG_LOGIN, msgType, buff,
                     msgLen)) {
    exit(2);
  }
  if (msgLen <= 0) {
    log_println(0, "Improper message");
    exit(3);
  }

  // get ids of tests to be run now
  buff[msgLen] = 0;
  log_println(5, "Received tests sequence: '%s'", buff);
  if ((strtokbuf = malloc(1024)) == NULL) {
    log_println(0, "Malloc failed!");
    exit(6);
  }
  ptr = strtok_r(buff, " ", &strtokbuf);

  // Run all tests requested, based on the ID.
  while (ptr) {
    if (check_int(ptr, &testId)) {
      log_println(0, "Invalid test ID");
      exit(4);
    }
    switch (testId) {
      case TEST_MID:
        if (test_mid_clt(ctlSocket, tests, host, conn_options, buf_size,
                         mid_resultstr)) {
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
        if (test_s2c_clt(ctlSocket, tests, host, conn_options, buf_size,
                         resultstr)) {
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
      case TEST_META:
        if (test_meta_clt(ctlSocket, tests, host, conn_options)) {
          log_println(0, "META test FAILED!");
          tests &= (~TEST_META);
        }
        break;
      default:
        log_println(0, "Unknown test ID");
        exit(5);
    }
    ptr = strtok_r(NULL, " ", &strtokbuf);
  }

  /* get the final results from server.
   *
   * The results are encapsulated by the MSG_RESULTS messages. The last
   * message is MSG_LOGOUT.
   */
  for (;;) {
    msgLen = sizeof(buff);
    memset(buff, 0, msgLen);  // reset buff and msgLen
    if ((retcode = (recv_msg(ctlSocket, &msgType, buff, &msgLen)))) {
      if (errno == ECONNRESET)
        log_println(0,
                    "Connection closed by server, No test performed.");
      else
        log_println(
            0,
            "Protocol error - expected results!  got '%s', msgtype=%d",
            buff, msgType);
      exit(1);
    }
    if (msgType == MSG_LOGOUT) {
      break;
    }
    if (check_msg_type("Tests results", MSG_RESULTS, msgType, buff,
                       msgLen)) {
      exit(2);
    }

    strlcat(resultstr, buff, sizeof(resultstr));
    log_println(6, "resultstr = '%s'", resultstr);
  }

  strlcpy(varstr, resultstr, sizeof(varstr));
  // print test results
  testResults(tests, resultstr, host);

  // print middlebox test results
  if (tests & TEST_MID) {
    middleboxResults(mid_resultstr, ctlSocket);
  }

  I2AddrFree(server_addr);

  // print extra information collected from web100 variables
  if ((tests & TEST_S2C) && (msglvl > 1))
    printVariables(varstr);
  return 0;
}
