#include "controller.h"
#include "feedbackLogger.h"
#include "net_utils.h"

static bool kill_thread = false;

void control(int s_server, int s_data, struct sockaddr_in send_addr, struct sockaddr_un data_addr, socklen_t data_len, struct parameters params);

double adjustRate(double current, double reported, double min, double max, bool burst);
void handleStartPacket(int s, struct sockaddr_in from, struct sockaddr_in expected);

int start_controller(bool android, struct sockaddr_in send_addr, int s_server, struct parameters params)
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

    control(s_server, s_data, send_addr, datagen_addr, datagen_len, params);
    close(s_data);
    close(s_server);
    return 0;
}


/* Main event loop */
void control(int s_server, int s_data, struct sockaddr_in send_addr, struct sockaddr_un data_addr, socklen_t data_len, struct parameters params)
{
    // parameter variables
    int BURST_SIZE = params.burst_size;
    int INTERVAL_SIZE = params.interval_size;
    double INTERVAL_TIME = params.interval_time;
    int INSTANT_BURST = params.instant_burst;
    int BURST_FACTOR = params.burst_factor;
    double MIN_SPEED = params.min_speed;
    double MAX_SPEED = params.max_speed;
    double START_SPEED = params.start_speed;
    int GRACE_PERIOD = params.grace_period;

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
    memset(&data_pkt, 0, sizeof(data_packet));
    memset(&recv_pkt, 0, sizeof(data_packet));

    typed_packet control_pkt;
    memset(&control_pkt, 0, sizeof(typed_packet));

    // Variables to deal with burst
    int burst_seq_recv = -1; // index of burst we have received from data
    int burst_seq_send = -1; // index of burst we have sent
    struct timeval tmNow;
    struct timeval tmPrev;

    data_packet pkt_buffer[BURST_SIZE];

    int seq = 0;
    int last_burst = 0;
    double rate = START_SPEED;

    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = TIMEOUT_USEC;
    expectedTimeout.tv_sec = 0;
    expectedTimeout.tv_usec = 0;

    for (;;)
    {
        if (kill_thread)
        {
            packet_header signal_pkt;
            memset(&signal_pkt, 0, sizeof(packet_header));

            signal_pkt.type = NETWORK_STOP;
            sendto(s_server, &signal_pkt, sizeof(packet_header), 0,
                   (struct sockaddr *)&server_addr, server_addr_len);
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
                    return;
                }
                if (len != sizeof(data_pkt.data))
                {
                    printf("ipc error, size not correct");
                    exit(1);
                }

                // Start burst
                if (/*INTERVAL_TIME */ 0 != 0)
                {
                    if (seq - last_burst >= INTERVAL_SIZE /*calculate*/)
                    {
                        burst_seq_recv = 0;
                        last_burst = seq;
                    }
                }
                else
                {
                    if (seq - last_burst >= INTERVAL_SIZE)
                    {
                        // printf("starting burst at seq %d\n", seq);
                        burst_seq_recv = 0;
                        last_burst = seq;
                    }
                }

                data_pkt.hdr.type = burst_seq_recv == -1 ? NETWORK_DATA : NETWORK_BURST;
                data_pkt.hdr.seq_num = seq;
                data_pkt.hdr.rate = burst_seq_recv == -1 ? rate : rate * BURST_FACTOR;
                data_pkt.hdr.burst_start = burst_seq_recv == -1 ? 0 : seq - burst_seq_recv;
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
                    if (!INSTANT_BURST && burst_seq_recv == BURST_SIZE / 2)
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

                        if (INSTANT_BURST) {
                            burst_seq_send = 0;
                        }

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


                if (seq % INTERVAL_SIZE == INTERVAL_SIZE / 2) {
                    sendFeedbackUpload(rate);
                }

                seq++;
            }
            if (FD_ISSET(s_server, &read_mask))
            {
                int len = recvfrom(s_server, &recv_pkt, sizeof(data_packet), 0, (struct sockaddr *)&server_addr, &server_addr_len);
                if (len <= 0)
                {
                    printf("data stream ended, exiting...\n");
                    return;
                }

                if (data_pkt.hdr.type == NETWORK_START)
                {
                    handleStartPacket(s_server, server_addr, send_addr);
                    continue;
                }

                if (data_pkt.hdr.type == NETWORK_STOP)
                {
                    return;
                }

                // Check if we have a decrepancy in our sending rates at sender and receiver
                if (recv_pkt.hdr.type == NETWORK_REPORT || recv_pkt.hdr.type == NETWORK_BURST_REPORT)
                {
                    double reportedRate = recv_pkt.hdr.rate;
                    bool burst = recv_pkt.hdr.type == NETWORK_BURST_REPORT;
                    // Adjust rate
                    rate = adjustRate(rate, reportedRate, MIN_SPEED, MAX_SPEED, burst);

                    control_pkt.rate = rate;
                    control_pkt.type = LOCAL_CONTROL;

                    sendto(s_data, &control_pkt, sizeof(control_pkt), 0, (struct sockaddr *)&data_addr, data_len);
                    sendFeedbackUpload(rate);
                    printf("Adjusted rate to %.4f\n", rate);
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


/* Given feeback from server, adjust to rate */
double adjustRate(double current, double reported, double min, double max, bool burst)
{
    double newRate = current;

    // feedback on burst
    if (burst)
    {
        if (0.9 * reported >= current + 1)
        {
            newRate = current + 1;
        }
        else
        {
            newRate = 0.9 * reported;
        }
    }
    // Non burst feedback
    else if (reported < current)
    {
        newRate = reported;
    }
    newRate = newRate >= max ? max : newRate;
    newRate = newRate <= min ? min : newRate;
    return newRate;
}

/* Reply to extraneous NETWORK_START packet */
void handleStartPacket(int s, struct sockaddr_in from, struct sockaddr_in expected)
{
    packet_header ack_pkt;
    memset(&ack_pkt, 0, sizeof(packet_header));

    if (from.sin_addr.s_addr == expected.sin_addr.s_addr && from.sin_port == expected.sin_port)
    {
        ack_pkt.type = NETWORK_START_ACK;
    }
    else
    {
        ack_pkt.type = NETWORK_BUSY;
    }

    sendto(s, &ack_pkt, sizeof(packet_header), 0,
            (struct sockaddr *)&from, sizeof(from));
}

void stop_controller_thread()
{
    kill_thread = true;
}
