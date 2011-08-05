/*
 * This file contains the functions needed to handle META
 * test (server part).
 *
 * Jakub Sławiński 2011-05-07
 * jeremian@poczta.fm
 */

#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "test_meta.h"
#include "logging.h"
#include "network.h"
#include "protocol.h"
#include "utils.h"
#include "testoptions.h"

/*
 * Function name: test_meta_srv
 * Description: Performs the META test.
 * Arguments: ctlsockfd - the client control socket descriptor
 *            agent - the Web100 agent used to track the connection
 *            testOptions - the test options
 *            conn_options - the connection options
 * Returns: 0 - success,
 *          >0 - error code.
 */

int
test_meta_srv(int ctlsockfd, web100_agent* agent, TestOptions* testOptions, int conn_options)
{
  int j;
  int msgLen, msgType;
  char buff[BUFFSIZE+1];
  struct metaentry *new_entry = NULL;
  char* value;

  if (testOptions->metaopt) {
    setCurrentTest(TEST_META);
    log_println(1, " <-- META test -->");


    j = send_msg(ctlsockfd, TEST_PREPARE, "", 0);
    if (j == -1 || j == -2) {
      log_println(6, "META Error!, Test start message not sent!");
      return j;
    }

    if (send_msg(ctlsockfd, TEST_START, "", 0) < 0) {
      log_println(6, "META test - Test-start message failed");
    }

    while (1) {
      msgLen = sizeof(buff);
      if (recv_msg(ctlsockfd, &msgType, buff, &msgLen)) {
        log_println(0, "Protocol error!");
        sprintf(buff, "Server (META test): Invalid meta data received");
        send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
        return 1;
      }
      if (check_msg_type("META test", TEST_MSG, msgType, buff, msgLen)) {
        log_println(0, "Fault, unexpected message received!");
        sprintf(buff, "Server (META test): Invalid meta data received");
        send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
        return 2;
      }
      if (msgLen < 0) {
        log_println(0, "Improper message");
        sprintf(buff, "Server (META test): Invalid meta data received");
        send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
        return 3;
      }

      if (msgLen == 0) {
        break;
      }

      buff[msgLen] = 0;
      value = index(buff, ':');
      if (value == NULL) {
        log_println(0, "Improper message");
        sprintf(buff, "Server (META test): Invalid meta data received");
        send_msg(ctlsockfd, MSG_ERROR, buff, strlen(buff));
        return 4;
      }
      *value = 0;
      value++;

      if (strcmp(META_CLIENT_OS, buff) == 0) {
        snprintf(meta.client_os, sizeof(meta.client_os), "%s", value);
        /*continue;*/
      }

      if (strcmp(META_BROWSER_OS, buff) == 0) {
        snprintf(meta.client_browser, sizeof(meta.client_browser), "%s", value);
        /*continue;*/
      }

      if (strcmp(META_CLIENT_APPLICATION, buff) == 0) {
        snprintf(meta.client_application, sizeof(meta.client_application), "%s", value);
        /*continue;*/
      }

      if (new_entry) {
        new_entry->next = (struct metaentry *) malloc(sizeof(struct metaentry));
        new_entry = new_entry->next;
      }
      else {
        new_entry = (struct metaentry *) malloc(sizeof(struct metaentry));
        meta.additional = new_entry;
      }
      snprintf(new_entry->key, sizeof(new_entry->key), "%s", buff);
      snprintf(new_entry->value, sizeof(new_entry->value), "%s", value);
    }

    if (send_msg(ctlsockfd, TEST_FINALIZE, "", 0) < 0) {
      log_println(6, "META test - failed to send finalize message");
    }

    log_println(1, " <-------------------------->");
    setCurrentTest(TEST_NONE);
  }
  return 0;
}
