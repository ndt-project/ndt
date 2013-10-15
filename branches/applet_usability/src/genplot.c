/*
 * readvar: Reads the current value for one or more web100 variables in
 *          a connection.  Web100 variables are separated by spaces.
 *
 * Usage: readvar <connection id> <var name> [<var name> ...]
 * Example: readvar 1359 LocalPort LocalAddress
 *
 * Copyright (c) 2001
 *      Carnegie Mellon University, The Board of Trustees of the University
 *      of Illinois, and University Corporation for Atmospheric Research.
 *      All rights reserved.  This software comes with NO WARRANTY.
 *
 * Since our code is currently under active development we prefer that
 * everyone gets the it directly from us.  This will permit us to
 * collaborate with all of the users.  So for the time being, please refer
 * potential users to us instead of redistributing web100.
 *
 * $Id: readvar.c,v 1.3 2002/09/30 19:48:27 engelhar Exp $
 */

#include "../config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/errno.h>
#include <getopt.h>
#include <sys/types.h>

#include "web100srv.h"
#include "usage.h"

char *color[16] = { "green", "blue", "orange", "red", "yellow", "magenta",
  "pink", "white", "black" };

static struct option long_options[] = { { "both", 0, 0, 'b' }, { "multi", 1, 0,
  'm' }, { "text", 0, 0, 't' }, { "CurCwnd", 0, 0, 'C' }, { "CurRwinRcvd",
    0, 0, 'R' }, { "throughput", 0, 0, 'S' }, { "cwndtime", 0, 0, 'c' }, {
      "help", 0, 0, 'h' }, { "version", 0, 0, 'v' }, { 0, 0, 0, 0 } };

/**
 * @param x 
 * @return x unless x == 2147483647 in which case -1 is returned.
 */
int checkmz(int x) {
  if (x == 2147483647) {
    return -1;
  }
  return x;
}


#if USE_WEB100

#define ELAPSED_TIME "Duration"
#define TIME_SENDER  "SndLimTimeSender"
#define DATA_OCT_OUT "DataBytesOut"

#elif USE_WEB10G

#define Chk(x) \
    do { \
        err = (x); \
        if (err != NULL) { \
            goto Cleanup; \
        } \
    } while (0)

/* ElapsedMicroSecs is the one of the only things not yet working in the
 * kernel patch. Instead use snaplog timestamps to fake this */
static uint64_t start_time = 0;
#define ELAPSED_TIME "ElapsedMicroSecs"
#define TIME_SENDER  "SndLimTimeSnd"
#define DATA_OCT_OUT "HCDataOctetsOut"

/* web10g-util.c needs logging but we don't want real logging
 * so let it link with this version and print to stderr */
void log_println(int lvl, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}

#endif

/**
 * Given a snap (likely read from a log) read a value as a double.
 * Upon failure will exit the program with error code EXIT_FAILURE.
 * 
 * 
 * @param name The name of the variable to read in Web100/Web10G.
 * @param snap A Web100/Web10G snap
 * @param group A Web100 group - ignored by Web10G should be NULL
 * @param agent A Web100 agent - ignored by Web10G should be NULL
 * 
 * @return The value of 'name' covnerted to a double.
 */
double tcp_stat_read_double(const char * name, tcp_stat_snap* snap,
                         tcp_stat_group* group, tcp_stat_agent* agent) {
#if USE_WEB100
  web100_var* var;
  char buf[256];
#elif USE_WEB10G
  estats_val val;
#endif

#if USE_WEB100
  if ((web100_agent_find_var_and_group(agent, name, &group,
                                       &var)) != WEB100_ERR_SUCCESS) {
    web100_perror("web100_agent_find_var_and_group");
    exit(EXIT_FAILURE);
  }

  if ((web100_snap_read(var, snap, buf)) != WEB100_ERR_SUCCESS) {
    web100_perror("web100_snap_read");
    exit(EXIT_FAILURE);
  }
  return atof(
              web100_value_to_text(web100_get_var_type(var),
                                   buf));
#elif USE_WEB10G
  int vartype = ESTATS_UNSIGNED32;
  /* ELAPSED_TIME not working in kernel patch, so use time on the 
   * snap */
  if (strcmp(name, ELAPSED_TIME) == 0) {
    val.masked = 0;
    val.uv32 = snap->tv.sec * 1000000 + snap->tv.usec - start_time;
  } else {
    vartype = web10g_find_val(snap, name, &val);
    if (vartype == -1) {
      printf("Bad vartype tcp_stat_read_double %s\n", name);
      exit(EXIT_FAILURE);
    }
  }
  switch (vartype) {
    case ESTATS_UNSIGNED64:
      return (double) val.uv64;
    case ESTATS_UNSIGNED32:
      return (double) val.uv32;
    case ESTATS_SIGNED32:
      return (double) val.sv32;
    case ESTATS_UNSIGNED16:
      return (double) val.uv16;
    case ESTATS_UNSIGNED8:
      return (double) val.uv8;
  }
  fprintf(stderr, "Bad vartype tcp_stat_read_double %s\n", name);
  exit(EXIT_FAILURE);
#endif
}

