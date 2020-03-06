#include "net_include.h"
static TimingPacket timing, *recvTiming;

/**
 * Calculates time difference between two time intervals
 */
struct timeval diffTime(struct timeval left, struct timeval right)
{
    struct timeval diff;
    diff.tv_sec = left.tv_sec - right.tv_sec;
    diff.tv_usec = left.tv_usec - right.tv_usec;
    if (diff.tv_usec < 0)
    {
        diff.tv_usec += 1000000;
        diff.tv_sec--;
    }
    if (diff.tv_sec < 0)
    {
        printf("WARNING: diffTime has negative result, returning 0!\n");
        diff.tv_sec = diff.tv_usec = 0;
    }
    return diff;
}

/**
 * Sends num_send timing packets with sequence numbers to send_addr using socket sk. 
 * Mark the last packet with sequence number -1
 */
void send_timing_packets(int sk, struct sockaddr_in send_addr, int num_send)
{
    int seq = 0;
    for (int i = 0; i < NUM_SEND - 1; i++)
    {
        timing.type = TIMING; // TODO: is it needed? it only needs to be set once right?
        timing.seq = seq;
        sendto(sk, &timing, sizeof(timing), 0,
               (struct sockaddr *)&send_addr, sizeof(send_addr));
        seq++;
    }
    timing.type = TIMING;
    timing.seq = -1;
    sendto(sk, &timing, sizeof(timing), 0,
           (struct sockaddr *)&send_addr, sizeof(send_addr));
}