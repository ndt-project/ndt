/* This program is part of the ANL Network Diagnostic package.
 * It is designed to perform packet pair timings to determine
 * what type of bottleneck link exists on the path between this
 * server and a remote desktop client.
 *
 *  Written by
 *  Richard Carlson <RACarlson@anl.gov>
 */

#include "../config.h"

#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#ifdef HAVE_LIBPCAP
#include <pcap.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/errno.h>

#include <netinet/ip.h>
#include <netinet/tcp.h>
#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif

#include "web100srv.h"
#include "usage.h"
#include "logging.h"
#include "utils.h"

struct spdpair fwd, rev;
int start, finish, fini;
extern int errno;

#ifdef HAVE_LIBPCAP
static pcap_t *pd;
#endif
extern int optind;
extern int opterr;
extern char *optarg;
FILE *fp;

int c2sport = 3002,
    s2cport = 3003;

static struct option long_options[] = {
  {"count", 1, 0, 'c'},
  {"debug", 0, 0, 'd'},
  {"file", 1, 0, 'f'},
  {"help", 0, 0, 'h'},
  {"interface", 1, 0, 'i'},
  {"log", 1, 0, 'l'},
  {"c2sport", 1, 0, 303},
  {"s2cport", 1, 0, 304},
  {"version", 0, 0, 'v'},
  {0, 0, 0, 0}
};

/*
 * Copy arg vector into a new buffer, concatenating arguments with spaces.
 */
char *
copy_argv(register char **argv)
{
  register char **p;
  register u_int len = 0;
  char *buf;
  char *src, *dst;

  p = argv;
  if (*p == 0)
    return 0;

  while (*p)
    len += strlen(*p++) + 1;

  buf = (char *)malloc(len);
  if (buf == NULL) {
    perror("copy_argv: malloc");
    exit(1);
  }

  p = argv;
  dst = buf;
  while ((src = *p++) != NULL) {
    while ((*dst++ = *src++) != '\0')
      ;
    dst[-1] = ' ';
  }
  dst[-1] = '\0';

  return buf;
}

/* initialize variables before starting to accumulate data */
void
init_vars(struct spdpair *cur)
{
  int i;

#if defined(AF_INET6)
	memset(cur->saddr, 0, 4);
	memset(cur->daddr, 0, 4);
#else
  cur->saddr = 0;
  cur->daddr = 0;
#endif
  cur->sport = 0;
  cur->dport = 0;
  cur->seq = 0;
  cur->ack = 0;
  cur->win = 0;
  cur->sec = 0;
  cur->usec = 0;
  cur->time = 0;
  cur->totalspd = 0;
  cur->totalcount = 0;
  for (i=0; i<16; i++)
    cur->links[i] = 0;
}


