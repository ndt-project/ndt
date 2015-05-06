#ifndef SRC_NETWORK_CLT_H_
#define SRC_NETWORK_CLT_H_

#include "network.h"

int send_json_msg(int ctlSocket, int type, const char* msg, int len,
		  int connectionFlags, int jsonConvertType,
                  const char *keys, const char *keysDelimiters,
                  const char *values, char *valuesDelimiters);
int send_json_message(int ctlSocket, int type, const char* msg, int len,
                      int connectionFlags, int jsonConvertType);
int send_msg(int ctlSocket, int type, const void* msg, int len);
int recv_msg(int ctlSocket, int* type, void* msg, int* len);
int writen(int fd, const void* buf, int amount);
size_t readn(int fd, void* buf, size_t amount);

#endif  // SRC_NETWORK_CLT_H_
