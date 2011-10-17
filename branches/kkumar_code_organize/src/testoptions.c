/**
 * This file contains the functions needed to handle various tests.
 *
 * Jakub Sï¿½awiï¿½ski 2006-06-24
 * jeremian@poczta.fm
 */

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#include "testoptions.h"
#include "network.h"
#include "mrange.h"
#include "logging.h"
#include "utils.h"
#include "protocol.h"
#include "I2util/util.h"
#include "runningtest.h"
#include "strlutils.h"

int mon_pipe1[2]; // used to store file descriptors of pipes created for ndttrace for C2S tests
int mon_pipe2[2]; // used to store file descriptors of pipes created for ndttrace data in S2c test

// Snap log characteristics
typedef struct snapArgs {
	web100_snapshot* snap; // web_100 snapshot indicator
	web100_log* log; // web_100 log
	int delay; // periodicity, in ms, of collecting snap
} SnapArgs;

// Worker thread characteristics used to record snaplog and Cwnd peaks
typedef struct workerArgs {
	SnapArgs* snapArgs; // snapArgs struct pointer
	web100_agent* agent; // web_100 agent pointer
	CwndPeaks* peaks; // data indicating Cwnd values
	int writeSnap; // enable writing snaplog
} WorkerArgs;

static int workerLoop = 0; // semaphore
static pthread_mutex_t mainmutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t maincond = PTHREAD_COND_INITIALIZER;
static int slowStart = 1;
static int prevCWNDval = -1;
static int decreasing = 0;

// error codes used to indicate status of test runs
static const int LISTENER_SOCKET_CREATE_FAILED = -1;
static const int SOCKET_CONNECT_TIMEOUT = -100;
static const int RETRY_EXCEEDED_WAITING_CONNECT = -101;
static const int RETRY_EXCEEDED_WAITING_DATA = -102;
static const int SOCKET_STATUS_FAILED = -1;

//other constants
static const int ETHERNET_MTU_SIZE = 1456;

/**
 * Count the CWND peaks from a snapshot and record the minimal and maximum one.
 * Also record the number of transitions between increasing or decreasing
 * trends of the values.
 * @param agent Web100 agent used to track the connection
 * @param peaks Structure containing CWND peaks information
 * @param snap Web100 snapshot structure
 */

void findCwndPeaks(web100_agent* agent, CwndPeaks* peaks, web100_snapshot* snap) {
	web100_group* group;
	web100_var* var;
	int CurCwnd;
	char tmpstr[256];

	web100_agent_find_var_and_group(agent, "CurCwnd", &group, &var);
	web100_snap_read(var, snap, tmpstr);
	CurCwnd = atoi(web100_value_to_text(web100_get_var_type(var), tmpstr));

	if (slowStart) {
		if (CurCwnd < prevCWNDval) {
			slowStart = 0;
			peaks->max = prevCWNDval;
			peaks->amount = 1;
			decreasing = 1;
		}
	} else {
		// current congestion window < previous value, so, decreasing
		if (CurCwnd < prevCWNDval) {
			// update values based on actual values
			if (prevCWNDval > peaks->max) {
				peaks->max = prevCWNDval;
			}
			if (!decreasing) {
				peaks->amount += 1;
			}
			decreasing = 1;
		} else if (CurCwnd > prevCWNDval) { // current congestion window size > previous value,
											// not decreasing.
			if ((peaks->min == -1) || (prevCWNDval < peaks->min)) {
				peaks->min = prevCWNDval;
			}
			decreasing = 0;
		}
	}
	prevCWNDval = CurCwnd;
}

/**
 * Print the appropriate message when the SIGALRM is caught.
 * @param signo The signal number (shuld be SIGALRM)
 */

void catch_s2c_alrm(int signo) {
	if (signo == SIGALRM) {
		log_println(1, "SIGALRM was caught");
		return;
	}
	log_println(0, "Unknown (%d) signal was caught", signo);
}

/**
 * Write the snap logs with fixed time intervals in a separate
 *              thread, locking and releasing resources as necessary.
 * @param arg pointer to the snapshot structure
 * @return void pointer null
 */

void*
snapWorker(void* arg) {
	WorkerArgs *workerArgs = (WorkerArgs*) arg;
	SnapArgs *snapArgs = workerArgs->snapArgs;
	web100_agent* agent = workerArgs->agent;
	CwndPeaks* peaks = workerArgs->peaks;
	int writeSnap = workerArgs->writeSnap;

	// snap log written into every "delay" milliseconds
	double delay = ((double) snapArgs->delay) / 1000.0;

	while (1) {
		pthread_mutex_lock(&mainmutex);
		if (workerLoop) {
			pthread_mutex_unlock(&mainmutex);
			pthread_cond_broadcast(&maincond);
			break;
		}
		pthread_mutex_unlock(&mainmutex);
		mysleep(0.01);
	}

	// Find Congestion window peaks from a web_100 snapshot, if enabled
	// Write snap log , if enable, all in a synchronous manner.
	while (1) {
		pthread_mutex_lock(&mainmutex);
		if (!workerLoop) {
			pthread_mutex_unlock(&mainmutex);
			break;
		}
		web100_snap(snapArgs->snap);
		if (peaks) {
			findCwndPeaks(agent, peaks, snapArgs->snap);
		}
		if (writeSnap) {
			web100_log_write(snapArgs->log, snapArgs->snap);
		}
		pthread_mutex_unlock(&mainmutex);
		mysleep(delay);
	}

	return NULL;
}

/**
 * Adds test id to the test suite
 * @param first first test indicator
 * @param buff test suite description
 * @param test_id id of the test
 */
void add_test_to_suite(int* first, char * buff, int test_id) {
	char tmpbuff[16];
	if (*first) {
		*first = 0;
		sprintf(buff, "%d", test_id);
	} else {
		memset(tmpbuff, 0, 16);
		sprintf(tmpbuff, " %d", test_id);
		//strcat(buff, tmpbuff);
		strlcat(buff, tmpbuff, 16); //setting buffsize= 16 as is initialized in main()
	}
}

/**
 * Initialize the tests for the client.
 * @param ctlsockfd Client control socket descriptor
 * @param options   Test options
 * @param buff 		Connection options
 * @return integer 0 on success,
 *          >0 - error code.
 *          Error codes:
 *          -1 message reading in error
 *			-2 Invalid test request
 *			-3 Invalid test suite request
 *			-4 client timed out
 *
 */

int initialize_tests(int ctlsockfd, TestOptions* options, char * buff) {
	unsigned char useropt = 0;
	int msgType;
	int msgLen = 1;
	int first = 1;
	char remhostarr[256], protologlocalarr[256];
	char *remhost_ptr = get_remotehost();

	assert(ctlsockfd != -1);
	assert(options);

	// read the test suite request
	if (recv_msg(ctlsockfd, &msgType, &useropt, &msgLen)) {
		sprintf(buff, "Invalid test suite request");
		send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
		return (-1);
	}
	if (msgLen == -1) {
		sprintf(buff, "Client timeout");
		return (-4);
	}
	// Expecting a MSG_LOGIN with payload byte indicating tests to be run
	if ((msgType != MSG_LOGIN) || (msgLen != 1)) {
		sprintf(buff, "Invalid test request");
		send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
		return (-2);
	}
	// client connect received correctly. Logging activity
	// log that client connected, and create log file
	log_println(0,
			"Client connect received from :IP %s to some server on socket %d",
			get_remotehost(), ctlsockfd);

	set_protologfile(get_remotehost(), protologlocalarr);

	if (!(useropt
			& (TEST_MID | TEST_C2S | TEST_S2C | TEST_SFW | TEST_STATUS
					| TEST_META))) {
		// message received does not indicate a valid test!
		sprintf(buff, "Invalid test suite request");
		send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
		return (-3);
	}
	// construct test suite request based on user options received
	if (useropt & TEST_MID) {
		add_test_to_suite(&first, buff, TEST_MID);
	}
	if (useropt & TEST_SFW) {
		add_test_to_suite(&first, buff, TEST_SFW);
	}
	if (useropt & TEST_C2S) {
		add_test_to_suite(&first, buff, TEST_C2S);
	}
	if (useropt & TEST_S2C) {
		add_test_to_suite(&first, buff, TEST_S2C);
	}
	if (useropt & TEST_META) {
		add_test_to_suite(&first, buff, TEST_META);
	}
	return useropt;
}