/* This routine prints results to the screen.  */
void
vt_print_bins(struct spdpair *cur)
{

  int i, total=0, max=0, s, index=0;
  char buff[256];
  int tzoffset = 6;

  /* the tzoffset value is fixed for CST (6), use 5 for CDT.  The code needs to find the
   * current timezone and use that value here! */
  s = (cur->st_sec - (tzoffset * 3600)) %86400; 

  for (i=0; i<16; i++) {
    total += cur->links[i];
    if (cur->links[i] > max) {
      max = cur->links[i];
      index = i;
    }
  }
  if ((max == cur->links[10]) || (max == cur->links[11])) {
    max = 0;
    for (i=0; i<10; i++) {
      if (max < cur->links[i]) {
        index = i;
        max = cur->links[i];
      }
    }
  }

  fprintf(stderr, "%02d:%02d:%02d.%06u   ",
      s / 3600, (s % 3600) / 60, s % 60, cur->st_usec);
  if ((cur->sport == c2sport) || (cur->dport == c2sport)) {
#if defined(AF_INET6)
    char str[136];
    memset(str, 0, 136);
    inet_ntop(AF_INET6, (void *) cur->saddr, str, sizeof(str));
    fprintf(stderr, "%s.%d --> ", str, cur->sport);
    memset(str, 0, 136);
    inet_ntop(AF_INET6, (void *) cur->daddr, str, sizeof(str));
    fprintf(stderr, "%s.%d  ", str, cur->dport);
#else
    fprintf(stderr, "%u.%u.%u.%u:%d --> ", (cur->saddr & 0xFF), ((cur->saddr >> 8) & 0xff),
        ((cur->saddr >> 16) & 0xff),  (cur->saddr >> 24), cur->sport);
    fprintf(stderr, "%u.%u.%u.%u:%d  ", (cur->daddr & 0xFF), ((cur->daddr >> 8) & 0xff),
        ((cur->daddr >> 16) & 0xff),  (cur->daddr >> 24), cur->dport);
#endif
  } else {
#if defined(AF_INET6)
    char str[136];
    memset(str, 0, 136);
    inet_ntop(AF_INET6, (void *) cur->daddr, str, sizeof(str));
    fprintf(stderr, "%s.%d --> ", str, cur->dport);
    memset(str, 0, 136);
    inet_ntop(AF_INET6, (void *) cur->saddr, str, sizeof(str));
    fprintf(stderr, "%s.%d  ", str, cur->sport);
#else
    fprintf(stderr, "%u.%u.%u.%u:%d --> ", (cur->daddr & 0xFF), ((cur->daddr >> 8) & 0xff),
        ((cur->daddr >> 16) & 0xff),  (cur->daddr >> 24), cur->dport);
    fprintf(stderr, "%u.%u.%u.%u:%d  ", (cur->saddr & 0xFF), ((cur->saddr >> 8) & 0xff),
        ((cur->saddr >> 16) & 0xff),  (cur->saddr >> 24), cur->sport);
#endif
  }
  if (max == 0) 
    fprintf(stderr, "\n\tNo packets collected\n");

  switch (index) {
    case -1: fprintf(stderr, "link=%d (Fault); ", index);
             break;
    case 0:  fprintf(stderr, "link=%d (RTT); ", index);
             break;
    case 1:  fprintf(stderr, "link=%d (dial-up); ", index);
             break;
    case 2:  fprintf(stderr, "link=%d (T1); ", index);
             break;
    case 3:  fprintf(stderr, "link=%d (Enet); ", index);
             break;
    case 4:  fprintf(stderr, "link=%d (T3); ", index);
             break;
    case 5:  fprintf(stderr, "link=%d (FastE); ", index);
             break;
    case 6:  fprintf(stderr, "link=%d (OC-12); ", index);
             break;
    case 7:  fprintf(stderr, "link=%d (GigE); ", index);
             break;
    case 8:  fprintf(stderr, "link=%d (OC-48); ", index);
             break;
    case 9:  fprintf(stderr, "link=%d (10 GigE); ", index);
             break;
    case 10:  fprintf(stderr, "retransmission; ");
              break;
    case 11:  fprintf(stderr, "link=%d (unknown); ", index);
              break;
  }
  fprintf(stderr, "packets=%d\n", total);
  fprintf(stderr, "Running Average = %0.2f Mbps  ", cur->totalspd2);
  fprintf(stderr, "Average speed = %0.2f Mbps\n", cur->totalspd/cur->totalcount);

  if (cur->totalcount > 0) {
    sprintf(buff, "%d %d %d %d %d %d %d %d %d %d %d %d %0.2f", cur->links[0], cur->links[1],
        cur->links[2], cur->links[3], cur->links[4], cur->links[5], cur->links[6],
        cur->links[7], cur->links[8], cur->links[9], cur->links[10], cur->links[11],
        cur->totalspd2);
    fprintf(stderr, "link counters are '%s'\n", buff);
  }

  init_vars(&*cur);
}

