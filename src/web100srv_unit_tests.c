#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "unit_testing.h"

/** Starts the web100srv process. Has a variable number of arguments, the last
 * of which should be null. */
pid_t start_server(int port, ...) {
  va_list arglist;
  int rv;
  char *args_for_exec[256] = {"./web100srv", "--port", NULL};
  int i = 2;  // the first index of args_for_exec that is not NULL
  char *arg;
  pid_t server_pid;
  siginfo_t server_status;
  char port_string[] = "32767";  // max port
  fprintf(stderr, "Starting the server\n");
  sprintf(port_string, "%d", port);
  if ((server_pid = fork()) == 0) {
    args_for_exec[i++] = port_string;
    va_start(arglist, port);
    arg = va_arg(arglist, char*);
    while (arg != NULL) {
      CHECK((args_for_exec[i++] = strdup(arg)) != NULL);
      arg = va_arg(arglist, char*);
    }
    va_end(arglist);
    args_for_exec[i++] = NULL;
    execv("./web100srv", args_for_exec);
    perror("SERVER START ERROR: SHOULD NEVER HAPPEN");
    FAIL("The server should never return - it should only be killed.");
    exit(1);
  }
  // Wait until the server port (hopefully) becomes available
  sleep(1);
  // Double-check that the server didn't crash on startup.
  server_status.si_pid = 0;
  rv = waitid(P_PID, server_pid, &server_status, WEXITED | WSTOPPED | WNOHANG);
  ASSERT(rv == 0 && server_status.si_pid == 0,
         "The server did not start successfully");
  return server_pid;
}

/** Runs an end-to-end test of the server and client code. */
void test_e2e() {
  pid_t server_pid;
  int server_exit_code;
  char hostname[1024];
  char command_line[1024];
  int port;
  srandom(time(NULL));
  port = (random() % 30000) + 1024;
  server_pid = start_server(port, NULL);
  // Find out the hostname.  We can't use "localhost" because then the test
  // won't go through the TCP stack and so web100 won't work.
  gethostname(hostname, sizeof(hostname) - 1);
  fprintf(stderr, "Starting the client, will attach to %s:%d\n", hostname,
          port);
  sprintf(command_line, "./web100clt --name=%s --port=%d", hostname, port);
  ASSERT(system(command_line) == 0, "%s did not exit with 0", command_line);
  kill(server_pid, SIGKILL);
  waitpid(server_pid, &server_exit_code, 0);
}

char* nodejs_command() {
  if (system("nodejs -e 'process.exit(0);' 2>&1 > /dev/null") == 0) {
    return "nodejs";
  } else if (system("node -e 'process.exit(0);'") == 0) {
    return "node";
  } else {
    fprintf(stderr, "Can not run node tests - node.js was not found.");
    return NULL;
  }
}

/** Runs an end-to-end test of the server over websockets. */
void test_node(int tests) {
  pid_t server_pid;
  int server_exit_code;
  char hostname[1024];
  char command_line[1024];
  int port;
  char* nodejs_name = nodejs_command();
  if (nodejs_name == NULL) return;
  srandom(time(NULL));
  port = (random() % 30000) + 1024;
  server_pid = start_server(port, NULL);
  gethostname(hostname, sizeof(hostname) - 1);
  sprintf(command_line,
          "%s node_tests/ndt_client.js --server %s --port %d --tests %d",
          nodejs_name, hostname, port, tests);
  ASSERT(system(command_line) == 0, "%s exited with non-zero exit code",
         command_line);
  kill(server_pid, SIGKILL);
  waitpid(server_pid, &server_exit_code, 0);
}

void test_run_all_tests_node() {
  test_node(2 | 4 | 32);
}

void test_node_meta_test() {
  test_node(32);
}

void test_run_two_tests_node() {
  // This test tickled a bug (now fixed) which caused the server to get stuck
  // in an infinite loop. Kept here to prevent regressions.
  test_node(2 | 4);
}

