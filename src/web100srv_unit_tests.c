#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "unit_testing.h"

/** Starts the web100srv process. */
pid_t start_server(int port, int extended) {
  pid_t server_pid;
  int server_return_value;
  char port_string[] = "32767";  // max port
  fprintf(stderr, "Starting the server\n");
  sprintf(port_string, "%d", port);
  if ((server_pid = fork()) == 0) {
	if (extended) {
	  execl("./web100srv", "./web100srv", "--port", port_string,
			  "--c2sduration", "8000", "--c2sstreamsnum", "4",
			  "--s2cduration", "8000", "--s2cstreamsnum", "4", NULL);
	} else {
      execl("./web100srv", "./web100srv", "--port", port_string, NULL);
    }
	perror("SERVER START ERROR: SHOULD NEVER HAPPEN");
    FAIL("The server should never return - it should only be killed.");
    exit(1);
  }
  // Wait until the server port (hopefully) becomes available
  sleep(1);
  return server_pid;
}

/** Runs an end-to-end test of the server and client code. */
void run_client(char* additionalOptions, int extended) {
  pid_t server_pid;
  int server_exit_code;
  char hostname[1024];
  char command_line[1024];
  int port;
  srandom(time(NULL));
  port = (random() % 30000) + 1024;
  server_pid = start_server(port, extended);
  // Find out the hostname.  We can't use "localhost" because then the test
  // won't go through the TCP stack and so web100 won't work.
  gethostname(hostname, sizeof(hostname) - 1);
  fprintf(stderr, "Starting the client, will attach to %s:%d\n", hostname,
		  port);
  sprintf(command_line, "./web100clt --name=%s --port=%d %s",
		  hostname, port, additionalOptions != NULL ? additionalOptions : "");
  ASSERT(system(command_line) == 0, "%s did not exit with 0", command_line);
  kill(server_pid, SIGKILL);
  waitpid(server_pid, &server_exit_code, 0);
}

void test_e2e() {
  run_client(NULL, 0);
}

void test_e2e_ext() {
  run_client("--enables2cext --enablec2sext", 1);
}

/** Runs an end-to-end test of the server over websockets. */
void test_javascript(int tests) {
  pid_t server_pid;
  int server_exit_code;
  char hostname[1024];
  char command_line[1024];
  int port;
  char* nodejs_name = NULL;
  if (system("nodejs -e 'process.exit(0);'") == 0) {
    nodejs_name = "nodejs";
  } else if (system("node -e 'process.exit(0);'") == 0) {
    nodejs_name = "node";
  } else {
    fprintf(stderr, "Can NOT e2e test websockets - node.js was not found.");
    return;
  }
  srandom(time(NULL));
  port = (random() % 30000) + 1024;
  server_pid = start_server(port, 0);
  gethostname(hostname, sizeof(hostname) - 1);
  sprintf(command_line, "%s node_tests/ndt_client.js %s %d %d", nodejs_name,
          hostname, port, tests);
  ASSERT(system(command_line) == 0, "%s exited with non-zero exit code",
         command_line);
  kill(server_pid, SIGKILL);
  waitpid(server_pid, &server_exit_code, 0);
}

void test_run_all_tests_javascript() {
  test_javascript(2 | 4 | 32);
}

void test_run_two_tests_javascript() {
  // This test tickles a bug which causes the server to get stuck in an
  // infinite loop.
  test_javascript(2 | 4);
}

/** Runs each test, returns non-zero to the shell if any tests fail. */
int main() {
  return RUN_TEST(test_e2e) | RUN_TEST(test_e2e_ext) |
      RUN_TEST(test_run_all_tests_javascript) |
      RUN_TEST(test_run_two_tests_javascript);
}