void
vt_calculate_spd(struct spdpair *cur, struct spdpair *cur2)
{

  float bits, spd, time;

  time = (((cur->sec - cur2->sec)*1000000) + (cur->usec - cur2->usec));
  if ((cur->dport == c2sport) || (cur->sport == s2cport)) {
    if (cur->seq >= cur2->seq)
      bits = (cur->seq - cur2->seq) * 8;
    else
      bits = 0;
  } else  {
    if (cur->ack >= cur2->ack)
      bits = (cur->ack - cur2->ack) * 8;
    else
      bits = 0;
  }
  spd = (bits/time);      /* convert to mbits/sec) */
  if ((spd > 0) && (spd <= 0.01))
    cur2->links[0]++;
  if ((spd > 0.01) && (spd <= 0.064))
    cur2->links[1]++;
  if ((spd > 0.064) && (spd <= 1.5))
    cur2->links[2]++;
  else if ((spd > 1.5) && (spd <= 10))
    cur2->links[3]++;
  else if ((spd > 10) && (spd <=40))
    cur2->links[4]++;
  else if ((spd > 40) && (spd <=100))
    cur2->links[5]++;
  else if ((spd > 100) && (spd <= 622))
    cur2->links[6]++;
  else if ((spd > 622) && (spd <= 1000))
    cur2->links[7]++;
  else if ((spd > 1000) && (spd <= 2400))
    cur2->links[8]++;
  else if ((spd > 2400) && (spd <= 10000))
    cur2->links[9]++;
  else if (spd == 0)
    cur2->links[10]++;
  else 
    cur2->links[11]++;
  cur2->seq = cur->seq;
  cur2->ack = cur->ack;
  cur2->win = cur->win;
  cur2->time = cur->time;
  cur2->sec = cur->sec;
  cur2->usec = cur->usec;
  if ((time > 10) && (spd > 0)) {
    cur2->totalspd += spd; 
    cur2->totalcount++;
    cur2->totalspd2 = (cur2->totalspd2 + spd) / 2;
  }
}

/* Catch termination signal(s) and print remaining statistics */
void
cleanup(int signo)
{
  if (signo == SIGALRM) {
    vt_print_bins(&fwd);
    vt_print_bins(&rev);
    return;
  }

#ifdef HAVE_LIBPCAP
  pcap_close(pd);
#endif
  exit(0);
}


#ifdef HAVE_LIBPCAP
/* This routine does the main work.  It steps through the input file
 * and calculates the link speed between each packet pair.  It then
 * increments the proper link bin.
 */
