/*
 * This file contains the function declarations to handle META test.
 *
 * Jakub Sławiński 2011-05-07
 * jeremian@poczta.fm
 */

#ifndef _JS_TEST_META_H
#define _JS_TEST_META_H

#define META_CLIENT_OS "client.os.name"
#define META_BROWSER_OS "client.browser.name"
#define META_CLIENT_KERNEL_VERSION "client.kernel.version"
#define META_CLIENT_VERSION "client.version"

int test_meta_clt(int ctlSocket, char tests, char* host, int conn_options);

#endif
