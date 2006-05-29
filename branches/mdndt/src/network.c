/*
 * Enhancements of NDT (Middlebox detector based on NDT)
 * Copyright (C) 2006 jeremian <jeremian [at] poczta.fm>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "network.h"

static int
OpenSocket(I2Addr addr, int options)
{
  struct addrinfo *fai;
  struct addrinfo *ai;
  int             on;
  int             fd=-1;

  if (!(fai = I2AddrAddrInfo(addr, NULL, NULL))) {
    return -2;
  }

  for (ai = fai; ai; ai = ai->ai_next) {
#ifdef AF_INET6
    if (options | OPT_IPV6_ONLY) {
      if(ai->ai_family != AF_INET6)
        continue;
    }
#endif
    
    if (options | OPT_IPV4_ONLY) {
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

    if (bind(fd,ai->ai_addr,ai->ai_addrlen) == 0) {

      if (!I2AddrSetSAddr(addr,ai->ai_addr,ai->ai_addrlen) ||
          !I2AddrSetProtocol(addr,ai->ai_protocol) ||
          !I2AddrSetSocktype(addr,ai->ai_socktype) ||
          !I2AddrSetFD(addr,fd,True)){
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

int
CreateListenSocket(I2Addr addr, char* serv, int options)
{
  int fd = -1;

  if (addr && (I2AddrFD(addr) > -1)) {
    /* TODO: log the error */
    goto error;
  }

  if ((!addr) && !(addr = I2AddrByWildcard(/* FIXME */NULL, SOCK_STREAM, serv))) {
    goto error;
  }

  if (!I2AddrSetPassive(addr,True)) {
    /* TODO: log the error */
    goto error;
  }
  
  fd = OpenSocket(addr, options);
  
  if (fd < 0) {
    /* TODO: log the error */
    goto error;
  }

  if (listen(fd, NDT_BACKLOG) < 0) {
    /* TODO: log the error */
    goto error;
  }

  return 0;
  
error:
    I2AddrFree(addr);
    return 1;
}

int
CreateConnectSocket(int* sockfd, I2Addr local_addr, I2Addr server_addr, int options)
{
  struct addrinfo *fai=NULL;
  struct addrinfo *ai=NULL;
  char            nodename[NI_MAXHOST];
  size_t          nodename_len = sizeof(nodename);
  char            servname[NI_MAXSERV];
  size_t          servname_len = sizeof(servname);
  char            *node,*serv;

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
    if (options | OPT_IPV6_ONLY) {
      if(ai->ai_family != AF_INET6)
        continue;
    }
#endif

    if (options | OPT_IPV4_ONLY) {
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
cleanup:
    while((close(*sockfd) < 0) && (errno == EINTR));
    return 1;
  }

error:
    /* TODO: log the error */
    return -1;
}
