#include <errno.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "logging.h"
#include "network.h"
#include "third_party/safe_iop.h"
#include "websocket.h"

#define OPCODE_CONTINUE 0x0
#define OPCODE_TEXT 0x1
#define OPCODE_BINARY 0x2
#define OPCODE_CLOSE 0x8
#define OPCODE_PING 0x9
#define OPCODE_PONG 0xA

#define FIN_BIT 0x80
#define MASK_BIT 0x80

// The length of the buffer required to hold a SHA1 digest.
#define SHA_HASH_SIZE 20

// The length of the buffer that will hold the base64 encoded sha digest.
#define BASE64_SHA_DIGEST_LENGTH 25

// The HTTP header specifying the websocket sub-protocol.
const static char WS_PROTOCOL[] = "Sec-WebSocket-Protocol: ";

/**
 * Send a close packet. We don't care whether it gets there or not, so we don't
 * check error conditions.
 * @param conn The Connection to send the close on
 */
void websocket_close_response(Connection* conn) {
  unsigned char close_packet[2] = {FIN_BIT | OPCODE_CLOSE, 0x00};
  writen_any(conn, close_packet, 2);
}

/**
 * Respond to a ping message.
 * @param conn The Connection to respond on
 * @param len The length of the data that must be read
 * @param mask true if the data is masked
 * @param masking_key the mask to apply
 * @returns 0 on success, or an error code.
 */
int websocket_ping_response(Connection* conn, uint64_t len, int mask,
                            unsigned char masking_key[4]) {
  unsigned char scratch[8] = {0};
  int i;
  // Immediately respond with a PONG containing the same data, but
  // unmasked.
  scratch[0] = FIN_BIT | OPCODE_PONG;
  if (writen_any(conn, scratch, 1) != 1) return -EIO;
  // Write the length
  if (len < 126) {
    scratch[0] = len & 0x7F;
    if (writen_any(conn, scratch, 1) != 1) return -EIO;
  } else if (len < (1 << 16)) {
    scratch[0] = 126;
    if (writen_any(conn, scratch, 1) != 1) return -EIO;
    scratch[0] = (len >> 8) & 0xFF;
    scratch[1] = len & 0xFF;
    if (writen_any(conn, scratch, 2) != 2) return -EIO;
  } else {
    scratch[0] = 127;
    if (writen_any(conn, scratch, 1) != 1) return -EIO;
    scratch[0] = (len >> 56) & 0xFF;
    scratch[1] = (len >> 48) & 0xFF;
    scratch[2] = (len >> 40) & 0xFF;
    scratch[3] = (len >> 32) & 0xFF;
    scratch[4] = (len >> 24) & 0xFF;
    scratch[5] = (len >> 16) & 0xFF;
    scratch[6] = (len >> 8) & 0xFF;
    scratch[7] = len & 0xFF;
    if (writen_any(conn, scratch, 8) != 8) return -EIO;
  }
  // Write the unmasked data.
  for (i = 0; i < len; i++) {
    if (readn_any(conn, scratch, 1) != 1) return -EIO;
    if (mask) scratch[0] ^= masking_key[i % 4];
    if (writen_any(conn, scratch, 1) != 1) return -EIO;
  }
  return 0;
}

/**
 * Receive a websocket message.  Receive the data into memory and unmask it.
 * Websockets are defined in RFC 6455.
 * @param conn the connection on which the websocket data is arriving
 * @param data the location to which data will be written. In the case of
 *             failure it may or may not be partially filled in. It should be a
 *             memory segment of at least max_len bytes.
 * @param max_len the maximum number of bytes we are willing to accept.  We are
 *                stricter than the standard and refuse to accept any packet
 *                that is larger than 2^62 bytes.  That still allows individual
 *                frames to be 9 exabytes in size, which should be sufficient.
 * @returns number of bytes read on success, error code otherwise.  0 means
 *          that we successfully read a message of length zero.
 */
