#include "controller.h"
#include "feedbackLogger.h"
#include "net_utils.h"

#define BURST_FACTOR 2 // cannot be changed since our protocol depends on it

/**
 * static variable used for Android client to terminate threads
 */
static bool kill_thread = false;

/**
 * Controller select loop to receive data from data generator and forward data to the server.
 * Controller sends control signals to data generator and receives report packet from the server.
 */
void control(int s_server, int s_data, struct sockaddr_in send_addr, struct sockaddr_un data_addr, socklen_t data_len, struct parameters params, bool android);

/**
 * Adjust rate based on the given feedback from the server
 */
double adjustRate(double current, double reported, double min, double max, bool burst);

/**
 * Reply to extraneous NETWORK_START packet
 */
void handleStartPacket(int s, struct sockaddr_in from, struct sockaddr_in expected);

/**
 * starts controller and connects to data generator
 */
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

    control(s_server, s_data, send_addr, datagen_addr, datagen_len, params, android);
    close(s_data);
    close(s_server);
    return 0;
}

/**
 * Controller select loop to receive data from data generator and forward data to the server.
 * Controller sends control signals to data generator and receives report packet from the server.
 */
void control(int s_server, int s_data, struct sockaddr_in send_addr, struct sockaddr_un data_addr, socklen_t data_len, struct parameters params, bool android)
{
    // parameter variables
    int BURST_SIZE = params.burst_size;
    int INTERVAL_SIZE = params.interval_size;
    double INTERVAL_TIME = params.interval_time;
    int INSTANT_BURST = params.instant_burst;
    double MIN_SPEED = params.min_speed;
    double MAX_SPEED = params.max_speed;
    double START_SPEED = params.start_speed;
//    int GRACE_PERIOD = params.grace_period;

    struct sockaddr_in from_addr;
    socklen_t from_addr_len = sizeof(from_addr);

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
    struct timeval tm_last_feedback;
    gettimeofday(&tm_last_feedback, NULL);

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
                   (struct sockaddr *)&send_addr, sizeof(send_addr));
            return;
        }

        struct timeval tm_now_feedback;
        gettimeofday(&tm_now_feedback, NULL);
        struct timeval tm_diff_feedback = diffTime(tm_now_feedback, tm_last_feedback);

        if (android)
        {
            if (tm_diff_feedback.tv_sec * 1000000 + tm_diff_feedback.tv_usec > FEEDBACK_FREQ_USEC)
            {
                sendFeedbackUpload(rate);
                tm_last_feedback = tm_now_feedback;
            }
        }
        else
        {
            if (tm_diff_feedback.tv_sec * 1000000 + tm_diff_feedback.tv_usec > PRINTOUT_FREQ_USEC)
            {
                sendFeedbackUpload(rate);
                tm_last_feedback = tm_now_feedback;
                // send an ECHO packet to android
                data_pkt.hdr.type = NETWORK_ECHO;
                memcpy(data_pkt.data, &tm_now_feedback.tv_sec, sizeof(tm_now_feedback.tv_sec));
                memcpy(data_pkt.data + sizeof(tm_now_feedback.tv_sec), &tm_now_feedback.tv_usec, sizeof(tm_now_feedback.tv_usec));
                sendto(s_server, &data_pkt, sizeof(data_pkt), 0,
                           (struct sockaddr *)&send_addr, sizeof(send_addr));
            }
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
                    printf("Upload: data stream ended, exiting...\n");
                    return;
                }
                if (len != sizeof(data_pkt.data))
                {
                    printf("Upload: ipc error, size not correct");
                    exit(1);
                }

                // Start burst
                if (INTERVAL_TIME != 0)
                {

                    if (seq - last_burst >= INTERVAL_TIME * rate * 1024.0 * 1024.0 / (8.0 * PACKET_SIZE))
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
                        printf("Upload Error: burst not done, on seq %d\n", burst_seq_send);
                        exit(1);
                    }

                    // Pass to server during normal operation
                    sendto(s_server, &data_pkt, sizeof(data_pkt), 0,
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

                        sendto(s_server, &pkt_buffer[burst_seq_send], sizeof(data_pkt), 0,
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

                        if (INSTANT_BURST)
                        {
                            burst_seq_send = 0;
                        }

                        //temp fix for burst out of ordering/slow
                        while (burst_seq_send != -1 && burst_seq_send < BURST_SIZE)
                        {
                            // printf("sending packet at end %d of burst\n", burst_seq_send);
                            sendto(s_server, &pkt_buffer[burst_seq_send], sizeof(data_pkt), 0,
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
                int len = recvfrom(s_server, &recv_pkt, sizeof(data_packet), 0, (struct sockaddr *)&from_addr, &from_addr_len);
                if (len <= 0)
                {
                    printf("Upload: data stream ended, exiting...\n");
                    return;
                }

                if (data_pkt.hdr.type == NETWORK_START)
                {
                    handleStartPacket(s_server, from_addr, send_addr);
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
                    printf("Upload: Adjusted rate to %.4f\n", rate);
                }

                if (!android && recv_pkt.hdr.type == NETWORK_ECHO)
                {
                    struct timeval tm_now_latency;
                    gettimeofday(&tm_now_latency, NULL);
                    struct timeval tm_last_latency;
                    memcpy(&(tm_last_latency.tv_sec), recv_pkt.data, sizeof(tm_last_latency.tv_sec));
                    memcpy(&(tm_last_latency.tv_usec), recv_pkt.data + sizeof(tm_last_latency.tv_sec), sizeof(tm_last_latency.tv_usec));
                    struct timeval tm_diff_latency = diffTime(tm_now_latency, tm_last_latency);
                    sendFeedbackLatency(tm_diff_latency.tv_sec * 1000 + tm_diff_latency.tv_usec / 1000.0);
                }
            }
        }
        else
        {
            // timeout
            if (burst_seq_send == -1)
            {
                printf("Upload: timeout while sending\n");
            }
            else
            {
                if (burst_seq_send >= burst_seq_recv && burst_seq_recv != -1)
                {
                    printf("Upload Error: burst trying to send seq not received %d\n", burst_seq_send);
                    continue;
                    //                    exit(1);
                }

                // printf("sending packet %d of burst\n", burst_seq_send);

                sendto(s_server, &pkt_buffer[burst_seq_send], sizeof(data_pkt), 0,
                           (struct sockaddr *)&send_addr, sizeof(send_addr));
                burst_seq_send++;

                gettimeofday(&tmNow, NULL);
                struct timeval baseTimeout = speed_to_interval(rate * BURST_FACTOR);
                struct timeval tmExtra = diffTime(diffTime(tmNow, tmPrev), expectedTimeout);
                // printf("BASETIMEOUT %.4f EXTRATIME %.4f \n", baseTimeout.tv_usec / 1000.0, tmExtra.tv_usec / 1000.0);

                while (gtTime(tmExtra, baseTimeout) && burst_seq_send < BURST_SIZE && burst_seq_send < burst_seq_recv)
                {
                    // printf("sending makeup packet %d of burst\n", burst_seq_send);
                    sendto(s_server, &pkt_buffer[burst_seq_send], sizeof(data_pkt), 0,
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

/**
 * Adjust rate based on the given feedback from the server
 */
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

/**
 * Reply to extraneous NETWORK_START packet
 */
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

/**
 * Breaks out controller select loop and stops controller thread
 */
void stop_controller_thread()
{
    kill_thread = true;
}
