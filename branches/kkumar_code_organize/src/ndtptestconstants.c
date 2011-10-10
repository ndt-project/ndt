/**
 * This file is used to store constants related to the NDTP tests.
 * It also defines functions to get  descriptive names for tests
 * and such utility functionality.
 *
 *  Created on: Sep 29, 2011
 *      Author: kkumar
 */

#include <string.h>
#include "ndtptests.h"

// The arrays below rely on the ordering of corresponding enum in the .h file.
// test names
static char *_teststatusdescarray[] = {
		"test_not_started",
		"test_started",
		"test_in_progress",
		"test_incomplete",
		"test_complete"
};

// names of tests.
static char *_testnamesarray[] = {
		"None",
		"Middlebox",
		"SFW",
		"C2S",
		"S2C",
		"Meta"
};

// names of test messages to log in descriptive names instead of numbers
static char * _testmsgtypesarray[] = {
  "COMM_FAILURE",
  "SRV_QUEUE",
  "MSG_LOGIN",
  "TEST_PREPARE",
  "TEST_START",
 "TEST_MSG",
 "TEST_FINALIZE",
 "MSG_ERROR",
 "MSG_RESULTS",
 "MSG_LOGOUT",
 "MSG_WAITING",
};

// names of protocol message transmission directions
static char *_txdirectionsarray[] = {
		"none",
		"client_to_server",
		"server_to_client"
};

/**
 * Get descriptive string for test name
 * @param testid Id of the test
 * @param *snamearg  storage for test name description
 * @return char*  Descriptive string for test name
 * */
char *get_testnamedesc(enum TEST_ID testid, char *snamearg) {
	snamearg = _testnamesarray[testid];
	//printf ("--current test name = %s for %d\n", snamearg ,testid);
	return snamearg;
}

/**
 * Get descriptive string for test status
 * @param teststatus Integer status of the test
 * @param *sstatusarg storage for test status description
 * @return char*  Descriptive string for test status
 * */
char *get_teststatusdesc(enum TEST_STATUS_INT teststatus, char *sstatusarg) {
	sstatusarg =  _teststatusdescarray[teststatus];
	//printf ("--current test status = %s, for %d \n", sstatusarg, teststatus);
	return sstatusarg;
}

/**
 * Get descriptive string for test direction.
 * Directions could be C->S or S->C, currently.
 * @param testdirection Test direction
 * @param *sdirnarg  storage for test direction
 * @return char*  Descriptive string fr Test direction
 * */
char *get_testdirectiondesc(enum Tx_DIRECTION testdirection, char *sdirnarg) {
	sdirnarg = _txdirectionsarray[testdirection];
	//printf ("--current test direction = %s , for %d\n", sdirnarg, testdirection);
	return sdirnarg;
}

/**
 * Get descriptive string for protocol message types.
 *
 * @param msgtype Test direction
 * @param *smsgtypearg  storage for test direction
 * @return char* Descriptive string for Message type
 * */
char *get_msgtypedesc(int msgtype, char *smsgtypearg) {
	smsgtypearg = _testmsgtypesarray[msgtype];
	//printf ("--current test type = %s , for %d\n", smsgtypearg, msgtype);
	return smsgtypearg;
}
