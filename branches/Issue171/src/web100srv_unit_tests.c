#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "unit_testing.h"

/** Runs an end-to-end test of the server and client code. */
void test_e2e() {
  pid_t server_pid;
  pid_t client_pid;
  int client_exit_code;
  char hostname[1024];
  fprintf(stderr, "Starting the server\n");
  if ((server_pid = fork()) == 0) {
    execl("./web100srv", "./web100srv", "--port=5555", NULL);
    perror("Server start error");
    exit(1);  // Children can't FAIL
  }
  // Wait until the server port (hopefully) becomes available
  sleep(1);
  // Find out the hostname.  We can't use "localhost" because then the test
  // won't go through the TCP stack and so web100 won't work.
  gethostname(hostname, (sizeof(hostname) / sizeof(char)) - 1);
  fprintf(stderr, "Starting the client, will attach to %s\n", hostname);
  if ((client_pid = fork()) == 0) {
    execl("./web100clt", "./web100clt", "--name", hostname, "--port=5555",
          NULL);
    perror("Client start error");
    exit(1);  // Children can't FAIL
  }
  waitpid(client_pid, &client_exit_code, 0);
  kill(server_pid, SIGKILL);
  ASSERT(WIFEXITED(client_exit_code) && WEXITSTATUS(client_exit_code) == 0,
         "client exit code was %d", WEXITSTATUS(client_exit_code));
}

/** Runs each test, returns non-zero to the shell if any tests fail. */
int main() {
  int success = 0;
  success |= run_test("end to end test", &test_e2e);
  return success;
}
