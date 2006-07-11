/*
 * This file contains the functions for the protocol handling.
 *
 * Jakub S³awiñski 2006-07-11
 * jeremian@poczta.fm
 */

#include "protocol.h"
#include "logging.h"

/*
 * Function name: check_msg_type
 * Description: Checks if the received msg type is compatible
 *              with the expected one.
 * Arguments: prefix - the prefix printed before the error message
 *            expected - the expected msg type
 *            received - the received msg type
 * Return: 0 - everything is ok
 *         1 - error code
 */

int
check_msg_type(char* prefix, int expected, int received)
{
  if (expected != received) {
    if (prefix) {
      log_print(0, "%s: ", prefix);
    }
    log_println(0, "ERROR: received message type %d (expected %d)", received, expected);
    return 1;
  }
  return 0;
}
