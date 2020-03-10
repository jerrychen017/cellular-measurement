#include "client.h"

static TimingPacket timing, *recvTiming;
static ReportPacket report, *recvReport;
static Packet pack, *packet = &pack;

/**
 * bind port with address
 * returns 1 if an error occurred or 0 if binding was successful
 */
int client_bind(const char *address, int port)
{
    // server socket address
    struct sockaddr_in server_addr;

    // socket both for sending and receiving
    int sk = socket(AF_INET, SOCK_DGRAM, 0);
    if (sk < 0)
    {
        printf("echo_client: socket error\n");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // bind socket with port
    if (bind(sk, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("echo_client: bind error\n"); // prints a warning but don't exit.
        // we can still send since it's already binded
        return 1;
    }
    return 0;
}

// send bytes stream with the given speed
// speed n is in Mbits per second
// send for t seconds
void send_bytes_stream(int sk, struct sockaddr_in send_addr, double n, double t)
{
    // n Mbits = 125000*n bytes
    // 1 / (125000*n) second = 8/n microseconds
    // we send a byte per 8/n microsecond
    // PACKET_SIZE bytes per 8*PACKET_SIZE/n microsend

    fd_set mask;
    int num;

    FD_ZERO(&mask);
    FD_SET(sk, &mask);

    struct timeval timeout;
    int seq = 0;
    double t_usec = t * 1000000;
    for (;;)
    {
        // timeout limit
        timeout.tv_sec = 0;
        timeout.tv_usec = 8 * PACKET_SIZE / n;

        num = select(FD_SETSIZE, NULL, NULL, NULL, &timeout);

        if (num > 0)
        {
        }
        else
        { //timeout
            if (t_usec > 0)
            {
                timing.type = 0;
                timing.seq = seq;
                sendto(sk, &timing, sizeof(timing), 0,
                       (struct sockaddr *)&send_addr, sizeof(send_addr));
                seq++;
                t_usec -= timeout.tv_usec;
            }
            else
            {
                return;
            }
        }
    }
}

/* Sends a stream of ~MTU (1400 bytes) packets to echo server 
 * to test latency, after 5 seconds in order to ignore lost packets.
 *
 * delay is ms between packets, n is total num. Timeouts 
 * s is the socekt, send_addr is the address of echo server
 *
 * Should report delay of each and avg delay
 */
char *client_receive_timing_packets(int s, struct sockaddr_in send_addr, int delay)
{
    char *output = malloc(sizeof(char) * NUM_SEND * 200);

    fd_set mask;
    fd_set read_mask;
    int num;
    FD_ZERO(&mask);
    FD_SET(s, &mask);

    // Compute interarrival time on client side as well
    //Using Monotonically increasing clock
    struct timespec ts_curr;
    struct timespec ts_prev;

    struct timespec ts_start;
    struct timespec ts_end;

    struct timeval timeout;

    double avg_interarrival = 0;
    int count = 0;
    int next_num = 0;
    int dropped = 0;
    double min_time = DBL_MAX;
    double max_time = 0;

    bool received_report = false;

    for (;;)
    {
        read_mask = mask;
        //Double timeout limit
        timeout.tv_sec = TIMEOUT_SEC * 2;
        timeout.tv_usec = TIMEOUT_USEC;

        num = select(FD_SETSIZE, &read_mask, NULL, NULL, &timeout);

        if (num > 0)
        {
            if (FD_ISSET(s, &read_mask))
            {
                recv(s, &pack, sizeof(Packet), 0);
                if (packet->type == TIMING)
                {
                    recvTiming = (TimingPacket *)packet;
                    //interarrival computation
                    if (next_num == 0)
                    {
                        // if first packet, mark start time
                        clock_gettime(CLOCK_MONOTONIC, &ts_start);
                    }
                    else if (recvTiming->seq == -1)
                    {
                        // if last packet, mark end time
                        clock_gettime(CLOCK_MONOTONIC, &ts_end);

                        /**
                         * When last packet is lost, when timeout, we can use 
                         * ts_cur as ts_end  
                         */
                    }

                    // mark time current packet
                    clock_gettime(CLOCK_MONOTONIC, &ts_curr);
                    if (next_num != recvTiming->seq)
                    {
                        sprintf(output + strlen(output), "EXPECTED SEQUENCE NUMBER %d RECEIVED %d\n", next_num, recvTiming->seq);
                        // dropped += recvTiming->seq - next_num; // TODO: shouldn't dropped be NUM_SEND - total num of received?
                        // next_num = recvTiming->seq; // TODO: if we set next_num to current seq, all the following packets will be out of order
                    }

                    // Trying and formatting new timer
                    struct timespec ts_diff;
                    ts_diff.tv_sec = ts_curr.tv_sec - ts_prev.tv_sec;
                    ts_diff.tv_nsec = ts_curr.tv_nsec - ts_prev.tv_nsec;
                    while (ts_diff.tv_nsec > 1000000000)
                    {
                        ts_diff.tv_sec++;
                        ts_diff.tv_nsec -= 1000000000;
                    }
                    while (ts_diff.tv_nsec < 0)
                    { // TODO: any reason using while loop here?
                        ts_diff.tv_sec--;
                        ts_diff.tv_nsec += 1000000000;
                    }
                    if (ts_diff.tv_sec < 0)
                    {
                        printf("NEGATIVE TIME\n");
                        exit(1);
                    }
                    double msec = ts_diff.tv_sec * 1000 + ((double)ts_diff.tv_nsec) / 1000000;
                    if (msec < min_time)
                    {
                        min_time = msec;
                    }
                    if (msec > max_time)
                    {
                        max_time = msec;
                    }
                    // Skip first few packets due to start up costs
                    if (next_num > 0)
                    {
                        avg_interarrival += msec;
                        count += 1;
                    }
                    sprintf(output + strlen(output), "%.5f ms interarrival time\n", msec);
                }
                next_num += 1;
                ts_prev = ts_curr;
            }
            else if (packet->type == REPORT)
            {
                // ignore report packet if already received one before
                if (!received_report)
                {
                    recvReport = (ReportPacket *)packet;
                }
            }
        }
        else
        {
            if (count > 0)
            {
                sprintf(output + strlen(output), "Dropped packets: %d\n", dropped);
                sprintf(output + strlen(output), "Average Interarrival time %.5f ms\n", avg_interarrival / count);
                float avg = avg_interarrival / count;
                float throughput = ((1400.0 * 8) / (1024 * 1024)) / (avg / 1000);
                sprintf(output + strlen(output), "Computed Throughput %f Mbps\n", throughput);

                //Compute with start -> end

                // Trying and formatting new timer
                struct timespec ts_diff;
                int size = sizeof(Packet);
                ts_diff.tv_sec = ts_end.tv_sec - ts_start.tv_sec;
                ts_diff.tv_nsec = ts_end.tv_nsec - ts_start.tv_nsec;
                while (ts_diff.tv_nsec > 1000000000)
                {
                    ts_diff.tv_sec++;
                    ts_diff.tv_nsec -= 1000000000;
                }
                while (ts_diff.tv_nsec < 0)
                {
                    ts_diff.tv_sec--;
                    ts_diff.tv_nsec += 1000000000;
                }

                if (ts_diff.tv_sec < 0)
                {
                    sprintf(output + strlen(output), "NEGATIVE TIME\n");
                }
                double s = ts_diff.tv_sec + ((double)ts_diff.tv_nsec) / 1000000000;
                sprintf(output + strlen(output), "Coarse bandwidth calculation %f Mbps\n", (size * 8 * (NUM_SEND - 1) / 1024.0 / 1024) / s);
            }
            // printf("Haven't heard response for over %d seconds, timeout!\n", TIMEOUT_SEC*2);
            fflush(0);
            avg_interarrival = 0;
            count = 0;
            next_num = 0;
            dropped = 0;
            min_time = DBL_MAX;
            max_time = 0;

            return output;
        }
    }
}

char *client_send_interarrival(const char *address, int port, int num_send)
{
    char *out = malloc(sizeof(char) * 300);

    // initialize starting packet and echo_packet
    char init_packet[BUFF_SIZE];
    char echo_packet[BUFF_SIZE];

    // server socket address
    struct sockaddr_in server_addr;
    // address of incoming packets
    struct sockaddr_in echo_pac_addr;
    // client socket address
    struct sockaddr_in client_addr;
    socklen_t client_len;

    // socket both for sending and receiving
    int sk = socket(AF_INET, SOCK_DGRAM, 0);
    if (sk < 0)
    {
        sprintf(out + strlen(out), "echo_client: socket error\n");
        return out;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // get server host address
    struct hostent *server_name;
    struct hostent server_name_copy;
    int server_fd;
    server_name = gethostbyname(address);
    if (server_name == NULL)
    {
        sprintf(out + strlen(out), "echo_client: invalid server address\n");
        return out;
    }

    memcpy(&server_name_copy, server_name, sizeof(server_name_copy));
    memcpy(&server_fd, server_name_copy.h_addr_list[0], sizeof(server_fd));

    // send echo_pac_addr to be server address
    echo_pac_addr.sin_family = AF_INET;
    echo_pac_addr.sin_addr.s_addr = server_fd;
    echo_pac_addr.sin_port = htons(port);

    send_timing_packets(sk, echo_pac_addr, num_send);

    char *report = client_receive_timing_packets(sk, echo_pac_addr, 100);
    free(out);
    return report;
}
