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
 * IPv6 port by Jakub S³awiñski <jeremian@poczta.fm>
 */

#include "../config.h"

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <getopt.h>

#include "tr-tree.h"
#include "usage.h"
#include "logging.h"

struct tr_tree *tr_root, *tr_cur;
char* DefaultTree = NULL;
static char dtfn[256];
int found_node;
#ifdef AF_INET6
struct tr_tree6 *tr_root6, *tr_cur6;
char* DefaultTree6 = NULL;
static char dt6fn[256];
#endif

static struct option long_options[] = {
  {"build", 0, 0, 'b'},
  {"compare", 1, 0, 'c'},
  {"file", 1, 0, 'f'},
  {"help", 0, 0, 'h'},
  {"print", 0, 0, 'p'},
  {"debug", 0, 0, 'd'},
  {"version", 0, 0, 'v'},
  {"dflttree", 1, 0, 301},
#ifdef AF_INET6
  {"dflttree6", 1, 0, 302},
  {"ipv4", 0, 0, '4'},
  {"ipv6", 0, 0, '6'},
#endif
  {0, 0, 0, 0}
};

/* Recursive sub-routine that walks through the tree.  In
 * build mode (flag = b) add a new node to the tree if
 * a leaf is reached and this new node should go under it.
 */
void
build_tree(struct tr_tree *tmp, struct tr_tree *new)
{
  if (new->ip_addr == tmp->ip_addr)
    return;
  if (new->ip_addr < tmp->ip_addr) {
    if (tmp->left == NULL) {
      tmp->left = new;
      return;
    }
    else {
      build_tree(tmp->left, new);
      return;
    }
  }
  if (new->ip_addr > tmp->ip_addr) {
    if (tmp->right == NULL) {
      tmp->right = new;
      return;
    }
    else {
      build_tree(tmp->right, new);
      return;
    }
  }
}
#ifdef AF_INET6
void
build_tree6(struct tr_tree6 *tmp, struct tr_tree6 *new)
{
  if (memcmp(new->ip_addr, tmp->ip_addr, sizeof(tmp->ip_addr)) == 0) {
    return;
  }
  if (memcmp(new->ip_addr, tmp->ip_addr, sizeof(tmp->ip_addr)) < 0) {
    if (tmp->left == NULL) {
      tmp->left = new;
      return;
    }
    else {
      build_tree6(tmp->left, new);
      return;
    }
  }
  if (memcmp(new->ip_addr, tmp->ip_addr, sizeof(tmp->ip_addr)) > 0) {
    if (tmp->right == NULL) {
      tmp->right = new;
      return;
    }
    else {
      build_tree6(tmp->right, new);
      return;
    }
  }
}
#endif

/* Walk through traceroute output file and return IP address.  handle
 * lines with *'s too.
 */
