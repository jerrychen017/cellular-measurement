#ifndef BANDWIDTH_UTILS_H
#define BANDWIDTH_UTILS_H
#include "net_include.h"

/**
 * Returns interval to send packets of size PACKET_SIZE
 * to acheive flow of speed Mbps (megabits per seconds)
 */
struct timeval speed_to_interval(double speed);

/**
 * Given the interval to receive num_packets of size
 * PACKET_SIZE, returns a calculated speed in Mbps
 */
double interval_to_speed(struct timeval interval, int num_packets);

/**
 * Calculates time difference between two time intervals
 */
struct timeval diffTime(struct timeval left, struct timeval right);

/**
 * Checks if the left timeval is greater than the second timeval
 */
int gtTime(struct timeval left, struct timeval right);

/**
 * deserialize struct fields from the buffer and put then in the struct
 */
void deserializeStruct(start_packet *recv_pkt, char* buf);

/**
 * manually serialize to ensure no extra struct padding
 */
int serializeStruct(start_packet *send_pkt, char* buf);

#endif 
