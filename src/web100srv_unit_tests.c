#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "logging.h"
#include "ndtptestconstants.h"
#include "unit_testing.h"
#include "web100srv.h"

/* On some of Measurement Lab's test servers, the value returned by gethostname
 * is incorrect. This function allows someone running tests to overwrite the
 * value returned by gethostname() by setting the environment variable
 * NDT_HOSTNAME. If that variable is set, then the name gets a copy of that
 * variable. If that variable is not set, then the name gets whatever
 * gethostname() puts in.
 * @param name The place to copy the hostname to
 * @param len The size of the buffer available
 * @return 0 on success, -1 otherwise
 */
int ndt_gethostname(char *name, size_t len) {
  char* env_value;
  env_value = getenv("NDT_HOSTNAME");
  if (env_value == NULL) {
    return gethostname(name, len);
  } else if (strlen(env_value) < len - 1) {
    strncpy(name, env_value, len);
    return 0;
  } else {
    FAIL("String not large enough to contain hostname");
    errno = E2BIG;
    return -1;
  }
}

// Creates the required certificate files according to the passed-in specs
void make_certificate_files(char *private_key_file, char *certificate_file) {
  char create_private_key_command_line[10240];
  char create_certificate_command_line[10240];
  // Set up certificates
  mkstemp(private_key_file);
  mkstemp(certificate_file);
  snprintf(create_private_key_command_line,
           sizeof(create_private_key_command_line),
           "( openssl genrsa -out %s 2>&1 ) > /dev/null", private_key_file);
  CHECK(system(create_private_key_command_line) == 0);
  snprintf(create_certificate_command_line,
           sizeof(create_certificate_command_line),
           "( openssl req -new -x509 -key %s -out %s -days 2 -subj "
           "/C=XX/ST=State/L=Locality/O=Org/OU=Unit/CN=Name"
           "/emailAddress=test@email.address 2>&1 ) > /dev/null",
           private_key_file, certificate_file);
  CHECK(system(create_certificate_command_line) == 0);
}

/** Starts the web100srv process. Either args must be NULL or the last element
 *  of **args must be NULL. */
pid_t start_server(int port, char **extra_args) {
  int extra_args_index = 0;
  char *arg;
  pid_t server_pid;
  siginfo_t server_status;
  char port_string[6];  // 32767 is the max port, so must hold 5 digits + \0
  int rv;
  char *args_for_exec[256] = {"./web100srv", "--port", NULL};
  int exec_args_index = 0;
  // Should be set to the first index of args_for_exec that is NULL
  while (args_for_exec[exec_args_index] != NULL) exec_args_index++;
  log_println(1, "Starting the server");
  if ((server_pid = fork()) == 0) {
    sprintf(port_string, "%d", port);
    args_for_exec[exec_args_index++] = port_string;
    while (extra_args != NULL && extra_args[extra_args_index] != NULL) {
      CHECK((args_for_exec[exec_args_index++] =
             strdup(extra_args[extra_args_index++])) != NULL);
    }
    args_for_exec[exec_args_index++] = NULL;
    execv("./web100srv", args_for_exec);
    perror("SERVER START ERROR: SHOULD NEVER HAPPEN");
    FAIL("The server should never return - it should only be killed.");
    exit(1);
  }
  // Wait until the server port (hopefully) becomes available. The server PID is
  // available immediately, but we need to make sure to lose the race condition
  // with the server startup process, so that the NDT port is open and ready to
  // receive traffic and we won't begin the test before the server's startup
  // routines are complete.
  sleep(1);
  // Double-check that the server didn't crash on startup.
  server_status.si_pid = 0;
  rv = waitid(P_PID, server_pid, &server_status, WEXITED | WSTOPPED | WNOHANG);
  ASSERT(rv == 0 && server_status.si_pid == 0,
         "The server did not start successfully (exit code %d)", rv);
  return server_pid;
}

// Choose random non-conflicting ports on which to run the server.
void choose_random_ports(int *port, int *tls_port) {
  srandom(time(NULL));
  *port = (random() % 30000) + 1024;
  if (tls_port != NULL) {
    do {
      *tls_port = (random() % 30000) + 1024;
    } while (*tls_port == *port);
  }
}

/** Runs an end-to-end test of the server and client code. */
void run_client(char *client_command_line_suffix, char **server_options) {
  pid_t server_pid;
  int server_exit_code;
  char hostname[1024];
  char command_line[1024];
  int port;
  choose_random_ports(&port, NULL);
  server_pid = start_server(port, server_options);
  // Find out the hostname.  We can't use "localhost" because then the test
  // won't go through the TCP stack and so web100 won't work.
  ndt_gethostname(hostname, sizeof(hostname) - 1);
  log_println(1, "Starting the client, will attach to %s:%d\n", hostname,
              port);
  sprintf(command_line, "(./web100clt --name=%s --port=%d %s 2>&1 ) > /dev/null",
          hostname, port,
          client_command_line_suffix == NULL ? "" : client_command_line_suffix);
  ASSERT(system(command_line) == 0, "%s did not exit with 0", command_line);
  kill(server_pid, SIGKILL);
  waitpid(server_pid, &server_exit_code, 0);
}

