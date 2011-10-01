/**
 * This file defines the various test types and other constants that the NDT protocol currently supports/uses.
 *
 * */

#ifndef _JS_NDTPTESTS_H
#define _JS_NDTPTESTS_H

#define TEST_NONE 0
#define TEST_MID (1L << 0)
#define TEST_C2S (1L << 1)
#define TEST_S2C (1L << 2)
#define TEST_SFW (1L << 3)
#define TEST_STATUS (1L << 4)
#define TEST_META (1L << 5)

#define TEST_NAME_DESC_SIZE 10 // will hold "string "middlebox", which is the longest name
#define TEST_STATUS_DESC_SIZE 15 // test status < 15 chars
#define TEST_DIRN_DESC_SIZE 20 // direction is either client_to_server or server_to_client
#define MSG_TYPE_DESC_SIZE 15 // max size for now derived from "TEST_FINALIZE"

// status of tests. Used mainly to log a "textual" explanation using below array
enum  TEST_STATUS_INT { TEST_NOT_STARTED, TEST_STARTED, TEST_INPROGRESS, TEST_INCOMPLETE, TEST_ENDED } teststatusint;

// Test IDs
enum TEST_ID{ NONE, MIDDLEBOX, SFW, C2S, S2C, META } testid;

// Transmission direction
enum Tx_DIRECTION { NO_DIR, C_S , S_C} txdirection;

char *get_testnamedesc(enum TEST_ID testid, char *stestnamearg);
char *get_teststatusdesc(enum TEST_STATUS_INT teststatus, char *steststatusarg);
char *get_testdirectiondesc(enum Tx_DIRECTION testdirection, char *stestdirectionarg);
char *get_msgtypedesc(int msgtypearg, char *smsgtypearg) ;

#endif
