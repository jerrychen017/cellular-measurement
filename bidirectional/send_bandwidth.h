#include "net_include.h"
struct send_bandwidth_args {
    struct sockaddr_in addr; 
    int port; 
};
void send_bandwidth(void * arg);