/** Method to start snap worker thread that collects snap logs
 * @param snaparg object
 * @param web100_agent Agent
 * @param snaplogenabled Is snap logging enabled?
 * @param workerlooparg integer used to syncronize writing/reading from snaplog/web100 snapshot
 * @param wrkrthreadidarg Thread Id of worker
 * @param metafilevariablename Which variable of the meta file gets assigned the snaplog name (unused now)
 * @param metafilename	value of metafile name
 * @param web100_connection connection pointer
 * @param web100_group group web100_group pointer
 */
void start_snap_worker(SnapArgs *snaparg, web100_agent *agentarg,
		char snaplogenabled, int *workerlooparg, pthread_t *wrkrthreadidarg,
		char *metafilevariablename, char *metafilename, web100_connection* conn,
		web100_group* group) {
	FILE *fplocal;

	WorkerArgs workerArgs;
	workerArgs.snapArgs = snaparg;
	workerArgs.agent = agentarg;
	workerArgs.peaks = NULL;
	workerArgs.writeSnap = snaplogenabled;

	group = web100_group_find(agentarg, "read");
	snaparg->snap = web100_snapshot_alloc(group, conn);

	if (snaplogenabled) {

		//memcpy(metafilevariablename, metafilename, strlen(metafilename));
		// The above could have been here, except for a caveat: metafile stores
		// just the file name, but full filename is needed to open the log file

		fplocal = fopen(get_logfile(), "a");
		snaparg->log = web100_log_open_write(metafilename, conn, group);
		if (fplocal == NULL) {
			log_println(
					0,
					"Unable to open log file '%s', continuing on without logging",
					get_logfile());
		} else {
			log_println(0, "Snaplog file: %s\n", metafilename);
			fprintf(fplocal, "Snaplog file: %s\n", metafilename);
			fclose(fplocal);
		}
	}

	if (pthread_create(wrkrthreadidarg, NULL, snapWorker,
			(void*) &workerArgs)) {
		log_println(0, "Cannot create worker thread for writing snap log!");
		*wrkrthreadidarg = 0;
	}

	pthread_mutex_lock(&mainmutex);
	*workerlooparg = 1;
	// obtain web100 snap into "snaparg.snap"
	web100_snap(snaparg->snap);
	if (snaplogenabled) {
		web100_log_write(snaparg->log, snaparg->snap);
	}
	pthread_cond_wait(&maincond, &mainmutex);
	pthread_mutex_unlock(&mainmutex);
}

/**
 * Stop snapWorker
 * @param workerThreadId Worker Thread's ID
 * @param snaplogenabled boolean indication whether snap logging is enabled
 * @param snapArgs_ptr  pointer to a snapArgs object
 * */
void stop_snap_worker(int *workerThreadId, char snaplogenabled,
		SnapArgs* snapArgs_ptr) {
	if (*workerThreadId) {
		pthread_mutex_lock(&mainmutex);
		workerLoop = 0;
		pthread_mutex_unlock(&mainmutex);
		pthread_join(*workerThreadId, NULL);
	}
	// close writing snaplog, if snaplog recording is enabled
	if (snaplogenabled) {
		web100_log_close_write(snapArgs_ptr->log);
	}

	web100_snapshot_free(snapArgs_ptr->snap);

}

/**
 * Start packet tracing for this client
 * @param socketfdarg socket file descriptor to initialize packet trace from
 * @param socketfdarg socket file descriptor to close
 * @param childpid pid resulting from fork()
 * @param imonarg int array of socket fd from pipe
 * @param cliaddrarg sock_addr used to determine client IP
 * @param clilenarg socket length
 * @param device device type
 * @param pairarg portpair struct instance
 * @param testindicator string indicating C2S/S2c test
 * @param iscompressionenabled  is compression enabled  (for log files)?
 * @param copylocationarg memory location to copy resulting string (from packet trace) into
 * */

void start_packet_trace(int socketfdarg, int socketfdarg2, pid_t *childpid,
		int *imonarg, struct sockaddr *cliaddrarg, socklen_t clilenarg,
		char* device, PortPair* pairarg, const char* testindicatorarg,
		int iscompressionenabled, char *copylocationarg) {

	char tmpstr[256];
	int i, readretval;

	I2Addr src_addr = I2AddrByLocalSockFD(get_errhandle(), socketfdarg, 0);

	if ((*childpid = fork()) == 0) {

		close(socketfdarg2);
		close(socketfdarg);
		log_println(0, "%s test Child thinks pipe() returned fd0=%d, fd1=%d",
				testindicatorarg, imonarg[0], imonarg[1]);

		init_pkttrace(src_addr, cliaddrarg, clilenarg, imonarg, device,
				&pairarg, testindicatorarg, iscompressionenabled);

		exit(0); // Packet trace finished, terminate gracefully
	}
	memset(tmpstr, 0, 256);
	for (i = 0; i < 5; i++) { // read nettrace file name into "tmpstr"
		readretval = read(imonarg[0], tmpstr, 128);
		if ((readretval == -1) && (errno == EINTR)) // socket interrupted, try reading again
			continue;
		break;
	}

	// name of nettrace file passed back from pcap child copied into meta structure
	if (strlen(tmpstr) > 5) {
		memcpy(copylocationarg, tmpstr, strlen(tmpstr));
		log_println(8, "**start pkt trace, memcopied dir name %s", tmpstr);
	}

	// free this address now.
	I2AddrFree(src_addr);

}

/**
 * Stop packet tracing activity.
 * @param monpipe_arr pointer to the monitor pipe file-descriptor array
 *
 * */
void stop_packet_trace(int *monpipe_arr) {
	int retval;
	int i;

	for (i = 0; i < RETRY_COUNT; i++) {
		retval = write(mon_pipe1[1], "c", 1);
		if (retval == 1)
			break;
		if ((retval == -1) && (errno == EINTR))
			continue;
	}
	close(monpipe_arr[0]);
	close(monpipe_arr[1]);

}

/**
 * Set Cwnd limit
 * @param connarg web100_connection pointer
 * @param group_arg web100 group pointer
 * @param agentarg web100 agent pointer
 * */
void setCwndlimit(web100_connection* connarg, web100_group* grouparg,
		web100_agent* agentarg, Options* optionsarg) {

	web100_var *LimRwin, *yar;
	u_int32_t limrwin_val;
	char yuff[32];

	if (optionsarg->limit > 0) {
		log_print(1, "Setting Cwnd limit - ");

		if (connarg != NULL) {
			log_println(1,
					"Got web100 connection pointer for recvsfd socket\n");
			web100_agent_find_var_and_group(agentarg, "CurMSS", &grouparg,
					&yar);
			web100_raw_read(yar, connarg, yuff);
			log_println(1, "MSS = %s, multiplication factor = %d",
					web100_value_to_text(web100_get_var_type(yar), yuff),
					optionsarg->limit);
			limrwin_val = optionsarg->limit
					* (atoi(
							web100_value_to_text(web100_get_var_type(yar),
									yuff)));
			web100_agent_find_var_and_group(agentarg, "LimRwin", &grouparg,
					&LimRwin);
			log_print(1, "now write %d to limit the Receive window",
					limrwin_val);
			web100_raw_write(LimRwin, connarg, &limrwin_val);
			log_println(1, "  ---  Done");
		}

	}
}

