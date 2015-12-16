#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "logging.h"
#include "network.h"
#include "unit_testing.h"
#include "websocket.h"

// Functions defined in websocket.c that we would like to unit test, but do not
// want to expose to the rest of the program.
int websocket_sha(const char* key, unsigned long len, unsigned char* dest);
int send_digest_base64(Connection* conn, const unsigned char* digest);

/* Creates a socket pair and forks a subprocess to send data in one end. After
 * reading the data from the other end, reports whether all the data was the
 * same.
 * @param raw_data_to_send The data to send down the pipe
 * @param raw_length The size of the data
 * @param expected_data The data we expect to receive in the websocket message
 * @param expected_length The length of the expected_data
 * @param expected_error The error we expect from the function.  Should be set
 *                       to zero if we expect success.
 */
void compare_sent_and_received(const unsigned char* raw_data_to_send,
                               int raw_length,
                               const unsigned char* expected_data,
                               int64_t expected_length, int expected_error) {
  unsigned char* received_data;
  int64_t bytes_read;
  int sockets[2];
  pid_t writer_pid;
  int writer_exit_code;
  int bytes_written;
  Connection child_conn = {-1, NULL}, parent_conn = {-1, NULL};
  int i;

  CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
  parent_conn.socket = sockets[0];
  child_conn.socket = sockets[1];
  if ((writer_pid = fork()) == 0) {
    bytes_written = writen_any(&child_conn, raw_data_to_send, raw_length);
    if (bytes_written == raw_length) {
      exit(0);
    } else {
      FAIL("write returned %d and not %d", bytes_written, raw_length);
    }
  } else {
    received_data = (unsigned char*)malloc(expected_length * sizeof(char));
    CHECK(received_data != NULL);
    bytes_read =
        recv_websocket_msg(&parent_conn, received_data, expected_length);
    if (expected_error) {
      ASSERT(bytes_read < 0, "Expected an error, but got success");
      bytes_read *= -1;
      ASSERT(bytes_read == expected_error,
             "Expected an error code of %d (%s), but we received %d (%s)",
             (int)bytes_read, strerror(bytes_read), expected_error,
             strerror(expected_error));
    } else {
      ASSERT(bytes_read == expected_length,
             "recv_websocket_msg returned %" PRIu64 " but we expected %" PRIu64,
             bytes_read, expected_length);
      ASSERT(bytes_read == expected_length,
             "Bad frame data length: %" PRIu64 " vs %" PRIu64, bytes_read,
             expected_length);
      for (i = 0; i < bytes_read; i++) {
        ASSERT(received_data[i] == expected_data[i],
               "received_data[%d] != expected data[%d] ('%c' != '%c')", i, i,
               received_data[i], expected_data[i]);
      }
    }
    waitpid(writer_pid, &writer_exit_code, 0);
    CHECK(WIFEXITED(writer_exit_code) && WEXITSTATUS(writer_exit_code) == 0);
    free(received_data);
  }
}

void test_recv_unmasked_msg_closes_connection() {
  const unsigned char raw_data[] = {0x82, 0x0A, 'a', 'b', 'c', 'd',
                                    'e',  '1',  '2', '3', '4', '5'};
  const unsigned char expected_data[10] = "abcde12345";
  compare_sent_and_received(raw_data, sizeof(raw_data), expected_data,
                            sizeof(expected_data), ENOLINK);
}

void test_recv_masked_msg() {
  // Actual raw data from sending 'abcde12345' using goog.net.WebSocket
  const unsigned char raw_data[] = {0x81, 0x8A, 0xA9, 0x8D, 0x85, 0x31,
                                    0xC8, 0xEF, 0xE6, 0x55, 0xCC, 0xBC,
                                    0xB7, 0x02, 0x9D, 0xB8};
  const unsigned char expected_data[10] = "abcde12345";
  compare_sent_and_received(raw_data, sizeof(raw_data), expected_data,
                            sizeof(expected_data), 0);
}