int64_t recv_websocket_msg(Connection* conn, void* data, int64_t max_len) {
  unsigned char scratch[8];  // 8 bytes of scratch space
  int mask;
  unsigned char fin = 0;
  unsigned char opcode = 0;
  uint64_t len = 0ULL;
  unsigned char masking_key[4] = {0};
  uint64_t i;
  uint64_t current_offset = 0ULL;
  uint64_t next_offset;
  int first_frame = 1;
  size_t extra_len_bytes = 0;
  size_t bytes_read;
  if (max_len < 0) return -EINVAL;
  // Read frames until you find a FIN frame ending the message.  The first
  // frame may be (and in many cases probably is) the final frame.
  // opcodes arrive only on the first frame, except for the control opcodes
  // of CLOSE, PING, and PONG, which may be randomly interspersed with data
  // frames.
  while (1) {
    // Framing protocol for websockets (from the RFC)
    //  0                   1                   2                   3
    //  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    // +-+-+-+-+-------+-+-------------+-------------------------------+
    // |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
    // |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
    // |N|V|V|V|       |S|             |   (if payload len==126/127)   |
    // | |1|2|3|       |K|             |                               |
    // +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
    // |     Extended payload length continued, if payload len == 127  |
    // + - - - - - - - - - - - - - - - +-------------------------------+
    // |                               |Masking-key, if MASK set to 1  |
    // +-------------------------------+-------------------------------+
    // | Masking-key (continued)       |          Payload Data         |
    // +-------------------------------- - - - - - - - - - - - - - - - +
    // :                     Payload Data continued ...                :
    // + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
    // |                     Payload Data continued ...                |
    // +---------------------------------------------------------------+
    // Read the header all the way up to the beginning of Payload Data
    // First read the 2-byte required header
    if ((bytes_read = readn_any(conn, scratch, 2)) != 2) {
      log_println(1, "Failed to read the 2 byte websocket header (%u)", bytes_read);
      return -EIO;
    }
    fin = scratch[0] & FIN_BIT;
    opcode = scratch[0] & 0x0F;
    mask = scratch[1] & MASK_BIT;
    // Length is either 7 bits, 16 bits, or 64 bits.
    len = scratch[1] & 0x7F;
    // Check for the signal values of len
    if (len == 126ULL) {
      extra_len_bytes = 2;
    } else if (len == 127ULL) {
      extra_len_bytes = 8;
    }
    // Read the extra length bytes, if required.
    if (extra_len_bytes > 0) {
      if ((bytes_read = readn_any(conn, scratch, extra_len_bytes)) != extra_len_bytes) {
        log_println(1, "Failed to read the %u extra length bytes (%u)", extra_len_bytes, bytes_read);
        return -EIO;
      }
      len = 0ULL;
      for (i = 0; i < extra_len_bytes; i++) {
        len = (len << 8) + scratch[i];
      }
    }
    // Make sure that integer operations will not overflow.
    if (sop_addx(NULL, sop_u64(len), sop_u64(current_offset)))
      next_offset = len + current_offset;
    else
      return -EOVERFLOW;
    // Make sure the message will fit in the provided memory
    if (next_offset > max_len) return -EMSGSIZE;
    // Read the 4 byte mask, if required
    if (mask) {
      if (readn_any(conn, masking_key, 4) != 4) return -EIO;
    } else {
      // According to RFC 6455 Sec. 5.1. "The server MUST close the connection
      // upon receiving a frame that is not masked."
      websocket_close_response(conn);
      return -ENOLINK;
    }
    // Do different things, depending on the received opcode.
    if (opcode == OPCODE_CLOSE) {
      // Immediately respond with a CLOSE.  Socket should be closed immediately
      // after this. Nothing more can be read.
      websocket_close_response(conn);
      return -ENOLINK;
    } else if (opcode == OPCODE_PING) {
      if (websocket_ping_response(conn, len, mask, masking_key) != 0)
        return -EIO;
    } else if (opcode == OPCODE_PONG) {
      // Read the PONG. Ignore it.
      for (i = 0; i < len; i++) {
        if (readn_any(conn, scratch, 1) != 1) return -EIO;
      }
    } else if ((first_frame &&
                (opcode == OPCODE_TEXT || opcode == OPCODE_BINARY)) ||
               (!first_frame && opcode == OPCODE_CONTINUE)) {
      // opcodes only apply to the first frame of a fragmented message.
      // Make sure it is safe to cast len to a size_t
      if (!sop_addx(NULL, sop_szt(0), sop_u64(len))) return -EOVERFLOW;
      // Read the frame data into memory
      if (readn_any(conn, &(((char*)data)[current_offset]), (size_t)len) !=
          len)
        return -EIO;
      // Unmask the data, if required
      if (mask) {
        for (i = 0; i < len; i++) {
          ((unsigned char*)data)[current_offset + i] ^= masking_key[i % 4];
        }
      }
      // Increment our state
      current_offset = next_offset;
      first_frame = 0;
      if (fin) break;
    } else {
      // No opcode and frame combination matched up. This should never occur if
      // the client is using a correct websocket libary.
      return -EINVAL;
    }
  }
  if (sop_addx(NULL, sop_s64(0), sop_u64(current_offset)))
    return (int64_t)current_offset;
  else
    return -EOVERFLOW;
}

