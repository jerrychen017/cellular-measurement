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

    // toggle Prediction mode 0 for EMWA and 1 for RunningAvg
    int predMode = atoi(argv[1]); // TODO: setup this on the client side

    // start handshake process. use two different port for sending and receiving
    int server_send_sk = setup_bound_socket(SERVER_SEND_PORT);
    int server_recv_sk = setup_bound_socket(SERVER_RECEIVE_PORT);

    // Select loop
    fd_set mask;
    fd_set read_mask;
    FD_ZERO(&mask);
    FD_SET(server_send_sk, &mask);
    FD_SET(server_recv_sk, &mask);
    struct timeval timeout;
    int num;
    int len;
    // client address for server receiving bandwidth
    struct sockaddr_in server_recv_addr;
    socklen_t server_recv_len;
    // client address for server sending bandwidth
    struct sockaddr_in server_send_addr;
    socklen_t server_send_len;

    data_packet data_pkt;
    packet_header ack_pkt;

    bool got_recv_addr = false;
    bool got_send_addr = false;

    for (;;)
    {
        read_mask = mask;
        timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = TIMEOUT_USEC;

        num = select(FD_SETSIZE, &read_mask, NULL, NULL, &timeout);

        if (num > 0)
        {
            if (FD_ISSET(server_send_sk, &read_mask))
            {
                len = recvfrom(server_send_sk, &data_pkt, sizeof(data_packet), 0,
                               (struct sockaddr *)&server_send_addr, &server_send_len);
                if (len < 0)
                {
                    perror("socket error");
                    exit(1);
                }
                if (data_pkt.hdr.type == NETWORK_START)
                {
                    got_send_addr = true;

                    ack_pkt.type = NETWORK_START_ACK;
                    ack_pkt.rate = 0;
                    ack_pkt.seq_num = 0;
                    sendto_dbg(server_send_sk, &ack_pkt, sizeof(packet_header), 0,
                               (struct sockaddr *)&server_send_addr, server_send_len);
                }
            }
            if (FD_ISSET(server_recv_sk, &read_mask))
            {
                len = recvfrom(server_recv_sk, &data_pkt, sizeof(data_packet), 0,
                               (struct sockaddr *)&server_recv_addr, &server_recv_len);
                if (len < 0)
                {
                    perror("socket error");
                    exit(1);
                }

                if (data_pkt.hdr.type == NETWORK_START)
                {
                    got_recv_addr = true;

                    ack_pkt.type = NETWORK_START_ACK;
                    ack_pkt.rate = 0;
                    ack_pkt.seq_num = 0;
                    sendto_dbg(server_recv_sk, &ack_pkt, sizeof(packet_header), 0,
                               (struct sockaddr *)&server_recv_addr, server_recv_len);
                }
            }
        }
        else
        {
            printf(".");
            fflush(0);
        }

        if (got_send_addr && got_send_addr)
        {
            pthread_t tid;                        // thread id
            struct send_bandwidth_args send_args; // arguments to be passed to send_bandwidth
            send_args.addr = server_send_addr;
            send_args.sk = server_send_sk;
            send_args.android = false;
            pthread_create(&tid, NULL, &send_bandwidth_pthread, (void *)&send_args);

            receive_bandwidth(server_recv_sk, predMode);
            // TODO: stop thread
            got_send_addr = false;
            got_recv_addr = false;
        }
    }

    return 0;
}
