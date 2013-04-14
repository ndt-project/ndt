/*
 * A handful of functions to handle some web10g specific stuff
 *
 * Author: Richard Sanger
 *
 */

#include "web100srv.h"
#include "logging.h"

/*
 * These are used to pass information between web10g_connection_from_socket
 * and getremote_callback. It would be nice if these were not globals.
 */
static struct sockaddr_storage local_name;
static struct sockaddr_storage peer_name;
static int connection_id;

/**
 * Callback function used by web10g_connection_from_socket, will set
 * connection_id if the correct connection is found.
 *
 * @param ct A tuple containing connection information
 */
static void fromsocket_callback(struct tcpe_connection_tuple* ct) {
  /* I'm assuming local_name and remote_name should both be on
   * the same addressing scheme i.e. either IPv4 or IPv6 not a mix of both */
  if (local_name.ss_family == AF_INET && peer_name.ss_family == AF_INET) {
    /* We are IPv4 check if this web10g connection also is */
    if ((ct->local_addr[16]) == TCPE_ADDRTYPE_IPV4 &&
        (ct->rem_addr[16]) == TCPE_ADDRTYPE_IPV4) {
      struct sockaddr_in * ipv4_local = (struct sockaddr_in *) &local_name;
      struct sockaddr_in * ipv4_peer = (struct sockaddr_in *) &peer_name;

      /* Compare local and remote ports and addresses */
      if (ct->local_port == ntohs(ipv4_local->sin_port) &&
          ct->rem_port == ntohs(ipv4_peer->sin_port) &&
          ((struct in_addr*) ct->rem_addr)->s_addr == ipv4_peer->sin_addr.s_addr &&
          ((struct in_addr*) ct->local_addr)->s_addr == ipv4_local->sin_addr.s_addr ) {
        /* Found it */
        connection_id = ct->cid;
        log_println(2, "Matched socket to web10g IPv4 connection #%d",
                    connection_id);
      }
    }
  } else if (local_name.ss_family == AF_INET6 &&
             peer_name.ss_family == AF_INET6) {
    /* We are IPv6 check if this web10g connection also is */
    if ((ct->local_addr[16]) == TCPE_ADDRTYPE_IPV6 &&
        (ct->rem_addr[16]) == TCPE_ADDRTYPE_IPV6) {
      struct sockaddr_in6 * ipv6_local = (struct sockaddr_in6 *) &local_name;
      struct sockaddr_in6 * ipv6_peer = (struct sockaddr_in6 *) &peer_name;

      /* Compare local and remote ports and addresses */
      if (ct->local_port == ntohs(ipv6_local->sin6_port) &&
          ct->rem_port == ntohs(ipv6_peer->sin6_port) &&
          memcmp(ct->rem_addr, ipv6_peer->sin6_addr.s6_addr, sizeof(struct in6_addr)) == 0 &&
          memcmp(ct->local_addr, ipv6_local->sin6_addr.s6_addr, sizeof(struct in6_addr)) == 0) {
        /* Found it */
        connection_id = ct->cid;
        log_println(2, "Matched socket to web10g IPv6 connection #%d",
                    connection_id);
      }
    }
  } else {
    log_println(1, "WARNING: Mismatch between local and peer family");
  }
}

/**
 * Converts a IPv4-mapped IPv6 sockaddr_in6 to a sockaddr_in4
 * 
 * @param ss a sockaddr_storage
 *
 * @return if the ss represents a IPv6 mapped IPv4 address it will be converted
 * into a IPv4 sockaddr_in. Otherwise ss will remain unchanged. 
 */
void ipv4mapped_to_ipv6(struct sockaddr_storage * ss){
  if (ss->ss_family == AF_INET6){
    if (IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *) ss)->sin6_addr)){
      // Ports already in the right place so just move the actual address
      ss->ss_family = AF_INET;
      ((struct sockaddr_in *) ss)->sin_addr.s_addr = 
                    ((uint32_t *) &((struct sockaddr_in6 *) ss)->sin6_addr)[3];
    }
  }
}

/**
 * Find the web10g connection number related to a given socket.
 *
 * @param client A web10g client
 * @param sockfd The socket file descriptor
 *
 * @return The connection number if successful. If an error occurs -1
 * will be returned.
 *
 */
