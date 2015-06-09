/**
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
#include "jsonutils.h"
#include "strlutils.h"
#include "websocket.h"

 void addAdditionalMetaEntry(struct metaentry **ptr, char* key, char* value);
 void addAdditionalMetaIntEntry(struct metaentry **ptr, char* key, int value);

/**
 * Performs the META test.
 * @param ctlsockfd Client control socket descriptor
 * @param agent UNUSED Web100 agent used to track the connection
 * @param testOptions The test options
 * @param conn_options The connection options
 * @param options Test Option variables
 * @return 0 - success,
 *          >0 - error code.
 *          Error codes:
 *           -1 - Cannot write message header information while attempting to send
 *           		 TEST_START message
 *          -2 - Cannot write message data while attempting to send
 *           		 TEST_START message
 *          1 - Message reception errors/inconsistencies
 *			2 - Unexpected message type received/no message received due to timeout
 *			3 - Received message is invalid
 *			4 - Invalid data format in received message
 */

int test_meta_srv(int ctlsockfd, tcp_stat_agent* agent,
                  TestOptions* testOptions, int conn_options, Options* options) {
  int j;
  int64_t err;
  int msgLen, msgType;
  char buff[BUFFSIZE + 1];
  struct metaentry *new_entry = NULL;
  char* value, *jsonMsgValue;

  // protocol validation logs
  enum TEST_ID testids = META;
  enum TEST_STATUS_INT teststatuses = TEST_NOT_STARTED;

  if (testOptions->metaopt) {
    setCurrentTest(TEST_META);
    log_println(1, " <-- META test -->");

    // log protocol validation details
     teststatuses = TEST_STARTED;
    protolog_status(testOptions->child0, testids, teststatuses, ctlsockfd);

    // first message exchanged is am empty TEST_PREPARE message
    j = send_json_message(ctlsockfd, TEST_PREPARE, "", 0,
                          testOptions->connection_flags, JSON_SINGLE_VALUE);
    if (j == -1 || j == -2) {  // Cannot write message headers/data
      log_println(6, "META Error!, Test start message not sent!");
      return j;
    }

    // Now, transmit an empty TEST_START message
    if (send_json_message(ctlsockfd, TEST_START, "", 0,
                          testOptions->connection_flags, JSON_SINGLE_VALUE) < 0) {
      log_println(6, "META test - Test-start message failed");
    }

    while (1) {
      msgLen = sizeof(buff);
      // Now read the meta data sent by client.

      if (testOptions->connection_flags & WEBSOCKET_SUPPORT) {
        err = recv_websocket_ndt_msg(ctlsockfd, &msgType, buff, &msgLen);
      } else {
        err = recv_msg(ctlsockfd, &msgType, buff, &msgLen);
      }
      if (err) {
        // message reading error
        log_println(0, "Protocol error!");
        snprintf(buff, sizeof(buff),
                 "Server (META test): Invalid meta data received");
        send_json_message(ctlsockfd, MSG_ERROR, buff, strlen(buff),
                          testOptions->connection_flags, JSON_SINGLE_VALUE);
        return 1;
      }
      if (check_msg_type("META test", TEST_MSG, msgType, buff, msgLen)) {
        // expected a TEST_MSG only
        log_println(0, "Fault, unexpected message received!");
        snprintf(buff, sizeof(buff),
                 "Server (META test): Invalid meta data received");
        send_json_message(ctlsockfd, MSG_ERROR, buff, strlen(buff),
                          testOptions->connection_flags, JSON_SINGLE_VALUE);
        return 2;
      }
      if (msgLen < 0) {
        //  meta data should be present at this stage
        log_println(0, "Improper message");
        snprintf(buff, sizeof(buff),
                 "Server (META test): Invalid meta data received");
        send_json_message(ctlsockfd, MSG_ERROR, buff, strlen(buff),
                          testOptions->connection_flags, JSON_SINGLE_VALUE);
        return 3;
      }

      buff[msgLen] = 0;
      if (testOptions->connection_flags & JSON_SUPPORT) {
        jsonMsgValue = json_read_map_value(buff, DEFAULT_KEY);
        if (strlen(jsonMsgValue) == 0) {
          free(jsonMsgValue);
          break;
        }
        strlcpy(buff, jsonMsgValue, sizeof(buff));
        free(jsonMsgValue);
      }

      // Received empty TEST_MSG. All meta_data has been received, and test
      //  can be finalized.
      if (msgLen == 0) {
        break;
      }

      value = index(buff, ':');
      if (value == NULL) {  // key-value separates by ":"
        log_println(0, "Improper message");
        snprintf(buff, sizeof(buff), "Server (META test): "
                 "Invalid meta data received");
        send_json_message(ctlsockfd, MSG_ERROR, buff, strlen(buff),
                          testOptions->connection_flags, JSON_SINGLE_VALUE);
        return 4;
      }
      *value = 0;
      value++;

      // get the recommended set of data expected: client os name,
      // client browser name,
      // why not if else-if for the whole set? TODO
      if (strcmp(META_CLIENT_OS, buff) == 0) {
        snprintf(meta.client_os, sizeof(meta.client_os), "%s", value);
        /*continue;*/
      }

      if (strcmp(META_CLIENT_APPLICATION, buff) == 0) {
        snprintf(meta.client_application, sizeof(meta.client_application),
                 "%s", value);
        /*continue;*/
      }

      if (strcmp(META_BROWSER_OS, buff) == 0) {
        snprintf(meta.client_browser, sizeof(meta.client_browser), "%s",
                 value);
        /*continue;*/
      }

      addAdditionalMetaEntry(&new_entry, buff, value);
    }
    if (testOptions->exttestsopt) {
      addAdditionalMetaIntEntry(&new_entry, "ext.c2s.duration", options->c2s_duration);
      addAdditionalMetaEntry(&new_entry, "ext.c2s.throughputsnaps", options->c2s_throughputsnaps ? "true" : "false");
      addAdditionalMetaIntEntry(&new_entry, "ext.c2s.snapsdelay", options->c2s_snapsdelay);
      addAdditionalMetaIntEntry(&new_entry, "ext.c2s.snapsoffset", options->c2s_snapsoffset);
      addAdditionalMetaIntEntry(&new_entry, "ext.c2s.streamsnum", options->c2s_streamsnum);
      addAdditionalMetaIntEntry(&new_entry, "ext.s2c.duration", options->s2c_duration);
      addAdditionalMetaEntry(&new_entry, "ext.s2c.throughputsnaps", options->s2c_throughputsnaps ? "true" : "false");
      addAdditionalMetaIntEntry(&new_entry, "ext.s2c.snapsdelay", options->s2c_snapsdelay);
      addAdditionalMetaIntEntry(&new_entry, "ext.s2c.snapsoffset", options->s2c_snapsoffset);
      addAdditionalMetaIntEntry(&new_entry, "ext.s2c.streamsnum", options->s2c_streamsnum);
    }

    // Finalize test by sending appropriate message, and setting status
    if (send_json_message(ctlsockfd, TEST_FINALIZE, "", 0,
                          testOptions->connection_flags, JSON_SINGLE_VALUE) < 0) {
      log_println(6, "META test - failed to send finalize message");
    }

    // log end of test and conclude
    log_println(1, " <-------------------------->");

    teststatuses = TEST_ENDED;  // protocol log section
    protolog_status(testOptions->child0, testids, teststatuses, ctlsockfd);

    setCurrentTest(TEST_NONE);
  }
  return 0;
}

void addAdditionalMetaEntry(struct metaentry **ptr, char* key, char* value) {
  if (*ptr) {
    (*ptr)->next = (struct metaentry *) malloc(sizeof(struct metaentry));
    (*ptr) = (*ptr)->next;
  } else {
    (*ptr) = (struct metaentry *) malloc(sizeof(struct metaentry));
    meta.additional = (*ptr);
  }
  (*ptr)->next = NULL;  // ensure meta list ends here
  snprintf((*ptr)->key, sizeof((*ptr)->key), "%s", key);
  snprintf((*ptr)->value, sizeof((*ptr)->value), "%s", value);
}

void addAdditionalMetaIntEntry(struct metaentry **ptr, char* key, int value) {
  char tmpbuff[256];
  snprintf(tmpbuff, sizeof(tmpbuff), "%d", value);
  addAdditionalMetaEntry(ptr, key, tmpbuff);
}
