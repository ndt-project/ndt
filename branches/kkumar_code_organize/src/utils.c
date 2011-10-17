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
	return -1;
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