int web10g_connection_from_socket(struct tcpe_client* client, int sockfd) {
  socklen_t local_name_len;
  socklen_t peer_name_len;

  local_name_len = sizeof(local_name);
  peer_name_len = sizeof(peer_name);
  connection_id = -1;

  /* Get the ip address of ourself on the localsocket */
  if (getsockname(sockfd, (struct sockaddr*) &local_name,
                  &local_name_len) == -1) {
    log_println(1, "getsockname() failed: %s ", strerror(errno));
    return -1;
  }
  ipv4mapped_to_ipv6(&local_name);

  /* Get the ip address of our peer */
  if (getpeername(sockfd, (struct sockaddr*) &peer_name,
                  &peer_name_len) == -1) {
    log_println(1, "getpeername() failed: %s ", strerror(errno));
    return -1;
  }
  ipv4mapped_to_ipv6(&peer_name);

  tcpe_list_conns(client, fromsocket_callback);

  return connection_id;
}

/**
 * Find the specified web10g variable given a connection at the current
 * time.
 * Similar to web10g_find_val except this also retrieves data from web10g
 * rather than from a provided capture. If many varibles are being read
 * it's probably best to capture the data then use web10g_find_val.
 *
 * @param data A web10g data capture
 * @param name The web10g variable name
 * @param *value A pointer to a web10g value structure. If successful
 *  this will contain the requested value upon return. If an error occurs
 *  it's contents will remain untouched.
 *
 * @return int 0 if successful otherwise 1 in the event of an error
 * (including the case the specified value cannot be found).
 *
 */
int web10g_get_val(struct tcpe_client* client, int conn, char* name,
                   struct tcpe_val* value) {
  int i;
  tcpe_data* data = NULL;

  tcpe_data_new(&data);
  tcpe_read_vars(data, conn, client);

  for (i = 0; i < ARRAYSIZE(data->val); i++) {
    if (data->val[i].mask) continue;

    if (strcmp(tcpe_var_array[i].name, name) == 0) {
            value->uv64 = data->val[i].uv64;
      value->mask = data->val[i].mask;
      i = -1;
      break;
    }
  }

  tcpe_data_free(&data);
  return i == -1 ? 1 : 0;
}

/*
 * These are used to pass information between web10g_get_remote_addr
 * and getremotecallback. It would be nice if these were not globals.
 */
static int connid;
static char * remote_name;
static int remote_name_size;

/**
 * Callback function used by web10g_get_remote_addr, will fill in
 * remote_name once if the correct connection is found and set remote_name
 * to NULL to indicate to web10g_get_remote_addr that the connection was
 * found.
 *
 * @param ct A tuple containing connection information
 *
 */
static void getremote_callback(struct tcpe_connection_tuple* ct) {
  if (ct->cid != connid)
    return;
  if (ct->local_addr[16] == TCPE_ADDRTYPE_IPV4) {
    inet_ntop(AF_INET, &(ct->rem_addr[0]), remote_name, remote_name_size);
    remote_name = NULL;
  } else if (ct->local_addr[16] == TCPE_ADDRTYPE_IPV6) {
    inet_ntop(AF_INET6, &(ct->rem_addr[0]), remote_name, remote_name_size);
    remote_name = NULL;
  }
}

/**
 * Get the remote address given connection number
 *
 * @param client Web10g client
 * @param conn The web10g connection number
 * @param out A pointer to a character buffer, into which the remote
 *  address will be returned if successful. Upon error the contents are
 *  remain unchanged.
 * @param size The size of the buffer 'out'.
 *
 * @return int 0 if successful otherwise 1 in the event of an error
 * (The connection could not be found).
 *
 */
int web10g_get_remote_addr(struct tcpe_client* client, int conn, char* out,
                           int size) {
  /* Pass these to the callback routine using those globals */
  connid = conn;
  remote_name = out;
  remote_name_size = size;
  /* This will call the getremote_callback once for every tcp connection */
  tcpe_list_conns(client, getremote_callback);

  return remote_name == NULL ? 0 : 1;
}

/**
 * Find the specified web10g variables value within a provided capture.
 * Similar to web10g_get_val except this works on a previously retrieved
 * set of data.
 *
 * @param data A web10g data capture
 * @param name The web10g variable name
 * @param *value A pointer to a web10g value structure. If successful
 *  this will contain the requested value upon return. If an error occurs
 *  its contents will remain untouched.
 *
 * @return int 0 if successful otherwise 1 in the event of an error
 * (including the case the specified value cannot be found).
 *
 */
int web10g_find_val(tcpe_data* data, char* name, struct tcpe_val* value) {
  int i;

  if (data == NULL || name == NULL || value == NULL)
    return 1;

  for (i = 0; i < ARRAYSIZE(data->val); i++) {
    if (data->val[i].mask) continue;
    if (strcmp(tcpe_var_array[i].name, name) == 0) {
      value->uv64 = data->val[i].uv64;
      value->mask = data->val[i].mask;
      return 0;
    }
  }

  return 1;
}

