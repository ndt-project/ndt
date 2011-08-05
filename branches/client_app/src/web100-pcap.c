/*
 * This file contains the libpcap routines needed by the web100srv
 * program.  The global headers and variables are defined in the
 * web100srv.h file. This should make it easier to maintain the
 * web100srv program.
 *
 * Richard Carlson 3/10/04
 * rcarlson@internet2.edu
 */

#include "../config.h"

#include <assert.h>

/* local include file contains needed structures */
#include "web100srv.h"
#include "network.h"
#include "logging.h"
#include <net/if.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>

int dumptrace;
pcap_t *pd;
pcap_dumper_t *pdump;
int mon_pipe1[2], mon_pipe2[2];
/* int sig1, sig2; */
int sigj=0, sigk=0; 
int ifspeed;

void get_iflist(void) 
{
	  /* pcap_addr_t *ifaceAddr; */
  pcap_if_t *alldevs, *dp;
  struct ethtool_cmd ecmd;
  int fd, cnt, i, err;
  struct ifreq ifr;
  char errbuf[256];

  /* scan through the interface device list and get the names/speeds of each
 *    * if.  The speed data can be used to cap the search for the bottleneck link
 *       * capacity.  The intent is to reduce the impact of interrupt coalescing on 
 *          * the bottleneck link detection algorithm
 *             * RAC 7/14/09
 *                */
  cnt=0;
  if (pcap_findalldevs(&alldevs, errbuf) == 0) {
    for (dp=alldevs; dp!=NULL; dp=dp->next) {
      memcpy(iflist.name[cnt++], dp->name, strlen(dp->name));
    }
  }
  for (i=0; i<cnt; i++) {
    if (strncmp((char *)iflist.name[i], "eth", 3) != 0)
      continue;
    memset(&ifr, 0, sizeof(ifr));
    memcpy(ifr.ifr_name, (char *)iflist.name[i], strlen(iflist.name[i]));
    /* strcpy(ifr.ifr_name, iflist.name[i]); */
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    ecmd.cmd = ETHTOOL_GSET;
    ifr.ifr_data = (caddr_t) &ecmd;
    err = ioctl(fd, SIOCETHTOOL, &ifr);
    if (err == 0) {
        switch (ecmd.speed) {
          case SPEED_10:
            iflist.speed[i] = 3;
            break;
          case SPEED_100:
            iflist.speed[i] = 5;
            break;
          case SPEED_1000:
            iflist.speed[i] = 7;
            break;
          case SPEED_10000:
            iflist.speed[i] = 9;
            break;
	  default :
	    iflist.speed[i] = 0;
      }
    }
  }
  if (alldevs != NULL)
    pcap_freealldevs(alldevs);
}


