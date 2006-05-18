/*
 *  fakewww [port]
 *   concurrent server                          thd@ornl.gov
 * reap children, and thus watch out for EINTR
 *  take http GET / and return a web page, then return requested file by
 *   next GET
 * can use this to "provide" a java client for machine without having
 * to run a web server
 */

#include        <stdio.h>
#include        <sys/types.h>
#include        <sys/socket.h>
#include        <netinet/in.h>
#include        <arpa/inet.h>
#include        <netdb.h>
#include        <signal.h>
/* #include	<getopt.h> */
#include	<unistd.h>
/* #include	<config.h> */
#include	<time.h>


#define PORT 7123
#define BUFFSIZE        1024
char buff[BUFFSIZE];
/* html message */
char *MsgOK  = "HTTP/1.0 200 OK\n\n";
char *MsgNope = "HTTP/1.0 404 Not found\n\n"
    "<HEAD><TITLE>File Not Found</TITLE></HEAD>\n"
        "<BODY><H1>The requested file could not be found</H1></BODY>\n";

char *MsgRedir1 = "<HTML><TITLE>FLM server Redirect Page</TITLE>\n"
		"  <BODY>\n    <meta http-equiv=\"refresh\" content=\"2; ";
		/* url=http://%s/tcpbw100.html>" */
char *MsgRedir2 = "\">\n\n<h2>FLM server re-direction page</h2>\n"
		"<p><font size=+2>Your client is being redirected to the 'closest' FLM server "
		"for configuration testing.\n <a ";
		/* href=http://%s/tcpbw100.html */
char *MsgRedir3 = ">Click Here  </a> if you are not "
		"automatically redirected in the next 2 seconds.\n  "
		"</font></BODY>\n</HTML>";

char *Mypagefile = "/tcpbw100.html";   /* we throw the slash away */
char *okfile[] = {"/tcpbw100.html", "/Tcpbw100.class", "/Tcpbw100$1.class",
		  "/Tcpbw100$clsFrame.class", "/Tcpbw100.jar", "/copyright.html", 
		  "/admin.html", "/Admin.class", 0};

#include <sys/errno.h>
extern int errno;

main(argc, argv)
int	argc;
char	*argv[];
{
	int port=PORT, one=1;
	int reap(), c;
	int n, sockfd, newsockfd, clilen, childpid, servlen;
	int federated=0, debug=0, max_ttl=10, i;
	time_t tt;

	char *LogFileName=NULL, *ctime();
	struct sockaddr_in	cli_addr, serv_addr;
	FILE *fp;


	/* if (argc > 1) port = atoi(argv[1]); */
	while ((c = getopt(argc, argv, "dhl:p:t:F")) != -1){
	    switch (c) {
		case 'd':
			debug++;
			break;
		case 'h':
			printf("Usage: %s {options}\n", argv[0]);
			printf("\t-d \tincrement debugging mode\n");
			printf("\t-h \tprint this help message\n");
			printf("\t-l log_FN \tlog connection requests in specified file\n");
			printf("\t-p port# \tspecify the port number (default is 7123)\n");
			printf("\t-t # \tspecify maximum number of hops in path (default is 10)\n");
			printf("\t-F \toperate in Federated mode\n");
			exit(0);
		case 'l':
			LogFileName = optarg;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 't':
			max_ttl = atoi(optarg);
			break;
		case 'F':
			federated = 1;
			break;
	    }
	}

	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err_sys("server: can't open stream socket");

	/*
	 * Bind our local address so that the client can send to us.
	 */

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port        = htons(port);

	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		err_sys("server: can't bind local address");

	clilen = sizeof(cli_addr);
	getsockname(sockfd, (struct sockaddr *)&cli_addr, &clilen);
	tt = time(0);
	if (debug > 0) {
	    fprintf(stderr, "%15.15s server started, listening on port %d",
			    ctime(&tt)+4, ntohs(cli_addr.sin_port));
	    if (federated == 1)
		fprintf(stderr, ", operating in Federated mode");
	    fprintf(stderr, "\n");
	}
	if (LogFileName != NULL) {
	    fp = fopen(LogFileName, "a");
	    if (fp != NULL) {
	        fprintf(fp, "%15.15s server started, listening on port %d",
				ctime(&tt)+4, ntohs(cli_addr.sin_port));
	        if (federated == 1)
		    fprintf(fp, ", operating in Federated mode");
	        fprintf(fp, "\n");
		fclose(fp);
	    }
	}
	listen(sockfd, 5);
	signal(SIGCHLD, (__sighandler_t)reap);		/* get rid of zombies */

		/*
		 * Wait for a connection from a client process.
		 * This is an example of a concurrent server.
		 */

	for(;;){
		int i;

		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0){
			if (errno == EINTR) continue; /*sig child */
			err_sys("Fakewww server: accept error");
		}

		if (fork() == 0){ /* child */
			alarm(300); 		/* kill child off after 5 minutes, should never happen */
			close(sockfd);
			dowww(newsockfd, cli_addr, port, LogFileName, debug, federated, max_ttl);
			exit(0);
		}
		close(newsockfd);
	}

}

#include        <sys/wait.h>
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

