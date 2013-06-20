/**
 * A handful of functions to handle some Web10G specific stuff.
 * This provides some functions similar to those in the Web100 library
 * for Web10G.
 * 
 * Naming conventions
 * web10g_* for functions added to ndt that extend the Web10G library, 
 * like everything in this file. The actual Web10G library uses 
 * estats_*.
 * 
 * tcp_stat_* is used for types and functions intended to be used 
 * interchangably between web10g and web100. 
 *
 * Author: Richard Sanger
 *         rsangerarj@gmail.com
 */

#include "web100srv.h"
#include "logging.h"
#include "utils.h"

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
int web10g_connection_from_socket(tcp_stat_agent* client, int sockfd) {
  struct sockaddr_storage local_name;
  struct sockaddr_storage peer_name;
  socklen_t local_name_len = sizeof(local_name);
  socklen_t peer_name_len = sizeof(peer_name);
  int connection_id = -1;
  struct estats_connection_list* clist = NULL;
  struct estats_list* list_pos;

  /* Get the ip address of ourself on the localsocket */
  if (getsockname(sockfd, (struct sockaddr*) &local_name,
                  &local_name_len) == -1) {
    log_println(1, "getsockname() failed: %s ", strerror(errno));
    return -1;
  }
  /* IPv6 socket connected to IPv4? Convert to a real IPv4 address */
  ipv4mapped_to_ipv4(&local_name);

  /* Get the ip address of our peer */
  if (getpeername(sockfd, (struct sockaddr*) &peer_name,
                  &peer_name_len) == -1) {
    log_println(1, "getpeername() failed: %s ", strerror(errno));
    return -1;
  }
  ipv4mapped_to_ipv4(&peer_name);

  /* We have our sockaddrs so find the match in the Web10g table */
  estats_connection_list_new(&clist);
  estats_list_conns(clist, client);


  ESTATS_LIST_FOREACH(list_pos, &(clist->connection_head)) {
    struct estats_connection* cp =
                  ESTATS_LIST_ENTRY(list_pos, estats_connection, list);
    struct estats_connection_tuple* ct =
                                  (struct estats_connection_tuple*) cp;

    if (local_name.ss_family == AF_INET &&
        peer_name.ss_family == AF_INET &&
        ct->local_addr[16] == ESTATS_ADDRTYPE_IPV4 &&
        ct->rem_addr[16] == ESTATS_ADDRTYPE_IPV4) {
        /* All IPv4 - compare addresses */
      struct sockaddr_in * ipv4_local =
                                  (struct sockaddr_in *) &local_name;
      struct sockaddr_in * ipv4_peer =
                                  (struct sockaddr_in *) &peer_name;

      /* Compare local and remote ports and addresses */
      if (ct->local_port == ntohs(ipv4_local->sin_port) &&
          ct->rem_port == ntohs(ipv4_peer->sin_port) &&
          ((struct in_addr*) ct->rem_addr)->s_addr ==
                    ipv4_peer->sin_addr.s_addr &&
          ((struct in_addr*) ct->local_addr)->s_addr ==
                    ipv4_local->sin_addr.s_addr ) {
        /* Found it */
        connection_id = ct->cid;
        log_println(2, "Matched socket to web10g IPv4 connection #%d",
                                                      connection_id);
        goto Cleanup;
      }
    } else if (local_name.ss_family == AF_INET6 &&
                peer_name.ss_family == AF_INET6 &&
                ct->local_addr[16] == ESTATS_ADDRTYPE_IPV6 &&
                ct->rem_addr[16] == ESTATS_ADDRTYPE_IPV6) {
      /* We are IPv6  - compare addresses */
      struct sockaddr_in6 * ipv6_local =
                  (struct sockaddr_in6 *) &local_name;
      struct sockaddr_in6 * ipv6_peer =
                  (struct sockaddr_in6 *) &peer_name;

      /* Compare local and remote ports and addresses */
      if (ct->local_port == ntohs(ipv6_local->sin6_port) &&
          ct->rem_port == ntohs(ipv6_peer->sin6_port) &&
          memcmp(ct->rem_addr, ipv6_peer->sin6_addr.s6_addr,
                sizeof(struct in6_addr)) == 0 &&
          memcmp(ct->local_addr, ipv6_local->sin6_addr.s6_addr,
                sizeof(struct in6_addr)) == 0) {
        /* Found it */
        connection_id = ct->cid;
        log_println(2, "Matched socket to web10g IPv6 connection #%d",
                                                        connection_id);
        goto Cleanup;
      }
    }
  }

 Cleanup:
  estats_connection_list_free(&clist);
  return connection_id;
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
int web10g_get_remote_addr(tcp_stat_agent* client,
                      tcp_stat_connection conn, char* out, int size) {
  struct estats_connection_list* clist = NULL;
  struct estats_list* list_pos;

  estats_connection_list_new(&clist);
  estats_list_conns(clist, client);
  out[0] = 0;

  ESTATS_LIST_FOREACH(list_pos, &(clist->connection_head)) {
    struct estats_connection* cp =
                  ESTATS_LIST_ENTRY(list_pos, estats_connection, list);
    struct estats_connection_tuple* ct =
                                  (struct estats_connection_tuple*) cp;

    if (ct->cid == conn) {
      if (ct->local_addr[16] == ESTATS_ADDRTYPE_IPV4) {
        inet_ntop(AF_INET, &(ct->rem_addr[0]), out, size);
        log_println(1, "Got remote address IPv4 %s", out);
        goto Cleanup;
      } else if (ct->local_addr[16] == ESTATS_ADDRTYPE_IPV6) {
        inet_ntop(AF_INET6, &(ct->rem_addr[0]), out, size);
        log_println(1, "Got remote address IPv6 %s", out);
        goto Cleanup;
      }
    }
  }

 Cleanup:
  estats_connection_list_free(&clist);
  return out[0] == 0 ? 1 : 0;
}

/**
 * Find the specified web10g variables value within a provided capture.
 * Similar to web10g_get_val except this works on a previously retrieved
 * set of data.
 *
 * This also handle some special cases to get web100 variables that
 * no longer exist in web10g namely:
 * MaxCwnd = MAX(MaxSsCwnd, MaxCaCwnd)
 * AckSegsIn = SegsIn - DataSegsIn 
 * AckSegsOut = SegsOut - DataSegsOut
 * 
 * @param data A web10g data capture
 * @param name The web10g variable name
 * @param *value A pointer to a web10g value structure. If successful
 *  this will contain the requested value upon return. If an error occurs
 *  its contents will remain untouched.
 *
 * @return the type if successful otherwise -1 in the event of an error
 * (including the case the specified value cannot be found).
 *
 */
int web10g_find_val(const tcp_stat_snap* data, const char* name,
                                  struct estats_val* value) {
  int i;

  if (data == NULL || name == NULL || value == NULL)
    return -1;

  /* Special cases for old web100 variables that no longer have a direct
   * mapping (i.e. must be calculated from others) */

  if (strcmp(name, "MaxCwnd") == 0) {
    /*  = MAX(MaxSsCwnd, MaxCaCwnd) */
    struct estats_val ss = {0};
    struct estats_val ca = {0};
    int type;
    /* Types should be the same, failing that an error (-1) will take 
     * presidence when or'd (uv32's) */
    type = web10g_find_val(data, "MaxSsCwnd", &ss);
    type |= web10g_find_val(data, "MaxCaCwnd", &ca);
    /* Get the max of the two */
    if (ss.uv32 > ca.uv32) {
      value->uv64 = ss.uv64;
      value->masked = ss.masked;
    } else {
      value->uv64 = ca.uv64;
      value->masked = ca.masked;
    }
    return type;
  }

  if (strcmp(name, "AckSegsIn") == 0) {
    /* = SegsIn - DataSegsIn */
    struct estats_val si = {0};
    struct estats_val dsi = {0};
    int type;
    /* Both uv32's */
    type = web10g_find_val(data, "SegsIn", &si);
    type |= web10g_find_val(data, "DataSegsIn", &dsi);

    value->masked = 0;
    value->uv32 = si.uv32 - dsi.uv32;
    return type;
  };

  if (strcmp(name, "AckSegsOut") == 0) {
    /* = SegsOut - DataSegsOut */
    struct estats_val so = {0};
    struct estats_val dso = {0};
    int type;
    /* Both uv32's */
    type = web10g_find_val(data, "SegsOut", &so);
    type |= web10g_find_val(data, "DataSegsOut", &dso);

    value->masked = 0;
    value->uv32 = so.uv32 - dso.uv32;
    return type;
  };

  for (i = 0; i < data->length; i++) {
    if (data->val[i].masked) continue;

    if (strcmp(estats_var_array[i].name, name) == 0) {
      value->uv64 = data->val[i].uv64;
      value->masked = data->val[i].masked; /* = 0*/
      return estats_var_array[i].valtype;
    }
  }
  log_println(0, "WARNING: Web10G failed to find name=%s", name);
  return -1;
}

/**
 * Find the specified web10g variable given a connection at the current
 * time.
 * Similar to web10g_find_val except this also retrieves data from web10g
 * rather than from a provided capture. If many variables are being read
 * it's probably best to capture the data then use web10g_find_val.
 *
 * @param data A web10g data capture
 * @param name The web10g variable name
 * @param *value A pointer to a web10g value structure. If successful
 *  this will contain the requested value upon return. If an error occurs
 *  it's contents will remain untouched.
 *
 * @return the type if successful otherwise -1 in the event of an error
 * (including the case the specified value cannot be found).
 *
 */
int web10g_get_val(tcp_stat_agent* client, tcp_stat_connection conn,
                        const char* name, struct estats_val* value) {
  int ret;
  estats_error* err;
  estats_val_data* data = NULL;

  if ((err = estats_val_data_new(&data)) != NULL) {
    estats_error_print(stderr, err);
    estats_error_free(&err);
    return -1;
  }

  if ((err = estats_read_vars(data, conn, client)) != NULL) {
    estats_error_print(stderr, err);
    estats_error_free(&err);
    estats_val_data_free(&data);
    return -1;
  }

  ret = web10g_find_val(data, name, value);
  estats_val_data_free(&data);
  return ret;
}
