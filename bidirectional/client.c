#include "send_bandwidth.h"
#include "receive_bandwidth.h"
#include "net_utils.h"

void start_client(const char *address, int pred_mode, bool android)
{

    int client_send_sk = setup_bound_socket(CLIENT_SEND_PORT);
    int client_recv_sk = setup_bound_socket(CLIENT_RECEIVE_PORT);

    struct sockaddr_in client_send_addr = addrbyname(address, CLIENT_SEND_PORT);
    struct sockaddr_in client_recv_addr = addrbyname(address, CLIENT_RECEIVE_PORT);

    // select loop
    fd_set mask;
    fd_set read_mask;
    FD_ZERO(&mask);
    FD_SET(client_send_sk, &mask);
    FD_SET(client_recv_sk, &mask);
    struct timeval timeout;
    int num, len;

    data_packet data_pkt;
    packet_header ack_pkt;
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);

    bool got_send_ack = false;
    bool got_recv_ack = false;

    // initiate handshake process
    ack_pkt.type = NETWORK_START;
    ack_pkt.rate = 0;
    ack_pkt.seq_num = 0;
    sendto_dbg(client_send_sk, &ack_pkt, sizeof(packet_header), 0,
               (struct sockaddr *)&client_send_addr, sizeof(client_send_addr));
    sendto_dbg(client_recv_sk, &ack_pkt, sizeof(packet_header), 0,
               (struct sockaddr *)&client_recv_addr, sizeof(client_recv_addr));

    for (;;)
    {
        read_mask = mask;
        timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = TIMEOUT_USEC;

        num = select(FD_SETSIZE, &read_mask, NULL, NULL, &timeout);

        if (num > 0)
        {
            if (FD_ISSET(client_send_sk, &read_mask))
            {
                len = recvfrom(client_send_sk, &data_pkt, sizeof(data_packet), 0,
                               (struct sockaddr *)&from_addr, &from_len);
                if (len < 0)
                {
                    perror("socket error");
                    exit(1);
                }

                if (data_pkt.hdr.type == NETWORK_START_ACK)
                {
                    printf("got send ack\n");
                    got_send_ack = true;
                }
            }
            if (FD_ISSET(client_recv_sk, &read_mask))
            {
                len = recvfrom(client_recv_sk, &data_pkt, sizeof(data_packet), 0,
                               (struct sockaddr *)&from_addr, &from_len);
                if (len < 0)
                {
                    perror("socket error");
                    exit(1);
                }

                if (data_pkt.hdr.type == NETWORK_START_ACK)
                {
                    printf("got recv ack\n");
                    got_recv_ack = true;
                }
            }
        }
        else
        {
            printf(".");
            fflush(0);
            // send meaningless packet to server first in order to receive packets
            ack_pkt.type = NETWORK_START;
            ack_pkt.rate = 0;
            ack_pkt.seq_num = 0;
            sendto_dbg(client_send_sk, &ack_pkt, sizeof(packet_header), 0,
                       (struct sockaddr *)&client_send_addr, sizeof(client_send_addr));
            sendto_dbg(client_recv_sk, &ack_pkt, sizeof(packet_header), 0,
                       (struct sockaddr *)&client_recv_addr, sizeof(client_recv_addr));
        }

        if (got_recv_ack && got_send_ack)
        {
            // start a new thread to run receive_bandwidth()
            pthread_t tid; // thread id
            struct recv_bandwidth_args send_args;
            send_args.sk = client_recv_sk;
            send_args.pred_mode = pred_mode;
            send_args.expected_addr = client_recv_addr;
            pthread_create(&tid, NULL, &receive_bandwidth_pthread, (void *)&send_args);

            send_bandwidth(client_send_addr, client_send_sk, android);
            return;
        }
    }
}
