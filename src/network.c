/**
 * This file contains the functions needed to handle network related
 * stuff.
 *
 * Jakub S³awiñski 2006-05-30
 * jeremian@poczta.fm
 */

#include <assert.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include "network.h"
#include "logging.h"

/**
 * Create and bind socket.
 * @param addr I2Addr structure, where the new socket will be stored
 * @param serv the port number
 * @param options the binding socket options
 * @returns The socket descriptor or error code (<0).
 *   Error codes:
 *     -1 : Unable to set socket address/port/file descriptor in address
 *          record "addr"
 *     -2 : Unable to set socket options
 */

#ifndef AF_INET6
#error This file assumes AF_INET6 is defined.
#endif

static int OpenSocket(I2Addr addr, char* serv, int options) {
  int fd = -1;
  int return_code = 0;

  struct addrinfo *fai = NULL;
  if (!(fai = I2AddrAddrInfo(addr, NULL, serv))) {
    return -2;
  }

  struct addrinfo* ai_ipv6 = NULL;
  struct addrinfo* ai_ipv4 = NULL;

  // Get references to the first IPv6 and first IPv4 addresses. If INET6 support
  // is not compiled in, ignore AF_INET6 entries.
  // TODO: build lists of all IPv6 and IPv4 entries.
  for (struct addrinfo* ai = fai; ai != NULL; ai = ai->ai_next) {
    if (ai->ai_family == AF_INET6 && ai_ipv6 == NULL)
      ai_ipv6 = ai;
    else if (ai->ai_family == AF_INET && ai_ipv4 == NULL)
      ai_ipv4 = ai;
    if (ai_ipv4 != NULL && ai_ipv6 != NULL)
      break;
  }

  // Determine which family the user would prefer, based on command line.
  int family = AF_UNSPEC;
  // options provided by user indicate V6 or V4 only
  if ((options & OPT_IPV6_ONLY) != 0)
    family = AF_INET6;
  else if ((options & OPT_IPV4_ONLY) != 0)
    family = AF_INET;

  // Prefer IPv6.
  if (family == AF_UNSPEC || family == AF_INET6) {
    fai = ai_ipv6;
    fai->ai_next = ai_ipv4;
  } else {
    fai = ai_ipv4;
  }

  // Attempt to connect to one of the chosen addresses.
  struct addrinfo* ai = NULL;
  for (ai = fai; ai; ai = ai->ai_next) {
    // create socket with obtained address domain, socket type and protocol
    fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);

    // socket create failed. Abandon further activities using this socket
    if (fd < 0)
      continue;

    // allow sockets to reuse local address while binding unless there
    // is an active listener. If unable to set this option, indicate failure
    int on = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0) {
      return_code = -2;
      goto failsock;
    }
    // end trying to set socket option to reuse local address

    if (family == AF_INET6) {
      // If we're binding to an IPv6 address and the user hasn't specified IPv6
      // only, bind to any address to allow IPv4 clients.
      if ((options & OPT_IPV6_ONLY) == 0) {
        struct sockaddr_in6* addr_in = (struct sockaddr_in6*) ai;
        addr_in->sin6_addr = in6addr_any;
      }

#if defined(IPPROTO_IPV6) && defined(IPV6_V6ONLY)
      // the IPv6 version socket option setup
      else if (
          setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) != 0) {
        return_code = -2;
        goto failsock;
      }
