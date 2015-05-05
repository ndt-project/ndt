#include "network_clt.h"
#include "network.h"

/**
 * Converts message to JSON format and sends it to the control socket.
 * @param ctlSocket control socket
 * @param type type of the message
 * @param msg message to send
 * @param len length of the message
 * @param connectionFlags indicates if JSON format is supported by the other side (connectionFlags & JSON_SUPPORT)
 *                        and if websockets are supported by the other side (connectionFlags & WEBSOCKET_SUPPORT)
 *                        It is expected, but not required, that WEBSOCKET_SUPPORT will always include JSON_SUPPORT.
 * @param jsonConvertType defines how message converting should be handled:
 *			JSON_SINGLE_VALUE: single key/value pair is being created (using default key)
 *							   with msg as value
 *			JSON_MULTIPLE_VALUES: multiple key/values pairs are being created using
 *                                keys, keys_delimiters, values and values_delimiters
 *                                params
 *			JSON_KEY_VALUE_PAIRS: given message contains one or many key/value pairs
 *                                with following syntax: key1: value1
 *                                                       key2: value2 etc
 * @param keys buffer containing keys' values (used only when jsonConvertType is set to
 * 			   JSON_MULTIPLE_VALUES)
 * @param keysDelimiters delimiters for keys parameter (used only when jsonConvertType is set to
 * 			   JSON_MULTIPLE_VALUES)
 * @param values buffer containing map values, key-value matching is being done by order in which
 * 		  		 they appear in keys and values params (used only when jsonConvertType is set to
 * 			     JSON_MULTIPLE_VALUES)
 * @param valuesDelimiters delimiters for values parameter (used only when jsonConvertType is
 * 			   set to JSON_MULTIPLE_VALUES)
 *
 * @return 0 on success, error code otherwise
 *        Error codes:
 *        -1 - Cannot write to socket at all
 *        -2 - Cannot complete writing full message data into socket
 *        -3 - Cannot write after retries
 *        -4 - Cannot convert msg to JSON
 *
 */
int send_json_msg(int ctlSocket, int type, const char* msg, int len,
                  int connectionFlags, int jsonConvertType,
                  const char *keys, const char *keysDelimiters,
                  const char *values, char *valuesDelimiters) {
  Connection ctl = {ctlSocket, NULL};
  return send_json_msg_any(&ctl, type, msg, len, connectionFlags,
			   jsonConvertType, keys, keysDelimiters, values,
                           valuesDelimiters);
}

/**
 * Shortest version of send_json_msg method. Uses default NULL values for
 * JSON_MULTIPLE_VALUES convert type specific parameters.
 */
int send_json_message(int ctlSocket, int type, const char* msg, int len,
                      int connectionFlags, int jsonConvertType) {
  Connection ctl = {ctlSocket, NULL};
  return send_json_message_any(&ctl, type, msg, len, connectionFlags, jsonConvertType);
}


/**
 * Sends the protocol message to the control socket.
 * @param ctlSocket control socket
 * @param type type of the message
 * @param msg message to send
 * @param len length of the message
 * @return 0 on success, error code otherwise
 *        Error codes:
 *        -1 - Cannot write to socket at all
 *        -2 - Cannot complete writing full message data into socket
 *        -3 - Cannot write after retries
 */
int send_msg(int ctlSocket, int type, const void* msg, int len) {
  Connection conn = {ctlSocket, NULL};
  return send_msg_any(&conn, type, msg, len);
}

/**
 * Receive the protocol message from the control socket.
 * @param ctlSocket control socket
 * @param type target place for type of the message
 * @param msg target place for the message body
 * @param len target place for the length of the message
 * @returns 0 on success, error code otherwise.
 *          Error codes:
 *          -1 : Error reading from socket
 *          -2 : No of bytes received were lesser than expected byte count
 *          -3 : No of bytes received did not match expected byte count
 */
int recv_msg(int ctlSocket, int* type, void* msg, int* len) {
  Connection ctl = {ctlSocket, NULL};
  return recv_msg_any(&ctl, type, msg, len);
}

/**
 * Write the given amount of data to the file descriptor.
 * @param fd the file descriptor
 * @param buf buffer with data to write
 * @param amount the size of the data
 * @return The amount of bytes written to the file descriptor
 */
int writen(int fd, const void* buf, int amount) {
  Connection connection = {fd, NULL};
  return writen_any(&connection, buf, amount);
}

/**
 * Read the given amount of data from the file descriptor.
 * @param fd the file descriptor
 * @param buf buffer for data
 * @param amount size of the data to read
 * @return The amount of bytes read from the file descriptor
 */
size_t readn(int fd, void* buf, size_t amount) {
  Connection conn = { fd, NULL };
  return readn_any(&conn, buf, amount);
}

