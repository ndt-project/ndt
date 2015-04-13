#ifndef SRC_WEBSOCKET_H
#define SRC_WEBSOCKET_H

int initialize_websocket_connection(int socket_fd, unsigned int skip_bytes,
                                    char* protocol);
int64_t recv_websocket_msg(int socket_fd, void* data, int64_t len);
int64_t recv_websocket_ndt_msg(int socket_fd, int* msg_type, char* msg_value,
                               int* msg_len);

int send_websocket_msg(int socket_fd, int type, const void* msg, uint64_t len);
#endif  // SRC_WEBSOCKET_H
