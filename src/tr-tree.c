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
 *  Add -g to this line to get gdb debugging info.
 *
 * Richard Carlson <rcarlson@internet2.edu>
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

#include "tr-tree.h"
#include "logging.h"

struct tr_tree *tr_root, *tr_cur;
int found_node;
char* DefaultTree;

/* Restore the default tree, stored by the save tree routine above.
 * Once restored, the comparison can take place.
 */
void
restore_tree(struct tr_tree *tmp, FILE *fp)
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
      continue;
    }
    restore_tree( &*tmp->branch[i], fp);
  }
}

u_int32_t
find_compare(u_int32_t IPlist[], int cnt)
{

  struct tr_tree *root, *current, *new;
  int i, j, k;
  uint32_t srv_addr;
  char h_name[256], c_name[256];
  FILE *fp;
  struct hostent *hp;

  root = NULL;
  current = NULL;

  fp = fopen(DefaultTree, "rb");
  if (fp == NULL) {
    log_println(5, "Error: Can't read default tree, exiting find_compare()");
    return 0;
  }
  new = (struct tr_tree *) malloc(sizeof(struct tr_tree));
  memset(&*new, 0, sizeof(struct tr_tree));
  if (fread(&*new, sizeof(struct tr_tree), 1, fp) == 0)
    return 0;
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
  srv_addr = 0;
  current = root;
  log_println(6, "route to client contains %d hops", cnt);
  for (i=0; i<=cnt; i++) {
    log_print(6, "New client node [%u.%u.%u.%u] ",
        (IPlist[i] & 0xff), ((IPlist[i] >> 8) & 0xff),
        ((IPlist[i] >> 16) & 0xff),  (IPlist[i] >> 24));
    log_println(6, "to map node [%u.%u.%u.%u]",
        (current->ip_addr & 0xff), ((current->ip_addr >> 8) & 0xff),
        ((current->ip_addr >> 16) & 0xff),  (current->ip_addr >> 24));
    if (current->ip_addr == IPlist[i])
      continue;
    for (j=0; j<current->branches; j++) {
      log_print(5, "Comparing map node [%u.%u.%u.%u] ",
          (current->branch[j]->ip_addr & 0xff),
          ((current->branch[j]->ip_addr >> 8) & 0xff),
          ((current->branch[j]->ip_addr >> 16) & 0xff), 
          (current->branch[j]->ip_addr >> 24));
      log_println(5, "to client node [%u.%u.%u.%u], cnt = %d",
          (IPlist[i] & 0xff), ((IPlist[i] >> 8) & 0xff),
          ((IPlist[i] >> 16) & 0xff),  (IPlist[i] >> 24), i);

      if (current->branch[j]->ip_addr == IPlist[i]) {
        current = current->branch[j];
        found_node = 0;
        for (k=0; k<current->branches; k++) {
          if (current->branch[k]->branches == 0) {
            srv_addr = current->branch[k]->ip_addr;
            log_println(5, "srv_addr set to [%u.%u.%u.%u]",
                (srv_addr & 0xff), ((srv_addr >> 8) & 0xff),
                ((srv_addr >> 16) & 0xff), (srv_addr >> 24));
            found_node = 1;
          }
        }
        break;
      }
      found_node = -1;
    }
    if (found_node == -1)
      break;
  }

  if (current->ip_addr == IPlist[cnt]) {
    srv_addr = IPlist[cnt];
    found_node = 1;
  }
  if (srv_addr != 0)
    found_node = 1;

  if (found_node == -1) {
    log_println(6, "Broke out of compare loop, setting current pointer");
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

  hp = gethostbyaddr((char *) &IPlist[i], 4, AF_INET);
  if (hp == NULL)
    strncpy(c_name, "Unknown Host", 13);
  else
    strncpy(c_name, hp->h_name, strlen(hp->h_name));

  if (found_node == 1) {
    log_println(5, "Router %s [%u.%u.%u.%u] is last common router in the path!",
        c_name, (IPlist[i] & 0xff), ((IPlist[i] >> 8) & 0xff),
        ((IPlist[i] >> 16) & 0xff),  (IPlist[i] >> 24));
    return(srv_addr);
  }
  log_println(6, "New Server Node found!  found_node set to %d", found_node);
  hp = (struct hostent *)gethostbyaddr((char *) &current->ip_addr, 4, AF_INET);
  if (hp == NULL)
    strncpy(h_name, "Unknown Host", 13);
  else
    strncpy(h_name, hp->h_name, strlen(hp->h_name));

  log_println(6, "\tThe eNDT server %s [%u.%u.%u.%u] is closest to %s [%u.%u.%u.%u]",
      h_name, (current->ip_addr & 0xff), ((current->ip_addr >> 8) & 0xff),
      ((current->ip_addr >> 16) & 0xff),  (current->ip_addr >> 24),
      c_name, (IPlist[cnt] & 0xff), ((IPlist[cnt] >> 8) & 0xff),
      ((IPlist[cnt] >> 16) & 0xff),  (IPlist[cnt] >> 24));

  return(current->ip_addr);
}
