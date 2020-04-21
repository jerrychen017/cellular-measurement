#ifndef SEND_BANDWIDTH
#define SEND_BANDWIDTH
#include "net_include.h"
struct send_bandwidth_args {
    struct sockaddr_in addr; 
    int port; 
    bool android; 
};
void * send_bandwidth(void * arg);

#endif
