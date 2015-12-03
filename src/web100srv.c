/*
   Copyright (c) 2003 University of Chicago.  All rights reserved.
   The Web100 Network Diagnostic Tool (NDT) is distributed subject to
   the following license conditions:
   SOFTWARE LICENSE AGREEMENT
Software: Web100 Network Diagnostic Tool (NDT)

1. The "Software", below, refers to the Web100 Network Diagnostic Tool (NDT)
(in either source code, or binary form and accompanying documentation). Each
licensee is addressed as "you" or "Licensee."

2. The copyright holder shown above hereby grants Licensee a royalty-free
nonexclusive license, subject to the limitations stated herein and U.S.
Government license rights.

3. You may modify and make a copy or copies of the Software for use within your
organization, if you meet the following conditions:
a. Copies in source code must include the copyright notice and this Software
License Agreement.
b. Copies in binary form must include the copyright notice and this Software
License Agreement in the documentation and/or other materials provided with the
copy.

4. You may make a copy, or modify a copy or copies of the Software or any
portion of it, thus forming a work based on the Software, and distribute copies
outside your organization, if you meet all of the following conditions:
a. Copies in source code must include the copyright notice and this
Software License Agreement;
b. Copies in binary form must include the copyright notice and this
Software License Agreement in the documentation and/or other materials
provided with the copy;
c. Modified copies and works based on the Software must carry prominent
notices stating that you changed specified portions of the Software.

5. Portions of the Software resulted from work developed under a U.S. Government
contract and are subject to the following license: the Government is granted
for itself and others acting on its behalf a paid-up, nonexclusive, irrevocable
worldwide license in this computer software to reproduce, prepare derivative
works, and perform publicly and display publicly.

6. WARRANTY DISCLAIMER. THE SOFTWARE IS SUPPLIED "AS IS" WITHOUT WARRANTY
OF ANY KIND. THE COPYRIGHT HOLDER, THE UNITED STATES, THE UNITED STATES
DEPARTMENT OF ENERGY, AND THEIR EMPLOYEES: (1) DISCLAIM ANY WARRANTIES,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO ANY IMPLIED WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE OR NON-INFRINGEMENT,
(2) DO NOT ASSUME ANY LEGAL LIABILITY OR RESPONSIBILITY FOR THE ACCURACY,
COMPLETENESS, OR USEFULNESS OF THE SOFTWARE, (3) DO NOT REPRESENT THAT USE
OF THE SOFTWARE WOULD NOT INFRINGE PRIVATELY OWNED RIGHTS, (4) DO NOT WARRANT
THAT THE SOFTWARE WILL FUNCTION UNINTERRUPTED, THAT IT IS ERROR-FREE OR THAT
ANY ERRORS WILL BE CORRECTED.

7. LIMITATION OF LIABILITY. IN NO EVENT WILL THE COPYRIGHT HOLDER, THE
UNITED STATES, THE UNITED STATES DEPARTMENT OF ENERGY, OR THEIR EMPLOYEES:
BE LIABLE FOR ANY INDIRECT, INCIDENTAL, CONSEQUENTIAL, SPECIAL OR PUNITIVE
DAMAGES OF ANY KIND OR NATURE, INCLUDING BUT NOT LIMITED TO LOSS OF PROFITS
OR LOSS OF DATA, FOR ANY REASON WHATSOEVER, WHETHER SUCH LIABILITY IS ASSERTED
ON THE BASIS OF CONTRACT, TORT (INCLUDING NEGLIGENCE OR STRICT LIABILITY), OR
OTHERWISE, EVEN IF ANY OF SAID PARTIES HAS BEEN WARNED OF THE POSSIBILITY OF
SUCH LOSS OR DAMAGES.
The Software was developed at least in part by the University of Chicago,
as Operator of Argonne National Laboratory (http://miranda.ctd.anl.gov:7123/).
*/

#include "../config.h"

#include <assert.h>
#include <time.h>
#include <ctype.h>
#include <getopt.h>
#include <math.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#define SYSLOG_NAMES
#include <pthread.h>
#include <syslog.h>
#include <sys/times.h>

#include "web100srv.h"
#include "network.h"
#include "usage.h"
#include "utils.h"
#include "mrange.h"
#include "logging.h"
#include "testoptions.h"
#include "protocol.h"
#include "web100-admin.h"
#include "test_sfw.h"
#include "ndt_odbc.h"
#include "runningtest.h"
#include "strlutils.h"
#include "heuristics.h"
#include "tests_srv.h"
#include "jsonutils.h"
#include "websocket.h"

static char lgfn[FILENAME_SIZE];  // log file name
static char wvfn[FILENAME_SIZE];  // file name of web100-variables list
static char apfn[FILENAME_SIZE];  // admin file name
static char slfa[256];            // syslog facility
static char logd[256];            // log dir name
static char portbuf[10];          // port number user option store
static char devicebuf[100];       // device name buf (seems unused)
static char dbDSNbuf[256];        // DB datasource name
static char dbUIDbuf[256];        // DB UID
static char dbPWDbuf[256];        // DB Password

// list of global variables used throughout this program.
static int window = 64000;  // TCP buffer size
static int count_vars = 0;
int dumptrace = 0;
static int usesyslog = 0;
static int multiple = 0;
static int compress = 1;
static int max_clients = 50;
static int set_buff = 0;
static int admin_view = 0;
static int queue = 1;
static int record_reverse = 0;
static int refresh = 30;
static int old_mismatch = 0;  // use the old duplex mismatch detection heuristic
/* int sig1, sig2, sig17; */

static Options options;
static CwndPeaks peaks;
static int webVarsValues = 0;
static char webVarsValuesLog[256];
static int cputime = 0;
static char cputimelog[256];
static pthread_t workerThreadId;
static int cputimeworkerLoop = 1;

static int useDB = 0;
static char *dbDSN = NULL;
static char *dbUID = NULL;
static char *dbPWD = NULL;

static char *VarFileName = NULL;
static char *AdminFileName = NULL;
static char *SysLogFacility = NULL;
static int syslogfacility = LOG_FACILITY;
static char *ConfigFileName = NULL;
static char buff[BUFFSIZE + 1];
static char rmt_host[256];
static char rmt_addr[256];
static char *device = NULL;
static char *port = PORT;
static TestOptions testopt;

static int conn_options = 0;
static int ndtpid;
static int testPort;
static char testName[256];

// The kind of data that is sent from the parent to the child.
typedef int ParentChildSignal;

// The kind of data that is sent to wake up the server.
typedef char ServerWakeupMessage;

// The file descriptor used to signal the main server process.
static int global_signalfd_write;

// Whether extended c2s and s2c tests should be allowed.
static int global_extended_tests_allowed = 1;

// When we support multiple clients, grow the queue by a constant factor
#define QUEUE_SIZE_MULTIPLIER 4

static struct option long_options[] = {{"adminview", 0, 0, 'a'},
                                       {"debug", 0, 0, 'd'},
                                       {"help", 0, 0, 'h'},
                                       {"multiple", 0, 0, 'm'},
                                       {"max_clients", 1, 0, 'x'},
                                       {"mrange", 1, 0, 301},
                                       {"old", 0, 0, 'o'},
                                       {"disable-queue", 0, 0, 'q'},
                                       {"record", 0, 0, 'r'},
                                       {"syslog", 0, 0, 's'},
                                       {"tcpdump", 0, 0, 't'},
                                       {"version", 0, 0, 'v'},
                                       {"gzip", 0, 0, 'z'},
                                       {"config", 1, 0, 'c'},
#ifdef EXPERIMENTAL_ENABLED
                                       {"avoidsndblockup", 0, 0, 306},
                                       {"snaplog", 0, 0, 307},
                                       {"snapdelay", 1, 0, 305},
                                       {"cwnddecrease", 0, 0, 308},
                                       {"cputime", 0, 0, 309},
                                       {"limit", 1, 0, 'y'},
#endif
                                       {"buffer", 1, 0, 'b'},
                                       {"file", 1, 0, 'f'},
                                       {"interface", 1, 0, 'i'},
                                       {"log", 1, 0, 'l'},
                                       {"protolog_dir", 1, 0, 'u'},
                                       {"enableprotolog", 0, 0, 'e'},
                                       {"port", 1, 0, 'p'},
                                       {"midport", 1, 0, 302},
                                       {"c2sport", 1, 0, 303},
                                       {"s2cport", 1, 0, 304},
                                       {"refresh", 1, 0, 'T'},
                                       {"adminfile", 1, 0, 'A'},
                                       {"log_dir", 1, 0, 'L'},
                                       {"logfacility", 1, 0, 'S'},
#if defined(HAVE_ODBC) && defined(DATABASE_ENABLED) && defined(HAVE_SQL_H)
                                       {"enableDBlogging", 0, 0, 310},
                                       {"dbDSN", 1, 0, 311},
                                       {"dbUID", 1, 0, 312},
                                       {"dbPWD", 1, 0, 313},
#endif
                                       {"c2sduration", 1, 0, 314},
                                       {"c2sthroughputsnaps", 0, 0, 315},
                                       {"c2ssnapsdelay", 1, 0, 316},
                                       {"c2ssnapsoffset", 1, 0, 317},
                                       {"c2sstreamsnum", 1, 0, 322},
                                       {"s2cduration", 1, 0, 318},
                                       {"s2cthroughputsnaps", 0, 0, 319},
                                       {"s2csnapsdelay", 1, 0, 320},
                                       {"s2csnapsoffset", 1, 0, 321},
                                       {"s2cstreamsnum", 1, 0, 323},
                                       {"savewebvalues", 0, 0, 324},
#ifdef AF_INET6
                                       {"ipv4", 0, 0, '4'},
                                       {"ipv6", 0, 0, '6'},
#endif
                                       {"tls", 0, 0, 325},
                                       {"private_key", 1, 0, 326},
                                       {"certificate", 1, 0, 327},
                                       {"disable_extended_tests", 0, 0, 328},
                                       {0, 0, 0, 0}};

/**
 * Catch termination signal(s) and print message in log file
 * @param signo Signal number
 */
