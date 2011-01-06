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

#ifdef HAVE_LIBWEB100
#include  <web100.h>
#endif

#include "usage.h"

char *color[16] = {"green",
		   "blue",
		   "orange",
		   "red",
		   "yellow",
		   "magenta",
		   "pink",
		   "white",
		   "black"
		  };

static struct option long_options[] = {
  {"both", 0, 0, 'b'},
  {"multi", 1, 0, 'm'},
  {"text", 0, 0, 't'},
  {"CurCwnd", 0, 0, 'C'},
  {"CurRwinRcvd", 0, 0, 'R'},
  {"throughput", 0, 0, 'S'},
  {"cwndtime", 0, 0, 'c'},
  {"help", 0, 0, 'h'},
  {"version", 0, 0, 'v'},
  {0, 0, 0, 0}
};

int
checkmz(int x)
{
    if (x == 2147483647) {
        return -1;
    }
    return x;
}

void get_title(web100_snapshot* snap, web100_log* log, web100_agent* agent,
		web100_group* group, char* title, char* remport)
{

	web100_var* var;
	char buf[128];

	if ((web100_snap_from_log(snap, log)) != WEB100_ERR_SUCCESS) {
            web100_perror("web100_log_open_read");
            return;
    	}

        if ((web100_agent_find_var_and_group(agent, "LocalAddress", &group, &var)) != WEB100_ERR_SUCCESS) {
            web100_perror("web100_agent_find_var_and_group");
            exit(EXIT_FAILURE);
        }
        if ((web100_snap_read(var, snap, buf)) != WEB100_ERR_SUCCESS) {
            web100_perror("web100_snap_read");
            exit(EXIT_FAILURE);
        }
        strcpy(title, web100_value_to_text(web100_get_var_type(var), buf));
	strncat(title, ":", 1);
        if ((web100_agent_find_var_and_group(agent, "LocalPort", &group, &var)) != WEB100_ERR_SUCCESS) {
            web100_perror("web100_agent_find_var_and_group");
            exit(EXIT_FAILURE);
        }
        if ((web100_snap_read(var, snap, buf)) != WEB100_ERR_SUCCESS) {
            web100_perror("web100_snap_read");
            exit(EXIT_FAILURE);
        }
        strcat(title, web100_value_to_text(web100_get_var_type(var), buf));
	strncat(title, " --> ", 5);
        if ((web100_agent_find_var_and_group(agent, "RemAddress", &group, &var)) != WEB100_ERR_SUCCESS) {
            web100_perror("web100_agent_find_var_and_group");
            exit(EXIT_FAILURE);
        }
        if ((web100_snap_read(var, snap, buf)) != WEB100_ERR_SUCCESS) {
            web100_perror("web100_snap_read");
            exit(EXIT_FAILURE);
        }
        strcat(title, web100_value_to_text(web100_get_var_type(var), buf));
        if ((web100_agent_find_var_and_group(agent, "RemPort", &group, &var)) != WEB100_ERR_SUCCESS) {
            web100_perror("web100_agent_find_var_and_group");
            exit(EXIT_FAILURE);
        }
        if ((web100_snap_read(var, snap, buf)) != WEB100_ERR_SUCCESS) {
            web100_perror("web100_snap_read");
            exit(EXIT_FAILURE);
        }
        strcpy(remport, web100_value_to_text(web100_get_var_type(var), buf));
        /* printf("%s:%s\n", title, remport); */
}

