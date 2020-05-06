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
void send_bandwidth_tcp(int sk, bool android);
void *send_bandwidth_pthread(void *arg);
void stop_tcp_thread();

#endif
