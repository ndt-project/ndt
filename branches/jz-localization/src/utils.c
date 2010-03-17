/*
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

/*
 * Function name: check_int
 * Description: Checks if the string is a valid int number.
 * Arguments: text - the string representing number
 *            number - the pointer where decoded number will be stored
 * Returns: 0 - success,
 *          1 - value from outside the int number range,
 *          2 - not the valid int number.
 */

int
check_int(char* text, int* number)
{
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
    }
    else {
        return 2;
    }
}

/*
 * Function name: check_rint
 * Description: Checks if the string is a valid int number from the specified
 *              range.
 * Arguments: text - the string representing number
 *            number - the pointer where decoded number will be stored
 *            minVal - the minimal value restriction (inclusive)
 *            maxVal - the maximum value restriction (inclusive)
 * Returns: 0 - success,
 *          1 - value from outside the specified range,
 *          2 - not the valid int number.
 */

int
check_rint(char* text, int* number, int minVal, int maxVal)
{
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

/*
 * Function name: check_long
 * Description: Checks if the string is a valid long number.
 * Arguments: text - the string representing number
 *            number - the pointer where decoded number will be stored
 * Returns: 0 - success,
 *          1 - value from outside the long number range,
 *          2 - not the valid long number.
 */

int
check_long(char* text, long* number)
{
    char* tmp;

    assert(text != NULL);
    assert(number != NULL);

    if (((*number = strtol(text, &tmp, 10)) == LONG_MAX) || (*number == LONG_MIN)) {
        return 1;
    }
    if ((*text != '\0') && (*tmp == '\0')) {
        return 0;
    }
    else {
        return 2;
    }
}

/*
 * Function name: secs
 * Description: Returns the seconds from the beginning of the epoch.
 * Returns: The seconds from the beginning of the epoch.
 */

double
secs()
{
    struct timeval ru;
    gettimeofday(&ru, (struct timezone *)0);
    return(ru.tv_sec + ((double)ru.tv_usec)/1000000);
}

/*
 * Function name: err_sys
 * Description: Prints the perror and exits.
 * Arguments: s - the argument to the perror() function
 */

void
err_sys(char* s)
{
    perror(s);
    return -1;
}

/*
 * Function name: sndq_len
 * Description: Returns the length of the sending queue of the given
 *              file descriptor.
 * Arguments: fd - file descriptor to check
 * Returns: The length of the sending queue.
 */

int
sndq_len(int fd)
{
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

/*
 * Function name: mysleep
 * Description: Sleeps for the given amount of seconds.
 * Arguments: time - the amount of seconds to sleep for
 */

void
mysleep(double time)
{
    int rc;
    struct timeval tv;
    tv.tv_sec = (int) time;
    tv.tv_usec = (int)(time * 1000000)%1000000;

    for (;;) {
        rc = select(0, NULL, NULL, NULL, &tv);
        if ((rc == -1) && (errno == EINTR))
	    continue;
	if (rc == 0)
	    return;
    }
}
