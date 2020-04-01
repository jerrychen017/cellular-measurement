#ifndef BANDWIDTH_CONTROLLER_H
#define BANDWIDTH_CONTROLLER_H

#include "net_include.h"
#include "bandwidth_utils.h"
#include "sendto_dbg.h"

void startup(int s_server, int s_data, struct sockaddr_in send_addr);
void control(int s_server, int s_data, struct sockaddr_in send_addr);
double estimate_change(double rate);

int setup_data_socket();
int setup_server_socket(int port);
struct sockaddr_in addrbyname(const char *hostname, int port);

/**
 * start_controller is called in android ndk to run controller 
 */
int start_controller(const char* address, int port);

#endif
