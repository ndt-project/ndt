/*
 * This file contains the functions to print usage screens.
 *
 * Jakub S³awiñski 2006-06-02
 * jeremian@poczta.fm
 */

#include "../config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <assert.h>

/*
 * Function name: short_usage
 * Description: Prints the short usage of the application.
 * Arguments: prog - the name of the application
 *            info - the text printed in the first line
 */

void
short_usage(char* prog, char* info)
{
    assert(prog != NULL);
    assert(info != NULL);
    printf("\n%s\n\n\n", info);
    printf("Try `%s --help' for more information.\n", prog);

    exit(1);
}

/*
 * Function name: srv_long_usage
 * Description: Prints the long usage of the web100srv.
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
    printf("  -s, --syslog           - use the syslog() to log some information\n");
    printf("  -t, --tcpdump          - write tcpdump formatted file to disk\n");
    printf("  -v, --version          - print version number\n");
    printf("  -x, --max_clients      - maximum numbers of clients permited in FIFO queue (default=50)\n");
    printf("  -z, --gzip	     - disable compression of tcptrace, snaplog, and cputime files\n\n");
    printf(" Configuration:\n\n");
    printf("  -c, --config #filename - specify the name of the file with configuration\n");
    printf("  -b, --buffer #size     - set TCP send/recv buffers to user value\n");
    printf("  -f, --file variable_FN - specify alternate 'web100_variables' file\n");
    printf("  -i, --interface device - specify network interface (libpcap device)\n");
    printf("  -l, --log Log_FN       - specify alternate 'web100srv.log' file\n");
    printf("  -p, --port #port       - specify primary port number (default 3001)\n");
    printf("  --midport #port        - specify Middlebox test port number (default 3003)\n");
    printf("  --c2sport #port        - specify C2S throughput test port number (default 3002)\n");
    printf("  --s2cport #port        - specify S2C throughput test port number (default 3003)\n");
    printf("  -T, --refresh #time    - specify the refresh time of the admin page\n");
    printf("  --mrange #range        - set the port range used in multi-test mode\n");
    printf("                           Note: this enables multi-test mode\n");
    printf("  -A, --adminfile #FN    - specify atlernate filename for Admin web page\n");
    printf("                           Note: this doesn't enable 'adminview'\n");
    printf("  -L, --log_dir DIR      - specify the base directory for snaplog and tcpdump files\n");
    printf("                           (default %s/serverdata)\n", BASEDIR);
    printf("  -S, --logfacility #F   - specify syslog facility name\n");
    printf("                           Note: this doesn't enable 'syslog'\n\n");
#ifdef EXPERIMENTAL_ENABLED
    printf(" Experimental code:\n\n");
    printf("  --avoidsndblockup      - enable code to avoid send buffers blocking in the S2C test\n");
    printf("  --snaplog              - enable the snaplog writing\n");
    printf("  --snapdelay #msec      - specify the delay in the snaplog thread (default 5 msec)\n");
    printf("                           Note: this doesn't enable 'snaplog'\n");
    printf("  --cwnddecrease         - enable analyzing of the cwnd changes during the S2C test\n");
    printf("                           Note: this automatically enables 'snaplog'\n");
    printf("  --cputime              - enable the cputime writing\n");
    printf("  -y, --limit #limit     - enable the throughput limiting code\n\n");
#endif
#if defined(HAVE_ODBC) && defined(DATABASE_ENABLED) && defined(HAVE_SQL_H)
    printf(" Database support:\n\n");
    printf("  --enableDBlogging      - enable the test results logging to the database\n");
    printf("  --dbDSN #dsn           - specify the DSN to use (this doesn't enable DBlogging)\n");
    printf("  --dbUID #uid           - specify the UID to use (this doesn't enable DBlogging)\n");
    printf("  --dbPWD #pwd           - specify the PWD to use (this doesn't enable DBlogging)\n\n");
#endif
#ifdef AF_INET6
    printf(" IP family:\n\n");
    printf("  -4, --ipv4             - use IPv4 addresses only\n");
    printf("  -6, --ipv6             - use IPv6 addresses only\n\n");
#endif

    exit(0);
}

/*
 * Function name: clt_long_usage
 * Description: Prints the long usage of the web100clt.
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
    printf("  --web100variables      - print the information about each Web100 variable\n");
    printf("  -v, --version          - print version number\n\n");
    printf(" Configuration:\n\n");
    printf("  -b, --buffer #size     - set send/receive buffer to value\n");
    printf("  --disablemid           - disable the Middlebox test\n");
    printf("  --disablec2s           - disable the C2S throughput test\n");
    printf("  --disables2c           - disable the S2C throughput test\n");
    printf("  --disablesfw           - disable the Simple firewall test\n\n");
#ifdef AF_INET6
    printf(" IP family:\n\n");
    printf("  -4, --ipv4             - use IPv4 addresses only\n");
    printf("  -6, --ipv6             - use IPv6 addresses only\n\n");
#endif

    exit(0);
}

/*
 * Function name: www_long_usage
 * Description: Prints the long usage of the fakewww.
 * Arguments: info - the text printed in the first line
 */

