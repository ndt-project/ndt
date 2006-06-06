/*
 * fakewww {options}
 * concurrent server                          thd@ornl.gov
 * reap children, and thus watch out for EINTR
 * take http GET / and return a web page, then return requested file by
 * next GET
 * can use this to "provide" a java client for machine without having
 * to run a web server
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include "../config.h"
#include "usage.h"
#include "troute.h"
#include "tr-tree.h"
#include "network.h"

#define PORT           "7123"
#define BUFFSIZE        1024
char buff[BUFFSIZE];
/* html message */
char *MsgOK  = "HTTP/1.0 200 OK\n\n";
char *MsgNope = "HTTP/1.0 404 Not found\n\n"
    "<HEAD><TITLE>File Not Found</TITLE></HEAD>\n"
        "<BODY><H1>The requested file could not be found</H1></BODY>\n";

char *MsgRedir1 = "<HTML><TITLE>FLM server Redirect Page</TITLE>\n"
    "  <BODY>\n    <meta http-equiv=\"refresh\" content=\"2; ";
char *MsgRedir2 = "\">\n\n<h2>FLM server re-direction page</h2>\n"
    "<p><font size=+2>Your client is being redirected to the 'closest' FLM server "
    "for configuration testing.\n <a ";
char *MsgRedir3 = ">Click Here  </a> if you are not "
    "automatically redirected in the next 2 seconds.\n  "
    "</font></BODY>\n</HTML>";

char *Mypagefile = "/tcpbw100.html";   /* we throw the slash away */
char *okfile[] = {"/tcpbw100.html", "/Tcpbw100.class", "/Tcpbw100$1.class",
      "/Tcpbw100$clsFrame.class", "/Tcpbw100.jar", "/copyright.html", 
      "/admin.html", "/Admin.class", 0};


static struct option long_options[] = {
  {"debug", 0, 0, 'd'},
  {"help", 0, 0, 'h'},
  {"log", 1, 0, 'l'},
  {"port", 1, 0, 'p'},
  {"ttl", 1, 0, 't'},
  {"federated", 0, 0, 'F'},
  {"version", 0, 0, 'v'},
#ifdef AF_INET6
  {"ipv4", 0, 0, '4'},
  {"ipv6", 0, 0, '6'},
#endif
  {0, 0, 0, 0}
};

void dowww(int sd, I2Addr addr, char* port, char* LogFileName,
    int debug, int fed_mode, int max_ttl);

void
err_sys(char* s)
{
  perror(s);
  exit(1);
}

int
main(int argc, char** argv)
{
  int reap(), c;
  int sockfd, newsockfd;
  int federated=0, debug=0, max_ttl=10;
  time_t tt;
  socklen_t clilen;
  char* srcname = NULL;
  char* listenport = PORT;
  int conn_options = 0;

  char *LogFileName=NULL, *ctime();
  struct sockaddr_storage cli_addr;
  I2Addr listenaddr = NULL;
  FILE *fp;

#ifdef AF_INET6
#define GETOPT_LONG_INET6(x) "46"x
#else
#define GETOPT_LONG_INET6(x) x
#endif
  
  while ((c = getopt_long(argc, argv,
          GETOPT_LONG_INET6("dhl:p:t:Fv"), long_options, 0)) != -1) {
    switch (c) {
      case '4':
        conn_options |= OPT_IPV4_ONLY;
        break;
      case '6':
        conn_options |= OPT_IPV6_ONLY;
        break;
      case 'd':
        debug++;
        break;
      case 'h':
        www_long_usage("ANL/Internet2 NDT version " VERSION " (fakewww)");
        break;
      case 'v':
        printf("ANL/Internet2 NDT version %s (fakewww)\n", VERSION);
        exit(0);
        break;
      case 'l':
        LogFileName = optarg;
        break;
      case 'p':
        listenport = optarg;
        break;
      case 't':
        max_ttl = atoi(optarg);
        break;
      case 'F':
        federated = 1;
        break;
      case '?':
        short_usage(argv[0], "");
        break;
    }
  }

  if (optind < argc) {
    short_usage(argv[0], "Unrecognized non-option elements");
  }

  /*
   * Bind our local address so that the client can send to us.
   */
  if (srcname && !(listenaddr = I2AddrByNode(NULL, srcname))) {
    err_sys("server: Invalid source address specified");
  }
  if ((listenaddr = CreateListenSocket(listenaddr, listenport, conn_options)) == NULL) {
    err_sys("server: CreateListenSocket failed");
  }
  sockfd = I2AddrFD(listenaddr);

  tt = time(0);
  if (debug > 0) {
    fprintf(stderr, "%15.15s server started, listening on port %d",
        ctime(&tt)+4, I2AddrPort(listenaddr));
    if (federated == 1)
      fprintf(stderr, ", operating in Federated mode");
    fprintf(stderr, "\n");
  }
  if (LogFileName != NULL) {
    fp = fopen(LogFileName, "a");
    if (fp != NULL) {
      fprintf(fp, "%15.15s server started, listening on port %d",
          ctime(&tt)+4, I2AddrPort(listenaddr));
      if (federated == 1)
        fprintf(fp, ", operating in Federated mode");
      fprintf(fp, "\n");
      fclose(fp);
    }
  }
  signal(SIGCHLD, (__sighandler_t)reap);    /* get rid of zombies */

  /*
   * Wait for a connection from a client process.
   * This is an example of a concurrent server.
   */

  for(;;){
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0){
      if (errno == EINTR) continue; /*sig child */
      err_sys("Fakewww server: accept error");
    }

    if (fork() == 0){ /* child */
      I2Addr caddr = I2AddrBySAddr(NULL, (struct sockaddr *) &cli_addr, clilen, 0, 0);
      alarm(300);     /* kill child off after 5 minutes, should never happen */
      close(sockfd);
      dowww(newsockfd, caddr, listenport, LogFileName, debug, federated, max_ttl);
      exit(0);
    }
    close(newsockfd);
  }

}

