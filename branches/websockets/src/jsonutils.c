/*
 * This file contains functions to handle json messages.
 * Jansson library is used for encoding/decoding JSON strings.
 * See http://www.digip.org/jansson/ for more details.
 *
 * Sebastian Kostuch 2014-04-30
 * skostuch@soldevelo.com
 */

#include <ctype.h>
#include <jansson.h>
#include <string.h>
#include "jsonutils.h"
#include "logging.h"

/**
 * Creates string representing JSON object with single key:value pair
 * where key is default and value is being taken from parameter.
 *
 * @param value null-terminated string containing value of map entry
 * @return encoded JSON string
 */
char* json_create_from_single_value(const char* value) {
	json_t *root;

	root = json_object();
	json_object_set_new(root, DEFAULT_KEY, json_string(value));

	char* ret = json_dumps(root, 0);
	json_decref(root);

	return ret;
}

/**
 * Creates string representing JSON object with multiple key:value pairs
 * where keys are taken from "keys" parameter (separated by "keys_separator"
 * character) and values are taken from "values" parameter (separated by
 * "values_separator" character). Number of elements in both keys and values
 * should match.
 *
 * @param keys null-terminated string containing keys for JSON map entries
 * @param keys_delimiters characters by which keys are separated
 * @param values null-terminated string containing values for JSON map entries
 * @param values_delimiters characters by which values are separated
 * @return encoded JSON string
 */
char* json_create_from_multiple_values(const char *keys, const char *keys_delimiters,
		                               const char *values, char *values_delimiters) {
	json_t *root;
	char keysbuff[8192], valuesbuff[8192];
	char *saveptr1, *saveptr2;

	root = json_object();
	strncpy(keysbuff, keys, strlen(keys));
	keysbuff[strlen(keys)] = '\0';

	strncpy(valuesbuff, values, strlen(values));
	valuesbuff[strlen(values)] = '\0';

	char *keyToken = strtok_r(keysbuff, keys_delimiters, &saveptr1);
	char *valueToken = strtok_r(valuesbuff, values_delimiters, &saveptr2);

	while (keyToken && valueToken) {
		json_object_set_new(root, keyToken, json_string(valueToken));

		keyToken = strtok_r(NULL, keys_delimiters, &saveptr1);
		valueToken = strtok_r(NULL, values_delimiters, &saveptr2);
	}

	char* ret = json_dumps(root, 0);
	json_decref(root);

	return ret;
}

/**
 * Creates string representing JSON object with proper key:value pairs.
 * Keys and values are taken from parameter and are separated by colon
 * while pairs are separated by new line character, e.g.:
 *  key1: value1
 *  key2: value2
 *
 * @param pairs null-terminated string containing key:value pairs
 * @return encoded JSON string
 */
char* json_create_from_key_value_pairs(const char* pairs) {
	json_t *root;
	char key[1024], value[1024], buff[8192];
	int i;
	char *saveptr;

	root = json_object();
	strncpy(buff, pairs, strlen(pairs));
	buff[strlen(pairs)] = '\0';
	char *token = strtok_r(buff, "\n", &saveptr);

	while (token) {
		i = strcspn(token, ":");
		strncpy(key, token, i);
		key[i] = '\0';
		while(isspace(token[i+1])) { // skip whitespace characters from beginning of value if any
			i++;
		}
		strncpy(value, token+i+1, strlen(token)-i);
		value[strlen(token)-i] = '\0';
		json_object_set_new(root, key, json_string(value));
		token = strtok_r(NULL, "\n", &saveptr);
	}

	char* ret = json_dumps(root, 0);
	json_decref(root);

	return ret;
}

/**
 * Reads value from JSON object represented by jsontext using specific key
 *
 * @param jsontext string representing JSON object
 * @param key by which value should be obtained from JSON map
 */
char* json_read_map_value(const char *jsontext, const char *key) {
	json_t *root;
	json_error_t error;
	json_t *data;

	root = json_loads(jsontext, 0, &error);

	if(!root)
	{
		log_println(0, "Error while reading value from JSON string: %s", error.text);
		return NULL;
	}

	if(!json_is_object(root))
	{
		log_println(0, "Error while reading value from JSON string: root is not an object");
		json_decref(root);
		return NULL;
	}

	data = json_object_get(root, key);
	if(data != NULL)
	{
		char *value = (char *)json_string_value(data);

		return value;
	}
	else
		return NULL;
}

/**
 * Checks if given msg has correct JSON format
 * @param msg message to check for correct JSON format
 * @return 0 if msg is correct JSON message
 *         -1 otherwise
 */
int json_check_msg(const char* msg) {
	json_t *root;
	json_error_t error;

	root = json_loads(msg, 0, &error);

	if(!root)
	{
		return -1;
	}

	return 0;
}