void
www_long_usage(char* info)
{
    assert(info != NULL);
    printf("\n%s\n\n\n", info);
    printf(" Basic options:\n\n");
    printf("  -d, --debug            - increment debugging mode\n");
    printf("                           Note: add multiple d's (-ddd) for more details\n");
    printf("  -h, --help             - print this help message\n");
    printf("  -F, --federated        - operate in Federated mode\n");
    printf("  -f, --file FN          - add the file to the allowed list\n");
    printf("  -s, --syslog           - use the syslog() to log some information\n");
    printf("  -v, --version          - print version number\n\n");
    printf(" Configuration:\n\n");
    printf("  -l, --alog alog_FN     - specify the access log filename\n");
    printf("  -e, --elog elog_FN     - specify the error log filename\n");
    printf("  -b, --basedir path     - set the basedir for the documents\n");
    printf("  -S, --logfacility #F   - specify syslog facility name\n");
    printf("                           Note: this doesn't enable 'syslog'\n");
    printf("  -p, --port #port       - specify the port number (default is 7123)\n");
    printf("  -t, --ttl #amount      - specify maximum number of hops in path (default is 10)\n");
    printf("  --dflttree fn          - specify alternate 'Default.tree' file\n");
#ifdef AF_INET6
    printf("  --dflttree6 fn         - specify alternate 'Default.tree6' file\n\n");
    printf(" IP family:\n\n");
    printf("  -4, --ipv4             - use IPv4 addresses only\n");
    printf("  -6, --ipv6             - use IPv6 addresses only\n\n");
#else
    printf("\n");
#endif

    exit(0);
}

/*
 * Function name: analyze_long_usage
 * Description: Prints the long usage of the analyze.
 * Arguments: info - the text printed in the first line
 */

void
analyze_long_usage(char* info)
{
    assert(info != NULL);
    printf("\n%s\n\n\n", info);
    printf(" Basic options:\n\n");
    printf("  -d, --debug            - increment debugging mode\n");
    printf("                           Note: add multiple d's (-ddd) for more details\n");
    printf("  -n, --nodns            - disable resolving DNS names\n");
    printf("  -h, --help             - print this help message\n");
    printf("  -l, --log log_FN       - specify the file with the logs\n");
    printf("  -v, --version          - print version number\n\n");

    exit(0);
}

/*
 * Function name: mkmap_long_usage
 * Description: Prints the long usage of the tr-mkmap.
 * Arguments: info - the text printed in the first line
 */

void
mkmap_long_usage(char* info)
{
    assert(info != NULL);
    printf("\n%s\n\n\n", info);
    printf(" Basic options:\n\n");
    printf("  -b, --build            - build a new default tree\n");
    printf("  -c, --compare fn       - compare new traceroute to tree\n");
    printf("                           Note: -b and -c are mutually exclusive\n");
    printf("  -f, --file fn          - specify the name of the input file\n");
    printf("  -h, --help             - print this help message\n");
    printf("  -p, --print            - [default] print out the current traceroute map\n");
    printf("  -d, --debug            - increment debugging mode\n");
    printf("                           Note: add multiple d's (-ddd) for more details\n");
    printf("  -v, --version          - print version number\n\n");
    printf(" Configuration:\n\n");
    printf("  --dflttree fn          - specify alternate 'Default.tree' file\n");
#ifdef AF_INET6
    printf("  --dflttree6 fn         - specify alternate 'Default.tree6' file\n\n");
    printf(" IP family:\n\n");
    printf("  -4, --ipv4             - use only IPv4 operational mode\n");
    printf("  -6, --ipv6             - use only IPv6 operational mode\n\n");
#else
    printf("\n");
#endif

    exit(0);
}

/*
 * Function name: vt_long_usage
 * Description: Prints the long usage of the viewtrace.
 * Arguments: info - the text printed in the first line
 */

void
vt_long_usage(char* info)
{
    assert(info != NULL);
    printf("\n%s\n\n\n", info);
    printf(" Basic options:\n\n");
    printf("  -c, --count #amount    - specify how many packets will be viewed\n");
    printf("  -d, --debug            - increment debugging mode\n");
    printf("                           Note: add multiple d's (-ddd) for more details\n");
    printf("  -f, --file fn          - specify the name of the file to offline trace\n");
    printf("  -h, --help             - print this help message\n");
    printf("  -i, --interface device - specify network interface (libpcap device)\n");
    printf("  --c2sport #port        - specify C2S throughput test port number (default 3002)\n");
    printf("  --s2cport #port        - specify S2C throughput test port number (default 3003)\n");
    printf("  -v, --version          - print version number\n\n");

    exit(0);
}

/*
 * Function name: genplot_long_usage
 * Description: Prints the long usage of the genplot.
 * Arguments: info - the text printed in the first line
 */

void
genplot_long_usage(char* info, char* argv0)
{
    assert(info != NULL);
    printf("\n%s\n\n\n", info);
    printf("Usage: %s [options] filelist\n", argv0);
    printf("This program generates xplot graphs from Web100 variables\n\n");
    printf(" Basic options:\n\n");
    printf("  -b, --both             - generate plot of both CurCwnd and CurRwinRcvd\n");
    printf("  -m, --multi varlist    - a comma separated list of Web100 vars to plot\n");
    printf("  -t, --text             - print variable values instead of generating plots\n");
    printf("  -C, --CurCwnd          - generate CurCwnd plot\n");
    printf("  -R, --CurRwinRcvd      - generate CurRwinRcvd plot\n");
    printf("  -S, --throughput       - generate throughput plot\n");
    printf("  -c, --cwndtime         - generate Cwnd time plot\n");
    printf("  -h, --help             - print this help message\n");
    printf("  -v, --version          - print version number\n\n");

    exit(0);
}
