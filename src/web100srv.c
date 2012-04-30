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
 nonexclusive license, subject to the limitations stated herein and U.S. Government
 license rights.

 3. You may modify and make a copy or copies of the Software for use within your
 organization, if you meet the following conditions: 
 a. Copies in source code must include the copyright notice and this Software
 License Agreement.
 b. Copies in binary form must include the copyright notice and this Software
 License Agreement in the documentation and/or other materials provided with the copy.

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

#include <time.h>
#include <ctype.h>
#include <math.h>
#define SYSLOG_NAMES
#include  <syslog.h>
#include <pthread.h>
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

static char lgfn[FILENAME_SIZE]; // log file name
static char wvfn[FILENAME_SIZE]; // file name of web100-variables list
static char apfn[FILENAME_SIZE]; // admin file name
static char slfa[256]; // syslog facility
static char logd[256]; // log dir name
static char portbuf[10]; // port number user option store
static char devicebuf[100]; // device name buf (seems unused)
static char dbDSNbuf[256]; // DB datasource name
static char dbUIDbuf[256]; // DB UID
static char dbPWDbuf[256]; // DB Password

// list of global variables used throughout this program.
int window = 64000; // TCP buffer size
int randomize = 0;
int count_vars = 0;
int dumptrace = 0;
int usesyslog = 0;
int multiple = 0;
int compress = 1;
int max_clients = 50;
int set_buff = 0;
int admin_view = 0;
int queue = 1;
int view_flag = 0;
int record_reverse = 0;
int testing; // a test is currently being performed.
int waiting; // # of many tests pending
int mclients; // multiple client mode client count
int refresh = 30;
int old_mismatch = 0; /* use the old duplex mismatch detection heuristic */
/* int sig1, sig2, sig17; */

Options options;
CwndPeaks peaks;
int cputime = 0;
char cputimelog[256];
pthread_t workerThreadId, zombieThreadId;
int cputimeworkerLoop = 1, zombie_check = 0;

int useDB = 0;
char* dbDSN = NULL;
char* dbUID = NULL;
char* dbPWD = NULL;

char *VarFileName = NULL;
char *AdminFileName = NULL;
char *SysLogFacility = NULL;
int syslogfacility = LOG_FACILITY;
char *ProcessName = { "web100srv" };
char *ConfigFileName = NULL;
char buff[BUFFSIZE + 1];char*rmt_host;
char spds[4][256], buff2[32];
char *device = NULL;
char* port = PORT;
TestOptions testopt;

pcap_t *pd;
pcap_dumper_t *pdump;
float run_ave[4];

int conn_options = 0;
struct ndtchild *head_ptr;
int ndtpid;
int testPort;
char testName[256];

#include <semaphore.h>
sem_t ndtq; /* create semaphore to allow only 1 process to modify the wait queue */

static struct option long_options[] = { { "adminview", 0, 0, 'a' }, { "debug",
		0, 0, 'd' }, { "help", 0, 0, 'h' }, { "multiple", 0, 0, 'm' }, {
		"max_clients", 1, 0, 'x' }, { "mrange", 1, 0, 301 },
		{ "old", 0, 0, 'o' }, { "disable-queue", 0, 0, 'q' }, { "record", 0, 0,
				'r' }, { "syslog", 0, 0, 's' }, { "tcpdump", 0, 0, 't' }, {
				"version", 0, 0, 'v' }, { "gzip", 0, 0, 'z' }, { "config", 1, 0,
				'c' },
#ifdef EXPERIMENTAL_ENABLED
		{	"avoidsndblockup", 0, 0, 306},
		{	"snaplog", 0, 0, 307},
		{	"snapdelay", 1, 0, 305},
		{	"cwnddecrease", 0, 0, 308},
		{	"cputime", 0, 0, 309},
		{	"limit", 1, 0, 'y'},
#endif
		{ "buffer", 1, 0, 'b' },
		{ "file", 1, 0, 'f' },
		{ "interface", 1, 0, 'i' },
		{ "log", 1, 0, 'l' },
		{ "protocol_log", 1, 0, 'u' },
		{ "enableprotolog", 0, 0, 'e' },
		{ "port", 1, 0, 'p' },
		{ "midport", 1, 0, 302 },
		{ "c2sport", 1, 0, 303 },
		{ "s2cport", 1, 0, 304 },
		{ "refresh", 1, 0, 'T' },
		{ "adminfile", 1, 0, 'A' },
		{ "log_dir", 1, 0, 'L' },
		{ "logfacility", 1, 0, 'S' },
#if defined(HAVE_ODBC) && defined(DATABASE_ENABLED) && defined(HAVE_SQL_H)
		{	"enableDBlogging", 0, 0, 310},
		{	"dbDSN", 1, 0, 311},
		{	"dbUID", 1, 0, 312},
		{	"dbPWD", 1, 0, 313},
#endif
#ifdef AF_INET6
		{	"ipv4", 0, 0, '4'},
		{	"ipv6", 0, 0, '6'},
#endif
		{ 0, 0, 0, 0 } };

/**
 * Process a SIGCHLD signal.
 * @param pid_t Process id to be processed
 * */
void child_sig(pid_t chld_pid) {
	int pid, status, retcode;
	struct ndtchild *child_proc1, *child_proc2;

	child_proc1 = head_ptr;
	log_println(
			2,
			"Processing SIGCHLD signal for active web100srv process [%d], sig17=%d",
			chld_pid, sig17);

	/* this routine cleans up after a child process has terminated.  There are 2 types of
	 * child processes.  The pkt-pair timing children are type 1 and the spawned children to run
	 * the test are type 0.  For the pkt-pair children, just acknowledge their signal to keep
	 * them from becoming defunct.  For the type 0 children, use wait3() to figure out which
	 * child terminated and then clean up the FIFO queue.
	 * RAC 2/8/10
	 *
	 * Added new type (-1) on 2/11/10.  This type indicates a child has terminated due to
	 * a communications fault.  For some reason the child is stuck in the FIFO queue and
	 * it needs to be removed.  In this case, the calling function will grab the head pointer's
	 * PID value, call this function with a -1 to remove the entry from the FIFO, issue a 
	 * SIGTERM signal and call this function with the childs PID.  This should prevent the
	 * code from entering into a loop.
	 * RAC 2/11/10
	 */
	if (chld_pid > 0) {
		while ((pid = wait4(chld_pid, &status, WNOHANG, (struct rusage *) 0))
				> -1) {
			log_println(3, "wait4() returned %d for PID=%d", status, pid);
			if (WIFEXITED(status) != 0) {
				log_println(3, "wexitstatus = '%d'", WEXITSTATUS(status));
				break;
			}
			if (WIFSIGNALED(status) == 1) {
				log_println(3, "wtermsig = %d", WTERMSIG(status));
			}
			if (WIFSTOPPED(status) == 1) {
				log_println(3, "wstopsig = %d", WSTOPSIG(status));
			}
			if (status != 0) {
				log_println(
						4,
						"child_sig() routine, wait4() non-zero status (%d) returned",
						status);
				return;
			}
		}
		return;
	}

	if (chld_pid == 0) {
		while ((pid = wait3(&status, WNOHANG, (struct rusage*) 0)) > 0) {
			log_println(3, "wait3() returned status=%d for PID=%d", status,
					pid);
			if (WIFEXITED(status) != 0) {
				log_println(3, "wexitstatus = '%d'", WEXITSTATUS(status));
				break;
			}
			if (WIFSIGNALED(status) == 1) {
				log_println(3, "wtermsig = %d", WTERMSIG(status));
			}
			if (WIFSTOPPED(status) == 1) {
				log_println(3, "wstopsig = %d", WSTOPSIG(status));
			}
			if (status != 0) {
				log_println(
						4,
						"child_sig() routine, wait3() non-zero status (%d) returned",
						status);
				return;
			}
		}
		log_println(6,
				"child_sig() called pid=%d, wait returned child=%d - status=%d",
				chld_pid, pid, status);
		if (pid == 0) {
			log_println(6, "wait3() failed to return non-zero PID, ignore it");
			if (sig17 > 0)
				sig17--;
			return;
		}
	} else { /* chld_pid must be -1, the error condition */
		pid = head_ptr->pid;
		log_println(
				6,
				"Stuck child at head of queue, set pid=%d and remove it from queue",
				pid);
	}

	/* the pid variable now holds the PID of the child that exited.  Find out if this is one
	 * of the queued children, and if so remove it from the queue and relink everything.  At
	 * the end of this, update the sig17 counter.
	 * RAC 2/8/10
	 */

	if (head_ptr == NULL)
		return;
	log_println(5, "checking for pktpair timing children, skip them");
	child_proc1 = head_ptr;
	while (child_proc1 != NULL) {
		log_println(
				5,
				"\tLooking for %d, curent queue Child %d, host: %s [%s], next=0x%x",
				pid, child_proc1->pid, child_proc1->host, child_proc1->addr,
				(u_int64_t) child_proc1->next);
		if (child_proc1->pid == pid) {
			log_println(4, "Main test process %d terminated, remove from queue",
					pid);
			break;
		}
		child_proc1 = child_proc1->next;
	}
	if (child_proc1 == NULL)
		return;

	reap_child: if (multiple == 1) {
		log_println(5, "mclient child '%d' (%d) has finished its test",
				mclients, pid);
		mclients--;
		/*     return; */
	}

	log_println(4, "Attempting to clean up child %d, head pid = %d", pid,
			head_ptr->pid);
	if (head_ptr->pid == pid) {

		if (get_debuglvl() > 5) {
			log_println(5, "Walkingqueue");
			child_proc1 = head_ptr;
			while (child_proc1 != NULL) {
				log_println(5, "\tChild %d, host: %s [%s], next=0x%x",
						child_proc1->pid, child_proc1->host, child_proc1->addr,
						(u_int64_t) child_proc1->next);
				if (child_proc1->next == NULL)
					break;
				child_proc1 = child_proc1->next;
			}
		}

		while ((retcode = sem_wait(&ndtq)) == -1 && errno == EINTR) {
			log_println(6, "Waiting for ndtq semaphore to free - 1");
			continue;
		}
		log_println(
				5,
				"Child process %d causing head pointer modification, semaphore locked",
				pid);
		if (head_ptr != NULL) {
			child_proc1 = head_ptr;
			log_println(6, "modifying queue child_proc1=0x%x, head_ptr=0x%x",
					child_proc1, head_ptr);
			head_ptr = head_ptr->next;
			log_println(6, "free child_proc1=0x%x", child_proc1);
			free(child_proc1);
		}
		if (head_ptr == NULL)
			testing = 0;
		if (multiple == 0)
			testing = 0;
		waiting--;
		log_println(3,
				"Removing Child from head, decremented waiting/mclients %d/%d",
				waiting, mclients);
		sem_post(&ndtq);
		log_println(6, "Free'd ndtq semaphore lock - 3");
		if (sig17 > 0)
			sig17--;
		return;
	} else {
		child_proc1 = head_ptr;
		while (child_proc1->next != NULL) {
			if (child_proc1->next->pid == pid) {
				while ((retcode = sem_wait(&ndtq)) == -1 && errno == EINTR) {
					log_println(6, "Waiting for ndtq semaphore to free - 2");
					continue;
				}
				log_println(
						4,
						"Child process %d causing task list modification, semaphore locked",
						chld_pid);
				child_proc2 = child_proc1->next;
				child_proc1->next = child_proc2->next;
				log_println(6, "free child_proc2=0x%x", child_proc2);
				free(child_proc2);
				/* testing = 0; */
				waiting--;
				log_println(
						3,
						"Removing Child from list, decremented waiting/mclients %d/%d",
						waiting, mclients);
				sem_post(&ndtq);
				log_println(6, "Free'd ndtq semaphore lock - 4");
				if (sig17 > 0)
					sig17--;
				return;
			}
			child_proc1 = child_proc1->next;
			log_println(6, "Looping through service queue ptr = 0x%x",
					(u_int64_t) child_proc1);
		}
	}
	if (sig17 > 0)
		sig17--;
	log_println(3,
			"SIGCHLD routine finished!, decremented sig17 counter now =%d",
			sig17);
	return;
	if (pid > 0) {
		chld_pid = head_ptr->pid;
		log_println(
				6,
				"stuck process or other problem, removing pointer from head of queue");
		goto reap_child;
	}
	log_println(6, "Did we get here???");
}

