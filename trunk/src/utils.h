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

