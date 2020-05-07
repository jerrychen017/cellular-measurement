#ifndef BANDWIDTH_CONTROLLER_H
#define BANDWIDTH_CONTROLLER_H

#include "net_include.h"
#include "bandwidth_utils.h"

/**
 * starts controller and connects to data generator
 */
int start_controller(bool android, struct sockaddr_in send_addr, int s_server, struct parameters params);

/**
 * Breaks out controller select loop and stops controller thread
 */
void stop_controller_thread();

#endif
