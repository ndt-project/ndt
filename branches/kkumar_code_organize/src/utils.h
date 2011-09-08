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
// NDT uses them for porta
#define MAX_TCP_PORT 65535
#define MIN_TCP_PORT 1
#define RESERVED_PORT "0"
#define MAX_TCP_PORT_STR "65535"
