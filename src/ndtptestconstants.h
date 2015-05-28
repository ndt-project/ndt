/**
 * This file defines the various test types and other constants that the NDT protocol currently supports/uses.
 * This is used to define constants related to the test-suite, specifically.
 *
 * */

#ifndef SRC_NDTPTESTCONSTANTS_H_
#define SRC_NDTPTESTCONSTANTS_H_

#define TEST_NONE 0
#define TEST_MID (1L << 0)
#define TEST_C2S (1L << 1)
#define TEST_S2C (1L << 2)
#define TEST_SFW (1L << 3)
#define TEST_STATUS (1L << 4)
#define TEST_META (1L << 5)
#define TEST_EXT (1L << 6)

// will hold "string "middlebox", which is the longest name
#define TEST_NAME_DESC_SIZE 10

#define TEST_STATUS_DESC_SIZE 18  // test status < 18 chars

// direction is either client_to_server or server_to_client
#define TEST_DIRN_DESC_SIZE 20

// max size for now derived from "TEST_FINALIZE"
#define MSG_TYPE_DESC_SIZE 15
#define MSG_BODY_FMT_SIZE 10  // max size for desc "NOT_KNOWN"

// port numbers
#define PORT  "3001"
#define PORT2 "3002"
#define PORT3 "3003"
#define PORT4 "3003"

// keys' names used for storing specific data in JSON format
#define SERVER_ADDRESS "ServerAddress"
#define CLIENT_ADDRESS "ClientAddress"
#define CUR_MSS "CurMSS"
#define WIN_SCALE_SENT "WinScaleSent"
#define WIN_SCALE_RCVD "WinScaleRcvd"

// status of tests. Used mainly to log a "textual" explanation using below array
enum TEST_STATUS_INT {
  TEST_NOT_STARTED, TEST_STARTED, TEST_INPROGRESS, TEST_INCOMPLETE, TEST_ENDED
} teststatusint;

// Test IDs
enum TEST_ID {
  NONE, MIDDLEBOX, SFW, C2S, S2C, META
} testid;

// Transmission direction
enum Tx_DIRECTION {
  NO_DIR, C_S, S_C
} txdirection;

enum MSG_BODY_TYPE {
  BITFIELD, STRING, NOT_KNOWN
} bodyformattype;

char *get_testnamedesc(enum TEST_ID testid, char *stestnamearg);
char *get_teststatusdesc(enum TEST_STATUS_INT teststatus, char *steststatusarg);
char *get_testdirectiondesc(enum Tx_DIRECTION testdirection,
                            char *stestdirectionarg);
char *get_msgtypedesc(int msgtypearg, char *smsgtypearg);
char *getmessageformattype(enum MSG_BODY_TYPE bodymsgformat,
                           char *smsgformattypearg);

#endif  // SRC_NDTPTESTCONSTANTS_H_
