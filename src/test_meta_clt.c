/**
 * This file contains the functions needed to handle META
 * test (client part).
 *
 * Jakub Sławiński 2011-05-07
 * jeremian@poczta.fm
 */

#include "../config.h"

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
#include "jsonutils.h"

int pkts, lth;
int sndqueue;
double spdout, c2sspd;

/**
 * The META test allows the Client to send additional information to the
 * Server that gets included along with the overall set of test results.
 * @param ctlSocket server control socket descriptor
 * @param tests set of tests to perform
 * @param host  hostname of the server
 * @param conn_options connection options
 * @param jsonSupport indicates if messages should be send using JSON format
 * @return integer
 * 		 0 = (the test has been finalized)
 *      >0 if protocol interactions were not as expected:
 *     		1: Unable to receive protocol message successfully
 * 			2: Wrong message type received
 */
int test_meta_clt(int ctlSocket, char tests, char* host, int conn_options, char* client_app_id, int jsonSupport) {
  char buff[1024], tmpBuff[512];
  int msgLen, msgType;
  FILE * fp;

  // Protocol validation variables
  enum TEST_STATUS_INT teststatuses = TEST_NOT_STARTED;
  enum TEST_ID testids = META;

  if (tests & TEST_META) {  // perform META tests
    log_println(1, " <-- META test -->");
    setCurrentTest(TEST_META);
    // protocol logs
    teststatuses = TEST_STARTED;
    protolog_status(getpid(), testids, teststatuses, ctlSocket);
    msgLen = sizeof(buff);
    // Server starts with a TEST_PREPARE message. Any other
    // .. type of message at this juncture is unexpected !
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed prepare message!");
      return 1;
    }
    if (check_msg_type(META_TEST_LOG, TEST_PREPARE, msgType, buff, msgLen)) {
      return 2;
    }

    // Server now sends a TEST_START message. Any other message type is
    // .. indicative of errors
    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed start message!");
      return 1;
    }
    if (check_msg_type(META_TEST_LOG, TEST_START, msgType, buff, msgLen)) {
      return 2;
    }

    // As a response to the Server's TEST_START message, client responds
    // with TEST_MSG. These messages may be used, as below, to send
    // configuration data name-value pairs. Note that there are length
    // constraints to keys- values: 64/256 characters respectively

    printf("sending meta information to server . . . . . ");
    fflush(stdout);

    snprintf(buff, sizeof(buff), "%s:%s", META_CLIENT_APPLICATION, client_app_id);
    send_json_message(ctlSocket, TEST_MSG, buff, strlen(buff),
                      jsonSupport, JSON_SINGLE_VALUE);
    // send client os name details
    if ((fp = fopen("/proc/sys/kernel/ostype", "r")) == NULL) {
      log_println(0, "Unable to determine client os type.");
    } else {
      fscanf(fp, "%s", tmpBuff);
      fclose(fp);
      snprintf(buff, sizeof(buff), "%s:%s", META_CLIENT_OS, tmpBuff);
      send_json_message(ctlSocket, TEST_MSG, buff, strlen(buff),
                        jsonSupport, JSON_SINGLE_VALUE);
    }

    // send client browser name
    snprintf(buff, sizeof(buff), "%s:%s", META_BROWSER_OS, "- (web100clt)");
    send_json_message(ctlSocket, TEST_MSG, buff, strlen(buff),
                      jsonSupport, JSON_SINGLE_VALUE);

    // send client kernel version
    if ((fp = fopen("/proc/sys/kernel/osrelease", "r")) == NULL) {
      log_println(0, "Unable to determine client kernel version.");
    } else {
      fscanf(fp, "%s", tmpBuff);
      fclose(fp);
      snprintf(buff, sizeof(buff), "%s:%s", META_CLIENT_KERNEL_VERSION,
               tmpBuff);
      send_json_message(ctlSocket, TEST_MSG, buff, strlen(buff),
                        jsonSupport, JSON_SINGLE_VALUE);
    }

    // send NDT client version
    snprintf(buff, sizeof(buff), "%s:%s", META_CLIENT_VERSION, VERSION);
    send_json_message(ctlSocket, TEST_MSG, buff, strlen(buff),
                      jsonSupport, JSON_SINGLE_VALUE);

    // Client can send any number of such meta data in a TEST_MSG
    // .. format, and signal the end of the transmission using an empty TEST_MSG
    send_json_message(ctlSocket, TEST_MSG, "", 0, jsonSupport, JSON_SINGLE_VALUE);

    printf("Done\n");

    // All data has been sent. Expect server to close this META test session
    // ... by sending an empty TEST_FINALIZE message
    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed finalize message!");
      return 1;
    }
    if (check_msg_type(META_TEST_LOG, TEST_FINALIZE, msgType, buff, msgLen)) {
      return 2;
    }
    log_println(1, " <------------------------->");
    // log protocol validation logs
    teststatuses = TEST_ENDED;
    protolog_status(getpid(), testids, teststatuses, ctlSocket);
    setCurrentTest(TEST_NONE);
  }
  return 0;
}
