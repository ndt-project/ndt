/* This program reads a set of 'traceroute -n' output lines and
 * builds a default traceroute tree.  This tree is then used
 * to determine which route best describes a new traceroute.
 * The basic operation is to run this once in the 'build' mode
 * (-b) option.  This generates the default tree.  Then new
 * traceroutes are compared to this tree using the 'compare'
 * (-c) option.
 *
 * The default tree should be built from traceroutes run between
 * peer eNDT servers.  The new trees are then run from 1 eNDT
 * to an arbritary client.  A match is made when a new leaf
 * would be added.  The leaf node of this branch gives the
 * IP address of the 'closest' eNDT server.
 *
 * This development code is designed as a proof-of-concept
 * for the tree building and matching algorithm.
 *
 * use "gcc -o tr-tree tr-tree.c" to compile this code
 *	Add -g to this line to get gdb debugging info.
 *
 * Richard Carlson <rcarlson@internet2.edu>
 *
 */

#include <stdio.h>
/* #include <netinet/ip.h> */
/* #include <netinet/tcp.h> */
#include <sys/types.h>
#include <netinet/in.h>
/* #include <stand.h> */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

struct tr_tree {
	uint32_t ip_addr;		/* IP addr of current node */
	int branches;
	char hostname[256];		/* Hostname placeholder */
	struct tr_tree *branch[25];		/* pointer to child */
	struct tr_tree *left;		/* pointer to child */
	struct tr_tree *right;		/* pointer to child */
};

#define DFLT_TREE "Default.tree"		/* file containing default tree */
struct tr_tree *tr_root, *tr_cur;
int found_node;


/* Compare tree sub-routine.  Sets global found_node flag to 
 * indicate completion status
 */
compare_tree(struct tr_tree *tmp, uint32_t ip_addr)
{

	if (tmp->ip_addr == ip_addr) {
	    found_node = 1;
	    tr_cur = tmp;
	    return;
	}
	if ((tmp->left == NULL) && (tmp->right == NULL)) {
	    found_node = 2;
	    tr_cur = tmp;
	    return;
	}
	if ((tmp->ip_addr < ip_addr) && (found_node == 0))
	    compare_tree(tmp->left, ip_addr);
	if ((tmp->ip_addr > ip_addr) && (found_node == 0))
	    compare_tree(tmp->right, ip_addr);
}

/* Restore the default tree, stored by the save tree routine above.
 * Once restored, the comparison can take place.
 */
void restore_tree(struct tr_tree *tmp, FILE *fp)
{
	struct tr_tree *new;
	int i, j;
	
	for (i=0; i<tmp->branches; i++) {
	    new = (struct tr_tree *) malloc(sizeof(struct tr_tree));
	    memset(&*new, 0, sizeof(struct tr_tree));
	    if (fread(&*new, sizeof(struct tr_tree), 1, fp) == 0)
	        return;
	    else {
	        for (j=0; j<25; j++)
		    new->branch[j] = NULL;
	    }
	    tmp->branch[i] = new;
	    if (new->branches == 0) {
		/* return; */
		/* restore_tree(&*tmp, fp); */
		continue;
	    }
	    restore_tree( &*tmp->branch[i], fp);
	    /* goto loop2; */
	}
	
}