void cleanup(int signo) {
  FILE *fp;
  ServerWakeupMessage msg;
  int status;

  if (signo != SIGINT && signo != SIGPIPE) {
    log_println(1, "Signal %d received by process %d", signo, getpid());
    if (get_debuglvl() > 0) {
      fp = fopen(get_logfile(), "a");
      if (fp != NULL) {
        fprintf(fp, "Signal %d received by process %d\n", signo, getpid());
        fclose(fp);
      }
    }
  }
  switch (signo) {
    default:
      fp = fopen(get_logfile(), "a");
      if (fp != NULL) {
        fprintf(fp,
                "Unexpected signal (%d) received, process (%d) may terminate\n",
                signo, getpid());
        fclose(fp);
      }
      break;
    case SIGSEGV:
      log_println(6, "DEBUG, caught SIGSEGV signal and terminated process (%d)",
                  getpid());
      if (getpid() != ndtpid) exit(-2);
      break;
    case SIGINT:
      exit(0);
    case SIGTERM:
      if (getpid() == ndtpid) {
        log_println(
            6,
            "DEBUG, SIGTERM signal received for parent process (%d), ignore it",
            ndtpid);
        break;
      }
      exit(0);
    case SIGUSR1:
      /* SIGUSR1 is used exclusively by C2S, to interrupt the pcap capture*/
      log_println(6,
                  "DEBUG, caught SIGUSR1, setting sig1 flag and calling "
                  "force_breakloop");
      force_breakloop();
      break;

    case SIGUSR2:
      /* SIGUSR2 is used exclusively by S2C, to interrupt the pcap capture*/
      log_println(6,
                  "DEBUG, caught SIGUSR2, setting sig2 flag and calling "
                  "force_breakloop");
      force_breakloop();
      break;

    case SIGALRM:
      switch (getCurrentTest()) {
        case TEST_MID:
          log_println(6, "Received SIGALRM signal [Middlebox test]");
          break;
        case TEST_C2S:
          log_println(6, "Received SIGALRM signal [C2S throughput test] pid=%d",
                      getpid());
          break;
        case TEST_C2S_EXT:
          log_println(6,
                      "Received SIGALRM signal [Extended C2S throughput test] pid=%d",
                      getpid());
          break;
        case TEST_S2C:
          log_println(6, "Received SIGALRM signal [S2C throughput test] pid=%d",
                      getpid());
          break;
        case TEST_S2C_EXT:
          log_println(6,
                      "Received SIGALRM signal [Extended S2C throughput test] pid=%d",
                      getpid());
          break;
        case TEST_SFW:
          log_println(6, "Received SIGALRM signal [Simple firewall test]");
          break;
        case TEST_META:
          log_println(6, "Received SIGALRM signal [META test]");
          break;
      }
      fp = fopen(get_logfile(), "a");
      if (fp != NULL) {
        if (get_debuglvl() > 4)
          fprintf(
              fp,
              "Received SIGALRM signal: terminating active web100srv process "
              "[%d]",
              getpid());
        switch (getCurrentTest()) {
          case TEST_MID:
            fprintf(fp, " [Middlebox test]\n");
            break;
          case TEST_C2S:
            fprintf(fp, " [C2S throughput test]\n");
            /* break; */
            if (wait_sig == 1) return;
            break;
          case TEST_C2S_EXT:
            fprintf(fp, " [Extended C2S throughput test]\n");
            /* break; */
            if (wait_sig == 1)
              return;
            break;
          case TEST_S2C:
            fprintf(fp, " [S2C throughput test]\n");
            /* break; */
            if (wait_sig == 1) return;
            break;
          case TEST_S2C_EXT:
            fprintf(fp, " [Extended S2C throughput test]\n");
            /* break; */
            if (wait_sig == 1)
              return;
            break;
          case TEST_SFW:
            fprintf(fp, " [Simple firewall test]\n");
            break;
          case TEST_META:
            fprintf(fp, " [META test]\n");
            break;
          default:
            fprintf(fp, "\n");
        }
        fclose(fp);
      }
      exit(0);
    case SIGPIPE:
      // SIGPIPE is an expected signal due to race conditions regarding the
      // possibility of writing a message to an already-terminated child.  Do
      // not let it kill the process.  The SIGCHLD handler will take care of
      // child process cleanup.
      fp = fopen(get_logfile(), "a");
      if ((fp != NULL) && (get_debuglvl() > 4)) {
        fprintf(fp, "Received SIGPIPE: a child has terminated early.\n");
        fclose(fp);
      }
      break;

    case SIGHUP:
      // Initialize Web100 structures
      count_vars = tcp_stat_init(VarFileName);

      // The administrator view automatically generates a usage page for the
      // NDT server.  This page is then accessible to the general public.
      // At this point read the existing log file and generate the necessary
      // data.  This data is then updated at the end of each test.
      if (admin_view == 1) view_init(refresh);
      break;

    case SIGCHLD:
      // When a child exits, send a message to global_signalfd_write that will
      // cause the server's select() to wake up.  Only send this message in the
      // server process.
      if (ndtpid == getpid()) {
        msg = '0';  // garbage value, never examined at the other end.
        write(global_signalfd_write, &msg, sizeof(ServerWakeupMessage));
        log_println(5, "Signal 17 (SIGCHLD) received - completed tests");
      }
      // To prevent zombies, make sure every child process is wait()ed upon.
      waitpid(-1, &status, WNOHANG);
      break;
  }
}

/**
 * LoadConfig routine copied from Internet2.
 * @param *name Pointer to Application name
 * @param **lbuf Line buffer
 * @param *lbuf_max Line buffer max - both help keep track of a dynamically
 *                  grown "line" buffer.
 */
static void LoadConfig(char *name, char **lbuf, size_t *lbuf_max) {
  FILE *conf;
  char keybuf[256], valbuf[256];
  char *key = keybuf, *val = valbuf;
  int retcode = 0;

  if (!(conf = fopen(ConfigFileName, "r"))) return;

  log_println(1, " Reading config file %s to obtain options", ConfigFileName);

  // Read the values for various keys and store them in appropriate variables
  while ((retcode = I2ReadConfVar(conf, retcode, key, val, 256, lbuf,
                                  lbuf_max)) > 0) {
    if (strncasecmp(key, "administrator_view", 5) == 0) {
      admin_view = 1;
      continue;
    } else if (strncasecmp(key, "multiple_clients", 5) == 0) {
      multiple = 1;
      continue;
    } else if (strncasecmp(key, "max_clients", 5) == 0) {
      max_clients = atoi(val);
      continue;
    } else if (strncasecmp(key, "record_reverse", 6) == 0) {
      record_reverse = 1;
      continue;
    } else if (strncasecmp(key, "write_trace", 5) == 0) {
      dumptrace = 1;
      continue;
    } else if (strncasecmp(key, "TCP_Buffer_size", 3) == 0) {
      if (check_int(val, &window)) {
        char tmpText[200];
        snprintf(tmpText, sizeof(tmpText), "Invalid window size: %s", val);
        short_usage(name, tmpText);
      }
      set_buff = 1;
      continue;
    } else if (strncasecmp(key, "debug", 5) == 0) {
      set_debuglvl(atoi(val));
      continue;
    } else if (strncasecmp(key, "variable_file", 6) == 0) {
#if USE_WEB100
      snprintf(wvfn, sizeof(wvfn), "%s", val);
      VarFileName = wvfn;
#elif USE_WEB10G
      log_println(0, "Web10G does not require variable file. Ignoring");
#endif
      continue;
    } else if (strncasecmp(key, "log_file", 3) == 0) {
      snprintf(lgfn, sizeof(lgfn), "%s", val);
      set_logfile(lgfn);
      snprintf(lgfn, sizeof(lgfn), "%s", val);
      continue;
    } else if (strncasecmp(key, "protolog_dir", 12) == 0) {
      snprintf(lgfn, sizeof(lgfn), "%s", val);
      set_protologdir(lgfn);
      continue;
    } else if (strncasecmp(key, "enableprotolog", 11) == 0) {
      enableprotocollogging();
      continue;
    } else if (strncasecmp(key, "admin_file", 10) == 0) {
      snprintf(apfn, sizeof(apfn), "%s", val);
      AdminFileName = apfn;
      continue;
    } else if (strncasecmp(key, "logfacility", 11) == 0) {
      snprintf(slfa, sizeof(slfa), "%s", val);
      SysLogFacility = slfa;
      continue;
    } else if (strncasecmp(key, "device", 5) == 0) {
      snprintf(devicebuf, sizeof(devicebuf), "%s", val);
      device = devicebuf;
      continue;
    } else if (strncasecmp(key, "port", 4) == 0) {
      snprintf(portbuf, sizeof(portbuf), "%s", val);
      port = portbuf;
      continue;
    } else if (strncasecmp(key, "syslog", 6) == 0) {
      usesyslog = 1;
      continue;
    } else if (strncasecmp(key, "disable_FIFO", 5) == 0) {
      queue = 0;
      continue;
    } else if (strncasecmp(key, "old_mismatch", 3) == 0) {
      old_mismatch = 1;
      continue;
    } else if (strncasecmp(key, "cputime", 3) == 0) {
      cputime = 1;
      continue;
    } else if (strncasecmp(key, "enableDBlogging", 8) == 0) {
      useDB = 1;
      continue;
    } else if (strncasecmp(key, "dbDSN", 5) == 0) {
      snprintf(dbDSNbuf, sizeof(dbDSNbuf), "%s", val);
      dbDSN = dbDSNbuf;
      continue;
    } else if (strncasecmp(key, "dbUID", 5) == 0) {
      snprintf(dbUIDbuf, sizeof(dbUIDbuf), "%s", val);
      dbUID = dbUIDbuf;
      continue;
    } else if (strncasecmp(key, "dbPWD", 5) == 0) {
      snprintf(dbPWDbuf, sizeof(dbPWDbuf), "%s", val);
      dbPWD = dbPWDbuf;
      continue;
    } else if (strncasecmp(key, "cwnddecrease", 5) == 0) {
      options.cwndDecrease = 1;
      options.snaplog = 1;
      continue;
    } else if (strncasecmp(key, "snaplog", 5) == 0) {
      options.snaplog = 1;
      continue;
    } else if (strncasecmp(key, "disablesnaps", 5) == 0) {
      options.snapshots = 0;
      continue;
    } else if (strncasecmp(key, "avoidsndblockup", 5) == 0) {
      options.avoidSndBlockUp = 1;
      continue;
    } else if (strncasecmp(key, "snapdelay", 5) == 0) {
      options.snapDelay = atoi(val);
      continue;
    } else if (strncasecmp(key, "limit", 5) == 0) {
      options.limit = atoi(val);
      continue;
    } else if (strncasecmp(key, "savewebvalues", 13) == 0) {
      webVarsValues = 1;
      continue;
    } else if (strncasecmp(key, "c2sduration", 9) == 0) {
      options.c2s_duration = atoi(val);
      continue;
    } else if (strncasecmp(key, "c2sthroughputsnaps", 16) == 0) {
      options.c2s_throughputsnaps = 1;
      continue;
    } else if (strncasecmp(key, "c2ssnapsdelay", 11) == 0) {
      options.c2s_snapsdelay = atoi(val);
      continue;
    } else if (strncasecmp(key, "c2ssnapsoffset", 12) == 0) {
      options.c2s_snapsoffset = atoi(val);
      continue;
    } else if (strncasecmp(key, "c2sstreamsnum", 11) == 0) {
      options.c2s_streamsnum = atoi(val);
      continue;
    } else if (strncasecmp(key, "s2cduration", 9) == 0) {
      options.s2c_duration = atoi(val);
      continue;
    } else if (strncasecmp(key, "s2cthroughputsnaps", 16) == 0) {
      options.s2c_throughputsnaps = 1;
      continue;
    } else if (strncasecmp(key, "s2csnapsdelay", 11) == 0) {
      options.s2c_snapsdelay = atoi(val);
      continue;
    } else if (strncasecmp(key, "s2csnapsoffset", 12) == 0) {
      options.s2c_snapsoffset = atoi(val);
      continue;
    } else if (strncasecmp(key, "s2cstreamsnum", 11) == 0) {
      options.s2c_streamsnum = atoi(val);
      continue;
    } else if (strncasecmp(key, "refresh", 5) == 0) {
      refresh = atoi(val);
      continue;
    } else if (strncasecmp(key, "mrange", 6) == 0) {
      if (mrange_parse(val)) {
        char tmpText[300];
        snprintf(tmpText, sizeof(tmpText), "Invalid range: %s", val);
        short_usage(name, tmpText);
      }
      continue;
    } else if (strncasecmp(key, "midport", 7) == 0) {
      if (check_int(val, &testopt.midsockport)) {
        char tmpText[200];
        snprintf(tmpText, sizeof(tmpText),
                 "Invalid Middlebox test port number: %s", val);
        short_usage(name, tmpText);
      }
      continue;
    } else if (strncasecmp(key, "c2sport", 7) == 0) {
      if (check_int(val, &testopt.c2ssockport)) {
        char tmpText[200];
        snprintf(tmpText, sizeof(tmpText),
                 "Invalid C2S throughput test port number: %s", val);
        short_usage(name, tmpText);
      }
      continue;
    } else if (strncasecmp(key, "s2cport", 7) == 0) {
      if (check_int(val, &testopt.s2csockport)) {
        char tmpText[200];
        snprintf(tmpText, sizeof(tmpText),
                 "Invalid S2C throughput test port number: %s", val);
        short_usage(name, tmpText);
      }
      continue;
    } else {
      log_println(0, "Unrecognized config option: %s", key);
      exit(1);
    }
  }
  if (retcode < 0) {
    log_println(0, "Syntax error in line %d of the config file %s", (-retcode),
                ConfigFileName);
    exit(1);
  }
  fclose(conf);
}


/**
 * Capture CPU time details
 *
 * @param arg* Filename of the log file used to record CPU time details
 */
void *cputimeWorker(void *arg) {
  char *logname = (char *)arg;
  FILE *file = fopen(logname, "w");
  struct tms buf;
  double start = secs();

  if (!file) return NULL;

  while (1) {
    if (!cputimeworkerLoop) {
      break;
    }
    times(&buf);
    fprintf(file, "%.2f %ld %ld %ld %ld\n", secs() - start, buf.tms_utime,
            buf.tms_stime, buf.tms_cutime, buf.tms_cstime);
    fflush(file);
    mysleep(0.1);
  }

  fclose(file);

  return NULL;
}

/**
 * Run all tests, process results, record them into relevant log files
 *
 * @param agent Pointer to tcp_stat agent
 * @param ctl Connection used for server->client communication
 * @param testopt TestOptions *
 * @param test_suite Pointer to string indicating tests to be run
 * @param ssl_context Pointer to the SSL context (may be null)
 */