/**
 * Check if receiver is clogged and use decision to temporarily
 * stop sending packets.
 * @param nextseqtosend integer indicating the Next Sequence Number To Be Sent
 * @param lastunackedseq integer indicating the oldest un-acked sequence number
 * @return integer indicating whether buffer is clogged
 * */
int is_buffer_clogged(int nextseqtosend, int lastunackedseq) {
	int recclog = 0;
	if ((RECLTH << 2) < (nextseqtosend - lastunackedseq - 1)) {
		recclog = 1;
	}
	return recclog;
}

/**
 * Perform the Middlebox test.  This is a brief throughput test from the Server to the Client
 * with a limited CWND to check for a duplex mismatch condition.
 * This test also uses a pre-defined MSS value to check if any intermediate node
 * is modifying connection settings.
 *
 * @param ctlsockfd Client control socket descriptor
 * @param agent Web100 agent used to track the connection
 * @param options  Test options
 * @param s2c_throughput_mid In-out parameter for S2C throughput results (evaluated by the MID TEST),
 * @param conn_options Connection options
 * @return 0 - success,
 *          >0 - error code.
 *          Error codes:
 * 				-1 - Listener socket creation failed
 *				-3 - web100 connection data not obtained
 *				-100 - timeout while waiting for client to connect to serverÕs ephemeral port
 *				-errno- Other specific socket error numbers
 *				-101 - Retries exceeded while waiting for client to connect
 *				-102 - Retries exceeded while waiting for data from connected client
 *			Other used return codes:
 *				1 - Message reception errors/inconsistencies
 *				2 - Unexpected message type received/no message received due to timeout
 *				3 Ð Received message is invalid
 *
 */

int test_mid(int ctlsockfd, web100_agent* agent, TestOptions* options,
		int conn_options, double* s2c_throughput_mid) {
	int maxseg = ETHERNET_MTU_SIZE;
	/* int maxseg=1456, largewin=16*1024*1024; */
	/* int seg_size, win_size; */
	int midsfd; // socket file-descriptor, used in mid-box throughput test from S->C
	int j; // temporary integer store
	int msgretvalue; // return value from socket read/writes
	struct sockaddr_storage cli_addr;
	/* socklen_t optlen, clilen; */
	socklen_t clilen;
	char buff[BUFFSIZE + 1]; // buf used for message payload
I2Addr 	midsrv_addr = NULL; // server address
	char listenmidport[10]; // listener socket for middlebox tests
	int msgType;
	int msgLen;
	web100_connection* conn;
	char tmpstr[256]; // temporary string storage
	struct timeval sel_tv; // time
	fd_set rfd; // receiver file descriptor

	// variables used for protocol validation logging
	enum TEST_ID thistestId = NONE;
	enum TEST_STATUS_INT teststatusnow = TEST_NOT_STARTED;
	enum PROCESS_STATUS_INT procstatusenum = PROCESS_STARTED;
	enum PROCESS_TYPE_INT proctypeenum = CONNECT_TYPE;

	assert(ctlsockfd != -1);
	assert(agent);
	assert(options);
	assert(s2c_throughput_mid);

	if (options->midopt) { // middlebox tests need to be run.

		// Start with readying up (initializing)
		setCurrentTest(TEST_MID);
		log_println(1, " <-- %d - Middlebox test -->", options->child0);

		// protocol validation logs indicating start of tests
		printf(" <--- %d - Middlebox test --->", options->child0);
		thistestId = MIDDLEBOX;
		teststatusnow = TEST_STARTED;
		protolog_status(options->child0, thistestId, teststatusnow);

		// determine port to be used. Compute based on options set earlier
		// by reading from config file, or use default port3 (3003),
		//strcpy(listenmidport, PORT3);
		strlcpy(listenmidport, PORT3, sizeof(listenmidport));

		if (options->midsockport) {
			sprintf(listenmidport, "%d", options->midsockport);
		} else if (options->mainport) {
			sprintf(listenmidport, "%d", options->mainport + 2);
		}

		if (options->multiple) {
			//strcpy(listenmidport, "0");
			strlcpy(listenmidport, "0", sizeof(listenmidport));
		}

		/*  RAC debug  */
		/*
		 if (KillHung() == 0)
		 log_println(5, "KillHung() returned 0, should have tried to kill off some LastAck process");
		 else
		 log_println(5, "KillHung(): returned non-0 response, nothing to kill or kill failed");
		 */

		while (midsrv_addr == NULL) {

			// attempt to bind to a new port and obtain address structure with details of listening port

			midsrv_addr =
					CreateListenSocket(
							NULL,
							(options->multiple ?
									mrange_next(listenmidport) : listenmidport), conn_options, 0)
							;
			if (midsrv_addr == NULL) {
				/*
				 log_println(5, " Calling KillHung() because midsrv_address failed to bind");
				 if (KillHung() == 0)
				 continue;
				 */
			}
			if (strcmp(listenmidport, "0") == 0) {
				log_println(0, "WARNING: ephemeral port number was bound");
				break;
			}
			if (options->multiple == 0) { // simultaneous tests from multiple clients not allowed, quit now
				break;
			}
		}

		if (midsrv_addr == NULL) {
			log_println(0,
					"Server (Middlebox test): CreateListenSocket failed: %s",
					strerror(errno));
			sprintf(buff,
					"Server (Middlebox test): CreateListenSocket failed: %s",
					strerror(errno));
			send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
			return -1;
		}

		// get socket FD and the ephemeral port number that client will connect to
		options->midsockfd = I2AddrFD(midsrv_addr);
		options->midsockport = I2AddrPort(midsrv_addr);
		log_println(1, "  -- port: %d", options->midsockport);

		// send this port number to client
		sprintf(buff, "%d", options->midsockport);
		if ((msgretvalue = send_msg(ctlsockfd, TEST_PREPARE, buff, strlen(buff)))
				< 0)
			return msgretvalue;

		/* set mss to 1456 (strange value), and large snd/rcv buffers
		 * should check to see if server supports window scale ?
		 */
		setsockopt(options->midsockfd, SOL_TCP, TCP_MAXSEG, &maxseg,
				sizeof(maxseg));

		/* Post a listen on port 3003.  Client will connect here to run the 
		 * middlebox test.  At this time it really only checks the MSS value
		 * and does NAT detection.  More analysis functions (window scale)
		 * will be done in the future.
		 */
		clilen = sizeof(cli_addr);

		// Wait on listening socket and read data once ready.
		// Choose a retry count of "5" to wait for activity on the socket
		FD_ZERO(&rfd);
		FD_SET(options->midsockfd, &rfd);
		sel_tv.tv_sec = 5;
		sel_tv.tv_usec = 0;
		for (j = 0; j < RETRY_COUNT; j++) {
			msgretvalue = select((options->midsockfd) + 1, &rfd, NULL, NULL,
					&sel_tv);
			if ((msgretvalue == -1) && (errno == EINTR)) // socket interruption. continue waiting for activity on socket
				continue;
			if (msgretvalue == 0) // timeout
				return SOCKET_CONNECT_TIMEOUT;
			if (msgretvalue < 0) // other socket errors, exit
				return -errno;
			if (j == 4) // retry exceeded. Quit
				return RETRY_EXCEEDED_WAITING_CONNECT;

			midfd:
			// if a valid connection request is received, client has connected. Proceed.
			// Note the new socket fd used in the throughput test is this (midsfd)
			if ((midsfd = accept(options->midsockfd,
					(struct sockaddr *) &cli_addr, &clilen)) > 0) {
				// log protocol validation indicating client connection
				procstatusenum = PROCESS_STARTED;
				proctypeenum = CONNECT_TYPE;
				protolog_procstatus(options->child0, thistestId, proctypeenum,
						procstatusenum);
				break;
			}

			if ((midsfd == -1) && (errno == EINTR)) // socket interrupted, wait some more
				goto midfd;

			sprintf(
					tmpstr,
					"-------     middlebox connection setup returned because (%d)",
					errno);
			if (get_debuglvl() > 1)
				perror(tmpstr);
			if (midsfd < 0)
				return -errno;
			if (j == 4)
				return RETRY_EXCEEDED_WAITING_DATA;
		}

		// get meta test details copied into results
		memcpy(&meta.c_addr, &cli_addr, clilen);
		/* meta.c_addr = cli_addr; */
		meta.family = ((struct sockaddr *) &cli_addr)->sa_family;

		buff[0] = '\0';
		// get web100 connection data
		if ((conn = web100_connection_from_socket(agent, midsfd)) == NULL) {
			log_println(
					0,
					"!!!!!!!!!!!  test_mid() failed to get web100 connection data, rc=%d",
					errno);
			/* exit(-1); */
			return -3;
		}

		// Perform S->C throughput test. Obtained results in "buff"
		web100_middlebox(midsfd, agent, conn, buff);

		// Transmit results in the form of a TEST_MSG message
		send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));

		// Expect client to send throughput as calculated at its end

		msgLen = sizeof(buff);
		if (recv_msg(ctlsockfd, &msgType, buff, &msgLen)) { // message reception error
			log_println(0, "Protocol error!");
			sprintf(
					buff,
					"Server (Middlebox test): Invalid CWND limited throughput received");
			send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
			return 1;
		}
		if (check_msg_type("Middlebox test", TEST_MSG, msgType, buff, msgLen)) { // only TEST_MSG type valid
			sprintf(
					buff,
					"Server (Middlebox test): Invalid CWND limited throughput received");
			send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
			return 2;
		}
		if (msgLen <= 0) { // received message's length has to be a valid one
			log_println(0, "Improper message");
			sprintf(
					buff,
					"Server (Middlebox test): Invalid CWND limited throughput received");
			send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
			return 3;
		}

		// message payload from client == midbox S->c throughput
		buff[msgLen] = 0;
		*s2c_throughput_mid = atof(buff);
		log_println(4, "CWND limited throughput = %0.0f kbps (%s)",
				*s2c_throughput_mid, buff);

		// finalize the midbox test ; disabling socket used for throughput test and closing out both sockets
		shutdown(midsfd, SHUT_WR);
		close(midsfd);
		close(options->midsockfd);
		send_msg(ctlsockfd, TEST_FINALIZE, "", 0);
		log_println(1, " <--------- %d ----------->", options->child0);

		printf(" <--- %d - Middlebox test --->", options->child0);

		// log end of test into protocol doc, just to delimit.
		teststatusnow = TEST_ENDED;
		//protolog_status(1, options->child0, thistestId, teststatusnow);
		protolog_status(options->child0, thistestId, teststatusnow);

		setCurrentTest(TEST_NONE);
		/* I2AddrFree(midsrv_addr); */
	}
	/* I2AddrFree(midsrv_addr); */
	return 0;
}