/**
 * Receives an NDT message sent over websockets. Arguments are modeled after
 * recv_msg in network.h.
 *
 * @param conn The Connection on which to listen
 * @param msg_type An outparam for the type of the message
 * @param msg_value Where to put the contents of the message
 * @param msg_len Both an outparam for the message length stored in msg_value,
 *                and, when the function is called, the maximum message length
 *                that can be stored in msg_value.
 */
int64_t recv_websocket_ndt_msg(Connection* conn, int* msg_type, char* msg_value,
                               int* msg_len) {
  const unsigned int HEADER_SIZE = 3;
  int64_t bytes;
  char* message;
  message = (char*)malloc(sizeof(char) * (*msg_len + HEADER_SIZE));
  if (message == NULL) return -ENOMEM;
  bytes = recv_websocket_msg(conn, message, *msg_len + HEADER_SIZE);
  if (bytes < 0) return bytes;
  if (bytes < 3) return -EBADMSG;
  *msg_type = message[0];
  *msg_len = (message[1] << 8) + message[2];
  if (*msg_len + HEADER_SIZE > bytes) {
    free(message);
    return -EBADMSG;
  }
  strncpy(msg_value, &(message[HEADER_SIZE]), *msg_len);
  free(message);
  return 0;
}

/**
 * Reads a line from the Connection, up to maxlen long. Returns the length
 * of the line on success, a negative number on failure. On success the string
 * in dest will be null terminated.
 * @param conn The connection to read from
 * @param dest The memory to put the read values
 * @param max_len The max number of allowed characters to read
 * @returns The number of characters in the line (not counting the terminating
 *          NULL), or an error code.
 */
int ws_readline(Connection* conn, char* dest, unsigned int max_len) {
  unsigned int count = 0;
  for (count = 0; count < max_len; count++) {
    if (readn_any(conn, &(dest[count]), 1) != 1) return -EIO;
    if (dest[count] == '\n') {
      // Be robust to UNIX and DOS newlines
      if (count > 0 && dest[count - 1] == '\r') {
        // null-terminate the string
        dest[count - 1] = '\0';
        return count - 1;
      } else {
        // null-terminate the string
        dest[count] = '\0';
        return count;
      }
    }
  }
  return -EMSGSIZE;
}

