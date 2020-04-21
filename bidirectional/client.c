#include "send_bandwidth.h"
#include "receive_bandwidth.h"
#include "net_utils.h"
/**
    CLI Client 
*/
int main(int argc, char *argv[])
{
    // args error checking
    if (argc != 3)
    {
        printf("client usage: client <server_address> <EWMA/RunningAvg>\n");
        exit(1);
    }
    // address
    char *address = argv[1];
    // prediction mode 
    int pred_mode = atoi(argv[2]); 
    pthread_t tid; // thread id
    struct sockaddr_in send_addr = addrbyname(address, CLIENT_SEND_PORT);
    // arguments to be passed in the new thread
    struct send_bandwidth_args send_args; 
    send_args.addr = send_addr; 
    send_args.port = CLIENT_SEND_PORT;
    pthread_create(&tid, NULL, &send_bandwidth, (void *)&send_args);

    // bind socket
    int sk = setup_bound_socket(CLIENT_RECEIVE_PORT);



    fd_set mask;
    fd_set read_mask;
    struct timeval timeout;
    
    FD_ZERO(&mask);
    FD_SET(sk, &mask);
    int num, len; 
    data_packet data_pkt;
    packet_header ack_pkt;
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);

    // send meaningless packet to server first in order to receive packets
    ack_pkt.type = NETWORK_START;
    ack_pkt.rate = 0;
    ack_pkt.seq_num = 0;
    send_addr.sin_port = htons(CLIENT_RECEIVE_PORT);
    sendto_dbg(sk, &ack_pkt, sizeof(packet_header), 0,
            (struct sockaddr *) &send_addr, sizeof(send_addr));
    sendto_dbg(sk, &ack_pkt, sizeof(packet_header), 0,
            (struct sockaddr *) &send_addr, sizeof(send_addr));
    sendto_dbg(sk, &ack_pkt, sizeof(packet_header), 0,
            (struct sockaddr *) &send_addr, sizeof(send_addr));
    sendto_dbg(sk, &ack_pkt, sizeof(packet_header), 0,
            (struct sockaddr *) &send_addr, sizeof(send_addr));

    for (;;) {
        read_mask = mask;
        timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = TIMEOUT_USEC;

        num = select(FD_SETSIZE, &read_mask, NULL, NULL, &timeout);

        if (num > 0) {
            if (FD_ISSET(sk, &read_mask)) {
                len = recvfrom(sk, &data_pkt, sizeof(data_packet), 0,
                               (struct sockaddr *) &from_addr, &from_len);
                if (len < 0) {
                    perror("socket error");
                    exit(1);
                }
                printf("received a packet\n");
                if (data_pkt.hdr.type == NETWORK_START) {
                    // TODO: get parameters from recv packet
                    ack_pkt.type = NETWORK_START_ACK;
                    ack_pkt.rate = 0;
                    ack_pkt.seq_num = 0;

                    sendto_dbg(sk, &ack_pkt, sizeof(packet_header), 0,
                            (struct sockaddr *) &from_addr, from_len);
            
                    receive_bandwidth(sk, pred_mode);
                 }
            }

        } else {
            // TODO: BIG CONFUSION!!
            printf(".");

            fflush(0);
        }
    }
    
    return 0;
}
