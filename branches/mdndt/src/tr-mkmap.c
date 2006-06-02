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
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

#include "tr-tree.h"

struct tr_tree *tr_root, *tr_cur;
int found_node;

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

/* Save the default tree in the file called DFLT_TREE.  In would
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

int
main(int argc, char *argv[])
{
  struct tr_tree *root, *current, *new;
  int i, debug=0;
  uint32_t ip_addr, IPlist[64];
  char h_name[256], c_name[256], buff[256], *cmp_ip=NULL, *tmpbuff;
  char tmpstr[256], *inputfile=NULL;
  int c, flag, flag_set=0;
  FILE *fp;
  struct hostent *hp;

  flag = 'b';
  while (( c = getopt(argc, argv, "dbhpc:f:")) != -1) {
    switch (c) {
      case 'b' :
        if (flag_set == 0) {
          flag = 'b';
          flag_set = 1;
        }
        break;
      case 'c' :
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
      default :
        printf("Unknown option entered\n");
      case 'h' :
        printf("Usage: %s {options}\n", argv[0]);
        printf("\t-b \tbuild a new default tree\n");
        printf("\t-c fn \tCompare new traceroute to tree\n");
        printf("\t-f fn \tSpecify the name of the input file\n");
        printf("\t\tNote: -b and -c are mutually exclusive\n");
        printf("\t-h \tPrint this help message\n");
        printf("\t-p \tPrint out the current traceroute map\n");
        exit(0);
    }
  }

  root = NULL;
  current = NULL;
  if (flag == 'b') {
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
    sprintf(tmpstr, "%s/%s", BASEDIR, DFLT_TREE);
    fp = fopen(tmpstr, "wb");
    if (fp == NULL) {
      printf("Error: Can't write default tree '%s', exiting save_tree()\n", tmpstr);
      exit (-15);
    }
    print_tree(&*root);
    printf("Finished printing default tree '%s'\n", tmpstr);
    save_tree(&*root, fp);
    fclose(fp);
  }

  else if (flag == 'c') {
    sprintf(tmpstr, "%s/%s", BASEDIR, DFLT_TREE);
    fp = fopen(tmpstr, "rb");
    if (fp == NULL) {
      printf("Error: Can't read default tree '%s', exiting restore_tree()\n", tmpstr);
      exit (-15);
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
      if (debug > 4)
        printf("IPlist[%d] = %u.%u.%u.%u \n", (i),
            (IPlist[i] & 0xff), ((IPlist[i] >> 8) & 0xff),
            ((IPlist[i] >> 16) & 0xff),  (IPlist[i] >> 24));
      i++;

    }

    ip_addr = find_compare(IPlist, --i, debug);

    hp = gethostbyaddr((char *) &ip_addr, 4, AF_INET);
    if (hp == NULL)
      strncpy(c_name, "Unknown Host", 13);
    else
      strncpy(c_name, hp->h_name, strlen(hp->h_name));

    if (found_node == 1) {
      printf("Host %s [%u.%u.%u.%u] is remote eNDT server!\n",
          c_name, (ip_addr & 0xff), ((ip_addr >> 8) & 0xff),
          ((ip_addr >> 16) & 0xff),  (ip_addr >> 24));
      exit(0);
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
    exit(0);

  }
  else if (flag == 'p') {
    printf("Printing Default Tree\n\n");
    sprintf(tmpstr, "%s/%s", BASEDIR, DFLT_TREE);
    fp = fopen(tmpstr, "rb");
    if (fp == NULL) {
      printf("Error: No default tree '%s' found, exiting compare\n", tmpstr);
      exit (-15);
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
    print_tree(&*root);
  }
  else
    printf("Nothing happened, how did I get here?\n");
  return 0;
}