void test_recv_large_msg() {
  // Three size codepaths. Small data (< 8 bits required to express length),
  // large (8-16 bits), and jumbo (16-64 bits). This code tests large.
  unsigned char expected_data[700];
  unsigned char raw_data[8 + 700] = {0x82,  // FIN_BIT | BINARY_OPCODE
                                     0xFE,  // MASK_BIT | "large" length flag
                                     // The two bytes that make up uint16 700
                                     0x02, 0xBC,
                                     // The four byte mask
                                     0x01, 0x01, 0x01, 0x01};
  memset(expected_data, 'a', sizeof(expected_data));
  memset(&(raw_data[8]), 'a' ^ 0x01, sizeof(expected_data));
  compare_sent_and_received(raw_data, sizeof(raw_data), expected_data,
                            sizeof(expected_data), 0);
}

void test_recv_jumbo_msg() {
  // Three size codepaths. Small data (< 8 bits required to express length),
  // large (8-16 bits), and jumbo (16-64 bits). This code tests jumbo.
  unsigned char expected_data[200000] = {'a'};
  unsigned char raw_data[14 + 200000] = {
      0x82,  // FIN_BIT | BINARY_OPCODE
      0xFF,  // MASK | "jumbo" length flag
      // The eight bytes that make uint64 200000
      0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x0D, 0x40,
      // The four byte mask
      0x01, 0x01, 0x01, 0x01};
  memset(expected_data, 'a', sizeof(expected_data));
  memset(&(raw_data[14]), 'a' ^ 0x01, sizeof(expected_data));
  compare_sent_and_received(raw_data, sizeof(raw_data), expected_data,
                            sizeof(expected_data), 0);
}

// Examples in Sec 5.7 of the RFC.
const unsigned char HELLO[5] = "Hello";
void test_rfc_example_masked_hello() {
  const unsigned char raw_data[] = {0x81, 0x85, 0x37, 0xfa, 0x21, 0x3d,
                                    0x7f, 0x9f, 0x4d, 0x51, 0x58};
  compare_sent_and_received(raw_data, sizeof(raw_data), HELLO, sizeof(HELLO),
                            0);
}

void test_rfc_example_fragmented_hello() {
  // A fragmented message!
  const unsigned char raw_data[] = {
      // First packet header
      0x01,       0x83,       0x04,       0x03, 0x02, 0x01,
      // First packet contents: "Hel"
      'H' ^ 0x04, 'e' ^ 0x03, 'l' ^ 0x02,
      // Second packet header
      0x80,       0x82,       0x04,       0x03, 0x02, 0x01,
      // Second packet contents: "lo"
      'l' ^ 0x04, 'o' ^ 0x03};
  compare_sent_and_received(raw_data, sizeof(raw_data), HELLO, sizeof(HELLO),
                            0);
}

void test_rfc_example_fragmented_with_ping() {
  // A fragmented message with a ping in the middle!
  const unsigned char raw_data[] = {
      0x01, 0x83, 0x00, 0x00, 0x00, 0x00, 0x48, 0x65, 0x6c,  // "Hel"
      0x89, 0x81, 0x00, 0x00, 0x00, 0x00, 'a',               // ping "a"
      0x80, 0x82, 0x00, 0x00, 0x00, 0x00, 0x6c, 0x6f};       // "lo"
  compare_sent_and_received(raw_data, sizeof(raw_data), HELLO, sizeof(HELLO),
                            0);
}

