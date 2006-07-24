/*
 * This file contains the functions needed to handle network related
 * stuff.
 *
 * Jakub S³awiñski 2006-05-30
 * jeremian@poczta.fm
 */

#include <unistd.h>
#include <assert.h>
#include <string.h>

#include "network.h"
#include "logging.h"

/*
 * Function name: OpenSocket
 * Description: Creates and binds the socket.
 * Arguments: addr - the I2Addr structure, where the new socket will be stored
 *            serv - the port number
 *            options - the binding socket options
 * Returns: The socket descriptor or error code (<0).
 */

static int
OpenSocket(I2Addr addr, char* serv, int options)
{
  struct addrinfo *fai;
  struct addrinfo *ai;
  int             on;
  int             fd=-1;

  if (!(fai = I2AddrAddrInfo(addr, NULL, serv))) {
    return -2;
  }

  for (ai = fai; ai; ai = ai->ai_next) {
#ifdef AF_INET6
    if (options & OPT_IPV6_ONLY) {
      if(ai->ai_family != AF_INET6)
        continue;
    }
#endif
    
    if (options & OPT_IPV4_ONLY) {
      if(ai->ai_family != AF_INET)
        continue;
    }

    fd = socket(ai->ai_family,ai->ai_socktype,ai->ai_protocol);

    if (fd < 0) {
      continue;
    }

    on=1;
    if (setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)) != 0) {
      goto failsock;
    }

#if defined(AF_INET6) && defined(IPPROTO_IPV6) && defined(IPV6_V6ONLY)
    if ((ai->ai_family == AF_INET6) && (options & OPT_IPV6_ONLY) &&
        setsockopt(fd,IPPROTO_IPV6,IPV6_V6ONLY,&on,sizeof(on)) != 0) {
      goto failsock;
    }
#endif

    if (bind(fd,ai->ai_addr,ai->ai_addrlen) == 0) {

      if (!I2AddrSetSAddr(addr,ai->ai_addr,ai->ai_addrlen) ||
          !I2AddrSetProtocol(addr,ai->ai_protocol) ||
          !I2AddrSetSocktype(addr,ai->ai_socktype)) {
        log_println(1, "OpenSocket: Unable to set saddr in address record");
        return -1;
      }
      if (!I2AddrPort(addr)) {
        struct sockaddr_storage tmp_addr;
        socklen_t tmp_addr_len = sizeof(tmp_addr);
        I2Addr tmpAddr;
        if (getsockname(fd, (struct sockaddr*) &tmp_addr, &tmp_addr_len)) {
          return -1;
        }
        tmpAddr = I2AddrBySAddr(NULL, (struct sockaddr*) &tmp_addr, tmp_addr_len, 0, 0);
        I2AddrSetPort(addr, I2AddrPort(tmpAddr));
        I2AddrFree(tmpAddr);
      }
      if (!I2AddrSetFD(addr,fd,True)) {
        log_println(1, "OpenSocket: Unable to set file descriptor in address record");
        return -1;
      }

      break;
    }

    if (errno == EADDRINUSE) {
      return -2;
    }

failsock:
    while((close(fd) < 0) && (errno == EINTR));
  }
  
  return fd;
}

/*
 * Function name: CreateListenSocket
 * Description: Creates the I2Addr structure with the listen socket.
 * Arguments: addr - the I2Addr structure, where listen socket should
 *                   be added, or NULL, if the new structure should be
 *                   created
 *            serv - the port number
 *            options - the binding socket options
 * Returns: The I2Addr structure with the listen socket.
 */

I2Addr
CreateListenSocket(I2Addr addr, char* serv, int options)
{
  int fd = -1;

  if (addr && (I2AddrFD(addr) > -1)) {
    log_println(1, "Invalid I2Addr record - fd already specified.");
    goto error;
  }

  if ((!addr) && !(addr = I2AddrByWildcard(/* FIXME */NULL, SOCK_STREAM, serv))) {
    log_println(1, "Unable to create I2Addr record by wildcard.");
    goto error;
  }

  if (!I2AddrSetPassive(addr,True)) {
    log_println(1, "Unable to set passive mode in I2Addr record.");
    goto error;
  }
  
  fd = OpenSocket(addr, serv, options);
  
  if (fd < 0) {
    log_println(1, "Unable to open socket.");
    goto error;
  }

  if (listen(fd, NDT_BACKLOG) < 0) {
    log_println(1, "listen(%d,%d):%s", fd, NDT_BACKLOG, strerror(errno));
    goto error;
  }

  return addr;
  
error:
    I2AddrFree(addr);
    return NULL;
}

/*
 * Function name: CreateConnectSocket
 * Description: Creates the connect socket and adds it to the I2Addr
 *              structure.
 * Arguments: sockfd - the target place for the socket descriptor
 *            local_addr - the I2Addr structure with the local address
 *                         to bind the connect socket to
 *            server_addr - the I2Addr structure with the remote
 *                          server address
 *            options - the connect socket options
 * Returns: 0 - success,
 *          !0 - error code.
 */