#include        <sys/wait.h>
void
reap()
{
  /*
   * avoid zombies, since we run forever
   * Use the wait3() system call with the WNOHANG option.
   */
  int             pid;
  union wait      status;

  while ( (pid = wait3(&status, WNOHANG, (struct rusage *) 0)) > 0)
    ;
}

/*
 * Read a line from a descriptor.  Read the line one byte at a time,
 * looking for the newline.  We store the newline in the buffer,
 * then follow it with a null (the same as fgets(3)).
 * We return the number of characters up to, but not including,
 * the null (the same as strlen(3)).
 */

int
readline(fd, ptr, maxlen)
register int  fd;
register char  *ptr;
register int  maxlen;
{
  int  n, rc;
  char  c;

  for (n = 1; n < maxlen; n++) {
    if ( (rc = read(fd, &c, 1)) == 1) {
      *ptr++ = c;
      if (c == '\n')
        break;
    } else if (rc == 0) {
      if (n == 1)
        return(0);  /* EOF, no data read */
      else
        break;    /* EOF, some data was read */
    } else
      return(-1);  /* error */
  }

  *ptr = 0;
  return(n);
}
/*
 * Write "n" bytes to a descriptor.
 * Use in place of write() when fd is a stream socket.
 */

int
writen(fd, ptr, nbytes)
register int  fd;
register char  *ptr;
register int  nbytes;
{
  int  nleft, nwritten;

  nleft = nbytes;
  while (nleft > 0) {
    nwritten = write(fd, ptr, nleft);
    if (nwritten <= 0)
      return(nwritten);    /* error */

    nleft -= nwritten;
    ptr   += nwritten;
  }
  return(nbytes - nleft);
}