#endif
    // If the user has not specified only V4 and we're listening as V4, allow V6
    // clients to connect.
    } else if ((options & OPT_IPV4_ONLY) == 0) {
      struct sockaddr_in* addr_in = (struct sockaddr_in*) ai;
      addr_in->sin_addr.s_addr = INADDR_ANY;
    }

    // try to bind to address
    if (bind(fd, ai->ai_addr, ai->ai_addrlen) == 0) {  // successful
      // set values in "addr" structure
      if (!I2AddrSetSAddr(addr, ai->ai_addr, ai->ai_addrlen) ||
          !I2AddrSetProtocol(addr, ai->ai_protocol) ||
          !I2AddrSetSocktype(addr, ai->ai_socktype)) {
        log_println(1, "OpenSocket: Unable to set saddr in address record");
        return_code = -1;
        goto failsock;
      }
      // set port if not already done, else return -1
      if (!I2AddrPort(addr)) {
        struct sockaddr_storage tmp_addr;
        socklen_t tmp_addr_len = sizeof(tmp_addr);
        I2Addr tmpAddr;
        if (getsockname(fd, (struct sockaddr*) &tmp_addr,
                        &tmp_addr_len)) {
          log_println(1, "OpenSocket: Unable to getsockname in address record");
          return_code = -1;
          goto failsock;
        }
        tmpAddr = I2AddrBySAddr(
            get_errhandle(), (struct sockaddr*) &tmp_addr, tmp_addr_len, 0, 0);
        I2AddrSetPort(addr, I2AddrPort(tmpAddr));
        I2AddrFree(tmpAddr);
      }
      // save socket file descriptor
      if (!I2AddrSetFD(addr, fd, True)) {
        log_println(1, "OpenSocket: Unable to set file descriptor in address "
                    "record");
        return_code = -1;
        goto failsock;
      }
      // end setting values in "addr" structure

      break;
    }

    // Address is indicated as being in use. Display actual socket options to
    // user and return
    if (errno == EADDRINUSE) {
      /* RAC debug statemement 10/11/06 */
      socklen_t onSize = sizeof(on);
      getsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, &onSize);
      log_println(1, "bind(%d) failed: Address already in use given as the "
                  "reason, getsockopt() returned %d", fd, on);
    } else {
      log_println(1, "bind(%d) failed: %s", fd, strerror(errno));
    }
    return_code = -1;
    goto failsock;
  }

  // set meta test's address domain family to the one used to create socket
  if (fd != -1 && meta.family == 0)
    meta.family = ai->ai_family;
  return fd;

  // If opening socket failed, print error, and try to close socket
  // file-descriptor.
 failsock:
  while ((close(fd) < 0) && (errno == EINTR)) { }
  return return_code;
}

/**
 * Createsthe I2Addr structure with the listen socket.
 * @param addr  the I2Addr structure, where listen socket should
 *                   be added, or NULL, if the new structure should be
 *                   created
 * @param serv  the port number
 * @param options the binding socket options
 * @param buf_size manually set the TCP send/receive socket buffer
 * @returns I2Addr structure with the listen socket.
 */

I2Addr CreateListenSocket(I2Addr addr, char* serv, int options, int buf_size) {
  int fd = -1;
  socklen_t optlen;
  int set_size;

  if (addr && (I2AddrFD(addr) > -1)) {
    log_println(1, "Invalid I2Addr record - fd already specified.");
    goto error;
  }

  if ((!addr) &&
      !(addr = I2AddrByWildcard(get_errhandle(), SOCK_STREAM, serv))) {
    log_println(1, "Unable to create I2Addr record by wildcard.");
    goto error;
  }

  if (!I2AddrSetPassive(addr, True)) {
    log_println(1, "Unable to set passive mode in I2Addr record.");
    goto error;
  }

  // create and bind socket using arguments
  fd = OpenSocket(addr, serv, options);

  if (fd < 0) {
    log_println(1, "Unable to open socket.");
    goto error;
  }

  /* Set sock opt code from Marion Nakanson <hakansom@ohsu.edu
   *  OHSU Advanced Computing Center
   * email on 2/19/09 correctly notes that setsockops must be made before open()
   * or listen() calls are made
   */

  optlen = sizeof(set_size);
  // get send buffer size
  getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &set_size, &optlen);
  log_print(5, "\nSend buffer initialized to %d, ", set_size);

  // get receive buffer size
  getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &set_size, &optlen);
  log_println(5, "Receive buffer initialized to %d", set_size);

  // now assign buffer sizes passed as arguments
  if (buf_size > 0) {
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size));
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(buf_size));
    // print values set to help user verify
    getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &set_size, &optlen);
    log_print(5, "Changed buffer sizes: Send buffer set to %d(%d), ",
              set_size, buf_size);
    getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &set_size, &optlen);
    log_println(5, "Receive buffer set to %d(%d)", set_size, buf_size);
  }

  // listen on socket for connections, with backlog queue length = NDT_BACKLOG
  if (listen(fd, NDT_BACKLOG) < 0) {  // if listen returns value <0, then error
    log_println(1, "listen(%d,%d):%s", fd, NDT_BACKLOG, strerror(errno));
    goto error;
  }

  return addr;

  // If error, try freeing memory
 error:
  I2AddrFree(addr);
  return NULL;
}

