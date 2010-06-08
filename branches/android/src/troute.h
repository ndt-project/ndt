/*
 * This file contains the function declarations to handle troute.
 *
 * Jakub S³awiñski 2006-06-03
 * jeremian@poczta.fm
 */

#ifndef _JS_TROUTE_H
#define _JS_TROUTE_H

void find_route(u_int32_t destIP, u_int32_t IPlist[], int max_ttl);
void find_route6(char* dst, u_int32_t IPlist[][4], int max_ttl);

#endif