u_int32_t find_compare (u_int32_t IPlist[], int cnt, int debug)
{

	struct tr_tree *root, *current, *new;
	int i, j;
	uint32_t ip_addr;
	char h_name[256], c_name[256], buff[256], *cmp_ip=NULL, *tmpbuff;
	int c, flag, flag_set=0;
	FILE *fp;
	struct sockaddr_in address;
	struct hostent *hp;

	root = NULL;
	current = NULL;

	fp = fopen(DFLT_TREE, "rb");
	if (fp == NULL) {
	    if (debug > 4) 
	        fprintf(stderr, "Error: Can't read default tree, exiting restore_tree()\n");
	    return(0);
	}
	new = (struct tr_tree *) malloc(sizeof(struct tr_tree));
	memset(&*new, 0, sizeof(struct tr_tree));
	if (fread(&*new, sizeof(struct tr_tree), 1, fp) == 0)
	        return;
	else {
	        for (i=0; i<25; i++)
		    new->branch[i] = NULL;
	}
	if (root == NULL) {
	        root = new;
	}
	restore_tree(&*root, fp);
	fclose(fp);
	found_node = 0;
	current = root;
	if (debug > 5)
		fprintf(stderr, "route to client contains %d hops\n", cnt);
	for (i=0; i<=cnt; i++) {
		if (debug > 5) {
			fprintf(stderr, "New client node [%u.%u.%u.%u] ",
			    (IPlist[i] & 0xff), ((IPlist[i] >> 8) & 0xff),
                	    ((IPlist[i] >> 16) & 0xff),  (IPlist[i] >> 24));
			fprintf(stderr, "to map node [%u.%u.%u.%u]\n",
			    (current->ip_addr & 0xff), ((current->ip_addr >> 8) & 0xff),
                	    ((current->ip_addr >> 16) & 0xff),  (current->ip_addr >> 24));
		}
		if (current->ip_addr == IPlist[i])
		    continue;
		for (j=0; j<current->branches; j++) {
		    if (debug > 5) {
			fprintf(stderr, "Comparing map node [%u.%u.%u.%u] ",
			    (current->branch[j]->ip_addr & 0xff),
			    ((current->branch[j]->ip_addr >> 8) & 0xff),
                	    ((current->branch[j]->ip_addr >> 16) & 0xff), 
			    (current->branch[j]->ip_addr >> 24));
			fprintf(stderr, "to client node [%u.%u.%u.%u]\n",
			    (IPlist[i] & 0xff), ((IPlist[i] >> 8) & 0xff),
                	    ((IPlist[i] >> 16) & 0xff),  (IPlist[i] >> 24));
		    }

		    if ((current->branch[j]->ip_addr == IPlist[i])
			 		&& (current->branch[j] != NULL)) {
			current = current->branch[j];
			break;
		    }
		}
	}

	hp = gethostbyaddr((char *) &IPlist[i], 4, AF_INET);
	if (hp == NULL)
		strcpy(c_name, "Unknown Host");
	else
		strcpy(c_name, hp->h_name);

	if (found_node == 1) {
	    if (debug > 4)
		fprintf(stderr, "Host %s [%u.%u.%u.%u] is remote eNDT server!\n",
			c_name, (IPlist[i] & 0xff), ((IPlist[i] >> 8) & 0xff),
                	((IPlist[i] >> 16) & 0xff),  (IPlist[i] >> 24));
	    return(ip_addr);
	}
	if (debug > 5)
	    fprintf(stderr, "Leaf Node found!\n");
	for (j=0; j<current->branches; j++) {
		if (current->branch[j]->branches == 0) {
		    current = current->branch[j];
		    break;
		}
	}
	hp = (struct hostent *)gethostbyaddr((char *) &current->ip_addr, 4, AF_INET);
	if (hp == NULL)
		strcpy(h_name, "Unknown Host");
	else
		strcpy(h_name, hp->h_name);

	if (debug > 5)
	    fprintf(stderr, "\tThe eNDT server %s [%u.%u.%u.%u] is closest to %s [%u.%u.%u.%u]\n",
		h_name, (current->ip_addr & 0xff), ((current->ip_addr >> 8) & 0xff),
                ((current->ip_addr >> 16) & 0xff),  (current->ip_addr >> 24),
		c_name, (IPlist[cnt] & 0xff), ((IPlist[cnt] >> 8) & 0xff),
                ((IPlist[cnt] >> 16) & 0xff),  (IPlist[cnt] >> 24));

	return(current->ip_addr);
}