void test_messages_too_large() {
  pid_t child_pid;
  int child_exit_code;
  int sockets[2];
  const int64_t MAX_INT64 = 0x7FFFFFFFFFFFFFFFll;
  // This pair of fragmented messages should cause the whole thing to fail.
  // They will try to make a message of size MAX_INT64 + 3.  The last frame
  // claims to be very long, but actually has no data.  This is because we
  // should fail before we try to read any data.
  const unsigned char raw_data[] = {
      // "Hello" frame, not marked as final
      0x01,       0x85,
      // 4 byte mask
      0x01,       0x02,       0x03,       0x04,
      // "Hello", with the mask applied
      'H' ^ 0x01, 'e' ^ 0x02, 'l' ^ 0x03, 'l' ^ 0x04, 'o' ^ 0x01,
      // FIN frame, "jumbo" len value, MASK bit set
      0x80,       0xFF,
      // 64 bits of int64 for the final frame length
      0xFF,       0xFF,       0xFF,       0xFF,       0xFF,
      0xFF,       0xFF,       0xFF,
      // 4 byte mask to complete the header
      0x01,       0x02,       0x03,       0x04};
  unsigned char expected[5];  // space for the Hello to be read
  int64_t err;
  char* err_str = "no error";
  Connection child_conn = {-1, NULL}, parent_conn = {-1, NULL};
  CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
  child_conn.socket = sockets[0];
  parent_conn.socket = sockets[1];
  if ((child_pid = fork()) == 0) {
    err = (int64_t)writen_any(&child_conn, raw_data, sizeof(raw_data));
    ASSERT(err == sizeof(raw_data), "writen failed (%d)", (int)err);
    exit(0);
  } else {
    err = recv_websocket_msg(&parent_conn, expected, MAX_INT64);
    if (err < 0) {
      err = -err;
      err_str = strerror(err);
    } else {
      FAIL("Bad error: %d", (int)err);
    }
    ASSERT(err == EOVERFLOW, "Error was %d (%s) and we wanted %d (%s)",
           (int)err, err_str, EOVERFLOW, strerror(EOVERFLOW));
    waitpid(child_pid, &child_exit_code, 0);
    ASSERT(WIFEXITED(child_exit_code) && WEXITSTATUS(child_exit_code) == 0,
           "Child exited with error code %d", child_exit_code);
  }
}

/**
 * Tests the receipt of a NDT message inside a websocket message.
 */
void test_recv_websocket_ndt_msg() {
  const unsigned char raw_data[] = {// Single message, 8 bytes, masked
                                    0x82, 0x88,
                                    // Masked with 0x00
                                    0x00, 0x00, 0x00, 0x00,
                                    // end websocket header, begin NDT message
                                    // NDT protocol MSG_RESULTS type
                                    0x08,
                                    // 5 bytes of content
                                    0x00, 0x05,
                                    // "HELLO"
                                    0x48, 0x65, 0x6c, 0x6c, 0x6f};
  const int expected_type = 8;
  // We expect the message to be hello.
  const char expected_contents[] = {0x48, 0x65, 0x6c, 0x6c, 0x6f};
  const int expected_len = sizeof(expected_contents);
  char received_contents[sizeof(expected_contents) * 10];
  int received_len = sizeof(received_contents);
  int received_type;
  int64_t err;
  int sockets[2];
  pid_t child_pid;
  int child_exit_code;
  Connection child_conn = {-1, NULL}, parent_conn = {-1, NULL};
  CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
  child_conn.socket = sockets[0];
  parent_conn.socket = sockets[1];
  if ((child_pid = fork()) == 0) {
    err = writen_any(&child_conn, raw_data, sizeof(raw_data));
    ASSERT(err = sizeof(raw_data), "writen failed (%d)", (int)err);
    exit(0);
  } else {
    err = recv_websocket_ndt_msg(&parent_conn, &received_type,
                                 received_contents, &received_len);
    CHECK(err == 0);
    ASSERT(received_len == sizeof(expected_contents), "%d != %zu", received_len,
           sizeof(expected_contents));
    CHECK(received_len == expected_len);
    CHECK(received_type == expected_type);
    CHECK(strncmp(expected_contents, received_contents, expected_len) == 0);
    waitpid(child_pid, &child_exit_code, 0);
    ASSERT(WIFEXITED(child_exit_code) && WEXITSTATUS(child_exit_code) == 0,
           "Child exited with error code %d", child_exit_code);
  }
}

/**
 * Sends the header to initialize the websocket, and then compares the response
 * with what was received.
 * @param header The headers of the HTTP request
 * @param response The expected response
 */
