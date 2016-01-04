#ifndef SRC_WEBSOCKET_H
#define SRC_WEBSOCKET_H

#include "connection.h"

int initialize_websocket_connection(Connection* conn, unsigned int skip_bytes,
                                    char* protocol);
int64_t recv_websocket_msg(Connection* conn, void* data, int64_t len);
int64_t recv_websocket_ndt_msg(Connection* conn, int* msg_type,
                               char* msg_value, int* msg_len);

int send_websocket_msg(Connection* conn, int type, const void* msg, uint64_t len);
#endif  // SRC_WEBSOCKET_H
