/* This program reads a set of 'traceroute6 -n' output lines and
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
 * use "gcc -o tr-tree6 tr-tree6.c" to compile this code
 *  Add -g to this line to get gdb debugging info.
 *
 * Richard Carlson <rcarlson@internet2.edu>
 *
 * IPv6 port by Jakub S³awiñski <jeremian@poczta.fm>
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "tr-tree.h"

#ifdef AF_INET6

struct tr_tree6 *tr_root6, *tr_cur6;
int found_node;

/* Restore the default tree, stored by the save tree routine above.
 * Once restored, the comparison can take place.
 */
void
restore_tree6(struct tr_tree6 *tmp, FILE *fp)
{
  struct tr_tree6 *new;
  int i, j;

  for (i=0; i<tmp->branches; i++) {
    new = (struct tr_tree6 *) malloc(sizeof(struct tr_tree6));
    memset(&*new, 0, sizeof(struct tr_tree6));
    if (fread(&*new, sizeof(struct tr_tree6), 1, fp) == 0)
      return;
    else {
      for (j=0; j<25; j++)
        new->branch[j] = NULL;
    }
    tmp->branch[i] = new;
    if (new->branches == 0) {
      continue;
    }
    restore_tree6( &*tmp->branch[i], fp);
  }
}

int
find_compare6(u_int32_t IPnode[4], u_int32_t IP6list[][4], int cnt, int debug)
{

  struct tr_tree6 *root, *current, *new;
  int i, j, k, fnode = 0;
  char h_name[256], c_name[256], buff[256];
  char nodename[200];
  size_t nnlen;
  FILE *fp;
  struct hostent *hp;

  root = NULL;
  current = NULL;

  sprintf(buff, "%s/%s", BASEDIR, DFLT_TREE6);
  fp = fopen(buff, "rb");
  if (fp == NULL) {
    if (debug > 4) 
      fprintf(stderr, "Error: Can't read default tree, exiting find_compare6()\n");
    return 0;
  }
  new = (struct tr_tree6 *) malloc(sizeof(struct tr_tree6));
  memset(&*new, 0, sizeof(struct tr_tree6));
  if (fread(&*new, sizeof(struct tr_tree6), 1, fp) == 0)
    return 0;
  else {
    for (i=0; i<25; i++)
      new->branch[i] = NULL;
  }
  if (root == NULL) {
    root = new;
  }
  restore_tree6(&*root, fp);
  fclose(fp);
  found_node = 0;
  current = root;
  if (debug > 5)
    fprintf(stderr, "route to client contains %d hops\n", cnt);
  for (i=0; i<=cnt; i++) {
    if (debug > 5) {
      memset(nodename, 0, 200);
      nnlen = 199;
      inet_ntop(AF_INET6, (void *) IP6list[i], nodename, nnlen);
      fprintf(stderr, "New client node [%s] ", nodename);
      memset(nodename, 0, 200);
      nnlen = 199;
      inet_ntop(AF_INET6, (void *) current->ip_addr, nodename, nnlen);
      fprintf(stderr, "to map node [%s]\n", nodename);
    }
    if (memcmp(IP6list[i], current->ip_addr, sizeof(current->ip_addr)) == 0) {
      continue;
    }
    for (j=0; j<current->branches; j++) {
      if (debug > 4) {
        memset(nodename, 0, 200);
        nnlen = 199;
        inet_ntop(AF_INET6, (void *) current->branch[j]->ip_addr, nodename, nnlen);
        fprintf(stderr, "Comparing map node [%s] ", nodename);
        memset(nodename, 0, 200);
        nnlen = 199;
        inet_ntop(AF_INET6, (void *) IP6list[i], nodename, nnlen);
        fprintf(stderr, "to client node [%s], cnt = %d\n", nodename, i);
      }

      if (memcmp(current->branch[j]->ip_addr, IP6list[i], sizeof(IP6list[i])) == 0) {
        current = current->branch[j];
        found_node = 0;
        for (k=0; k<current->branches; k++) {
          if (current->branch[k]->branches == 0) {
            memcpy(IPnode, current->branch[k]->ip_addr, 16);
            if (debug > 4) {
              memset(nodename, 0, 200);
              nnlen = 199;
              inet_ntop(AF_INET6, (void *) IPnode, nodename, nnlen);
              fprintf(stderr, "srv_addr set to [%s]\n", nodename);
            }
            found_node = 1;
            fnode = 1;
          }
        }
        break;
      }
      found_node = -1;
    }
    if (found_node == -1)
      break;
  }

  if (memcmp(current->ip_addr, IP6list[cnt], sizeof(IP6list[cnt])) == 0) {
    memcpy(IPnode, IP6list[cnt], 16);
    found_node = 1;
    fnode = 1;
  }
  if (fnode) {
    found_node = 1;
  }

  if (found_node == -1) {
    if (debug > 5)
      fprintf(stderr, "Broke out of compare loop, setting current pointer\n");
    if (current->branches == 1) {
      current = current->branch[0];
      if (current->branches == 0)
        found_node = 2;
      else {
        found_node = 4;
        current = root;
      }
    }
    else {
      found_node = 3;
      current = root;
    }
  }

  hp = gethostbyaddr((char *) IP6list[i], 16, AF_INET6);
  if (hp == NULL)
    strncpy(c_name, "Unknown Host", 13);
  else
    strncpy(c_name, hp->h_name, strlen(hp->h_name));

  if (found_node == 1) {
    if (debug > 4) {
      memset(nodename, 0, 200);
      nnlen = 199;
      inet_ntop(AF_INET6, (void *) IP6list[i], nodename, nnlen);
      fprintf(stderr, "Router %s [%s] is last common router in the path!\n", c_name, nodename);
    }
    return 1;
  }
  if (debug > 5)
    fprintf(stderr, "New Server Node found!  found_node set to %d\n", found_node);
  hp = (struct hostent *)gethostbyaddr((char *) current->ip_addr, 16, AF_INET6);
  if (hp == NULL)
    strncpy(h_name, "Unknown Host", 13);
  else
    strncpy(h_name, hp->h_name, strlen(hp->h_name));

  if (debug > 5) {
    memset(nodename, 0, 200);
    nnlen = 199;
    inet_ntop(AF_INET6, (void *) current->ip_addr, nodename, nnlen);
    fprintf(stderr, "\tThe eNDT server %s [%s]", h_name, nodename);
    memset(nodename, 0, 200);
    nnlen = 199;
    inet_ntop(AF_INET6, (void *) IP6list[cnt], nodename, nnlen);
    fprintf(stderr, " is closest to %s [%s]\n", c_name, nodename);
  }

  memcpy(IPnode, current->ip_addr, 16);
  return 1;
}

#endif
