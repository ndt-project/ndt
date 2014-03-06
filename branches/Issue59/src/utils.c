/**
 * This file contains functions needed to handle numbers sanity checks
 * and some other utility things.
 *
 * Jakub S³awiñski 2006-06-12
 * jeremian@poczta.fm
 */

#include "../config.h"

#ifdef HAVE_LINUX_SOCKIOS_H
#include <linux/sockios.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "utils.h"
#include "strlutils.h"

/**
 * Check if the string is a valid int number.
 * @param text the string representing number
 * @param number the pointer where decoded number will be stored
 * @return 0 on success,
 *         1 - value from outside the int number range,
 *         2 - not the valid int number.
 */

int check_int(char* text, int* number) {
  char* znak;
  long tmp;

  assert(text != NULL);
  assert(number != NULL);

  if (((tmp = strtol(text, &znak, 10)) >= INT_MAX) || (tmp <= INT_MIN)) {
    return 1;
  }
  if ((*text != '\0') && (*znak == '\0')) {
    *number = tmp;
    return 0;
  } else {
    return 2;
  }
}

/**
 * Check if the string is a valid int number from the specified
 *              range.
 * @param text  string representing number
 * @param number pointer where decoded number will be stored
 * @param minVal the minimal value restriction (inclusive)
 * @param maxVal the maximum value restriction (inclusive)
 * @return 0 - success,
 *          1 - value from outside the specified range,
 *          2 - not the valid int number.
 */

int check_rint(char* text, int* number, int minVal, int maxVal) {
  int ret;

  assert(maxVal >= minVal);

  if ((ret = check_int(text, number))) {
    return ret;
  }
  if ((*number < minVal) || (*number > maxVal)) {
    return 1;
  }
  return 0;
}

/**
 * Check if the string is a valid long number.
 * @param text the string representing number
 * @param number the pointer where decoded number will be stored
 * @return  0 - success,
 *          1 - value from outside the long number range,
 *          2 - not the valid long number.
 */

int check_long(char* text, long* number) {
  char* tmp;

  assert(text != NULL);
  assert(number != NULL);

  if (((*number = strtol(text, &tmp, 10)) == LONG_MAX)
      || (*number == LONG_MIN)) {
    return 1;
  }
  if ((*text != '\0') && (*tmp == '\0')) {
    return 0;
  } else {
    return 2;
  }
}

/**
 * Return the seconds from the beginning of the epoch.
 * @return seconds from the beginning of the epoch.
 */

double secs() {
  struct timeval ru;
  gettimeofday(&ru, (struct timezone *) 0);
  return (ru.tv_sec + ((double) ru.tv_usec) / 1000000);
}

/**
 * Print the perror and exit.
 * @param s  the argument to the perror() function
 */

void err_sys(char* s) {
  perror(s);
  exit(1);
  /* return -1; */
}

/**
 * Return the length of the sending queue of the given
 *              file descriptor.
 * @param fd file descriptor to check
 * @return length of the sending queue.
 */

int sndq_len(int fd) {
#ifdef SIOCOUTQ
  int length = -1;

  if (ioctl(fd, SIOCOUTQ, &length)) {
    return -1;
  }
  return length;
#else
  return 0;
#endif
}

/**
 * Sleep for the given amount of seconds.
 * @arg time in seconds to sleep for
 */

void mysleep(double time) {
  int rc;
  struct timeval tv;
  tv.tv_sec = (int) time;
  tv.tv_usec = (int) (time * 1000000) % 1000000;

  for (;;) {
    rc = select(0, NULL, NULL, NULL, &tv);
    if ((rc == -1) && (errno == EINTR))
      continue;
    if (rc == 0)
      return;
  }
}

/**
 * Utility function to trim unwanted characters from a string.
 * @param line	String to trim
 * @param line_size	Size of line to be trimmed
 * @param output_buf Location to store trimmed line in
 * @param output_buf_size output buffer size
 * @return strlen of output string
 */
int trim(char *line, int line_size,
         char * output_buf, int output_buf_size) {
  static char whitespacearr[4]= {  '\n', ' ', '\t', '\r' };

  int i, j, k;

  for (i = j = 0; i < line_size && j < output_buf_size - 1; i++) {
    // find any matching characters among the quoted
    int is_whitespace = 0;
    for (k = 0; k < 4; k++) {
      if (line[i] == whitespacearr[k]) {
        is_whitespace = 1;
        break;
      }
    }
    if (!is_whitespace) {
      output_buf[j] = line[i];
      j++;
    }
  }
  output_buf[j] = '\0';  // null terminate
  // log_println(8,"Received=%s; len=%d; dest=%d; MSG=%s\n", line, line_size,
  //             strlen(output_buf), output_buf);
  return j - 1;
}

/**
 * Converts a IPv4-mapped IPv6 sockaddr_in6 to a sockaddr_in4
 * 
 * @param ss a sockaddr_storage
 *
 * @return if the ss represents a IPv6 mapped IPv4 address it will be converted
 * into a IPv4 sockaddr_in. Otherwise ss will remain unchanged. 
 */
void ipv4mapped_to_ipv4(struct sockaddr_storage * ss) {
#ifdef AF_INET6
  if (ss->ss_family == AF_INET6) {
    if (IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *) ss)->sin6_addr)) {
      // Ports already in the right place so just move the actual address
      ss->ss_family = AF_INET;
      ((struct sockaddr_in *) ss)->sin_addr.s_addr =
                    ((uint32_t *) &((struct sockaddr_in6 *) ss)->sin6_addr)[3];
    }
  }
#endif
}

/**
 * Get a string representation of an ip address. 
 * 
 * NOTE: This will modify addr if its an IPv4 mapped address and return
 * it as a IPv4 sockaddr.
 *
 * @param addr A sockaddr structure which contains the address
 * @param buf A buffer to fill with the ip address as a string
 * @param len The length of buf.
 */
void addr2a(struct sockaddr_storage * addr, char * buf, int len) {
  ipv4mapped_to_ipv4(addr);

  if (((struct sockaddr *)addr)->sa_family == AF_INET) {
    /* IPv4 */
    inet_ntop(AF_INET, &(((struct sockaddr_in *)addr)->sin_addr),
              buf, len);
  }
#ifdef AF_INET6
  else if (((struct sockaddr *)addr)->sa_family == AF_INET6) {
    /* IPv6 */
    inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)addr)->sin6_addr),
              buf, len);
  }
#endif
}

/**
 * Get a string representation of an port number.
 *
 * @param addr A sockaddr structure which contains the port number
 * @param buf A buffer to fill with the port number as a string
 * @param len The length of buf.
 */
void port2a(struct sockaddr_storage* addr, char* buf, int len) {
  if (((struct sockaddr*)addr)->sa_family == AF_INET) {
    snprintf(buf, len, "%hu", ntohs(((struct sockaddr_in*)addr)->sin_port));
  }
#ifdef AF_INET6
  else if (((struct sockaddr*)addr)->sa_family == AF_INET6) {
    snprintf(buf, len, "%hu", ntohs(((struct sockaddr_in6*)addr)->sin6_port));
  }
#endif
}
