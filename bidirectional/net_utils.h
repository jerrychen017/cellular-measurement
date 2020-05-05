#ifndef NET_UTILS_H
#define NET_UTILS_H
#include "net_include.h"
struct sockaddr_un get_datagen_addr(bool android, socklen_t *len);
struct sockaddr_un get_controller_addr(bool android, socklen_t *len);
int setup_unix_socket(struct sockaddr_un addr, socklen_t len);

struct sockaddr_in addrbyname(const char *hostname, int port);
int setup_bound_socket(int port);
int setup_tcp_socket_send(const char *hostname, int port);
int setup_tcp_socket_recv(int port);
#endif