void check_websocket_handshake(const char* header, const char* response) {
  char scratch[1024];
  int sockets[2];
  int child_socket, parent_socket;
  pid_t client_pid;
  int client_exit_code;
  int bytes_written, bytes_read;
  int err;
  int i;
  Connection conn = {-1, NULL};

  CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
  child_socket = sockets[0];
  parent_socket = sockets[1];
  if ((client_pid = fork()) == 0) {
    conn.socket = child_socket;
    err = initialize_websocket_connection(&conn, 0, "ndt");
    ASSERT(err == 0, "Initialization failed with exit code %d (%s)", err,
           strerror(err));
    CHECK(writen_any(&conn, "SIGNAL", 6) == 6);
    exit(0);  // Everything was fine.
  } else {
    // The writer uses the second socket.
    conn.socket = parent_socket;
    bytes_written = writen_any(&conn, header, strlen(header));
    ASSERT(bytes_written == strlen(header), "write returned %d and not %zu",
           bytes_written, strlen(header));
    bytes_read = readn_any(&conn, scratch, strlen(response));
    ASSERT(bytes_read == strlen(response), "readn returned %d, we wanted %zu",
           bytes_read, strlen(response));
    for (i = 0; i < strlen(response); i++) {
      ASSERT(scratch[i] == response[i], "Differ at char %d (%d vs %d)\n", i,
             (int)scratch[i], (int)response[i]);
    }
    ASSERT(strncmp(scratch, response, strlen(response)) == 0,
           "response differed from what was read. We read:\n'%s' but we "
           "wanted:\n'%s'\n",
           scratch, response);
    CHECK(readn_any(&conn, scratch, 6) == 6);
    CHECK(strncmp(scratch, "SIGNAL", 6) == 0);
    waitpid(client_pid, &client_exit_code, 0);
    CHECK(WIFEXITED(client_exit_code) && WEXITSTATUS(client_exit_code) == 0);
  }
}

void test_websocket_handshake() {
  const char* header =
      "GET /ndt_protocol HTTP/1.1\r\n"
      "Host: server.example.com\r\n"
      "Upgrade: websocket\r\n"
      "Connection: Upgrade\r\n"
      "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
      "Origin: http://example.com\r\n"
      "Sec-WebSocket-Protocol: ndt, superchat\r\n"
      "Sec-WebSocket-Version: 13\r\n"
      "\r\n";
  const char* response =
      "HTTP/1.1 101 Switching Protocols\r\n"
      "Upgrade: websocket\r\n"
      "Connection: Upgrade\r\n"
      "Sec-WebSocket-Protocol: ndt\r\n"
      "Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n"
      "\r\n";
  check_websocket_handshake(header, response);
}

void test_firefox_websocket_handshake() {
  const char* header = 
      "GET /ndt_protocol HTTP/1.1\r\n"
      "Host: ndt.iupui.mlab2.nuq0t.measurement-lab.org:7000\r\n"
      "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:35.0) Gecko/20100101\r\n"
      "Firefox/35.0 Iceweasel/35.0.1\r\n"
      "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
      "Accept-Language: en,es;q=0.7,en-us;q=0.3\r\n"
      "Accept-Encoding: gzip, deflate\r\n"
      "Sec-WebSocket-Version: 13\r\n"
      "Origin: null\r\n"
      "Sec-WebSocket-Protocol: ndt\r\n"
      "Sec-WebSocket-Key: FXsRm8SyAc2WCzXCn248UQ==\r\n"
      "Connection: keep-alive, Upgrade\r\n"
      "Pragma: no-cache\r\n"
      "Cache-Control: no-cache\r\n"
      "Upgrade: websocket\r\n"
      "\r\n";
  const char* response =
      "HTTP/1.1 101 Switching Protocols\r\n"
      "Upgrade: websocket\r\n"
      "Connection: Upgrade\r\n"
      "Sec-WebSocket-Protocol: ndt\r\n"
      "Sec-WebSocket-Accept: pe2T6IrtSYYz+YUvcOLw1n/ioLc=\r\n"
      "\r\n";
  check_websocket_handshake(header, response);
}