/**
 * Perform the C2S Throughput test. This test intends to measure throughput
 * from the client to the server by performing a 10 seconds memory-to-memory data transfer.
 *
 * Protocol messages are exchanged between the Client and the Server using the same
 * connection and message format as the NDTP-Control protocol.Throughput packets are
 * sent on the new connection and do not follow the NDTP-Control protocol message format.
 *
 * When the Client stops streaming the test data (or the server test routine times out),
 * the Server sends the Client its calculated throughput value.
 *
 * @param ctlsockfd Client control socket descriptor
 * @param agent Web100 agent used to track the connection
 * @param testOptions Test options
 * @param conn_options connection options
 * @param c2sspd In-out parameter to store C2S throughput value
 * @param set_buff enable setting TCP send/recv buffer size to be used (seems unused in file)
 * @param window value of TCP send/rcv buffer size intended to be used.
 * @param autotune autotuning option. Deprecated.
 * @param device string devine name inout parameter
 * @param options Test Option variables
 * @param record_reverse integer indicating whether receiver-side statistics have to be logged
 * @param count_vars count of web100 variables
 * @param spds[] [] speed check array
 * @param spd_index  index used for speed check array
 * @param conn_options Connection options
 * @return 0 - success,
 *          >0 - error code
 *          Error codes:
 *          -1 - Listener socket creation failed
 *          -100 - timeout while waiting for client to connect to serverÕs ephemeral port
 * 			-errno - Other specific socket error numbers
 *			-101 - Retries exceeded while waiting for client to connect
 *			-102 - Retries exceeded while waiting for data from connected client
 *
 */

