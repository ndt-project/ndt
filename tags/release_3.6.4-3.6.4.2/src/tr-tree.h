/*
 * This file contains the definitions and function declarations to
 * handle tr-trees.
 *
 * Jakub S³awiñski 2006-06-02
 * jeremian@poczta.fm
 */

#ifndef _JS_TR_TREE_H
#define _JS_TR_TREE_H

struct tr_tree {
  uint32_t ip_addr;                /* IP addr of current node */
  int branches;
  char hostname[256];              /* Hostname placeholder */
  struct tr_tree *branch[25];      /* pointer to child */
  struct tr_tree *left;            /* pointer to child */
  struct tr_tree *right;           /* pointer to child */
};

#define DFLT_TREE "Default.tree"   /* file containing default tree */

void restore_tree(struct tr_tree *tmp, FILE *fp);
u_int32_t find_compare(u_int32_t IPlist[], int cnt);

#ifdef AF_INET6

struct tr_tree6 {
  uint32_t ip_addr[4];             /* IP addr of the current node */
  int branches;
  char hostname[256];              /* Hostname placeholder */
  struct tr_tree6 *branch[25];     /* pointer to child */
  struct tr_tree6 *left;           /* pointer to child */
  struct tr_tree6 *right;          /* pointer to child */
};

#define DFLT_TREE6 "Default.tree6" /* file containing default tree for IPv6 nodes */

void restore_tree6(struct tr_tree6 *tmp, FILE *fp);
int find_compare6(u_int32_t IPnode[4], u_int32_t IP6list[][4], int cnt);

#endif

#endif