void test_ie11_websocket_handshake() {
  const char* header = 
       "GET /ndt_protocol HTTP/1.1\r\n"
       "Origin: http://internethealthtest.org\r\n"
       "Sec-WebSocket-Protocol: ndt\r\n"
       "Sec-WebSocket-Key: 3XPlfzNozv805tiHXD6ZyA==\r\n"
       "Connection: Upgrade\r\n"
       "Upgrade: Websocket\r\n"  // Thanks IE11, for capitalizing Websocket
       "Sec-WebSocket-Version: 13\r\n"
       "User-Agent: Mozilla/5.0 (Windows NT 6.3; Trident/7.0; rv:11.0) like Gecko\r\n"
       "Host: ndt.iupui.mlab1.mia05.measurement-lab.org:3001\r\n"
       "DNT: 1\r\n"
       "Cache-Control: no-cache\r\n"
       "\r\n";
  const char* response =
      "HTTP/1.1 101 Switching Protocols\r\n"
      "Upgrade: websocket\r\n"
      "Connection: Upgrade\r\n"
      "Sec-WebSocket-Protocol: ndt\r\n"
      "Sec-WebSocket-Accept: LHroUp9DVDHErY83w6Bm0zUY9kg=\r\n"
      "\r\n";
  check_websocket_handshake(header, response);
}

void test_websocket_sha() {
  const char key[] = "dGhlIHNhbXBsZSBub25jZQ==";
  const unsigned char expected_digest[20] = {
      0xb3, 0x7a, 0x4f, 0x2c, 0xc0, 0x62, 0x4f, 0x16, 0x90, 0xf6,
      0x46, 0x06, 0xcf, 0x38, 0x59, 0x45, 0xb2, 0xbe, 0xc4, 0xea};
  unsigned char digest[20];
  CHECK(websocket_sha(key, sizeof(key) - sizeof(char), digest) == 0);
  CHECK(memcmp(digest, expected_digest, 20) == 0);
}

void check_send_digest_base64(const unsigned char* digest,
                              const char* expected) {
  pid_t child_pid;
  int child_exit_code;
  char scratch[1024];
  int sockets[2];
  Connection child_conn = {-1, NULL}, parent_conn = {-1, NULL};
  CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
  child_conn.socket = sockets[0];
  parent_conn.socket = sockets[1];
  if ((child_pid = fork()) == 0) {
    // Send the data down the pipe.
    CHECK(send_digest_base64(&child_conn, digest) == 0);
    CHECK(writen_any(&child_conn, "SIGNAL", 6) == 6);
    exit(0);
  } else {
    // Receive the data and verify that it matches expectations.
    CHECK(readn_any(&parent_conn, scratch, strlen(expected)) == strlen(expected));
    CHECK(strncmp(expected, scratch, strlen(expected)) == 0);
    CHECK(readn_any(&parent_conn, scratch, 6) == 6);
    CHECK(strncmp("SIGNAL", scratch, 6) == 0);
    // Make sure the child exited successfully
    waitpid(child_pid, &child_exit_code, 0);
    CHECK(WIFEXITED(child_exit_code) && WEXITSTATUS(child_exit_code) == 0);
  }
}

void test_send_digest_base64() {
  const unsigned char digest[20] = {0xb3, 0x7a, 0x4f, 0x2c, 0xc0, 0x62, 0x4f,
                                    0x16, 0x90, 0xf6, 0x46, 0x06, 0xcf, 0x38,
                                    0x59, 0x45, 0xb2, 0xbe, 0xc4, 0xea};
  const char expected[] = "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=";
  check_send_digest_base64(digest, expected);
}

int main() {
  set_debuglvl(1);
  return RUN_TEST(test_messages_too_large) | RUN_TEST(test_recv_jumbo_msg) |
         RUN_TEST(test_recv_large_msg) | RUN_TEST(test_recv_masked_msg) |
         RUN_TEST(test_recv_unmasked_msg_closes_connection) |
         RUN_TEST(test_recv_websocket_ndt_msg) |
         RUN_TEST(test_rfc_example_fragmented_hello) |
         RUN_TEST(test_rfc_example_fragmented_with_ping) |
         RUN_TEST(test_rfc_example_masked_hello) |
         RUN_TEST(test_send_digest_base64) |
	 RUN_TEST(test_websocket_handshake) |
         RUN_TEST(test_firefox_websocket_handshake) |
         RUN_TEST(test_ie11_websocket_handshake) |
         RUN_TEST(test_websocket_sha);
}
