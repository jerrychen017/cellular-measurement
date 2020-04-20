#ifndef RECEIVE_BANDWIDTH
#define RECEIVE_BANDWIDTH

#include "net_include.h"
#include "bandwidth_utils.h"
#include "sendto_dbg.h"

#define ALPHA 0.1  // closer to 0 is smoother, closer to 1 is quicker reaction (90 packets ~ 1Mbps) 0.2/0.1 for regular
#define THRESHOLD 0.95 // percent drop threshold

int setup_server_socket(int port);
void receive_bandwidth(int sk, int predMode);

#endif