uint32_t
get_addr(char *tmpbuff)
{
  char *tmpstr;
  int i;
  uint32_t ip_addr;
  struct sockaddr_in address;

  /* no response from intermediate/destination host found, line should
   * just contain * * * sequence.  return -1 (255.255.255.255)
   */
  if ((strchr(tmpbuff, '*') != NULL) && (strchr(tmpbuff, '.') == NULL))
    return(-1);

  /* No IP address found on this line (can this occur or will the previous
   * test catch everything?
   */
  if (strchr(tmpbuff, '.') == NULL)
    return(-1);

  /* now check to see if '-n' option was used.  Output could be
   * line_no. hostname (ip_addr) time1 time2 time3 flag
   * OR '-n'
   * line_no ip_addr time1 time2 time3
   */
  if (strchr(tmpbuff, '(') != NULL) {
    tmpstr = strchr(tmpbuff, '(');
    tmpstr++;
    tmpbuff = strtok(tmpstr, ")");
  }
  else {
    tmpstr = strchr(tmpbuff, '.');
    while (tmpstr[0] != ' ') {
      if (tmpstr == tmpbuff)
        break;
      tmpstr--;
    }
    tmpbuff = strtok(tmpstr, " ");
  }
  if ((i = inet_aton(tmpbuff, &address.sin_addr)) == -1) {
    printf("Error: Improper format for traceroute compare file\n");
    exit(-39);
  }
  ip_addr = address.sin_addr.s_addr;
  return(ip_addr);
}
#ifdef AF_INET6
int
get_addr6(uint32_t dst_addr[4], char *tmpbuff)
{
  char *tmpstr;
  int i;
  struct sockaddr_in6 address;

  /* no response from intermediate/destination host found, line should
   * just contain IP t1 !x t2 !x t3 !x sequence.  Set addr to 16x255, return 0.
   */
  if ((tmpstr = strchr(tmpbuff, '!')) != NULL) {
    tmpstr++;
    if ((tmpstr = strchr(tmpstr, '!')) != NULL) {
      tmpstr++;
      if (strchr(tmpstr, '!') != NULL) {
        memset(dst_addr, 255, 16);
        return 0;
      }
    }
  }

  /* No IP address found on this line. Set addr to 16x255, return 0.
   */
  if (strchr(tmpbuff, ':') == NULL) {
    memset(dst_addr, 255, 16);
    return 0;
  }

  /* now check to see if '-n' option was used.  Output could be
   * line_no. hostname (ip_addr) time1 time2 time3 flag
   * OR '-n'
   * line_no ip_addr time1 time2 time3
   */
  if (strchr(tmpbuff, '(') != NULL) {
    tmpstr = strchr(tmpbuff, '(');
    tmpstr++;
    tmpbuff = strtok(tmpstr, ")");
  }
  else {
    tmpstr = strchr(tmpbuff, ':');
    while (tmpstr[0] != ' ') {
      if (tmpstr == tmpbuff)
        break;
      tmpstr--;
    }
    tmpbuff = strtok(tmpstr, " ");
  }
  if ((i = inet_pton(AF_INET6, tmpbuff, &address.sin6_addr)) == -1) {
    printf("Error: Improper format for traceroute6 compare file\n");
    exit(-39);
  }
  memcpy(dst_addr, &address.sin6_addr, 16);
  return 1;
}
#endif

/* Compare tree sub-routine.  Sets global found_node flag to 
 * indicate completion status
 */
void
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
#ifdef AF_INET6
void
compare_tree6(struct tr_tree6 *tmp, uint32_t ip_addr[4])
{
  if (memcmp(ip_addr, tmp->ip_addr, sizeof(tmp->ip_addr)) == 0) {
    found_node = 1;
    tr_cur6 = tmp;
    return;
  }
  if ((tmp->left == NULL) && (tmp->right == NULL)) {
    found_node = 2;
    tr_cur6 = tmp;
    return;
  }
  if ((memcmp(ip_addr, tmp->ip_addr, sizeof(tmp->ip_addr)) > 0) && (found_node == 0)) {
    compare_tree6(tmp->left, ip_addr);
  }
  if ((memcmp(ip_addr, tmp->ip_addr, sizeof(tmp->ip_addr)) < 0) && (found_node == 0)) {
    compare_tree6(tmp->right, ip_addr);
  }
}
#endif

/* Save the default tree in the file called DFLT_TREE.  It would
 * probably be better to keep this program running and have the
 * default tree in memory. This works for testing.
 */

void
save_tree(struct tr_tree *cur, FILE *fp)
{
  int i;

  for (i=0; i<cur->branches; i++) {
    if (i == 0)
      fwrite(cur, sizeof(struct tr_tree), 1, fp);
    save_tree(&*cur->branch[i], fp);
  }
  if (cur->branches == 0) {
    fwrite(cur, sizeof(struct tr_tree), 1, fp);
    free(cur);
  }
}
#ifdef AF_INET6
void
save_tree6(struct tr_tree6 *cur, FILE *fp)
{
  int i;

  for (i=0; i<cur->branches; i++) {
    if (i == 0)
      fwrite(cur, sizeof(struct tr_tree6), 1, fp);
    save_tree6(&*cur->branch[i], fp);
  }
  if (cur->branches == 0) {
    fwrite(cur, sizeof(struct tr_tree6), 1, fp);
    free(cur);
  }
}
#endif

/* Restore the default tree, stored by the save tree routine above.
 * Once restored, the comparison can take place.
 */
void
restore_tree2(struct tr_tree *tmp, FILE *fp)
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
#ifdef AF_INET6
void
restore_tree26(struct tr_tree6 *tmp, FILE *fp)
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
#endif