int
CreateConnectSocket(int* sockfd, I2Addr local_addr, I2Addr server_addr, int options)
{
  struct addrinfo *fai=NULL;
  struct addrinfo *ai=NULL;

  assert(sockfd);
  assert(server_addr);

  if (!server_addr) {
    goto error;
  }

  if ((*sockfd = I2AddrFD(server_addr)) > -1) {
    return 0;
  }

  if (!(fai = I2AddrAddrInfo(server_addr, NULL, NULL))) {
    goto error;
  }

  for (ai=fai; ai; ai=ai->ai_next) {
#ifdef AF_INET6
    if (options & OPT_IPV6_ONLY) {
      if(ai->ai_family != AF_INET6)
        continue;
    }
#endif

    if (options & OPT_IPV4_ONLY) {
      if(ai->ai_family != AF_INET)
        continue;
    }

    *sockfd = socket(ai->ai_family,ai->ai_socktype,ai->ai_protocol);
    if (*sockfd < 0) {
      continue;
    }
    
    if (local_addr) {
      /* TODO: local bind of the fd */
    }
    
    if (connect(*sockfd,ai->ai_addr,ai->ai_addrlen) == 0) {
        if(I2AddrSetSAddr(server_addr,ai->ai_addr,ai->ai_addrlen) &&
                I2AddrSetSocktype(server_addr,ai->ai_socktype) &&
                I2AddrSetProtocol(server_addr,ai->ai_protocol) &&
                I2AddrSetFD(server_addr,*sockfd,True)){
          return 0;
        }
    }
    log_println(1, "I2Addr functions failed after successful connection");
    while((close(*sockfd) < 0) && (errno == EINTR));
    return 1;
  }

error:
  log_println(1, "Unable to create connect socket.");
  return -1;
}

/*
 * Function name: send_msg
 * Description: Sends the protocol message to the control socket.
 * Arguments: ctlSocket - the control socket
 *            type - the type of the message
 *            msg - the message to send
 *            len - the length of the message
 * Returns: 0 - success,
 *          !0 - error code.
 */

int
send_msg(int ctlSocket, int type, void* msg, int len)
{
  unsigned char buff[3];
  
  assert(msg);
  assert(len >= 0);

  buff[0] = type;
  buff[1] = len >> 8;
  buff[2] = len;

  if (writen(ctlSocket, buff, 3) != 3) {
    return 1;
  }
  if (writen(ctlSocket, msg, len) != len) {
    return 2;
  }
  log_println(5, ">>> send_msg: type=%d, len=%d", type, len);
  return 0;
}

/*
 * Function name: recv_msg
 * Description: Receives the protocol message from the control socket.
 * Arguments: ctlSocket - the control socket
 *            type - the target place for type of the message
 *            buf - the target place for the message body
 *            len - the target place for the length of the message
 * Returns: 0 - success
 *          !0 - error code.
 */

int
recv_msg(int ctlSocket, int* type, void* msg, int* len)
{
  unsigned char buff[3];
  int length;
  
  assert(type);
  assert(buf);
  assert(len);

  if (readn(ctlSocket, buff, 3) != 3) {
    return 1;
  }
  *type = buff[0];
  length = buff[1];
  length = (length << 8) + buff[2];
  assert(length > (*len));
  if (length > (*len)) {
    log_println(0, "recv_msg: length [%d] > *len [%d]", length, *len);
    return 2;
  }
  *len = length;
  if (readn(ctlSocket, msg, length) != length) {
    return 3;
  }
  log_println(5, "<<< recv_msg: type=%d, len=%d", *type, *len);
  return 0;
}

/*
 * Function name: writen
 * Description: Writes the given amount of data to the file descriptor.
 * Arguments: fd - the file descriptor
 *            buf - buffer with data to write
 *            amount - the size of the data
 * Returns: The amount of bytes written to the file descriptor
 */

int
writen(int fd, void* buf, int amount)
{
  int sent, n;
  char* ptr = buf;
  sent = 0;
  assert(amount > 0);
  while (sent < amount) {
    n = write(fd, ptr+sent, amount - sent);
    assert(n != 0);
    if (n != -1) {
      sent += n;
    }
    if (n == -1) {
      if (errno != EAGAIN)
        return 0;
    }
  }
  return amount;
}

/*
 * Function name: readn
 * Description: Reads the given amount of data from the file descriptor.
 * Arguments: fd - the file descriptor
 *            buf - buffer for data
 *            amount - the size of the data to read
 * Returns: The amount of bytes read from the file descriptor.
 */

int
readn(int fd, void* buf, int amount)
{
  int sent, n;
  char* ptr = buf;
  sent = 0;
  assert(amount > 0);
  while (sent < amount) {
    n = read(fd, ptr+sent, amount - sent);
    if (n != -1) {
      sent += n;
    }
    if (n == 0)
      return 0;
    if (n == -1) {
      if (errno != EAGAIN)
        return 0;
    }
  }
  return amount;
}