int test_c2s(int ctlsockfd, web100_agent* agent, TestOptions* testOptions,
		int conn_options, double* c2sspd, int set_buff, int window,
		int autotune, char* device, Options* options, int record_reverse,
		int count_vars, char spds[4][256], int* spd_index) {
	int recvsfd; // receiver socket file descriptor
	pid_t c2s_childpid = 0; // child process pids
	int msgretvalue, tmpbytecount; // used during the "read"/"write" process
	int i, j; // used as loop iterators

	struct sockaddr_storage cli_addr;

	socklen_t clilen;
	char tmpstr[256]; // string array used for all sorts of temp storage purposes
	double tmptime; // time indicator
	double bytes_read = 0; // number of bytes read during the throughput tests
	struct timeval sel_tv; // time
	fd_set rfd; // receive file descriptor
	char buff[BUFFSIZE + 1]; // message "payload" buffer
PortPair 	pair; // socket ports
	I2Addr c2ssrv_addr = NULL; // c2s test's server address
	//I2Addr src_addr=NULL;			// c2s test source address
	char listenc2sport[10]; // listening port
	pthread_t workerThreadId;

	// web_100 related variables
	web100_group* group;
	web100_connection* conn;

	// snap related variables
	SnapArgs snapArgs;
	snapArgs.snap = NULL;
	snapArgs.log = NULL;
	snapArgs.delay = options->snapDelay;
	wait_sig = 0;

	// Test ID and status descriptors
	enum TEST_ID testids = C2S;
	enum TEST_STATUS_INT teststatuses = TEST_NOT_STARTED;
	enum PROCESS_STATUS_INT procstatusenum = UNKNOWN;
	enum PROCESS_TYPE_INT proctypeenum = CONNECT_TYPE;
	char namesuffix[256] = "c2s_snaplog";

	if (testOptions->c2sopt) {
		setCurrentTest(TEST_C2S);
		log_println(1, " <-- %d - C2S throughput test -->",
				testOptions->child0);
		strlcpy(listenc2sport, PORT2, sizeof(listenc2sport));

		//log protocol validation logs
		teststatuses = TEST_STARTED;
		protolog_status(testOptions->child0, testids, teststatuses);

		// Determine port to be used. Compute based on options set earlier
		// by reading from config file, or use default port2 (3002).
		if (testOptions->c2ssockport) {
			sprintf(listenc2sport, "%d", testOptions->c2ssockport);
		} else if (testOptions->mainport) {
			sprintf(listenc2sport, "%d", testOptions->mainport + 1);
		}

		if (testOptions->multiple) {
			strlcpy(listenc2sport, "0", sizeof(listenc2sport));
		}

		// attempt to bind to a new port and obtain address structure with details of listening port
		while (c2ssrv_addr == NULL) {
			c2ssrv_addr =
					CreateListenSocket(
							NULL,
							(testOptions->multiple ?
									mrange_next(listenc2sport) : listenc2sport), conn_options, 0)
							;
			if (strcmp(listenc2sport, "0") == 0) {
				log_println(0, "WARNING: ephemeral port number was bound");
				break;
			}
			if (testOptions->multiple == 0) {
				break;
			}
		}
		if (c2ssrv_addr == NULL) {
			log_println(
					0,
					"Server (C2S throughput test): CreateListenSocket failed: %s",
					strerror(errno));
			sprintf(
					buff,
					"Server (C2S throughput test): CreateListenSocket failed: %s",
					strerror(errno));
			send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
			return -1;
		}

		// get socket FD and the ephemeral port number that client will connect to run tests
		testOptions->c2ssockfd = I2AddrFD(c2ssrv_addr);
		testOptions->c2ssockport = I2AddrPort(c2ssrv_addr);
		log_println(1, "  -- port: %d", testOptions->c2ssockport);
		pair.port1 = testOptions->c2ssockport;
		pair.port2 = -1;

		log_println(
				1,
				"listening for Inet connection on testOptions->c2ssockfd, fd=%d",
				testOptions->c2ssockfd);

		log_println(
				1,
				"Sending 'GO' signal, to tell client %d to head for the next test",
				testOptions->child0);
		sprintf(buff, "%d", testOptions->c2ssockport);

		// send TEST_PREPARE message with ephemeral port detail, indicating start of tests
		if ((msgretvalue = send_msg(ctlsockfd, TEST_PREPARE, buff, strlen(buff)))
				< 0)
			return msgretvalue;

		// Wait on listening socket and read data once ready.
		// Retry 5 times,  waiting for activity on the socket
		clilen = sizeof(cli_addr);
		log_println(6, "child %d - sent c2s prepare to client",
				testOptions->child0);
		FD_ZERO(&rfd);
		FD_SET(testOptions->c2ssockfd, &rfd);
		sel_tv.tv_sec = 5;
		sel_tv.tv_usec = 0;

		for (j = 0; j < RETRY_COUNT; j++) {
			msgretvalue = select((testOptions->c2ssockfd) + 1, &rfd, NULL, NULL,
					&sel_tv);
			if ((msgretvalue == -1) && (errno == EINTR)) // socket interrupted. continue waiting for activity on socket
				continue;
			if (msgretvalue == 0) // timeout
				return -SOCKET_CONNECT_TIMEOUT;
			if (msgretvalue < 0) // other socket errors. exit
				return -errno;
			if (j == (RETRY_COUNT - 1)) // retry exceeded. exit
				return -RETRY_EXCEEDED_WAITING_CONNECT;
			recfd:

			// If a valid connection request is received, client has connected. Proceed.
			// Note the new socket fd - recvsfd- used in the throughput test
			recvsfd = accept(testOptions->c2ssockfd,
					(struct sockaddr *) &cli_addr, &clilen);
			if (recvsfd > 0) {
				log_println(6, "accept() for %d completed",
						testOptions->child0);

				// log protocol validation indicating client accept
				procstatusenum = PROCESS_STARTED;
				proctypeenum = CONNECT_TYPE;
				protolog_procstatus(testOptions->child0, testids, proctypeenum,
						procstatusenum);
				break;
			}
			if ((recvsfd == -1) && (errno == EINTR)) { // socket interrupted, wait some more
				log_println(
						6,
						"Child %d interrupted while waiting for accept() to complete",
						testOptions->child0);
				goto recfd;
			}
			log_println(
					6,
					"-------     C2S connection setup for %d returned because (%d)",
					testOptions->child0, errno);
			if (recvsfd < 0) { // other socket errors, 	quit
				return -errno;
			}
			if (j == (RETRY_COUNT - 1)) { // retry exceeded, quit
				log_println(
						6,
						"c2s child %d, uable to open connection, return from test",
						testOptions->child0);
				return RETRY_EXCEEDED_WAITING_DATA;
			}
		}

		// Get address associated with the throughput test. Used for packet tracing
		log_println(6, "child %d - c2s ready for test with fd=%d",
				testOptions->child0, recvsfd);

		// commenting out below to move to init_pkttrace function
		// src_addr = I2AddrByLocalSockFD(get_errhandle(), recvsfd, 0);

		// Get web100 connection. Used to collect web100 variable statistics
		conn = web100_connection_from_socket(agent, recvsfd);

		// set up packet tracing. Collected data is used for bottleneck link calculations
		if (getuid() == 0) {

			pipe(mon_pipe1);
			log_println(0, "C2S test calling pkt_trace_start() with pd=%d",
					clilen);
			start_packet_trace(recvsfd, testOptions->c2ssockfd, &c2s_childpid,
					mon_pipe1, (struct sockaddr *) &cli_addr, clilen, device,
					&pair, "c2s", options->compress, meta.c2s_ndttrace);
			log_println(3, "--tracefile after packet_trace %s",
					meta.c2s_ndttrace);
		}

		log_println(5, "C2S test Parent thinks pipe() returned fd0=%d, fd1=%d",
				mon_pipe1[0], mon_pipe1[1]);

		// experimental code, delete when finished
		setCwndlimit(conn, group, agent, options);

		// Create C->S snaplog directories, and perform some initialization based on options

		create_client_logdir((struct sockaddr *) &cli_addr, clilen,
				options->c2s_logname, sizeof(options->c2s_logname), namesuffix,
				sizeof(namesuffix));

		sleep(2);

		// send empty TEST_START indicating start of the test
		send_msg(ctlsockfd, TEST_START, "", 0);
		/* alarm(30); *//* reset alarm() again, this 10 sec test should finish before this signal
		 * is generated.  */

		// If snaplog recording is enabled, update meta file to indicate the same
		//  and proceed to get snapshot and log it. For the C2S test, the
		// snapshot is not needed any further, and hence it is valid to move the
		// obtaining of a snapshot based on options->snaplog!
		if (options->snaplog) {
			memcpy(meta.c2s_snaplog, namesuffix, strlen(namesuffix));
			// somewhat a hack - meta file stores names without the full directory
			// but fopen needs full path
			start_snap_worker(&snapArgs, agent, options->snaplog, &workerLoop,
					&workerThreadId, meta.c2s_snaplog, options->c2s_logname,
					conn, group);
		}

		// Wait on listening socket and read data once ready.
		tmptime = secs();
		sel_tv.tv_sec = 11; // time out after 11 seconds
		sel_tv.tv_usec = 0;
		FD_ZERO(&rfd);
		FD_SET(recvsfd, &rfd);
		for (;;) {
			msgretvalue = select(recvsfd + 1, &rfd, NULL, NULL, &sel_tv);
			if ((msgretvalue == -1) && (errno == EINTR)) {
				// socket interrupted. Continue waiting for activity on socket
				continue;
			}
			if (msgretvalue > 0) { // read from socket
				tmpbytecount = read(recvsfd, buff, sizeof(buff));
				if ((tmpbytecount == -1) && (errno == EINTR)) // read interrupted,continue waiting
					continue;
				if (tmpbytecount == 0) // all data has been read
					break;
				bytes_read += tmpbytecount; // data byte count has to be increased
				continue;
			}
			break;
		}

		tmptime = secs() - tmptime;
		//  throughput in kilo bits per sec = (transmitted_byte_count * 8) / (time_duration)*(1000)
		*c2sspd = (8.e-3 * bytes_read) / tmptime;

		// c->s throuput value calculated and assigned ! Release resources, conclude snap writing.
		stop_snap_worker(&workerThreadId, options->snaplog, &snapArgs);

		// send the server calculated value of C->S throughput as result to client
		sprintf(buff, "%6.0f kbps outbound for child %d", *c2sspd,
				testOptions->child0);
		log_println(1, "%s", buff);
		sprintf(buff, "%0.0f", *c2sspd);
		send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));

		// get receiver side Web100 stats and write them to the log file. close sockets
		if (record_reverse == 1)
			web100_get_data_recv(recvsfd, agent, conn, count_vars);

		close(recvsfd);
		close(testOptions->c2ssockfd);

		// Next, send speed-chk a flag to retrieve the data it collected.
		// Skip this step if speed-chk isn't running.

		if (getuid() == 0) {
			log_println(1, "Signal USR1(%d) sent to child [%d]", SIGUSR1,
					c2s_childpid);
			testOptions->child1 = c2s_childpid;
			kill(c2s_childpid, SIGUSR1);
			FD_ZERO(&rfd);
			FD_SET(mon_pipe1[0], &rfd);
			sel_tv.tv_sec = 1;
			sel_tv.tv_usec = 100000;
			i = 0;

			for (;;) {
				msgretvalue = select(mon_pipe1[0] + 1, &rfd, NULL, NULL,
						&sel_tv);
				if ((msgretvalue == -1) && (errno == EINTR))
					continue;
				if (((msgretvalue == -1) && (errno != EINTR))
						|| (msgretvalue == 0)) {
					log_println(
							4,
							"Failed to read pkt-pair data from C2S flow, retcode=%d, reason=%d",
							msgretvalue, errno);
					sprintf(
							spds[(*spd_index)++],
							" -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0 -1");
					sprintf(
							spds[(*spd_index)++],
							" -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0 -1");
					break;
				}
				/* There is something to read, so get it from the pktpair child.  If an interrupt occurs, 
				 * just skip the read and go on
				 * RAC 2/8/10
				 */
				if (msgretvalue > 0) {
					if ((msgretvalue = read(mon_pipe1[0], spds[*spd_index], 128))
							< 0)
						sprintf(
								spds[*spd_index],
								" -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0 -1");
					log_println(1, "%d bytes read '%s' from C2S monitor pipe",
							msgretvalue, spds[*spd_index]);
					(*spd_index)++;
					if (i++ == 1)
						break;
					sel_tv.tv_sec = 1;
					sel_tv.tv_usec = 100000;
					continue;
				}

			}
		}

		// An empty TEST_FINALIZE message is sent to conclude the test
		send_msg(ctlsockfd, TEST_FINALIZE, "", 0);

		//  Close opened resources for packet capture
		if (getuid() == 0) {
			stop_packet_trace(mon_pipe1);
		}

		// log end of C->S test
		log_println(1, " <----------- %d -------------->", testOptions->child0);
		//protocol logs
		teststatuses = TEST_ENDED;
		//protolog_status(0,testOptions->child0, testids, teststatuses);
		protolog_status(testOptions->child0, testids, teststatuses);

		//set current test status and free address
		setCurrentTest(TEST_NONE);

	}

	return 0;
}

