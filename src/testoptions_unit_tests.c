#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "logging.h"
#include "network.h"
#include "testoptions.h"
#include "unit_testing.h"

/**
 * Runs data through initialize_tests and compares the received data with the
 * expected data.
 * @param data the data we are planning on sending
 * @param data_len how many bytes of data
 * @param expected the data we expect to see in response
 * @param expected_len how many bytes of response
 * @param test_options the options object that carries the stream status
 * @param test_suite which NDT tests are being requested
 * @param test_suite_strlen length of test_suite
 * @param expected_return_value expected return value from initialize_tests
 */
void send_data_and_compare_response(const char* data, size_t data_len,
                                    const char* expected, size_t expected_len,
                                    TestOptions* test_options, char* test_suite,
                                    size_t test_suite_strlen,
                                    int expected_return_value) {
  char end_msg[] = "THE END OF THE STREAM";
  char* received_data;
  pid_t child_pid;
  int child_exit_code;
  int sockets[2];
  int child_socket, parent_socket;
  int bytes_written;
  int return_value;
  CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
  child_socket = sockets[0];
  parent_socket = sockets[1];
  if ((child_pid = fork()) == 0) {
    bytes_written = writen(child_socket, data, data_len);
    ASSERT(bytes_written == data_len, "write wrote %d bytes, and not %d",
           bytes_written, data_len);
    if (expected_len) {
      received_data = (char*)malloc(expected_len * sizeof(char));
      CHECK(received_data != NULL);
      CHECK(expected_len == readn(child_socket, received_data, expected_len));
      CHECK(strncmp(received_data, expected, expected_len) == 0);
      free(received_data);
    }
    received_data = (char*)malloc(sizeof(end_msg));
    CHECK(received_data != NULL);
    CHECK(sizeof(end_msg) ==
          readn(child_socket, received_data, sizeof(end_msg)));
    ASSERT(strncmp(received_data, end_msg, sizeof(end_msg)) == 0,
           "Received_data (%s)", received_data);
    free(received_data);
    exit(0);
  } else {
    return_value = initialize_tests(parent_socket, test_options, test_suite,
                                    test_suite_strlen);
    ASSERT(return_value == expected_return_value,
           "initialize_tests returned %d and not %d", return_value,
           expected_return_value);
    CHECK(sizeof(end_msg) == writen(parent_socket, end_msg, sizeof(end_msg)));
    waitpid(child_pid, &child_exit_code, 0);
    CHECK(WIFEXITED(child_exit_code) && WEXITSTATUS(child_exit_code) == 0);
  }
}

/**
 * Send a MSG_LOGIN to the server, expect the "KICK OFF old clients" response.
 */
void test_initialize_MSG_LOGIN_tests() {
  // a MSG_LOGIN requesting only a meta test
  char test_suite[16] = {0};
  char data[] = {MSG_LOGIN, 0, 1, 32};
  char expected[] = {'1', '2', '3', '4', '5', '6', ' ',
                     '6', '5', '4', '3', '2', '1'};
  TestOptions test_options;
  test_options.connection_flags = 0;
  send_data_and_compare_response(data, sizeof(data), expected, sizeof(expected),
                                 &test_options, test_suite, sizeof(test_suite),
                                 32);
  CHECK(test_options.connection_flags == 0);
}

/**
 * Send a MSG_EXTENDED_LOGIN to the server, expect the "KICK OFF old clients"
 * response.
 */
void test_initialize_MSG_EXTENDED_LOGIN_tests() {
  // a MSG_EXTENDED_LOGIN requesting only a meta test
  char test_suite[16] = {0};
  char expected[] = {'1', '2', '3', '4', '5', '6', ' ',
                     '6', '5', '4', '3', '2', '1'};
  TestOptions test_options;
  // The first char of msg is TEST_META|TEST_STATUS == 32|16 == 48 == '0'
  char data[] = "XXX{ \"msg\": \"0v3.5.5\"}";
  data[0] = MSG_EXTENDED_LOGIN;
  data[1] = 0;
  data[2] = strlen(data + 3);
  test_options.connection_flags = 0;
  send_data_and_compare_response(data, sizeof(data) - 1, expected,
                                 sizeof(expected), &test_options, test_suite,
                                 sizeof(test_suite), 16 | 32);
  CHECK(test_options.connection_flags & JSON_SUPPORT);
  CHECK(!(test_options.connection_flags & WEBSOCKET_SUPPORT));
}

/**
 * Log in to the server with websockets.  The server should properly sniff that
 * the request was not a normal NDT message and then authenticate with the
 * client.
 */
void test_initialize_websocket_tests() {
  int i;
  char test_suite[16] = {0};
  TestOptions test_options;
  const char request[] =
      "GET /ndt_after_user_privacy_agreement HTTP/1.1\r\n"
      "Host: server.example.com\r\n"
      "Upgrade: websocket\r\n"
      "Connection: Upgrade\r\n"
      "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
      "Origin: http://example.com\r\n"
      "Sec-WebSocket-Protocol: ndt, superchat\r\n"
      "Sec-WebSocket-Version: 13\r\n"
      "\r\n";
  size_t request_len = sizeof(request) - 1;
  const char response[] =
      "HTTP/1.1 101 Switching Protocols\r\n"
      "Upgrade: websocket\r\n"
      "Connection: Upgrade\r\n"
      "Sec-WebSocket-Protocol: ndt\r\n"
      "Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n"
      "\r\n";
  size_t response_len = sizeof(response) - 1;
  unsigned char websocket_header[] = {0x80 | 0x1, 0x80, 'M', 'A', 'S', 'K'};
  size_t websocket_header_len = sizeof(websocket_header);
  unsigned char websocket_payload[] = "XXX{ \"msg\": \"0v3.5.5\"}";
  size_t websocket_payload_len = sizeof(websocket_payload) - 1;
  char* message_to_send;
  websocket_payload[0] = MSG_EXTENDED_LOGIN;
  websocket_payload[1] = 0;
  websocket_payload[2] = strlen((char*)&(websocket_payload[3]));
  websocket_header[1] |= websocket_payload_len;
  fprintf(stderr, "Predicted length = %d\n", websocket_header[1] & 0x7F);
  for (i = 0; i < websocket_payload_len; i++) {
    websocket_payload[i] ^= websocket_header[2 + (i % 4)];
  }
  message_to_send =
      (char*)malloc(request_len + websocket_header_len + websocket_payload_len);
  CHECK(message_to_send != NULL);
  memcpy(message_to_send, request, request_len);
  memcpy(message_to_send + request_len, websocket_header, websocket_header_len);
  memcpy(message_to_send + request_len + websocket_header_len,
         websocket_payload, websocket_payload_len);
  test_options.connection_flags = 0;
  send_data_and_compare_response(
      message_to_send,
      request_len + websocket_header_len + websocket_payload_len, response,
      response_len, &test_options, test_suite, sizeof(test_suite), 16 | 32);
  CHECK(test_options.connection_flags & JSON_SUPPORT);
  CHECK(test_options.connection_flags & WEBSOCKET_SUPPORT);
}

int main() {
  set_debuglvl(1024);
  return RUN_TEST(test_initialize_MSG_LOGIN_tests) |
         RUN_TEST(test_initialize_MSG_EXTENDED_LOGIN_tests) |
         RUN_TEST(test_initialize_websocket_tests);
}
