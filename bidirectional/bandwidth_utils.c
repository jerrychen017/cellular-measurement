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

void deserializeStruct(start_packet *recv_pkt, char* buf){
    int offset = 0;
    memcpy(&recv_pkt->type, buf + offset, sizeof(recv_pkt->type));
    offset += sizeof(recv_pkt->type);
    memcpy(&recv_pkt->params.burst_size, buf + offset, sizeof(recv_pkt->params.burst_size));
    offset += sizeof(recv_pkt->params.burst_size);
    memcpy(&recv_pkt->params.interval_size, buf + offset, sizeof(recv_pkt->params.interval_size));
    offset += sizeof(recv_pkt->params.interval_size);
    memcpy(&recv_pkt->params.grace_period , buf + offset, sizeof(recv_pkt->params.grace_period));
    offset += sizeof(recv_pkt->params.grace_period);
    memcpy(&recv_pkt->params.instant_burst , buf + offset, sizeof(recv_pkt->params.instant_burst));
    offset += sizeof(recv_pkt->params.instant_burst);
    memcpy(&recv_pkt->params.pred_mode, buf + offset, sizeof(recv_pkt->params.pred_mode));
    offset += sizeof(recv_pkt->params.pred_mode);
    memcpy(&recv_pkt->params.use_tcp, buf + offset, sizeof(recv_pkt->params.use_tcp));
    offset += sizeof(recv_pkt->params.use_tcp);
    memcpy(&recv_pkt->params.alpha , buf + offset, sizeof(recv_pkt->params.alpha));
    offset += sizeof(recv_pkt->params.alpha);
    memcpy(&recv_pkt->params.threshold , buf + offset, sizeof(recv_pkt->params.threshold));
    offset += sizeof(recv_pkt->params.threshold);
    memcpy(&recv_pkt->params.interval_time, buf + offset, sizeof(recv_pkt->params.interval_time));
    offset += sizeof(recv_pkt->params.interval_time);
    memcpy(&recv_pkt->params.min_speed, buf + offset, sizeof(recv_pkt->params.min_speed));
    offset += sizeof(recv_pkt->params.min_speed);
    memcpy(&recv_pkt->params.max_speed, buf + offset, sizeof(recv_pkt->params.max_speed));
    offset += sizeof(recv_pkt->params.max_speed);
    memcpy(&recv_pkt->params.start_speed, buf + offset, sizeof(recv_pkt->params.start_speed));
    offset += sizeof(recv_pkt->params.start_speed);
}

// manually serialize to ensure no extra struct paddting
int serializeStruct(start_packet *send_pkt, char* buf){
    int offset = 0;
    memcpy(buf + offset, &send_pkt->type, sizeof(send_pkt->type));
    offset += sizeof(send_pkt->type);
    memcpy(buf + offset , &send_pkt->params.burst_size, sizeof(send_pkt->params.burst_size));
    offset += sizeof(send_pkt->params.burst_size);
    memcpy(buf + offset , &send_pkt->params.interval_size, sizeof(send_pkt->params.interval_size));
    offset += sizeof(send_pkt->params.interval_size);
    memcpy(buf + offset , &send_pkt->params.grace_period, sizeof(send_pkt->params.grace_period));
    offset += sizeof(send_pkt->params.grace_period);
    memcpy(buf + offset , &send_pkt->params.instant_burst, sizeof(send_pkt->params.instant_burst));
    offset += sizeof(send_pkt->params.instant_burst);
    memcpy(buf + offset, &send_pkt->params.pred_mode, sizeof(send_pkt->params.pred_mode));
    offset += sizeof(send_pkt->params.pred_mode);
    memcpy(buf + offset, &send_pkt->params.use_tcp, sizeof(send_pkt->params.use_tcp));
    offset += sizeof(send_pkt->params.use_tcp);
    memcpy(buf + offset, &send_pkt->params.alpha, sizeof(send_pkt->params.alpha));
    offset += sizeof(send_pkt->params.alpha);
    memcpy(buf + offset, &send_pkt->params.threshold, sizeof(send_pkt->params.threshold));
    offset += sizeof(send_pkt->params.threshold);
    memcpy(buf + offset, &send_pkt->params.interval_time, sizeof(send_pkt->params.interval_time));
    offset += sizeof(send_pkt->params.interval_time);
    memcpy(buf + offset, &send_pkt->params.min_speed, sizeof(send_pkt->params.min_speed));
    offset += sizeof(send_pkt->params.min_speed);
    memcpy(buf + offset , &send_pkt->params.max_speed, sizeof(send_pkt->params.max_speed));
    offset += sizeof(send_pkt->params.max_speed);
    memcpy(buf + offset , &send_pkt->params.start_speed, sizeof(send_pkt->params.start_speed));
    offset += sizeof(send_pkt->params.start_speed);

    return offset;
}