/**
 * Perform the S2C Throughput test. This throughput test tests the achievable
 * network bandwidth from the Server to the Client by performing a 10 seconds
 * memory-to-memory data transfer.
 *
 * The Server also collects web100 data variables, that are sent to the Client
 * at the end of the test session.
 *
 * Protocol messages exchanged between the Client and Server
 * are sent using the same connection and message format as the NDTP-Control protocol.
 * The throughput packets are sent on the new connection, though, and do not
 * follow the NDTP-Control protocol message format.
 *
 * @param ctlsockfd - the client control socket descriptor
 * @param agent - the Web100 agent used to track the connection
 * @param testOptions - the test options
 * @param conn_options - the connection options
 * @param testOptions Test options
 * @param s2cspd In-out parameter to store C2S throughput value
 * @param set_buff enable setting TCP send/recv buffer size to be used (seems unused in file)
 * @param window value of TCP send/rcv buffer size intended to be used.
 * @param autotune autotuning option. Deprecated.
 * @param device string devine name inout parameter
 * @param options Test Option variables
 * @param spds[][] speed check array
 * @param spd_index  index used for speed check array
 * @param count_vars count of web100 variables
 * @param peaks Cwnd peaks structure pointer
 *
 * @return 0 - success,
 *         >0 - error code.
 *     Error codes:
 * 			 	-1	   - Message reception errors/inconsistencies in clientÕs final message, or Listener socket creation failed or cannot write message header information while attempting to send
 *        		 TEST_PREPARE message
 *				-2 	   - Cannot write message data while attempting to send
 *           		 TEST_PREPARE message, or Unexpected message type received
 *           	-3 	   -  Received message is invalid
 *          	-100   - timeout while waiting for client to connect to serverÕs ephemeral port
 *				-101   - Retries exceeded while waiting for client to connect
 *				-102   - Retries exceeded while waiting for data from connected client
 *  			-errno - Other specific socket error numbers
 */

