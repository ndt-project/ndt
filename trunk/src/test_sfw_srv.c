/**
 * This file contains the functions needed to handle simple firewall
 * test (server part).
 *
 * Jakub S³awiñski 2006-07-15
 * jeremian@poczta.fm
 */

#include <assert.h>
#include <pthread.h>

#include "test_sfw.h"
#include "logging.h"
#include "protocol.h"
#include "network.h"
#include "utils.h"
#include "testoptions.h"
#include "runningtest.h"
#include "strlutils.h"

static pthread_mutex_t mainmutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t maincond = PTHREAD_COND_INITIALIZER;
static I2Addr sfwcli_addr = NULL;
static int testTime = 3;
static int toWait = 1;
static pthread_t threadId = -1;

/**
 * Prints the appropriate message when the SIGALRM is caught.
 * @param signo Signal number (should be SIGALRM)
 */

void catch_alrm(int signo) {
	if (signo == SIGALRM) {
		log_println(1, "SFW - SIGALRM was caught");
		if (threadId != -1) { /* we forward the signal to the osfw thread */
			pthread_kill(threadId, SIGALRM);
			threadId = -1;
		}
		return;
	}
	log_println(0, "Unknown (%d) signal was caught", signo);
}

/**
 * Performs the server part of the opposite Simple
 *              firewall test in a separate thread.
 * In other words, sends the S->C TEST_MSG with message body
 * "Simple firewall test"
 * @param vptr void pointer
 */

void*
test_osfw_srv(void* vptr) {
	int sfwsock;
	struct sigaction new, old;

	// ignore the alarm signal
	memset(&new, 0, sizeof(new));
	new.sa_handler = catch_alrm;
	sigaction(SIGALRM, &new, &old);
	alarm(testTime);

	// connect to client and send TEST_MSG message containing a pre-defined string
	if (CreateConnectSocket(&sfwsock, NULL, sfwcli_addr, 0, 0) == 0) {
		send_msg(sfwsock, TEST_MSG, "Simple firewall test", 20);
	}

	alarm(0);
	sigaction(SIGALRM, &old, NULL);

	// signal sleeping threads to wake up
	pthread_mutex_lock(&mainmutex);
	toWait = 0;
	pthread_cond_broadcast(&maincond);
	pthread_mutex_unlock(&mainmutex);

	return NULL;
}

/**
 * Wait for the every thread to conclude and finalize
 *              the SFW test.
 * @param ctlsockfd Client control socket descriptor
 */

void finalize_sfw(int ctlsockfd) {
	enum TEST_ID thistestId = SFW;
	enum TEST_STATUS_INT teststatusnow = NONE;
	// wait for mutex to be released before attempting to finalize
	while (toWait) {
		pthread_mutex_lock(&mainmutex);
		pthread_cond_wait(&maincond, &mainmutex);
		pthread_mutex_unlock(&mainmutex);
	}

	// close the SFW test by sending a nil (0 length) message
	send_msg(ctlsockfd, TEST_FINALIZE, "", 0);

	// log
	teststatusnow = TEST_ENDED;
	protolog_status(getpid(), thistestId, teststatusnow, ctlsockfd);
	log_println(1, " <-------------------------->");
	// unset test name
	setCurrentTest(TEST_NONE);
}

/**
 * Performs the server part of the Simple firewall test.
 * @param ctlsockfd Client control socket descriptor
 * @param agent web100_agent
 * @param options The test options
 * @param conn_options The connection options
 * @returns Integer with values:
 * 			0 - success (no firewalls on the path),
 *          1 - Message reception errors/inconsistencies
 *			2 - Unexpected message type received/no message received due to timeout
 *			3 - Received message is invalid
 *			4 - Client port number not received
 *			5 - Unable to resolve client address
 */

