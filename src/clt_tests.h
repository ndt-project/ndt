/*
 * This file contains the function declarations to handle client's
 * parts of the various tests.
 *
 * Jakub S�awi�ski 2006-08-02
 * jeremian@poczta.fm
 */

#ifndef SRC_CLT_TESTS_H_
#define SRC_CLT_TESTS_H_

#include "logging.h"

#define MIDBOX_TEST_LOG "Middlebox test"
#define S2C_TEST_LOG "S2C throughput test"
#define MIDBOX_TEST_RES_SIZE 512

int test_mid_clt(int ctlSocket, char tests, char* host, int conn_options,
                 int buf_size, char* tmpstr2, int jsonSupport);
int test_c2s_clt(int ctlSocket, char tests, char* host, int conn_options,
                 int buf_size, struct throughputSnapshot **c2s_ThroughputSnapshots,
                 int jsonSupport, int extended);
int test_s2c_clt(int ctlSocket, char tests, char* host, int conn_options,
                 int buf_size, char* tmpstr,
                 struct throughputSnapshot **s2c_ThroughputSnapshots,
                 int jsonSupport, int extended);

#endif  // SRC_CLT_TESTS_H_
