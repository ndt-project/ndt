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

/* initialize variables before starting to accumlate data */
void init_vars(struct spdpair *cur)
{

	int i;

	cur->saddr = 0;
	cur->daddr = 0;
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
void print_bins(struct spdpair *cur, int monitor_pipe[2], char *LogFileName, int debug)
{

	int i, total=0, max=0, s, index;
	char buff[256];
	int tzoffset = 6;
	FILE *fp;

	/* the tzoffset value is fixed for CST (6), use 5 for CDT.  The code needs to find the
	 * current timezone and use that value here! */
	s = (cur->st_sec - (tzoffset * 3600)) %86400; 
	/* s = (cur->st_sec - 5) %86400;  */
	/* s = (cur->st_sec + gmt2local(0)) %86400; */
	fp = fopen(LogFileName, "a");
	if (debug > 0) 
	    fprintf(stderr, "%02d:%02d:%02d.%06u   ", s / 3600, (s % 3600) / 60,
			s % 60, cur->st_usec);

	for (i=0; i<16; i++) {
	    total += cur->links[i];
	    if (cur->links[i] > max) {
		max = cur->links[i];
	    }
	}
	if (debug > 2) {
	    fprintf(fp, "%u.%u.%u.%u:%d -->", (cur->saddr & 0xFF), ((cur->saddr >> 8) & 0xff),
		((cur->saddr >> 16) & 0xff),  (cur->saddr >> 24), cur->sport);
	    fprintf(fp, "%u.%u.%u.%u:%d  ", (cur->daddr & 0xFF), ((cur->daddr >> 8) & 0xff),
		((cur->daddr >> 16) & 0xff),  (cur->daddr >> 24), cur->dport);
	    fprintf(stderr, "%u.%u.%u.%u:%d -->", (cur->saddr & 0xFF), ((cur->saddr >> 8) & 0xff),
		((cur->saddr >> 16) & 0xff),  (cur->saddr >> 24), cur->sport);
	    fprintf(stderr, "%u.%u.%u.%u:%d ", (cur->daddr & 0xFF), ((cur->daddr >> 8) & 0xff),
		((cur->daddr >> 16) & 0xff),  (cur->daddr >> 24), cur->dport);
	}
	if (max == 0) {
	    if (debug > 2) {
	        fprintf(fp, "\n\tNo packets collected\n");
		fprintf(stderr, "No data Packets collected\n");
	    }
	    for (i=0; i<16; i++)
		cur->links[i] = -1;
	    index = -1;
	}
	if (debug > 2)
	    fprintf(stderr, "Collected pkt-pair data max = %d\n", max);
	if (max == cur->links[8]) {
	    max = 0;
	    for (i=0; i<10; i++) {
		if (max < cur->links[i]) {
		    index = i;
		    max = cur->links[i];
		}
	    }
	}
	if (debug > 2) {
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

	sprintf(buff, "  %d %d %d %d %d %d %d %d %d %d %d %d %0.2f %d %d %d %d %d\0", cur->links[0], cur->links[1],
		cur->links[2], cur->links[3], cur->links[4], cur->links[5], cur->links[6],
		cur->links[7], cur->links[8], cur->links[9], cur->links[10], cur->links[11],
		cur->totalspd2, cur->inc_cnt, cur->dec_cnt, cur->same_cnt, cur->timeout, cur->dupack);
	i = write(monitor_pipe[1], buff, 128);
	/* i = write(monitor_pipe[1], buff, 256); */
	if (debug > 5) {
	    fprintf(stderr, "wrote %d bytes: link counters are '%s'\n", i, buff);
	    fprintf(stderr, "#$#$#$#$ pcap routine says window increases = %d, decreases = %d, no change = %d\n",
			cur->inc_cnt, cur->dec_cnt, cur->same_cnt);
	}

}

void calculate_spd(struct spdpair *cur, struct spdpair *cur2, int port2, int port3)
{
	
	float bits, spd, time;

	time = (((cur->sec - cur2->sec)*1000000) + (cur->usec - cur2->usec));
	/* time = curt->time - cur2->time; */
	if ((cur->dport == port2) || (cur->sport == port3)) {
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
