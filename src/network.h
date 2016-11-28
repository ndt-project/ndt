/**
 * This file contains the definitions and function declarations to
 * handle network related stuff.
 *
 * Jakub S�awi�ski 2006-05-30
 * jeremian@poczta.fm
 */

#ifndef SRC_NETWORK_H_
#define SRC_NETWORK_H_

#include <I2util/util.h>
#include "connection.h"

#define NDT_BACKLOG 5
#define BUFFSIZE  8192

#define OPT_IPV6_ONLY 1
#define OPT_IPV4_ONLY 2

#define JSON_SUPPORT 1
#define WEBSOCKET_SUPPORT 2
#define TLS_SUPPORT 4

I2Addr CreateListenSocket(I2Addr addr, char* serv, int options, int buf_size);
int CreateConnectSocket(int* sockfd, I2Addr local_addr, I2Addr server_addr,
                        int option, int buf_sizes);
int send_json_msg_any(Connection* conn, int type, const char* msg, int len,
		      int connectionFlags, int jsonConvertType,
                      const char *keys, const char *keysDelimiters,
                      const char *values, char *valuesDelimiters);
int send_json_message_any(Connection* ctl, int type, const char* msg, int len,
                          int connectionFlags, int jsonConvertType);
int send_msg_any(Connection* conn, int type, const void* msg, int len);
int recv_msg_any(Connection* conn, int* type, void* msg, int* len);
int recv_any_msg(Connection* conn, int* type, void* msg, int* len,
                 int connectionFlags);
int writen_any(Connection* conn, const void* buf, int amount);
size_t readn_any(Connection* conn, void* buf, size_t amount);

/* web100-util.c routine used in network. */
int KillHung(void);

int setup_SSL_connection(Connection *conn, SSL_CTX *ctx);
int is_recoverable_ssl_error(int ssl_error, int ssl_errno);
const char* ssl_error_str(int ssl_error);

void shutdown_connection(Connection *conn);
void close_connection(Connection *conn);

void set_socket_timeout_or_die(int sockfd);

#endif  // SRC_NETWORK_H_