/**
 * Get the title info from the snap. That is local address:port remote
 * address:port.
 * 
 * For Web10G this also logs the start_time because this is expected
 * to be the first snap captured.
 * 
 * @param snap A Web100/Web10G snap
 * @param agent A Web100 agent - ignored by Web10G should be NULL
 * @param group A Web100 group - ignored by Web10G should be NULL
 * @param title Upon return contains the string 
 *          "<localaddr>:<localport> --> <remoteaddr>"
 * @param remport Upon return contains the remote port as a string
 */
void get_title(tcp_stat_snap* snap, tcp_stat_agent* agent,
               tcp_stat_group* group, char* title, char* remport) {
#if USE_WEB100
  web100_var* var;
  char buf[128];

  if ((web100_agent_find_var_and_group(agent, "LocalAddress", &group, &var))
      != WEB100_ERR_SUCCESS) {
    web100_perror("web100_agent_find_var_and_group");
    exit(EXIT_FAILURE);
  }
  if ((web100_snap_read(var, snap, buf)) != WEB100_ERR_SUCCESS) {
    web100_perror("web100_snap_read");
    exit(EXIT_FAILURE);
  }
  strcpy(title, web100_value_to_text(web100_get_var_type(var), buf));
  strncat(title, ":", 1);
  if ((web100_agent_find_var_and_group(agent, "LocalPort", &group, &var))
      != WEB100_ERR_SUCCESS) {
    web100_perror("web100_agent_find_var_and_group");
    exit(EXIT_FAILURE);
  }
  if ((web100_snap_read(var, snap, buf)) != WEB100_ERR_SUCCESS) {
    web100_perror("web100_snap_read");
    exit(EXIT_FAILURE);
  }
  strcat(title, web100_value_to_text(web100_get_var_type(var), buf));
  strncat(title, " --> ", 5);
  if ((web100_agent_find_var_and_group(agent, "RemAddress", &group, &var))
      != WEB100_ERR_SUCCESS) {
    web100_perror("web100_agent_find_var_and_group");
    exit(EXIT_FAILURE);
  }
  if ((web100_snap_read(var, snap, buf)) != WEB100_ERR_SUCCESS) {
    web100_perror("web100_snap_read");
    exit(EXIT_FAILURE);
  }
  strcat(title, web100_value_to_text(web100_get_var_type(var), buf));
  if ((web100_agent_find_var_and_group(agent, "RemPort", &group, &var))
      != WEB100_ERR_SUCCESS) {
    web100_perror("web100_agent_find_var_and_group");
    exit(EXIT_FAILURE);
  }
  if ((web100_snap_read(var, snap, buf)) != WEB100_ERR_SUCCESS) {
    web100_perror("web100_snap_read");
    exit(EXIT_FAILURE);
  }
  strcpy(remport, web100_value_to_text(web100_get_var_type(var), buf));
  /* printf("%s:%s\n", title, remport); */
#elif USE_WEB10G
  estats_error* err = NULL;
  struct estats_connection_tuple_ascii tuple_ascii;

  /* Quite a convenient little function we have */
  if ((err = estats_connection_tuple_as_strings(&tuple_ascii,
                                            &snap->tuple)) != NULL) {
    /* If using the 3.5 kernel to make snaps it appears that the
     * address isn't filled in, so continue with unknown */
    fprintf(stderr, "WARNING estats_connection_tuple_as_string has"
        " failed this could be due to using the 3.5 kernel patch which"
        " doesn't log this information!!!!");
    estats_error_print(stderr, err);
    estats_error_free(&err);
    sprintf(title, "unknown:unknown --> unknown");
    sprintf(remport, "unknown");
    // exit(EXIT_FAILURE);
  }

  sprintf(title, "%s:%s --> %s", tuple_ascii.local_addr,
                  tuple_ascii.local_port, tuple_ascii.rem_addr);
  sprintf(remport, "%s", tuple_ascii.rem_port);
  /* Notes the time fo this the first snap in the global start_time
   * because ElapsedTimeMicroSec is unimplemented in the kernel patch */
  start_time = snap->tv.sec * 1000000 + snap->tv.usec;

#endif
}

