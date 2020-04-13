#include "net_include.h"
#include "bandwidth_utils.h"
#include "sendto_dbg.h"

#define ALPHA 0.1  // closer to 0 is smoother, closer to 1 is quicker reaction (90 packets ~ 1Mbps) 0.2/0.1 for regular
#define THRESHOLD 0.95 // percent drop threshold

int setup_server_socket(int port);
void receive(int s, int predMode);

int main(int argc, char **argv)
{

    // argc error checking
    if (argc != 3)
    {
        printf("Usage: echo_server <port> <EWMA/RunningAvg> \n");
        exit(0);
    }
    sendto_dbg_init(0);
    int port = atoi(argv[1]);
    // toggle Prediction mode 0 for EMWA and 1 for RunningAvg
    int predMode = atoi(argv[2]);

    // create a socket both for sending and receiving
    int s = setup_server_socket(port);
    receive(s, predMode);

    return 0;
}


void receive(int s, int predMode)
{
    struct sockaddr_in from_addr;
    socklen_t from_len;

    // Select loop
    fd_set mask;
    fd_set read_mask;
    struct timeval timeout;
    int num;
    int len;

    // state variables
    // Current rate the server is receiving at, caclulated according to predMode
    double rate = START_SPEED;

    // next expected seq number
    int seq = 0;
    // Arrival time of last BURST_SIZE packets
    struct timeval arrivals[BURST_SIZE];
    // whether we have received a packet in window
    int received[BURST_SIZE];

    // analogous to above for bursts
    int burstSeq = 0;
    // first burst packet received
    int bFirst = 0;
    struct timeval barrivals[BURST_SIZE];
    int breceived[BURST_SIZE];

    memset(received, 0, sizeof(received));
    memset(breceived, 0, sizeof(breceived));
    

    // variables to store caclulations in
    double calculated_speed;
    struct timeval tm_diff;

    // packet buffers
    data_packet data_pkt;
    packet_header report_pkt;
    pacekt_header ack_pkt;


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
                if (len < 0) {
                    perror("socket error");
                    exit(1);
                }
                printf("received %d bytes, seq %d at rate %f\n", len, data_pkt.hdr.seq_num, data_pkt.hdr.rate );

                // When we receive a new START message, reset the server
                if (data_pkt.hdr.type == NETWORK_START) {
                    int seq = 0;
                    int burstSeq = 0;
                    int bFirst = 0;

                    memset(received, 0, sizeof(received));
                    memset(breceived, 0, sizeof(breceived));

                    ack_pkt.type = NETWORK_START_ACK;
                    ack_pkt.rate = 0;
                    ack_pkt.seq_num = 0;

                    sendto_dbg(s, &ack_pkt, sizeof(ack_pkt), 0,
                            (struct sockaddr *) &from_addr, from_len);
                }


                double expectedRate = data_pkt.hdr.rate;
                int currSeq = data_pkt.hdr.seq_num;

                // out of order packet, drop
                if (currSeq < seq) {
                    continue;
                }


                // mark packets up to received seq as not received
                for (; seq < currSeq; seq++) {
                    received[seq % BURST_SIZE] = 0;
                }
                // mark current pacekt as received and record arrival time
                int i = currSeq % BURST_SIZE;
                received[i] = 1;
                gettimeofday(&arrivals[i], NULL);
                
                if (data_pkt.hdr.type == NETWORK_BURST){
                    // first burst packet we have received
                    if (burstSeq == 0) {
                        memset(breceived, 0, sizeof(breceived));
                    }

                    // Calculate index within burst
                    int bursti = (currSeq % INTERVAL_SIZE) - (INTERVAL_SIZE - BURST_SIZE);

                    if (bursti >= burstSeq) {
                        if (burstSeq == 0) bFirst = bursti;
                        gettimeofday(&barrivals[bursti], NULL);
                        breceived[i] = 1;
                        burstSeq = bursti + 1;
                    }
                }
                else {
                    // burst just finished, send report
                    if (burstSeq != 0) {
                        tm_diff = diffTime(barrivals[burstSeq - 1], barrivals[bFirst]);
                        if (burstSeq - 1 != bFirst) {
                            calculated_speed = interval_to_speed(tm_diff, (burstSeq - 1) - bFirst);
                            printf("Burst calculated speed of %.4f Mbps\n", calculated_speed);
                            rate = calculated_speed;
                            report_pkt.type = NETWORK_REPORT;
                            report_pkt.rate = rate;
                            report_pkt.seq_num = 0;

                            sendto_dbg(s, &report_pkt, sizeof(report_pkt), 0,
                                       (struct sockaddr *) &from_addr, from_len);
                            burstSeq = 0;
                        }
                    }


                    // EWMA
                    if (predMode == 0 && (currSeq % INTERVAL_SIZE) >= 2) {
                        if (!received[(currSeq - 1) % BURST_SIZE]) continue;

                        tm_diff = diffTime(arrivals[currSeq % BURST_SIZE], arrivals[(currSeq - 1) % BURST_SIZE]);   
                        calculated_speed = interval_to_speed(tm_diff, 1);
                        rate = (ALPHA * calculated_speed) + (1 - ALPHA) * rate;   
                        printf("Computed sending rate of %.4f Mbps\n", rate);

                    }
                    // Running Avg
                    if (predMode == 1 && (currSeq % INTERVAL_SIZE) >= BURST_SIZE) {
                        if (!received[(currSeq + 1) % BURST_SIZE]) continue;

                        // Wrap around average of packets
                        tm_diff = diffTime(arrivals[currSeq % BURST_SIZE], arrivals[(currSeq + 1) % BURST_SIZE]);   
                        calculated_speed = interval_to_speed(tm_diff, BURST_SIZE - 1);
                        rate = calculated_speed; //Figure out threshold
                        printf("Computed sending rate of %.4f Mbps\n", rate);
                    }

                    // Send report packet if we are under 90 percent of expected rate
                    if (rate <= THRESHOLD*expectedRate) {
                        report_pkt.hdr.type = NETWORK_REPORT;
                        report_pkt.hdr.rate = rate;
                        report_pkt.hdr.seq_num = 0;
                        sendto_dbg(s, &report_pkt, sizeof(report_pkt.hdr), 0,
                            (struct sockaddr *) &from_addr, from_len);
                        printf("Computed rate %.4f below threshold, actual rate %.4f\n", rate, expectedRate);
                    }
                }
               
                // increment seq
                seq = currSeq + 1;
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