/** Run an end-to-end test using the CLI client and no special args. */
void test_e2e() { run_client(NULL, NULL); }

/** Run an end-to-end test of the multiport tests. */
void test_e2e_ext() {
  char *server_args[] = {"--c2sduration", "8000", "--c2sstreamsnum", "4",
                         "--s2cduration", "8000", "--s2cstreamsnum", "4", NULL};
  run_client("--enables2cext --enablec2sext", server_args);
}

const char *nodejs_command() {
  static const char *node = "node";
  static const char *nodejs = "nodejs";
  if (system("node -e 'process.exit(0);'") == 0) {
    return node;
  } else if (system("nodejs -e 'process.exit(0);'") == 0) {
    return nodejs;
  } else {
    log_print(0, "Cannot run node tests - node.js was not found.\n");
    return NULL;
  }
}

void run_node_client(int port, int tests, char *protocol) {
  char hostname[1024];
  char command_line[1024];
  const char *nodejs_name = nodejs_command();
  if (nodejs_name == NULL) return;
  ndt_gethostname(hostname, sizeof(hostname) - 1);
  sprintf(command_line,
          "%s node_tests/ndt_client.js --server %s --protocol %s --port %d --tests %d --acceptinvalidcerts",
          nodejs_name, hostname, protocol, port, tests);
  ASSERT(system(command_line) == 0, "%s exited with non-zero exit code",
         command_line);
}

/** Runs an end-to-end test of the server over websockets. */
void test_node(int tests) {
  pid_t server_pid;
  int server_exit_code;
  char hostname[1024];
  char command_line[1024];
  int port;
  const char *nodejs_name = nodejs_command();
  if (nodejs_name == NULL) return;
  choose_random_ports(&port, NULL);
  server_pid = start_server(port, NULL);
  run_node_client(port, tests, "ws");
  kill(server_pid, SIGKILL);
  waitpid(server_pid, &server_exit_code, 0);
}

void test_run_all_tests_node() { test_node(TEST_S2C | TEST_C2S | TEST_META); }
void test_node_meta_test() { test_node(TEST_META); }

// This test tickled a bug (now fixed) which caused the server to get stuck in
// an infinite loop. Kept here to prevent regressions.
void test_run_two_tests_node() { test_node(TEST_S2C | TEST_C2S); }

void run_ssl_test(void test_fn(int port, const char *hostname)) {
  char private_key_file[] = "/tmp/web100srv_test_key.pem-XXXXXX";
  char certificate_file[] = "/tmp/web100srv_test_cert.pem-XXXXXX";
  char hostname[1024];
  char tls_port_string[6];  // Max port # has 5 digits and then a NULL
  int port;
  int tls_port;
  pid_t server_pid;
  int server_exit_code;
  int err;
  char *server_args[] = {"--tls_port", tls_port_string, "--private_key", private_key_file, "--certificate", certificate_file, "--snaplog", "--log_dir", "/tmp", NULL};
  // Set up certificates
  make_certificate_files(private_key_file, certificate_file);
  // Start the server with the right mode
  choose_random_ports(&port, &tls_port);
  sprintf(tls_port_string, "%d", tls_port);
  server_pid = start_server(port, server_args);
  ndt_gethostname(hostname, sizeof(hostname) - 1);
  // Run the passed-in test
  (*test_fn)(tls_port, hostname);
  // Free our resources
  unlink(private_key_file);
  unlink(certificate_file);
  kill(server_pid, SIGKILL);
  waitpid(server_pid, &server_exit_code, 0);
}

void make_connection(int port, const char *hostname) {
  char openssl_client_command_line[1024];
  int err;
  sprintf(openssl_client_command_line,
          "(echo | openssl s_client -connect %s:%d 2>&1) > /dev/null", hostname, port);
  err = system(openssl_client_command_line);
  ASSERT(err == 0, "%s failed with error code %d", openssl_client_command_line,
         err);
}

void test_ssl_connection() { run_ssl_test(&make_connection); }

void end_to_end_ssl_test(int port, const char *hostname, int tests) {
  char command_line[1024];
  const char *nodejs = nodejs_command();
  int err;
  if (nodejs == NULL) return;
  sprintf(command_line,
          "%s node_tests/ndt_client.js --server %s --port %d --protocol wss "
          "--acceptinvalidcerts --tests %d",
          nodejs, hostname, port, tests);
  err = system(command_line);
  ASSERT(err == 0, "%s failed with error code %d", command_line, err);
}

void end_to_end_nodejs_ssl_test(int port, const char *hostname) {
  end_to_end_ssl_test(port, hostname, TEST_S2C | TEST_C2S | TEST_META);
}

void ndt_ssl_meta_test(int port, const char *hostname) {
  end_to_end_ssl_test(port, hostname, TEST_META);
}

void ndt_ssl_c2s_test(int port, const char *hostname) {
  end_to_end_ssl_test(port, hostname, TEST_C2S);
}

void test_ssl_ndt() { run_ssl_test(&end_to_end_nodejs_ssl_test); }
void test_ssl_meta_test() { run_ssl_test(&ndt_ssl_meta_test); }
void test_ssl_c2s_test() { run_ssl_test(&ndt_ssl_c2s_test); }