int run_test(tcp_stat_agent *agent, Connection *ctl, TestOptions *testopt,
             char *test_suite, SSL_CTX *ssl_context) {
#if USE_WEB100
  tcp_stat_connection conn = NULL;
#elif USE_WEB10G
  tcp_stat_connection conn = -1;
#endif
  char date[32];      // date indicator
  char spds[4][256];  // speed "bin" array containing counters for speeds
  char logstr1[4096], logstr2[1024];  // log
  char tmpstr[256];
  char isoTime[64];

  // int n;  // temporary iterator variable --// commented out -> calc_linkspeed
  struct tcp_vars vars[MAX_STREAMS];
  struct throughputSnapshot *s2c_ThroughputSnapshots, *c2s_ThroughputSnapshots;

  int link = CANNOT_DETERMINE_LINK;  // local temporary variable indicative of
  // link speed. Transmitted but unused at client end , which has a similar
  // link speed variable
  int mismatch = 0, bad_cable = 0;
  int half_duplex = NO_HALF_DUPLEX;
  int congestion = 0, totaltime;
  int ret, spd_index;
  // int index;  // commented out -> calc_linkspeed
  // int links[16];  // commented out -> calc_linkspeed
  // int max;  // commented out -> calc_linkspeed
  // int total;// commented out -> calc_linkspeed
  // C->S data link speed indicator determined using results
  int c2s_linkspeed_data = 0;
  int c2s_linkspeed_ack = 0;
  // S->C data link speed indicator determined using results
  int s2c_linkspeed_data = 0;
  int s2c_linkspeed_ack = 0;
  // int j;        // commented out -> calc_linkspeed
  int i;
  int totalcnt;
  int autotune;
  // values collected from the speed tests
  u_int32_t dec_cnt, same_cnt, inc_cnt;
  int timeout, dupack;
  // int ifspeed;

  time_t stime;

  double rttsec;      // average round trip time
  double swin, cwin;  // send, congestion window sizes respectively
  double rwin;        // max window size advertisement rcvd
  double rwintime;    // time spent being limited due to rcvr end
  double cwndtime;    // time spent being limited due to congestion
  double sendtime;    // time spent being limited due to sender's own fault

  double oo_order;  // out-of-order packet ratio
  double waitsec;
  double bw_theortcl;     // theoretical bandwidth
  double avgrtt;          // Average round-trip time
  double timesec;         // Total test time in microseconds
  double packetloss_s2c;  // Packet loss as calculated from S->c tests.
  double RTOidle;         // Proportion of idle time spent waiting for packets
  double s2cspd;          // average throughput as calculated by S->C test
  double c2sspd;          // average throughput as calculated by C->S test
  double s2c2spd;         // average throughput as calculated by midbox test
  double realthruput;     // total send throughput in S->C
  double aspd = 0;
  float runave[4];

  FILE *fp;

  // start with a clean slate of currently running test and direction
  setCurrentTest(TEST_NONE);
  log_println(7, "Remote host= %s", get_remotehostaddress());

  stime = time(0);
  log_println(4, "Child process %d started", getpid());
  testopt->child0 = getpid();

  // initialize speeds array
  for (spd_index = 0; spd_index < 4; spd_index++)
    for (ret = 0; ret < 256; ret++)
      spds[spd_index][ret] = 0x00;
  spd_index = 0;

  // obtain web100 connection and check auto-tune status
  conn = tcp_stat_connection_from_socket(agent, ctl->socket);
  autotune = tcp_stat_autotune(ctl->socket, agent, conn);

  // client needs to be version compatible. Send current version
  snprintf(buff, sizeof(buff), "v%s", VERSION "-" TCP_STAT_NAME);
  send_json_message_any(ctl, MSG_LOGIN, buff, strlen(buff),
                        testopt->connection_flags, JSON_SINGLE_VALUE);

  // initiate test with MSG_LOGIN message.
  log_println(3, "run_test() routine, asking for test_suite = %s",
              test_suite);
  send_json_message_any(ctl, MSG_LOGIN, test_suite, strlen(test_suite),
                        testopt->connection_flags, JSON_SINGLE_VALUE);

  log_println(1, "Starting test suite:");
  if (testopt->midopt) {
    log_println(1, " > Middlebox test");
  }
  if (testopt->sfwopt) {
    log_println(1, " > Simple firewall test");
  }
  if (testopt->c2sopt) {
    log_println(1, " > C2S throughput test");
  }
  if (testopt->c2sextopt) {
    log_println(1, " > Extended C2S throughput test");
  }
  if (testopt->s2copt) {
    log_println(1, " > S2C throughput test");
  }
  if (testopt->s2cextopt) {
    log_println(1, " > Extended S2C throughput test");
  }
  if (testopt->metaopt) {
    log_println(1, " > META test");
  }

  /*  alarm(15); */
  // Run scheduled test. Log error code if necessary
  log_println(6, "Starting middlebox test");
  if ((ret = test_mid(ctl, agent, testopt, conn_options, &s2c2spd)) != 0) {
    if (ret < 0)
      log_println(6, "Middlebox test failed with rc=%d", ret);
    log_println(0, "Middlebox test FAILED!, rc=%d", ret);
    testopt->midopt = TOPT_DISABLED;
    return ret;
  }

  /*  alarm(20); */
  log_println(6, "Starting simple firewall test");
  if ((ret = test_sfw_srv(ctl, agent, testopt, conn_options)) != 0) {
    if (ret < 0)
      log_println(6, "SFW test failed with rc=%d", ret);
  }

  /*  alarm(25); */
  log_println(6, "Starting c2s throughput test");
  if ((ret = test_c2s(ctl, agent, testopt, conn_options, &c2sspd, set_buff,
                      window, autotune, device, &options, record_reverse,
                      count_vars, spds, &spd_index, ssl_context,
                      &c2s_ThroughputSnapshots, 0)) != 0) {
    if (ret < 0)
      log_println(6, "C2S test failed with rc=%d", ret);
    log_println(0, "C2S throughput test FAILED!, rc=%d", ret);
    testopt->c2sopt = TOPT_DISABLED;
    return ret;
  }

  log_println(6, "Starting extended c2s throughput test");
  if ((ret = test_c2s(ctl, agent, testopt, conn_options, &c2sspd,
                      set_buff, window, autotune, device, &options,
                      record_reverse, count_vars, spds, &spd_index,
                      ssl_context, &c2s_ThroughputSnapshots, 1)) != 0) {
    if (ret < 0)
      log_println(6, "Extended C2S test failed with rc=%d", ret);
    log_println(0, "Extended C2S throughput test FAILED!, rc=%d", ret);
    testopt->c2sextopt = TOPT_DISABLED;
    return ret;
  }

  /*  alarm(25); */
  log_println(6, "Starting s2c throughput test");
  if ((ret = test_s2c(ctl, agent, testopt, conn_options, &s2cspd,
                      set_buff, window, autotune, device, &options, spds,
                      &spd_index, count_vars, &peaks, ssl_context, &s2c_ThroughputSnapshots, 0)) != 0) {
    if (ret < 0)
      log_println(6, "S2C test failed with rc=%d", ret);
    log_println(0, "S2C throughput test FAILED!, rc=%d", ret);
    testopt->s2copt = TOPT_DISABLED;
    return ret;
  }

  log_println(6, "Starting extended s2c throughput test");
  if ((ret = test_s2c(ctl, agent, testopt, conn_options, &s2cspd,
                      set_buff, window, autotune, device, &options, spds,
                      &spd_index, count_vars, &peaks, ssl_context,
                      &s2c_ThroughputSnapshots, 1)) != 0) {
    if (ret < 0)
      log_println(6, "Extended S2C test failed with rc=%d", ret);
    log_println(0, "Extended S2C throughput test FAILED!, rc=%d", ret);
    testopt->s2cextopt = TOPT_DISABLED;
    return ret;
  }

  log_println(6, "Starting META test");
  if ((ret = test_meta_srv(ctl, agent, testopt, conn_options, &options)) != 0) {
    if (ret < 0) {
      log_println(6, "META test failed with rc=%d", ret);
    }
  }

  // Compute variable values from test results and deduce results
  log_println(4, "Finished testing C2S = %0.2f Mbps, S2C = %0.2f Mbps",
              c2sspd / 1000, s2cspd / 1000);

  // Determine link speed
  calc_linkspeed(spds, spd_index, &c2s_linkspeed_data, &c2s_linkspeed_ack,
                 &s2c_linkspeed_data, &s2c_linkspeed_ack, runave, &dec_cnt,
                 &same_cnt, &inc_cnt, &timeout, &dupack, testopt->c2sopt);
  // Get web100 vars

  // ...determine number of times congestion window has been changed
  if (options.cwndDecrease) {
    dec_cnt = inc_cnt = same_cnt = 0;
    CwndDecrease(options.s2c_logname[0], &dec_cnt, &same_cnt, &inc_cnt);
    log_println(2, "####### decreases = %d, increases = %d, no change = %d",
                dec_cnt, inc_cnt, same_cnt);
  }

  // ...other variables
  memset(&vars, 0xFF, sizeof(vars));
  for (i = 0; i < (testopt->s2cextopt ? options.s2c_streamsnum : 1); ++i) {
    tcp_stat_logvars(&vars[i], i, count_vars);
  }

  if (webVarsValues) {
    tcp_stat_logvars_to_file(webVarsValuesLog, testopt->s2cextopt ? options.s2c_streamsnum : 1, vars);
    tcp_stat_log_agg_vars_to_file(webVarsValuesLog, testopt->s2cextopt ? options.s2c_streamsnum : 1, vars);
  }

  // end getting web100 variable values

  // section to calculate duplex mismatch
  // Calculate average round trip time and convert to seconds
  rttsec = calc_avg_rtt(vars[0].SumRTT, vars[0].CountRTT, &avgrtt);
  // Calculate packet loss
  packetloss_s2c = calc_packetloss(vars[0].CongestionSignals, vars[0].PktsOut,
                                   c2s_linkspeed_data);

  // Calculate ratio of packets arriving out of order
  oo_order = calc_packets_outoforder(vars[0].DupAcksIn, vars[0].AckPktsIn);

  // calculate theoretical maximum goodput in bits
  bw_theortcl = calc_max_theoretical_throughput(vars[0].CurrentMSS, rttsec,
                                                packetloss_s2c);

  // get window sizes
  calc_window_sizes(&vars[0].SndWinScale, &vars[0].RcvWinScale, vars[0].Sndbuf,
                    vars[0].MaxRwinRcvd, vars[0].MaxCwnd, &rwin, &swin, &cwin);

  // Total test time
  totaltime = calc_totaltesttime(vars[0].SndLimTimeRwin, vars[0].SndLimTimeCwnd,
                                 vars[0].SndLimTimeSender);

  // time spent being send-limited due to client's recv window
  rwintime = calc_sendlimited_rcvrfault(vars[0].SndLimTimeRwin, totaltime);

  // time spent in being send-limited due to congestion window
  cwndtime = calc_sendlimited_cong(vars[0].SndLimTimeCwnd, totaltime);

  // time spent in being send-limited due to own fault
  sendtime = calc_sendlimited_sndrfault(vars[0].SndLimTimeSender, totaltime);

  timesec = totaltime / MEGA;  // total time in microsecs

  // get fraction of total test time waiting for packets to arrive
  RTOidle = calc_RTOIdle(vars[0].Timeouts, vars[0].CurrentRTO, timesec);

  // get timeout, retransmission, acks and dup acks ratios.
  /*tmoutsratio = (double) vars[0].Timeouts / vars[0].PktsOut;
  rtranratio = (double) vars[0].PktsRetrans / vars[0].PktsOut;
  acksratio = (double) vars[0].AckPktsIn / vars[0].PktsOut;
  dackratio = (double) vars[0].DupAcksIn / (double) vars[0].AckPktsIn;*/

  // get actual throughput in Mbps (totaltime is in microseconds)
  realthruput = calc_real_throughput(vars[0].DataBytesOut, totaltime);

  // total time spent waiting
  waitsec = cal_totalwaittime(vars[0].CurrentRTO, vars[0].Timeouts);

  log_println(2, "CWND limited test = %0.2f while unlimited = %0.2f", s2c2spd,
              s2cspd);

  // Is thruput measured with limited cwnd(midbox test) > as reported S->C test
  if (is_limited_cwnd_throughput_better(s2c2spd, s2cspd) &&
      isNotMultipleTestMode(multiple)) {
    log_println(
        2,
        "Better throughput when CWND is limited, may be duplex mismatch");
  } else {
    log_println(2,
                "Better throughput without CWND limits - normal operation");
  }

  // remove the following line when the new detection code is ready for release
  // retaining old comment above
  // client link duplex mismatch detection heuristic
  old_mismatch = 1;

  if (old_mismatch == 1) {
    if (detect_duplexmismatch(cwndtime, bw_theortcl, vars[0].PktsRetrans, timesec,
                              vars[0].MaxSsthresh, RTOidle, link, s2cspd, s2c2spd,
                              multiple)) {
      if (is_c2s_throughputbetter(c2sspd, s2cspd)) {
        // also, S->C throughput is lesser than C->S throughput
        mismatch = DUPLEX_OLD_ALGO_INDICATOR;
        // possible duplex, from Old Duplex-Mismatch logic
      } else {
        mismatch = DUPLEX_SWITCH_FULL_HOST_HALF;
        // switch full, host half
      }
      link = LINK_ALGO_FAILED;
    }

    // test for uplink with duplex mismatch condition
    if (detect_internal_duplexmismatch(
        (s2cspd / 1000), realthruput, rwintime, packetloss_s2c)) {
      mismatch = DUPLEX_SWITCH_FULL_HOST_HALF;  // switch full, host half
      link = LINK_ALGO_FAILED;
    }
  } else {
    /* This section of code is being held up for non-technical reasons.
     * Once those issues are resolved a new mismatch detection algorim
     * will be placed here.
     *  RAC 5-11-06
     */
  }

  // end section calculating duplex mismatch

  // Section to deduce if there is faulty hardware links

  // estimate is less than actual throughput, something is wrong
  if (bw_theortcl < realthruput)
    link = LINK_ALGO_FAILED;

  // Faulty hardware link heuristic.
  if (detect_faultyhardwarelink(packetloss_s2c, cwndtime, timesec,
                                vars[0].MaxSsthresh))
    bad_cable = POSSIBLE_BAD_CABLE;

  // test for Ethernet link (assume Fast E.)
  if (detect_ethernetlink(realthruput, s2cspd, packetloss_s2c, oo_order,
                          link))
    link = LINK_ETHERNET;

  // test for wireless link
  if (detect_wirelesslink(sendtime, realthruput, bw_theortcl,
                          vars[0].SndLimTransRwin, vars[0].SndLimTransCwnd, rwintime,
                          link)) {
    link = LINK_WIRELESS;
  }

  // test for DSL/Cable modem link
  if (detect_DSLCablelink(vars[0].SndLimTimeSender, vars[0].SndLimTransSender,
                          realthruput, bw_theortcl, link)) {
    link = LINK_DSLORCABLE;
  }

  // full/half link duplex setting heuristic:
  // receiver-limited- time > 95%,
  //  .. number of transitions into the 'Receiver Limited' state is greater
  //  than 30 ps
  //  ...and the number of transitions into the 'Sender Limited' state is
  //  greater than 30 per second

  if (detect_halfduplex(rwintime, vars[0].SndLimTransRwin, vars[0].SndLimTransSender,
                        timesec))
    half_duplex = POSSIBLE_HALF_DUPLEX;

  // congestion detection heuristic
  if (detect_congestionwindow(cwndtime, mismatch, cwin, rwin, rttsec))
    congestion = POSSIBLE_CONGESTION;

  // Send results and variable values to clients
  snprintf(buff, sizeof(buff), "c2sData: %d\nc2sAck: %d\ns2cData: %d\n"
           "s2cAck: %d\n", c2s_linkspeed_data, c2s_linkspeed_ack,
           s2c_linkspeed_data, s2c_linkspeed_ack);
  send_json_message_any(ctl, MSG_RESULTS, buff, strlen(buff),
                        testopt->connection_flags, JSON_SINGLE_VALUE);

  snprintf(buff, sizeof(buff),
           "half_duplex: %d\nlink: %d\ncongestion: %d\nbad_cable: %d\n"
           "mismatch: %d\nspd: %0.2f\n", half_duplex, link, congestion,
           bad_cable, mismatch, realthruput);
  send_json_message_any(ctl, MSG_RESULTS, buff, strlen(buff),
                        testopt->connection_flags, JSON_SINGLE_VALUE);

  snprintf(buff, sizeof(buff),
           "bw: %0.2f\nloss: %0.9f\navgrtt: %0.2f\nwaitsec: %0.2f\n"
           "timesec: %0.2f\norder: %0.4f\n", bw_theortcl, packetloss_s2c,
           avgrtt, waitsec, timesec, oo_order);
  send_json_message_any(ctl, MSG_RESULTS, buff, strlen(buff),
                        testopt->connection_flags, JSON_SINGLE_VALUE);

  snprintf(buff, sizeof(buff),
           "rwintime: %0.4f\nsendtime: %0.4f\ncwndtime: %0.4f\n"
           "rwin: %0.4f\nswin: %0.4f\n", rwintime, sendtime, cwndtime, rwin,
           swin);
  send_json_message_any(ctl, MSG_RESULTS, buff, strlen(buff),
                        testopt->connection_flags, JSON_SINGLE_VALUE);

  snprintf(buff, sizeof(buff),
           "cwin: %0.4f\nrttsec: %0.6f\nSndbuf: %"VARtype"\naspd: %0.5f\n"
           "CWND-Limited: %0.2f\n", cwin, rttsec, vars[0].Sndbuf, aspd, s2c2spd);
  send_json_message_any(ctl, MSG_RESULTS, buff, strlen(buff),
                        testopt->connection_flags, JSON_SINGLE_VALUE);

  snprintf(buff, sizeof(buff),
           "minCWNDpeak: %d\nmaxCWNDpeak: %d\nCWNDpeaks: %d\n",
           peaks.min, peaks.max, peaks.amount);
  send_json_message_any(ctl, MSG_RESULTS, buff, strlen(buff),
                        testopt->connection_flags, JSON_SINGLE_VALUE);

  // Signal end of test results to client
  send_json_message_any(ctl, MSG_LOGOUT, "", 0, testopt->connection_flags,
                        JSON_SINGLE_VALUE);

  // Copy collected values into the meta data structures. This section
  // seems most readable, easy to debug here.
  snprintf(meta.date, sizeof(meta.date), "%s",
           get_ISOtime(isoTime, sizeof(isoTime)));

  log_println(9, "meta.date=%s, meta.clientip =%s:%s:%d", meta.date,
              meta.client_ip, rmt_addr, strlen(rmt_addr));
  memcpy(meta.client_ip, rmt_addr, strlen(rmt_addr));
  log_println(9, "2. meta.clientip =%s:%s:%d", meta.client_ip, rmt_addr);

  memset(tmpstr, 0, sizeof(tmpstr));
  snprintf(tmpstr, sizeof(tmpstr), "%d,%d,%d,%"VARtype",%"VARtype",%"
           VARtype",%"VARtype",%"VARtype",%"VARtype",%"VARtype",%"
           VARtype",%"VARtype",%"VARtype",",
           (int) s2c2spd, (int) s2cspd, (int) c2sspd, vars[0].Timeouts,
           vars[0].SumRTT, vars[0].CountRTT, vars[0].PktsRetrans, vars[0].FastRetran,
           vars[0].DataPktsOut, vars[0].AckPktsOut, vars[0].CurrentMSS, vars[0].DupAcksIn,
           vars[0].AckPktsIn);
  memcpy(meta.summary, tmpstr, strlen(tmpstr));
  memset(tmpstr, 0, sizeof(tmpstr));
  snprintf(tmpstr, sizeof(tmpstr), "%"VARtype",%"VARtype",%"VARtype",%"
           VARtype",%"VARtype",%"VARtype",%"VARtype",%"VARtype
           ",%"VARtype",%"VARtype",%"VARtype",%"VARtype",%"VARtype",",
           vars[0].MaxRwinRcvd, vars[0].Sndbuf, vars[0].MaxCwnd, vars[0].SndLimTimeRwin,
           vars[0].SndLimTimeCwnd, vars[0].SndLimTimeSender, vars[0].DataBytesOut,
           vars[0].SndLimTransRwin, vars[0].SndLimTransCwnd, vars[0].SndLimTransSender,
           vars[0].MaxSsthresh, vars[0].CurrentRTO, vars[0].CurrentRwinRcvd);

  strlcat(meta.summary, tmpstr, sizeof(meta.summary));
  memset(tmpstr, 0, sizeof(tmpstr));
  snprintf(tmpstr, sizeof(tmpstr), "%d,%d,%d,%d,%d", link, mismatch, bad_cable,
           half_duplex, congestion);

  strlcat(meta.summary, tmpstr, sizeof(meta.summary));
  memset(tmpstr, 0, sizeof(tmpstr));
  snprintf(tmpstr, sizeof(tmpstr), ",%d,%d,%d,%d,%"VARtype",%"VARtype
           ",%"VARtype",%"VARtype",%d",
           c2s_linkspeed_data, c2s_linkspeed_ack, s2c_linkspeed_data,
           s2c_linkspeed_ack, vars[0].CongestionSignals, vars[0].PktsOut, vars[0].MinRTT,
           vars[0].RcvWinScale, autotune);

  strlcat(meta.summary, tmpstr, sizeof(meta.summary));
  memset(tmpstr, 0, sizeof(tmpstr));
  snprintf(tmpstr, sizeof(tmpstr), ",%"VARtype",%"VARtype",%"VARtype",%"
           VARtype",%"VARtype",%"VARtype",%"VARtype",%"VARtype",%"
           VARtype",%"VARtype,
           vars[0].CongAvoid, vars[0].CongestionOverCount, vars[0].MaxRTT,
           vars[0].OtherReductions, vars[0].CurTimeoutCount, vars[0].AbruptTimeouts,
           vars[0].SendStall, vars[0].SlowStart, vars[0].SubsequentTimeouts,
           vars[0].ThruBytesAcked);

  strlcat(meta.summary, tmpstr, sizeof(meta.summary));
  memset(tmpstr, 0, sizeof(tmpstr));
  snprintf(tmpstr, sizeof(tmpstr), ",%d,%d,%d", peaks.min, peaks.max,
           peaks.amount);

  strlcat(meta.summary, tmpstr, sizeof(meta.summary));
  writeMeta(options.compress, cputime, options.snapshots, options.snaplog, dumptrace, s2c_ThroughputSnapshots, c2s_ThroughputSnapshots);

  // Write into log files, DB
  fp = fopen(get_logfile(), "a");
  if (fp == NULL) {
    log_println(0,
                "Unable to open log file '%s', continuing on without logging",
                get_logfile());
  } else {
    snprintf(date, sizeof(date), "%15.15s", ctime(&stime) + 4);
    fprintf(fp, "%s,", date);
    fprintf(fp, "%s,%d,%d,%d,%"VARtype",%"VARtype",%"VARtype",%"
            VARtype",%"VARtype",%"VARtype",%"VARtype",%"VARtype",%"
            VARtype",%"VARtype",", rmt_addr,
            (int) s2c2spd, (int) s2cspd, (int) c2sspd, vars[0].Timeouts,
            vars[0].SumRTT, vars[0].CountRTT, vars[0].PktsRetrans, vars[0].FastRetran,
            vars[0].DataPktsOut, vars[0].AckPktsOut, vars[0].CurrentMSS, vars[0].DupAcksIn,
            vars[0].AckPktsIn);
    fprintf(fp, "%"VARtype",%"VARtype",%"VARtype",%"VARtype",%"VARtype","
            "%"VARtype",%"VARtype",%"VARtype",%"VARtype",%"VARtype",%"
            VARtype",%"VARtype",%"VARtype",", vars[0].MaxRwinRcvd,
            vars[0].Sndbuf, vars[0].MaxCwnd, vars[0].SndLimTimeRwin, vars[0].SndLimTimeCwnd,
            vars[0].SndLimTimeSender, vars[0].DataBytesOut, vars[0].SndLimTransRwin,
            vars[0].SndLimTransCwnd, vars[0].SndLimTransSender, vars[0].MaxSsthresh,
            vars[0].CurrentRTO, vars[0].CurrentRwinRcvd);
    fprintf(fp, "%d,%d,%d,%d,%d", link, mismatch, bad_cable, half_duplex,
            congestion);
    fprintf(fp, ",%d,%d,%d,%d,%"VARtype",%"VARtype",%"VARtype",%"VARtype",%d",
            c2s_linkspeed_data,
            c2s_linkspeed_ack, s2c_linkspeed_data, s2c_linkspeed_ack,
            vars[0].CongestionSignals, vars[0].PktsOut, vars[0].MinRTT, vars[0].RcvWinScale,
            autotune);
    fprintf(fp, ",%"VARtype",%"VARtype",%"VARtype",%"VARtype",%"VARtype
            ",%"VARtype",%"VARtype",%"VARtype",%"VARtype",%"VARtype,
            vars[0].CongAvoid,
            vars[0].CongestionOverCount, vars[0].MaxRTT, vars[0].OtherReductions,
            vars[0].CurTimeoutCount, vars[0].AbruptTimeouts, vars[0].SendStall,
            vars[0].SlowStart, vars[0].SubsequentTimeouts, vars[0].ThruBytesAcked);
    fprintf(fp, ",%d,%d,%d\n", peaks.min, peaks.max, peaks.amount);
    fclose(fp);
  }
  db_insert(spds, runave, cputimelog, options.s2c_logname[0],
            options.c2s_logname, testName, testPort, date, rmt_addr, s2c2spd,
            s2cspd, c2sspd, vars[0].Timeouts, vars[0].SumRTT, vars[0].CountRTT,
            vars[0].PktsRetrans, vars[0].FastRetran, vars[0].DataPktsOut,
            vars[0].AckPktsOut, vars[0].CurrentMSS, vars[0].DupAcksIn, vars[0].AckPktsIn,
            vars[0].MaxRwinRcvd, vars[0].Sndbuf, vars[0].MaxCwnd, vars[0].SndLimTimeRwin,
            vars[0].SndLimTimeCwnd, vars[0].SndLimTimeSender, vars[0].DataBytesOut,
            vars[0].SndLimTransRwin, vars[0].SndLimTransCwnd, vars[0].SndLimTransSender,
            vars[0].MaxSsthresh, vars[0].CurrentRTO, vars[0].CurrentRwinRcvd, link,
            mismatch, bad_cable, half_duplex, congestion, c2s_linkspeed_data,
            c2s_linkspeed_ack, s2c_linkspeed_data, s2c_linkspeed_ack,
            vars[0].CongestionSignals, vars[0].PktsOut, vars[0].MinRTT, vars[0].RcvWinScale,
            autotune, vars[0].CongAvoid, vars[0].CongestionOverCount, vars[0].MaxRTT,
            vars[0].OtherReductions, vars[0].CurTimeoutCount, vars[0].AbruptTimeouts,
            vars[0].SendStall, vars[0].SlowStart, vars[0].SubsequentTimeouts,
            vars[0].ThruBytesAcked, peaks.min, peaks.max, peaks.amount);
  if (usesyslog == 1) {
    snprintf(
        logstr1, sizeof(logstr1),
        "client_IP=%s,c2s_spd=%2.0f,s2c_spd=%2.0f,Timeouts=%"VARtype","
        "SumRTT=%"VARtype","
        "CountRTT=%"VARtype",PktsRetrans=%"VARtype","
        "FastRetran=%"VARtype",DataPktsOut=%"VARtype","
        "AckPktsOut=%"VARtype","
        "CurrentMSS=%"VARtype",DupAcksIn=%"VARtype","
        "AckPktsIn=%"VARtype",",
        rmt_addr, c2sspd, s2cspd, vars[0].Timeouts, vars[0].SumRTT, vars[0].CountRTT,
        vars[0].PktsRetrans, vars[0].FastRetran, vars[0].DataPktsOut, vars[0].AckPktsOut,
        vars[0].CurrentMSS, vars[0].DupAcksIn, vars[0].AckPktsIn);
    snprintf(
        logstr2, sizeof(logstr2),
        "MaxRwinRcvd=%"VARtype",Sndbuf=%"VARtype","
        "MaxCwnd=%"VARtype",SndLimTimeRwin=%"VARtype","
        "SndLimTimeCwnd=%"VARtype",SndLimTimeSender=%"VARtype","
        "DataBytesOut=%"VARtype","
        "SndLimTransRwin=%"VARtype",SndLimTransCwnd=%"VARtype","
        "SndLimTransSender=%"VARtype","
        "MaxSsthresh=%"VARtype",CurrentRTO=%"VARtype","
        "CurrentRwinRcvd=%"VARtype",",
        vars[0].MaxRwinRcvd, vars[0].Sndbuf, vars[0].MaxCwnd, vars[0].SndLimTimeRwin,
        vars[0].SndLimTimeCwnd, vars[0].SndLimTimeSender, vars[0].DataBytesOut,
        vars[0].SndLimTransRwin, vars[0].SndLimTransCwnd, vars[0].SndLimTransSender,
        vars[0].MaxSsthresh, vars[0].CurrentRTO, vars[0].CurrentRwinRcvd);
    strlcat(logstr1, logstr2, sizeof(logstr1));
    snprintf(
        logstr2, sizeof(logstr2),
        "link=%d,mismatch=%d,bad_cable=%d,half_duplex=%d,congestion=%d,"
        "c2s_linkspeed_data=%d,c2sack=%d,s2cdata=%d,s2cack=%d,"
        "CongestionSignals=%"VARtype",PktsOut=%"VARtype",MinRTT=%"
        VARtype",RcvWinScale=%"VARtype"\n",
        link, mismatch, bad_cable, half_duplex, congestion, c2s_linkspeed_data,
        c2s_linkspeed_ack, s2c_linkspeed_data, s2c_linkspeed_ack,
        vars[0].CongestionSignals, vars[0].PktsOut, vars[0].MinRTT, vars[0].RcvWinScale);
    strlcat(logstr1, logstr2, sizeof(logstr1));
    syslog(LOG_FACILITY | LOG_INFO, "%s", logstr1);
    closelog();
    log_println(4, "%s", logstr1);
  }

  // close resources

  if (testopt->s2copt) {
    close(testopt->s2csockfd);
  }

  // If the admin view is turned on then the client process is going to update
  // these variables.  They need to be shipped back to the parent so the admin
  // page can be updated.  Otherwise the changes are lost when the client
  // terminates.
  if (admin_view == 1) {
    totalcnt = calculate(date, vars[0].SumRTT, vars[0].CountRTT,
                         vars[0].CongestionSignals, vars[0].PktsOut, vars[0].DupAcksIn,
                         vars[0].AckPktsIn, vars[0].CurrentMSS, vars[0].SndLimTimeRwin,
                         vars[0].SndLimTimeCwnd, vars[0].SndLimTimeSender,
                         vars[0].MaxRwinRcvd, vars[0].CurrentCwnd, vars[0].Sndbuf,
                         vars[0].DataBytesOut, mismatch, bad_cable, (int) c2sspd,
                         (int) s2cspd, c2s_linkspeed_data, s2c_linkspeed_ack,
                         1);
    gen_html((int) c2sspd, (int) s2cspd, vars[0].MinRTT, vars[0].PktsRetrans,
             vars[0].Timeouts, vars[0].Sndbuf, vars[0].MaxRwinRcvd, vars[0].CurrentCwnd,
             mismatch, bad_cable, totalcnt, refresh);
  }
  close_connection(ctl);
  shutdown(ctl->socket, SHUT_WR);
  return (0);
}

