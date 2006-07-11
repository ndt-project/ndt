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

int check_msg_type(char* prefix, int expected, int received);

#endif
