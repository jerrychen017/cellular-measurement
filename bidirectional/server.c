#include "receive_bandwidth.h"
#include "send_bandwidth.h"
#include "net_utils.h"
#include "sendto_dbg.h"

/**
 * Bidirectional server main function
 */
int main(int argc, char **argv)
{
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
    socklen_t server_recv_len = sizeof(server_recv_addr);
    // client address for server sending bandwidth
    struct sockaddr_in server_send_addr;
    socklen_t server_send_len = sizeof(server_send_addr);

    start_packet recv_pkt;
    packet_header ack_pkt;
    memset(&ack_pkt, 0, sizeof(packet_header));
    memset(&recv_pkt, 0, sizeof(start_packet));

    bool got_recv_addr = false;
    bool got_send_addr = false;
    
    int offset = 0; // used for parsking the recv buffer
    char buf[sizeof(start_packet)]; //Buffer for the serialized struct

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
                len = recvfrom(server_send_sk, buf, sizeof(buf), 0,
                               (struct sockaddr *)&server_send_addr, &server_send_len);
                if (len < 0)
                {
                    perror("socket error");
                    exit(1);
                }

                deserializeStruct(&recv_pkt, buf);

                if (recv_pkt.type == NETWORK_START)
                {
                    got_send_addr = true;

                    ack_pkt.type = NETWORK_START_ACK;
                    ack_pkt.rate = 0;
                    ack_pkt.seq_num = 0;
                    ack_pkt.burst_start = 0;
                    sendto_dbg(server_send_sk, &ack_pkt, sizeof(packet_header), 0,
                               (struct sockaddr *)&server_send_addr, server_send_len);
                    printf("sending send ack\n");
                }
            }
            if (FD_ISSET(server_recv_sk, &read_mask))
            {
                len = recvfrom(server_recv_sk, buf, sizeof(buf), 0,
                               (struct sockaddr *)&server_recv_addr, &server_recv_len);
                if (len < 0)
                {
                    perror("socket error");
                    exit(1);
                }

                deserializeStruct(&recv_pkt, buf);

                if (recv_pkt.type == NETWORK_START)
                {
                    got_recv_addr = true;

                    ack_pkt.type = NETWORK_START_ACK;
                    ack_pkt.rate = 0;
                    ack_pkt.seq_num = 0;
                    sendto_dbg(server_recv_sk, &ack_pkt, sizeof(packet_header), 0,
                               (struct sockaddr *)&server_recv_addr, server_recv_len);
                    printf("sending recv ack\n");
                }
            }
        }
        else
        {
            printf(".");
            fflush(0);
        }

        if (got_send_addr && got_recv_addr)
        {

            struct parameters recv_params;
            recv_params = recv_pkt.params;

            printf("burst size is %d\n", recv_params.burst_size);
            printf("interval_size is %d\n", recv_params.interval_size);
            printf("interval_time is %f\n", recv_params.interval_time);
            printf("instant_burst is %d\n", recv_params.instant_burst);
            printf("min_speed is %f\n", recv_params.min_speed);
            printf("max_speed is %f\n", recv_params.max_speed);
            printf("start_speed is %f\n", recv_params.start_speed);
            printf("grace_period is %d\n", recv_params.grace_period);
            printf("pred_mode is %d\n", recv_params.pred_mode);
            printf("use_tcp is %d\n", recv_params.use_tcp);
            printf("size of params is %d\n", sizeof(recv_params));

            pthread_t tid;                        // thread id
            struct send_bandwidth_args send_args; // arguments to be passed to send_bandwidth
            if (recv_params.use_tcp) {
                close(server_send_sk);
                close(server_recv_sk);
                server_recv_sk = setup_tcp_socket_recv(SERVER_RECEIVE_PORT);
                server_send_sk = setup_tcp_socket_recv(SERVER_SEND_PORT);


                send_args.sk = server_send_sk;
                pthread_create(&tid, NULL, &server_send_bandwidth_tcp_pthread, (void *)&send_args);

                server_receive_bandwidth_tcp(server_recv_sk, false);
            } else {
                send_args.addr = server_send_addr;
                send_args.sk = server_send_sk;
                send_args.android = false;
                send_args.params = recv_params;
                pthread_create(&tid, NULL, &send_bandwidth_pthread, (void *)&send_args);

                receive_bandwidth(server_recv_sk, server_recv_addr, recv_params, false);
                stop_controller_thread();
                stop_data_generator_thread();
                pthread_join(tid, NULL);
            }

            // re-open socket that was closed by controller process
            server_send_sk = setup_bound_socket(SERVER_SEND_PORT);
            server_recv_sk = setup_bound_socket(SERVER_RECEIVE_PORT);
            got_send_addr = false;
            got_recv_addr = false;
        }
    }

    return 0;
}