/**
 * Closes the connection to a child and frees its memory.
 * @param child A double pointer to the child to remove
 */
void free_ndtchild(ndtchild **child) {
  if (child == NULL || *child == NULL) return;
  close((*child)->pipe);
  free(*child);
  *child = NULL;
}

/**
 * Read a single message sent from the parent server process to the child
 * process, and perform any necessary communication that message necessitates.
 * @param parent_pipe Where to read the message from
 * @return The message value on success, a negative error code on failure
 */
int read_from_parent(int parent_pipe) {
  ParentChildSignal parent_message;
  int retcode;
  retcode = read(parent_pipe, &parent_message, sizeof(ParentChildSignal));
  log_println(6, "Child %d got '%d' from parent", getpid(), parent_message);
  if (retcode < 0) {
    retcode = errno;  // Save errno in case log_println sets it.
    log_println(1, "Child %d couldn't read from the pipe.");
    return -retcode;
  }
  if (parent_message < 0) {
    // The message received is a negative number. This should never happen.
    log_println(1, "Child %d got a negative number (%d) from the parent.",
                getpid(), parent_message);
    return -EBADMSG;
  }
  return parent_message;
}

/**
 * Sends a single SRV_QUEUE message to client. Calls exit() if the sending
 * fails.
 * @param ctl The Connection to send the message on
 * @param testopt The options used to determine how to talk on the Connection
 * @param message The message to send
 */