void
plot_var(char *list, int cnt, char *name, web100_snapshot* snap,
        web100_log* log, web100_agent* agent, web100_group* group,
        int(*func)(const int arg, const int value))
{

    char *varg;
    char buf[256];
    web100_var* var;
    char varlist[256], lname[256], remport[8];
    char title[256];
    int i, first=0;
    float x1, x2, y1[32], y2[32];
    FILE *fn;


    /* Write a xplot file out to the requested file.
     * Start by extracting the connection info for the 
     * page title.  Then its a series of line statements
     * with the x1 y1 x2 y2 coordinates.
     */

    memset(lname, 0, 256);

    get_title(snap, log, agent, group, title, remport);

    if (name == NULL) {
        fn = stdout;
        strncpy(name, "Unknown", 7);
    } else {
        strncpy(lname, name, strlen(name));
        strcat(lname, ".");
        strcat(lname, remport);
        strcat(lname, ".xpl");
        fn = fopen(lname, "w");
    }

    fprintf(fn, "double double\ntitle\n");
    fprintf(fn, "%s:%s (%s)\n", title, remport, name);
    if ((strncmp(name, "Throughput", 10)) == 0) 
    	fprintf(fn, "xlabel\nTime\nylabel\nMbits/sec\n");
    else
    	fprintf(fn, "xlabel\nTime\nylabel\nKilo Bytes\n");

    x1 = x2 = 0;
    for (i=0; i<32; i++) {
        y1[i] = 0;
        y2[i] = 0;
    }
    first = 0;

    for (;;) {
        if (first != 0) {
            if ((web100_snap_from_log(snap, log)) != WEB100_ERR_SUCCESS) {
                /* web100_perror("web100_log_open_read"); */
                fprintf(fn, "go\n");
                return;
            }
        }
        strncpy(varlist, list, strlen(list)+1);
        varg=strtok(varlist, ",");
        for (i=0; i<cnt; i++) {
            if ((web100_agent_find_var_and_group(agent, varg, &group, &var)) != WEB100_ERR_SUCCESS) {
                web100_perror("web100_agent_find_var_and_group");
                exit(EXIT_FAILURE);
            }

            if ((web100_snap_read(var, snap, buf)) != WEB100_ERR_SUCCESS) {
                web100_perror("web100_snap_read");
                exit(EXIT_FAILURE);
            }

            if (i == 0) {
                if (first == 0) {
                    if (func) {
                        x1 = func(i, checkmz(atoi(web100_value_to_text(web100_get_var_type(var), buf))));
                    }
                    else {
                        x1 = checkmz(atoi(web100_value_to_text(web100_get_var_type(var), buf)));
                    }
                } else {
                    x1 = x2;
                }
            } else {
                if (first == 0) {
                    if (func) {
                        y1[i-1] = func(i, checkmz(atoi(web100_value_to_text(web100_get_var_type(var), buf))));
                    }
                    else {
                        y1[i-1] = checkmz(atoi(web100_value_to_text(web100_get_var_type(var), buf)));
                    }
                } else {
                    y1[i-1] = y2[i-1];
                }
            }
            varg = strtok(NULL, ",");
        }

        first++;
        strncpy(varlist, list, strlen(list)+1);
        varg=strtok(varlist, ",");
        for (i=0; i<cnt; i++) {
            if ((web100_agent_find_var_and_group(agent, varg, &group, &var)) != WEB100_ERR_SUCCESS) {
                web100_perror("web100_agent_find_var_and_group");
                exit(EXIT_FAILURE);
            }

            if ((web100_snap_read(var, snap, buf)) != WEB100_ERR_SUCCESS) {
                web100_perror("web100_snap_read");
                exit(EXIT_FAILURE);
            }

            if (i == 0) {
                if (func) {
                    x2 = func(i, checkmz(atoi(web100_value_to_text(web100_get_var_type(var), buf))));
                }
                else {
                    x2 = checkmz(atoi(web100_value_to_text(web100_get_var_type(var), buf)));
                }
            }
            else {
                if (func) {
                    y2[i-1] = func(i, checkmz(atoi(web100_value_to_text(web100_get_var_type(var), buf))));
                }
                else {
                    y2[i-1] = checkmz(atoi(web100_value_to_text(web100_get_var_type(var), buf)));
                }
                fprintf(fn, "%s\nline %0.4f %0.4f %0.4f %0.4f\n", color[i-1], x1/1000000, y1[i-1]/1024,
                        x2/1000000, y2[i-1]/1024);
            }
            varg = strtok(NULL, ",");
        }
    }
    fprintf(fn, "go\n");

}

