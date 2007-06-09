/*
 * This file contains the definitions and function declarations to
 * handle various tests.
 *
 * Jakub S³awiñski 2006-06-24
 * jeremian@poczta.fm
 */

#ifndef _JS_TESTOPTIONS_H
#define _JS_TESTOPTIONS_H

#include "web100srv.h"

typedef struct testoptions {
  int multiple;
  int mainport;
  
  int midopt;
  int midsockfd;
  int midsockport;
  
  int c2sopt;
  int c2ssockfd;
  int c2ssockport;
  
  int s2copt;
  int s2csockfd;
  int s2csockport;

  int sfwopt;
} TestOptions;

int initialize_tests(int ctlsockfd, TestOptions* options, int conn_options);
int test_mid(int ctlsockfd, web100_agent* agent, TestOptions* options, int conn_options, double* s2c2spd);
int test_c2s(int ctlsockfd, web100_agent* agent, TestOptions* testOptions, int conn_options, double* c2sspd,
    int set_buff, int window, int autotune, char* device, Options* options,
    int record_reverse, int count_vars, char spds[4][256], int* spd_index);
int test_s2c(int ctlsockfd, web100_agent* agent, TestOptions* testOptions, int conn_options, double* s2cspd,
    int set_buff, int window, int autotune, char* device, Options* options, char spds[4][256],
    int* spd_index, int count_vars);
int test_sfw_srv(int ctlsockfd, web100_agent* agent, TestOptions* options, int conn_options);
int getCurrentTest();
void setCurrentTest(int testId);

#endif