/**
 * Catch termination signal(s) and print message in log file
 * @param signo Signal number
 * */
void cleanup(int signo) {
	FILE * fp;

	log_println(1, "Signal %d received by process %d", signo, getpid());
	if (get_debuglvl() > 0) {
		fp = fopen(get_logfile(), "a");
		if (fp != NULL) {
			fprintf(fp, "Signal %d received by process %d\n", signo, getpid());
			fclose(fp);
		}
	}
	switch (signo) {
	default:
		fp = fopen(get_logfile(), "a");
		if (fp != NULL) {
			fprintf(
					fp,
					"Unexpected signal (%d) received, process (%d) may terminate\n",
					signo, getpid());
			fclose(fp);
		}
		break;
	case SIGSEGV:
		log_println(6,
				"DEBUG, caught SIGSEGV signal and terminated process (%d)",
				getpid());
		if (getpid() != ndtpid)
			exit(-2);
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
		log_println(6,
				"DEBUG, caught SIGUSR1, setting sig1 flag to force exit");
		sig1 = 1;
		/* check_signal_flags(); */
		break;

	case SIGUSR2:
		log_println(6,
				"DEBUG, caught SIGUSR2, setting sig2 flag to force exit");
		sig2 = 1;
		/* check_signal_flags(); */
		break;

	case SIGALRM:
		switch (getCurrentTest()) {
		case TEST_MID:
			log_println(6, "Received SIGALRM signal [Middlebox test]");
			break;
		case TEST_C2S:
			log_println(6,
					"Received SIGALRM signal [C2S throughput test] pid=%d",
					getpid());
			break;
		case TEST_S2C:
			log_println(6,
					"Received SIGALRM signal [S2C throughput test] pid=%d",
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
						"Received SIGALRM signal: terminating active web100srv process [%d]",
						getpid());
			switch (getCurrentTest()) {
			case TEST_MID:
				fprintf(fp, " [Middlebox test]\n");
				break;
			case TEST_C2S:
				fprintf(fp, " [C2S throughput test]\n");
				/* break; */
				if (wait_sig == 1)
					return;
				break;
			case TEST_S2C:
				fprintf(fp, " [S2C throughput test]\n");
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
		fp = fopen(get_logfile(), "a");
		if ((fp != NULL) && (get_debuglvl() > 4)) {
			fprintf(
					fp,
					"Received SIGPIPE signal: terminating active web100srv process [%d]\n",
					getpid());
			fclose(fp);
		}
		sig13 = 1;
		break;

	case SIGHUP:
		/* Initialize Web100 structures */
		count_vars = web100_init(VarFileName);

		/* The administrator view automatically generates a usage page for the
		 * NDT server.  This page is then accessable to the general public.
		 * At this point read the existing log file and generate the necessary
		 * data.  This data is then updated at the end of each test.
		 * RAC 3/11/04
		 */
		if (admin_view == 1)
			view_init(refresh);
		break;

	case SIGCHLD:
		/* moved actions to child_sig() routine on 3/10/05  RAC
		 * Now all we do here is set a flag and return the flag
		 * is checked near the top of the main wait loop, so it
		 * will only be accessed once and only the testing proces
		 * will attempt to do something with it.
		 */
		/* sig17 = 1; */
		if (ndtpid != getppid()) {
			sig17++;
			log_println(5,
					"Signal 17 (SIGCHLD) received - completed tests = %d",
					sig17);
		}
		break;
	}
}

/** LoadConfig routine copied from Internet2.
 * @param *name Pointer to Application name
 * @param **lbuf line buffer
 * @param *lbuf_max line buffer max - both help keep track of a dynamically grown "line" buffer.*/

static void LoadConfig(char* name, char **lbuf, size_t *lbuf_max) {
	FILE *conf;
	char keybuf[256], valbuf[256];
	char *key = keybuf, *val = valbuf;
	int retcode = 0;

	if (!(conf = fopen(ConfigFileName, "r")))
		return;

	log_println(1, " Reading config file %s to obtain options", ConfigFileName);

	// Read the values for various keys and store them in appropriate variables
	while ((retcode = I2ReadConfVar(conf, retcode, key, val, 256, lbuf,
			lbuf_max)) > 0) {
		if (strncasecmp(key, "administrator_view", 5) == 0) {
			admin_view = 1;
			continue;
		}

		else if (strncasecmp(key, "multiple_clients", 5) == 0) {
			multiple = 1;
			continue;
		}

		else if (strncasecmp(key, "max_clients", 5) == 0) {
			max_clients = atoi(val);
			continue;
		}

		else if (strncasecmp(key, "record_reverse", 6) == 0) {
			record_reverse = 1;
			continue;
		}

		else if (strncasecmp(key, "write_trace", 5) == 0) {
			dumptrace = 1;
			continue;
		}

		else if (strncasecmp(key, "TCP_Buffer_size", 3) == 0) {
			if (check_int(val, &window)) {
				char tmpText[200];
				snprintf(tmpText, 200, "Invalid window size: %s", val);
				short_usage(name, tmpText);
			}
			set_buff = 1;
			continue;
		}

		else if (strncasecmp(key, "debug", 5) == 0) {
			set_debuglvl(atoi(val));
			continue;
		}

		else if (strncasecmp(key, "variable_file", 6) == 0) {
			sprintf(wvfn, "%s", val);
			VarFileName = wvfn;
			continue;
		}

		else if (strncasecmp(key, "log_file", 3) == 0) {
			sprintf(lgfn, "%s", val);
			set_logfile(lgfn);
			sprintf(lgfn, "%s", val);
			continue;
		} else if (strncasecmp(key, "protocol_log", 12) == 0) {
			sprintf(lgfn, "%s", val);
			set_protologdir(lgfn);
			continue;
		} else if (strncasecmp(key, "enableprotolog", 11) == 0) {
			enableprotocollogging();
			continue;
		} else if (strncasecmp(key, "admin_file", 10) == 0) {
			sprintf(apfn, "%s", val);
			AdminFileName = apfn;
			continue;
		}

		else if (strncasecmp(key, "logfacility", 11) == 0) {
			sprintf(slfa, "%s", val);
			SysLogFacility = slfa;
			continue;
		}

		else if (strncasecmp(key, "device", 5) == 0) {
			snprintf(devicebuf, 100, "%s", val);
			device = devicebuf;
			continue;
		}

		else if (strncasecmp(key, "port", 4) == 0) {
			snprintf(portbuf, 10, "%s", val);
			port = portbuf;
			continue;
		}

		else if (strncasecmp(key, "syslog", 6) == 0) {
			usesyslog = 1;
			continue;
		}

		else if (strncasecmp(key, "disable_FIFO", 5) == 0) {
			queue = 0;
			continue;
		}

		else if (strncasecmp(key, "old_mismatch", 3) == 0) {
			old_mismatch = 1;
			continue;
		}

		else if (strncasecmp(key, "cputime", 3) == 0) {
			cputime = 1;
			continue;
		}

		else if (strncasecmp(key, "enableDBlogging", 8) == 0) {
			useDB = 1;
			continue;
		}

		else if (strncasecmp(key, "dbDSN", 5) == 0) {
			snprintf(dbDSNbuf, 255, "%s", val);
			dbDSN = dbDSNbuf;
			continue;
		}

		else if (strncasecmp(key, "dbUID", 5) == 0) {
			snprintf(dbUIDbuf, 255, "%s", val);
			dbUID = dbUIDbuf;
			continue;
		}

		else if (strncasecmp(key, "dbPWD", 5) == 0) {
			snprintf(dbPWDbuf, 255, "%s", val);
			dbPWD = dbPWDbuf;
			continue;
		}

		else if (strncasecmp(key, "cwnddecrease", 5) == 0) {
			options.cwndDecrease = 1;
			options.snaplog = 1;
			continue;
		}

		else if (strncasecmp(key, "snaplog", 5) == 0) {
			options.snaplog = 1;
			continue;
		}

		else if (strncasecmp(key, "avoidsndblockup", 5) == 0) {
			options.avoidSndBlockUp = 1;
			continue;
		}

		else if (strncasecmp(key, "snapdelay", 5) == 0) {
			options.snapDelay = atoi(val);
			continue;
		}

		else if (strncasecmp(key, "limit", 5) == 0) {
			options.limit = atoi(val);
			continue;
		}

		else if (strncasecmp(key, "refresh", 5) == 0) {
			refresh = atoi(val);
			continue;
		}

		else if (strncasecmp(key, "mrange", 6) == 0) {
			if (mrange_parse(val)) {
				char tmpText[300];
				snprintf(tmpText, 300, "Invalid range: %s", val);
				short_usage(name, tmpText);
			}
			continue;
		}

		else if (strncasecmp(key, "midport", 7) == 0) {
			if (check_int(val, &testopt.midsockport)) {
				char tmpText[200];
				snprintf(tmpText, 200, "Invalid Middlebox test port number: %s",
						val);
				short_usage(name, tmpText);
			}
			continue;
		}

		else if (strncasecmp(key, "c2sport", 7) == 0) {
			if (check_int(val, &testopt.c2ssockport)) {
				char tmpText[200];
				snprintf(tmpText, 200,
						"Invalid C2S throughput test port number: %s", val);
				short_usage(name, tmpText);
			}
			continue;
		}

		else if (strncasecmp(key, "s2cport", 7) == 0) {
			if (check_int(val, &testopt.s2csockport)) {
				char tmpText[200];
				snprintf(tmpText, 200,
						"Invalid S2C throughput test port number: %s", val);
				short_usage(name, tmpText);
			}
			continue;
		}

		else {
			log_println(0, "Unrecognized config option: %s", key);
			exit(1);
		}
	}
	if (retcode < 0) {
		log_println(0, "Syntax error in line %d of the config file %s",
				(-retcode), ConfigFileName);
		exit(1);
	}
	fclose(conf);
}

/**
 *  This routine walks through the list of queued clients and kills off those
 * that don't respond.  New clients (after v3.5.5) will respond to this query
 * older clients wouldn't, so use the oldclient flag to tell them apart.
 * RAC 7/8/09
 * @param *head_ptr Pointer to the head of the list
 */
void *
zombieWorker(void *head_ptr) {

	struct ndtchild *tmp_ptr, *tmp, *pre_ptr = NULL;
	int i = 0, retcode;
	struct timeval sel_tv;
	fd_set rfd;
	char tmpstr[8], buff[32];
	int msgType, msgLen = 1;

	tmp_ptr = (struct ndtchild *) head_ptr;
	/* walk through this chain until the 4th client is reached, this helps
	 * prevent cases where the current client finishes and the next one begins
	 * before we are done looking
	 * RAC 7/8/09
	 */
	for (i = 0; i < 4; i++) {
		log_println(6, "Bumping queue pointer by 1 child=%d", tmp_ptr->pid);
		if (i == 3)
			pre_ptr = tmp_ptr;
		tmp_ptr = tmp_ptr->next;
	}
	sprintf(tmpstr, "9990");
	i = 0;
	while (tmp_ptr != NULL) {
		if (tmp_ptr->oldclient == 0) {
			log_println(
					6,
					"old client found in queue, can't tell if it's a zombie, child=%d",
					tmp_ptr->pid);
			tmp_ptr = tmp_ptr->next;
			pre_ptr = pre_ptr->next;
			continue;
		}
		log_println(6, "New client found, checking for response, child=%d",
				tmp_ptr->pid);

		// send "keep-alive" SRV_QUEUE message to client and expect a response
		retcode = send_msg(tmp_ptr->ctlsockfd, SRV_QUEUE, tmpstr,
				strlen(tmpstr));
		log_println(6,
				"send_msg() returned %d during zombie check on client %d",
				retcode, tmp_ptr->pid);
		FD_ZERO(&rfd);
		FD_SET(tmp_ptr->ctlsockfd, &rfd);
		sel_tv.tv_sec = 1;
		sel_tv.tv_usec = 500000;
		for (;;) {
			retcode = select((tmp_ptr->ctlsockfd) + 1, &rfd, NULL, NULL,
					&sel_tv);
			switch (retcode) {
			case 0:
				//  a timeout occurred, remove zombie client from list
				log_println(
						6,
						"New client didn't respond - must be a zombie, get rid of it, child=%d",
						tmp_ptr->pid);
				while ((retcode = sem_wait(&ndtq)) == -1 && errno == EINTR) {
					log_println(
							6,
							"Waiting for ndtq semaphore to free - adding new client 1");
					continue;
				}
				log_println(6, "removing client from FIFO, semaphore locked");
				tmp = tmp_ptr;
				pre_ptr->next = tmp_ptr->next;
				tmp_ptr = tmp_ptr->next;
				log_println(6, "timeout free tmp=0x%x", tmp);
				free(tmp);
				i++;
				sem_post(&ndtq);
				log_println(6, "Free'd semaphore lock - 4");
				break;
			default:
				log_println(6,
						"%d new client(s) responded, bumping pointers child=%d",
						retcode, tmp_ptr->pid);
				recv_msg(tmp_ptr->ctlsockfd, &msgType, buff, &msgLen);
				tmp_ptr = tmp_ptr->next;
				pre_ptr = pre_ptr->next;
				/*
				 if ((sig1 > 0) || (sig2 > 0))
				 check_signal_flags();
				 if (sig17 > 0)
				 child_sig(0);
				 */
				break;
			case -1: // some error status
				if (errno == EINTR) {
					log_println(
							6,
							"select() interrupted by signal, continue waiting for action or timeout");
					continue;
				}
				log_println(6, "select returned errno=%d do nothing, child=%d",
						errno, tmp_ptr->pid);
				break;
			}
		}
	}
	zombie_check = i;
	return NULL;
}

/**
 * Capture CPU time details
 *
 * @param arg* void pointer to the log file used to record CPU time details
 * */
void*
cputimeWorker(void* arg) {
	char *logname = (char*) arg;
	FILE* file = fopen(logname, "w");
	struct tms buf;
	double start = secs();

	if (!file)
		return NULL;

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
 * @param agent pointer to web_100 agent
 * @param ctlsockfd socket used for server->client communication
 * @param testopt TestOptions *
 * @param test_suite pointer to string indicating tests to be run
 * */

int run_test(web100_agent* agent, int ctlsockfd, TestOptions* testopt,
		char *test_suite) {

	char date[32]; // date indicator
	char spds[4][256]; // speed "bin" array containing counters for speeds
	char logstr1[4096], logstr2[1024]; // log
	char tmpstr[256];
	char isoTime[64];

	//int n; // temporary iterator variable --// commented out -> calc_linkspeed
	int Timeouts, SumRTT, CountRTT, PktsRetrans, FastRetran, DataPktsOut;
	int AckPktsOut, CurrentMSS, DupAcksIn, AckPktsIn, MaxRwinRcvd, Sndbuf;
	int CurrentCwnd, SndLimTimeRwin, SndLimTimeCwnd, SndLimTimeSender,
			DataBytesOut;
	int SndLimTransRwin, SndLimTransCwnd, SndLimTransSender, MaxSsthresh;
	int CurrentRTO, CurrentRwinRcvd, MaxCwnd, CongestionSignals, PktsOut,
			MinRTT;
	int CongAvoid, CongestionOverCount, MaxRTT, OtherReductions,
			CurTimeoutCount;
	int AbruptTimeouts, SendStall, SlowStart, SubsequentTimeouts,
			ThruBytesAcked;
	int RcvWinScale, SndWinScale;
	int link = CANNOT_DETERMINE_LINK; // local temporary variable indicative of link speed. Transmitted
	// but unused at client end , which has a similar link speed variable
	int mismatch = 0, bad_cable = 0;
	int half_duplex = NO_HALF_DUPLEX;
	int congestion = 0, totaltime;
	int ret, spd_index;
	//int index; // commented out -> calc_linkspeed
	//int links[16]; // commented out -> calc_linkspeed
	// int max; // commented out -> calc_linkspeed
	// int total;// commented out -> calc_linkspeed
	int c2s_linkspeed_data = 0; // C->S data link speed indicator determined using results
	int c2s_linkspeed_ack = 0; //
	int s2c_linkspeed_data = 0; // S->C data link speed indicator determined using results
	int s2c_linkspeed_ack = 0;
	//int j;        // commented out -> calc_linkspeed
	int totalcnt;
	int autotune;
	int dec_cnt, same_cnt, inc_cnt, timeout, dupack; // values collected from the speed tests
	//int ifspeed;

	time_t stime;

	double rttsec; // average round trip time
	double swin, cwin; //send, congestion window sizes respectively
	double rwin; // max window size advertisement rcvd
	double rwintime; // time spent being limited due to rcvr end
	double cwndtime; // time spent being limited due to congestion
	double sendtime; // time spent being limited due to sender's own fault

	double oo_order; // out-of-order packet ratio
	double waitsec;
	double bw_theortcl; // theoretical bandwidth
	double avgrtt; // Average round-trip time
	double timesec; // Total test time in microseconds
	double packetloss_s2c; // Packet loss as calculated from S->c tests.
	double RTOidle; // Proportion of idle time spent waiting for packets to arrive
	double s2cspd; // average throughput as calculated by S->C test
	double c2sspd; // average throughput as calculated by C->S test
	double s2c2spd; // average throughput as calculated by midbox test
	double realthruput; // total send throughput in S->C
	double acksratio; // ratio of acks over packets sent
	double aspd = 0;
	double tmoutsratio; // timeouts fraction
	double rtranratio, dackratio; // ratio of retransmissions and duplicate acks over packets sent
	float runave[4];

	FILE * fp;

	web100_connection* conn;

	// start with a clean slate of currently running test and direction
	setCurrentTest(TEST_NONE);
	printf("Remote host= %s", get_remotehost());

	stime = time(0);
	log_println(4, "Child process %d started", getpid());
	testopt->child0 = getpid();

	// initialize speeds array
	for (spd_index = 0; spd_index < 4; spd_index++)
		for (ret = 0; ret < 256; ret++)
			spds[spd_index][ret] = 0x00;
	spd_index = 0;

	// obtain web100 connection and check auto-tune status
	conn = web100_connection_from_socket(agent, ctlsockfd);
	autotune = web100_autotune(ctlsockfd, agent, conn);

	// client needs to be version compatible. Send current version
	sprintf(buff, "v%s", VERSION);
	send_msg(ctlsockfd, MSG_LOGIN, buff, strlen(buff));

	// initiate test with MSG_LOGIN message.
	log_println(3, "run_test() routine, asking for test_suite = %s",
			test_suite);
	send_msg(ctlsockfd, MSG_LOGIN, test_suite, strlen(test_suite));
	/* if ((n = initialize_tests(ctlsockfd, &testopt, conn_options))) {
	 log_println(0, "ERROR: Tests initialization failed (%d)", n);
	 return;
	 }
	 */

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
	if (testopt->s2copt) {
		log_println(1, " > S2C throughput test");
	}
	if (testopt->metaopt) {
		log_println(1, " > META test");
	}

	/*  alarm(15); */
	// Run scheduled test. Log error code if necessary
	log_println(6, "Starting middlebox test");
	if ((ret = test_mid(ctlsockfd, agent, &*testopt, conn_options, &s2c2spd))
			!= 0) {
		if (ret < 0)
			log_println(6, "Middlebox test failed with rc=%d", ret);
		log_println(0, "Middlebox test FAILED!, rc=%d", ret);
		testopt->midopt = TOPT_DISABLED;
		return ret;
	}

	/*  alarm(20); */
	log_println(6, "Starting simple firewall test");
	if ((ret = test_sfw_srv(ctlsockfd, agent, &*testopt, conn_options)) != 0) {
		if (ret < 0)
			log_println(6, "SFW test failed with rc=%d", ret);
	}

	/*  alarm(25); */
	log_println(6, "Starting c2s throughput test");
	if ((ret = test_c2s(ctlsockfd, agent, &*testopt, conn_options, &c2sspd,
			set_buff, window, autotune, device, &options, record_reverse,
			count_vars, spds, &spd_index)) != 0) {
		if (ret < 0)
			log_println(6, "C2S test failed with rc=%d", ret);
		log_println(0, "C2S throughput test FAILED!, rc=%d", ret);
		testopt->c2sopt = TOPT_DISABLED;
		return ret;
	}

	/*  alarm(25); */
	log_println(6, "Starting s2c throughput test");
	if ((ret = test_s2c(ctlsockfd, agent, &*testopt, conn_options, &s2cspd,
			set_buff, window, autotune, device, &options, spds, &spd_index,
			count_vars, &peaks)) != 0) {
		if (ret < 0)
			log_println(6, "S2C test failed with rc=%d", ret);
		log_println(0, "S2C throughput test FAILED!, rc=%d", ret);
		testopt->s2copt = TOPT_DISABLED;
		return ret;
	}

	log_println(6, "Starting META test");
	if ((ret = test_meta_srv(ctlsockfd, agent, &*testopt, conn_options)) != 0) {
		if (ret < 0) {
			log_println(6, "META test failed with rc=%d", ret);
		}
	}

	// Compute variable values from test results and deduce results
	log_println(4, "Finished testing C2S = %0.2f Mbps, S2C = %0.2f Mbps",
			c2sspd / 1000, s2cspd / 1000);

	// Determine link speed
	calc_linkspeed(spds, spd_index, &c2s_linkspeed_data, &c2s_linkspeed_ack, &s2c_linkspeed_data, &s2c_linkspeed_ack,
			runave, &dec_cnt, &same_cnt, &inc_cnt, &timeout, &dupack,
			testopt->c2sopt);
	// Get web100 vars

	// ...determine number of times congestion window has been changed
	if (options.cwndDecrease) {
		dec_cnt = inc_cnt = same_cnt = 0;
		CwndDecrease(agent, options.s2c_logname, &dec_cnt, &same_cnt, &inc_cnt);
		log_println(2, "####### decreases = %d, increases = %d, no change = %d",
				dec_cnt, inc_cnt, same_cnt);
	}

	// ...other variables
	web100_logvars(&Timeouts, &SumRTT, &CountRTT, &PktsRetrans, &FastRetran,
			&DataPktsOut, &AckPktsOut, &CurrentMSS, &DupAcksIn, &AckPktsIn,
			&MaxRwinRcvd, &Sndbuf, &CurrentCwnd, &SndLimTimeRwin,
			&SndLimTimeCwnd, &SndLimTimeSender, &DataBytesOut, &SndLimTransRwin,
			&SndLimTransCwnd, &SndLimTransSender, &MaxSsthresh, &CurrentRTO,
			&CurrentRwinRcvd, &MaxCwnd, &CongestionSignals, &PktsOut, &MinRTT,
			count_vars, &RcvWinScale, &SndWinScale, &CongAvoid,
			&CongestionOverCount, &MaxRTT, &OtherReductions, &CurTimeoutCount,
			&AbruptTimeouts, &SendStall, &SlowStart, &SubsequentTimeouts,
			&ThruBytesAcked);

	// end getting web100 variable values
	/* if (rc == 0) { */

	// section to calculate duplex mismatch
	// Calculate average round trip time and convert to seconds
	rttsec = calc_avg_rtt(SumRTT, CountRTT, &avgrtt);

	// Calculate packet loss
	packetloss_s2c = calc_packetloss(CongestionSignals, PktsOut, c2s_linkspeed_data);

	// Calculate ratio of packets arriving out of order
	oo_order = calc_packets_outoforder(DupAcksIn, AckPktsIn);

	// calculate theoretical maximum goodput in bits
	bw_theortcl = calc_max_theoretical_throughput(CurrentMSS, rttsec,
			packetloss_s2c);

	//get window sizes
	calc_window_sizes(&SndWinScale, &RcvWinScale, Sndbuf, MaxRwinRcvd,
			MaxCwnd, &rwin, &swin, &cwin);

	// Total test time
	totaltime = calc_totaltesttime(SndLimTimeRwin, SndLimTimeCwnd,
			SndLimTimeSender);

	// time spent being send-limited due to client's recv window
	rwintime = calc_sendlimited_rcvrfault(SndLimTimeRwin, totaltime);

	// time spent in being send-limited due to congestion window
	cwndtime = calc_sendlimited_cong(SndLimTimeCwnd, totaltime);

	// time spent in being send-limited due to own fault
	sendtime = calc_sendlimited_sndrfault(SndLimTimeSender, totaltime);

	timesec = totaltime / MEGA; // total time in microsecs

	// get fraction of total test time waiting for packets to arrive
	RTOidle = calc_RTOIdle(Timeouts, CurrentRTO, timesec);

	// get timeout, retransmission, acks and dup acks ratios.
	tmoutsratio = (double) Timeouts / PktsOut;
	rtranratio = (double) PktsRetrans / PktsOut;
	acksratio = (double) AckPktsIn / PktsOut;
	dackratio = (double) DupAcksIn / (double) AckPktsIn;

	// get actual throughput in Mbps (totaltime is in microseconds)
	realthruput = calc_real_throughput(DataBytesOut, totaltime);

	// total time spent waiting
	waitsec = cal_totalwaittime(CurrentRTO, Timeouts);

	log_println(2, "CWND limited test = %0.2f while unlimited = %0.2f", s2c2spd,
			s2cspd);

	// Is thruput measured with limited cwnd(midbox test) > as reported S->C test
	if (is_limited_cwnd_throughput_better(s2c2spd, s2cspd)
			&& isNotMultipleTestMode(multiple))
		log_println(
				2,
				"Better throughput when CWND is limited, may be duplex mismatch");
	else
		log_println(2,
				"Better throughput without CWND limits - normal operation");

	/* remove the following line when the new detection code is ready for release */
	// retaining old comment above
	// client link duplex mismatch detection heuristic
	old_mismatch = 1;

	if (old_mismatch == 1) {
		if (detect_duplexmismatch(cwndtime, bw_theortcl, PktsRetrans, timesec,
				MaxSsthresh, RTOidle, link, s2cspd, s2c2spd, multiple)) {
			if (is_c2s_throughputbetter(c2sspd, s2cspd)) { // also, S->C throughput is lesser than C->S throughput
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
				(s2cspd / 1000), realthruput, rwintime, packetloss_s2c )) {
			mismatch = DUPLEX_SWITCH_FULL_HOST_HALF; // switch full, host half
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
			MaxSsthresh))
		bad_cable = POSSIBLE_BAD_CABLE;

	// test for Ethernet link (assume Fast E.)
	if (detect_ethernetlink(realthruput, s2cspd, packetloss_s2c, oo_order,
			link))
		link = LINK_ETHERNET;

	// test for wireless link
	if (detect_wirelesslink(sendtime, realthruput, bw_theortcl, SndLimTransRwin,
			SndLimTransCwnd, rwintime, link))
		link = LINK_WIRELESS;

	// test for DSL/Cable modem link
	if (detect_DSLCablelink(SndLimTimeSender, SndLimTransSender, realthruput,
			bw_theortcl, link))
		link = LINK_DSLORCABLE;

	// full/half link duplex setting heuristic:
	// receiver-limited- time > 95%,
	//  .. number of transitions into the 'Receiver Limited' state is greater than 30 ps
	//  ...and the number of transitions into the 'Sender Limited' state is greater than 30 per second

	if (detect_halfduplex(rwintime, SndLimTransRwin, SndLimTransSender,
			timesec))
		half_duplex = POSSIBLE_HALF_DUPLEX;

	// congestion detection heuristic
	if (detect_congestionwindow(cwndtime, mismatch, cwin, rwin, rttsec))
		congestion = POSSIBLE_CONGESTION;

	// Send results and variable values to clients
	sprintf(buff, "c2sData: %d\nc2sAck: %d\ns2cData: %d\ns2cAck: %d\n", c2s_linkspeed_data,
			c2s_linkspeed_ack, s2c_linkspeed_data, s2c_linkspeed_ack);
	send_msg(ctlsockfd, MSG_RESULTS, buff, strlen(buff));

	sprintf(
			buff,
			"half_duplex: %d\nlink: %d\ncongestion: %d\nbad_cable: %d\nmismatch: %d\nspd: %0.2f\n",
			half_duplex, link, congestion, bad_cable, mismatch, realthruput);
	send_msg(ctlsockfd, MSG_RESULTS, buff, strlen(buff));

	sprintf(
			buff,
			"bw: %0.2f\nloss: %0.9f\navgrtt: %0.2f\nwaitsec: %0.2f\ntimesec: %0.2f\norder: %0.4f\n",
			bw_theortcl, packetloss_s2c, avgrtt, waitsec, timesec, oo_order);
	send_msg(ctlsockfd, MSG_RESULTS, buff, strlen(buff));

	sprintf(
			buff,
			"rwintime: %0.4f\nsendtime: %0.4f\ncwndtime: %0.4f\nrwin: %0.4f\nswin: %0.4f\n",
			rwintime, sendtime, cwndtime, rwin, swin);
	send_msg(ctlsockfd, MSG_RESULTS, buff, strlen(buff));

	sprintf(
			buff,
			"cwin: %0.4f\nrttsec: %0.6f\nSndbuf: %d\naspd: %0.5f\nCWND-Limited: %0.2f\n",
			cwin, rttsec, Sndbuf, aspd, s2c2spd);
	send_msg(ctlsockfd, MSG_RESULTS, buff, strlen(buff));

	sprintf(buff, "minCWNDpeak: %d\nmaxCWNDpeak: %d\nCWNDpeaks: %d\n",
			peaks.min, peaks.max, peaks.amount);
	send_msg(ctlsockfd, MSG_RESULTS, buff, strlen(buff));

	// Signal end of test results to client
	send_msg(ctlsockfd, MSG_LOGOUT, "", 0);

	// Copy collected values into the meta data structures. This section
	// seems most readable, easy to debug here.
	sprintf(meta.date, "%s", get_ISOtime(isoTime, sizeof(isoTime)));

	log_println(9, "meta.date=%s, meta.clientip =%s:%s:%d", meta.date,
			meta.client_ip, rmt_host, strlen(rmt_host));
	memcpy(meta.client_ip, rmt_host, strlen(rmt_host));
	log_println(9, "2. meta.clientip =%s:%s:%d", meta.client_ip, rmt_host);

	memset(tmpstr, 0, 255);
	sprintf(tmpstr, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,", (int) s2c2spd,
			(int) s2cspd, (int) c2sspd, Timeouts, SumRTT, CountRTT, PktsRetrans,
			FastRetran, DataPktsOut, AckPktsOut, CurrentMSS, DupAcksIn,
			AckPktsIn);
	memcpy(meta.summary, tmpstr, strlen(tmpstr));
	memset(tmpstr, 0, 255);
	sprintf(tmpstr, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,", MaxRwinRcvd,
			Sndbuf, MaxCwnd, SndLimTimeRwin, SndLimTimeCwnd, SndLimTimeSender,
			DataBytesOut, SndLimTransRwin, SndLimTransCwnd, SndLimTransSender,
			MaxSsthresh, CurrentRTO, CurrentRwinRcvd);

	strlcat(meta.summary, tmpstr, sizeof(meta.summary));
	memset(tmpstr, 0, 255);
	sprintf(tmpstr, "%d,%d,%d,%d,%d", link, mismatch, bad_cable, half_duplex,
			congestion);

	strlcat(meta.summary, tmpstr, sizeof(meta.summary));
	memset(tmpstr, 0, 255);
	sprintf(tmpstr, ",%d,%d,%d,%d,%d,%d,%d,%d,%d", c2s_linkspeed_data, c2s_linkspeed_ack, s2c_linkspeed_data,
			s2c_linkspeed_ack, CongestionSignals, PktsOut, MinRTT, RcvWinScale, autotune);

	strlcat(meta.summary, tmpstr, sizeof(meta.summary));
	memset(tmpstr, 0, 255);
	sprintf(tmpstr, ",%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", CongAvoid,
			CongestionOverCount, MaxRTT, OtherReductions, CurTimeoutCount,
			AbruptTimeouts, SendStall, SlowStart, SubsequentTimeouts,
			ThruBytesAcked);

	strlcat(meta.summary, tmpstr, sizeof(meta.summary));
	memset(tmpstr, 0, 255);
	sprintf(tmpstr, ",%d,%d,%d", peaks.min, peaks.max, peaks.amount);

	strlcat(meta.summary, tmpstr, sizeof(meta.summary));
	writeMeta(options.compress, cputime, options.snaplog, dumptrace);

	// Write into log files, DB
	fp = fopen(get_logfile(), "a");
	if (fp == NULL) {
		log_println(0,
				"Unable to open log file '%s', continuing on without logging",
				get_logfile());
	} else {
		sprintf(date, "%15.15s", ctime(&stime) + 4);
		fprintf(fp, "%s,", date);
		fprintf(fp, "%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,", rmt_host,
				(int) s2c2spd, (int) s2cspd, (int) c2sspd, Timeouts, SumRTT,
				CountRTT, PktsRetrans, FastRetran, DataPktsOut, AckPktsOut,
				CurrentMSS, DupAcksIn, AckPktsIn);
		fprintf(fp, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,", MaxRwinRcvd,
				Sndbuf, MaxCwnd, SndLimTimeRwin, SndLimTimeCwnd,
				SndLimTimeSender, DataBytesOut, SndLimTransRwin,
				SndLimTransCwnd, SndLimTransSender, MaxSsthresh, CurrentRTO,
				CurrentRwinRcvd);
		fprintf(fp, "%d,%d,%d,%d,%d", link, mismatch, bad_cable, half_duplex,
				congestion);
		fprintf(fp, ",%d,%d,%d,%d,%d,%d,%d,%d,%d", c2s_linkspeed_data, c2s_linkspeed_ack, s2c_linkspeed_data,
				s2c_linkspeed_ack, CongestionSignals, PktsOut, MinRTT, RcvWinScale,
				autotune);
		fprintf(fp, ",%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", CongAvoid,
				CongestionOverCount, MaxRTT, OtherReductions, CurTimeoutCount,
				AbruptTimeouts, SendStall, SlowStart, SubsequentTimeouts,
				ThruBytesAcked);
		fprintf(fp, ",%d,%d,%d\n", peaks.min, peaks.max, peaks.amount);
		fclose(fp);
	}
	db_insert(spds, runave, cputimelog, options.s2c_logname,
			options.c2s_logname, testName, testPort, date, rmt_host, s2c2spd,
			s2cspd, c2sspd, Timeouts, SumRTT, CountRTT, PktsRetrans, FastRetran,
			DataPktsOut, AckPktsOut, CurrentMSS, DupAcksIn, AckPktsIn,
			MaxRwinRcvd, Sndbuf, MaxCwnd, SndLimTimeRwin, SndLimTimeCwnd,
			SndLimTimeSender, DataBytesOut, SndLimTransRwin, SndLimTransCwnd,
			SndLimTransSender, MaxSsthresh, CurrentRTO, CurrentRwinRcvd, link,
			mismatch, bad_cable, half_duplex, congestion, c2s_linkspeed_data, c2s_linkspeed_ack,
			s2c_linkspeed_data, s2c_linkspeed_ack, CongestionSignals, PktsOut, MinRTT, RcvWinScale,
			autotune, CongAvoid, CongestionOverCount, MaxRTT, OtherReductions,
			CurTimeoutCount, AbruptTimeouts, SendStall, SlowStart,
			SubsequentTimeouts, ThruBytesAcked, peaks.min, peaks.max,
			peaks.amount);
	if (usesyslog == 1) {
		sprintf(
				logstr1,
				"client_IP=%s,c2s_spd=%2.0f,s2c_spd=%2.0f,Timeouts=%d,SumRTT=%d,CountRTT=%d,PktsRetrans=%d,FastRetran=%d,DataPktsOut=%d,AckPktsOut=%d,CurrentMSS=%d,DupAcksIn=%d,AckPktsIn=%d,",
				rmt_host, c2sspd, s2cspd, Timeouts, SumRTT, CountRTT,
				PktsRetrans, FastRetran, DataPktsOut, AckPktsOut, CurrentMSS,
				DupAcksIn, AckPktsIn);
		sprintf(
				logstr2,
				"MaxRwinRcvd=%d,Sndbuf=%d,MaxCwnd=%d,SndLimTimeRwin=%d,SndLimTimeCwnd=%d,SndLimTimeSender=%d,DataBytesOut=%d,SndLimTransRwin=%d,SndLimTransCwnd=%d,SndLimTransSender=%d,MaxSsthresh=%d,CurrentRTO=%d,CurrentRwinRcvd=%d,",
				MaxRwinRcvd, Sndbuf, MaxCwnd, SndLimTimeRwin, SndLimTimeCwnd,
				SndLimTimeSender, DataBytesOut, SndLimTransRwin,
				SndLimTransCwnd, SndLimTransSender, MaxSsthresh, CurrentRTO,
				CurrentRwinRcvd);
		strcat(logstr1, logstr2);
		sprintf(
				logstr2,
				"link=%d,mismatch=%d,bad_cable=%d,half_duplex=%d,congestion=%d,c2s_linkspeed_data=%d,c2sack=%d,s2cdata=%d,s2cack=%d,CongestionSignals=%d,PktsOut=%d,MinRTT=%d,RcvWinScale=%d\n",
				link, mismatch, bad_cable, half_duplex, congestion, c2s_linkspeed_data,
				c2s_linkspeed_ack, s2c_linkspeed_data, s2c_linkspeed_ack, CongestionSignals, PktsOut, MinRTT,
				RcvWinScale);
		strcat(logstr1, logstr2);
		syslog(LOG_FACILITY | LOG_INFO, "%s", logstr1);
		closelog();
		log_println(4, "%s", logstr1);
	}

	// close resources

	if (testopt->s2copt) {
		close(testopt->s2csockfd);
	}

	/* If the admin view is turned on then the client process is going to update these
	 * variables.  The need to be shipped back to the parent so the admin page can be
	 * updated.  Otherwise the changes are lost when the client terminates.
	 */
	if (admin_view == 1) {
		totalcnt = calculate(date, SumRTT, CountRTT, CongestionSignals, PktsOut,
				DupAcksIn, AckPktsIn, CurrentMSS, SndLimTimeRwin,
				SndLimTimeCwnd, SndLimTimeSender, MaxRwinRcvd, CurrentCwnd,
				Sndbuf, DataBytesOut, mismatch, bad_cable, (int) c2sspd,
				(int) s2cspd, c2s_linkspeed_data, s2c_linkspeed_ack, 1);
		gen_html((int) c2sspd, (int) s2cspd, MinRTT, PktsRetrans, Timeouts,
				Sndbuf, MaxRwinRcvd, CurrentCwnd, mismatch, bad_cable, totalcnt,
				refresh);
	}
	shutdown(ctlsockfd, SHUT_WR);
	/* shutdown(ctlsockfd, SHUT_RDWR); */
	return (0);
}

/**
 * Initializes data structures,
 *  web100 structures and logging systems.  Read/load configuration, get process
 *  execution options. Accept test requests and manage their execution order and
 *  initiate tests. Keep track of running processes.
 * @param argc Number of arguments
 * @param argv string command line arguments
 * */
int main(int argc, char** argv) {
	pid_t chld_pid;
	int retcode;
	int tpid, mwaiting = 0;
	int ctlsockfd = -1;
	int c, chld_pipe[2];
	int i, loopcnt, t_opts = 0;
	struct sockaddr_storage cli_addr;
	struct sigaction new;
	web100_agent* agent;
	char *lbuf = NULL, *ctime();
	char buff[32], tmpstr[256];
	char test_suite[16];
	FILE * fp;
	size_t lbuf_max = 0;
	fd_set rfd;
	struct timeval sel_tv;
	struct ndtchild *tmp_ptr = NULL, *new_child = NULL, *mchild = NULL;
	time_t tt;
	socklen_t clilen;

	I2Addr listenaddr = NULL;
	int listenfd;
	char* srcname = NULL;
	char isoTime[64], dir[256];
	int debug = 0;

	DIR *dp;
	int j;
	char *name;

	// variables used for protocol validation logs
	//char startsrvmsg[256]; // used to log start of server process
	//char *srvstatusdesc;
	//enum PROCESS_STATUS_INT srvstatusenum = UNKNOWN;
	//char statustemparr[PROCESS_STATUS_DESC_SIZE]; // temp storage for process name
	enum PROCESS_TYPE_INT proctypeenum = PROCESS_TYPE;
	enum PROCESS_STATUS_INT procstatusenum = UNKNOWN;

	options.limit = 0;
	options.snapDelay = 5;
	options.avoidSndBlockUp = 0;
	options.snaplog = 0;
	options.cwndDecrease = 0;
	memset(options.s2c_logname, 0, 256);
	memset(options.c2s_logname, 0, 256);
	peaks.min = -1;
	peaks.max = -1;
	peaks.amount = -1;
	DataDirName = NULL;

	memset(&testopt, 0, sizeof(testopt));
	/* sigset_t newmask, oldmask; */

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
	while ((c =
			getopt_long(
					argc,
					argv,
					// GETOPT_LONG_INET6(GETOPT_LONG_EXP("adhmoqrstvzc:x:b:f:i:l:p:T:A:S:")), long_options, 0)) != -1) {
					GETOPT_LONG_INET6(GETOPT_LONG_EXP("adhmoqrstvzc:x:b:f:i:l:u:e:p:T:A:S:")),
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

	if (ConfigFileName == NULL)
		ConfigFileName = CONFIGFILE;

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

	// Get server execution options
	while ((c =
			getopt_long(
					argc,
					argv,
					// GETOPT_LONG_INET6(GETOPT_LONG_EXP("adhmoqrstvzc:x:b:f:i:l:p:T:A:S:")), long_options, 0)) != -1) {
					GETOPT_LONG_INET6(GETOPT_LONG_EXP("adhmoqrstvzc:x:b:f:i:l:u:e:p:T:A:S:")),
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
				snprintf(tmpText, 200, "Invalid primary port number: %s",
						optarg);
				short_usage(argv[0], tmpText);
			}
			break;
		case 302:
			if (check_int(optarg, &testopt.midsockport)) {
				char tmpText[200];
				snprintf(tmpText, 200, "Invalid Middlebox test port number: %s",
						optarg);
				short_usage(argv[0], tmpText);
			}
			break;
		case 303:
			if (check_int(optarg, &testopt.c2ssockport)) {
				char tmpText[200];
				snprintf(tmpText, 200,
						"Invalid C2S throughput test port number: %s", optarg);
				short_usage(argv[0], tmpText);
			}
			break;
		case 304:
			if (check_int(optarg, &testopt.s2csockport)) {
				char tmpText[200];
				snprintf(tmpText, 200,
						"Invalid S2C throughput test port number: %s", optarg);
				short_usage(argv[0], tmpText);
			}
			break;
		case 'a':
			admin_view = 1;
			break;
		case 'f':
			VarFileName = optarg;
			break;
		case 'i':
			device = optarg;
			break;
		case 'l':
			set_logfile(optarg);
			break;
		case 'u':
			printf("Calling set protolog from case-u");
			set_protologdir(optarg);
			break;
		case 'e':
			printf("Enabling protocol logging");
			enableprotocollogging();
			break;
		case 'o':
			old_mismatch = 1;
			break;
		case 301:
			if (mrange_parse(optarg)) {
				char tmpText[300];
				snprintf(tmpText, 300, "Invalid range: %s", optarg);
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
				snprintf(tmpText, 200, "Invalid window size: %s", optarg);
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
		case '?':
			short_usage(argv[0], "");
			break;
		}
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
				0,
				"Warning: This program must be run as root to enable the Link Type");
		log_println(
				0,
				" detection algorithm.\nContinuing execution without this algorithm");
	}

	if (VarFileName == NULL) {
		sprintf(wvfn, "%s/%s", BASEDIR, WEB100_FILE);
		VarFileName = wvfn;
	}

	if (AdminFileName == NULL) {
		sprintf(apfn, "%s/%s", BASEDIR, ADMINFILE);
		AdminFileName = apfn;
	}

	if (DataDirName == NULL) {
		sprintf(logd, "%s/%s/", BASEDIR, LOGDIR);
		DataDirName = logd;
	} else if (DataDirName[0] != '/') {
		sprintf(logd, "%s/%s/", BASEDIR, DataDirName);
		DataDirName = logd;
	}

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
					0,
					"Warning: Unknown syslog facility [%s] --> using default (%d)",
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

	initialize_db(useDB, dbDSN, dbUID, dbPWD);

	memset(&new, 0, sizeof(new));
	new.sa_handler = cleanup;

	// Grab all signals and run them through my cleanup routine.  2/24/05 */
	// sigemptyset(&newmask);
	// sigemptyset(&oldmask);
	for (i = 1; i < 32; i++) {
		if ((i == SIGKILL) || (i == SIGSTOP))
			continue; /* these signals can't be caught */
		sigaction(i, &new, NULL);
		/* sigaddset(&newmask, i); */
	}

	// Bind our local address so that the client can send to us.

	if (srcname && !(listenaddr = I2AddrByNode(get_errhandle(), srcname))) {
		err_sys("server: Invalid source address specified");
	}
	if ((listenaddr = CreateListenSocket(listenaddr, port, conn_options,
			((set_buff > 0) ? window : 0))) == NULL) {
		err_sys("server: CreateListenSocket failed");
	}
	listenfd = I2AddrFD(listenaddr);

	log_println(1, "server ready on port %s", port);

	// Initialize Web100 structures
	count_vars = web100_init(VarFileName);
	if (count_vars == -1) {
		log_println(0, "No web100 variables file found, terminating program");
		exit(-5);
	}

	//   The administrator view automatically generates a usage page for the
	//    NDT server.  This page is then accessible to the general public.
	//    At this point read the existing log file and generate the necessary
	//    data.  This data is then updated at the end of each test.
	//    RAC 11/28/03

	if (admin_view == 1)
		view_init(refresh);

	// Get the server's metadata info (OS name and kernel version
	// RAC 7/7/09

	if ((fp = fopen("/proc/sys/kernel/hostname", "r")) == NULL)
		log_println(0, "Unable to determine server Hostname.");
	else {
		fscanf(fp, "%s", meta.server_name);
		fclose(fp);
	}
	if ((fp = fopen("/proc/sys/kernel/osrelease", "r")) == NULL)
		log_println(0, "Unable to determine server OS Version.");
	else {
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
		fprintf(fp, "Web100srv (ver %s) process (%d) started %15.15s\n",
				VERSION, ndtpid, ctime(&tt) + 4);
		fclose(fp);
	}
	if (usesyslog == 1)
		syslog(LOG_FACILITY | LOG_INFO, "Web100srv (ver %s) process started",
				VERSION);

	// create protocol validation entry every time a process starts up
	// This whole block is not needed since there are protocol log files per client
	// and server start is not a protocol process
	/*
	sprintf(startsrvmsg, "Web100srv (ver %s)", VERSION);
	srvstatusenum = PROCESS_STARTED;
	srvstatusdesc = get_procstatusdesc(srvstatusenum, statustemparr);
	protolog_printgeneric(srvstatusdesc, startsrvmsg);
	*/

	// scan through the interface device list and get the names/speeds of each
	//  if.  The speed data can be used to cap the search for the bottleneck link
	//  capacity.  The intent is to reduce the impact of interrupt coalescing on
	//  the bottleneck link detection algorithm
	//  RAC 7/14/09

	get_iflist();

	for (i = 0; iflist.speed[i] > 0; i++)
		log_println(4, "Generated iflist with device=%s and if_speed=%d",
				iflist.name[i], iflist.speed[i]);

	// Wait at accept() for a new connection from a client process.

	// These 2 flags keep track of running processes.  The 'testing' flag
	//  indicates a test is currently being performed.  The 'waiting' counter
	//  shows how many tests are pending.
	//  Rich Carlson 3/11/04

	options.compress = compress;
	testing = 0;
	mclients = 0;
	waiting = 0;
	loopcnt = 0;
	head_ptr = NULL;
	sig13 = 0;
	sig17 = 0;
	sig1 = 0;
	sig2 = 0;
	sem_init(&ndtq, 0, 1);

	for (;;) {

		mainloop: if (head_ptr == NULL)
			log_println(3, "nothing in queue");
		else
			log_println(
					3,
					"Queue pointer=%d, testing=%d, waiting=%d, mclients=%d, zombie_check=%d",
					head_ptr->pid, testing, waiting, mclients, zombie_check);

		// moved condition from interrupt handler to here
		/* if ((sig1 > 0) || (sig2 > 0))
		 * 	check_signal_flags;
		 */

		if (sig13 == 1) {
			log_println(5, "todo: Handle SIGPIPE signal, terminate child?");
			child_sig(0);
			sig13 = 0;
		}

		if (sig17 > 0) {
			log_println(5, "Handle pending SIGCHLD signal, count=%d", sig17);
			child_sig(0);
		}

		if ((multiple == 1) && (mclients < max_clients)
				&& (waiting >= max_clients)) {
			// This condition means that there are clients waiting and there are open slots
			//   in the test queue, so dispatch another client.
			//   RAC 12/11/09

			log_println(5,
					"Empty slot in test queue, find new client to dispatch");
			/* tmp_ptr = head_ptr; */
			mchild = head_ptr;
			i = 0;
			while (mchild != NULL) {
				i++; // Keep count of how many times we go through this loop
				log_println(
						2,
						"walking queue look for non-running client current=%d, running=%d, next=0x%x",
						mchild->pid, mchild->running, mchild->next);
				if (mchild->running == 0) {
					/* mchild = tmp_ptr; */
					log_println(
							6,
							"found non-running client %d, update queue and dispatch this client",
							mchild->pid);
					break;
				}
				mchild = mchild->next;
			}
			if (i > max_clients) {
				log_println(6,
						"walked through running client list, no empty slots!");
				continue;
			}

			/* if ((mchild->next == NULL) && (mchild->running == 0))
			 *   mchild = tmp_ptr;
			 * if (mchild != head_ptr) {
			 */
			tmp_ptr = mchild;

			// Update queued clients, send message to client when it moves
			//  up in the queue enough to get closer to running a test.  This happens
			//  when the client falls into the next lower maxqueue bin
			//  RAC 3/21/10

			int rac;
			if (waiting > (2 * max_clients)) {
				for (i = max_clients; i <= waiting; i++) {
					if (tmp_ptr == NULL)
						break;
					if (i == (2 * max_clients)) {
						rac = send_msg(tmp_ptr->ctlsockfd, SRV_QUEUE, "1", 1);
						log_println(
								6,
								"sent 45 sec update message to client %d on fd=%d, send_msg() returned %d",
								tmp_ptr->pid, tmp_ptr->ctlsockfd, rac);
					}
					if (i == (3 * max_clients)) {
						rac = send_msg(tmp_ptr->ctlsockfd, SRV_QUEUE, "2", 1);
						log_println(
								6,
								"sent 90 sec update message to client %d on fd=%d, send_msg() returned %d",
								tmp_ptr->pid, tmp_ptr->ctlsockfd, rac);
					}
					tmp_ptr = tmp_ptr->next;
				}
			}
			goto dispatch_client;
		}

		if ((waiting < 0) || (mclients < 0)) {
			log_println(
					6,
					"Fault: Negative number of clents waiting=%d, mclients=%d, nuke them",
					waiting, mclients);
			while (head_ptr != NULL) {
				/* send_msg(head_ptr->ctlsockfd, SRV_QUEUE, "9933", 4); */
				send_msg(head_ptr->ctlsockfd, SRV_QUEUE, "9988", 4); // indicate server waiting in queue
				shutdown(head_ptr->ctlsockfd, SHUT_WR);
				close(head_ptr->ctlsockfd);
				tpid = head_ptr->pid;
				child_sig(-1);
				kill(tpid, SIGTERM);
				child_sig(tpid);
			}
			waiting = 0;
			mclients = 0;
			continue;
		}

		if ((waiting == 0) && (head_ptr != NULL)) {
			log_println(6,
					"Fault: Something [%d] in queue, but no waiting clients",
					head_ptr->pid);
			while (head_ptr != NULL) {
				/* send_msg(head_ptr->ctlsockfd, SRV_QUEUE, "9977", 4); */
				send_msg(head_ptr->ctlsockfd, SRV_QUEUE, "9988", 4);
				shutdown(head_ptr->ctlsockfd, SHUT_WR);
				close(head_ptr->ctlsockfd);
				tpid = head_ptr->pid;
				child_sig(-1);
				kill(tpid, SIGTERM);
				child_sig(tpid);
			}
			waiting = 0;
			mclients = 0;
			continue;
		}

		if (head_ptr != NULL) {
			if ((time(0) - head_ptr->stime) > 70) { // 70 => WAIT-TIME-THRESH
				log_println(
						6,
						"Fault: Something in queue, but child %d (fd=%d) has exceeded wait time",
						head_ptr->pid, head_ptr->ctlsockfd);
				// Should send new 9977 'test aborted' signal to client.  Using this for now
				// rac 3/26/10

				log_println(6,
						"pid=%d, client='%s', stime=%ld, qtime=%ld now=%ld",
						head_ptr->pid, head_ptr->addr, head_ptr->stime,
						head_ptr->qtime, time(0));
				log_println(
						6,
						"pipe-fd=%d, running=%d, ctlsockfd=%d, client-type=%d, tests='%s'",
						head_ptr->pipe, head_ptr->running, head_ptr->ctlsockfd,
						head_ptr->oldclient, head_ptr->tests);
				send_msg(head_ptr->ctlsockfd, SRV_QUEUE, "9988", 4); // boot the client
				shutdown(head_ptr->ctlsockfd, SHUT_WR);
				close(head_ptr->ctlsockfd);
				tpid = head_ptr->pid;
				child_sig(-1);
				kill(tpid, SIGTERM);
				child_sig(tpid);
				continue;
			}
		}

		if ((multiple == 1) && (mclients > waiting)) {
			log_println(
					5,
					"Multi-client mode has uncaught terminated clients mclient=%d, waiting=%d",
					mclients, waiting);
			while (mclients > waiting) {
				child_sig(0);
				mclients--;
				log_println(5,
						"Removed terminated client from count mclients=%d",
						mclients);
			}
		}

		FD_ZERO(&rfd);
		FD_SET(listenfd, &rfd);
		if (waiting > 0) { // there are clients waiting
			sel_tv.tv_sec = 3; // 3 seconds == WAIT_TIME_SRVR
			sel_tv.tv_usec = 0;
			log_println(3, "Waiting for new connection, timer running");
			sel_11: retcode = select(listenfd + 1, &rfd, NULL, NULL, &sel_tv);
			if ((retcode == -1) && (errno == EINTR))
				/* continue; */ // a signal caused the select() to exit, re-enter loop & check
				goto sel_11;
			tt = time(0);

		} else {
			// Nothing is in the queue, so wait forever until a new connection request arrives
			log_println(3, "Timer not running, waiting for new connection");
			mclients = 0;
			sel_12: retcode = select(listenfd + 1, &rfd, NULL, NULL, NULL);
			if ((retcode == -1) && (errno == EINTR))
				goto sel_12;
			// a signal caused the select() to exit, re-enter loop & check
		}

		if (retcode < 0) {
			// an interrupt or signal caused the select() to exit, go back and start over
			log_println(5, "Select exited with return code = %d", retcode);
			continue;
		}

		if (retcode == 0) { // select exited due to timer expiration
			log_println(3, "Timer expired while waiting for a new connection");
			/* if ((waiting > 0) && (testing == 0)) */
			if (multiple == 0) {
				if ((waiting > 0) && (testing == 0))
					goto ChldRdy;
				continue;
			} else {
				if (waiting > 0)
					goto ChldRdy;
				continue;
			}
		} else {
			log_println(
					3,
					"New connection received, waiting for accept() to complete");
			if ((waiting > 0) && (testing == 0)) // no clients waiting, no test in progress
				goto ChldRdy;
			/* } */
			clilen = sizeof(cli_addr);
			memset(&cli_addr, 0, clilen);
			log_println(6, "Select() found %d clients ready, highest fd=%d",
					retcode, listenfd);
			if (retcode > 1) {
				for (i = 3; i <= listenfd; i++) {
					if (FD_ISSET(i, &rfd)) {
						listenfd = i;
						break;
					}
				}
			}
			for (i = 0; i < RETRY_COUNT; i++) {
				ctlsockfd = 0;
				ctlsockfd = accept(listenfd, (struct sockaddr *) &cli_addr,
						&clilen);

				if ((ctlsockfd == -1) && (errno == EINTR)) // socket interrupted, retry
					continue; // sig child
				size_t tmpstrlen = sizeof(tmpstr);
				memset(tmpstr, 0, tmpstrlen);
				// get addr details based on socket info available
				I2Addr tmp_addr = I2AddrBySockFD(get_errhandle(), ctlsockfd,
						False);
				I2AddrNodeName(tmp_addr, tmpstr, &tmpstrlen);
				/* I2AddrFree(tmp_addr); */
				log_println(4,
						"New connection received from 0x%x [%s] sockfd=%d.",
						tmp_addr, tmpstr, ctlsockfd);

				break;
			}

			// verify that accept really worked and don't process connections that've failed
			// RAC 2/11/10
			if (ctlsockfd <= 0) {
				log_println(4,
						"New connection request failed sockfd=%d reason-%d.",
						ctlsockfd, errno);
				continue;
			} else { // accept worked, log protocol validation log to indicate that client tried connecting
				procstatusenum = PROCESS_STARTED;
				proctypeenum = CONNECT_TYPE;
				protolog_procstatus(getpid(), getCurrentTest(), proctypeenum,
						procstatusenum, ctlsockfd );
			}

			// the specially crafted data that kicks off the old clients
			for (i = 0; i < RETRY_COUNT; i++) {
				retcode = write(ctlsockfd, "123456 654321", 13);
				if ((retcode == -1) && (errno == EINTR)) // interrupted, retry
					continue;
				if (retcode == 13) // 13 bytes correctly written, exit
					break;
				if (retcode == -1) { // socket error hindering further retries
					log_println(1,
							"Initial contact with client failed errno=%d",
							errno);
					close(chld_pipe[0]);
					close(chld_pipe[1]);
					shutdown(ctlsockfd, SHUT_WR);
					close(ctlsockfd);
					goto mainloop;
				}

				log_println(6, "xxx, calling initialize_tests()");
				t_opts = initialize_tests(ctlsockfd, &testopt, test_suite);

				if (t_opts < 1) {
					log_println(
							3,
							"Invalid test suite string '%s' received, terminate child",
							test_suite);
					close(chld_pipe[0]);
					close(chld_pipe[1]);
					shutdown(ctlsockfd, SHUT_WR);
					close(ctlsockfd);

					/* todo: handle other error contitions */
				}
			}
			new_child = (struct ndtchild *) malloc(sizeof(struct ndtchild));
			memset(new_child, 0, sizeof(struct ndtchild));
			tt = time(0);
			name = tmpstr;

			rmt_host = tmpstr;

			// At this point we have received a connection from a client, meaning that
			//  a test is being requested.  At this point we should apply any policy
			//  or AAA functions to the incoming connection.  If we don't like the
			//  client, we can refuse the connection and loop back to the begining.
			//  There would need to be some additional logic installed if this AAA
			//  test relied on more than the client's IP address.  The client would
			//  also require modification to allow more credentials to be created/passed
			//  between the user and this application.

		}

		if (pipe(chld_pipe) == -1)
			log_println(6, "pipe() failed errno=%d", errno);
		chld_pid = fork();

		switch (chld_pid) {
		case -1: /* an error occured, log it and quit */
			log_println(0, "fork() failed, errno = %d", errno);
			/* todo: handle error and continue */
			break;
		default: // this is the parent process, handle scheduling and
				 // queuing of multiple incoming clients

			log_println(5, "Parent process spawned child = %d", chld_pid);
			log_println(5, "Parent thinks pipe() returned fd0=%d, fd1=%d",
					chld_pipe[0], chld_pipe[1]);

			close(chld_pipe[0]);

			// Check to see if we have more than max_clients waiting in the queue
			//  If so, tell them to go away.
			//  changed for M-Lab deployment  1/28/09  RAC

			if (((multiple == 0) && (waiting >= (max_clients - 1)))
					|| ((multiple == 1) && (waiting >= ((4 * max_clients) - 1)))) {
				log_println(
						0,
						"Too many clients/mclients (%d) waiting to be served, Please try again later.",
						chld_pid);
				sprintf(tmpstr, "9988"); // 9988 = server_BUSY_OR_ERROR
				send_msg(ctlsockfd, SRV_QUEUE, tmpstr, strlen(tmpstr));
				close(chld_pipe[0]);
				close(chld_pipe[1]);
				shutdown(ctlsockfd, SHUT_WR);
				close(ctlsockfd);
				kill(chld_pid, SIGTERM);
				if (new_child != NULL) {
					log_println(6, "Too many clients freeing child=0x%x",
							new_child);
					free(new_child);
				}
				continue;
			}

			t_opts = initialize_tests(ctlsockfd, &testopt, test_suite);
			if (t_opts < 1) { // some error in initialization routines
				log_println(
						3,
						"Invalid test suite string '%s' received, terminate child",
						test_suite);
				close(chld_pipe[0]);
				close(chld_pipe[1]);
				shutdown(ctlsockfd, SHUT_WR);
				close(ctlsockfd);
				kill(chld_pid, SIGTERM);
				if (new_child != NULL) {
					log_println(6, "invalid test suite, freeing new_child=0x%x",
							new_child);
					free(new_child);
				}
				continue;
			}
			while ((retcode = sem_wait(&ndtq)) == -1 && errno == EINTR) { // socket interrupt, retry
				log_println(6, "Waiting for ndtq semaphore to free - 1");
				continue;
			}
			log_println(6, "creating new child - semaphore locked");
			/*sigprocmask(SIG_BLOCK, &newmask, &oldmask); */
			new_child->pid = chld_pid;
			/* strncpy(new_child->addr, rmt_host, strlen(rmt_host));
			 strncpy(new_child->host, name, strlen(name));*/
			strlcpy(new_child->addr, rmt_host, sizeof(new_child->addr));
			strlcpy(new_child->host, name, sizeof(new_child->host));

			// compute start time based in the number of waiting clients
			// set other properties on this child process
			if (multiple == 0)
				new_child->stime = tt + (waiting * 45);
			else
				new_child->stime = tt + ((waiting / max_clients) * 45);
			new_child->qtime = tt;
			new_child->pipe = chld_pipe[1];
			new_child->running = 0;
			new_child->ctlsockfd = ctlsockfd;
			if (t_opts & TEST_STATUS
				)
				new_child->oldclient = 1;
			else
				new_child->oldclient = 0;
			memset(new_child->tests, 0, sizeof(test_suite));
			memcpy(new_child->tests, test_suite, strlen(test_suite));
			new_child->next = NULL;
			/* sigprocmask(SIG_SETMASK, &oldmask, NULL); */
			sem_post(&ndtq);
			log_println(6, "Free'd ndtq semaphore lock - 1");
			if (multiple == 1)
				mwaiting++;
			mchild = new_child;

			log_println(
					3,
					"initialize_tests returned old/new client = %d, test_suite = %s",
					new_child->oldclient, new_child->tests);

			close(chld_pipe[0]);

			if ((testing == 1) && (queue == 0)) {
				log_println(
						3,
						"queuing disabled and testing in progress, tell client no");
				/* send_msg(new_child->ctlsockfd, SRV_QUEUE, "9944", 4); */
				send_msg(new_child->ctlsockfd, SRV_QUEUE, "9988", 4);
				close(chld_pipe[1]);
				shutdown(new_child->ctlsockfd, SHUT_WR);
				close(new_child->ctlsockfd);
				log_println(6, "no queuing, free new_child=0x%x", new_child);
				free(new_child);
				continue;
			}

			waiting++; // new client has arrived and is queue-able. increment waiting
			log_println(5, "Incrementing waiting variable now = %d", waiting);
			if (head_ptr == NULL)
				head_ptr = new_child;
			else {
				log_println(
						4,
						"New request has arrived, adding request to queue list");
				while ((retcode = sem_wait(&ndtq)) == -1 && errno == EINTR) {
					log_println(
							6,
							"Waiting for ndtq semaphore to free, adding new client 2");
					continue;
				}
				log_println(6,
						"Adding new client to fifo queue - semaphore locked");
				tmp_ptr = head_ptr;
				while (tmp_ptr->next != NULL)
					tmp_ptr = tmp_ptr->next;
				tmp_ptr->next = new_child;
				sem_post(&ndtq);
				log_println(6, "Free'd ndtq semaphore lock - 2");

				if (get_debuglvl() > 3) {
					log_println(4, "Walking scheduling queue");
					tmp_ptr = head_ptr;
					while (tmp_ptr != NULL) {
						log_println(4, "\tChild %d, host: %s [%s], next=0x%x",
								tmp_ptr->pid, tmp_ptr->host, tmp_ptr->addr,
								(u_int64_t) tmp_ptr->next);
						if (tmp_ptr->next == NULL)
							break;
						tmp_ptr = tmp_ptr->next;
					}
				}
			}

			// At this point send a message to the client via the ctlsockfd
			//  saying that N clients are waiting in the queue & testing will
			//   begin within Nx60 seconds.  Only if (waiting > 0)
			//    Clients who leave will be handled in the run_test routine.

			if ((multiple == 0) && (waiting > 1)) {
				log_println(
						3,
						"%d clients waiting, telling client (%d) testing will begin within %d minutes",
						(waiting - 1), tmp_ptr->pid, (waiting-1));
						sprintf(tmpstr, "%d", (waiting-1));
						send_msg(tmp_ptr->ctlsockfd, SRV_QUEUE, tmpstr, strlen(tmpstr));
	  continue;
        }

				if ((multiple == 1) && (waiting >= max_clients)) {
					int xx = waiting/max_clients;
					log_println(3, "%d mclients waiting, tell client (%d) test will begin within %d minutes",
							(waiting-max_clients), mchild->pid, xx);
					sprintf(tmpstr, "%d", xx);
					send_msg(mchild->ctlsockfd, SRV_QUEUE, tmpstr, strlen(tmpstr));
					continue;
				}

				/*
				 *       if ((multiple == 0) && (testing == 1)) 
				 *         continue;
				 */

				log_println(3, "Line just before ChldRdy: label, did't get here via the goto statement!");
				ChldRdy:
				log_println(3, "___-------______  client process is ready for testing!");

				// There are clients waiting in the queue, but if running in multi-client mode and
				// there are already the max number of mclients being served, then just continue
				// 9/2/09 RAC

				if ((multiple ==1) && (mclients >= max_clients))
				continue;

				testing = 1;

				// update all the clients so they have some idea that progress is occurring
				if ((multiple == 0) && (waiting > 1)) {
					j = waiting - 1;
					tmp_ptr = head_ptr->next;
					if (tmp_ptr == NULL) // no more clients waiting after the current one
					waiting = 1;
					while (tmp_ptr != NULL) {
						log_println(3, "%d clients waiting, updating client %d testing will begin within %d minutes",
								(waiting-1), tmp_ptr->pid, (waiting-j));

						sprintf(tmpstr, "%d", (waiting-j));
						send_msg(tmp_ptr->ctlsockfd, SRV_QUEUE, tmpstr, strlen(tmpstr));
						tmp_ptr = tmp_ptr->next;
						j--;
					}
					if ((waiting > 5) && (zombie_check == 0)) { // more then 5 clients waiting, killing zombies
						zombie_check = -1;
						log_println(4, "More than 5 clients in the queue, remove zombies");
						if (pthread_create(&zombieThreadId, NULL, zombieWorker, (void *)head_ptr)) {
							log_println(0, "Cannot create thread to kill off zombie clients!");
							zombie_check = 0;
						}
						if (zombie_check > 0) {
							waiting -= zombie_check;
							zombie_check = 0;
						}
					}
				}

				/* multi_client: */
				if ((multiple == 1) && (mclients < max_clients)) {
					if (mwaiting == 0)
					continue;
					mwaiting--;
					log_println(3, "Multi-client mode operation, dispatch a new client pid=%d.", mchild->pid);
				}
				else
				log_println(3, "Telling client %d testing will begin now", head_ptr->pid);

				// at this point we have successfully set everything up and are ready to
				//  start testing.  The write() on the control socket tells the client
				//  that the applet should drop out of the wait loop and get ready to begin
				//  testing.  The write() on the pipe tells the child process it's at the
				//  head of the queue, or it's in multi-client mode, and its OK for this
				//  client process to wake up and begin testing.
				//
				//  This is the point in time where we can/should contact any meta
				//  scheduler to verify that it is OK to really start a test.  If
				//  we are sharing the server host with other applications, and we
				//  want unfettered access to the link, now is the time to make this
				//  request.

				dispatch_client:
				memset(tmpstr, 0, sizeof(tmpstr));
				if (multiple == 1) {
					if (mchild == NULL)
					mchild = head_ptr;
					if (mchild->running == 1)
					continue;
					log_println(3, "New mclient '%d'(%d) asking for service", mclients, mchild->pid);
					mchild->stime = time(0);
					mchild->running = 1;
					mclients++;
					sprintf(tmpstr, "go %d %s", t_opts, mchild->tests);
					log_println(5, "sending 'GO' signal to client msg='%s'", tmpstr);
					send_msg(mchild->ctlsockfd, SRV_QUEUE, "0", 1); // test session starts now
					for (i=0; i<5; i++) {
						retcode = write(mchild->pipe, tmpstr, strlen(tmpstr));
						log_println(6, "write(%d) returned %d, errno=%d", mchild->pid, retcode, errno);
						if ((retcode == -1) && (errno == EINTR))
						continue;
						if (retcode == strlen(tmpstr))
						break;
						log_println(6, "Failed to write 'GO' message to client %d, reason=%d, errno=%d",
								mchild->pid, retcode, errno);
						/* TODO: handle other error conditions */
						if (retcode == -1) {
							log_println(1, "Dispatch multi-client failed because '%s'", strerror(errno));
							shutdown(mchild->ctlsockfd, SHUT_WR);
							close(mchild->ctlsockfd);
							kill(chld_pid, SIGTERM);
							goto mainloop;
						}
					}
					close(mchild->pipe);
					close(mchild->ctlsockfd);
				} // multiple client loop complete
				else {
					head_ptr->stime = time(0);
					head_ptr->running = 1;
					sprintf(tmpstr, "go %d %s", t_opts, head_ptr->tests);
					log_println(5, "sending 'GO' signal to client msg='%s'", tmpstr);
					send_msg(head_ptr->ctlsockfd, SRV_QUEUE, "0", 1);
					for (i=0; i<5; i++) {
						retcode = write(head_ptr->pipe, tmpstr, strlen(tmpstr));
						if ((retcode == -1) && (errno == EINTR))
						continue;
						if (retcode == strlen(tmpstr))
						break;
						/* TODO: handle other error conditions */
						if (retcode == -1) {
							log_println(1, "Dispatch multi-client failed because '%s'", strerror(errno));
							shutdown(head_ptr->ctlsockfd, SHUT_WR);
							close(head_ptr->ctlsockfd);
							kill(chld_pid, SIGTERM);
							goto mainloop;
						}
					}
					close(head_ptr->pipe);
					close(head_ptr->ctlsockfd);
				}
				continue;
				break;
				case 0: // this is the child process, it handles
						//   the testing function
				log_println(4, "Child thinks pipe() returned fd0=%d, fd1=%d for pid=%d",
						chld_pipe[0], chld_pipe[1], chld_pid);
				close(listenfd);
				close(chld_pipe[1]);
				if ((agent = web100_attach(WEB100_AGENT_TYPE_LOCAL, NULL)) == NULL) {
					web100_perror("web100_attach");
					return 1;
				}

				// This is the child process from the above fork().  The parent
				//  is in control, and will send this child a signal when it gets
				//  to the head of the testing queue.  Post a read() and simply
				//  wait for the parent to let us know it's time to move on.
				//  Rich Carlson 3/11/04
				for (;;) {
					memset(buff, 0, sizeof(buff));

					// the read() could return if an interrupt was caught.  This condition
					// should be checked for and the read() restarted if necessary
					// RAC 3/18/10
					//
					retcode = read(chld_pipe[0], buff, 32);
					log_println(6, "Child %d received '%s' from parent", getpid(), buff);
					if ((retcode == -1) && (errno == EINTR))
					continue;
					if (strncmp(buff, "go", 2) == 0) {
						log_println(6, "Got 'go' signal from parent, ready to start testing %d", getpid());
						break;
					}
					if (buff[0] == 0) {
						log_println(6, "Child %d received 'go' signal, exiting", getpid());
						exit(-1);
					}
				}

				set_timestamp();

				// The next 3 instructions retrieve the passed in values for the tests to run.
				//  t_opts will be the bitmapped version of this data and test_suite will be a
				//   character string with the same info.  Note: this assumes that the structure
				//    of buff is fixed with the character strings starting in these locations.
				//     RAC 7/8/09

				memset(test_suite, 0, sizeof(test_suite));
				t_opts = atoi(buff+3);
				memcpy(test_suite, buff+5, (strlen(buff)-5));
				/* memcpy(test_suite, buff+6, 7); */
				log_println(5, "extracting test_suite '%s' and t_opts '%x' from buff '%s'", test_suite, t_opts, buff);

				// construct cputime log folder
				{
					I2Addr tmp_addr = I2AddrBySockFD(get_errhandle(), ctlsockfd, False);
					testPort = I2AddrPort(tmp_addr);
					meta.ctl_port = testPort;
					snprintf(testName, 250, "%s", name);
					I2AddrFree(tmp_addr);
					memset(cputimelog, 0, 256);
					if (cputime) {
						sprintf(dir, "%s_%s:%d.cputime", get_ISOtime(isoTime, sizeof(isoTime)), name, testPort);
						log_println(8, "CPUTIME:suffix=%s",dir);
						create_named_logdir(cputimelog, sizeof(cputimelog),dir);
						memcpy(meta.CPU_time, dir, strlen(dir));
						if (pthread_create(&workerThreadId, NULL, cputimeWorker, (void*) cputimelog)) {
							log_println(0, "Cannot create worker thread for writing cpu usage!");
							workerThreadId = 0;
							memset(cputimelog, 0, 256);
						}
					}
				}
				// write the incoming connection data into the log file
				fp = fopen(get_logfile(),"a");
				if (fp == NULL) {
					log_println(0, "Unable to open log file '%s', continuing on without logging", get_logfile());
				}
				else {
					fprintf(fp,"%15.15s  %s port %d\n",
							ctime(&tt)+4, name, testPort);
					if (cputime && workerThreadId) {
						log_println(1, "cputime trace file: %s\n", cputimelog);
						fprintf(fp, "cputime trace file: %s\n", cputimelog);
					}
					fclose(fp);
				}
				close(chld_pipe[0]);

				// Set test options
				if (t_opts & TEST_MID)
				testopt.midopt = TOPT_ENABLED;
				if (t_opts & TEST_SFW)
				testopt.sfwopt = TOPT_ENABLED;
				if (t_opts & TEST_META)
				testopt.metaopt = TOPT_ENABLED;
				if (t_opts & TEST_C2S)
				testopt.c2sopt = TOPT_ENABLED;
				if (t_opts & TEST_S2C)
				testopt.s2copt = TOPT_ENABLED;
				alarm(120);// die in 120 seconds, but only if a test doesn't get started
							 // reset alarm() before every test
				log_println(6, "setting master alarm() to 120 seconds, tests must start (complete?) before this timer expires");

							 // run tests based on options
				if (strncmp(test_suite, "Invalid", 7) != 0) {
					log_println(3, "Valid test sequence requested, run test for client=%d", getpid());
					retcode = run_test(agent, ctlsockfd, &testopt, test_suite);
				}

				// conclude all test runs
				if (retcode == 0)
				log_println(3, "Successfully returned from run_test() routine");
				else {
					log_println(3, "Child %d returned non-zero (%d) from run_test() results some test failed!", getpid(), retcode);
					child_sig(0);
				}
				close(ctlsockfd);
				web100_detach(agent);
				log_free();

				if (cputime && workerThreadId) {
					cputimeworkerLoop = 0;
					pthread_join(workerThreadId, NULL);
				}

				// At this point the tests have been run and we need to clean up and handle
				// and child processes that might still be lying around.  If we don't we get
				// zombies. The pkt-pair handling created 2 childern and we need to get
				// rid of them. To know what PIDs to look for on 1/19/10 the run_test()
				// routine was modified to pass around these values. The values are set in
				// the proper routine in the testoptions.c file.
				// Then add call to child_sig() routine and pass in these PID's so we handle
				// each child in sequence

				log_println(6, "remove pkt-pair children c2s=%d, s2c=%d", testopt.child1, testopt.child2);
				if (testopt.child1 != 0)
				child_sig(testopt.child1);
				if (testopt.child2 != 0)
				child_sig(testopt.child2);
				exit(0);
				break;
			}
		}
	}

	/**
	 * Method to get remote host's address.
	 * @return remote host name
	 * */
char *get_remotehost() {
	return rmt_host;
}

