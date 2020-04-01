#ifndef INTERACTIVE_CLIENT_H
#define INTERACTIVE_CLIENT_H
#include "include/net_include.h"
#include "utils.h"

int interactive_connect(int id, char name[NAME_LENGTH]);

/**
 * send an interactive packet to the server and returns the sequence number
 * @param x x_coordinate
 * @param y y_coordinate
 * @return status code
 */
int send_interactive_packet(int seq_num, float x, float y);

/**
 * receive an interactive packet with a certain sequence number
 * @return EchoPacket
 */
EchoPacket receive_interactive_packet();

void init_socket(const char *address, int port);

#endif //UDP_TOOLS_ECHO_CLIENT_H
