#ifndef BANDWIDTH_UTILS_H
#define BANDWIDTH_UTILS_H
#include "net_include.h"

struct timeval diffTime(struct timeval left, struct timeval right);
int gtTime(struct timeval left, struct timeval right); 

struct timeval speed_to_interval(double speed);
double interval_to_speed(struct timeval interval, int num_packets);

void deserializeStruct(start_packet *recv_pkt, char* buf);
int serializeStruct(start_packet *send_pkt, char* buf);

#endif 
