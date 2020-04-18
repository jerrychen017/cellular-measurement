#ifndef NET_UTILS_H
#define NET_UTILS_H
#include "net_include.h"
struct sockaddr_un get_datagen_addr(bool android, socklen_t *len);
struct sockaddr_un get_controller_addr(bool android, socklen_t *len);

int setup_unix_socket(struct sockaddr_un *send_addr, struct sockaddr_un *recv_addr,
        socklen_t send_len, socklen_t recv_len);

struct sockaddr_in addrbyname(const char *hostname, int port);
#endif
