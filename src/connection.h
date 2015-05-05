#ifndef SRC_CONNECTION_H
#define SRC_CONNECTION_H

#include <openssl/ssl.h>

typedef struct connectionStruct {
  int socket;
  SSL* ssl;  // If ssl != NULL, then it is an SSL connection.
} Connection;

#endif  // SRC_CONNECTION_H
