/*
 * This file contains the libpcap routines needed by the web100srv
 * program.  The global headers and variables are defined in the
 * web100srv.h file. This should make it easier to maintain the
 * web100srv program.
 *
 * Richard Carlson 3/10/04
 * rcarlson@internet2.edu
 *
 */

/* local include file contains needed structures */
#include "web100srv.h"
#include "../config.h"
#include "network.h"
#include "logging.h"

int dumptrace;
pcap_t *pd;
pcap_dumper_t *pdump;
int mon_pipe1[2], mon_pipe2[2];
int sig1, sig2;

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
      print_bins(&rev, mon_pipe1);
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
      print_bins(&rev, mon_pipe2);
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

	/* the tzoffset value is fixed for CST (6), use 5 for CDT.  The code needs to find the
	 * current timezone and use that value here! */
	s = (cur->st_sec - (tzoffset * 3600)) %86400; 
	fp = fopen(get_logfile(), "a");
  /* FIXME: no \n in log_println? */
  log_println(1, "%02d:%02d:%02d.%06u   ", s / 3600, (s % 3600) / 60, s % 60, cur->st_usec);

	for (i=0; i<16; i++) {
	    total += cur->links[i];
	    if (cur->links[i] > max) {
		max = cur->links[i];
	    }
	}
  /* FIXME: use log_println here */
	if (get_debuglvl() > 2) {
#ifdef AF_INET6
    if (cur->family == 4) {
      fprintf(fp, "%u.%u.%u.%u:%d --> ", (cur->saddr[0] & 0xFF), ((cur->saddr[0] >> 8) & 0xff),
          ((cur->saddr[0] >> 16) & 0xff),  (cur->saddr[0] >> 24), cur->sport);
      fprintf(fp, "%u.%u.%u.%u:%d  ", (cur->daddr[0] & 0xFF), ((cur->daddr[0] >> 8) & 0xff),
          ((cur->daddr[0] >> 16) & 0xff),  (cur->daddr[0] >> 24), cur->dport);
      fprintf(stderr, "%u.%u.%u.%u:%d --> ", (cur->saddr[0] & 0xFF), ((cur->saddr[0] >> 8) & 0xff),
          ((cur->saddr[0] >> 16) & 0xff),  (cur->saddr[0] >> 24), cur->sport);
      fprintf(stderr, "%u.%u.%u.%u:%d ", (cur->daddr[0] & 0xFF), ((cur->daddr[0] >> 8) & 0xff),
          ((cur->daddr[0] >> 16) & 0xff),  (cur->daddr[0] >> 24), cur->dport);
    }
#else
    fprintf(fp, "%u.%u.%u.%u:%d --> ", (cur->saddr & 0xFF), ((cur->saddr >> 8) & 0xff),
        ((cur->saddr >> 16) & 0xff),  (cur->saddr >> 24), cur->sport);
    fprintf(fp, "%u.%u.%u.%u:%d  ", (cur->daddr & 0xFF), ((cur->daddr >> 8) & 0xff),
        ((cur->daddr >> 16) & 0xff),  (cur->daddr >> 24), cur->dport);
    fprintf(stderr, "%u.%u.%u.%u:%d --> ", (cur->saddr & 0xFF), ((cur->saddr >> 8) & 0xff),
        ((cur->saddr >> 16) & 0xff),  (cur->saddr >> 24), cur->sport);
    fprintf(stderr, "%u.%u.%u.%u:%d ", (cur->daddr & 0xFF), ((cur->daddr >> 8) & 0xff),
        ((cur->daddr >> 16) & 0xff),  (cur->daddr >> 24), cur->dport);
#endif
#ifdef AF_INET6
    else {
      char name[200];
      socklen_t len;
      memset(name, 0, 200);
      len = 199;
      inet_ntop(AF_INET6, cur->saddr, name, len);
      fprintf(fp, "%s:%d --> ", name, cur->sport);
      fprintf(stderr, "%s:%d --> ", name, cur->sport);
      memset(name, 0, 200);
      len = 199;
      inet_ntop(AF_INET6, cur->daddr, name, len);
      fprintf(fp, "%s:%d  ", name, cur->dport);
      fprintf(stderr, "%s:%d  ", name, cur->dport);
    }
#endif
	}
  if (max == 0) {
    log_println(3, "No data Packets collected");
    if (get_debuglvl() > 2) {
      fprintf(fp, "\n\tNo packets collected\n");
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

	sprintf(buff, "  %d %d %d %d %d %d %d %d %d %d %d %d %0.2f %d %d %d %d %d", cur->links[0], cur->links[1],
		cur->links[2], cur->links[3], cur->links[4], cur->links[5], cur->links[6],
		cur->links[7], cur->links[8], cur->links[9], cur->links[10], cur->links[11],
		cur->totalspd2, cur->inc_cnt, cur->dec_cnt, cur->same_cnt, cur->timeout, cur->dupack);
	i = write(monitor_pipe[1], buff, 128);
  log_println(6, "wrote %d bytes: link counters are '%s'", i, buff);
  log_println(6, "#$#$#$#$ pcap routine says window increases = %d, decreases = %d, no change = %d",
      cur->inc_cnt, cur->dec_cnt, cur->same_cnt);
}

void calculate_spd(struct spdpair *cur, struct spdpair *cur2, int portA, int portB)
{
	
	float bits = 0, spd, time;

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

#if defined(AF_INET6)
  if (fwd.saddr[0] == 0) {
    log_println(1, "New packet trace started -- initializing counters");
    fwd.saddr[0] = current.saddr[0];
    fwd.daddr[0] = current.daddr[0];
#else
  if (fwd.saddr == 0) {
    log_println(1, "New packet trace started -- initializing counters");
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

    if ((fwd.saddr[0] == 0) && (fwd.saddr[1] == 0) && (fwd.saddr[2] == 0) && (fwd.saddr[3] == 0)) {
      log_println(1, "New packet trace started -- initializing counters");
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
      fwd.family = 6;
      rev.family = 6;
      return;
    }

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
  log_println(6, "Fault: unknown packet received with src/dst port = %d/%d", current.sport, current.dport);
}

/* This routine performs the open and initialization functions needed
 * by the libpcap routines.  The print_speed function above, is passed
 * to the pcap_open_live() function as the pcap_handler.
 */

void
init_pkttrace(struct sockaddr *sock_addr, socklen_t saddrlen, int monitor_pipe[2],
    char *device, PortPair* pair)
{
  char cmdbuf[256];
  pcap_handler printer;
  u_char * pcap_userdata = pair;
  struct bpf_program fcode;
  char errbuf[PCAP_ERRBUF_SIZE];
  int cnt, pflag = 0;
  char c;
  char namebuf[200];
  unsigned int nameBufLen = 199;
  I2Addr sockAddr = NULL;

  cnt = -1;  /* read forever, or until end of file */
  sig1 = 0;
  sig2 = 0;

  init_vars(&fwd);
  init_vars(&rev);

  if (device == NULL) {
    device = pcap_lookupdev(errbuf);
    if (device == NULL) {
      fprintf(stderr, "pcap_lookupdev failed: %s\n", errbuf);
    }
  }

  sockAddr = I2AddrBySAddr(NULL, sock_addr, saddrlen, 0, 0);
  sock_addr = I2AddrSAddr(sockAddr, 0);
  /* special check for localhost, set device accordingly */
  if (I2SockAddrIsLoopback(sock_addr, saddrlen) > 0) {
    strncpy(device, "lo", 3);
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
  }

  if (pcap_setfilter(pd, &fcode) < 0) {
    fprintf(stderr, "pcap_setfiler failed %s\n", pcap_geterr(pd));
  }

  if (dumptrace == 1) {
    fprintf(stderr, "Creating trace file for connection\n");
    memset(cmdbuf, 0, 256);
    sprintf(cmdbuf, "ndttrace.%s.%d", namebuf, I2AddrPort(sockAddr));
    pdump = pcap_dump_open(pd, cmdbuf);
    if (pdump == NULL) {
      fprintf(stderr, "Unable to create trace file '%s'\n", cmdbuf);
      dumptrace = 0;
    }
  }

  printer = (pcap_handler) print_speed;
  write(monitor_pipe[1], "Ready", 128);

  /* kill process off if parent doesn't send a signal. */
  alarm(45);

  if (pcap_loop(pd, cnt, printer, pcap_userdata) < 0) {
    log_println(5, "pcap_loop failed %s", pcap_geterr(pd));
  }
  log_println(5, "Pkt-Pair data collection ended, waiting for signal to terminate process");

  if (sig1 == 2) {
    read(mon_pipe1[0], &c, 1);
    close(mon_pipe1[0]);
    close(mon_pipe1[1]);
    sig1 = 0;
  }
  if (sig2 == 2) {
    while ((read(mon_pipe2[0], &c, 1)) < 0)
      ;
    sleep(5);
    close(mon_pipe2[0]);
    close(mon_pipe2[1]);
    sig2 = 0;
  }

  log_println(8, "Finally Finished reading data from network, process %d should terminate now", getpid());
}