int
check_signal_flags()
{
  if ((sig1 == 1) || (sig2 == 1)) {
    log_println(5, "Received SIGUSRx signal terminating data collection loop for pid=%d", getpid());
    if (sig1 == 1) {
      log_println(4, "Sending pkt-pair data back to parent on pipe %d, %d", mon_pipe1[0], mon_pipe1[1]);
    if (get_debuglvl() > 3) {
#ifdef AF_INET6
        if (fwd.family == 4) {
          fprintf(stderr, "fwd.saddr = %x:%d, rev.saddr = %x:%d\n",
              fwd.saddr[0], fwd.sport, rev.saddr[0], rev.sport);
#else
          fprintf(stderr, "fwd.saddr = %x:%d, rev.saddr = %x:%d\n",
              fwd.saddr, fwd.sport, rev.saddr, rev.sport);
#endif
#ifdef AF_INET6
        }
        else if (fwd.family == 6) {
          char str[136];
          memset(str, 0, 136);
          inet_ntop(AF_INET6, (void *) fwd.saddr, str, sizeof(str));
          fprintf(stderr, "fwd.saddr = %s:%d", str, fwd.sport);
          memset(str, 0, 136);
          inet_ntop(AF_INET6, (void *) rev.saddr, str, sizeof(str));
          fprintf(stderr, ", rev.saddr = %s:%d\n", str, rev.sport);
        }
        else {
          fprintf(stderr, "check_signal_flags: Unknown IP family (%d)\n", fwd.family);
        }
#endif
      }
      print_bins(&fwd, mon_pipe1);
      usleep(30000);	/* wait here 30 msec, for parent to read this data */
      print_bins(&rev, mon_pipe1);
      usleep(30000);	/* wait here 30 msec, for parent to read this data */
      if (pd != NULL)
        pcap_close(pd);
      if (dumptrace == 1)
        pcap_dump_close(pdump);
      sig1 = 2;
    }

    if (sig2 == 1) {
      log_println(4, "Sending pkt-pair data back to parent on pipe %d, %d", mon_pipe2[0], mon_pipe2[1]);
      if (get_debuglvl() > 3) {
#ifdef AF_INET6
        if (fwd.family == 4) {
          fprintf(stderr, "fwd.saddr = %x:%d, rev.saddr = %x:%d\n",
              fwd.saddr[0], fwd.sport, rev.saddr[0], rev.sport);
#else
          fprintf(stderr, "fwd.saddr = %x:%d, rev.saddr = %x:%d\n",
              fwd.saddr, fwd.sport, rev.saddr, rev.sport);
#endif
#ifdef AF_INET6
        }
        else if (fwd.family == 6) {
          char str[136];
          memset(str, 0, 136);
          inet_ntop(AF_INET6, (void *) fwd.saddr, str, sizeof(str));
          fprintf(stderr, "fwd.saddr = %s:%d", str, fwd.sport);
          memset(str, 0, 136);
          inet_ntop(AF_INET6, (void *) rev.saddr, str, sizeof(str));
          fprintf(stderr, ", rev.saddr = %s:%d\n", str, rev.sport);
        }
        else {
          fprintf(stderr, "check_signal_flags: Unknown IP family (%d)\n", fwd.family);
        }
#endif
      }
      print_bins(&fwd, mon_pipe2);
      usleep(30000);	/* wait here 30 msec, for parent to read this data */
      print_bins(&rev, mon_pipe2);
      usleep(30000);	/* wait here 30 msec, for parent to read this data */
      if (pd != NULL)
        pcap_close(pd);
      if (dumptrace == 1)
        pcap_dump_close(pdump);
      sig2 = 2;
    }
    log_println(6, "Finished reading pkt-pair data from network, process %d should terminate now", getpid());
    return 1;
  }
  return 0;
}

