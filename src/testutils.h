/* These are helper methods which are used in multiple test_XXX_srv.c files. */

int make_non_blocking(int fd);
int wait_for_readable_fd(int fd);
void packet_trace_emergency_shutdown(int *mon_pipe);
