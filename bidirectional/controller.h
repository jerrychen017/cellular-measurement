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
int start_controller(const char* address, int port, bool android);
void stop_controller_thread();

#endif