/* Print out the default tree using this routine.  Use a
 * standard recursive print algorithm
 */
void
print_tree(struct tr_tree *cur)
{
  static int i;
  int j;

  if (i == 0)
    printf("Root node is [%u.%u.%u.%u]\n",
        (cur->ip_addr & 0xff), ((cur->ip_addr >> 8) & 0xff),
        ((cur->ip_addr >> 16) & 0xff),  (cur->ip_addr >> 24));
  else  {
    printf("Leaf %d node [%u.%u.%u.%u]\n", i,
        (cur->ip_addr & 0xff), ((cur->ip_addr >> 8) & 0xff),
        ((cur->ip_addr >> 16) & 0xff),  ((cur->ip_addr >> 24) & 0xff));
  }
  for (j=0; j<cur->branches; j++) {
    i++;
    print_tree(&*cur->branch[j]);
    i--;
  }
}
#ifdef AF_INET6
void
print_tree6(struct tr_tree6 *cur)
{
  static int i;
  int j;
  char nodename[200];
  socklen_t nnlen = 199;

  if (i == 0) {
    inet_ntop(AF_INET6, cur->ip_addr, nodename, nnlen);
    printf("Root node is [%s]\n", nodename);
  }
  else  {
    inet_ntop(AF_INET6, cur->ip_addr, nodename, nnlen);
    printf("Leaf %d node [%s]\n", i, nodename);
  }
  for (j=0; j<cur->branches; j++) {
    i++;
    print_tree6(&*cur->branch[j]);
    i--;
  }
}
#endif

/* Builds the tree from the given file
 */
void
build(char* inputfile)
{
  struct tr_tree *root, *current, *new;
  int i;
  uint32_t ip_addr;
  char buff[256], *tmpbuff;
  FILE *fp;
  
  root = NULL;
  current = NULL;
  
  printf("\nBuilding default tree (IPv4)\n\n");
  if (inputfile == NULL)
    inputfile = "/tmp/traceroute.data";
  fp = fopen(inputfile, "r");

  if (fp == NULL) {
    printf("Error: Default tree input file '%s' missing!\n", inputfile);
    exit (-5);
  }
  while ((fgets(buff, 256, fp)) != NULL) {
    tmpbuff = strtok(buff, "\n");
    if (tmpbuff == NULL) {
      current = root;
      continue;
    }
    ip_addr = get_addr(tmpbuff);
    new = (struct tr_tree *) malloc(sizeof(struct tr_tree));
    if (new == NULL) {
      printf("Error: malloc failed, out of memory\n");
      exit (-10);
    }
    memset(&*new, 0, sizeof(struct tr_tree));
    new->ip_addr = ip_addr;
    sprintf(new->hostname, "%u.%u.%u.%u",
        (ip_addr & 0xff), ((ip_addr >> 8) & 0xff),
        ((ip_addr >> 16) & 0xff),  (ip_addr >> 24));
    /* resolve host name and store for later use */

    if (root == NULL) {
      root = new;
    }
    else {
      if ((current == root) && (current->ip_addr == new->ip_addr)) {
        free(new);
        continue;
      }
      for (i=0; i<current->branches; i++) {
        if (current->branch[i]->ip_addr == new->ip_addr) {
          current = current->branch[i];
          free(new);
          new = NULL;
          break;
        }
      }
      if (new == NULL) {
        continue;
      } else {
        i = 0;
        while (current->branch[i] != NULL)
          i++;
        current->branch[i] = new;
        current->branches++;
      }
    }
    current = new;
  }
  fp = fopen(DefaultTree, "wb");
  if (fp == NULL) {
    printf("Error: Can't write default tree '%s', exiting save_tree()\n", DefaultTree);
    exit (-15);
  }
  print_tree(&*root);
  printf("Finished printing default tree '%s'\n", DefaultTree);
  save_tree(&*root, fp);
  fclose(fp);
}
#ifdef AF_INET6
void
build6(char* inputfile)
{
  struct tr_tree6 *root, *current, *new;
  int i;
  uint32_t ip_addr[4];
  char buff[256], *tmpbuff;
  char nodename[200];
  socklen_t nnlen = 199;
  FILE *fp;
  
  root = NULL;
  current = NULL;
  
  printf("\nBuilding default tree (IPv6)\n\n");
  if (inputfile == NULL)
    inputfile = "/tmp/traceroute6.data";
  fp = fopen(inputfile, "r");

  if (fp == NULL) {
    printf("Error: Default tree6 input file '%s' missing!\n", inputfile);
    exit (-5);
  }
  while ((fgets(buff, 256, fp)) != NULL) {
    tmpbuff = strtok(buff, "\n");
    if (tmpbuff == NULL) {
      current = root;
      continue;
    }
    get_addr6(ip_addr, tmpbuff);
    new = (struct tr_tree6 *) malloc(sizeof(struct tr_tree6));
    if (new == NULL) {
      printf("Error: malloc failed, out of memory\n");
      exit (-10);
    }
    memset(&*new, 0, sizeof(struct tr_tree6));
    memcpy(new->ip_addr, ip_addr, sizeof(new->ip_addr));
    inet_ntop(AF_INET6, new->ip_addr, nodename, nnlen);
    sprintf(new->hostname, "%s", nodename);
    /* resolve host name and store for later use */

    if (root == NULL) {
      root = new;
    }
    else {
      if ((current == root) && (memcmp(current->ip_addr, new->ip_addr, sizeof(new->ip_addr)) == 0)) {
        free(new);
        continue;
      }
      for (i=0; i<current->branches; i++) {
        if (memcmp(current->branch[i]->ip_addr, new->ip_addr, sizeof(new->ip_addr)) == 0) {
          current = current->branch[i];
          free(new);
          new = NULL;
          break;
        }
      }
      if (new == NULL) {
        continue;
      } else {
        i = 0;
        while (current->branch[i] != NULL)
          i++;
        current->branch[i] = new;
        current->branches++;
      }
    }
    current = new;
  }
  fp = fopen(DefaultTree6, "wb");
  if (fp == NULL) {
    printf("Error: Can't write default tree6 '%s', exiting save_tree()\n", DefaultTree6);
    exit (-15);
  }
  print_tree6(&*root);
  printf("Finished printing default tree6 '%s'\n", DefaultTree6);
  save_tree6(&*root, fp);
  fclose(fp);
}
#endif

