#ifndef ECHO_CLIENT_H
#define ECHO_CLIENT_H
#include "interactive_net_include.h"
#include "../bidirectional/bandwidth_utils.h"

/**
 * Setup a UDP socket and binds the socket
 */
int client_bind(const char* address, int port);

/**
 * Sends an UDP echo packet to the interactive server
 */
char * echo_send(const char* address, int port, int seq);
#endif //UDP_TOOLS_INTERACTIVE_CLIENT_H