void send_srv_queue_message_or_die(Connection *ctl, TestOptions *testopt,
                                   int message) {
  char serialized_message[16];
  snprintf(serialized_message, sizeof(serialized_message), "%d", message);
  if (send_json_message_any(
          ctl, SRV_QUEUE, serialized_message, strlen(serialized_message),
          testopt->connection_flags, JSON_SINGLE_VALUE) != 0) {
    log_println(1, "Client failed to send message");
    exit(1);
  }
}

/**
 * Performs the heartbeat back & forth with the specified client. All errors in
 * the heartbeat protcol are (and should be) fatal.
 * @param conn The Connection on which we should communicate
 * @param testopt The TestOptions for our communication
 */
void check_heartbeat(Connection *conn, TestOptions *testopt) {
  char msg[256];
  int msg_len;
  int msg_type;
  send_srv_queue_message_or_die(conn, testopt, SRV_QUEUE_HEARTBEAT);
  msg_len = sizeof(msg);
  if (recv_any_msg(conn, &msg_type, msg, &msg_len, testopt->connection_flags) !=
      0) {
    log_println(1, "Client failed to respond to heartbeat message");
    exit(0);
  }
}

/**
 * Process a single parent message. If the message is fatal this will exit.
 * @param parent_message The message to process.
 * @param ctl The connection to the client
 * @param testopt The TestOptions, required to communicate with the client
 * @param t_opts The tests requested by the client
 */
void process_parent_message(int parent_message, Connection *ctl,
                            TestOptions *testopt, int t_opts) {
  if (parent_message < 0) {
    if (parent_message == -EINTR) {
      // EINTR means that a signal handler fired or some other interruption
      // occurred. It means that this read() failed, but the next might not.
      return;
    } else {
      exit(0);
    }
  }
  // Perform any I/O message entailed by parent_message
  if (parent_message == SRV_QUEUE_HEARTBEAT) {
    // Perform the heartbeat check (if we can).
    if (t_opts & SRV_QUEUE) check_heartbeat(ctl, testopt);
  } else {
    // All other messages should pass-through to the client.
    send_srv_queue_message_or_die(ctl, testopt, parent_message);
  }
  // Update the child's local state based on the parent message
  // This switch should include a case for every "magic value" specified for
  // SRV_QUEUE messages in protocol.h
  switch (parent_message) {
    case SRV_QUEUE_TEST_STARTS_NOW:
      log_println(0,
                  "This should never happen - failed to start test when "
                  "the code was SRV_QUEUE_TEST_STARTS_NOW");
      break;
    case SRV_QUEUE_SERVER_FAULT:
      exit(0);
      break;
    case SRV_QUEUE_SERVER_BUSY:
      exit(0);
      break;
    case SRV_QUEUE_HEARTBEAT:
      break;
    case SRV_QUEUE_SERVER_BUSY_60s:
      // The SRV_QUEUE_SERVER_BUSY_60s message is not emitted by any
      // codepaths in the server.  This message is likely obsolete.
      // Set the watchdog alarm for 60 seconds, plus some slop.
      alarm(120);
      break;
    default:
      // If the message was not one of the magic values, then it is a number
      // of minutes to wait until the test starts.  Set the watchdog alarm
      // for that many minutes, plus a little bit extra to avoid race
      // conditions.
      alarm((parent_message + 1) * 60);
      break;
  }
}

/**
 * The code run by the child process. This function never returns, it only
 * calls exit().  It also has an alarm() timer which by default prevents any
 * client from living for more than 5 minutes.  The parent is in control, and
 * will send this child a signal when it gets to the head of the testing queue
 * (and will also send other status messages that are relayed by the client).
 * Only the child process can communicate to the client (OpenSSL connections
 * can only be used by one process), so we pass along any queueing messages we
 * get from the parent.
 */