int test_s2c(int ctlsockfd, web100_agent* agent, TestOptions* testOptions,
		int conn_options, double* s2cspd, int set_buff, int window,
		int autotune, char* device, Options* options, char spds[4][256],
		int* spd_index, int count_vars, CwndPeaks* peaks) {

	int ret; // ctrl protocol read/write return status
	int j, k, n;
	int xmitsfd; // transmit (i.e server) socket fd
	pid_t s2c_childpid = 0; // s2c_childpid

	char tmpstr[256]; // string array used for temp storage of many char*
	struct sockaddr_storage cli_addr;

	socklen_t clilen;
	double bytes_written; // bytes written in the throughput test
	double tx_duration; // total time for which data was txed
	double tmptime; // temporary time store
	double x2cspd; // s->c test throuhput
	struct timeval sel_tv; // time
	fd_set rfd; // receive file descriptor
	char buff[BUFFSIZE + 1]; // message payload buffer
int 	bufctrlattempts = 0; // number of buffer control attempts
	int i; // temporary var used for iterators etc
	PortPair pair; // socket ports
	I2Addr s2csrv_addr = NULL;
	char listens2cport[10];
	int msgType;
	int msgLen;
	int sndqueue;
	struct sigaction new, old;

	/* experimental code to capture and log multiple copies of the
	 * web100 variables using the web100_snap() & log() functions.
	 */
	web100_snapshot* tsnap = NULL;
	web100_snapshot* rsnap = NULL;
	web100_group* group;
	web100_group* tgroup;
	web100_group* rgroup;
	web100_connection* conn;
	web100_var* var;
	pthread_t workerThreadId;
	int nextseqtosend = 0, lastunackedseq = 0;
	int drainingqueuecount = 0, bufctlrnewdata = 0; // sent data attempt queue, Draining Queue.

	// variables used for protocol validation logs
	enum TEST_STATUS_INT teststatuses = TEST_NOT_STARTED;
	enum TEST_ID testids = S2C;
	enum PROCESS_STATUS_INT procstatusenum = UNKNOWN;
	enum PROCESS_TYPE_INT proctypeenum = CONNECT_TYPE;
	char snaplogsuffix[256] = "s2c_snaplog";

	SnapArgs snapArgs;
	snapArgs.snap = NULL;
	snapArgs.log = NULL;
	snapArgs.delay = options->snapDelay;
	wait_sig = 0;

	// Determine port to be used. Compute based on options set earlier
	// by reading from config file, or use default port2 (3003)
	if (testOptions->s2copt) {
		setCurrentTest(TEST_S2C);
		log_println(1, " <-- %d - S2C throughput test -->",
				testOptions->child0);

		//protocol logs
		teststatuses = TEST_STARTED;
		protolog_status(testOptions->child0, testids, teststatuses);

		strlcpy(listens2cport, PORT4, sizeof(listens2cport));

		if (testOptions->s2csockport) {
			sprintf(listens2cport, "%d", testOptions->s2csockport);
		} else if (testOptions->mainport) {
			sprintf(listens2cport, "%d", testOptions->mainport + 2);
		}

		if (testOptions->multiple) {
			strlcpy(listens2cport, "0", sizeof(listens2cport));
		}

		// attempt to bind to a new port and obtain address structure with details of listening port
		while (s2csrv_addr == NULL) {
			s2csrv_addr =
					CreateListenSocket(
							NULL,
							(testOptions->multiple ?
									mrange_next(listens2cport) : listens2cport), conn_options, 0)
							;
			if (s2csrv_addr == NULL) {
				/*
				 log_println(1, " Calling KillHung() because s2csrv_address failed to bind");
				 if (KillHung() == 0)
				 continue;
				 */
			}
			if (strcmp(listens2cport, "0") == 0) {
				log_println(0, "WARNING: ephemeral port number was bound");
				break;
			}
			if (testOptions->multiple == 0) {
				break;
			}
		}
		if (s2csrv_addr == NULL) {
			log_println(
					0,
					"Server (S2C throughput test): CreateListenSocket failed: %s",
					strerror(errno));
			sprintf(
					buff,
					"Server (S2C throughput test): CreateListenSocket failed: %s",
					strerror(errno));
			send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
			return -1;
		}

		// get socket FD and the ephemeral port number that client will connect to run tests
		testOptions->s2csockfd = I2AddrFD(s2csrv_addr);
		testOptions->s2csockport = I2AddrPort(s2csrv_addr);
		log_println(1, "  -- s2c %d port: %d", testOptions->child0,
				testOptions->s2csockport);
		pair.port1 = -1;
		pair.port2 = testOptions->s2csockport;

		// Data received from speed-chk. Send TEST_PREPARE "GO" signal with port number
		sprintf(buff, "%d", testOptions->s2csockport);
		j = send_msg(ctlsockfd, TEST_PREPARE, buff, strlen(buff));
		if (j == -1) {
			log_println(6, "S2C %d Error!, Test start message not sent!",
					testOptions->child0);
			return j;
		}
		if (j == -2) { // could not write message data
			log_println(6, "S2C %d Error!, server port [%s] not sent!",
					testOptions->child0, buff);
			return j;
		}

		// ok, await for connect on 3rd port
		// This is the second throughput test, with data streaming from
		// the server back to the client.  Again stream data for 10 seconds.

		log_println(1, "%d waiting for data on testOptions->s2csockfd",
				testOptions->child0);

		clilen = sizeof(cli_addr);
		FD_ZERO(&rfd);
		FD_SET(testOptions->c2ssockfd, &rfd);
		sel_tv.tv_sec = 5; // wait for 5 secs
		sel_tv.tv_usec = 0;
		for (j = 0; j < RETRY_COUNT; j++) {
			ret = select((testOptions->s2csockfd) + 1, &rfd, NULL, NULL,
					&sel_tv);
			if ((ret == -1) && (errno == EINTR))
				continue;
			if (ret == 0)
				return SOCKET_CONNECT_TIMEOUT; // timeout
			if (ret < 0)
				return -errno; // other socket errors. exit
			if (j == 4)
				return RETRY_EXCEEDED_WAITING_CONNECT; // retry exceeded. exit

			// If a valid connection request is received, client has connected. Proceed.
			// Note the new socket fd - xmitfd - used in the throughput test
			ximfd: xmitsfd = accept(testOptions->s2csockfd,
					(struct sockaddr *) &cli_addr, &clilen);
			if (xmitsfd > 0) {
				log_println(6, "accept() for %d completed",
						testOptions->child0);
				procstatusenum = PROCESS_STARTED;
				proctypeenum = CONNECT_TYPE;
				protolog_procstatus(testOptions->child0, testids, proctypeenum,
						procstatusenum);
				break;
			}
			if ((xmitsfd == -1) && (errno == EINTR)) { // socket interrupted, wait some more
				log_println(
						6,
						"Child %d interrupted while waiting for accept() to complete",
						testOptions->child0);
				goto ximfd;
			}
			log_println(
					6,
					"-------     S2C connection setup for %d returned because (%d)",
					testOptions->child0, errno);
			if (xmitsfd < 0) // other socket errors, quit
				return -errno;
			if (++j == 4) { // retry exceeded, quit
				log_println(
						6,
						"s2c child %d, unable to open connection, return from test",
						testOptions->child0);
				return -102;
			}
		}
		//src_addr = I2AddrByLocalSockFD(get_errhandle(), xmitsfd, 0);
		conn = web100_connection_from_socket(agent, xmitsfd);

		// set up packet capture. The data collected is used for bottleneck link calculations
		if (xmitsfd > 0) {
			log_println(6, "S2C child %d, ready to fork()",
					testOptions->child0);
			if (getuid() == 0) {
				pipe(mon_pipe2);
				start_packet_trace(xmitsfd, testOptions->s2csockfd,
						&s2c_childpid, mon_pipe2, (struct sockaddr *) &cli_addr,
						clilen, device, &pair, "s2c", options->compress,
						meta.s2c_ndttrace);

			}

			/* experimental code, delete when finished */
			setCwndlimit(conn, group, agent, options);
			/* End of test code */

			// create directory to write web100 snaplog trace
			create_client_logdir((struct sockaddr *) &cli_addr, clilen,
					options->s2c_logname, sizeof(options->s2c_logname),
					snaplogsuffix, sizeof(snaplogsuffix));

			/* Kludge way of nuking Linux route cache.  This should be done
			 * using the sysctl interface.
			 */
			if (getuid() == 0) {
				// system("/sbin/sysctl -w net.ipv4.route.flush=1");
				system("echo 1 > /proc/sys/net/ipv4/route/flush");
			}
			rgroup = web100_group_find(agent, "read");
			rsnap = web100_snapshot_alloc(rgroup, conn);
			tgroup = web100_group_find(agent, "tune");
			tsnap = web100_snapshot_alloc(tgroup, conn);

			// fill send buffer with random printable data for throughput test
			bytes_written = 0;
			k = 0;
			for (j = 0; j <= BUFFSIZE; j++) {
				while (!isprint(k & 0x7f))
					k++;
				buff[j] = (k++ & 0x7f);
			}

			// Send message to client indicating TEST_START
			if (send_msg(ctlsockfd, TEST_START, "", 0) < 0)
				log_println(6,
						"S2C test - Test-start message failed for pid=%d",
						s2c_childpid);
			// ignore the alarm signal
			memset(&new, 0, sizeof(new));
			new.sa_handler = catch_s2c_alrm;
			sigaction(SIGALRM, &new, &old);

			// capture current values (i.e take snap shot) of web_100 variables
			// Write snap logs if option is enabled. update meta log to point to this snaplog

			// If snaplog option is enabled, save snaplog details in meta file
			if (options->snaplog) {
				memcpy(meta.s2c_snaplog, snaplogsuffix, strlen(snaplogsuffix));
			}
			// get web100 snapshot and also log it based on options
			start_snap_worker(&snapArgs, agent, options->snaplog, &workerLoop,
					&workerThreadId, meta.s2c_snaplog, options->s2c_logname,
					conn, group);

			/* alarm(20); */
			tmptime = secs(); // current time
			tx_duration = tmptime + 10.0; // set timeout to 10 s in future

			log_println(6, "S2C child %d beginning test", testOptions->child0);

			while (secs() < tx_duration) {
				bufctrlattempts++; // Increment total attempts at sending-> buffer control
				if (options->avoidSndBlockUp) { // Do not block send buffers
					pthread_mutex_lock(&mainmutex);

					// get details of next sequence # to be sent and fetch value from snap file
					web100_agent_find_var_and_group(agent, "SndNxt", &group,
							&var);
					web100_snap_read(var, snapArgs.snap, tmpstr);
					nextseqtosend = atoi(
							web100_value_to_text(web100_get_var_type(var),
									tmpstr));
					// get oldest un-acked sequence number
					web100_agent_find_var_and_group(agent, "SndUna", &group,
							&var);
					web100_snap_read(var, snapArgs.snap, tmpstr);
					lastunackedseq = atoi(
							web100_value_to_text(web100_get_var_type(var),
									tmpstr));
					pthread_mutex_unlock(&mainmutex);

					// Temporarily stop sending data if you sense that the buffer is overwhelmed
					// This is calculated by checking if (8192 * 4) <
					//        ((Next Sequence Number To Be Sent) - (Oldest Unacknowledged Sequence Number) - 1)
					// Increment draining queue value
					if (is_buffer_clogged(nextseqtosend, lastunackedseq)) {
						drainingqueuecount++;
						continue;
					}
				}

				// attempt to write random data into the client socket
				n = write(xmitsfd, buff, RECLTH);
				if ((n == -1) && (errno == EINTR)) // socket interrupted, continue attempting to write
					continue;
				if (n < 0)
					break; // all data written. Exit
				bytes_written += n;

				if (options->avoidSndBlockUp) {
					bufctlrnewdata++; // increment "sent data" queue
				}
			} // Completed end of trying to transmit data for the goodput test
			/* alarm(10); */
			sigaction(SIGALRM, &old, NULL);
			sndqueue = sndq_len(xmitsfd);

			// finalize the midbox test ; disabling socket used for throughput test
			log_println(6, "S2C child %d finished test", testOptions->child0);
			shutdown(xmitsfd, SHUT_WR); /* end of write's */

			// get actual time duration during which data was transmitted
			tx_duration = secs() - tmptime;

			// Throughput in kbps = (no of bits sent * 8) / (1000 * time data was sent)
			x2cspd = (8.e-3 * bytes_written) / tx_duration;

			// Release semaphore, and close snaplog file.  finalize other data
			stop_snap_worker(&workerThreadId, options->snaplog, &snapArgs);

			// send the x2cspd to the client
			memset(buff, 0, sizeof(buff));

			// Send throughput, unsent byte count, total sent byte count to client
			sprintf(buff, "%0.0f %d %0.0f", x2cspd, sndqueue, bytes_written);
			if (send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff)) < 0)
				log_println(6,
						"S2C test - failed to send test message to pid=%d",
						s2c_childpid);

			web100_snap(rsnap);
			web100_snap(tsnap);

			log_println(1, "sent %d bytes to client in %0.2f seconds",
					(int) bytes_written, tx_duration);
			log_println(
					1,
					"Buffer control counters Total = %d, new data = %d, Draining Queue = %d",
					bufctrlattempts, bufctlrnewdata, drainingqueuecount);

			/* Next send speed-chk a flag to retrieve the data it collected.
			 * Skip this step if speed-chk isn't running.
			 */

			if (getuid() == 0) {
				log_println(1, "Signal USR2(%d) sent to child [%d]", SIGUSR2,
						s2c_childpid);
				testOptions->child2 = s2c_childpid;
				kill(s2c_childpid, SIGUSR2);
				FD_ZERO(&rfd);
				FD_SET(mon_pipe2[0], &rfd);
				sel_tv.tv_sec = 1;
				sel_tv.tv_usec = 100000;
				i = 0;

				for (;;) {
					ret = select(mon_pipe2[0] + 1, &rfd, NULL, NULL, &sel_tv);
					if ((ret == -1) && (errno == EINTR)) {
						log_println(
								6,
								"Interrupt received while waiting for s2c select90 to finish, continuing");
						continue;
					}
					if (((ret == -1) && (errno != EINTR)) || (ret == 0)) {
						log_println(
								4,
								"Failed to read pkt-pair data from S2C flow, retcode=%d, reason=%d, EINTR=%d",
								ret, errno, EINTR);
						sprintf(
								spds[(*spd_index)++],
								" -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0 -1");
						sprintf(
								spds[(*spd_index)++],
								" -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0 -1");
						break;
					}
					/* There is something to read, so get it from the pktpair child.  If an interrupt occurs, 
					 * just skip the read and go on
					 * RAC 2/8/10
					 */
					if (ret > 0) {
						if ((ret = read(mon_pipe2[0], spds[*spd_index], 128))
								< 0)
							sprintf(
									spds[*spd_index],
									" -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0.0 0 0 0 0 0 -1");
						log_println(1,
								"%d bytes read '%s' from S2C monitor pipe", ret,
								spds[*spd_index]);
						(*spd_index)++;
						if (i++ == 1)
							break;
						sel_tv.tv_sec = 1;
						sel_tv.tv_usec = 100000;
						continue;
					}
				}
			}

			log_println(1, "%6.0f kbps inbound pid-%d", x2cspd, s2c_childpid);
		}

		/* alarm(30); *//* reset alarm() again, this 10 sec test should finish before this signal
		 * is generated.  */
		// Get web100 variables from snapshot taken earlier and send to client
		log_println(6, "S2C-Send web100 data vars to client pid=%d",
				s2c_childpid);
		ret = web100_get_data(tsnap, ctlsockfd, agent, count_vars); //send web100 data to client
		web100_snapshot_free(tsnap);
		ret = web100_get_data(rsnap, ctlsockfd, agent, count_vars); //send tuning-related web100 data collected to client
		web100_snapshot_free(rsnap);

		// If sending web100 variables above failed, indicate to client
		if (ret < 0) {
			log_println(6, "S2C - No web100 data received for pid=%d",
					s2c_childpid);
			sprintf(buff, "No Data Collected: 000000");
			send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
		}

		// Wait for message from client. Client sends its calculated throughput value
		msgLen = sizeof(buff);
		if (recv_msg(ctlsockfd, &msgType, buff, &msgLen)) {
			log_println(0, "Protocol error!");
			sprintf(
					buff,
					"Server (S2C throughput test): Invalid S2C throughput received");
			send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
			return -1;
		}
		if (check_msg_type("S2C throughput test", TEST_MSG, msgType, buff,
				msgLen)) {
			sprintf(
					buff,
					"Server (S2C throughput test): Invalid S2C throughput received");
			send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
			return -2;
		}
		if (msgLen <= 0) {
			log_println(0, "Improper message");
			sprintf(
					buff,
					"Server (S2C throughput test): Invalid S2C throughput received");
			send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
			return -3;
		}
		buff[msgLen] = 0;
		*s2cspd = atoi(buff); // save Throughput value as seen by client

		// Final activities of ending tests. Close sockets, file descriptors,
		//    send finalise message to client
		close(xmitsfd);
		if (send_msg(ctlsockfd, TEST_FINALIZE, "", 0) < 0)
			log_println(6,
					"S2C test - failed to send finalize message to pid=%d",
					s2c_childpid);

		if (getuid() == 0) {
			stop_packet_trace(mon_pipe2);
		}

		// log end of test (generic and protocol logs)
		log_println(1, " <------------ %d ------------->", testOptions->child0);
		//log protocol validation logs
		teststatuses = TEST_ENDED;
		protolog_status(testOptions->child0, testids, teststatuses);

		setCurrentTest(TEST_NONE);

	}
	return 0;
}

