#ifndef INTERACTIVE_CLIENT_H
#define INTERACTIVE_CLIENT_H
#include "interactive_net_include.h"
#include "../bandwidth_measurement/bandwidth_utils.h"

int interactive_connect(const char name[NAME_LENGTH]);

/**
 * send an interactive packet to the server and returns the sequence number
 * @param x x_coordinate
 * @param y y_coordinate
 * @return status code
 */
int send_interactive_packet(int seq_num, float x, float y);

/**
 * receive an interactive packet with a certain sequence number
 * @return InteractivePacket
 */
InteractivePacket receive_interactive_packet();

void init_socket(const char *address, int port);

#endif //UDP_TOOLS_ECHO_CLIENT_H