/**
 * Calculate the sha1 of the key concatenated with the special websocket
 * protocol string.
 * @param key The key string received from the client
 * @param len The length of the key in bytes
 * @param dest a pointer to a 20 byte array where the resulting message digest
 *             will be written.
 * @returns 0 on success, an error code otherwise.
 */
int websocket_sha(const char* key, unsigned int key_len, unsigned char* dest) {
  const static char WS_SHA_STRING[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  SHA_CTX context;
  if (SHA1_Init(&context) && SHA1_Update(&context, key, key_len) &&
      SHA1_Update(&context, WS_SHA_STRING, strlen(WS_SHA_STRING)) &&
      SHA1_Final(dest, &context)) {
    return 0;
  } else {
    return EINVAL;
  }
}

/**
 * Send the 20 byte message digest base64encoded.
 * @param conn the connection to which we should write.
 * @param message_digest the 20 byte digest to encode and send.
 * @returns 0 on success, error code otherwise
 */
int send_digest_base64(Connection* conn, const unsigned char* message_digest) {
  BIO* mem;
  BIO* b64;
  BUF_MEM* buffer;
  int error = 0;
  // Allocate all the BIO objects.  We use a mem bio object instead of
  // attaching directly to the conn->ssl or conn->socket because BIO_push and
  // BIO_free are not side-effect free, even in the presence of BIO_NOCLOSE
  mem = BIO_new(BIO_s_mem());
  b64 = BIO_new(BIO_f_base64());
  // If the allocations succeeded
  if (b64 && mem) {
    // Make sure b64 doesn't put newlines all over the place
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    // Connect the BIO objects to each other in series
    BIO_push(b64, mem);
    // Write the data
    if (BIO_write(b64, message_digest, SHA_HASH_SIZE) != SHA_HASH_SIZE)
      error = EIO;
    BIO_flush(b64);
    BIO_get_mem_ptr(mem, &buffer);
    error = writen_any(conn, buffer->data, buffer->length);
    if (error < 0) {
      error = -error;
    } else {
      error = 0;
    }
  } else {
    error = ENOMEM;
  }
  // Cleanup
  if (b64) BIO_free(b64);
  if (mem) BIO_free(mem);
  return error;
}

/**
 * Reads the websocket header and determines whether it is well-formed.
 * @param socket_fd The socket on which the connection is happening.
 * @param skip_bytes How many bytes of the initial handshake have already
 *                   been read and validated?
 * @param expected_protocol The protocol name we are expecting from the client.
  *                         May be NULL or "" if no protocol name is desired.
 * @param key An outparam to hold the websocket key - must point to a string
 *            at least 25 chars long.
 * @returns 0 on success, error code otherwise.
 */
int read_websocket_header(Connection* conn, unsigned int skip_bytes,
                          char* expected_protocol, char* key) {
  // Neither of these limits is in the HTTP spec, but they are the limits
  // enforced by the Apache defaults. In particular, it is recommended that
  // there not be more than 8K of headers total, so these limits are generous.
  // http://stackoverflow.com/questions/686217/maximum-on-http-header-values
  const static unsigned int MAX_HEADER_COUNT = 1024;
  char line[8192];  // Max length for a single line
  // String constants used when making a websocket connection.
  const static char UPGRADE_HEADER[] = "Upgrade: websocket";
  const static char CONNECTION_HEADER[] = "Connection: ";
  const static char CONNECTION_HEADER_VALUE[] = " Upgrade";
  const static char VERSION_HEADER[] = "Sec-WebSocket-Version: 13";
  const static char WS_KEY[] = "Sec-WebSocket-Key: ";
  const static char FIRST_LINE[] = "GET /ndt_protocol HTTP/1.1";
  // Variables that actually vary
  const char* suffix;
  int validated_connection = 0;
  int validated_upgrade = 0;
  int validated_version = 0;
  int validated_protocol = 0;
  int i;
  // You can only fastforward into the very first line
  if (skip_bytes >= strlen(FIRST_LINE)) return EINVAL;
  // Read the first line.
  if (ws_readline(conn, line, sizeof(line)) < 0) return EIO;
  if (strcmp(line, &(FIRST_LINE[skip_bytes])) != 0) return EINVAL;
  // HTTP headers end with a blank line
  // We only save the header values we care about. The headers we only need to
  // check (i.e. have no data we need for later) are handled as they arrive.
  if (ws_readline(conn, line, sizeof(line)) < 0) return EIO;
  for (i = 0; i < MAX_HEADER_COUNT; i++) {
    if (strcmp(line, UPGRADE_HEADER) == 0) {
      validated_upgrade = 1;
    } else if (strncmp(line, CONNECTION_HEADER, strlen(CONNECTION_HEADER)) == 0) {
      if (strstr(line, CONNECTION_HEADER_VALUE) != NULL) {
        validated_connection = 1;
      }
    } else if (strcmp(line, VERSION_HEADER) == 0) {
      validated_version = 1;
    } else if (strncmp(line, WS_PROTOCOL, strlen(WS_PROTOCOL)) == 0) {
      if (expected_protocol == NULL || strcmp(expected_protocol, "") == 0) {
        validated_protocol = 1;
      } else {
        suffix = &(line[strlen(WS_PROTOCOL)]);
        validated_protocol = (strstr(suffix, expected_protocol) != NULL);
      }
    } else if (strncmp(line, WS_KEY, strlen(WS_KEY)) == 0) {
      suffix = &(line[strlen(WS_KEY)]);
      if (strlen(suffix) + 1 != BASE64_SHA_DIGEST_LENGTH) return EBADMSG;
      strncpy(key, suffix, BASE64_SHA_DIGEST_LENGTH);
    }
    if (ws_readline(conn, line, sizeof(line)) < 0) return EIO;
    if (strcmp(line, "") == 0) break;
  }
  if (strcmp(line, "") == 0 && validated_connection && validated_upgrade &&
      validated_version && validated_protocol) {
    return 0;
  } else {
    return EBADMSG;
  }
}

/** Write a websocket header in response to a well-formed request.
 * @param conn The connection we are using
 * @param protocol The protocol for the websocket
 * @param key The key for the websocket connection
 */
int write_websocket_header(Connection* conn, char* protocol, char* key) {
  const static char WS_RESPONSE[] =
      "HTTP/1.1 101 Switching Protocols\r\n"
      "Upgrade: websocket\r\n"
      "Connection: Upgrade\r\n";
  const static char WS_ACCEPT[] = "Sec-WebSocket-Accept: ";
  unsigned char message_digest[SHA_HASH_SIZE];
  const size_t WS_RESPONSE_LEN = sizeof(WS_RESPONSE) - sizeof(char);
  const size_t WS_ACCEPT_LEN = sizeof(WS_ACCEPT) - sizeof(char);
  const size_t WS_PROTOCOL_LEN = sizeof(WS_PROTOCOL) - sizeof(char);
  // Write the constant header
  if (writen_any(conn, WS_RESPONSE, WS_RESPONSE_LEN) != WS_RESPONSE_LEN) {
    return EIO;
  }
  // Agree to the protocol (if necessary)
  if (protocol != NULL && strcmp(protocol, "") != 0) {
    if (writen_any(conn, WS_PROTOCOL, WS_PROTOCOL_LEN) != WS_PROTOCOL_LEN) {
      return EIO;
    }
    if (writen_any(conn, protocol, strlen(protocol)) != strlen(protocol)) {
      return EIO;
    }
    // End the WS_PROTOCOL header
    if (writen_any(conn, "\r\n", 2) != 2) return EIO;
  }
  // Send the WS_ACCEPT header and value (requires sha1 and base64enc)
  if (writen_any(conn, WS_ACCEPT, WS_ACCEPT_LEN) != WS_ACCEPT_LEN) {
    return EIO;
  }
  websocket_sha(key, strlen(key), message_digest);
  if (send_digest_base64(conn, message_digest) != 0) return EIO;
  // End the WS_ACCEPT header
  if (writen_any(conn, "\r\n", 2) != 2) return EIO;
  // A blank line signals the end of the headers.
  if (writen_any(conn, "\r\n", 2) != 2) return EIO;
  return 0;
}

/**
 * Sets up a websocket connection.  There are times when we don't determine it's
 * a websocket connection until a few bytes have been read, so this function
 * supports specifying the number of bytes that have already been read, and
 * will try to fast-forward to that point in the handshake.
 * @param conn The Connection on which the handshake is happening.
 * @param skip_bytes How many bytes of the initial handshake have already
 *                   been read and validated?
 * @param expected_protocol The protocol name we are expecting from the client.
 *                          May be NULL or "" if no protocol name is desired.
 * @returns 0 on success, error code otherwise.
 */
int initialize_websocket_connection(Connection* conn, unsigned int skip_bytes,
                                    char* expected_protocol) {
  // Keys are base64 encoded 16 byte values.  So we need to allocate enough
  // memory on the stack to hold the entire key and null terminator.
  char key[BASE64_SHA_DIGEST_LENGTH] = {0};
  int err;
  err = read_websocket_header(conn, skip_bytes, expected_protocol, key);
  if (err != 0) return err;
  // We have received a well-formed header. We should respond with a
  // well-formed response of our own.
  err = write_websocket_header(conn, expected_protocol, key);
  if (err != 0) return err;
  return 0;
}

/**
 * Sends a websocket frame from the server to a client.  Messages sent from a
 * server MUST NOT be masked, according to the RFC, and so this function does
 * not support masking.  The arguments are modeled after send_msg.
 * @param conn The Connection on which tto send data
 * @param type The NDT message type
 * @param msg A pointer to the data to be sent
 * @param len The number of bytes to be sent
 */
int send_websocket_msg(Connection* conn, int type, const void* msg, uint64_t len) {
  int websocket_hdr_len;
  int err;
  int i;
  uint64_t websocket_msg_len;
  char websocket_msg[10];  // Max websocket header size if there is no mask
  if (sop_addx(NULL, sop_u64(len), sop_u64(3))) {
    websocket_msg_len = len + 3;  // NDT header is always 3 bytes
  } else {
    return EINVAL;
  }
  // Set up the websocket header
  websocket_msg[0] = (char)0x82;  // FIN_BIT | BINARY_OPCODE
  if (websocket_msg_len < 126) {
    websocket_hdr_len = 2;
    if (websocket_msg == NULL) return ENOMEM;
    // 7 bits for the length
    websocket_msg[1] = (char)(websocket_msg_len & 0x7F);
  } else if (websocket_msg_len < (1 << 16)) {
    websocket_hdr_len = 4;
    if (websocket_msg == NULL) return ENOMEM;
    // Signal value for "2 byte length"
    websocket_msg[1] = 126;
    // 16 bits for the length
    websocket_msg[2] = (websocket_msg_len >> 8) & 0xFF;
    websocket_msg[3] = websocket_msg_len & 0xFF;
  } else {
    websocket_hdr_len = 10;
    if (websocket_msg == NULL) return ENOMEM;
    // Signal value for "8 byte length"
    websocket_msg[1] = 127;
    // 64 bits for the length
    for (i = 0; i < 8; ++i) {
      websocket_msg[i + 2] = (websocket_msg_len >> ((7 - i) * 8)) & 0xFF;
    }
  }
  // Send the header
  if (writen_any(conn, websocket_msg, websocket_hdr_len) !=
      websocket_hdr_len) {
    return EIO;
  }
  // Websocket server -> client messages are not masked, so we can just send it
  // like it's an NDT message.
  err = send_msg_any(conn, type, msg, (int)len);
  return err;
}