void
print_speed(u_char *user, const struct pcap_pkthdr *h, const u_char *p)
{
  struct ether_header *enet;
  const struct ip *ip;
#if defined(AF_INET6)
  const struct ip6_hdr *ip6;
#endif
  const struct tcphdr *tcp;
  struct spdpair current;

  current.sec = h->ts.tv_sec;
  current.usec = h->ts.tv_usec;
  current.time = (current.sec*1000000) + current.usec;

  enet = (struct ether_header *)p;
  p += sizeof(struct ether_header);   /* move packet pointer past ethernet fields */

  ip = (const struct ip *)p;
#if defined(AF_INET6)
  if (ip->ip_v == 4) {
#endif
  p += (ip->ip_hl) * 4;
  tcp = (const struct tcphdr *)p;
#if defined(AF_INET6)
  current.saddr[0] = ip->ip_src.s_addr;
  current.daddr[0] = ip->ip_dst.s_addr;
#else
  current.saddr = ip->ip_src.s_addr;
  current.daddr = ip->ip_dst.s_addr;
#endif
  current.sport = ntohs(tcp->source);
  current.dport = ntohs(tcp->dest);
  current.seq = ntohl(tcp->seq);
  current.ack = ntohl(tcp->ack_seq);
  current.win = ntohs(tcp->window);

  if (tcp->rst == 1) {
    fprintf(stderr, "Reset packet found, aborting data collection\n");
    vt_print_bins(&fwd);
    vt_print_bins(&rev);
    return;
  }
#if defined(AF_INET6)
  if (fwd.saddr[0] == 0) {
    fprintf(stderr, "Started data collection for sockets %d:%d\n", current.dport,
        current.sport);
    fwd.saddr[0] = current.saddr[0];
    fwd.daddr[0] = current.daddr[0];
#else
  if (fwd.saddr == 0) {
    fprintf(stderr, "Started data collection for sockets %d:%d\n", current.dport,
        current.sport);
    fwd.saddr = current.saddr;
    fwd.daddr = current.daddr;
#endif
    fwd.sport = current.sport;
    fwd.dport = current.dport;
    fwd.st_sec = current.sec;
    fwd.st_usec = current.usec;
#if defined(AF_INET6)
    rev.saddr[0] = current.daddr[0];
    rev.daddr[0] = current.saddr[0];
#else
    rev.saddr = current.daddr;
    rev.daddr = current.saddr;
#endif
    rev.sport = current.dport;
    rev.dport = current.sport;
    rev.st_sec = current.sec;
    rev.st_usec = current.usec;
    log_println(1, "Completed data collection for port %d", current.dport);
    return;
  }

#if defined(AF_INET6)
  if (fwd.saddr[0] == current.saddr[0]) {
#else
  if (fwd.saddr == current.saddr) {
#endif
    if (current.dport == c2sport)
      vt_calculate_spd(&current, &fwd);
    else if (current.sport == s2cport)
      vt_calculate_spd(&current, &fwd);
  }
#if defined(AF_INET6)
  if (rev.saddr[0] == current.saddr[0]) {
#else
  if (rev.saddr == current.saddr) {
#endif
    if (current.sport == c2sport)
      vt_calculate_spd(&current, &rev);
    else if (current.dport == s2cport)
      vt_calculate_spd(&current, &rev);
  }
#if defined(AF_INET6)
  }
  else {
    ip6 = (const struct ip6_hdr *)p;

    p += 40;
    tcp = (const struct tcphdr *)p;
    memcpy(current.saddr, (void *) &ip6->ip6_src, 16);
    memcpy(current.daddr, (void *) &ip6->ip6_dst, 16);

    current.sport = ntohs(tcp->source);
    current.dport = ntohs(tcp->dest);
    current.seq = ntohl(tcp->seq);
    current.ack = ntohl(tcp->ack_seq);
    current.win = ntohs(tcp->window);
    
    if (tcp->rst == 1) {
      fprintf(stderr, "Reset packet found, aborting data collection\n");
      vt_print_bins(&fwd);
      vt_print_bins(&rev);
      return;
    }
    if ((fwd.saddr[0] == 0) && (fwd.saddr[1] == 0) && (fwd.saddr[2] == 0) && (fwd.saddr[3] == 0)) {
      fprintf(stderr, "Started data collection for sockets %d:%d\n", current.dport,
          current.sport);
      memcpy(fwd.saddr, (void *) current.saddr, 16);
      memcpy(fwd.daddr, (void *) current.daddr, 16);
      fwd.sport = current.sport;
      fwd.dport = current.dport;
      fwd.st_sec = current.sec;
      fwd.st_usec = current.usec;
      memcpy(rev.saddr, (void *) current.daddr, 16);
      memcpy(rev.daddr, (void *) current.saddr, 16);
      rev.sport = current.dport;
      rev.dport = current.sport;
      rev.st_sec = current.sec;
      rev.st_usec = current.usec;
      fwd.dec_cnt = 0;
      fwd.inc_cnt = 0;
      fwd.same_cnt = 0;
      fwd.timeout = 0;
      fwd.dupack = 0;
      rev.dec_cnt = 0;
      rev.inc_cnt = 0;
      rev.same_cnt = 0;
      rev.timeout = 0;
      rev.dupack = 0;
      return;
    }

    if ((fwd.saddr[0] == current.saddr[0]) &&
        (fwd.saddr[1] == current.saddr[1]) &&
        (fwd.saddr[2] == current.saddr[2]) &&
        (fwd.saddr[3] == current.saddr[3])) {
      if (current.dport == c2sport)
        vt_calculate_spd(&current, &fwd);
      else if (current.sport == s2cport)
        vt_calculate_spd(&current, &fwd);
    }
    if ((rev.saddr[0] == current.saddr[0]) &&
        (rev.saddr[1] == current.saddr[1]) &&
        (rev.saddr[2] == current.saddr[2]) &&
        (rev.saddr[3] == current.saddr[3])) {
      if (current.sport == c2sport)
        vt_calculate_spd(&current, &rev);
      else if (current.dport == s2cport)
        vt_calculate_spd(&current, &rev);
    }
  }
#endif
}
#endif

int
main(int argc, char **argv)
{

  char *read_file, *cmdbuf, *device;
#ifdef HAVE_LIBPCAP
  pcap_handler printer;
  u_char * pcap_userdata = NULL;
  struct bpf_program fcode;
  char errbuf[PCAP_ERRBUF_SIZE];
#endif
  int cnt, pflag = 0, debug = 0, c;
  struct sigaction new;

  read_file = NULL;
  device = NULL;
  cnt = -1;  /* read forever, or until end of file */
  while ((c = getopt_long(argc, argv, "c:df:hi:l:v", long_options, 0)) != -1) {
    switch (c) {
      case 'c':
        cnt = atoi(optarg);
        break;
      case 'd':
        debug++;
        break;
      case 'f' :
        read_file = optarg;
        break;
      case 'i' :
        device = optarg;
        break;
      case 'h':
#ifdef HAVE_LIBPCAP
        vt_long_usage("ANL/Internet2 NDT version " VERSION " (viewtrace)");
#else
        vt_long_usage("ANL/Internet2 NDT version " VERSION " (viewtrace) [missing pcap library]");
#endif
        break;
      case 'v':
#ifdef HAVE_LIBPCAP
        printf("ANL/Internet2 NDT version %s (viewtrace)\n", VERSION);
#else
        printf("ANL/Internet2 NDT version %s (viewtrace) [missing pcap library]\n", VERSION);
#endif
        exit(0);
        break;
      case 303:
        if (check_int(optarg, &c2sport)) {
          char tmpText[200];
          snprintf(tmpText, 200, "Invalid C2S throughput test port number: %s", optarg);
          short_usage(argv[0], tmpText);
        }
        break;
      case 304:
        if (check_int(optarg, &s2cport)) {
          char tmpText[200];
          snprintf(tmpText, 200, "Invalid S2C throughput test port number: %s", optarg);
          short_usage(argv[0], tmpText); 
        }
        break;
      case '?':
        short_usage(argv[0], "");
        break;
    }
  }

  if (optind < argc) {
    short_usage(argv[0], "Unrecognized non-option elements");
  }

  log_init(argv[0], debug);

#ifdef HAVE_LIBPCAP
  init_vars(&fwd);
  init_vars(&rev);

  if (read_file == NULL) {
    if (device == NULL) {
      device = pcap_lookupdev(errbuf);
      if (device == NULL) {
        fprintf(stderr, "pcap_lookupdev failed: %s\n", errbuf);
        exit (-1);
      }
    }
    if ((pd = pcap_open_live(device, 68, !pflag, 1000, errbuf)) == NULL) {
      fprintf(stderr, "pcap_open_live failed: %s\n", errbuf);
      exit (-9);
    }
    memset(&new, 0, sizeof(new));
    new.sa_handler = cleanup;
    sigaction(SIGTERM, &new, NULL);
    sigaction(SIGINT, &new, NULL);
    sigaction(SIGALRM, &new, NULL);

  }
  else {
    if ((pd = pcap_open_offline(read_file, errbuf)) == NULL) {
      fprintf(stderr, "pcap_open_offline failed: %s\n", errbuf);
      exit (-2);
    }
  }

  cmdbuf = copy_argv(&argv[optind]);

  if (pcap_compile(pd, &fcode, cmdbuf, 0, 0xFFFFFF00) < 0) {
    fprintf(stderr, "pcap_compile failed %s\n", pcap_geterr(pd));
    exit(-2);
  }

  if (pcap_setfilter(pd, &fcode) < 0) {
    fprintf(stderr, "pcap_setfiler failed %s\n", pcap_geterr(pd));
    exit (-2);
  }

  printer = (pcap_handler) print_speed;
  if (pcap_loop(pd, cnt, printer, pcap_userdata) < 0) {
    fprintf(stderr, "pcap_loop failed %s\n", pcap_geterr(pd));
    exit(-2);
  }
  if (fwd.sport == s2cport) {
    vt_print_bins(&rev);
    vt_print_bins(&fwd);
  } else {
    vt_print_bins(&fwd);
    vt_print_bins(&rev);
  }
  pcap_close(pd);
#else
  log_println(0, "\n!!! In order to use viewtrace utility you have to compile it with the pcap library !!!\n");
#endif
  return 0;
}