void run_ssl_test(void test_fn(int port, const char* hostname)) {
  char private_key_file[] = "/tmp/web100srv_test_key.pem-XXXXXX";
  char certificate_file[] = "/tmp/web100srv_test_cert.pem-XXXXXX";
  char create_private_key_command_line[1024];
  char create_certificate_command_line[1024];
  char hostname[1024];
  int port;
  pid_t server_pid;
  int server_exit_code;
  int err;
  // Set up certificates
  mkstemp(private_key_file);
  mkstemp(certificate_file);
  sprintf(create_private_key_command_line, "openssl genrsa -out %s",
          private_key_file);
  CHECK(system(create_private_key_command_line) == 0);
  sprintf(create_certificate_command_line, 
          "openssl req -new -x509 -key %s -out %s -days 2 -subj /C=XX/ST=State"
          "/L=Locality/O=Org/OU=Unit/CN=Name/emailAddress=test@email.address",
          private_key_file, certificate_file);
  CHECK(system(create_certificate_command_line) == 0);
  // Start the server with the right mode
  srandom(time(NULL));
  port = (random() % 30000) + 1024;
  server_pid = start_server(port, "--tls", "--private_key", private_key_file,
                            "--certificate", certificate_file, NULL);
  gethostname(hostname, sizeof(hostname) - 1);
  // Run the passed-in test
  (*test_fn)(port, hostname);
  // Free our resources
  unlink(private_key_file);
  unlink(certificate_file);
  kill(server_pid, SIGKILL);
  waitpid(server_pid, &server_exit_code, 0);
}

void make_connection(int port, const char* hostname) {
  char openssl_client_command_line[1024];
  int err;
  sprintf(openssl_client_command_line, "echo | openssl s_client -connect %s:%d",
          hostname, port);
  err = system(openssl_client_command_line);
  ASSERT(err == 0, "%s failed with error code %d", openssl_client_command_line,
         err);
}

void test_ssl_connection() {
  run_ssl_test(&make_connection);
}

void end_to_end_ssl_test(int port, const char* hostname, int tests) {
  char command_line[1024];
  char* nodejs = nodejs_command();
  int err;
  if (nodejs == NULL) return;
  sprintf(command_line, "%s node_tests/ndt_client.js --server %s --port %d "
          "--protocol wss --acceptinvalidcerts --tests %d", nodejs, hostname,
          port, tests);
  err = system(command_line);
  ASSERT(err == 0, "%s failed with error code %d", command_line, err);
}

void end_to_end_nodejs_ssl_test(int port, const char* hostname) {
  end_to_end_ssl_test(port, hostname, 2 | 4 | 32);
}

void ndt_ssl_meta_test(int port, const char* hostname) {
  end_to_end_ssl_test(port, hostname, 32);
}

void ndt_ssl_c2s_test(int port, const char* hostname) {
  end_to_end_ssl_test(port, hostname, 2);
}

void test_ssl_ndt() {
  run_ssl_test(&end_to_end_nodejs_ssl_test);
}

void test_ssl_meta_test() {
  run_ssl_test(&ndt_ssl_meta_test);
}

void test_ssl_c2s_test() {
  run_ssl_test(&ndt_ssl_c2s_test);
}

/** Runs each test, returns non-zero to the shell if any tests fail. */
int main() {
  // Formatted this way to allow developers to comment out individual
  // end-to-end tests that are not relevant to their current task.  Running the
  // full suite can take a long time, and can add unwanted latency to a
  // compile-run-debug cycle.
  return 
      RUN_TEST(test_ssl_connection) | 
      RUN_TEST(test_ssl_c2s_test) | 
      RUN_TEST(test_ssl_ndt) | 
      RUN_TEST(test_e2e) |
      RUN_TEST(test_run_all_tests_node) |
      RUN_TEST(test_run_two_tests_node) | 
      RUN_TEST(test_node_meta_test) | 
      RUN_TEST(test_ssl_meta_test) | 
      0;
}
