#ifndef BANDWIDTH_UTILS_H
#define BANDWIDTH_UTILS_H

sstruct timeval diffTime(struct timeval left, struct timeval right);
int gtTime(struct timeval left, struct timeval right); 

struct timeval speed_to_interval(double speed);
double interval_to_speed(struct timeval interval, int num_packets);

#endif 
