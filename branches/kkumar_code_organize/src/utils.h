/*
 * This file contains function declarations to handle numbers sanity checks
 * and some other utility things.
 *
 * Jakub S³awiñski 2006-06-12
 * jeremian@poczta.fm
 */

#ifndef _JS_UTILS_H
#define _JS_UTILS_H

int check_int(char* text, int* number);
int check_rint(char* text, int* number, int minVal, int maxVal);
int check_long(char* text, long* number);
double secs();
void err_sys(char* s);
int sndq_len(int fd);
void mysleep(double time);

#endif

// Numbers 1 and 65535 are used in mrange.c for determining "valid" ranges
// While the methods could be used for any "range" comparison,
// NDT uses them for ports
#define MAX_TCP_PORT 65535
#define MIN_TCP_PORT 1
#define RESERVED_PORT "0"
#define MAX_TCP_PORT_STR "65535"
#define KILO_BITS 1024
#define BITS_8 8
#define WINDOW_SCALE_THRESH 15
#define MEGA 1000000
#define KILO 1000

// duplex indicators
#define DUPLEX_OK_INDICATOR  0;
#define DUPLEX_OLD_ALGO_INDICATOR 1;
#define DUPLEX_SWITCH_FULL_HOST_HALF  2;

// link indicators
#define CANNOT_DETERMINE_LINK 100
#define LINK_ALGO_FAILED 0
#define LINK_ETHERNET 10
#define LINK_WIRELESS 3
#define LINK_DSLORCABLE 2

// half duplex condition indicators
#define NO_HALF_DUPLEX 0
#define POSSIBLE_HALF_DUPLEX 1

// congestion condition indicators
#define NO_CONGESTION 0
#define POSSIBLE_CONGESTION 1

// bad cable indicators
#define NO_BAD_CABLE 0
#define POSSIBLE_BAD_CABLE 1

// SFW Test default message length
#define SFW_TEST_DEFAULT_LEN 20

// generic system wide retry counts
#define RETRY_COUNT 5

// middlebox test default MSS
#define MIDDLEBOX_PREDEFINED_MSS 8192

// MAX TCP window size in bytes
#define MAX_TCP_WIN_BYTES 64