void child_process(int parent_pipe, SSL_CTX *ssl_context, int ctlsockfd) {
  FILE *fp;
  time_t tt;
  char isoTime[64], dir[256];
  int t_opts = 0;
  int retcode = 0, parent_message;
  char test_suite[256];
  // Initial length (in seconds) of the child's watchdog timer.
  int alarm_time = 120;
  tcp_stat_agent *agent;
  Connection ctl = {-1, NULL};
  ctl.socket = ctlsockfd;
  // this is the child process, it handles the connection with the client and
  // runs the actual tests.
  // First, start the watchdog timer
  alarm(300);

  // Set up the connection with the client (with optional SSL and websockets)
  if (options.tls) {
    if (setup_SSL_connection(&ctl, ssl_context) != 0) {
      log_println(1, "setup_SSL_connection failed");
      exit(-1);
    }
  }

  // Read the login message and send the kickoff message (if applicable)
  t_opts = initialize_tests(&ctl, &testopt, test_suite, sizeof(test_suite));
  if (t_opts < 1) {  // some error in initialization routines
    log_println(3, "Invalid test suite received, terminate child");
    exit(-1);
  }

#if USE_WEB100
  if ((agent = web100_attach(WEB100_AGENT_TYPE_LOCAL, NULL)) == NULL) {
    web100_perror("web100_attach");
    exit(1);
  }
#elif USE_WEB10G
  if (estats_nl_client_init(&agent) != NULL) {
    log_println(0, "Error: estats_client_init failed. Unable to use web10g.");
    exit(1);
  }
#endif
  // Wait in the queue until the child process is told to start
  while ((parent_message = read_from_parent(parent_pipe)) !=
         SRV_QUEUE_TEST_STARTS_NOW) {
    process_parent_message(parent_message, &ctl, &testopt, t_opts);
  }

  // Tell the client the test is about to start
  send_srv_queue_message_or_die(&ctl, &testopt, SRV_QUEUE_TEST_STARTS_NOW);
  set_timestamp();

  // construct cputime log folder
  {
    I2Addr tmp_addr = I2AddrBySockFD(get_errhandle(), ctl.socket, False);
    testPort = I2AddrPort(tmp_addr);
    meta.ctl_port = testPort;
    snprintf(testName, sizeof(testName), "%s", rmt_host);
    I2AddrFree(tmp_addr);
    memset(cputimelog, 0, sizeof(cputimelog));
    if (cputime) {
      snprintf(dir, sizeof(dir), "%s_%s:%d.cputime",
               get_ISOtime(isoTime, sizeof(isoTime)), rmt_host, testPort);
      log_println(8, "CPUTIME:suffix=%s", dir);
      create_named_logdir(cputimelog, sizeof(cputimelog), dir, 0);
      memcpy(meta.CPU_time, dir, strlen(dir));
      if (pthread_create(&workerThreadId, NULL, cputimeWorker,
                         (void *)cputimelog)) {
        log_println(0, "Cannot create worker thread for writing cpu usage!");
        workerThreadId = 0;
        memset(cputimelog, 0, sizeof(cputimelog));
      }
    }
    memset(webVarsValuesLog, 0, sizeof(webVarsValuesLog));
    if (webVarsValues) {
      snprintf(dir, sizeof(dir), "%s_%s:%d_%s.log", get_ISOtime(isoTime, sizeof(isoTime)), rmt_host, testPort, TCP_STAT_NAME);
      create_named_logdir(webVarsValuesLog, sizeof(webVarsValuesLog), dir, 0);
      if (sizeof(meta.web_variables_log) >= (strlen(dir) + 1)) {
        strncpy(meta.web_variables_log, dir, strlen(dir) + 1);
      } else {
        log_println(
            0,
            "Not enough space in meta.web_variables_log to store dir.");
      }
    }
  }

  // write the incoming connection data into the log file
  fp = fopen(get_logfile(), "a");
  if (fp == NULL) {
    log_println(0,
                "Unable to open log file '%s', continuing on without logging",
                get_logfile());
  } else {
    fprintf(fp, "%15.15s  %s port %d\n", ctime(&tt) + 4, rmt_host, testPort);
    if (cputime && workerThreadId) {
      log_println(1, "cputime trace file: %s\n", cputimelog);
      fprintf(fp, "cputime trace file: %s\n", cputimelog);
    }
    fclose(fp);
  }
  close(parent_pipe);

  // Set test options
  if (t_opts & TEST_MID) testopt.midopt = TOPT_ENABLED;
  if (t_opts & TEST_SFW) testopt.sfwopt = TOPT_ENABLED;
  if (t_opts & TEST_META) testopt.metaopt = TOPT_ENABLED;
  if (t_opts & TEST_C2S) testopt.c2sopt = TOPT_ENABLED;
  if (t_opts & TEST_S2C) testopt.s2copt = TOPT_ENABLED;
  if (global_extended_tests_allowed && t_opts & TEST_C2S_EXT) {
    testopt.c2sextopt = TOPT_ENABLED;
    alarm_time += options.c2s_duration / 1000.0;
  }
  if (global_extended_tests_allowed && t_opts & TEST_S2C_EXT) {
    testopt.s2cextopt = TOPT_ENABLED;
    alarm_time += options.s2c_duration / 1000.0;
  }
  // die in 120 seconds, but only if a test doesn't get started
  alarm(alarm_time);
  // reset alarm() before every test
  log_println(6,
              "setting master alarm() to 120 seconds, tests must start "
              "(complete?) before this timer expires");

  // run tests based on options
  if (strncmp(test_suite, "Invalid", 7) != 0) {
    log_println(3, "Valid test sequence requested, run test for client=%d",
                getpid());
    retcode = run_test(agent, &ctl, &testopt, test_suite, ssl_context);
  }

  // conclude all test runs
  if (retcode == 0) {
    log_println(3, "Successfully returned from run_test() routine");
  } else {
    log_println(3,
                "Child %d returned non-zero (%d) from run_test() results some "
                "test failed!",
                getpid(), retcode);
  }
  close_connection(&ctl);
#if USE_WEB100
  web100_detach(agent);
#elif USE_WEB10G
  estats_nl_client_destroy(&agent);
#endif

  if (cputime && workerThreadId) {
    cputimeworkerLoop = 0;
    pthread_join(workerThreadId, NULL);
  }
  exit(0);
}

static int max(int a, int b) { return (a > b) ? a : b; }

/**
 * Waits for a new client to connect, for one of the child processes to die, or
 * (optionally) until a timeout occurs.  If the select() is woken up by a new
 * client connecting, then this routine will return true.
 * @param listenfd The file descriptor which listens to the network
 * @param signalfd The pipe which will be written to upon receipt of SIGCHILD
 * @param wait_forever True if the server should wait forever
 * @return True if there is a new client on listenfd
 */
int wait_for_wakeup(int listenfd, int signalfd, int wait_forever) {
  fd_set fds;
  int retcode;
  ServerWakeupMessage signal_value;
  struct timeval sel_tv, *psel_tv = NULL;

  FD_ZERO(&fds);
  FD_SET(listenfd, &fds);
  FD_SET(signalfd, &fds);
  if (!wait_forever) {
    sel_tv.tv_sec = 3;  // Timeout after 3 seconds
    sel_tv.tv_usec = 0;
    psel_tv = &sel_tv;
  }
  retcode = select(max(listenfd, signalfd) + 1, &fds, NULL, NULL, psel_tv);
  if (retcode == -1) {
    if (errno != EINTR) {
      // EINTR is expected every now and then due to signal handling.
      log_println(0, "Error in server's select call: %d (%s)", errno,
                  strerror(errno));
    }
    return 0;
  }
  // If we woke up due to data on the signalfd, then clear the buffer.
  if (FD_ISSET(signalfd, &fds)) {
    read(signalfd, &signal_value, sizeof(ServerWakeupMessage));
  }
  return FD_ISSET(listenfd, &fds);
}

/**
 * There is a new client waiting to start on the listenfd socket. Accept the
 * connection, and fork off a new process to run all the associated tests.
 * @param listenfd The file descriptor which listens to the network
 * @param ssl_context The ssl_context for any new connections
 * @return A newly initialized ndtchild struct - the calling function
 *         owns the struct and its memory; NULL on error
 */
ndtchild *spawn_new_child(int listenfd, SSL_CTX *ssl_context) {
  int child_pipe[2];
  pid_t child_pid;
  int ctlsockfd;
  I2Addr cli_I2Addr;
  socklen_t cli_addr_len;
  struct sockaddr_storage cli_addr;
  size_t rmt_host_strlen;
  ndtchild *new_child = NULL;
  // Accept the connection, initialize variables, fire up the new child.
  cli_addr_len = sizeof(cli_addr);
  ctlsockfd = accept(listenfd, (struct sockaddr *)&cli_addr, &cli_addr_len);
  if (ctlsockfd < 0) {
    log_println(1, "accept() on ctlsockfd failed");
    return NULL;
  }
  if (cli_addr_len > sizeof(meta.c_addr)) {
    log_println(0, "cli_addr_len > sizeof(meta.c_addr). Should never happen");
    return NULL;
  }
  // Copy connection data into global variables for the run_test() function.
  // Get meta test details copied into results.
  memcpy(&meta.c_addr, &cli_addr, cli_addr_len);
  meta.family = ((struct sockaddr *)&cli_addr)->sa_family;

  memset(rmt_addr, 0, sizeof(rmt_addr));
  addr2a(&cli_addr, rmt_addr, sizeof(rmt_addr));

  // Get addr details based on socket info available.
  cli_I2Addr = I2AddrBySockFD(get_errhandle(), ctlsockfd, False);
  rmt_host_strlen = sizeof(rmt_host);
  memset(rmt_host, 0, rmt_host_strlen);
  I2AddrNodeName(cli_I2Addr, rmt_host, &rmt_host_strlen);
  log_println(4, "New connection received from 0x%x [%s] sockfd=%d.", cli_I2Addr,
              rmt_host, ctlsockfd);
  protolog_procstatus(getpid(), getCurrentTest(), CONNECT_TYPE, PROCESS_STARTED,
                      ctlsockfd);

  // Set up connection channel to the new child
  if (pipe(child_pipe) == -1) {
    log_println(6, "pipe() failed errno=%d", errno);
    close(ctlsockfd);
    return NULL;
  }

  // At this point we have received a connection from a client, meaning that
  // a test is being requested.  At this point we should apply any policy or
  // AAA functions to the incoming connection.  If we don't like the client,
  // we can refuse the connection and loop back to the begining.  There
  // would need to be some additional logic installed if this AAA test
  // relied on more than the client's IP address.  The client would also
  // require modification to allow more credentials to be created/passed
  // between the user and this application.

  child_pid = fork();
  if (child_pid == -1) {
    // An error occurred, log it and return.
    log_println(0, "fork() failed, errno = %d (%s)", errno, strerror(errno));
    close(child_pipe[0]);
    close(child_pipe[1]);
    close(ctlsockfd);
    return NULL;
  } else if (child_pid == 0) {
    // This is the child. Clean up and close the resources the child does not
    // need, then call the child_process routine which will initialize the
    // connection and then wait for the go/queue/nogo signals from the parent
    // process.
    log_println(4, "Child thinks pipe() returned fd0=%d, fd1=%d for pid=%d",
                child_pipe[0], child_pipe[1], child_pid);
    close(listenfd);
    close(child_pipe[1]);
    child_process(child_pipe[0], ssl_context, ctlsockfd);
    log_println(1, "The child returned! This should never happen.");
    exit(1);
  }

  // This is the parent process. Handle scheduling and queuing of multiple
  // incoming clients.
  log_println(5, "Parent process spawned child = %d", child_pid);
  log_println(5, "Parent thinks pipe() returned fd0=%d, fd1=%d", child_pipe[0],
              child_pipe[1]);

  // Close the open resources that should only be used by the child.
  close(child_pipe[0]);
  close(ctlsockfd);

  // Initialize the members of new_child
  new_child = (ndtchild *)calloc(1, sizeof(ndtchild));
  if (new_child == NULL) {
    log_println(1, "calloc() failed errno=%d", errno);
    close(child_pipe[1]);
    kill(child_pid, SIGKILL);
    return NULL;
  }
  new_child->pid = child_pid;
  new_child->pipe = child_pipe[1];
  strlcpy(new_child->addr, rmt_addr, sizeof(new_child->addr));
  strlcpy(new_child->host, rmt_host, sizeof(new_child->host));
  new_child->qtime = time(0);
  new_child->running = 0;
  new_child->next = NULL;

  return new_child;
}

