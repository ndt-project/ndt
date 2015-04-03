/* A minimal unit testing framework. Runs a test, prints debug output, returns
 * the return value from the test.  If non-zero, then the test failed.
 * */

#ifndef SRC_UNIT_TESTING_H
#define SRC_UNIT_TESTING_H

#include <stdio.h>

/** We chose exit code 125 as a magic number to mean "test failed".  All other
 * error codes correspond to the test crashing.  If the test crashes with error
 * code 125, then the error message will be wrong. */
#define FAILURE_EXIT_CODE 125

/**
 * Asserts a condition and with an asssociated error message and subsequent
 * arguments to fprintf.  You must supply both the condition and the error
 * message, but subsequent arguments are optional. If you want to assert a
 * condition and have a nice default error message, use CHECK instead.
 *
 * @param COND The condition being asserted. The condition will be avaluated in
 *             a boolean context.
 * @param ... The message and associated data to be printed out on error.  The
 *            first argument (required) is a char* holding the message to be
 *            printed.  The char* and all subsequent arguments are passed
 *            directly to fprintf, which allows you to do things like:
 *              ASSERT(the_answer == 42, "Bad the_answer: %d", the_answer);
 *            in order to create nice error messages.
 */
#define ASSERT(COND, ...)                                              \
  do {                                                                 \
    if (!(COND)) {                                                     \
      fprintf(stderr, "Error at line %d in %s: ", __LINE__, __FILE__); \
      fprintf(stderr, __VA_ARGS__);                                    \
      fprintf(stderr, "\n");                                           \
      exit(FAILURE_EXIT_CODE);                                         \
    }                                                                  \
  } while (0)

/**
 * Asserts a condition using the text of the condition as the error message.
 *
 * @param COND The condition being asserted.
 */
#define CHECK(COND) ASSERT(COND, #COND)

/**
 * Unconditionally causes the test to fail. Useful in error-handling code and
 * for ensuring certain code paths are never taken.
 *
 * @param ... The message and arguments for fprintf indicating why the failure
 *            occurred
 */
#define FAIL(...) ASSERT(0, __VA_ARGS__)

int run_unit_test(const char* test_name, void (*test_func)());

#define RUN_TEST(FN) run_unit_test(#FN, &(FN))

#endif  // SRC_UNIT_TESTING_H
