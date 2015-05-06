/*
 * This file contains the function declarations to handle META test.
 *
 * Jakub Sławiński 2011-05-07
 * jeremian@poczta.fm
 */

#ifndef SRC_TEST_META_H_
#define SRC_TEST_META_H_

#define META_CLIENT_OS "client.os.name"
#define META_BROWSER_OS "client.browser.name"
#define META_CLIENT_KERNEL_VERSION "client.kernel.version"
#define META_CLIENT_VERSION "client.version"
#define META_CLIENT_APPLICATION "client.application"
#define META_CLIENT_APPLICATION_ID "cli"
 
#define META_TEST_LOG "META test"

int test_meta_clt(int ctlSocket, char tests, char* host, int conn_options, char* client_app_id, int jsonSupport);

#endif  // SRC_TEST_META_H_