/**
 * Create the connect socket and adds it to the I2Addr
 *              structure.
 * @param sockfd  target place for the socket descriptor
 * @param local_addr  I2Addr structure with the local address
 *                         to bind the connect socket to
 * @param server_addr  I2Addr structure with the remote
 *                          server address
 * @param options connect socket options
 * @param buf_size manually set the TCP send/receive buffer size`
 * @return   0 - success,
 *          !0 - error code.
 */

int CreateConnectSocket(int* sockfd, I2Addr local_addr, I2Addr server_addr,
                        int options, int buf_size) {
  struct addrinfo *fai = NULL;
  struct addrinfo *ai = NULL;
  struct addrinfo *lfai = NULL;
  struct addrinfo *lai = NULL;
  socklen_t optlen;
  int set_size;

  assert(sockfd);
  assert(server_addr);

  if (!server_addr) {
    log_println(1, "Invalid server address");
    goto error;
  }

  // already connected and bound
  if ((*sockfd = I2AddrFD(server_addr)) > -1) {
    return 0;
  }

  if (!(fai = I2AddrAddrInfo(server_addr, NULL, NULL))) {
    log_println(1, "Failed to get address info for server address");
    goto error;
  }

  int family = AF_UNSPEC;
  // options provided by user indicate V6 or V4 only
  if ((options & OPT_IPV6_ONLY) != 0)
    family = AF_INET6;
  else if ((options & OPT_IPV4_ONLY) != 0)
    family = AF_INET;

  for (ai = fai; ai; ai = ai->ai_next) {
    if (family != AF_UNSPEC && ai->ai_family != family) {
      log_println(1, "Skipping family %d", family);
      continue;
    }

    // create socket with obtained address domain, socket type and protocol
    *sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (*sockfd < 0) {
      // socket create failed. Abandon further activities using this socket
      log_println(1, "Failed to create %d %d %d", ai->ai_family, ai->ai_socktype, ai->ai_protocol);
      continue;
    }

    // local address has been specified. Get details and bind to this adderess
    if (local_addr) {
      printf("local_addr\n");
      int bindFailed = 1;
      if (!(lfai = I2AddrAddrInfo(local_addr, NULL, NULL)))
        continue;

      // Validate INET address family
      for (lai = lfai; lai; lai = lai->ai_next) {
        if (lai->ai_family != family)
          continue;

        // bind to local address
        if (bind((*sockfd), lai->ai_addr, lai->ai_addrlen) == 0) {
          bindFailed = 0;  // bind successful
          break; /* success */
        }
      }

      // Failed to bind. Close socket file-descriptor and move on
      if (bindFailed == 1) {
        close((*sockfd)); /* ignore this one */
        continue;
      }
    }  // end local address

    /* Set sock opt code from Marion Nakanson <hakansom@ohsu.edu
     *  OHSU Advanced Computing Center
     * email on 2/19/09 correctly notes that setsockops must be made before open()
     * or listen() calls are made
     */

    optlen = sizeof(set_size);
    // get send buffer size for logs
    getsockopt(*sockfd, SOL_SOCKET, SO_SNDBUF, &set_size, &optlen);
    log_print(5, "\nSend buffer initialized to %d, ", set_size);
    // get receive buffer size for logs
    getsockopt(*sockfd, SOL_SOCKET, SO_RCVBUF, &set_size, &optlen);
    log_println(5, "Receive buffer initialized to %d", set_size);

    // now assign buffer sizes passed as arguments
    if (buf_size > 0) {
      setsockopt(*sockfd, SOL_SOCKET, SO_SNDBUF, &buf_size,
                 sizeof(buf_size));
      setsockopt(*sockfd, SOL_SOCKET, SO_RCVBUF, &buf_size,
                 sizeof(buf_size));
      // log values for reference
      getsockopt(*sockfd, SOL_SOCKET, SO_SNDBUF, &set_size, &optlen);
      log_print(5, "Changed buffer sizes: Send buffer set to %d(%d), ",
                set_size, buf_size);
      getsockopt(*sockfd, SOL_SOCKET, SO_RCVBUF, &set_size, &optlen);
      log_println(5, "Receive buffer set to %d(%d)", set_size, buf_size);
    }

    // Connect to target socket
    if (connect(*sockfd, ai->ai_addr, ai->ai_addrlen) == 0) {
      log_println(1, "Connected!");
      // save server address values
      if (I2AddrSetSAddr(server_addr, ai->ai_addr, ai->ai_addrlen) &&
          I2AddrSetSocktype(server_addr, ai->ai_socktype) &&
          I2AddrSetProtocol(server_addr, ai->ai_protocol) &&
          I2AddrSetFD(server_addr, *sockfd, True)) {
        log_println(1, "Client socket created");
        return 0;
      }
      // unable to save
      log_println(1, "I2Addr functions failed after successful connection");
      while ((close(*sockfd) < 0) && (errno == EINTR)) { }
      return 1;
    } else {
      log_println(0, "Failed to connect: %s", strerror(errno));
      //goto error;
    }
  }

  log_println(0, "No sockets could be created that match requirements.");

 error:
  return -1;
}

