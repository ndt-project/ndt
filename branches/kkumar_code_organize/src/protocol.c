/**
 * This file contains the functions for the protocol handling.
 *
 * Jakub S³awiñski 2006-07-11
 * jeremian@poczta.fm
 */

#include "protocol.h"
#include "logging.h"

/**
 * Check if the received msg type is compatible
 *              with the expected one.
 * @param  prefix  the prefix printed before the error message
 * @param  expected the expected msg type
 * @param  received the received msg type
 * @param  buff actual message received
 * @param  len length of received message
 * @return  0 if everything is ok, 1 otherwise
 *
 */

int
check_msg_type(char* prefix, int expected, int received, char* buff, int len)
{
  // check if expected and received messages are the same
  if (expected != received) {
    if (prefix) { // Add prefix to log message
      log_print(0, "%s: ", prefix);
    }
    if (received == MSG_ERROR) { // if Error message was actually received,
    							 // then its not an unexpected message exchange
        buff[len] = 0; // terminate string
        log_println(0, "ERROR MSG: %s", buff);
    }
    else {
    	// certainly an unexpected message
        log_println(0, "ERROR: received message type %d (expected %d)", received, expected);
    }
    return 1;
  }
  // everything is well, return 0
  return 0;
}
