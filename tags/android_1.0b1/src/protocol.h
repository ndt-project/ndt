/*
 * This file contains the definitions and function declarations
 * for the protocol handling.
 *
 * Jakub S³awiñski 2006-07-11
 * jeremian@poczta.fm
 */

#ifndef _JS_PROTOCOL_H
#define _JS_PROTOCOL_H

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

#define TEST_NONE 0
#define TEST_MID (1L << 0)
#define TEST_C2S (1L << 1)
#define TEST_S2C (1L << 2)
#define TEST_SFW (1L << 3)
#define TEST_STATUS (1L << 4)

#define TOPT_DISABLED 0
#define TOPT_ENABLED 1

/* the difference between server's and client't throughput views that trigger
 * an alarm
 */
#define VIEW_DIFF 0.1

int check_msg_type(char* prefix, int expected, int received, char* buff, int len);

#endif