/**
 * Sends the protocol message to the control socket.
 * @param ctlSocket control socket
 * @param type type of the message
 * @param msg message to send
 * @param len length of the message
 * @return 0 on success, error code otherwise
 *        Error codes:
 *        -1 - Cannot write to socket at all
 *        -2 - Cannot complete writing full message data into socket
 *        -3 - Cannot write after retries
 *
 */

int send_msg(int ctlSocket, int type, void* msg, int len) {
  unsigned char buff[3];
  int rc, i;

  assert(msg);
  assert(len >= 0);

  /*  memset(0, buff, 3); */
  // set message type and length into message itself
  buff[0] = type;
  buff[1] = len >> 8;
  buff[2] = len;

  // retry sending data 5 times
  for (i = 0; i < 5; i++) {
    // Write initial data about length and type to socket
    rc = writen(ctlSocket, buff, 3);
    if (rc == 3)  // write completed
      break;
    if (rc == 0)  // nothing written yet,
      continue;
    if (rc == -1)  // error writing to socket..cannot continue
      return -1;
  }

  // Exceeded retries, return as "failed trying to write"
  if (i == 5)
    return -3;

  // Now write the actual message
  for (i = 0; i < 5; i++) {
    rc = writen(ctlSocket, msg, len);
    // all the data has been written successfully
    if (rc == len)
      break;
    // data writing not complete, continue
    if (rc == 0)
      continue;
    if (rc == -1)  // error writing to socket, cannot continue writing data
      return -2;
  }
  if (i == 5)
    return -3;
  log_println(8, ">>> send_msg: type=%d, len=%d, msg=%s, pid=%d", type, len,
              msg, getpid());

  protolog_sendprintln(type, msg, len, getpid(), ctlSocket);

  return 0;
}

/**
 * Receive the protocol message from the control socket.
 * @param ctlSocket control socket
 * @param typetarget place for type of the message
 * @param msg target place for the message body
 * @param len  target place for the length of the message
 * @returns 0 on success, error code otherwise.
 *          Error codes:
 *          -1 : Error reading from socket
 *          -2 : No of bytes received were lesser than expected byte count
 *          -3 : No of bytes received did not match expected byte count
 */

