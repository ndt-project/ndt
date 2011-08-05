/*
 * This file contains the function declarations to print
 * usage screens.
 *
 * Jakub S³awiñski 2006-06-02
 * jeremian@poczta.fm
 */

#ifndef _JS_USAGE_H
#define _JS_USAGE_H

void short_usage(char* prog, char* info);
void srv_long_usage(char* info);
void clt_long_usage(char* info);
void www_long_usage(char* info);
void analyze_long_usage(char* info);
void mkmap_long_usage(char* info);
void vt_long_usage(char* info);
void genplot_long_usage(char* info, char* argv0);

#endif
