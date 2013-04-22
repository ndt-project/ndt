// web100srv.c-wide variable:  queue of children
// listenfd: the socket to listen for incoming requests on

typedef enum { CHILD_STARTING, CHILD_WAITING, CHILD_RUNNING, CHILD_DEAD } ndt_child_status;

typedef ndt_child_struct {
    pid_t pid;
    int control_sd;
    ndt_child_status status;
    int pipe[2];
    I2Addr addr;
    // list.h doubly-linked list
} ndtchild;

void main_loop() {
    while(1) {
        FD_ZERO(&rfd);
        FD_SET(listenfd, &rfd);
       
        sel_tv.tv_sec = 3;  // 3 seconds == WAIT_TIME_SRVR
        sel_tv.tv_usec = 0;
        log_println(3, "Waiting for new connection");
        int n = select(listenfd + 1, &rfd, NULL, NULL, &sel_tv);
        if (n > 0) {
           int client_fd;

           for (i = 0; i < RETRY_COUNT; i++) {
              int client_fd = accept(listenfd, (struct sockaddr *) &cli_addr, &clilen);
              // socket interrupted, retry
              if ((client_fd == -1) && (errno == EINTR))
                continue;  // sig child

              if (client_fd == -1)
                break;

              break;
            }

            if (client_fd == -1) {
              log_println(4, "New connection request failed: reason-%d.", errno);
            }
            else {
              ndtchild *child;
              char tmpstr[255];
              size_t tmpstrlen = sizeof(tmpstr);

              memset(tmpstr, 0, tmpstrlen);
              // get addr details based on socket info available
              I2Addr client_addr = I2AddrBySockFD(get_errhandle(), client_fd,
                                               False);
              I2AddrNodeName(client_addr, tmpstr, &tmpstrlen);
              log_println(4,
                          "New connection received from 0x%x [%s] sockfd=%d.",
                          client_addr, tmpstr, client_fd);

              child = create_new_child(client_addr);

              // send the "kick off" message
              // write(client_fd, "123456 654321", 13);
           }
        }

        process_children();
    }
}

int create_new_child(I2Addr addr) {
    int pid;
    ndtchild *child;

    ndtchild = malloc(sizeof(ndtchild));
    if (!child) {
        goto failed;
    }

    if (pipe(child->pipe) != 0) {
        goto failed_child;
    }

    pid = fork();

    if (pid == 0) {
        child_run(child);
        exit(0);
    }

    child->pid = pid;
    child->addr = addr;
    child->status = CHILD_STARTING;

    // enqueue the child

    return 0;

failed_child:
    free(child);
failed:
    return -1;
}

void process_children() {
/*
   int current_ts = time();
   int num_waiting = 0;
   int num_running = 0;
   LIST_FOREACH
     if (child->timeout <= ts) {
         // kill the child, and all associated processes
         next;
     }

     if (child->status == CHILD_STARTING) {
	 // try to read the login message from the child. if actually read,
	 // push them to the back of the queue so that the queue is in order,
	 // and set their state to CHILD_WAITING
     }

     if (child->status == CHILD_RUNNING) {
         // continue if it's not exited, and hasn't timed out
         num_running++;
         next;
     }

     if (child->status == CHILD_DEAD) {
         // kill any remaining anything with the child
         next;
     }

     if (child->status == CHILD_WAITING && slot_available) {
         dispatch_child(child);
         num_running++;
     }
     else if (child->status == CHILD_WAITING) {
         // send a message back telling them how much longer they have
         num_waiting++;
     }
*/
}

void dispatch_child(ndtchild *child) {
/*
    send message on the pipe telling them what tests to run
    update state to "RUNNING"
    set the timeout so we know when to kill them off
*/
}

void child_run(ndtchild *child) {
    int n;
    char buf[32];

    // wait for the parent to give us the go-ahead to start testing with the client
    n = read(child->pipe[0], buf, sizeof(buf));
    if ((retcode == -1) && (errno == EINTR))
        continue;

    // buf should now contain the tests the client requested

    // send the "go ahead message" to the client
    send_msg(child->control_sd, SRV_QUEUE, "0", 1);

    // send the client the version number
    // send the client the list of tests to perform
    // perform the tests
}