/**
 * Plot anything else.
 * 
 * @param list A comma seperated list of Web100/Web10G names to
 * 	        plot.
 * @param cnt The number of items in list to plot, starting from the 
 *          start. Shouldn't be larger than the number of items in 'list'
 * @param name The file name to output to (gets .<remport>.xpls
 *          appended to the end).
 * @param snap Allocated storage for Web100 - NULL for Web10G
 * @param log A open Web100/Web10G log file
 * @param agent A Web100 agent - ignored by Web10G should be NULL
 * @param group A Web100 group - ignored by Web10G should be NULL
 * @param func (Can be NULL) A function that expects two integers for
 * 	        its arguments and returns a new integer. The first interger 
 * 			corrosponds to the item in 'list' 0 for the first etc. 
 *          The second the value in the snap. The returned value is 
 *          plotted.
 *          Called once for every list item for ever snap in the log.
 */
void plot_var(char *list, int cnt, char *name, tcp_stat_snap* snap,
              tcp_stat_log* log, tcp_stat_agent* agent, tcp_stat_group* group,
              int(*func)(const int arg, const int value)) {
#if USE_WEB10G
  estats_error* err = NULL;
#endif
  char *varg;
  /*char buf[256];
  web100_var* var;*/
  char varlist[256], lname[256], remport[8];
  char title[256];
  int i, first = 0;
  float x1, x2, y1[32], y2[32];
  FILE *fn;

  /* Write a xplot file out to the requested file.
   * Start by extracting the connection info for the 
   * page title.  Then its a series of line statements
   * with the x1 y1 x2 y2 coordinates.
   */

  memset(lname, 0, 256);

  /* Get the first snap from the log */
#if USE_WEB100
  if ((web100_snap_from_log(snap, log)) != WEB100_ERR_SUCCESS) {
    web100_perror("web100_snap_from_log");
    return;
  }
#elif USE_WEB10G
  if ((err = estats_record_read_data(&snap, log)) != NULL) {
    estats_record_close(&log);
    estats_error_print(stderr, err);
    estats_error_free(&err);
    return;
  }
#endif

  get_title(snap, agent, group, title, remport);

  if (name == NULL) {
    fn = stdout;
    /* XXX writing into a NULL pointer?? */
    // strncpy(name, "Unknown", 7);
    name = "Unknown";
  } else {
    snprintf(lname, sizeof(lname), "%s.%s.xpl", name, remport);
    fn = fopen(lname, "w");
  }

  fprintf(fn, "double double\ntitle\n");
  fprintf(fn, "%s:%s (%s)\n", title, remport, name);
  if ((strncmp(name, "Throughput", 10)) == 0)
    fprintf(fn, "xlabel\nTime\nylabel\nMbits/sec\n");
  else
    fprintf(fn, "xlabel\nTime\nylabel\nKilo Bytes\n");

  x1 = x2 = 0;
  for (i = 0; i < 32; i++) {
    y1[i] = 0;
    y2[i] = 0;
  }
  first = 0;

  for (;;) {
    /* We've already read the first item to use with get_title */
    if (first != 0) {
#if USE_WEB100
      if ((web100_snap_from_log(snap, log)) != WEB100_ERR_SUCCESS) {
#elif USE_WEB10G
      if ((err = estats_record_read_data(&snap, log)) != NULL) {
        estats_error_free(&err);
#endif
        fprintf(fn, "go\n");
        return;
      }
    }
    strncpy(varlist, list, strlen(list) + 1);
    varg = strtok(varlist, ",");
    for (i = 0; i < cnt; i++) {
      if (i == 0) {
        if (first == 0) {
          if (func) {
            x1 = func(
                i,
                checkmz(
                        tcp_stat_read_double(varg, snap, group, agent)));
          } else {
            x1 = checkmz(
                    tcp_stat_read_double(varg, snap, group, agent));
          }
        } else {
          x1 = x2;
        }
      } else {
        if (first == 0) {
          if (func) {
            y1[i - 1] = func(
                i,
                checkmz(
                    tcp_stat_read_double(varg, snap, group, agent)));
          } else {
            y1[i - 1] = checkmz(
                        tcp_stat_read_double(varg, snap, group, agent));
          }
        } else {
          y1[i - 1] = y2[i - 1];
        }
      }
      varg = strtok(NULL, ",");
    }

    first++;
    strncpy(varlist, list, strlen(list) + 1);
    varg = strtok(varlist, ",");
    for (i = 0; i < cnt; i++) {
      if (i == 0) {
        if (func) {
          x2 = func(
              i,
              checkmz(
                  tcp_stat_read_double(varg, snap, group, agent)));
        } else {
          x2 = checkmz(
              tcp_stat_read_double(varg, snap, group, agent));
        }
      } else {
        if (func) {
          y2[i - 1] = func(
              i,
              checkmz(
                  tcp_stat_read_double(varg, snap, group, agent)));
        } else {
          y2[i - 1] = checkmz(
              tcp_stat_read_double(varg, snap, group, agent));
        }
        fprintf(fn, "%s\nline %0.4f %0.4f %0.4f %0.4f\n", color[i - 1],
                x1 / 1000000, y1[i - 1] / 1024, x2 / 1000000,
                y2[i - 1] / 1024);
      }
      varg = strtok(NULL, ",");
    }
#if USE_WEB10G
    estats_val_data_free(&snap);
#endif
  }
  fprintf(fn, "go\n");
}


/**
 * Make a plot CwndTime(%) against total time.
 * 
 * @param name The file name to output to (gets .<remport>.xpls
 *          appended to the end).
 * @param snap Allocated storage for Web100 - NULL for Web10G
 * @param log A open Web100/Web10G log file
 * @param agent A Web100 agent - ignored by Web10G should be NULL
 * @param group A Web100 group - ignored by Web10G should be NULL
 */
void plot_cwndtime(char *name, tcp_stat_snap* snap, tcp_stat_log* log,
                   tcp_stat_agent* agent, tcp_stat_group* group) {
#if USE_WEB10G
  estats_error* err = NULL;
#endif
  double SndLimTimeRwin = 0, SndLimTimeSender = 0;
  char lname[256], remport[8];
  char title[256];
  char* variables[] = { ELAPSED_TIME, "SndLimTimeRwin", TIME_SENDER,
    "SndLimTimeCwnd" };
  int i, first = 0;
  double x1, x2, y1, y2;
  FILE *fn;

  memset(lname, 0, 256);

  /* Get the first snap from the log */
#if USE_WEB100
  if ((web100_snap_from_log(snap, log)) != WEB100_ERR_SUCCESS) {
    web100_perror("web100_snap_from_log");
    return;
  }
#elif USE_WEB10G
  if ((err = estats_record_read_data(&snap, log)) != NULL) {
    estats_record_close(&log);
    estats_error_print(stderr, err);
    estats_error_free(&err);
    return;
  }
#endif

  get_title(snap, agent, group, title, remport);

  if (name == NULL) {
    fn = stdout;
    /* XXX writing into a NULL pointer?? */
    // strncpy(name, "Unknown", 7);
    name = "Unknown";
  } else {
    snprintf(lname, sizeof(lname), "%s.%s.xpl", name, remport);
    fn = fopen(lname, "w");
  }

  fprintf(fn, "double double\ntitle\n");
  fprintf(fn, "%s:%s (%s)\n", title, remport, name);
  fprintf(fn, "xlabel\nTime\nylabel\nPercentage\n");

  x1 = x2 = y1 = y2 = 0;
  first = 0;

  for (;;) {
    /* We've already read the first item to use with get_title */
    if (first != 0) {
#if USE_WEB100
      if ((web100_snap_from_log(snap, log)) != WEB100_ERR_SUCCESS) {
#elif USE_WEB10G
      if ((err = estats_record_read_data(&snap, log)) != NULL) {
        estats_error_free(&err);
#endif
        fprintf(fn, "go\n");
        return;
      }
    }
    for (i = 0; i < 4; i++) {
      if (first == 0) {
        /* Give everything starting values */
        switch (i) {
          case 0: /* ELAPSED_TIME */
            x1 = tcp_stat_read_double(variables[i], snap, group, agent);
          break;
          case 1: /* "SndLimTimeRwin" */
            SndLimTimeRwin = tcp_stat_read_double(variables[i],
                                                    snap, group, agent);
          break;
          case 2: /* "SndLimTimeSender" */
            SndLimTimeSender = tcp_stat_read_double(variables[i],
                                                    snap, group, agent);
          break;
          case 3: /* "SndLimTimeCwnd" */
            y1 = tcp_stat_read_double(variables[i], snap, group, agent);
            y1 = y1 / (SndLimTimeRwin + SndLimTimeSender + y1);
          break;
        }
      } else {
        switch (i) {
          case 0: /* ELAPSED_TIME */
            x1 = x2;
            break;
          case 3: /* "SndLimTimeCwnd" */
            y1 = y2;
            break;
        }
      }
    }

    first++;
    for (i = 0; i < 4; i++) {
      if (i == 0) { /* ELAPSED_TIME */
        x2 = tcp_stat_read_double(variables[i], snap, group, agent);
      } else if (i == 1) { /* "SndLimTimeRwin" */
        SndLimTimeRwin = tcp_stat_read_double(variables[i],
                                                    snap, group, agent);
      } else if (i == 2) { /* "SndLimTimeSender" */
        SndLimTimeSender = tcp_stat_read_double(variables[i],
                                                    snap, group, agent);
      } else { /* "SndLimTimeCwnd" */
        y2 = tcp_stat_read_double(variables[i], snap, group, agent);
        y2 = y2 / (SndLimTimeRwin + SndLimTimeSender + y2);
        fprintf(fn, "%s\nline %0.4f %0.4f %0.4f %0.4f\n", color[i - 1],
                x1 / 1000000, y1, x2 / 1000000, y2);
      }
    }
#if USE_WEB10G
      estats_val_data_free(&snap);
#endif
  }
  fprintf(fn, "go\n");
}

/**
 * 
 * Loop through the supplied list of variables and prints them to
 * stdout as a new row for every snap in the log file.
 * 
 * A function 'func' can be provided to changed how these are printed.
 * 
 * @param varlist A comma seperated list of Web100/Web10G names to
 * 					print.
 * @param snap Allocated storage for Web100 - NULL for Web10G
 * @param log A open Web100/Web10G log file
 * @param agent A Web100 agent - ignored by Web10G should be NULL
 * @param group A Web100 group - ignored by Web10G should be NULL
 * @param func (Can be NULL) A function that expects two integers for
 * 	        its arguments. The first corrosponds to the item in varlist
 *          0 for the first etc. The second the value in the snap.
 *          Called once for every varlist item for ever snap in the log.
 */
void print_var(char *varlist, tcp_stat_snap* snap, tcp_stat_log* log,
               tcp_stat_agent* agent, tcp_stat_group* group,
               void(*func)(const int arg, const int value)) {
  char *varg, savelist[256];
  char title[256], remport[8];
  int i, j;
#if USE_WEB10G
  estats_error* err = NULL;
#endif
  FILE* fn;

  fn = stdout;

  /* Get the first snap from the log */
#if USE_WEB100
  if ((web100_snap_from_log(snap, log)) != WEB100_ERR_SUCCESS) {
    web100_perror("web100_snap_from_log");
    return;
  }
#elif USE_WEB10G
  if ((err = estats_record_read_data(&snap, log)) != NULL) {
    estats_record_close(&log);
    estats_error_print(stderr, err);
    estats_error_free(&err);
    return;
  }
#endif

  get_title(snap, agent, group, title, remport);
  fprintf(fn, "Extracting Data from %s:%s connection\n\n", title, remport);

  strncpy(savelist, varlist, strlen(varlist) + 1);
  printf("Index\t");
  varg = strtok(varlist, ",");
  /* Loop through varlist and print them out as the column names */
  for (j = 0;; j++) {
    if (varg == NULL)
      break;
    if (func) {
      func(-1, j);
    } else {
      printf("%10s\t", varg);
    }
    varg = strtok(NULL, ",");
  }
  printf("\n");
  /* Loop over the log file */
  for (i = 0;; i++) {
    /* We've already read the first item to use with get_title */
    if (i != 0) {
#if USE_WEB100
      if ((web100_snap_from_log(snap, log)) != WEB100_ERR_SUCCESS) {
#elif USE_WEB10G
      if ((err = estats_record_read_data(&snap, log)) != NULL) {
        estats_error_free(&err);
#endif
        printf("-------------- End Of Data  --------------\n\n");
        return;
      }
    }
    printf("%5d\t", i);
    strncpy(varlist, savelist, strlen(savelist) + 1);
    varg = strtok(varlist, ",");
    /* Loop over the vars we are printing out */
    for (j = 0;; j++) {
      if (varg == NULL) {
        break;
      }
      if (func) {
        /* Let the provided function do the printing */
        func(j, (int) tcp_stat_read_double(varg, snap, group, agent));
      } else {
        /* Do it ourself */
        double value = tcp_stat_read_double(varg, snap, group, agent);
        // weird magic number ((2^32) - 1) - 919
        if ((int) value == 4294966376) {
          printf("%10s\t", "-1");
        } else {
          printf("%10"PRId64"\t", (int64_t) value);
        }
      }
      varg = strtok(NULL, ",");
    }
#if USE_WEB10G
    estats_val_data_free(&snap);
#endif
    printf("\n");
  }
}

/* workers */
void throughput(const int arg, const int value) {
  static int duration;

  if (arg == -1) {
    if (value) {
      printf("%10s\t", "Throughput (mB/s)");
    } else {
      printf("%10s\t", ELAPSED_TIME);
    }
    return;
  }

  if (!arg) { /* duration */
    duration = value;
    printf("%10d\t", value);
  } else { /* DataBytesOut */
    printf(
        "%10.2f",
        ((8 * ((double) value)) / ((double) duration)) * 1000000.0
        / 1024.0 / 1024.0);
  }
}

int throughputPlot(const int arg, const int value) {
  static int duration;

  if (!arg) { /* duration */
    duration = value;
    return value;
  } else { /* DataBytesOut */
    return (((double) value) / ((double) duration)) * 1000000.0;
  }
}

void cwndtime(const int arg, const int value) {
  static int SndLimTimeRwin, SndLimTimeSender;

  if (arg == -1) {
    if (value == 0) {
      printf("%10s\t", ELAPSED_TIME);
    } else if (value == 3) {
      printf("%10s\t", "CwndTime (%% of total time)");
    }
    return;
  }

  if (arg == 0) { /* duration */
    printf("%10d\t", value);
  } else if (arg == 1) { /* SndLimTimeRwin */
    SndLimTimeRwin = value;
  } else if (arg == 2) { /* SndLimTimeSender */
    SndLimTimeSender = value;
  } else { /* SndLimTimeCwnd */
    printf(
        "%10.2f",
        ((double) value)
        / (((double) SndLimTimeRwin)
           + ((double) SndLimTimeSender) + ((double) value)));
  }
}

/* --- */

int main(int argc, char** argv) {
  tcp_stat_agent* agent = NULL;
  tcp_stat_connection conn = NULL;
  tcp_stat_group* group = NULL;
  tcp_stat_log* log = NULL;
  tcp_stat_snap* snap = NULL;
#if USE_WEB10G
  estats_error* err = NULL;
#endif
  char fn[128];
  char *varlist = NULL, list[1024];
  char *varg;
  int j, c, plotspd = 0, plotuser = 0;
  int plotboth = 0, plotcwnd = 0, plotrwin = 0;
  int plotcwndtime = 0;
  int k, txt = 0;

  while ((c = getopt_long(argc, argv, "hCScRbtm:v", long_options, 0)) != -1) {
    switch (c) {
      case 'b':
        plotboth = 1;
        break;
      case 'h':
        genplot_long_usage("ANL/Internet2 NDT version " VERSION " (genplot)",
                           argv[0]);
        break;
      case 'v':
        printf("ANL/Internet2 NDT version %s (genplot)\n", VERSION);
        exit(0);
        break;
      case 't':
        txt = 1;
        break;
      case 'C':
        plotcwnd = 1;
        break;
      case 'R':
        plotrwin = 1;
        break;
      case 'S':
        plotspd = 1;
        break;
      case 'c':
        plotcwndtime = 1;
        break;
      case 'm':
        varlist = optarg;
        plotuser = 1;
        break;
    }
  }

  if (optind == argc) {
    short_usage(argv[0], "Missing snaplog file");
  }

  if (argc == 1) {
    short_usage(argv[0], "ANL/Internet2 NDT version " VERSION " (genplot)");
  }

  for (j = optind; j < argc; j++) {
    snprintf(fn, sizeof(fn), "%s", argv[j]);
#if USE_WEB100
    if ((log = web100_log_open_read(fn)) == NULL) {
      web100_perror("web100_log_open_read");
      exit(EXIT_FAILURE);
    }

    if ((agent = web100_get_log_agent(log)) == NULL) {
      web100_perror("web100_get_log_agent");
      exit(EXIT_FAILURE);
    }

    if ((group = web100_get_log_group(log)) == NULL) {
      web100_perror("web100_get_log_group");
      exit(EXIT_FAILURE);
    }

    if ((conn = web100_get_log_connection(log)) == NULL) {
      web100_perror("web100_get_log_connection");
      exit(EXIT_FAILURE);
    }
#elif USE_WEB10G
    if ((err = estats_record_open(&log, fn, "r")) != NULL) {
      estats_error_print(stderr, err);
      estats_error_free(&err);
      exit(EXIT_FAILURE);
    }
#endif
    fprintf(stderr, "Extracting data from Snaplog '%s'\n\n", fn);

#if USE_WEB100
    if ((snap = web100_snapshot_alloc_from_log(log)) == NULL) {
      web100_perror("web100_snapshot_alloc_from_log");
      exit(EXIT_FAILURE);
    }
#endif

    if (plotuser == 1) {
      memset(list, 0, 1024);
      strncpy(list, ELAPSED_TIME",", 1024);
      strncat(list, varlist, strlen(varlist));
      varg = strtok(list, ",");
      for (k = 1;; k++) {
        if ((varg = strtok(NULL, ",")) == NULL)
          break;
      }
      memset(list, 0, 1024);
      strncpy(list, ELAPSED_TIME",", 1024);
      strncat(list, varlist, strlen(varlist));
      if (txt == 1)
        print_var(list, snap, log, agent, group, NULL);
      else
        plot_var(list, k, "User Defined", snap, log, agent, group,
                 NULL);
    }
    if (plotspd == 1) {
      memset(list, 0, 1024);
      strncpy(list, ELAPSED_TIME","DATA_OCT_OUT, 1024);
      if (txt == 1)
        print_var(list, snap, log, agent, group, throughput);
      else
        plot_var(list, 2, "Throughput", snap, log, agent, group,
                 throughputPlot);
    }
    if (plotcwndtime == 1) {
      memset(list, 0, 1024);
      strncpy(list,
              ELAPSED_TIME",SndLimTimeRwin,"TIME_SENDER",SndLimTimeCwnd",
              1024);
      if (txt == 1)
        print_var(list, snap, log, agent, group, cwndtime);
      else
        plot_cwndtime("Cwnd Time", snap, log, agent, group);
    }
    if (plotcwnd == 1) {
      memset(list, 0, 1024);
      strncpy(list, ELAPSED_TIME",CurCwnd", 1024);
      if (txt == 1)
        print_var(list, snap, log, agent, group, NULL);
      else
        plot_var(list, 2, "CurCwnd", snap, log, agent, group, NULL);
    }
    if (plotrwin == 1) {
      memset(list, 0, 1024);
      strncpy(list, ELAPSED_TIME",CurRwinRcvd", 1024);
      if (txt == 1)
        print_var(list, snap, log, agent, group, NULL);
      else
        plot_var(list, 2, "CurRwinRcvd", snap, log, agent, group, NULL);
    }
    if (plotboth == 1) {
      memset(list, 0, 1024);
      strncpy(list, ELAPSED_TIME",CurCwnd,CurRwinRcvd", 1024);
      if (txt == 1)
        print_var(list, snap, log, agent, group, NULL);
      else
        plot_var(list, 3, "Both", snap, log, agent, group, NULL);
    }
#if USE_WEB100
  web100_log_close_read(log);
#elif USE_WEB10G
  estats_record_close(&log);
#endif
  }

  exit(0);
}
