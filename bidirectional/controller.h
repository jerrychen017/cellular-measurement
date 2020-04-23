#ifndef BANDWIDTH_CONTROLLER_H
#define BANDWIDTH_CONTROLLER_H

#include "net_include.h"
#include "bandwidth_utils.h"
#include "sendto_dbg.h"

/**
 * start_controller is called in android ndk to run controller 
 */
int start_controller(bool android, struct sockaddr_in send_addr, int s_server);
void stop_controller_thread();

#endif
