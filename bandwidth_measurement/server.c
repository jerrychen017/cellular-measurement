#include "net_include.h"
#include "utils.h"

int setup_server_socket(int port);
void receive(int s);

int main(int argc, char **argv)
{

    // argc error checking
    if (argc != 2)
    {
        printf("Usage: echo_server <port>\n");
        exit(0);
    }

    int port = atoi(argv[1]);

    // create a socket both for sending and receiving
    int s = setup_server_socket(port);
    receive(s);

    return 0;
}

void receive(int s)
{
    struct sockaddr_in from_addr;
    socklen_t from_len;


    fd_set mask;
    fd_set read_mask;
    struct timeval timeout;
    int num;
    int len;
    int ind = 0;
    int nonBurstPkt = 0; // try and recover after measuring burst packets, accure at least another burst size of packets at original rate
    double calculated_speed;

    data_packet data_pkt;
    data_packet report_pkt;

    // calculate average for burst rather than EMWA
    // use T_cur = alpha * T_new + (1 - alpha) * T_cur
    double rate = 1.0;
    double alpha = 0.6; // closer to 0 is smoother, closer to 1 is quicker reaction (90 packets ~ 1Mbps) 0.2/0.1 for regular
    

    // state variables
    // one for burst packets and one for regular packet arrivals
    struct timeval burstArrivals[BURST_SIZE];
    struct timeval arrivals[BURST_SIZE];
    struct timeval tm_diff;

    FD_ZERO(&mask);
    FD_SET(s, &mask);

    for (;;) {
        read_mask = mask;
        timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = TIMEOUT_USEC;

        num = select(FD_SETSIZE, &read_mask, NULL, NULL, &timeout);
  
        if (num > 0) {
            if (FD_ISSET(s, &read_mask)) {
                len = recvfrom(s, &data_pkt, sizeof(data_packet), 0,
                        (struct sockaddr *) &from_addr, &from_len);


                printf("received %d bytes, seq %d at rate %f\n", len, data_pkt.hdr.seq_num, data_pkt.hdr.rate );
                int seq = data_pkt.hdr.seq_num;
                if (data_pkt.hdr.isBurst){
                    gettimeofday(&burstArrivals[ind], NULL);
                    nonBurstPkt = 0;
                    ind++;
                }
                else{
                    gettimeofday(&arrivals[seq % BURST_SIZE], NULL);
                    nonBurstPkt++;
                }
                if(ind >= BURST_SIZE){
                    tm_diff = diffTime(burstArrivals[ind - 1], burstArrivals[0]);
                    calculated_speed = interval_to_speed(tm_diff, BURST_SIZE);
                    printf("Burst calculated speed of %.4f Mbps\n", calculated_speed);
                    report_pkt.hdr.type = NETWORK_REPORT;
                    report_pkt.hdr.rate = calculated_speed;
                    sendto(s, &report_pkt, sizeof(data_packet), 0,
                        (struct sockaddr *) &from_addr, from_len);
                    ind = 0;
                }
                if(nonBurstPkt >= BURST_SIZE){
                    tm_diff = diffTime(arrivals[seq % BURST_SIZE], arrivals[(seq + 1) % BURST_SIZE]);   
                    calculated_speed = interval_to_speed(tm_diff, BURST_SIZE - 1);
                    rate = (alpha * calculated_speed) + (1 - alpha) * rate;
                    printf("calculated speed of %.4f Mbps\n", calculated_speed);
                }
            }
        } else {
            printf(".");
            fflush(0);
        }
    }
}

int setup_server_socket(int port)
{
    struct sockaddr_in name;

    int s = socket(AF_INET, SOCK_DGRAM, 0);  /* socket for receiving (udp) */
    if (s < 0) {
        perror("socket error\n");
        exit(1);
    }

    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(port);

    if (bind( s, (struct sockaddr *)&name, sizeof(name) ) < 0 ) {
        perror("bind error\n");
        exit(1);
    }

    return s;
}
