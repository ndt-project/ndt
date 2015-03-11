/*
 * This file contains function declarations to handle json messages
 *
 * Sebastian Kostuch 2014-04-30
 * skostuch@soldevelo.com
 */

#ifndef SRC_JSONUTILS_H_
#define SRC_JSONUTILS_H_

#include <jansson.h>
#include <string.h>

#define JSON_SINGLE_VALUE 1
#define JSON_MULTIPLE_VALUES 2
#define JSON_KEY_VALUE_PAIRS 3
#define DEFAULT_KEY "msg"

char* json_create_from_single_value(const char* value);
char* json_create_from_multiple_values(const char *keys, const char *keys_delimiters,
		                               const char *values, char *values_delimiters);
char* json_create_from_key_value_pairs(const char* pairs);
char* json_read_map_value(const char *jsontext, const char *key);
int json_check_msg(const char* msg);

#endif  // SRC_JSONUTILS_H_