// A function in web100srv that we don't want to export, but we do want to test.
int is_child_process_alive(pid_t pid);

void test_is_child_process_alive() {
  pid_t child_pid;
  setpgid(0, 0);
  child_pid = fork();
  if (child_pid == 0) {
    // This is the child.
    sleep(10);
    exit(0);
  } else {
    // This is the parent.
    ASSERT(is_child_process_alive(child_pid), "The child should be alive");
    kill(child_pid, SIGKILL);
    sleep(1);  // Let the kill take effect
    ASSERT(!is_child_process_alive(child_pid), "The child should be dead");
  }
}

void test_is_child_process_alive_ignores_bad_pgid() {
  pid_t child_pid;
  setpgid(0, 0);
  child_pid = fork();
  if (child_pid == 0) {
    // This is the child.
    setpgid(0, 0);
    sleep(10);
    exit(0);
  } else {
    // This is the parent.
    sleep(1);  // Make sure the child has a chance to set its pgid
    CHECK(!is_child_process_alive(child_pid));  // No longer counts as a child
    kill(child_pid, SIGKILL);
  }
}

/** Runs 20 simultaneous tests, therefore exercising the queuing code. */
void test_queuing() {
  char private_key_file[] = "/tmp/web100srv_test_key.pem-XXXXXX";
  char certificate_file[] = "/tmp/web100srv_test_cert.pem-XXXXXX";
  pid_t server_pid;
  int server_exit_code;
  int client_exit_code;
  char hostname[1024];
  char command_line[1024];
  int port, tls_port;
  char tls_port_string[6];  // Maximum of 5 digits plus NULL
  int i;
  time_t start_time, current_time;
  const int num_clients = 20;
  char *server_args[] = {"--max_clients=5", "--tls_port", tls_port_string,
                         "--private_key", private_key_file,
                         "--certificate", certificate_file, "--multiple",
                         "--disable_extended_tests", NULL};
  choose_random_ports(&port, &tls_port);
  sprintf(tls_port_string, "%d", tls_port);
  make_certificate_files(private_key_file, certificate_file);
  server_pid = start_server(port, server_args);
  // Find out the hostname.  We can't use "localhost" because then the test
  // won't go through the TCP stack and so web100 won't work.
  ndt_gethostname(hostname, sizeof(hostname) - 1);
  log_print(1, "Starting %d clients, will attach to %s:%d\n", num_clients,
            hostname, port);
  time(&start_time);
  for (i = 0; i < num_clients; i++) {
    if (fork() == 0) {
      if (i % 3 == 0) {
        sprintf(
            command_line,
            "(./web100clt --name=%s --port=%d 2>&1) > /dev/null && echo %d done",
            hostname, port, i);
        ASSERT(system(command_line) == 0, "%s did not exit with 0", command_line);
      } else if (i % 3 == 1) {
        run_node_client(port, TEST_C2S | TEST_S2C | TEST_META, "ws");
      } else {
        run_node_client(tls_port, TEST_C2S | TEST_S2C | TEST_META, "wss");
      }
      exit(0);
    }
  }
  i = 0;
  while (i < num_clients) {
    if (wait(&client_exit_code) != -1) i++;
    ASSERT(WIFEXITED(client_exit_code) && WEXITSTATUS(client_exit_code) == 0,
           "client exited with non-zero error code %d",
           WEXITSTATUS(client_exit_code));
    time(&current_time);
    log_print(1, "[test_queuing] %d/%d clients finished in %g seconds\n", i,
              num_clients, (double)(current_time - start_time));
  }
  kill(server_pid, SIGKILL);
  waitpid(server_pid, &server_exit_code, 0);
}

#define RUN_LONG_TEST(TEST, TIME) run_unit_test(#TEST " - may take up to " TIME " to pass", &(TEST))

/** Runs each test, returns non-zero to the shell if any tests fail. */
int main() {
  set_debuglvl(-1);
  // Formatted this way to allow developers to comment out individual tests
  // that are not relevant to their current task.  Running the full suite can
  // take a long time and can add unwanted latency to a compile-run-debug
  // cycle.  The tests are in ascending order by expected runtime in an effort
  // to fail fast.
  return
      RUN_TEST(test_is_child_process_alive) ||
      RUN_TEST(test_is_child_process_alive_ignores_bad_pgid) ||
      RUN_TEST(test_node_meta_test) ||
      RUN_TEST(test_ssl_connection) ||
      RUN_TEST(test_ssl_meta_test) ||
      RUN_LONG_TEST(test_ssl_c2s_test, "15 seconds") ||
      RUN_LONG_TEST(test_run_all_tests_node, "30 seconds") ||
      RUN_LONG_TEST(test_ssl_ndt, "30 seconds") ||
      RUN_LONG_TEST(test_e2e, "30 seconds") ||
      //RUN_TEST(test_e2e_ext) ||
      RUN_LONG_TEST(test_run_two_tests_node, "30 seconds") ||
      RUN_LONG_TEST(test_queuing, "2 minutes") ||
      0;
}
