/*
 * This file contains the functions to handle number ranges.
 *
 * Jakub S³awiñski 2006-06-12
 * jeremian@poczta.fm
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "utils.h"
#include "logging.h"
#include "strlutils.h"

typedef struct range {
  int min; /**< lower end of the range */
  int max; /**< upper end of the range */
  struct range *next; /**< pointer to the next range member */
} Range;

static Range* mrange_root;

/*
 * Function name: mrange_parse
 * Description: Parses the string argument and adds the ranges to
 *              the free pool.
 * Arguments: text - the string containing ranges
 * Returns: 0 - the string was parsed correctly
 *          1 - there was a syntax error in the string
 */

int mrange_parse(char* text) {
  char tmp[300];
  char* ptr, *sptr;
  Range* mr_ptr = NULL;

  assert(text);

  memset(tmp, 0, 300);
  if (strlen(text) > 299) {
    return 1;
  }
  // strcpy(tmp, text);
  strlcpy(tmp, text, sizeof(tmp));
  // tokenize based on a "," character.
  // An example of the string : 2003:3000,4000:5000
  ptr = strtok(tmp, ",");
  while (ptr != NULL) {  // tokens found
    if ((sptr = strchr(ptr, ':')) != NULL) {  // also found a ":" character,
      // with sptr pointing to its location
      *sptr++ = 0;
      if (strchr(sptr, ':') != NULL) {  // should not find any more range
        return 1;
      }
    } else {
      sptr = ptr;
    }
    if ((mr_ptr = malloc(sizeof(Range))) == NULL) {
      log_println(0, "FATAL: cannot allocate memory");
      return 1;
    }
    if (*ptr == 0) {
      ptr = "1";
    }
    // Check if the input string is within range and store
    // result as "minimum"
    // For ex: Is 2003 in a string like "2003:4000" within integer range ?
    // If not, free the memory allocated to store the range and return
    if (check_rint(ptr, &mr_ptr->min, 1, MAX_TCP_PORT)) {
      free(mr_ptr);
      return 1;
    }
    if (*sptr == 0) {
      sptr = MAX_TCP_PORT_STR;
    }
    // now validate if "maximum" is within range  and store
    // result as "maximum"
    if (check_rint(sptr, &mr_ptr->max, 1, MAX_TCP_PORT)) {
      free(mr_ptr);
      return 1;  // if invalid range, free allocated memory and return
    }
    mr_ptr->next = mrange_root;
    mrange_root = mr_ptr;  // ready to point to next member
    ptr = strtok(NULL, ",");
  }
  free(mr_ptr);
  return 0;
}

/**
 * Checks if a given number (passed in as string argument)
 *  is available as a valid integer in the free pool. For example,
 *  if we construct a list of valid port ranges, then this function
 *  could be used to parse through the list to check for a usable port
 *
 * @param port  string containing port
 * @returns char* containing 0 if the argument is invalid,
 *  or the port passed as parameter, if valid
 */

char*
mrange_next(char* port, size_t port_strlen) {
  int val;
  Range* ptr;

  assert(port);

  if (check_rint(port, &val, 0, MAX_TCP_PORT)) {  // check if valid
    log_println(0, "WARNING: invalid port number");
    snprintf(port, port_strlen, RESERVED_PORT);
    return port;
  }
  val++;
  while (val <= MAX_TCP_PORT) {  // Maximum port number not exceeded
    ptr = mrange_root;
    while (ptr != NULL) {  // While there is some data
      if ((val >= ptr->min) && (val <= ptr->max)) {  // check range
        // and return port if valid
        snprintf(port, port_strlen, "%d", val);
        return port;
      }
      ptr = ptr->next;
    }
    val++;
  }
  snprintf(port, port_strlen, RESERVED_PORT);
  return port;
}