void
plot_cwndtime(char *name, web100_snapshot* snap,
        web100_log* log, web100_agent* agent, web100_group* group)
{
    double SndLimTimeRwin = 0, SndLimTimeSender = 0;
    char buf[256];
    web100_var* var;
    char lname[256], remport[8];
    char title[256];
    char* variables[] = {"Duration", "SndLimTimeRwin", "SndLimTimeSender", "SndLimTimeCwnd"};
    int i, first=0;
    double x1, x2, y1, y2;
    FILE *fn;

    memset(lname, 0, 256);

    get_title(snap, log, agent, group, title, remport);

    if (name == NULL) {
        fn = stdout;
        strncpy(name, "Unknown", 7);
    } else {
        strncpy(lname, name, strlen(name));
        strcat(lname, ".");
        strcat(lname, remport);
        strcat(lname, ".xpl");
        fn = fopen(lname, "w");
    }

    fprintf(fn, "double double\ntitle\n");
    fprintf(fn, "%s:%s (%s)\n", title, remport, name);
    fprintf(fn, "xlabel\nTime\nylabel\nPercentage\n");

    x1 = x2 = y1 = y2 = 0;
    first = 0;

    for (;;) {
        if (first != 0) {
            if ((web100_snap_from_log(snap, log)) != WEB100_ERR_SUCCESS) {
                fprintf(fn, "go\n");
                return;
            }
        }
        for (i=0; i<4; i++) {
            if ((web100_agent_find_var_and_group(agent, variables[i], &group, &var)) != WEB100_ERR_SUCCESS) {
                web100_perror("web100_agent_find_var_and_group");
                exit(EXIT_FAILURE);
            }

            if ((web100_snap_read(var, snap, buf)) != WEB100_ERR_SUCCESS) {
                web100_perror("web100_snap_read");
                exit(EXIT_FAILURE);
            }

            if (i == 0) {
                if (first == 0) {
                    x1 = atoi(web100_value_to_text(web100_get_var_type(var), buf));
                } else {
                    x1 = x2;
                }
            }
            else if (i == 1) {
                if (first == 0) {
                    SndLimTimeRwin = atoi(web100_value_to_text(web100_get_var_type(var), buf));
                }
            }
            else if (i == 2) {
                if (first == 0) {
                    SndLimTimeSender = atoi(web100_value_to_text(web100_get_var_type(var), buf));
                }
            }
            else {
                if (first == 0) {
                    y1 = atoi(web100_value_to_text(web100_get_var_type(var), buf));
                    y1 = y1 / (SndLimTimeRwin + SndLimTimeSender + y1);
                }
                else {
                    y1 = y2;
                }
            }
        }

        first++;
        for (i=0; i<4; i++) {
            if ((web100_agent_find_var_and_group(agent, variables[i], &group, &var)) != WEB100_ERR_SUCCESS) {
                web100_perror("web100_agent_find_var_and_group");
                exit(EXIT_FAILURE);
            }

            if ((web100_snap_read(var, snap, buf)) != WEB100_ERR_SUCCESS) {
                web100_perror("web100_snap_read");
                exit(EXIT_FAILURE);
            }

            if (i == 0) {
                x2 = atoi(web100_value_to_text(web100_get_var_type(var), buf));
            }
            else if (i == 1) {
                SndLimTimeRwin = atoi(web100_value_to_text(web100_get_var_type(var), buf));
            }
            else if (i == 2) {
                SndLimTimeSender = atoi(web100_value_to_text(web100_get_var_type(var), buf));
            }
            else {
                y2 = atoi(web100_value_to_text(web100_get_var_type(var), buf));
                y2 = y2 / (SndLimTimeRwin + SndLimTimeSender + y2);
                fprintf(fn, "%s\nline %0.4f %0.4f %0.4f %0.4f\n", color[i-1], x1/1000000, y1,
                        x2/1000000, y2);
            }
        }
    }
    fprintf(fn, "go\n");

}

void
print_var(char *varlist, web100_snapshot* snap, web100_log* log,
        web100_agent* agent, web100_group* group, void(*func)(const int arg, const int value))
{

    char *varg, savelist[256], *text;
    char buf[256], title[256], remport[8];
    int i, j;
    web100_var* var;
    FILE* fn;

    fn = stdout;
    get_title(snap, log, agent, group, title, remport);
    fprintf(fn, "Extracting Data from %s:%s connection\n\n", title, remport);

    strncpy(savelist, varlist, strlen(varlist)+1);
    printf("Index\t");
    varg = strtok(varlist, ",");
    for (j=0; ;j++) {
        if (varg == NULL)
            break;
        if (func) {
            func(-1, j);
        }
        else {
            printf("%10s\t", varg);
        }
        varg = strtok(NULL, ",");
    }
    printf("\n");
    for (i=0; ; i++) {
        if ((web100_snap_from_log(snap, log)) != WEB100_ERR_SUCCESS) {
            /* web100_perror("web100_log_open_read"); */
            printf("-------------- End Of Data  --------------\n\n");
            return;
        }
        printf("%5d\t", i);

        strncpy(varlist, savelist, strlen(savelist)+1);
        varg=strtok(varlist, ",");
        for (j=0; ; j++) {
            if (varg == NULL)
                break;
            if ((web100_agent_find_var_and_group(agent, varg, &group, &var)) != WEB100_ERR_SUCCESS) {
                web100_perror("web100_agent_find_var_and_group");
                exit(EXIT_FAILURE);
            }

            if ((web100_snap_read(var, snap, buf)) != WEB100_ERR_SUCCESS) {
                web100_perror("web100_snap_read");
                exit(EXIT_FAILURE);
            }
            if (func) {
                func(j, atoi(web100_value_to_text(web100_get_var_type(var), buf)));
            }
            else {
                text = web100_value_to_text(web100_get_var_type(var), buf);
                if (strcmp(text, "4294966376") == 0) {
                    printf("%10s\t", "-1");
                }
                else {
                    printf("%10s\t", text);
                }
            }
            varg = strtok(NULL, ",");
        }
        printf("\n");
    }

}

