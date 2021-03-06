#include "send_bandwidth.h"
#include "receive_bandwidth.h"
#include "net_utils.h"

/**
 * CLI start client function. Sends START packet to the server with all parameters and wait for ACKs.
 * Once connected with the server, run send_bandwidth() in a new thread and call receive_bandwidth()
 * @param address server address
 * @param params parameters
 */
void start_client(const char *address, struct parameters params, int CLIENT_SEND_PORT, int CLIENT_RECEIVE_PORT)
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

    start_packet start_pkt;
    data_packet data_pkt;
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    char buf[sizeof(start_pkt)]; //buffer to serialize struct

    bool got_send_ack = false;
    bool got_recv_ack = false;

    // initiate handshake packet and send to the server
    start_pkt.type = NETWORK_START;
    start_pkt.params = params;

    int buf_len = serializeStruct(&start_pkt, buf);

    for (;;)
    {
        read_mask = mask;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        // re-send NETWORK_START packets when timeout
        if (!got_send_ack)
        {
            sendto(client_send_sk, buf, buf_len, 0,
                   (struct sockaddr *)&client_send_addr, sizeof(client_send_addr));
        }

        if (!got_recv_ack)
        {
            sendto(client_recv_sk, buf, buf_len, 0,
                   (struct sockaddr *)&client_recv_addr, sizeof(client_recv_addr));
        }

        printf("re-sending NETWORK_START\n");

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
                    return;
                }

                if (data_pkt.hdr.type == NETWORK_START_ACK)
                {
                    printf("got send ack\n");
                    got_send_ack = true;
                    FD_CLR(client_send_sk, &mask);
                }

                if (data_pkt.hdr.type == NETWORK_BUSY)
                {
                    printf("server is busy\n");
                    close(client_send_sk);
                    close(client_recv_sk);
                    return;
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
                    FD_CLR(client_recv_sk, &mask);
                }

                if (data_pkt.hdr.type == NETWORK_BUSY)
                {
                    printf("server is busy\n");
                    close(client_recv_sk);
                    close(client_send_sk);
                    return;
                }
            }
        }
        else
        {
            printf(".");
            fflush(0);

            if (num < 0)
            {
                perror("num is negative\n");
                exit(1);
            }
        }

        if (got_recv_ack && got_send_ack)
        {
            // start a new thread to run receive_bandwidth()
            pthread_t tid; // thread id
            struct recv_bandwidth_args send_args;
            send_args.sk = client_recv_sk;
            send_args.expected_addr = client_recv_addr;
            send_args.params = params;
            send_args.android = true;
            pthread_create(&tid, NULL, &receive_bandwidth_pthread, (void *)&send_args);

            send_bandwidth(client_send_addr, client_send_sk, false, params);
            return;
        }
    }
}