/* initialize variables before starting to accumlate data */
void init_vars(struct spdpair *cur)
{

	int i;

  assert(cur);

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
void print_bins(struct spdpair *cur, int monitor_pipe[2])
{

	int i, total=0, max=0, s, index = -1;
	char buff[256];
	int tzoffset = 6;
	FILE *fp;
	int j;

  assert(cur);

	/* the tzoffset value is fixed for CST (6), use 5 for CDT.  The code needs to find the
	 * current timezone and use that value here! */
	s = (cur->st_sec - (tzoffset * 3600)) %86400; 
	fp = fopen(get_logfile(), "a");
  log_print(1, "%02d:%02d:%02d.%06u   ", s / 3600, (s % 3600) / 60, s % 60, cur->st_usec);

	for (i=0; i<16; i++) {
	    total += cur->links[i];
	    if (cur->links[i] > max) {
		max = cur->links[i];
	    }
	}
	if (get_debuglvl() > 2) {
#ifdef AF_INET6
    if (cur->family == 4) {
      if (fp) {
        fprintf(fp, "%u.%u.%u.%u:%d --> ", (cur->saddr[0] & 0xFF), ((cur->saddr[0] >> 8) & 0xff),
            ((cur->saddr[0] >> 16) & 0xff),  (cur->saddr[0] >> 24), cur->sport);
        fprintf(fp, "%u.%u.%u.%u:%d  ", (cur->daddr[0] & 0xFF), ((cur->daddr[0] >> 8) & 0xff),
            ((cur->daddr[0] >> 16) & 0xff),  (cur->daddr[0] >> 24), cur->dport);
      }
      log_print(3, "%u.%u.%u.%u:%d --> ", (cur->saddr[0] & 0xFF), ((cur->saddr[0] >> 8) & 0xff),
          ((cur->saddr[0] >> 16) & 0xff),  (cur->saddr[0] >> 24), cur->sport);
      log_print(3, "%u.%u.%u.%u:%d ", (cur->daddr[0] & 0xFF), ((cur->daddr[0] >> 8) & 0xff),
          ((cur->daddr[0] >> 16) & 0xff),  (cur->daddr[0] >> 24), cur->dport);
    }
#else
    if (fp) {
      fprintf(fp, "%u.%u.%u.%u:%d --> ", (cur->saddr & 0xFF), ((cur->saddr >> 8) & 0xff),
          ((cur->saddr >> 16) & 0xff),  (cur->saddr >> 24), cur->sport);
      fprintf(fp, "%u.%u.%u.%u:%d  ", (cur->daddr & 0xFF), ((cur->daddr >> 8) & 0xff),
          ((cur->daddr >> 16) & 0xff),  (cur->daddr >> 24), cur->dport);
    }
    log_print(3, "%u.%u.%u.%u:%d --> ", (cur->saddr & 0xFF), ((cur->saddr >> 8) & 0xff),
        ((cur->saddr >> 16) & 0xff),  (cur->saddr >> 24), cur->sport);
    log_print(3, "%u.%u.%u.%u:%d ", (cur->daddr & 0xFF), ((cur->daddr >> 8) & 0xff),
        ((cur->daddr >> 16) & 0xff),  (cur->daddr >> 24), cur->dport);
#endif
#ifdef AF_INET6
    else {
      char name[200];
      socklen_t len;
      memset(name, 0, 200);
      len = 199;
      inet_ntop(AF_INET6, cur->saddr, name, len);
      if (fp) {
        fprintf(fp, "%s:%d --> ", name, cur->sport);
      }
      log_print(3, "%s:%d --> ", name, cur->sport);
      memset(name, 0, 200);
      len = 199;
      inet_ntop(AF_INET6, cur->daddr, name, len);
      if (fp) {
        fprintf(fp, "%s:%d  ", name, cur->dport);
      }
      log_print(3, "%s:%d  ", name, cur->dport);
    }
#endif
	}
  if (max == 0) {
    log_println(3, "No data Packets collected");
    if (get_debuglvl() > 2) {
      if (fp) {
        fprintf(fp, "\n\tNo packets collected\n");
      }
    }
    for (i=0; i<16; i++)
      cur->links[i] = -1;
    index = -1;
  }
  log_println(3, "Collected pkt-pair data max = %d", max);
	if (max == cur->links[8]) {
	    max = 0;
	    for (i=0; i<10; i++) {
		if (max < cur->links[i]) {
		    index = i;
		    max = cur->links[i];
		}
	    }
	}
  if (fp) {
    if (get_debuglvl() > 2) {
      switch (index) {
        case -1:	fprintf(fp, "link=System Fault; ");
                  break;
        case 0:	fprintf(fp, "link=RTT; ");
                break;
        case 1:	fprintf(fp, "link=dial-up; ");
                break;
        case 2:	fprintf(fp, "link=T1; ");
                break;
        case 3:	fprintf(fp, "link=Enet; ");
                break;
        case 4:	fprintf(fp, "link=T3; ");
                break;
        case 5:	fprintf(fp, "link=FastE; ");
                break;
        case 6:	fprintf(fp, "link=OC-12; ");
                break;
        case 7:	fprintf(fp, "link=GigE; ");
                break;
        case 8:	fprintf(fp, "link=OC-48; ");
                break;
        case 9:	fprintf(fp, "link=10 GigE; ");
                break;
        case 10:	fprintf(fp, "retransmission; ");
                  break;
        case 11:	fprintf(fp, "link=unknown; ");
                  break;
      }

      fprintf(fp, "packets=%d\n", total);
      fprintf(fp, "Running Average = %0.2f Mbps  ", cur->totalspd2);
      fprintf(fp, "Average speed = %0.2f Mbps\n", cur->totalspd/cur->totalcount);
      fprintf(fp, "\tT1=%d (%0.2f%%); ", cur->links[2], ((float) cur->links[2]*100/total));
      fprintf(fp, "Ethernet=%d (%0.2f%%); ", cur->links[3], ((float) cur->links[3]*100/total));
      fprintf(fp, "T3=%d (%0.2f%%); ", cur->links[4], ((float) cur->links[4]*100/total));
      fprintf(fp, "FastEthernet=%d (%0.2f%%);\n", cur->links[5], ((float) cur->links[5]*100/total));
      fprintf(fp, "OC-12=%d (%0.2f%%); ", cur->links[6], ((float) cur->links[6]*100/total));
      fprintf(fp, "\tGigabit Ethernet=%d (%0.2f%%); ", cur->links[7], ((float) cur->links[7]*100/total));
      fprintf(fp, "OC-48=%d (%0.2f%%); ", cur->links[8], ((float) cur->links[8]*100/total));
      fprintf(fp, "10 Gigabit Enet=%d (%0.2f%%);\n", cur->links[9], ((float) cur->links[9]*100/total));
      fprintf(fp, "\tRetransmissions=%d (%0.2f%%); ", cur->links[10], ((float) cur->links[10]*100/total));
      fprintf(fp, "Unknown=%d (%0.2f%%);\n", cur->links[11], ((float) cur->links[11]*100/total));
    }
    fclose(fp);
  }

	sprintf(buff, "  %d %d %d %d %d %d %d %d %d %d %d %d %0.2f %d %d %d %d %d %d",
		cur->links[0], cur->links[1], cur->links[2], cur->links[3], cur->links[4],
		cur->links[5], cur->links[6], cur->links[7], cur->links[8], cur->links[9],
		cur->links[10], cur->links[11], cur->totalspd2, cur->inc_cnt, cur->dec_cnt,
		cur->same_cnt, cur->timeout, cur->dupack, ifspeed);
	for (j=0; j<5; j++) {
	  i = write(monitor_pipe[1], buff, strlen(buff));
	  if (i == strlen(buff))
	    break;
	  if ((i == -1) && (errno == EINTR))
	    continue;
	}
        log_println(6, "wrote %d bytes: link counters are '%s'", i, buff);
        log_println(6, "#$#$#$#$ pcap routine says window increases = %d, decreases = %d, no change = %d",
       			 cur->inc_cnt, cur->dec_cnt, cur->same_cnt);
}

void calculate_spd(struct spdpair *cur, struct spdpair *cur2, int portA, int portB)
{
	
	float bits = 0, spd, time;

  assert(cur);
  assert(cur2);

	time = (((cur->sec - cur2->sec)*1000000) + (cur->usec - cur2->usec));
	/* time = curt->time - cur2->time; */
	if ((cur->dport == portA) || (cur->sport == portB)) {
	    if (cur->seq >= cur2->seq)
		bits = (cur->seq - cur2->seq) * 8;
	    else
		bits = 0;
	    if (time > 200000) {
	        cur2->timeout++;
	    }
	} else  {
	    if (cur->ack > cur2->ack)
		bits = (cur->ack - cur2->ack) * 8;
	    else if (cur->ack == cur2->ack)
		cur2->dupack++;
	    else
		bits = 0;
	    if (cur->win > cur2->win)
		cur2->inc_cnt++;
	    if (cur->win == cur2->win)
		cur2->same_cnt++;
	    if (cur->win < cur2->win)
		cur2->dec_cnt++;
	}
	spd = (bits/time);			/* convert to mbits/sec) */
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

/* This routine does the main work of reading packets from the network
 * interface. It steps through the input file and calculates the link
 * speed between each packet pair.  It then increments the proper link
 * bin.
 */

void
print_speed(u_char *user, const struct pcap_pkthdr *h, const u_char *p)
{

  struct ether_header *enet;
  const struct ip *ip = NULL;
  PortPair* pair = (PortPair*) user;
#if defined(AF_INET6)
  const struct ip6_hdr *ip6;
#endif
  const struct tcphdr *tcp;
  struct spdpair current;
  int port2 = pair->port1;
  int port4 = pair->port2;

  assert(user);

  if (dumptrace == 1)
      pcap_dump((u_char *)pdump, h, p);

  if (pd == NULL) {
    fprintf(stderr, "!#!#!#!# Error, trying to process IF data, but pcap fd closed\n");
    return;
  }

  if (check_signal_flags()) {
    return;
  }

  current.sec = h->ts.tv_sec;
  current.usec = h->ts.tv_usec;
  current.time = (current.sec*1000000) + current.usec;

  enet = (struct ether_header *)p;
  p += sizeof(struct ether_header);   /* move packet pointer past ethernet fields */

  ip = (const struct ip *)p;
#if defined(AF_INET6)
  if (ip->ip_v == 4) {
#endif

/* This section grabs the IP & TCP header values from an IPv4 packet and loads the various
 * variables with the packet's values.  
 */
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

/* the current structure now has copies of the IP/TCP header values, if this is the
 * first packet, then there is nothing to compare them to, so just finish the initialization
 * step and return.
 */

    if (fwd.seq == 0) {
      log_println(1, "New IPv4 packet trace started -- initializing counters");
      fwd.seq = current.seq;
      
      fwd.st_sec = current.sec;
      fwd.st_usec = current.usec;
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
      fwd.family = 4;
      rev.family = 4;
    return;
  }

/* a new IPv4 packet has been received and it isn't the 1st one, so calculate the bottleneck link
 * capacity based on the times between this packet and the previous one.
 */

#if defined(AF_INET6)
    if (fwd.saddr[0] == current.saddr[0]) {
#else
    if (fwd.saddr == current.saddr) {
#endif
      if (current.dport == port2) {
        calculate_spd(&current, &fwd, port2, port4);
        return;
      }
      if (current.sport == port4) {
        calculate_spd(&current, &fwd, port2, port4);
        return;
      }
    }
#if defined(AF_INET6)
    if (rev.saddr[0] == current.saddr[0]) {
#else
    if (rev.saddr == current.saddr) {
#endif
      if (current.sport == port2) {
        calculate_spd(&current, &rev, port2, port4);
        return;
      }
      if (current.dport == port4) {
        calculate_spd(&current, &rev, port2, port4);
        return;
      }
    }
#if defined(AF_INET6)
  }
  else { 	/*  IP header value is not = 4, so must be IPv6 */

/* This is an IPv6 packet, grab the IP & TCP header values for further use.
 */

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

/* The 1st packet has been received, finish the initialization process and return.
 */

    /* if ((fwd.saddr[0] == 0) && (fwd.saddr[1] == 0) && (fwd.saddr[2] == 0) && (fwd.saddr[3] == 0)) { */
    if (fwd.seq == 0) {
      log_println(1, "New IPv6 packet trace started -- initializing counters");
      fwd.seq = current.seq;
      fwd.st_sec = current.sec;
      fwd.st_usec = current.usec;
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
      fwd.family = 6;
      rev.family = 6;
      return;
    }

/* A new packet has been recieved, and it's not the 1st.  Use it to calculate the
 * bottleneck link capacity for this flow.
 */

    if ((fwd.saddr[0] == current.saddr[0]) &&
        (fwd.saddr[1] == current.saddr[1]) &&
        (fwd.saddr[2] == current.saddr[2]) &&
        (fwd.saddr[3] == current.saddr[3])) {
      if (current.dport == port2) { 
            calculate_spd(&current, &fwd, port2, port4);
        return;
      }
      if (current.sport == port4) { 
            calculate_spd(&current, &fwd, port2, port4);
        return;
      }
    }
    if ((rev.saddr[0] == current.saddr[0]) &&
        (rev.saddr[1] == current.saddr[1]) &&
        (rev.saddr[2] == current.saddr[2]) &&
        (rev.saddr[3] == current.saddr[3])) {
      if (current.sport == port2) { 
            calculate_spd(&current, &rev, port2, port4);
        return;
      }
      if (current.dport == port4) { 
            calculate_spd(&current, &rev, port2, port4);
        return;
      }
    }
  }
#endif

/* a packet has been recieved, so it matched the filter, but the src/dst ports are backward for some reason.
 * Need to fix this by reversing the values.
 */

  if (sigk == 0) {
    sigk++;
    log_println(6, "Fault: unknown packet received with src/dst port = %d/%d", current.sport, current.dport);
  }
  if (sigj == 0) {
      log_println(6, "Ports need to be reversed now port1/port2 = %d/%d", pair->port1, pair->port2);
      int tport = pair->port1;
      pair->port1 = pair->port2;
      pair->port2 = tport;;
      fwd.st_sec = current.sec;
      fwd.st_usec = current.usec;
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
      log_println(6, "Ports should have been reversed now port1/port2 = %d/%d", pair->port1, pair->port2);
      sigj = 1;
  }
}

/* This routine performs the open and initialization functions needed
 * by the libpcap routines.  The print_speed function above, is passed
 * to the pcap_open_live() function as the pcap_handler.
 */

void
init_pkttrace(I2Addr srcAddr, struct sockaddr *sock_addr, socklen_t saddrlen, int monitor_pipe[2],
    char *device, PortPair* pair, char *direction, int compress)
{
  char cmdbuf[256], dir[256];
  pcap_handler printer;
  u_char * pcap_userdata=(u_char*) pair;
  struct bpf_program fcode;
  char errbuf[PCAP_ERRBUF_SIZE];
  int cnt, pflag=0, i;
  char c;
  char namebuf[200], isoTime[64];
  size_t nameBufLen=199;
  I2Addr sockAddr=NULL;
  struct sockaddr *src_addr;
  pcap_if_t *alldevs, *dp;
  pcap_addr_t *curAddr;
  DIR *dip;
  int rc;

  cnt = -1;  /* read forever, or until end of file */
  sig1 = 0;
  sig2 = 0;
  

  init_vars(&fwd);
  init_vars(&rev);

  sockAddr = I2AddrBySAddr(get_errhandle(), sock_addr, saddrlen, 0, 0);
  sock_addr = I2AddrSAddr(sockAddr, 0);
  src_addr = I2AddrSAddr(srcAddr, 0);
  /* special check for localhost, set device accordingly */
  if (I2SockAddrIsLoopback(sock_addr, saddrlen) > 0)
    strncpy(device, "lo", 3);

  if (device == NULL) {
    if (pcap_findalldevs(&alldevs, errbuf) == 0) {
	for (dp=alldevs; dp!=NULL; dp=dp->next) {
	    for (curAddr=dp->addresses; curAddr!=NULL; curAddr=curAddr->next) {
		switch (curAddr->addr->sa_family) {
		    case AF_INET:
  			memset(namebuf, 0, 200);
			inet_ntop(AF_INET, &((struct sockaddr_in *)curAddr->addr)->sin_addr,
					namebuf, INET_ADDRSTRLEN);
			 log_println(3, "IPv4 interface found address=%s", namebuf);
			if (((struct sockaddr_in *)curAddr->addr)->sin_addr.s_addr ==
					((struct sockaddr_in *)src_addr)->sin_addr.s_addr) {
			    log_println(4, "IPv4 address match, setting device to '%s'", dp->name);
			    device = dp->name;
			    ifspeed = -1;
			    for(i=0; iflist.name[0][i]!='0'; i++) {
				if (strncmp((char *)iflist.name[i], device, 4) == 0) {
				    ifspeed = iflist.speed[i];
				    break;
				}
			    }
				
			    if (direction[0] == 's') {
#if defined(AF_INET6)
      			    	fwd.saddr[0] = ((struct sockaddr_in *)src_addr)->sin_addr.s_addr;
      			    	fwd.daddr[0] = ((struct sockaddr_in *)sock_addr)->sin_addr.s_addr;
      			    	rev.saddr[0] = ((struct sockaddr_in *)sock_addr)->sin_addr.s_addr;
      			    	rev.daddr[0] = ((struct sockaddr_in *)src_addr)->sin_addr.s_addr;
#else
      			    	fwd.saddr = ((struct sockaddr_in *)src_addr)->sin_addr.s_addr;
      			    	fwd.daddr = ((struct sockaddr_in *)sock_addr)->sin_addr.s_addr;
      			    	rev.saddr = ((struct sockaddr_in *)sock_addr)->sin_addr.s_addr;
      			    	rev.daddr = ((struct sockaddr_in *)src_addr)->sin_addr.s_addr;
#endif
      			    	fwd.sport = ntohs(((struct sockaddr_in *)src_addr)->sin_port);
      			    	fwd.dport = ntohs(((struct sockaddr_in *)sock_addr)->sin_port);
      			    	rev.sport = ntohs(((struct sockaddr_in *)sock_addr)->sin_port);
      			    	rev.dport = ntohs(((struct sockaddr_in *)src_addr)->sin_port);
			    } else {
#if defined(AF_INET6)
      			    	rev.saddr[0] = ((struct sockaddr_in *)src_addr)->sin_addr.s_addr;
      			    	rev.daddr[0] = ((struct sockaddr_in *)sock_addr)->sin_addr.s_addr;
      			    	fwd.saddr[0] = ((struct sockaddr_in *)sock_addr)->sin_addr.s_addr;
      			    	fwd.daddr[0] = ((struct sockaddr_in *)src_addr)->sin_addr.s_addr;
#else
      			    	rev.saddr = ((struct sockaddr_in *)src_addr)->sin_addr.s_addr;
      			    	rev.daddr = ((struct sockaddr_in *)sock_addr)->sin_addr.s_addr;
      			    	fwd.saddr = ((struct sockaddr_in *)sock_addr)->sin_addr.s_addr;
      			    	fwd.daddr = ((struct sockaddr_in *)src_addr)->sin_addr.s_addr;
#endif
      			    	rev.sport = ntohs(((struct sockaddr_in *)src_addr)->sin_port);
      			    	rev.dport = ntohs(((struct sockaddr_in *)sock_addr)->sin_port);
      			    	fwd.sport = ntohs(((struct sockaddr_in *)sock_addr)->sin_port);
      			    	fwd.dport = ntohs(((struct sockaddr_in *)src_addr)->sin_port);
			    }
			    goto endLoop;
			}
			break;
#if defined(AF_INET6)
		    case AF_INET6:
  			memset(namebuf, 0, 200);
			inet_ntop(AF_INET6, &((struct sockaddr_in6 *)curAddr->addr)->sin6_addr,
					namebuf, INET6_ADDRSTRLEN);
  			/* I2AddrNodeName(srcAddr, namebuf, &nameBufLen); */
			log_println(3, "IPv6 interface found address=%s", namebuf);
			if (memcmp(((struct sockaddr_in6 *)curAddr->addr)->sin6_addr.s6_addr,
					((struct sockaddr_in6 *)src_addr)->sin6_addr.s6_addr,
					16) == 0) {
			    log_println(4, "IPv6 address match, setting device to '%s'", dp->name);
			    device = dp->name;
				
			    if (direction[0] == 's') {
      			    	memcpy(fwd.saddr, ((struct sockaddr_in6 *)src_addr)->sin6_addr.s6_addr, 16);
      			    	memcpy(fwd.daddr, ((struct sockaddr_in6 *)sock_addr)->sin6_addr.s6_addr, 16);
      			    	memcpy(rev.saddr, ((struct sockaddr_in6 *)sock_addr)->sin6_addr.s6_addr, 16);
      			    	memcpy(rev.daddr, ((struct sockaddr_in6 *)src_addr)->sin6_addr.s6_addr, 16);
      			    	fwd.sport = ntohs(((struct sockaddr_in6 *)src_addr)->sin6_port);
      			    	fwd.dport = ntohs(((struct sockaddr_in6 *)sock_addr)->sin6_port);
      			    	rev.sport = ntohs(((struct sockaddr_in6 *)sock_addr)->sin6_port);
      			    	rev.dport = ntohs(((struct sockaddr_in6 *)src_addr)->sin6_port);
			    } else {
      			    	memcpy(rev.saddr, ((struct sockaddr_in6 *)src_addr)->sin6_addr.s6_addr, 16);
      			    	memcpy(rev.daddr, ((struct sockaddr_in6 *)sock_addr)->sin6_addr.s6_addr, 16);
      			    	memcpy(fwd.saddr, ((struct sockaddr_in6 *)sock_addr)->sin6_addr.s6_addr, 16);
      			    	memcpy(fwd.daddr, ((struct sockaddr_in6 *)src_addr)->sin6_addr.s6_addr, 16);
      			    	rev.sport = ntohs(((struct sockaddr_in6 *)src_addr)->sin6_port);
      			    	rev.dport = ntohs(((struct sockaddr_in6 *)sock_addr)->sin6_port);
      			    	fwd.sport = ntohs(((struct sockaddr_in6 *)sock_addr)->sin6_port);
      			    	fwd.dport = ntohs(((struct sockaddr_in6 *)src_addr)->sin6_port);
			    }
			    goto endLoop;
			}
			break;
#endif
		    default:
			log_println(4, "Unknown address family=%d found", curAddr->addr->sa_family);
		}
	    }
	}
    } 
  }
endLoop:

  /*  device = pcap_lookupdev(errbuf); */
    if (device == NULL) {
      fprintf(stderr, "pcap_lookupdev failed: %s\n", errbuf);
    }

  log_println(1, "Opening network interface '%s' for packet-pair timing", device);

  if ((pd = pcap_open_live(device, 68, !pflag, 1000, errbuf)) == NULL) {
    fprintf(stderr, "pcap_open_live failed: %s\n", errbuf);
  }

  log_println(2, "pcap_open_live() returned pointer 0x%x", (int) pd);

  memset(namebuf, 0, 200);
  I2AddrNodeName(sockAddr, namebuf, &nameBufLen);
  memset(cmdbuf, 0, 256);
  sprintf(cmdbuf, "host %s and port %d", namebuf, I2AddrPort(sockAddr));

  log_println(1, "installing pkt filter for '%s'", cmdbuf);
  log_println(1, "Initial pkt src data = %x", (int) fwd.saddr);

  if (pcap_compile(pd, &fcode, cmdbuf, 0, 0xFFFFFF00) < 0) {
    fprintf(stderr, "pcap_compile failed %s\n", pcap_geterr(pd));
    return;
  }

  if (pcap_setfilter(pd, &fcode) < 0) {
    fprintf(stderr, "pcap_setfiler failed %s\n", pcap_geterr(pd));
    return;
  }

  if (dumptrace == 1) {
    fprintf(stderr, "Creating trace file for connection\n");
    memset(cmdbuf, 0, 256);
    strncpy(cmdbuf, DataDirName, strlen(DataDirName));
    if ((dip = opendir(cmdbuf)) == NULL && errno == ENOENT)
	mkdir(cmdbuf, 0755);
    closedir(dip);
    get_YYYY(dir);
    strncat(cmdbuf, dir, 4); 
    if ((dip = opendir(cmdbuf)) == NULL && errno == ENOENT)
	mkdir(cmdbuf, 0755);
    closedir(dip);
    strncat(cmdbuf, "/", 1);
    get_MM(dir);
    strncat(cmdbuf, dir, 2); 
    if ((dip = opendir(cmdbuf)) == NULL && errno == ENOENT)
	mkdir(cmdbuf, 0755);
    closedir(dip);
    strncat(cmdbuf, "/", 1);
    get_DD(dir);
    strncat(cmdbuf, dir, 2); 
    if ((dip = opendir(cmdbuf)) == NULL && errno == ENOENT)
	mkdir(cmdbuf, 0755);
    closedir(dip);
    strncat(cmdbuf, "/", 1);
    sprintf(dir, "%s_%s:%d.%s_ndttrace", get_ISOtime(isoTime), namebuf, I2AddrPort(sockAddr), direction);
    strncat(cmdbuf, dir, strlen(dir));
    pdump = pcap_dump_open(pd, cmdbuf);
    fprintf(stderr, "Opening '%s' log fine\n", cmdbuf);
    if (pdump == NULL) {
      fprintf(stderr, "Unable to create trace file '%s'\n", cmdbuf);
      dumptrace = 0;
    }
  }

  printer = (pcap_handler) print_speed;
  if (dumptrace == 0) {
     for (i=0; i<5; i++) {
       rc = write(monitor_pipe[1], "Ready", 6); 
       if (rc == 6)
	 break;
       if ((rc == -1) && (errno == EINTR))
	 continue;
     }
  } else {
     for (i=0; i<5; i++) {
       rc = write(monitor_pipe[1], dir, strlen(dir));
       if (rc == strlen(dir))
	 break;
       if ((rc == -1) && (errno == EINTR))
	 continue;
     }
  }

  /* kill process off if parent doesn't send a signal. */
  alarm(60);

  if (pcap_loop(pd, cnt, printer, pcap_userdata) < 0) {
    log_println(5, "pcap_loop exited %s", pcap_geterr(pd));
  }
  log_println(5, "Pkt-Pair data collection ended, waiting for signal to terminate process");
  /*    Temporarily remove these free statements, The memory should be free'd when
   *      the child process terminates, so we don't have a leak.  There might be a bug in
   *      the pcap lib (on-line reference found from 2003) and on 10/14/09 I was seeing
   *      child crashes with a traceback pointing to the freecode() routine inside the pcap-lib
   *      as the culprit.  RAC 10/15/09
   *
   * if (alldevs != NULL)
   *    pcap_freealldevs(alldevs);
   *  if (&fcode != NULL)
   *    pcap_freecode(&fcode);
   */
  free(sockAddr);

  if (sig1 == 2) {
    while ((read(mon_pipe1[0], &c, 1)) < 0)
      ;
    close(mon_pipe1[0]);
    close(mon_pipe1[1]);
    sig1 = 0;
  }
  if (sig2 == 2) {
    while ((read(mon_pipe2[0], &c, 1)) < 0)
      ;
    sleep(2);
    close(mon_pipe2[0]);
    close(mon_pipe2[1]);
    sig2 = 0;
  }

  log_println(8, "Finally Finished reading data from network, process %d should terminate now", getpid());
}
