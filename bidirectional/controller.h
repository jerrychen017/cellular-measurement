#ifndef BANDWIDTH_CONTROLLER_H
#define BANDWIDTH_CONTROLLER_H

#include "net_include.h"
#include "bandwidth_utils.h"
#include "sendto_dbg.h"

double estimate_change(double rate);

int setup_server_socket(int port, bool android);

/**
 * start_controller is called in android ndk to run controller 
 */
int start_controller(bool android, const char* address, int port, struct sockaddr_in sk_addr);
void stop_controller_thread();

#endif