#include "include/net_include.h"
#include "utils.h"

static TimingPacket timing, *recvTiming;
static ReportPacket report, *recvReport;
static EchoPacket echo, * recvEcho;
static Packet pack, *packet = &pack;


/* Sends NUM_SEND ~MTU packets
 *
 */

void send_timing_packets(int s, struct sockaddr_in send_addr)
{ 
    int seq = 0;
    for (int i = 0; i < NUM_SEND; i++) {
        timing.type = 0;
        timing.seq = seq;
        sendto(s, &timing, sizeof(timing), 0, 
                (struct sockaddr*)&send_addr, sizeof(send_addr));
        seq++;
    }
}

void compute_bandwidth(int s, struct sockaddr_in m, int delay)
{
    fd_set mask;
    fd_set read_mask;
    int num;

    FD_ZERO(&mask);
    FD_SET(s, &mask);

    struct sockaddr_in sockaddr_client_pac;
    socklen_t sockaddr_client_pac_len;

    // Compute interarrival time on client side as well
    //Using Monotonically increasing clock
    struct timespec ts_curr;
    struct timespec ts_prev;

    struct timespec ts_start;

    struct timeval timeout;

    double avg_interarrival = 0;
    int count = 0;
    int next_num = 0;
    int dropped = 0;
    double min_time = DBL_MAX;
    double max_time = 0;

    for (;;) { 
        read_mask = mask;
        timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = TIMEOUT_USEC;

        num = select(FD_SETSIZE, &read_mask, NULL, NULL, &timeout);

        if (num > 0) {
            if (FD_ISSET(s, &read_mask)) {
                sockaddr_client_pac_len = sizeof(sockaddr_client_pac);
                recvfrom(s, &pack, sizeof(Packet), 0, (struct sockaddr*) &sockaddr_client_pac, &sockaddr_client_pac_len);
                if(packet->type == 0){ // Timing packet 
                    recvTiming = (TimingPacket *) packet;
                }
                else if (packet->type == 1) { // Report packet  
                    recvReport = (ReportPacket *) packet;
                } else { // Echo packet 
                    recvEcho = (EchoPacket *) packet;
                    echo.type = ECHO; 
                    echo.seq = recvEcho->seq;
		    echo.x = recvEcho->x;
		    echo.y = recvEcho->y;
                    sendto(s, &echo, sizeof(echo), 0, 
                        (struct sockaddr*)&sockaddr_client_pac, sizeof(sockaddr_client_pac));
                    continue; // next packet
                }

                //interarrival computation
                if(next_num == 0){
                    clock_gettime(CLOCK_MONOTONIC, &ts_start);
                }

                //interarrival computation
                clock_gettime(CLOCK_MONOTONIC, &ts_curr);
                if (next_num != recvTiming->seq){
                    printf("UNEXPECTED SEQUENCE NUMBER, EXITING...%d %d\n", next_num, recvTiming->seq);
                    dropped += recvTiming->seq - next_num;
                    next_num = recvTiming->seq;
                }
                if (next_num != 0) {
                    // Trying and formatting new timer
                    struct timespec ts_diff;
                    ts_diff.tv_sec = ts_curr.tv_sec - ts_prev.tv_sec;
                    ts_diff.tv_nsec = ts_curr.tv_nsec - ts_prev.tv_nsec;
                    while (ts_diff.tv_nsec > 1000000000) {
                            ts_diff.tv_sec++;
                            ts_diff.tv_nsec -= 1000000000;
                    }
                    while (ts_diff.tv_nsec < 0) {
                            ts_diff.tv_sec--;
                            ts_diff.tv_nsec += 1000000000;
                    }

                    if (ts_diff.tv_sec < 0) {
                        printf("NEGATIVE TIME\n");
                        exit(1);
                    }
                    double msec = ts_diff.tv_sec * 1000 + ((double) ts_diff.tv_nsec) / 1000000;
                    if(msec < min_time){
                        min_time = msec;
                    }
                    if(msec > max_time){
                        max_time = msec;
                    }

                    // Skip first few packets due to start up costs
                    if(next_num > 0){
                        avg_interarrival += msec;
                        count += 1;
                    }
                    printf("%.5f ms interarrival time\n", msec);
                }
                next_num += 1;
                ts_prev = ts_curr;
            }
        } else {
            // wait the timer out
            if(count > 0){
                printf("Dropped packets: %d\n", dropped);
                printf("Min Interarrival time %.5f ms\n", min_time);
                printf("Average Interarrival time %.5f ms\n", avg_interarrival/count);
                printf("Max Interarrival time %.5f ms\n", max_time);
                float avg = avg_interarrival/count;
                float throughput = ((1400.0*8)/(1024*1024))/(avg/1000);
                printf("Computed Throughput %f Mbps\n", throughput);

                // Trying and formatting new timer
                struct timespec ts_diff;
                int size = sizeof(Packet);
                ts_diff.tv_sec = ts_prev.tv_sec - ts_start.tv_sec;
                ts_diff.tv_nsec = ts_prev.tv_nsec - ts_start.tv_nsec;
                while (ts_diff.tv_nsec > 1000000000) {
                        ts_diff.tv_sec++;
                        ts_diff.tv_nsec -= 1000000000;
                }
                while (ts_diff.tv_nsec < 0) {
                        ts_diff.tv_sec--;
                        ts_diff.tv_nsec += 1000000000;
                }

                if (ts_diff.tv_sec < 0) {
                    printf("NEGATIVE TIME\n");
                    exit(1);
                }
                double sec = ts_diff.tv_sec + ((double) ts_diff.tv_nsec) / 1000000000;
                printf("Coarse bandwidth calculation %f Mbps\n", (size*8*(NUM_SEND-1)/1024.0/1024)/sec);
                send_timing_packets(s, sockaddr_client_pac);
            }
            else{
                printf(".");
            }
            fflush(0);
            avg_interarrival = 0;
            count = 0;
            next_num = 0;
            dropped = 0;
            min_time = DBL_MAX;
            max_time = 0;
            // return;
        }
    }

}

int main(int argc, char** argv) {

    // argc error checking
    if (argc != 2) {
        printf("Usage: echo_server <port>\n");
        exit(0);
    }

    int port = atoi(argv[1]);

    // create a socket both for sending and receiving
    int sk = socket(AF_INET, SOCK_DGRAM, 0);
    if (sk < 0) {
        perror("echo_server: cannot create a socket for sending and receiving\n");
        exit(1);
    }

    struct sockaddr_in sockaddr_server;
    sockaddr_server.sin_family = AF_INET;
    sockaddr_server.sin_addr.s_addr = INADDR_ANY;
    sockaddr_server.sin_port = htons(port);

    // bind the socket with PORT
    if (bind(sk, (struct sockaddr*)&sockaddr_server, sizeof(sockaddr_server)) < 0) {
        perror("echo_server: cannot bind socket with server address");
        exit(1);
    }

    fd_set read_mask;
    fd_set mask;
    FD_ZERO(&mask);
    FD_SET(sk, &mask);


    // socket address of received packet from client
    struct sockaddr_in sockaddr_client_pac;
    socklen_t sockaddr_client_pac_len;
    // socket address of current client
    struct sockaddr_in sockaddr_client;

    // packet received from client
    char init_packet[BUFF_SIZE];
    // packet to be sent from server
    char echo_packet[BUFF_SIZE];

    // record starting time of transfer   
    struct timeval start_time;
    struct timeval last_time;

    // wait interval for connection requests
    struct timeval idle_interval;

    compute_bandwidth(sk, sockaddr_client_pac, 100);

    return 0; 
}
