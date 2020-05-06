#ifndef SEND_BANDWIDTH
#define SEND_BANDWIDTH
#include "net_include.h"
struct send_bandwidth_args
{
    struct sockaddr_in addr;
    int sk;
    bool android;
    struct parameters params;
};
void send_bandwidth(struct sockaddr_in addr, int sk, bool android, struct parameters params);
void client_send_bandwidth_tcp(int sk);
void server_send_bandwidth_tcp(int sk);
void *send_bandwidth_pthread(void *arg);
void * server_send_bandwidth_tcp_pthread(void * args);
void stop_tcp_send_thread();

#endif
