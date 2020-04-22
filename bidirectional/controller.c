#include "controller.h"
#include "feedbackLogger.h"
#include "net_utils.h"

void startup(int s_server, struct sockaddr_in send_addr);
void control(int s_server, int s_data, struct sockaddr_in send_addr, struct sockaddr_un data_addr, socklen_t data_len);

static bool kill_thread = false;
int start_controller(bool android, struct sockaddr_in send_addr, int s_server)
{
    kill_thread = false;

    // setup unix socket
    socklen_t my_len, datagen_len;
    struct sockaddr_un my_addr = get_controller_addr(android, &my_len);
    struct sockaddr_un datagen_addr = get_datagen_addr(android, &datagen_len);
    int s_data = setup_unix_socket(my_addr, my_len);

    // connect with data generator
    fd_set mask;
    fd_set read_mask;
    int num;
    struct timeval timeout;

    FD_ZERO(&mask);
    FD_SET(s_data, &mask);
    for (;;)
    {
        printf("Connecting for data_generator\n");

        typed_packet pkt;
        pkt.type = LOCAL_START;
        sendto(s_data, &pkt, sizeof(pkt.type), 0, (struct sockaddr *)&datagen_addr, datagen_len);

        timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = TIMEOUT_USEC;
        read_mask = mask;

        num = select(FD_SETSIZE, &read_mask, NULL, NULL, &timeout);
        if (num > 0)
        {
            recv(s_data, &pkt, sizeof(pkt.type), 0);
            break;
        }
    }

    control(s_server, s_data, send_addr, datagen_addr, datagen_len);
    return 0;
}