/* workers */
void
throughput(const int arg, const int value)
{
    static int duration;

    if (arg == -1) {
        if (value) {
            printf("%10s\t", "Throughput (kB/s)");
        }
        else {
            printf("%10s\t", "Duration");
        }
        return;
    }

    if (!arg) { /* duration */
        duration = value;
        printf("%10d\t", value);
    }
    else { /* DataBytesOut */
        printf("%10.2f", ((8*((double) value)) / ((double) duration)) * 1000000.0 / 1024.0 / 1024.0);
    }
}

int
throughputPlot(const int arg, const int value)
{
    static int duration;

    if (!arg) { /* duration */
        duration = value;
        return value;
    }
    else { /* DataBytesOut */
        return (((double) value) / ((double) duration)) * 1000000.0;
    }
}

void
cwndtime(const int arg, const int value)
{
    static int SndLimTimeRwin, SndLimTimeSender;

    if (arg == -1) {
        if (value == 0) {
            printf("%10s\t", "Duration");
        }
        else if (value == 3) {
            printf("%10s\t", "CwndTime (%% of total time)");
        }
        return;
    }

    if (arg == 0) { /* duration */
        printf("%10d\t", value);
    }
    else if (arg == 1) { /* SndLimTimeRwin */
        SndLimTimeRwin = value;
    }
    else if (arg == 2) { /* SndLimTimeSender */
        SndLimTimeSender = value;
    }
    else { /* SndLimTimeCwnd */
        printf("%10.2f", ((double) value) /
                (((double) SndLimTimeRwin) + ((double) SndLimTimeSender) + ((double) value)));
    }
}

/* --- */

int
main(int argc, char** argv)
{
    web100_agent* agent;
    web100_connection* conn;
    web100_group* group;
    web100_log* log;
    web100_snapshot* snap;
    char fn[128];
    char *varlist=NULL, list[1024];
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
                genplot_long_usage("ANL/Internet2 NDT version " VERSION " (genplot)", argv[0]);
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

    for (j=optind; j<argc; j++) {
        sprintf(fn, "%s",  argv[j]); 
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

        fprintf(stderr, "Extracting data from Snaplog '%s'\n\n", fn);

        if ((snap = web100_snapshot_alloc_from_log(log)) == NULL) {
            web100_perror("web100_snapshot_alloc_from_log");
            exit(EXIT_FAILURE);
        }

        if (plotuser == 1) {
            memset(list, 0, 1024);
            strncpy(list, "Duration,", 9);
            strncat(list, varlist, strlen(varlist));
            varg=strtok(list, ",");
            for (k=1; ; k++) {
                if ((varg=strtok(NULL, ",")) == NULL)
                    break;
            }
            memset(list, 0, 1024);
            strncpy(list, "Duration,", 9);
            strncat(list, varlist, strlen(varlist));
            if (txt == 1)
                print_var(list, snap, log, agent, group, NULL);
            else
                plot_var(list, k, "User Defined", snap, log, agent, group, NULL);
        }
        if (plotspd == 1) {
            memset(list, 0, 1024);
            strncpy(list, "Duration,DataBytesOut", 21);
            if (txt == 1)
                print_var(list, snap, log, agent, group, throughput);
            else
                plot_var(list, 2, "Throughput", snap, log, agent, group, throughputPlot);
        }
        if (plotcwndtime == 1) {
            memset(list, 0, 1024);
            strncpy(list, "Duration,SndLimTimeRwin,SndLimTimeSender,SndLimTimeCwnd", 55);
            if (txt == 1)
                print_var(list, snap, log, agent, group, cwndtime);
            else
                plot_cwndtime("Cwnd Time", snap, log, agent, group);
        }
        if (plotcwnd == 1) {
            memset(list, 0, 1024);
            strncpy(list, "Duration,CurCwnd", 16);
            if (txt == 1)
                print_var(list, snap, log, agent, group, NULL);
            else
                plot_var(list, 2, "CurCwnd", snap, log, agent, group, NULL);
        }
        if (plotrwin == 1) {
            memset(list, 0, 1024);
            strncpy(list, "Duration,CurRwinRcvd", 20);
            if (txt == 1)
                print_var(list, snap, log, agent, group, NULL);
            else
                plot_var(list, 2, "CurRwinRcvd", snap, log, agent, group, NULL);
        }
        if (plotboth == 1) {
            memset(list, 0, 1024);
            strncpy(list, "Duration,CurCwnd,CurRwinRcvd", 28);
            if (txt == 1)
                print_var(list, snap, log, agent, group, NULL);
            else
                plot_var(list, 3, "Both", snap, log, agent, group, NULL);
        }
        web100_log_close_read(log);
    }

    exit(0);
}

