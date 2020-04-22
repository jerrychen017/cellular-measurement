#ifndef RECEIVE_BANDWIDTH
#define RECEIVE_BANDWIDTH

#include "net_include.h"
#include "bandwidth_utils.h"
#include "sendto_dbg.h"

struct recv_bandwidth_args
{
    int sk;
    int pred_mode;
    struct sockaddr_in expected_addr;
};

void stop_receiving_thread();
void receive_bandwidth(int sk, int predMode, struct sockaddr_in expected_addr);
void *receive_bandwidth_pthread(void *);

#endif
