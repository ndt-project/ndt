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
int trim(char *line, int line_size,
		char * output_buf, int output_buf_size);

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
#define BITS_8_FLOAT 8.0
#define WINDOW_SCALE_THRESH 15
#define MEGA 1000000
#define KILO 1000

// duplex indicators
#define DUPLEX_OK_INDICATOR 0
#define DUPLEX_OLD_ALGO_INDICATOR 1
#define DUPLEX_SWITCH_FULL_HOST_HALF 2
#define DUPLEX_SWITCH_HALF_HOST_FULL 3
#define DUPLEX_SWITCH_FULL_HOST_HALF_POSS 4
#define DUPLEX_SWITCH_HALF_HOST_FULL_POSS 5
#define DUPLEX_SWITCH_HALF_HOST_FULL_WARN 7

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

// generic system wide constants
#define RETRY_COUNT 5		// retry counts
//#define BUFFSIZE  8192		// Buffer size

// middlebox test default MSS
// Cleaner to move this to clt_tests.h if not for the name of "clt_tests.h".
// ..web100-util.c would need to import this header file.
#define MIDDLEBOX_PREDEFINED_MSS 8192

// MAX TCP window size in kilo-bytes
#define MAX_TCP_WIN_BYTES 64
#define ETHERNET_MTU_SIZE 1456

// link speed indicators
#define DATA_RATE_INSUFFICIENT_DATA -2
#define DATA_RATE_SYSTEM_FAULT -1
#define DATA_RATE_RTT 0
#define DATA_RATE_DIAL_UP 1
#define DATA_RATE_T1 2
#define DATA_RATE_ETHERNET 3
#define DATA_RATE_T3 4
#define DATA_RATE_FAST_ETHERNET 5
#define DATA_RATE_OC_12 6
#define DATA_RATE_GIGABIT_ETHERNET 7
#define DATA_RATE_OC_48 8
#define DATA_RATE_10G_ETHERNET 9
#define DATA_RATE_RETRANSMISSIONS 10