int recv_msg(int ctlSocket, int* type, void* msg, int* len) {
  unsigned char buff[3];
  int length;

  char *msgtemp = (char*) msg;

  assert(type);
  assert(msg);
  assert(len);

  // if 3 bytes are not explicitly read, signal error
  if (readn(ctlSocket, buff, 3) != 3) {
    return -1;
  }

  // get msg type, and calculate length as sum of the next 2 bytes
  *type = buff[0];
  length = buff[1];
  length = (length << 8) + buff[2];

  // if received buffer size < length as conveyed by buffer contents, then error
  assert(length <= (*len));
  if (length > (*len)) {
    log_println(3, "recv_msg: length [%d] > *len [%d]", length, *len);
    return -2;
  }
  *len = length;
  if (readn(ctlSocket, msg, length) != length) {
    return -3;
  }
  log_println(8, "<<< recv_msg: type=%d, len=%d", *type, *len);

  protolog_rcvprintln(*type, msgtemp, *len, getpid(), ctlSocket);

  return 0;
}

/**
 * Write the given amount of data to the file descriptor.
 * @param fd the file descriptor
 * @param buf buffer with data to write
 * @param amount the size of the data
 * @return The amount of bytes written to the file descriptor
 */

int writen(int fd, void* buf, int amount) {
  int sent, n;
  char* ptr = buf;
  sent = 0;
  assert(amount >= 0);
  while (sent < amount) {
    n = write(fd, ptr + sent, amount - sent);
    if (n == -1) {
      if (errno == EINTR)  // interrupted, retry writing again
        continue;
      if (errno != EAGAIN) {  // some genuine socket write error
        log_println(6,
                    "writen() Error! write(%d) failed with err='%s(%d) pic=%d'",
                    fd, strerror(errno), errno, getpid());
        return -1;
      }
    }
    assert(n != 0);
    if (n != -1) {  // success writing "n" bytes. Increment total bytes written
      sent += n;
    }
  }
  return sent;
}

/**
 * Read the given amount of data from the file descriptor.
 * @param fd the file descriptor
 * @param buf buffer for data
 * @param amount size of the data to read
 * @return The amount of bytes read from the file descriptor
 */

int readn(int fd, void* buf, int amount) {
  int received = 0, n, rc;
  char* ptr = buf;
  struct timeval sel_tv;
  fd_set rfd;

  assert(amount >= 0);

  FD_ZERO(&rfd);  // initialize with zeroes
  FD_SET(fd, &rfd);
  sel_tv.tv_sec = 600;
  sel_tv.tv_usec = 0;

  /* modified readn() routine 11/26/09 - RAC
   * addedd in select() call, to timeout if no read occurs after 10 seconds of waiting.
   * This should fix a bug where the server hangs at it looks like it's in this read
   * state.  The select() should check to see if there is anything to read on this socket.
   * if not, and the 3 second timer goes off, exit out and clean up.
   */
  while (received < amount) {
    // check if fd+1 socket is ready to be read
    rc = select(fd + 1, &rfd, NULL, NULL, &sel_tv);
    if (rc == 0) {
      /* A timeout occurred, nothing to read from socket after 3 seconds */
      log_println(6, "readn() routine timeout occurred, return error signal "
                  "and kill off child");
      return received;
    }
    if ((rc == -1) && (errno == EINTR)) /* a signal was processed, ignore it */
      continue;
    n = read(fd, ptr + received, amount - received);
    if (n == -1) {  // error
      if (errno == EINTR)  // interrupted , try reading again
        continue;
      if (errno != EAGAIN)  // genuine socket read error, return
        return -errno;
    }
    if (n != -1) {  // if no errors reading, increment data byte count
      received += n;
    }
    if (n == 0)
      return 0;
  }
  return received;
}
