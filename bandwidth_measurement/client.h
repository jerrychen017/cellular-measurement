#ifndef CLIENT_H
#define CLIENT_H
#include "net_include.h"
#include "utils.h"

int client_bind(const char *address, int port);

void send_bytes_stream(int sk, struct sockaddr_in send_addr, double n, double t);

char *client_receive_timing_packets(int s, struct sockaddr_in send_addr, int delay);

char *client_send_interarrival(const char *address, int port, int num_send);
#endif

// change num_send to a variable