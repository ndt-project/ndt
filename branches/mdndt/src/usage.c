/*
 * This file contains the functions to print usage screens.
 *
 * Jakub S³awiñski 2006-06-02
 * jeremian@poczta.fm
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/*
 * Function name: srv_short_usage
 * Description: Prints the short usage of the web100srv.
 * Arguments: info - the text printed in the first line
 */

void
srv_short_usage(char* info)
{
  assert(info != NULL);
  printf("\n%s\n\n\n", info);
  printf("Try `web100srv --help' for more information.\n");

  exit(1);
}

/*
 * Function name: srv_long_usage
 * Description: Prints the short usage of the web100srv.
 * Arguments: info - the text printed in the first line
 */

void
srv_long_usage(char* info)
{
  assert(info != NULL);
  printf("\n%s\n\n\n", info);
  printf(" Basic options:\n\n");
  printf("  -a, --adminview        - generate administrator view html page\n");
  printf("  -d, --debug            - print additional diagnostic messages\n");
  printf("                           Note: add multiple d's (-ddd) for more details\n");
  printf("  -h, --help             - print help message (this message)\n");
  printf("  -m, --multiple         - select single or multi-test mode\n");
  printf("  -o, --old              - use old Duplex Mismatch heuristic\n");
  printf("  -q, --disable-queue    - disable FIFO queuing of client requests\n");
  printf("  -r, --record           - record client to server Web100 variables\n");
  printf("  -s, --syslog           - export Web100 data via the syslog() LOG_LOCAL0 facility\n");
  printf("  -t, --tcpdump          - write tcpdump formatted file to disk\n");
  printf("  -x, --experimental     - enable any experimental code\n");
  printf("  -v, --version          - print version number\n\n");
  printf(" Configuration:\n\n");
  printf("  -c, --config #filename - specify the name of the file with configuration\n");
  printf("  -y, --limit #limit     - enable the experimental throughput limiting code\n");
  printf("  -b, --buffer #size     - set TCP send/recv buffers to user value\n");
  printf("  -f, --file variable_FN - specify alternate 'web100_variables' file\n");
  printf("  -i, --interface device - specify network interface (libpcap device)\n");
  printf("  -l, --log Log_FN       - specify alternate 'web100srv.log' file\n");
  printf("  -p, --port #port       - specify primary port number (default 3001)\n");
  printf("  -T, --refresh #time    - specify the refresh time of the admin page\n\n");
#ifdef AF_INET6
  printf(" IP family:\n\n");
  printf("  -4, --ipv4             - use IPv4 addresses only\n");
  printf("  -6, --ipv6             - use IPv6 addresses only\n\n");
#endif

  exit(0);
}

/*
 * Function name: clt_short_usage
 * Description: Prints the short usage of the web100clt.
 * Arguments: info - the text printed in the first line
 */

void
clt_short_usage(char* info)
{
  assert(info != NULL);
  printf("\n%s\n\n\n", info);
  printf("Try `web100clt --help' for more information.\n");

  exit(1);
}

/*
 * Function name: clt_long_usage
 * Description: Prints the short usage of the web100clt.
 * Arguments: info - the text printed in the first line
 */

void
clt_long_usage(char* info)
{
  assert(info != NULL);
  printf("\n%s\n\n\n", info);
  printf(" Basic options:\n\n");
  printf("  -n, --name #name       - specify name of the server\n");
  printf("  -p, --port #port       - specify port server is listening on\n");
  printf("  -d, --debug            - Increase debug level details\n");
  printf("                           Note: add multiple d's (-ddd) for more details\n");
  printf("  -h, --help             - print this help message\n");
  printf("  -l, --msglvl           - increase message level details\n");
  printf("  -v, --version          - print version number\n\n");
  printf(" Configuration:\n\n");
  printf("  -b, --buffer #size     - set send/receive buffer to value\n\n");
 #ifdef AF_INET6
  printf(" IP family:\n\n");
  printf("  -4, --ipv4             - use IPv4 addresses only\n");
  printf("  -6, --ipv6             - use IPv6 addresses only\n\n");
#endif

  exit(0);
}
