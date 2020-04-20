#include "net_include.h"
#include "bandwidth_utils.h"

/* 
 * Returns interval to send packets of size PACKET_SIZE
 * to acheive flow of speed Mbps (megabits per seconds)
 */
struct timeval speed_to_interval(double speed)
{
    struct timeval ret;
//    long long usec = 1000000l * PACKET_SIZE * 8l / (speed * 1024 * 1024);
    // we use 0.9536743164 for 1000000/1024/1024 to avoid long overflow
    long usec = 0.9536743164 * PACKET_SIZE * 8l / (speed);
    ret.tv_sec = usec / 1000000;
    ret.tv_usec = usec % 1000000;
    return ret;
}

/* 
 * Given the interval to receive num_packets of size
 * PACKET_SIZE, returns a calculated speed in Mbps
 */
double interval_to_speed(struct timeval interval, int num_packets)
{
    long usec = interval.tv_usec + interval.tv_sec * 1000000l;
    double ret = num_packets * PACKET_SIZE * 8.0/(usec);
    return 0.9536743164 * ret;
}

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
//        printf("WARNING: diffTime has negative result, returning 0!\n");
        diff.tv_sec = diff.tv_usec = 0;
    }
    return diff;
}

int gtTime(struct timeval left, struct timeval right) 
{
    return (left.tv_sec > right.tv_sec) ||
        (left.tv_sec == right.tv_sec && left.tv_usec > right.tv_usec);
}
