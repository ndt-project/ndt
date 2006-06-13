/*
 * This file contains the functions needed to handle network related
 * stuff.
 *
 * Jakub S³awiñski 2006-05-30
 * jeremian@poczta.fm
 */

#include <unistd.h>
#include <assert.h>

#include "network.h"

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
        /* TODO: log the error */
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
        /* TODO: log the error */
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

I2Addr
CreateListenSocket(I2Addr addr, char* serv, int options)
{
  int fd = -1;

  if (addr && (I2AddrFD(addr) > -1)) {
    /* TODO: log the error */
    goto error;
  }

  if ((!addr) && !(addr = I2AddrByWildcard(/* FIXME */NULL, SOCK_STREAM, serv))) {
    /* TODO: log the error */
    goto error;
  }

  if (!I2AddrSetPassive(addr,True)) {
    /* TODO: log the error */
    goto error;
  }
  
  fd = OpenSocket(addr, serv, options);
  
  if (fd < 0) {
    /* TODO: log the error */
    goto error;
  }

  if (listen(fd, NDT_BACKLOG) < 0) {
    /* TODO: log the error */
    goto error;
  }

  return addr;
  
error:
    I2AddrFree(addr);
    return NULL;
}

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
    /* TODO: log: "I2Addr functions failed after successful connection" */
    while((close(*sockfd) < 0) && (errno == EINTR));
    return 1;
  }

error:
    /* TODO: log the error */
    return -1;
}