void
dowww(int sd, I2Addr addr, char* port, char* LogFileName, int debug, int fed_mode, int max_ttl)
{
  /* process web request */
  int fd, n, i, ok;
  char *p, filename[256], line[256], *ctime();
  char htmlfile[256];
  u_int32_t IPlist[64], srv_addr;
  u_int32_t IP6list[64][4];
  u_int32_t srv_addr6[4];
  I2Addr serv_addr = NULL;
  FILE *lfd;
  time_t tt;
  char nodename[200];
  size_t nlen = 199;
  memset(nodename, 0, 200);
  I2AddrNodeName(addr, nodename, &nlen);

  while ((n = readline(sd, buff, sizeof(buff))) > 0){
    buff[n] = '\0';
    if (n < 3)
      break;  /* end of html input */
    p = (char *) strstr(buff, "GET");
    if (p == NULL) 
      continue;
    sscanf(p+4, "%s", filename);
    if (strcmp(filename, "/") == 0){
      /* feed em the default page */
      /* strcpy(filename, Mypagefile); */
      /* By default we now send out the redirect page */

      if (debug > 3) {
        fprintf(stderr, "Received connection from [%s]\n", nodename);
      }

      if (fed_mode == 1) {
        struct sockaddr* csaddr;
        csaddr = I2AddrSAddr(addr, NULL);
        if (csaddr->sa_family == AF_INET) { /* make the IPv4 find */
          struct sockaddr_in* cli_addr = (struct sockaddr_in*) csaddr;
          char onenodename[200];
          find_route(cli_addr->sin_addr.s_addr, IPlist, max_ttl, debug);
          for (i=0; IPlist[i]!=cli_addr->sin_addr.s_addr; i++) {
            sprintf(p, "%u.%u.%u.%u", IPlist[i] & 0xff, (IPlist[i] >> 8) & 0xff,
                (IPlist[i] >> 16) & 0xff, (IPlist[i] >> 24) & 0xff);
            if (debug > 3)
              fprintf(stderr, "loop IPlist[%d] = %s\n", i, p);
            if (i == max_ttl) {
              if (debug > 3)
                fprintf(stderr, "Oops, destination not found!\n");
              break;
            }
          }
          /* print out last item on list */
          sprintf(p, "%u.%u.%u.%u", IPlist[i] & 0xff,
              (IPlist[i] >> 8) & 0xff,
              (IPlist[i] >> 16) & 0xff, (IPlist[i] >> 24) & 0xff);
          if (debug > 3)
            fprintf(stderr, "IPlist[%d] = %s\n", i, p);

          srv_addr = find_compare(IPlist, i, debug);

          /* the find_compare() routine returns the IP address of the 'closest'
           * NDT server.  It does this by comparing the clients address to a
           * map of routes between all servers.  If this comparison fails, the
           * routine returns 0.  In that case, simply use this server.
           */
          if (srv_addr == 0) {
            serv_addr = I2AddrByLocalSockFD(NULL, sd, False);
            memset(onenodename, 0, 200);
            nlen = 199;
            I2AddrNodeName(serv_addr, onenodename, &nlen);
            if (debug > 3) {
              fprintf(stderr, "find_compare() returned 0, reset to [%s]\n", onenodename);
            }
            srv_addr = ((struct sockaddr_in*)I2AddrSAddr(serv_addr, NULL))->sin_addr.s_addr;
          }

          if (debug > 3)
            fprintf(stderr, "Client host [%s] should be redirected to FLM server [%u.%u.%u.%u]\n", 
                inet_ntoa(cli_addr->sin_addr), srv_addr & 0xff, (srv_addr >> 8) & 0xff,
                (srv_addr >> 16) & 0xff, (srv_addr >> 24) & 0xff);

          /* At this point, the srv_addr variable contains the IP address of the
           * server we want to re-direct the connect to.  So we should generate a
           * new html page, and sent that back to the client.  This new page will
           * use the HTML refresh option with a short (2 second) timer to cause the
           * client's browser to just to the new server.
           * 
           * RAC 3/9/04
           */

          writen(sd, MsgOK, strlen(MsgOK));
          writen(sd, MsgRedir1, strlen(MsgRedir1));
          sprintf(line, "url=http://%u.%u.%u.%u:%s/tcpbw100.html", 
              srv_addr & 0xff, (srv_addr >> 8) & 0xff,
              (srv_addr >> 16) & 0xff, (srv_addr >> 24) & 0xff,
              port);
          writen(sd, line, strlen(line));
          writen(sd, MsgRedir2, strlen(MsgRedir2));
          sprintf(line, "href=http://%u.%u.%u.%u:%s/tcpbw100.html", 
              srv_addr & 0xff, (srv_addr >> 8) & 0xff,
              (srv_addr >> 16) & 0xff, (srv_addr >> 24) & 0xff,
              port);
          writen(sd, line, strlen(line));
          writen(sd, MsgRedir3, strlen(MsgRedir3));
          if (debug > 2) {
            fprintf(stderr, "%s redirected to remote server [%u.%u.%u.%u:%s]\n",
                inet_ntoa(cli_addr->sin_addr),
                srv_addr & 0xff, (srv_addr >> 8) & 0xff,
                (srv_addr >> 16) & 0xff, (srv_addr >> 24) & 0xff, port);
          }
          tt = time(0);
          if (LogFileName != NULL) {
            lfd = fopen(LogFileName, "a");
            if (lfd != NULL) {
              fprintf(lfd, "%15.15s [%s] redirected to remote server [%u.%u.%u.%u:%s]\n",
                  ctime(&tt)+4, inet_ntoa(cli_addr->sin_addr),
                  srv_addr & 0xff, (srv_addr >> 8) & 0xff,
                  (srv_addr >> 16) & 0xff, (srv_addr >> 24) & 0xff, port);

              fclose(lfd);
            }
          }
          continue;
        }
#ifdef AF_INET6
        else if (csaddr->sa_family == AF_INET6) {
          struct sockaddr_in6* cli_addr = (struct sockaddr_in6*) csaddr;
          char onenodename[200];
          socklen_t onenode_len;
          find_route6(nodename, IP6list, max_ttl, debug);
          for (i = 0; memcmp(IP6list[i], &cli_addr->sin6_addr, sizeof(cli_addr->sin6_addr)); i++) {
            memset(onenodename, 0, 200);
            onenode_len = 199;
            inet_ntop(AF_INET6, (void *) IP6list[i], onenodename, onenode_len);
            if (debug > 3) {
              fprintf(stderr, "loop IP6list[%d], = %s\n", i, onenodename);
            }
            if (i == max_ttl) {
              if (debug > 3) {
                fprintf(stderr, "Oops, destination not found!\n");
              }
              break;
            }
          }
          /* print out last item on list */
          if (debug > 3) {
            memset(onenodename, 0, 200);
            onenode_len = 199;
            inet_ntop(AF_INET6, (void *) IP6list[i], onenodename, onenode_len);
            fprintf(stderr, "IP6list[%d] = %s\n", i, onenodename);
          }

          srv_addr = find_compare6(srv_addr6, IP6list, i, debug);
          if (srv_addr == 0) {
            serv_addr = I2AddrByLocalSockFD(NULL, sd, False);
            memset(onenodename, 0, 200);
            nlen = 199;
            I2AddrNodeName(serv_addr, onenodename, &nlen);
            if (debug > 3) {
              fprintf(stderr, "find_compare6() returned 0, reset to [%s]\n", onenodename);
            }
            memcpy(srv_addr6, &((struct sockaddr_in6*)I2AddrSAddr(serv_addr, NULL))->sin6_addr, 16);
          }
          
          nlen = 199;
          memset(onenodename, 0, 200);
          inet_ntop(AF_INET6, (void *) srv_addr6, onenodename, nlen);
          
          if (debug > 3) {
            fprintf(stderr, "Client host [%s]", nodename);
            fprintf(stderr, " should be redirected to FLM server [%s]\n", onenodename);
          }

          writen(sd, MsgOK, strlen(MsgOK));
          writen(sd, MsgRedir1, strlen(MsgRedir1));
          sprintf(line, "url=http://[%s]:%s/tcpbw100.html", onenodename, port);
          writen(sd, line, strlen(line));
          writen(sd, MsgRedir2, strlen(MsgRedir2));
          sprintf(line, "href=http://[%s]:%s/tcpbw100.html", onenodename, port);
          writen(sd, line, strlen(line));
          writen(sd, MsgRedir3, strlen(MsgRedir3));
          if (debug > 2) {
            fprintf(stderr, "%s redirected to remote server [%s:%s]\n", nodename, onenodename, port);
          }
          tt = time(0);
          if (LogFileName != NULL) {
            lfd = fopen(LogFileName, "a");
            if (lfd != NULL) {
              fprintf(lfd, "%15.15s [%s] redirected to remote server [%s:%s]\n",
                  ctime(&tt)+4, nodename, onenodename, port);

              fclose(lfd);
            }
          }
          continue;
        }
#endif
      }
    }

    /* try to open and give em what they want */
    tt = time(0);
    writen(sd, MsgOK, strlen(MsgOK));
    if (debug > 2)
      fprintf(stderr, "%15.15s [%s] requested file '%s' - ", ctime(&tt)+4,
          nodename, filename);
    if (LogFileName != NULL) {
      lfd = fopen(LogFileName, "a");
      if (lfd != NULL) {
        fprintf(lfd, "%15.15s [%s] requested file '%s' - ", ctime(&tt)+4,
            nodename, filename);
        fclose(lfd);
      }
    }
    ok = 0;
    if (strcmp(filename, "/") == 0)
      strncpy(filename, "/tcpbw100.html", 15);
    for(i=0; okfile[i]; i++) {
      /* restrict file access */
      if (strcmp(okfile[i], filename) == 0) {
        ok=1;
        if (debug > 2)
          fprintf(stderr, "sent to client\n");
        if (LogFileName != NULL) {
          lfd = fopen(LogFileName, "a");
          if (lfd != NULL) {
            fprintf(lfd, "sent to client\n");
            fclose(lfd);
          }
        }
        break;
      }
    }
    if (ok == 0) {
      writen(sd, MsgNope, strlen(MsgNope));
      if (debug > 2)
        fprintf(stderr, "access denied\n");
      if (LogFileName != NULL) {
        lfd = fopen(LogFileName, "a");
        if (lfd != NULL) {
          fprintf(lfd, "access denied\n");
          fclose(lfd);
        }
      }
      continue;
    }
    sprintf(htmlfile, "%s/%s", BASEDIR, filename+1);
    fd = open(htmlfile, 0);  /* open file for read */
    if (fd < 0) {
      close(fd);
      writen(sd, MsgNope, strlen(MsgNope));
      if (debug > 2)
        fprintf(stderr, " not found\n");
      if (LogFileName != NULL) {
        lfd = fopen(LogFileName, "a");
        if (lfd != NULL) {
          fprintf(lfd, " not found\n");
          fclose(lfd);
        }
      }
      continue;
    }
    /* reply: */
    while( (n = read(fd, buff, sizeof(buff))) > 0){
      writen(sd, buff, n);
    }
    close(fd);
  }
  close(sd);
}