/* Compares the traceroute to the default tree
 */
void
compare(char* cmp_ip)
{
  struct tr_tree *root, *current, *new;
  int i;
  uint32_t ip_addr, IPlist[64];
  char h_name[256], c_name[256], buff[256], *tmpbuff;
  struct hostent *hp;
  FILE *fp;
  
  root = NULL;
  current = NULL;

  printf("\nComparing traceroute (IPv4)\n\n");
  fp = fopen(DefaultTree, "rb");
  if (fp == NULL) {
    printf("Error: Can't read default tree '%s', exiting restore_tree()\n", DefaultTree);
    exit (-15);
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
  fp = fopen(cmp_ip, "r");
  if (fp == NULL) {
    printf("Error: Can't read comparison file '%s', exiting main()\n", cmp_ip);
    exit (-17);
  }
  found_node = 0;
  current = root;
  i = 0;
  while ((fgets(buff, 256, fp)) != NULL) {
    tmpbuff = strtok(buff, "\n");
    IPlist[i] = get_addr(tmpbuff);
    log_println(5, "IPlist[%d] = %u.%u.%u.%u", (i),
        (IPlist[i] & 0xff), ((IPlist[i] >> 8) & 0xff),
        ((IPlist[i] >> 16) & 0xff),  (IPlist[i] >> 24));
    i++;

  }

  ip_addr = find_compare(IPlist, --i);

  hp = gethostbyaddr((char *) &ip_addr, 4, AF_INET);
  if (hp == NULL)
    strncpy(c_name, "Unknown Host", 13);
  else
    strncpy(c_name, hp->h_name, strlen(hp->h_name));

  if (found_node == 1) {
    printf("Host %s [%u.%u.%u.%u] is remote eNDT server!\n",
        c_name, (ip_addr & 0xff), ((ip_addr >> 8) & 0xff),
        ((ip_addr >> 16) & 0xff),  (ip_addr >> 24));
    return;
  }
  printf("Leaf Node found!\n");
  for (i=0; i<current->branches; i++) {
    if (current->branch[i]->branches == 0) {
      current = current->branch[i];
      break;
    }
  }
  hp = (struct hostent *)gethostbyaddr((char *) &current->ip_addr, 4, AF_INET);
  if (hp == NULL)
    strncpy(h_name, "Unknown Host", 13);
  else
    strncpy(h_name, hp->h_name, strlen(hp->h_name));

  printf("\tThe eNDT server %s [%u.%u.%u.%u] is closest to host %s [%u.%u.%u.%u]\n",
      h_name, (current->ip_addr & 0xff), ((current->ip_addr >> 8) & 0xff),
      ((current->ip_addr >> 16) & 0xff),  (current->ip_addr >> 24),
      c_name, (ip_addr & 0xff), ((ip_addr >> 8) & 0xff),
      ((ip_addr >> 16) & 0xff),  (ip_addr >> 24));
}
#ifdef AF_INET6
void
compare6(char* cmp_ip)
{
  struct tr_tree6 *root, *current, *new;
  int i;
  uint32_t ip_addr[4], IPlist[64][4];
  char h_name[256], c_name[256], buff[256], *tmpbuff;
  struct hostent *hp;
  char nodename[200];
  socklen_t nnlen = 199;
  FILE *fp;
  
  root = NULL;
  current = NULL;

  printf("\nComparing traceroute (IPv6)\n\n");
  fp = fopen(DefaultTree6, "rb");
  if (fp == NULL) {
    printf("Error: Can't read default tree6 '%s', exiting restore_tree()\n", DefaultTree6);
    exit (-15);
  }
  new = (struct tr_tree6 *) malloc(sizeof(struct tr_tree6));
  memset(&*new, 0, sizeof(struct tr_tree6));
  if (fread(&*new, sizeof(struct tr_tree6), 1, fp) == 0)
    return;
  else {
    for (i=0; i<25; i++)
      new->branch[i] = NULL;
  }
  if (root == NULL) {
    root = new;
  }
  restore_tree6(&*root, fp);
  fclose(fp);
  fp = fopen(cmp_ip, "r");
  if (fp == NULL) {
    printf("Error: Can't read comparison file '%s', exiting main()\n", cmp_ip);
    exit (-17);
  }
  found_node = 0;
  current = root;
  i = 0;
  while ((fgets(buff, 256, fp)) != NULL) {
    tmpbuff = strtok(buff, "\n");
    get_addr6(IPlist[i], tmpbuff);
    if (get_debuglvl() > 4) {
      memset(nodename, 0, 200);
      inet_ntop(AF_INET6, IPlist[i], nodename, nnlen);
      log_println(5, "IP6list[%d] = %s", i, nodename);
    }
    i++;
  }

  find_compare6(ip_addr, IPlist, --i);

  hp = gethostbyaddr((char *) ip_addr, 16, AF_INET6);
  if (hp == NULL)
    strncpy(c_name, "Unknown Host", 13);
  else
    strncpy(c_name, hp->h_name, strlen(hp->h_name));

  if (found_node == 1) {
    nnlen = 199;
    memset(nodename, 0, 200);
    inet_ntop(AF_INET6, ip_addr, nodename, nnlen);
    printf("Host %s [%s] is remote eNDT server!\n", c_name, nodename);
    return;
  }
  printf("Leaf Node found!\n");
  for (i=0; i<current->branches; i++) {
    if (current->branch[i]->branches == 0) {
      current = current->branch[i];
      break;
    }
  }
  hp = (struct hostent *)gethostbyaddr((char *) current->ip_addr, 16, AF_INET6);
  if (hp == NULL)
    strncpy(h_name, "Unknown Host", 13);
  else
    strncpy(h_name, hp->h_name, strlen(hp->h_name));

  nnlen = 199;
  memset(nodename, 0, 200);
  inet_ntop(AF_INET6, current->ip_addr, nodename, nnlen);
  printf("\tThe eNDT server %s [%s]", h_name, nodename);
  nnlen = 199;
  memset(nodename, 0, 200);
  inet_ntop(AF_INET6, ip_addr, nodename, nnlen);
  printf(" is closest to host %s [%s]\n", c_name, nodename);
}
#endif

/* Prints the default tree
 */
void
print()
{
  struct tr_tree *root = NULL, *new;
  int i;
  FILE *fp;
  
  printf("\nPrinting Default Tree (IPv4)\n\n");
  fp = fopen(DefaultTree, "rb");
  if (fp == NULL) {
    printf("Error: No default tree '%s' found, exiting compare\n", DefaultTree);
    return;
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
  print_tree(&*root);
}
#ifdef AF_INET6
void
print6()
{
  struct tr_tree6 *root = NULL, *new;
  int i;
  FILE *fp;
  
  printf("\nPrinting Default Tree (IPv6)\n\n");
  fp = fopen(DefaultTree6, "rb");
  if (fp == NULL) {
    printf("Error: No default tree6 '%s' found, exiting compare6\n", DefaultTree6);
    return;
  }
  new = (struct tr_tree6 *) malloc(sizeof(struct tr_tree6));
  memset(&*new, 0, sizeof(struct tr_tree6));
  if (fread(&*new, sizeof(struct tr_tree6), 1, fp) == 0)
    return;
  else {
    for (i=0; i<25; i++)
      new->branch[i] = NULL;
  }
  if (root == NULL) {
    root = new;
  }
  restore_tree6(&*root, fp);
  fclose(fp);   
  print_tree6(&*root);
}
#endif

int
main(int argc, char *argv[])
{
  char *cmp_ip=NULL;
  char *inputfile=NULL;
  int c, flag, flag_set=0;
  int v4only = 0;
  int v6only = 0;
  int debug = 0;

  flag = 'p';

#ifdef AF_INET6
#define GETOPT_LONG_INET6(x) "46"x
#else
#define GETOPT_LONG_INET6(x) x
#endif
  
  while ((c = getopt_long(argc, argv, GETOPT_LONG_INET6("bc:f:hpdv"), long_options, 0)) != -1) {
    switch (c) {
      case '4':
        v4only = 1;
        break;
      case '6':
        v6only = 1;
        break;
      case 'b':
        if (flag_set == 0) {
          flag = 'b';
          flag_set = 1;
        }
        break;
      case 'c':
        if (flag_set == 0) {
          flag = 'c';
          cmp_ip = optarg;
          flag_set = 1;
        }
        break;
      case 'f':
        inputfile = optarg;
        break;
      case 'p':
        flag = 'p';
        break;
      case 'd':
        debug++;
        break;
      case 'h':
        mkmap_long_usage("ANL/Internet2 NDT version " VERSION " (tr-mkmap)");
        break;
      case 'v':
        printf("ANL/Internet2 NDT version %s (tr-mkmap)\n", VERSION);
        exit(0);
        break;
      case 301:
        DefaultTree = optarg;
        break;
#ifdef AF_INET6
      case 302:
        DefaultTree6 = optarg;
        break;
#endif
      case '?':
        short_usage(argv[0], "");
        break;
    }
  }

  if (optind < argc) {
    short_usage(argv[0], "Unrecognized non-option elements");
  }

  log_init(argv[0], debug);

  if (DefaultTree == NULL) {
    sprintf(dtfn, "%s/%s", BASEDIR, DFLT_TREE);
    DefaultTree = dtfn;
  }
  
#ifdef AF_INET6
  if (DefaultTree6 == NULL) {
    sprintf(dt6fn, "%s/%s", BASEDIR, DFLT_TREE6);
    DefaultTree6 = dt6fn;
  }
#endif

  if (flag == 'b') {
#ifdef AF_INET6
    if (v4only == 0) {
      build6(inputfile);
    }
#endif
    if (v6only == 0) {
      build(inputfile);
    }
  }
  else if (flag == 'c') {
#ifdef AF_INET6
    if (v4only == 0) {
      compare6(cmp_ip);
    }
#endif
    if (v6only == 0) {
      compare(cmp_ip);
    }
  }
  else if (flag == 'p') {
#ifdef AF_INET6
    if (v4only == 0) {
      print6();
    }
#endif
    if (v6only == 0) {
      print();
    }
  }
  else {
    printf("Nothing happened, how did I get here?\n");
  }
  return 0;
}
