/*
 * This file contains the definitions and function declarations to
 * handle the server-side tests.
 *
 * kkumar@internet2.edu
 * Oct 2011
 */

#ifndef SRC_TESTS_SRV_H_
#define SRC_TESTS_SRV_H_

#include "testoptions.h"
#include "logging.h"

int test_c2s(Connection* ctl, tcp_stat_agent* agent, TestOptions* testOptions,
             int conn_options, double* c2sspd, int set_buff, int window,
             int autotune, char* device, Options* options, int record_reverse,
             int count_vars, char spds[4][256], int* spd_index, SSL_CTX* ctx,
             struct throughputSnapshot **c2s_ThroughputSnapshots, int extended);

// S2C test
int test_s2c(Connection* ctl, tcp_stat_agent* agent, TestOptions* testOptions,
             int conn_options, double* s2cspd, int set_buff, int window,
             int autotune, char* device, Options* options, char spds[4][256],
             int* spd_index, int count_vars, CwndPeaks* peaks, SSL_CTX* ctx,
             struct throughputSnapshot **s2c_ThroughputSnapshots, int extended);

// the middlebox test
int test_mid(Connection* ctl, tcp_stat_agent* agent, TestOptions* testOptions,
             int conn_options, double* s2c2spd);

#endif  // SRC_TESTS_SRV_H_