int test_sfw_srv(int ctlsockfd, web100_agent* agent, TestOptions* options,
		int conn_options) {
	char buff[BUFFSIZE + 1];I2Addr sfwsrv_addr = NULL;
	int sfwsockfd, sfwsockport, sockfd, sfwport;
	struct sockaddr_storage cli_addr;
	socklen_t clilen;
	fd_set fds;
	struct timeval sel_tv;
	int msgLen, msgType;
	web100_var* var;
	web100_connection* cn;
	web100_group* group;
	int maxRTT, maxRTO;
	char hostname[256];
	int rc;

	// variables used for protocol validation
	enum TEST_ID thistestId = NONE;
	enum TEST_STATUS_INT teststatusnow = TEST_NOT_STARTED;
	enum PROCESS_TYPE_INT proctypeenum = CONNECT_TYPE;
	enum PROCESS_STATUS_INT procstatusenum = UNKNOWN;

	assert(ctlsockfd != -1);
	assert(options);

	if (options->sfwopt) {
		setCurrentTest(TEST_SFW);
		log_println(1, " <-- %d - Simple firewall test -->", options->child0);
		thistestId = SFW;
		teststatusnow = TEST_STARTED;
		protolog_status(options->child0, thistestId, teststatusnow,ctlsockfd);

		// bind to a new port and obtain address structure with details of port etc
		sfwsrv_addr = CreateListenSocket(NULL, "0", conn_options, 0);
		if (sfwsrv_addr == NULL) {
			log_println(0, "Server (Simple firewall test): CreateListenSocket failed: %s", strerror(errno));
			snprintf(buff, sizeof(buff), "Server (Simple firewall test): CreateListenSocket failed: %s", strerror(errno));
			send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
			return -1;
		}

		// get socket FD and the ephemeral port number to be used
		sfwsockfd = I2AddrFD(sfwsrv_addr);
		sfwsockport = I2AddrPort(sfwsrv_addr);
		log_println(1, "  -- port: %d", sfwsockport);

		cn = web100_connection_from_socket(agent, ctlsockfd);
		if (cn) {

			// Get remote end's address
			web100_agent_find_var_and_group(agent, "RemAddress", &group, &var);
			web100_raw_read(var, cn, buff);
			memset(hostname, 0, 256);
			//strncpy(hostname, web100_value_to_text(web100_get_var_type(var), buff), 255);
			strlcpy(hostname, web100_value_to_text(web100_get_var_type(var), buff), sizeof(hostname));

			// Determine test time in seconds.
			// test-time = max(round trip time, timeout) > 3 ? 3 : 1

			web100_agent_find_var_and_group(agent, "MaxRTT", &group, &var);
			web100_raw_read(var, cn, buff);
			maxRTT = atoi(web100_value_to_text(web100_get_var_type(var), buff));
			web100_agent_find_var_and_group(agent, "MaxRTO", &group, &var);
			web100_raw_read(var, cn, buff);
			maxRTO = atoi(web100_value_to_text(web100_get_var_type(var), buff));
			if (maxRTT > maxRTO)
			maxRTO = maxRTT;
			if ((((double) maxRTO) / 1000.0) > 3.0)
			/* `testTime = (((double) maxRTO) / 1000.0) * 4 ; */
			testTime = 3;
			else
			testTime = 1;
		}
		else {
			log_println(0, "Simple firewall test: Cannot find connection");
			snprintf(buff, sizeof(buff), "Server (Simple firewall test): Cannot find connection");
			send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
			I2AddrFree(sfwsrv_addr);
			return -1;
		}
		log_println(1, "  -- SFW time: %d", testTime);

		// try sending TEST_PREPARE msg with ephemeral port number to client.
		// If unable to, return
		snprintf(buff, sizeof(buff), "%d %d", sfwsockport, testTime);
		if ((rc = send_msg(ctlsockfd, TEST_PREPARE, buff, strlen(buff))) < 0)
		return (rc);

		// Listen for TEST_MSG from client's port number sent as data.
		// Any other type of message is unexpected at this juncture.
		msgLen = sizeof(buff);
		if (recv_msg(ctlsockfd, &msgType, buff, &msgLen)) { // message reception error
			log_println(0, "Protocol error!");
			snprintf(buff, sizeof(buff), "Server (Simple firewall test): Invalid port number received");
			send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
			I2AddrFree(sfwsrv_addr);
			return 1;
		}
		if (check_msg_type("Simple firewall test", TEST_MSG, msgType, buff, msgLen)) {
			log_println(0, "Fault, unexpected message received!");
			snprintf(buff, sizeof(buff), "Server (Simple firewall test): Invalid port number received");
			send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
			I2AddrFree(sfwsrv_addr);
			return 2;
		}
		if (msgLen <= 0) { // message reception has error
			log_println(0, "Improper message");
			snprintf(buff, sizeof(buff), "Server (Simple firewall test): Invalid port number received");
			send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
			I2AddrFree(sfwsrv_addr);
			return 3;
		}
		// Note: The same error message is transmitted to the client
		// under any error condition, but the log messages  indicate the difference

		buff[msgLen] = 0;
		if (check_int(buff, &sfwport)) { // message data is not number, thus no port info received
			log_println(0, "Invalid port number");
			snprintf(buff, sizeof(buff), "Server (Simple firewall test): Invalid port number received");
			send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
			I2AddrFree(sfwsrv_addr);
			return 4;
		}

		// Get node, port(if present) and other details of client end.
		// If not able to resolve it, the test cannot proceed to the "throughput" stage
		if ((sfwcli_addr = I2AddrByNode(get_errhandle(), hostname)) == NULL) {
			log_println(0, "Unable to resolve server address"); //todo, this is the client address we cannot resolve?
			send_msg(ctlsockfd, TEST_FINALIZE, "", 0);

			// log end
			teststatusnow = TEST_ENDED;
			protolog_status(options->child0, thistestId, teststatusnow, ctlsockfd);
			log_println(1, " <-------------------------->");
			I2AddrFree(sfwsrv_addr);
			return 5;
		}
		I2AddrSetPort(sfwcli_addr, sfwport);
		log_println(1, "  -- oport: %d", sfwport);

		// send S->C side TEST_MSG
		send_msg(ctlsockfd, TEST_START, "", 0);

		// send the S->C default test message in a separate thread to client
		pthread_create(&threadId, NULL, &test_osfw_srv, NULL);

		FD_ZERO(&fds);
		FD_SET(sfwsockfd, &fds);
		sel_tv.tv_sec = testTime;
		sel_tv.tv_usec = 0;

		// determine status of listening socket - looking for TEST_MSG from client.
		// The following switch statement checks for cases where this socket status
		// indicates error , and finalizes the test at this point.

		switch (select(sfwsockfd+1, &fds, NULL, NULL, &sel_tv)) {
			case -1: // If SOCKET_ERROR - status of firewall unknown
			log_println(0, "Simple firewall test: select exited with error");
			snprintf(buff, sizeof(buff), "%d", SFW_UNKNOWN);
			send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
			finalize_sfw(ctlsockfd);
			I2AddrFree(sfwsrv_addr);
			I2AddrFree(sfwcli_addr);
			return 1;
			case 0:// Time expiration. SFW possible in C->S side
			log_println(0, "Simple firewall test: no connection for %d seconds", testTime);
			snprintf(buff, sizeof(buff), "%d", SFW_POSSIBLE);
			send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
			finalize_sfw(ctlsockfd);
			I2AddrFree(sfwsrv_addr);
			I2AddrFree(sfwcli_addr);
			return 2;
		}

		// Now read actual data from the sockets listening for client message.
		// Based on data, conclude SFW status and send TEST_MSG with result.
		// In all cases, finalize the test
		clilen = sizeof(cli_addr);
		sockfd = accept(sfwsockfd, (struct sockaddr *) &cli_addr, &clilen);
		// protocol validation log to indicate client tried connecting
		procstatusenum = PROCESS_STARTED;
		protolog_procstatus(options->child0, thistestId, proctypeenum, procstatusenum, sockfd);

		msgLen = sizeof(buff);
		if (recv_msg(sockfd, &msgType, buff, &msgLen)) { // message received in error
			log_println(0, "Simple firewall test: unrecognized message");
			snprintf(buff, sizeof(buff), "%d", SFW_UNKNOWN);
			send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
			close(sockfd);
			finalize_sfw(ctlsockfd);
			I2AddrFree(sfwsrv_addr);
			I2AddrFree(sfwcli_addr);
			return 1;
		}
		if (check_msg_type("Simple firewall test", TEST_MSG, msgType, buff, msgLen)) {
			// unexpected message type received
			snprintf(buff, sizeof(buff), "%d", SFW_UNKNOWN);
			send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
			close(sockfd);
			finalize_sfw(ctlsockfd);
			I2AddrFree(sfwsrv_addr);
			I2AddrFree(sfwcli_addr);
			return 1;
		}
		if (msgLen != SFW_TEST_DEFAULT_LEN) { // Expecting default 20 byte long "Simple firewall test" message
			log_println(0, "Simple firewall test: Improper message");
			snprintf(buff, sizeof(buff), "%d", SFW_UNKNOWN);
			send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
			close(sockfd);
			finalize_sfw(ctlsockfd);
			I2AddrFree(sfwsrv_addr);
			I2AddrFree(sfwcli_addr);
			return 1;
		}
		buff[msgLen] = 0;
		if (strcmp(buff, "Simple firewall test") != 0) {
			// Message was of correct length, but was not expected content-wise
			log_println(0, "Simple firewall test: Improper message");
			snprintf(buff, sizeof(buff), "%d", SFW_UNKNOWN);
			send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
			close(sockfd);
			finalize_sfw(ctlsockfd);
			I2AddrFree(sfwsrv_addr);
			I2AddrFree(sfwcli_addr);
			return 1;
		}

		// All messages were received correctly, hence no firewall
		snprintf(buff, sizeof(buff), "%d", SFW_NOFIREWALL);
		send_msg(ctlsockfd, TEST_MSG, buff, strlen(buff));
		close(sockfd);
		finalize_sfw(ctlsockfd);
		I2AddrFree(sfwsrv_addr);
		I2AddrFree(sfwcli_addr);
	}
	return 0;
}