/**
 * Returns true when the current queue size indicates that a new client would
 * overload the server.
 */
int server_queue_is_full(int queue_size) {
  if (queue) {
    if (multiple) {
      return queue_size >= QUEUE_SIZE_MULTIPLIER * max_clients;
    } else {
      return queue_size >= max_clients;
    }
  } else {
    return queue_size >= 1;
  }
}

/**
 * Removes the element after okay_child from the list.  The calling function
 * owns the memory containing the removed element.
 * @param head The head of the list - may not be NULL or point to NULL
 * @param okay_child The element before the element to remove - may be NULL if
 *                   the head of the list is the element to be removed.
 */
void remove_next_child(ndtchild **head, ndtchild *okay_child) {
  if (okay_child == NULL) {
    // We are removing the head
    *head = (*head)->next;
  } else {
    // We are removing a non-head element
    okay_child->next = okay_child->next->next;
  }
}

/**
 * Enqueues a new child onto the list.
 * @param head The head of the list
 * @param new_child The child to enqueue
 */
void enqueue_child(ndtchild **head, ndtchild *new_child) {
  ndtchild *current;
  if (*head == NULL) {
    // Enqueue onto the empty list
    *head = new_child;
  } else {
    // Enqueue onto the end of a non-empty list
    current = *head;
    while (current->next != NULL) current = current->next;
    current->next = new_child;
  }
  new_child->next = NULL;
}

/**
 * Sends a message to the child process.
 * @param child_pipe The file descriptor of the pipe to write to
 * @param message The message to send
 */
void send_message_to_child(int child_pipe, ParentChildSignal message) {
  write(child_pipe, &message, sizeof(ParentChildSignal));
}

/**
 * Attempts to enqueue a new ndtchild struct onto the queue of clients to be
 * run. This will not enqueue the child unless the server has spare capacity.
 * This function takes ownership of the memory pointed to by new_child, and
 * either frees that memory or passes ownership to the linked list that starts
 * at *head.
 * @param new_child The new client
 * @param head The head of the client queue
 */
void attempt_enqueue(ndtchild *new_child, ndtchild **head) {
  ndtchild *current;
  int client_count = 0;
  // Count the number of connected clients
  for (current = *head; current != NULL; current = current->next) {
    client_count++;
  }

  if (!server_queue_is_full(client_count)) {
    // The server is not overloaded, so queue up the new client.
    enqueue_child(head, new_child);
  } else {
    // The server is overloaded. Reject the client and discard it.
    log_println(0,
                "Too many clients/mclients (%d) waiting to be served, "
                "Please try again later.",
                new_child->pid);
    send_message_to_child(new_child->pipe, SRV_QUEUE_SERVER_BUSY);
    // Whether the write succeeds or not, we are done with this child. It
    // will either kill itself in response to this message, or it will kill
    // itself with its own watchdog timer. Either way, not our problem
    // anymore. Send the message, close the pipe, and move on.
    log_println(6, "Too many clients, freeing child=0x%x", new_child);
    free_ndtchild(&new_child);
  }
}

/**
 * Returns true if a process is alive and a child of the current process.
 */
int is_child_process_alive(pid_t child_pid) {
  siginfo_t child_status = {0};
  int rv;
  rv = waitid(P_PID, child_pid, &child_status, WEXITED | WSTOPPED | WNOHANG);
  if (rv == 0 && child_status.si_pid == 0) {
    // There is a running process with that PID, but is it one of ours?
    return getpgid(child_pid) == getpgid(getpid());
  }
  return 0;
}

/**
 * Walk the list of connected clients, reaping those clients with dead PIDs.
 * @param head The head of the list of clients
 */
void reap_dead_clients(ndtchild **head) {
  ndtchild *current, *previous, *client_to_free;
  current = *head;
  previous = NULL;
  while (current != NULL) {
    if (is_child_process_alive(current->pid)) {
      previous = current;
      current = current->next;
    } else {
      // Free the dead client and set *current to the next unexamined element
      // in the list.
      client_to_free = current;
      current = current->next;
      remove_next_child(head, previous);
      free_ndtchild(&client_to_free);
    }
  }
}

/**
 * Returns the number of tests that may be run at the same time.
 */
int max_simultaneous_tests() {
  if (!queue || !multiple) {
    return 1;
  } else {
    return max_clients;
  }
}

/**
 * Reaps dead clients from the queue of clients and then updates all clients
 * regarding their (possibly new) queue position and starts any new clients for
 * whom capacity has opened up on the server.
 */
void perform_queue_maintenance(ndtchild **head) {
  int queue_position;
  int running_count = 0;
  ndtchild *current;

  reap_dead_clients(head);

  // Count the number of clients and the number of running clients.
  for (current = *head; current != NULL; current = current->next) {
    if (current->running) {
      running_count++;
    }
  }

  // Walk the list of clients, sending the GO signal to clients that have
  // reached the front part of the queue.
  queue_position = 0;
  for (current = *head; current != NULL; current = current->next) {
    if (!current->running) {
      if (running_count < max_simultaneous_tests()) {
        send_message_to_child(current->pipe, SRV_QUEUE_TEST_STARTS_NOW);
        current->running = 1;
        running_count++;
      } else {
        // Assuming we can service max_simultaneous_tests per minute, then the
        // amount of time (in minutes) that the child should expect to wait is
        // equal to the number of tests ahead of it, divided by the number of
        // tests we can service per minute. If the children are receiving too
        // many updates, don't change the update frequency here. Instead,
        // change how the children respond to updates in child_process().
        send_message_to_child(
            current->pipe, max(queue_position / max_simultaneous_tests(), 1));
        send_message_to_child(current->pipe, SRV_QUEUE_HEARTBEAT);
      }
    }
    queue_position++;
  }
}

/**
 * The server's main loop.  This is the function that, once all arguments are
 * processed and the server environment has been set up, will keep waiting for
 * new connections and then forking off children to handle those connections.
 * @param ssl_context The context to create new SSL connections - may be NULL
 * @param listenfd The server socket on which to listen for new clients
 * @param signalfd The socket used to wake up the server after a signal handler
 */
void NDT_server_main_loop(SSL_CTX *ssl_context, int listenfd, int signalfd) {
  ndtchild *queue_head = NULL, *new_child;
  for (;;) {
    // Wait for a new connection, an interruption, or a timeout.
    if (wait_for_wakeup(listenfd, signalfd, (queue_head == NULL))) {
      new_child = spawn_new_child(listenfd, ssl_context);
      if (new_child != NULL) attempt_enqueue(new_child, &queue_head);
    }
    // Perform queue maintenance: send messages to clients and reap the dead.
    perform_queue_maintenance(&queue_head);
  }
}

/**
 * Retrieve the error message from OpenSSL, print it out, and exit the process.
 * @param prefix The text to print out before the message
 * @param message An extra error message, to give context to the error
 */
void report_SSL_error(const char *prefix, const char *message) {
  char ssl_error_message[256];
  unsigned long ssl_error;
  ssl_error = ERR_get_error();
  if (ssl_error != 0) {
    ERR_error_string_n(ssl_error, ssl_error_message, sizeof(ssl_error_message));
    log_println(0, "%s: %s", prefix, ssl_error_message);
  }
  log_println(0, "%s: %s", prefix, message);
  exit(-1);
}

/**
 * Set things up so that the server can accept incoming TLS connections.
 * @param certificate_file The filename containing the certificate (.pem)
 * @param private_key_file The filename containing the private key (.pem)
 */
SSL_CTX *setup_SSL(const char *certificate_file, const char *private_key_file) {
  SSL_CTX *ssl_context;

  SSL_library_init();
  OpenSSL_add_all_algorithms();
  SSL_load_error_strings();
  ssl_context = SSL_CTX_new(SSLv23_server_method());
  if (ssl_context == NULL) {
    report_SSL_error("SSL_CTX_new", "SSL/TLS context could not be created.");
  }
  SSL_CTX_set_mode(ssl_context, SSL_MODE_AUTO_RETRY);
  if (SSL_CTX_use_certificate_chain_file(ssl_context, certificate_file
                                   ) != 1) {
    report_SSL_error("SSL_CTX_use_certificate_chain_file",
                     "SSL/TLS certificate file could not be loaded.");
  }
  if (SSL_CTX_use_PrivateKey_file(ssl_context, private_key_file,
                                  SSL_FILETYPE_PEM) != 1) {
    report_SSL_error("SSL_CTX_use_PrivateKey_file",
                     "SSL/TLS private key file could not be loaded.");
  }
  if (!SSL_CTX_check_private_key(ssl_context)) {
    report_SSL_error("SSL_CTX_check_private_key",
                     "Private key and certificate do not match");
  }
  return ssl_context;
}

/* web100srv.c contains both a main() that runs things, but is also a source of
 * library code run by other parts of the program.  In order to test those
 * other parts, we must be able to compile this file without the main()
 * function.  To use this file as a library, pass in
 * -DUSE_WEB100SRV_ONLY_AS_LIBRARY as a compile-time option.
 */
#ifndef USE_WEB100SRV_ONLY_AS_LIBRARY
/**
 * Initialize data structures, web100 structures and logging systems.
 * Read/load configuration, get process execution options. Accept test requests
 * and manage their execution order and initiate tests. Keep track of running
 * processes.
 * @param argc Number of arguments
 * @param argv String command line arguments
 */
