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

typedef struct range {
  int min;
  int max;
  struct range *next;
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

int
mrange_parse(char* text)
{
  char tmp[300];
  char* ptr, *sptr;
  Range* mr_ptr; 
  
  assert(text);

  memset(tmp, 0, 300);
  if (strlen(text) > 299) {
    return 1;
  }
  strcpy(tmp, text);
  ptr = strtok(tmp, ",");
  while (ptr != NULL) {
    if ((sptr = strchr(ptr, ':')) != NULL) {
      *sptr++ = 0;
      if (strchr(sptr, ':') != NULL) {
        return 1;
      }
    }
    else {
      sptr = ptr;
    }
    if ((mr_ptr = malloc(sizeof(Range))) == NULL) {
      log_println(0, "FATAL: cannot allocate memory");
      return 1;
    }
    if (*ptr == 0) {
      ptr = "1";
    }
    if (check_rint(ptr, &mr_ptr->min, 1, 65535)) {
      free(mr_ptr);
      return 1;
    }
    if (*sptr == 0) {
      sptr = "65535";
    }
    if (check_rint(sptr, &mr_ptr->max, 1, 65535)) {
      free(mr_ptr);
      return 1;
    }
    mr_ptr->next = mrange_root;
    mrange_root = mr_ptr;
    ptr = strtok(NULL, ",");
  }
  free(mr_ptr);
  return 0;
}

/*
 * Function name: mrange_parse
 * Description: Parses the string argument and adds the ranges to
 *              the free pool.
 * Arguments: text - the string containing ranges
 * Returns: 0 - the string was parsed correctly
 *          1 - there was a syntax error in the string
 */

char*
mrange_next(char* port)
{
  int val;
  Range* ptr; 

  assert(port);
  
  if (check_rint(port, &val, 0, 65535)) {
    log_println(0, "WARNING: invalid port number");
    sprintf(port, "0");
    return port;
  }
  val++;
  while (val < 65536) {
    ptr = mrange_root;
    while (ptr != NULL) {
      if ((val >= ptr->min) && (val <= ptr->max)) {
        sprintf(port, "%d", val);
        return port;
      }
      ptr = ptr->next;
    }
    val++;
  }
  sprintf(port, "0");
  return port;
}
