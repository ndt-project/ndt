/*
 * This file contains the function declarations to handle simple
 * firewall test.
 *
 * Jakub S³awiñski 2006-07-15
 * jeremian@poczta.fm
 */

#ifndef SRC_TEST_SFW_H_
#define SRC_TEST_SFW_H_

#define SFW_NOTTESTED  0
#define SFW_NOFIREWALL 1
#define SFW_UNKNOWN    2
#define SFW_POSSIBLE   3

#define SFW_TEST_DEFAULT_LEN 20
#define SFW_PREDEFINED_TEST_MSG "Simple firewall test"
#define SFW_TEST_LOG "Simple firewall test"

int test_sfw_clt(int ctlsockfd, char tests, char* host, int conn_options);
int results_sfw(char tests, char* host);

#endif  // SRC_TEST_SFW_H_
