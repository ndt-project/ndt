/*
 * This file contains the definitions and function declarations
 * for the protocol handling.
 *
 * Jakub S�awi�ski 2006-07-11
 * jeremian@poczta.fm
 */

#ifndef SRC_PROTOCOL_H_
#define SRC_PROTOCOL_H_

// new addition after separating out ndtptests header
#include "ndtptestconstants.h"  // protocol validation
// Todo could be made into enumeration
#define COMM_FAILURE 0
#define SRV_QUEUE 1
#define MSG_LOGIN 2
#define TEST_PREPARE 3
#define TEST_START 4
#define TEST_MSG 5
#define TEST_FINALIZE 6
#define MSG_ERROR 7
#define MSG_RESULTS 8
#define MSG_LOGOUT 9
#define MSG_WAITING 10
#define MSG_EXTENDED_LOGIN 11

#define CS_VERSION_LENGTH_MAX 64

#define SRV_QUEUE_TEST_STARTS_NOW  0
#define SRV_QUEUE_SERVER_FAULT 9977
#define SRV_QUEUE_SERVER_BUSY 9987
#define SRV_QUEUE_HEARTBEAT 9990
#define SRV_QUEUE_SERVER_BUSY_60s 9999

/*
 #define TEST_NONE 0
 #define TEST_MID (1L << 0)
 #define TEST_C2S (1L << 1)
 #define TEST_S2C (1L << 2)
 #define TEST_SFW (1L << 3)
 #define TEST_STATUS (1L << 4)
 #define TEST_META (1L << 5)
 */

#define TOPT_DISABLED 0
#define TOPT_ENABLED 1

/* the difference between server's and client't throughput views that trigger
 * an alarm
 */
#define VIEW_DIFF 0.1

int check_msg_type(char* prefix, int expected, int received, char* buff,
                   int len);

#endif  // SRC_PROTOCOL_H_
