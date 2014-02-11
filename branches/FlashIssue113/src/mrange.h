/*
 * This file contains the definitions and function declarations to
 * handle number ranges.
 *
 * Jakub S³awiñski 2006-06-12
 * jeremian@poczta.fm
 */

#ifndef SRC_MRANGE_H_
#define SRC_MRANGE_H_

int mrange_parse(char* text);
char* mrange_next(char* port, size_t port_strlen);

#endif  // SRC_MRANGE_H_
