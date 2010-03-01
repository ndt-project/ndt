/*
 * This file contains the function declarations to handle simple
 * firewall test.
 *
 * Jakub S³awiñski 2006-07-15
 * jeremian@poczta.fm
 */

#ifndef _JS_TEST_SFW_H
#define _JS_TEST_SFW_H

#define SFW_NOTTESTED  0
#define SFW_NOFIREWALL 1
#define SFW_UNKNOWN    2
#define SFW_POSSIBLE   3

int test_sfw_clt(int ctlsockfd, char tests, char* host, int conn_options);
int results_sfw(char tests, char* host);

#endif
