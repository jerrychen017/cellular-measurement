#ifndef NET_UTILS_H
#define NET_UTILS_H
#include "net_include.h"

/**
 * Gets the data generator local addr based on if running on Android
 */
struct sockaddr_un get_datagen_addr(bool android, socklen_t *len);

/**
 * Gets the controller local addr based on if running on Android
 */
struct sockaddr_un get_controller_addr(bool android, socklen_t *len);

/**
 * Setup unix socket, binds and connects to addr
 */
int setup_unix_socket(struct sockaddr_un addr, socklen_t len);

/**
 * given host name and port, get address
 */
struct sockaddr_in addrbyname(const char *hostname, int port);

/**
 * Setup UDP socket and bind
 */
int setup_bound_socket(int port);

/**
 * Setup TCP socket and connect to the host
 */
int setup_tcp_socket_send(const char *hostname, int port);

/**
 * Setup TCP socket, bind and listen
 */
int setup_tcp_socket_recv(int port);
#endif
