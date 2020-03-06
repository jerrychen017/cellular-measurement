#ifndef UTILS_H
#define UTILS_H

/* Performs left - right and returns the result as a struct timeval.
 * In case of negative result (right > left), zero elapsed time is returned
 */
struct timeval diffTime(struct timeval left, struct timeval right);
void send_timing_packets(int s, struct sockaddr_in send_addr, int num_send); 
#endif