/* Main event loop */
void control(int s_server, int s_data, struct sockaddr_in send_addr, struct sockaddr_un data_addr, socklen_t data_len)
{
    static char feedbackBuf[256];

    struct sockaddr_in server_addr;
    socklen_t server_addr_len = sizeof(server_addr);

    fd_set mask;
    fd_set read_mask;
    int num;

    FD_ZERO(&mask);
    FD_SET(s_server, &mask);
    FD_SET(s_data, &mask);

    struct timeval timeout;
    struct timeval expectedTimeout;

    data_packet data_pkt;
    data_packet recv_pkt;
    packet_header ack_pkt;

    typed_packet control_pkt;
    memset(&control_pkt, 0, sizeof(typed_packet));

    // Variables to deal with burst
    int burst_seq_recv = -1; // index of burst we have received from data
    int burst_seq_send = -1; // index of burst we have sent
    struct timeval tmNow;
    struct timeval tmPrev;

    data_packet pkt_buffer[BURST_SIZE];

    int seq = 0;
    double rate = START_SPEED;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = TIMEOUT_USEC;
    expectedTimeout.tv_sec = 0;
    expectedTimeout.tv_usec = 0;

    for (;;)
    {
        if (kill_thread)
        {
            close(s_data);
            close(s_server);
            return;
        }
        read_mask = mask;
        // printf("TIMEOUT %.4f\n", timeout.tv_usec / 1000.0);
        num = select(FD_SETSIZE, &read_mask, NULL, NULL, &timeout);

        if (num > 0)
        {
            if (FD_ISSET(s_data, &read_mask))
            {
                //printf("receive data from datagen\n");
                int len = recv(s_data, data_pkt.data, sizeof(data_pkt.data), 0);
                if (len == 0)
                {
                    printf("data stream ended, exiting...\n");
                    close(s_data);
                    close(s_server);
                    return;
                }
                if (len != sizeof(data_pkt.data))
                {
                    printf("ipc error, size not correct");
                    exit(1);
                }

                // Start burst
                if (seq % INTERVAL_SIZE == INTERVAL_SIZE - BURST_SIZE)
                {
                    // printf("starting burst at seq %d\n", seq);
                    burst_seq_recv = 0;
                }

                data_pkt.hdr.type = burst_seq_recv == -1 ? NETWORK_DATA : NETWORK_BURST;
                data_pkt.hdr.seq_num = seq;
                data_pkt.hdr.rate = burst_seq_recv == -1 ? rate : rate * BURST_FACTOR;

                if (burst_seq_recv == -1)
                {
                    if (burst_seq_send != -1)
                    {
                        printf("Error: burst not done, on seq %d\n", burst_seq_send);
                        exit(1);
                    }

                    // Pass to server during normal operation
                    sendto_dbg(s_server, &data_pkt, sizeof(data_pkt), 0,
                               (struct sockaddr *)&send_addr, sizeof(send_addr));
                    //                    printf("send data to server\n");
                }
                else
                {
                    // printf("storing packet seq %d, in spot %d\n", seq, burst_seq_recv);
                    // printf("TIMEOUT %.4f EXEPECTEDTIMEOUT %.4f\n", timeout.tv_usec / 1000.0, expectedTimeout.tv_usec / 1000.0);
                    pkt_buffer[burst_seq_recv] = data_pkt;

                    // After receiving half of the packets, we can start sending
                    if (burst_seq_recv == BURST_SIZE / 2)
                    {
                        burst_seq_send = 0;

                        sendto_dbg(s_server, &pkt_buffer[burst_seq_send], sizeof(data_pkt), 0,
                                   (struct sockaddr *)&send_addr, sizeof(send_addr));
                        burst_seq_send++;

                        gettimeofday(&tmPrev, NULL);
                        expectedTimeout = speed_to_interval(rate * BURST_FACTOR);
                        timeout = expectedTimeout;
                    }

                    burst_seq_recv++;

                    // We have recieved all the packets in the burst, so we can stop storing
                    if (burst_seq_recv == BURST_SIZE)
                    {
                        burst_seq_recv = -1;
                        //temp fix for burst out of ordering/slow
                        while (burst_seq_send != -1 && burst_seq_send < BURST_SIZE)
                        {
                            // printf("sending packet at end %d of burst\n", burst_seq_send);
                            sendto_dbg(s_server, &pkt_buffer[burst_seq_send], sizeof(data_pkt), 0,
                                       (struct sockaddr *)&send_addr, sizeof(send_addr));
                            burst_seq_send++;
                        }
                        burst_seq_send = -1;
                    }
                }
                seq++;
            }
            if (FD_ISSET(s_server, &read_mask))
            {
                int len = recvfrom(s_server, &recv_pkt, sizeof(data_packet), 0, (struct sockaddr *)&server_addr, &server_addr_len);
                if (len <= 0)
                {
                    printf("data stream ended, exiting...\n");
                    close(s_data);
                    close(s_server);
                    exit(0);
                }

                if (data_pkt.hdr.type == NETWORK_START)
                {
                    if (server_addr.sin_addr.s_addr == send_addr.sin_addr.s_addr && server_addr.sin_port == send_addr.sin_port)
                    {
                        ack_pkt.type = NETWORK_START_ACK;
                    }
                    else
                    {
                        ack_pkt.type = NETWORK_BUSY;
                    }
                    ack_pkt.rate = 0;
                    ack_pkt.seq_num = 0;
                    sendto(s_server, &ack_pkt, sizeof(packet_header), 0,
                           (struct sockaddr *)&server_addr, server_addr_len);
                    continue;
                }

                // Check if we have a decrepancy in our sending rates at sender and receiver
                if (recv_pkt.hdr.type == NETWORK_REPORT || recv_pkt.hdr.type == NETWORK_BURST_REPORT)
                {
                    double reportedRate = recv_pkt.hdr.rate;
                    double newRate;

                    // printf("feedback from %d on cur seq %d\n", seq, recv_pkt.hdr.seq_num);
                    if (recv_pkt.hdr.type == NETWORK_BURST_REPORT)
                    {
                        if (reportedRate >= rate + 1) {
                            newRate = rate + 1;
                        } else {
                            newRate = 0.95 * reportedRate;
                        }
                    }
                    else
                    {
                        if (reportedRate >= rate)
                        {
                            continue;
                        }
                        // set new rate to the max of less than reported rate to flush queue
                        newRate = reportedRate - .5 * (rate - reportedRate);
                        if (newRate < 0.75 * reportedRate)
                        {
                            newRate = 0.75 * reportedRate;
                        }
                    }

                    rate = recv_pkt.hdr.rate >= MAX_SPEED ? MAX_SPEED : newRate;

                    control_pkt.rate = rate;
                    control_pkt.type = LOCAL_CONTROL;

                    sendto(s_data, &control_pkt, sizeof(control_pkt), 0, (struct sockaddr *)&data_addr, data_len);

                    sprintf(feedbackBuf, "%.4f", rate);
                    sendFeedbackBandwidth(rate);
                    printf("%.4f\n", rate);
                }
            }
        }
        else
        {
            // timeout
            if (burst_seq_send == -1)
            {
                printf("timeout while sending\n");
            }
            else
            {
                if (burst_seq_send >= burst_seq_recv && burst_seq_recv != -1)
                {
                    printf("Error: burst trying to send seq not received %d\n", burst_seq_send);
                    continue;
                    //                    exit(1);
                }

                // printf("sending packet %d of burst\n", burst_seq_send);

                sendto_dbg(s_server, &pkt_buffer[burst_seq_send], sizeof(data_pkt), 0,
                           (struct sockaddr *)&send_addr, sizeof(send_addr));
                burst_seq_send++;

                gettimeofday(&tmNow, NULL);
                struct timeval baseTimeout = speed_to_interval(rate * BURST_FACTOR);
                struct timeval tmExtra = diffTime(diffTime(tmNow, tmPrev), expectedTimeout);
                // printf("BASETIMEOUT %.4f EXTRATIME %.4f \n", baseTimeout.tv_usec / 1000.0, tmExtra.tv_usec / 1000.0);

                while (gtTime(tmExtra, baseTimeout) && burst_seq_send < BURST_SIZE && burst_seq_send < burst_seq_recv)
                {
                    // printf("sending makeup packet %d of burst\n", burst_seq_send);
                    sendto_dbg(s_server, &pkt_buffer[burst_seq_send], sizeof(data_pkt), 0,
                               (struct sockaddr *)&send_addr, sizeof(send_addr));
                    tmExtra = diffTime(tmExtra, baseTimeout);
                    burst_seq_send++;
                }

                expectedTimeout = diffTime(baseTimeout, tmExtra);
                timeout = expectedTimeout;
                tmPrev = tmNow;
                // We are done
                if (burst_seq_send == BURST_SIZE)
                {
                    burst_seq_send = -1;
                }
            }
        }

        // if we are not sending at burst, we can just send when the data
        // source gives us a packet. However, if we are sending a burst,
        // we want to timeout to send at a certain rate
        if (burst_seq_send == -1)
        {
            timeout.tv_sec = TIMEOUT_SEC;
            timeout.tv_usec = TIMEOUT_USEC;
        }
    }
}

double estimate_change(double rate)
{
    return 0;
}

void stop_controller_thread()
{
    kill_thread = true;
}
//if (reportedRate < rate)
// max (reportedRate - (rate - reportedRate), .5 * reportedRate)
