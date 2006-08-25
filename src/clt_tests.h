/*
 * This file contains the function declarations to handle client's
 * parts of the various tests.
 *
 * Jakub S³awiñski 2006-08-02
 * jeremian@poczta.fm
 */

#ifndef _JS_CLT_TESTS_H
#define _JS_CLT_TESTS_H

int test_mid_clt(int ctlSocket, char tests, char* host, int conn_options, int buf_size, char* tmpstr2);
int test_c2s_clt(int ctlSocket, char tests, char* host, int conn_options, int buf_size);
int test_s2c_clt(int ctlSocket, char tests, char* host, int conn_options, int buf_size, char* tmpstr);

#endif
