#include <fcntl.h>
#include <unistd.h>
#include "logging.h"
#include "testoptions.h"
#include "testutils.h"

/** Makes the passed-in file descriptor into one that will not block.
 * @param fd the file descriptor
 * @returns non-zero if successful, zero on failure with errno as set by fcntl.
 */
int make_non_blocking(int fd) {
  int flags;
  flags = fcntl(fd, F_GETFL, NULL);
  if (flags == -1) return 0;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

/** Attempts to use the pipe to shutdown packet tracing. Will not block. After
 * this function the file descriptors of the pipe are likely set to
 * non-blocking. This should not affect any code because any shutdown messages
 * sent should be the last usage of the pipe anyway.
 * @param mon_pipe the pipe on which to send shutdown messages.
 */
void packet_trace_emergency_shutdown(int *mon_pipe) {
  // Attempt to shut down the trace, but only after making sure that all
  // attempts to write to the pipe will never block.
  if (make_non_blocking(mon_pipe[1]) && make_non_blocking(mon_pipe[0])) {
    stop_packet_trace(mon_pipe);
  } else {
    log_println(0,
                "Couldn't make pipe non-blocking (errno=%d) and so was "
                "unable to safely call stop_packet_trace",
                errno);
  }
}

/** Waits up to one second for the passed-in fd to become readable.
 * @param fd the file descriptor to wait for
 * @returns true if the fd is readable, false otherwise
 */
int wait_for_readable_fd(int fd) {
  fd_set rfd;
  struct timeval sel_tv = {0};
  FD_ZERO(&rfd);
  FD_SET(fd, &rfd);
  sel_tv.tv_sec = 1;  // Wait for up to 1 second
  return (1 == select(fd + 1, &rfd, NULL, NULL, &sel_tv));
}