err_sys(s)
char *s;
{
	perror(s);
	exit(1);
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
register int	fd;
register char	*ptr;
register int	maxlen;
{
	int	n, rc;
	char	c;

	for (n = 1; n < maxlen; n++) {
		if ( (rc = read(fd, &c, 1)) == 1) {
			*ptr++ = c;
			if (c == '\n')
				break;
		} else if (rc == 0) {
			if (n == 1)
				return(0);	/* EOF, no data read */
			else
				break;		/* EOF, some data was read */
		} else
			return(-1);	/* error */
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
register int	fd;
register char	*ptr;
register int	nbytes;
{
	int	nleft, nwritten;

	nleft = nbytes;
	while (nleft > 0) {
		nwritten = write(fd, ptr, nleft);
		if (nwritten <= 0)
			return(nwritten);		/* error */

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(nbytes - nleft);
}

dowww(sd, cli_addr, port, LogFileName, debug, fed_mode, max_ttl)
	int sd;
	struct sockaddr_in cli_addr;
	int port;
	char *LogFileName;
	int debug;
	int fed_mode;
	int max_ttl;
{
	/* process web request */
	int fd, n, i, ok;
	char *p, filename[256], line[256], *ctime();
	char htmlfile[256];
	u_int32_t IPlist[64], srv_addr;
	FILE *lfd;
	struct sockaddr_in serv_addr;
	time_t tt;
	
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

			p = inet_ntoa(cli_addr.sin_addr);
			if (debug > 3)
			    fprintf(stderr, "Received connection from [%s]\n", p);

			if (fed_mode == 1) {
			    find_route(cli_addr.sin_addr, &IPlist, max_ttl, debug);
			    for (i=0; IPlist[i]!=cli_addr.sin_addr.s_addr; i++) {
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
			    /* if (i == 0) { */
	    		    sprintf(p, "%u.%u.%u.%u", IPlist[i] & 0xff,
				(IPlist[i] >> 8) & 0xff,
				(IPlist[i] >> 16) & 0xff, (IPlist[i] >> 24) & 0xff);
			    if (debug > 3)
	    		        fprintf(stderr, "IPlist[%d] = %s\n", i, p);
			    /* } */
		
			    srv_addr = find_compare(IPlist, i, debug);

			    /* the find_compare() routine returns the IP address of the 'closest'
			     * NDT server.  It does this by comparing the clients address to a
			     * map of routes between all servers.  If this comparison fails, the
			     * routine returns 0.  In that case, simply use this server.
			     */
			    i = sizeof(serv_addr);
			    if (srv_addr == 0) {
				getsockname(sd, (struct sockaddr *) &serv_addr, &i);
				if (debug > 3)
				    fprintf(stderr, "find_comapre() returned 0, reset to [%s]\n",
					inet_ntoa(serv_addr.sin_addr));
				srv_addr = serv_addr.sin_addr.s_addr;
			    }
		
			    if (debug > 3)
			        fprintf(stderr, "Client host [%s] should be redirected to FLM server [%u.%u.%u.%u]\n", 
				    inet_ntoa(cli_addr.sin_addr), srv_addr & 0xff, (srv_addr >> 8) & 0xff,
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
			    sprintf(line, "url=http://%u.%u.%u.%u:%d/tcpbw100.html", 
				 srv_addr & 0xff, (srv_addr >> 8) & 0xff,
				(srv_addr >> 16) & 0xff, (srv_addr >> 24) & 0xff,
				port);
			    writen(sd, line, strlen(line));
			    writen(sd, MsgRedir2, strlen(MsgRedir2));
			    sprintf(line, "href=http://%u.%u.%u.%u:%d/tcpbw100.html", 
				 srv_addr & 0xff, (srv_addr >> 8) & 0xff,
				(srv_addr >> 16) & 0xff, (srv_addr >> 24) & 0xff,
				port);
			    writen(sd, line, strlen(line));
			    writen(sd, MsgRedir3, strlen(MsgRedir3));
			    if (debug > 2) {
			        fprintf(stderr, "%s redirected to remote server [%u.%u.%u.%u:%d]\n",
			    	    inet_ntoa(cli_addr.sin_addr),
				     srv_addr & 0xff, (srv_addr >> 8) & 0xff,
				    (srv_addr >> 16) & 0xff, (srv_addr >> 24) & 0xff, port);
			    }
			    tt = time(0);
			    if (LogFileName != NULL) {
		   		lfd = fopen(LogFileName, "a");
		   		if (lfd != NULL) {
			            fprintf(lfd, "%15.15s [%s] redirected to remote server [%u.%u.%u.%u:%d]\n",
			    	         ctime(&tt)+4, inet_ntoa(cli_addr.sin_addr),
				         srv_addr & 0xff, (srv_addr >> 8) & 0xff,
				        (srv_addr >> 16) & 0xff, (srv_addr >> 24) & 0xff, port);

				fclose(lfd);
		    		}
			    }
			    continue;
		    }
		}

		/* try to open and give em what they want */
		tt = time(0);
		writen(sd, MsgOK, strlen(MsgOK));
		if (debug > 2)
		    fprintf(stderr, "%15.15s [%s] requested file '%s' - ", ctime(&tt)+4,
				inet_ntoa(cli_addr.sin_addr), filename);
		if (LogFileName != NULL) {
		    lfd = fopen(LogFileName, "a");
		    if (lfd != NULL) {
		        fprintf(lfd, "%15.15s [%s] requested file '%s' - ", ctime(&tt)+4,
				inet_ntoa(cli_addr.sin_addr), filename);
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