int main(int argc, char **argv) {
  int c, i;
  struct sigaction new;
  char *lbuf = NULL, *ctime();
  FILE *fp;
  size_t lbuf_max = 0;
  char *private_key_file = NULL;
  char *certificate_file = NULL;
  SSL_CTX *ssl_context = NULL;
  time_t tt;
  I2Addr listenaddr = NULL;
  int listenfd;
  char *srcname = NULL;
  int signalfd_pipe[2];
  int signalfd_read;
  int debug = 0;

  // variables used for protocol validation logs
  // char startsrvmsg[256];  // used to log start of server process
  // char *srvstatusdesc;
  // enum PROCESS_STATUS_INT srvstatusenum = UNKNOWN;
  // // temp storage for process name
  // char statustemparr[PROCESS_STATUS_DESC_SIZE];

  memset(&options, 0, sizeof(options));
  options.snapDelay = 5;
  options.avoidSndBlockUp = 0;
  options.snaplog = 0;
  options.snapshots = 1;
  options.cwndDecrease = 0;
  for (i = 0; i < MAX_STREAMS; i++)
    memset(options.s2c_logname[i], 0, 256);
  memset(options.c2s_logname, 0, sizeof(options.c2s_logname));
  options.c2s_duration = 10000;
  options.c2s_throughputsnaps = 0;
  options.c2s_snapsdelay = 5000;
  options.c2s_snapsoffset = 1000;
  options.c2s_streamsnum = 1;
  options.s2c_duration = 10000;
  options.s2c_throughputsnaps = 0;
  options.s2c_snapsdelay = 5000;
  options.s2c_snapsoffset = 1000;
  options.s2c_streamsnum = 1;

  peaks.min = -1;
  peaks.max = -1;
  peaks.amount = -1;
  DataDirName = NULL;

  memset(&testopt, 0, sizeof(testopt));

#ifdef AF_INET6
#define GETOPT_LONG_INET6(x) "46"x
#else
#define GETOPT_LONG_INET6(x) x
#endif

#ifdef EXPERIMENTAL_ENABLED
#define GETOPT_LONG_EXP(x) "y:"x
#else
#define GETOPT_LONG_EXP(x) x
#endif

  opterr = 0;
  while (
      (c = getopt_long(argc, argv, GETOPT_LONG_INET6(GETOPT_LONG_EXP(
                                       "adhmoqrstvzc:x:b:f:i:l:u:e:p:T:A:S:")),
                       long_options, 0)) != -1) {
    switch (c) {
      case 'c':
        ConfigFileName = optarg;
        break;
      case 'd':
        debug++;
        break;
    }
  }

  // Initialize logging system, and then read configuration
  log_init(argv[0], debug);

  if (ConfigFileName == NULL) ConfigFileName = CONFIGFILE;

  // Load Config file.
  // lbuf/lbuf_max keep track of a dynamically grown "line" buffer.
  // (It is grown using realloc.)
  // This will be used throughout all the config file reading and
  // should be free'd once all config files have been read.

  opterr = optind = 1;
  LoadConfig(argv[0], &lbuf, &lbuf_max);
  debug = 0;

  // set options for test direction
  enum Tx_DIRECTION currentDirn = S_C;
  setCurrentDirn(currentDirn);
  // end protocol logging

#if USE_WEB10G
  log_println(0,
              "WARNING: The Web10G NDT server is still in testing"
              " and may contain bugs.");
#endif
  // Get server execution options
  while (
      (c = getopt_long(argc, argv, GETOPT_LONG_INET6(GETOPT_LONG_EXP(
                                       "adhmoqrstvzc:x:b:f:i:l:u:e:p:T:A:S:")),
                       long_options, 0)) != -1) {
    switch (c) {
      case '4':
        conn_options |= OPT_IPV4_ONLY;
        break;
      case '6':
        conn_options |= OPT_IPV6_ONLY;
        break;
      case 'r':
        record_reverse = 1;
        break;
      case 'h':
        srv_long_usage("ANL/Internet2 NDT version " VERSION " (server)");
        break;
      case 'v':
        printf("ANL/Internet2 NDT version %s (server)\n", VERSION);
        exit(0);
        break;
      case 'p':
        port = optarg;

        if (check_int(port, &testopt.mainport)) {
          char tmpText[200];
          snprintf(tmpText, sizeof(tmpText), "Invalid primary port number: %s",
                   optarg);
          short_usage(argv[0], tmpText);
        }
        break;
      case 302:
        if (check_int(optarg, &testopt.midsockport)) {
          char tmpText[200];
          snprintf(tmpText, sizeof(tmpText),
                   "Invalid Middlebox test port number: %s", optarg);
          short_usage(argv[0], tmpText);
        }
        break;
      case 303:
        if (check_int(optarg, &testopt.c2ssockport)) {
          char tmpText[200];
          snprintf(tmpText, sizeof(tmpText),
                   "Invalid C2S throughput test port number: %s", optarg);
          short_usage(argv[0], tmpText);
        }
        break;
      case 304:
        if (check_int(optarg, &testopt.s2csockport)) {
          char tmpText[200];
          snprintf(tmpText, sizeof(tmpText),
                   "Invalid S2C throughput test port number: %s", optarg);
          short_usage(argv[0], tmpText);
        }
        break;
      case 'a':
        admin_view = 1;
        break;
      case 'f':
#if USE_WEB100
        VarFileName = optarg;
#elif USE_WEB10G
        log_println(2, "Web10G doesn't require varfile. Ignored.");
#endif
        break;
      case 'i':
        device = optarg;
        break;
      case 'l':
        set_logfile(optarg);
        break;
      case 'u':
        set_protologdir(optarg);
        break;
      case 'e':
        log_println(7, "Enabling protocol logging");
        enableprotocollogging();
        break;
      case 'o':
        old_mismatch = 1;
        break;
      case 301:
        if (mrange_parse(optarg)) {
          char tmpText[300];
          snprintf(tmpText, sizeof(tmpText), "Invalid range: %s", optarg);
          short_usage(argv[0], tmpText);
        }
      case 'z':
        compress = 0;
        break;
      case 'm':
        multiple = 1;
        break;
      case 'x':
        max_clients = atoi(optarg);
        break;
      case 'q':
        queue = 0;
        break;
      case 's':
        usesyslog = 1;
        break;
      case 't':
        dumptrace = 1;
        break;
      case 'b':
        if (check_int(optarg, &window)) {
          char tmpText[200];
          snprintf(tmpText, sizeof(tmpText), "Invalid window size: %s", optarg);
          short_usage(argv[0], tmpText);
        }
        set_buff = 1;
        break;
      case 'd':
        debug++;
        break;
      case 305:
        options.snapDelay = atoi(optarg);
        break;
      case 'y':
        options.limit = atoi(optarg);
        break;
      case 306:
        options.avoidSndBlockUp = 1;
        break;
      case 308:
        options.cwndDecrease = 1;
      case 307:
        options.snaplog = 1;
        break;
      case 309:
        cputime = 1;
        break;
      case 310:
        useDB = 1;
        break;
      case 311:
        dbDSN = optarg;
        break;
      case 312:
        dbUID = optarg;
        break;
      case 313:
        dbPWD = optarg;
        break;
      case 314:
        options.c2s_duration = atoi(optarg);
        break;
      case 315:
        options.c2s_throughputsnaps = 1;
        break;
      case 316:
        options.c2s_snapsdelay = atoi(optarg);
        break;
      case 317:
        options.c2s_snapsoffset = atoi(optarg);
        break;
      case 322:
        if (check_rint(optarg, &options.c2s_streamsnum, 1, 7)) {
          char tmpText[200];
          snprintf(tmpText, sizeof(tmpText), "Invalid number of streams for upload test: %s", optarg);
          short_usage(argv[0], tmpText);
        }
        break;
      case 318:
        options.s2c_duration = atoi(optarg);
        break;
      case 319:
        options.s2c_throughputsnaps = 1;
        break;
      case 320:
        options.s2c_snapsdelay = atoi(optarg);
        break;
      case 321:
        options.s2c_snapsoffset = atoi(optarg);
        break;
      case 323:
        if (check_rint(optarg, &options.s2c_streamsnum, 1, 7)) {
          char tmpText[200];
          snprintf(tmpText, sizeof(tmpText), "Invalid number of streams for download test: %s", optarg);
          short_usage(argv[0], tmpText);
        }
        break;
      case 324:
        webVarsValues = 1;
        break;
      case 'T':
        refresh = atoi(optarg);
        break;
      case 'A':
        AdminFileName = optarg;
        break;
      case 'L':
        DataDirName = optarg;
        break;
      case 'S':
        SysLogFacility = optarg;
        break;
      case 325:
        options.tls = 1;
        break;
      case 326:
        private_key_file = optarg;
        break;
      case 327:
        certificate_file = optarg;
        break;
      case 328:
        global_extended_tests_allowed = 0;
        break;
      case '?':
        short_usage(argv[0], "");
        break;
    }
  }

  if (options.tls) {
    if (private_key_file == NULL || certificate_file == NULL) {
      short_usage(argv[0], "TLS requires a private key and a certificate");
    }
    ssl_context = setup_SSL(certificate_file, private_key_file);
  }

  if (optind < argc) {
    short_usage(argv[0], "Unrecognized non-option elements");
  }

  if (debug > get_debuglvl()) {
    set_debuglvl(debug);
  }

  testopt.multiple = multiple;

  // First check to see if program is running as root.  If not, then warn
  // the user that link type detection is suspect.  Then downgrade effective
  // userid to non-root user until needed.

  if (getuid() != 0) {
    log_print(
        1, "Warning: This program must be run as root to enable the Link Type");
    log_println(
        1,
        " detection algorithm.\nContinuing execution without this algorithm");
  }

  if (VarFileName == NULL) {
    snprintf(wvfn, sizeof(wvfn), "%s/%s", BASEDIR, WEB100_FILE);
    VarFileName = wvfn;
  }

  if (AdminFileName == NULL) {
    snprintf(apfn, sizeof(apfn), "%s/%s", BASEDIR, ADMINFILE);
    AdminFileName = apfn;
  }

  if (DataDirName == NULL) {
    snprintf(logd, sizeof(logd), "%s/%s/", BASEDIR, LOGDIR);
    DataDirName = logd;
  } else if (DataDirName[0] != '/') {
    snprintf(logd, sizeof(logd), "%s/%s/", BASEDIR, DataDirName);
    DataDirName = logd;
  }

  create_protolog_dir();

  if (SysLogFacility != NULL) {
    i = 0;
    while (facilitynames[i].c_name) {
      if (strcmp(facilitynames[i].c_name, SysLogFacility) == 0) {
        syslogfacility = facilitynames[i].c_val;
        break;
      }
      ++i;
    }
    if (facilitynames[i].c_name == NULL) {
      log_println(
          0, "Warning: Unknown syslog facility [%s] --> using default (%d)",
          SysLogFacility, syslogfacility);
      SysLogFacility = NULL;
    }
  }

  log_println(1, "ANL/Internet2 NDT ver %s", VERSION);
  log_println(1, "\tVariables file = %s\n\tlog file = %s", VarFileName,
              get_logfile());
  if (admin_view) {
    log_println(1, "\tAdmin file = %s", AdminFileName);
  }
  if (usesyslog) {
    log_println(1, "\tSyslog facility = %s (%d)",
                SysLogFacility ? SysLogFacility : "default", syslogfacility);
  }
  log_println(1, "\tDebug level set to %d", debug);

  log_println(3, "\tExtended tests options:");
  log_println(3, "\t\t * upload: duration = %d, streams = %d, throughput snapshots: enabled = %s, delay = %d, offset = %d",
              options.c2s_duration, options.c2s_streamsnum, options.c2s_throughputsnaps ? "true" : "false",
              options.c2s_snapsdelay, options.c2s_snapsoffset);
  log_println(3, "\t\t * download: duration = %d, streams = %d, throughput snapshots: enabled = %s, delay = %d, offset = %d",
              options.s2c_duration, options.s2c_streamsnum, options.s2c_throughputsnaps ? "true" : "false",
              options.s2c_snapsdelay, options.s2c_snapsoffset);

  initialize_db(useDB, dbDSN, dbUID, dbPWD);

  memset(&new, 0, sizeof(new));
  new.sa_handler = cleanup;

  // Grab all signals and run them through the cleanup routine.
  for (i = 1; i < 32; i++) {
    if ((i == SIGKILL) || (i == SIGSTOP)) continue;
    sigaction(i, &new, NULL);
  }

  // Bind our local address so that the client can send to us.

  if (srcname && !(listenaddr = I2AddrByNode(get_errhandle(), srcname))) {
    err_sys("server: Invalid source address specified");
  }
  if ((listenaddr = CreateListenSocket(listenaddr, port, conn_options,
                                       ((set_buff > 0) ? window : 0))) ==
      NULL) {
    err_sys("server: CreateListenSocket failed");
  }
  listenfd = I2AddrFD(listenaddr);

  if (listenfd == -1) {
    log_println(0, "ERROR: Socket already in use.");
    return 0;
  }

  log_println(1, "server ready on port %s (family %d)", port, meta.family);

  // Initialize tcp_stat structures
  count_vars = tcp_stat_init(VarFileName);
  if (count_vars == -1) {
    log_println(0, "No Web100 variables file found, terminating program");
    exit(-5);
  }

  // The administrator view automatically generates a usage page for the NDT
  // server.  This page is then accessible to the general public.  At this
  // point read the existing log file and generate the necessary data.  This
  // data is then updated at the end of each test.
  // RAC 11/28/03

  if (admin_view == 1) view_init(refresh);

  // Get the server's metadata info (OS name and kernel version
  // RAC 7/7/09

  if ((fp = fopen("/proc/sys/kernel/hostname", "r")) == NULL) {
    log_println(0, "Unable to determine server Hostname.");
  } else {
    fscanf(fp, "%s", meta.server_name);
    fclose(fp);
  }
  if ((fp = fopen("/proc/sys/kernel/osrelease", "r")) == NULL) {
    log_println(0, "Unable to determine server OS Version.");
  } else {
    fscanf(fp, "%s", meta.server_os);
    fclose(fp);
  }

  // create a log file entry every time the web100srv process starts up
  ndtpid = getpid();
  tt = time(0);
  log_println(6, "NDT server (v%s) process [%d] started at %15.15s", VERSION,
              ndtpid, ctime(&tt) + 4);
  fp = fopen(get_logfile(), "a");
  if (fp == NULL) {
    log_println(0,
                "Unable to open log file '%s', continuing on without logging",
                get_logfile());
  } else {
    fprintf(fp, "Web100srv (ver %s) process (%d) started %15.15s\n", VERSION,
            ndtpid, ctime(&tt) + 4);
    fclose(fp);
  }
  if (usesyslog == 1)
    syslog(LOG_FACILITY | LOG_INFO, "Web100srv (ver %s) process started",
           VERSION);

  // Make the server part of a process group.
  setpgid(0, 0);
  options.compress = compress;
  pipe(signalfd_pipe);
  signalfd_read = signalfd_pipe[0];
  global_signalfd_write = signalfd_pipe[1];

  NDT_server_main_loop(ssl_context, listenfd, signalfd_read);
  return 0;
}
#endif  // USE_WEB100SRV_ONLY_AS_LIBRARY

/**
 * Method to get remote host's address.
 * @return remote host's address
 */
char *get_remotehostaddress() { return rmt_addr; }
