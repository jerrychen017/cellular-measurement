#include "receive_bandwidth.h"
#include "send_bandwidth.h"
#include "net_utils.h"
#include "sendto_dbg.h"

/**
 * Bidirectional server main function
 */
int main(int argc, char **argv)
{
    // argc error checking
    if (argc != 2)
    {
        printf("Usage: server <EWMA/RunningAvg>\n");
        exit(0);
    }
    // get separate ports for bandwidth and interactive applications
    int portBandwidth = SERVER_RECEIVE_PORT;
    // toggle Prediction mode 0 for EMWA and 1 for RunningAvg
    int predMode = atoi(argv[1]);  // TODO: setup this on the client side

    // Bandwidth application socket
    int s_bw = setup_bound_socket(portBandwidth);

    // Select loop
    fd_set mask;
    fd_set read_mask;
    struct timeval timeout;
    int num;
    int len;
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    FD_SET(s_bw, &mask);

    data_packet data_pkt;
    packet_header ack_pkt;
    pthread_t tid; // thread id 
    struct send_bandwidth_args send_args; // arguments to be passed to send_bandwidth

    for (;;) {
        read_mask = mask;
        timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = TIMEOUT_USEC;

        num = select(FD_SETSIZE, &read_mask, NULL, NULL, &timeout);

        if (num > 0) {
            if (FD_ISSET(s_bw, &read_mask)) {
                len = recvfrom(s_bw, &data_pkt, sizeof(data_packet), 0,
                               (struct sockaddr *) &from_addr, &from_len);
                if (len < 0) {
                    perror("socket error");
                    exit(1);
                }

                if (data_pkt.hdr.type == NETWORK_START) {
                    // TODO: get parameters from recv packet
                    ack_pkt.type = NETWORK_START_ACK;
                    ack_pkt.rate = 0;
                    ack_pkt.seq_num = 0;

                    sendto_dbg(s_bw, &ack_pkt, sizeof(packet_header), 0,
                            (struct sockaddr *) &from_addr, from_len);
                    
                    send_args.addr = from_addr;
                    send_args.port = SERVER_SEND_PORT;
                    pthread_create(&tid, NULL, &send_bandwidth, (void *)&send_args);
                    receive_bandwidth(s_bw, predMode);
                    // TODO: stop thread
                 }
            }

        } else {
            printf(".");
            fflush(0);
        }
    }






  

    // TODO: send send_bandwidth

    return 0;
}
