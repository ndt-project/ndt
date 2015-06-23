#include "unit_testing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * The workhorse method of the test framework. Due to this framework's
 * minimality, the run_unit_test() method must be explicitly called in main()
 * for each test you want to run.  Also, there is no timeout for tests, which
 * means an infinite loop will hang the testing framework.
 *
 * @param *test_name The displayed name of the test to be run
 * @param *test_func A pointer to the void() test function
 */
int run_unit_test(const char* test_name, void (*test_func)()) {
  int test_exit_code;
  int scratch;
  pid_t child_test_pid;
  fprintf(stderr, "Running test %s...\n", test_name);
  //  Run each child test in its own subprocess to help prevent tests from
  //  interfering with one another.
  if ((child_test_pid = fork()) == 0) {
    // Run the test in the child process.
    (*test_func)();
    // The child test process didn't crash or exit(), so exit with success!
    exit(0);
  } else {
    // Wait for the child process to exit, hopefully successfully.
    waitpid(child_test_pid, &test_exit_code, 0);
    if (WIFEXITED(test_exit_code) && WEXITSTATUS(test_exit_code) == 0) {
      fprintf(stderr, " ...Success! (%s)\n", test_name);
      return 0;
    } else {
      if (WIFEXITED(test_exit_code) &&
          WEXITSTATUS(test_exit_code) == FAILURE_EXIT_CODE) {
        fprintf(stderr, " ...TEST FAILED. (%s)\n", test_name);
      } else {
        fprintf(stderr, " ...TEST CRASHED (%s, return code=%d, %s).\n",
                test_name, test_exit_code, strerror(test_exit_code));
      }
      if (!WIFEXITED(test_exit_code)) {
        kill(child_test_pid, SIGKILL);
        waitpid(child_test_pid, &scratch, 0);
      }
      // Make sure at least one of the bottom 7 bits is set.  Some systems only
      // pay attention to the bottom 7 bits of the process return code.
      return (test_exit_code & 127) ? test_exit_code : (1 | test_exit_code);
    }
  }
}
