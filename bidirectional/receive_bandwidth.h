#ifndef RECEIVE_BANDWIDTH
#define RECEIVE_BANDWIDTH

#include "net_include.h"
#include "bandwidth_utils.h"
#include "sendto_dbg.h"

struct recv_bandwidth_args
{
    int sk;
    int pred_mode;
};
void receive_bandwidth(int sk, int predMode);
void *receive_bandwidth_pthread(void *);

#endif
