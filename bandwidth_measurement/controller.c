#include "controller.h"
#include "feedbackLogger.h"
static bool kill_thread = false;
int start_controller(const char* address, int port, bool android)
{
    kill_thread = false;
    int s_server = setup_server_socket(port, android);


    struct sockaddr_in send_addr = addrbyname(address, port);

    startup(s_server, send_addr);

    int s_data = setup_data_socket(android);
    typed_packet pkt;
    pkt.type = LOCAL_START;
    send(s_data, &pkt, sizeof(pkt.type), 0);

    control(s_server, s_data, send_addr);

    return 0;
}

/* 
 * First pings server to reset it and wait for ack
 */
void startup(int s_server, struct sockaddr_in send_addr)
{
    fd_set mask;
    fd_set read_mask;
    int num;
    struct timeval timeout;

    packet_header server_pkt;

    FD_ZERO(&mask);
    FD_SET(s_server, &mask);
    while (1) {
        // create NETWORK_START packet
        server_pkt.type = NETWORK_START;
        server_pkt.seq_num = 0;
        server_pkt.rate = 0;

        printf("Connecting to server...\n");
        sendto(s_server, &server_pkt, sizeof(packet_header), 0, 
                (struct sockaddr *) &send_addr, sizeof(send_addr));

        // reset select loop
        read_mask = mask;
        timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = TIMEOUT_USEC;

        num = select(FD_SETSIZE, &read_mask, NULL, NULL, &timeout);
        
        if (num > 0) {
            int len = recv(s_server, &server_pkt, sizeof(server_pkt), 0); 
            if (len < 0) {
                perror("socket error\n");
                exit(1);
            }

            if (server_pkt.type == NETWORK_START_ACK) {
                printf("Controller: connected to server!\n");
                break;
            }
        }
        else {
            printf("timeout, trying again\n");
        }
    }

}


/* Main event loop */
void control(int s_server, int s_data, struct sockaddr_in send_addr)
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


    for (;;) {
        if (kill_thread) {
            close(s_data);
            close(s_server);
            return;
        }
        read_mask = mask;
        // printf("TIMEOUT %.4f\n", timeout.tv_usec / 1000.0);
        num = select(FD_SETSIZE, &read_mask, NULL, NULL, &timeout);

        if (num > 0) {
            if (FD_ISSET(s_data, &read_mask)) {
                int len = recv(s_data, data_pkt.data, sizeof(data_pkt.data), 0); 
                if (len == 0) {
                    printf("data stream ended, exiting...\n");
                    close(s_data);
                    close(s_server);
                    return;
                }
                if (len != sizeof(data_pkt.data)) {
                    printf("ipc error, size not correct");
                    exit(1);
                }

                // Start burst
                if (seq % INTERVAL_SIZE == INTERVAL_SIZE - BURST_SIZE) {
                    printf("starting burst at seq %d\n", seq);
                    burst_seq_recv = 0; 
                }

                data_pkt.hdr.type = burst_seq_recv == -1 ? NETWORK_DATA : NETWORK_BURST;
                data_pkt.hdr.seq_num = seq;
                data_pkt.hdr.rate = burst_seq_recv == -1 ? rate : rate * BURST_FACTOR;


                if (burst_seq_recv == -1) {
                    if (burst_seq_send != -1) {
                        printf("Error: burst not done, on seq %d\n", burst_seq_send);
                        exit(1);
                    }

                    // Pass to server during normal operation
                    sendto_dbg(s_server, &data_pkt, sizeof(data_pkt), 0,
                            (struct sockaddr *) &send_addr, sizeof(send_addr));
                } else {
                    printf("storing packet seq %d, in spot %d\n", seq, burst_seq_recv);
                    // printf("TIMEOUT %.4f EXEPECTEDTIMEOUT %.4f\n", timeout.tv_usec / 1000.0, expectedTimeout.tv_usec / 1000.0);
                    pkt_buffer[burst_seq_recv] = data_pkt;
                    
                    // After receiving half of the packets, we can start sending
                    if (burst_seq_recv == BURST_SIZE / 2) {
                        burst_seq_send = 0;

                        sendto_dbg(s_server, &pkt_buffer[burst_seq_send], sizeof(data_pkt), 0,
                                   (struct sockaddr *) &send_addr, sizeof(send_addr));
                        burst_seq_send++;

                        gettimeofday(&tmPrev, NULL);
                        expectedTimeout = speed_to_interval(rate * BURST_FACTOR);
                        timeout = expectedTimeout;
                    }

                    burst_seq_recv++;

                    // We have recieved all the packets in the burst, so we can stop storing
                    if (burst_seq_recv == BURST_SIZE) {
                        burst_seq_recv = -1;
                        //temp fix for burst out of ordering/slow
                        while (burst_seq_send != -1 && burst_seq_send < BURST_SIZE) {
                            printf("sending packet at end %d of burst\n", burst_seq_send);
                            sendto_dbg(s_server, &pkt_buffer[burst_seq_send], sizeof(data_pkt), 0,
                                    (struct sockaddr *) &send_addr, sizeof(send_addr));
                            burst_seq_send++;
                        }
                        burst_seq_send = -1;
                    }
                }
                seq++;
            }
            if (FD_ISSET(s_server, &read_mask)) {
                int len = recvfrom(s_server, &recv_pkt, sizeof(data_packet), 0, (struct sockaddr *)&server_addr, &server_addr_len);
                if (len <= 0) {
                    printf("data stream ended, exiting...\n");
                    close(s_data);
                    close(s_server);
                    exit(0);
                }

                // Check if we have a decrepancy in our sending rates at sender and receiver
                if(recv_pkt.hdr.type == NETWORK_REPORT){
                    double reportedRate = 0.0;
                    reportedRate = recv_pkt.hdr.rate;


                    if (reportedRate < rate){
                        // set new rate to the max of less than reported rate to flush queue
                        double newRate = reportedRate - .5 * (rate - reportedRate);
                        if(newRate < 0.75*reportedRate){
                            reportedRate = 0.75*reportedRate;
                        }
                        else{
                            reportedRate = newRate;
                        }
                    }

                    rate = recv_pkt.hdr.rate >= MAX_SPEED ? MAX_SPEED : reportedRate;

                    control_pkt.rate = rate;
                    control_pkt.type = LOCAL_CONTROL;

                    send(s_data, &control_pkt, sizeof(control_pkt), 0);

                    sprintf(feedbackBuf, "%.4f", rate);
                    sendFeedbackMessage(feedbackBuf);

                }
            }
        } else {
            // timeout
            if (burst_seq_send == -1) {
                printf("timeout while sending\n");
            }
            else {  
                if (burst_seq_send >= burst_seq_recv && burst_seq_recv != -1) {
                    printf("Error: burst trying to send seq not received %d\n", burst_seq_send);
                    continue;
//                    exit(1);
                }

                printf("sending packet %d of burst\n", burst_seq_send);

                sendto_dbg(s_server, &pkt_buffer[burst_seq_send], sizeof(data_pkt), 0,
                        (struct sockaddr *) &send_addr, sizeof(send_addr));
                burst_seq_send++;

                
                gettimeofday(&tmNow, NULL);
                struct timeval baseTimeout = speed_to_interval(rate * BURST_FACTOR); 
                struct timeval tmExtra = diffTime(diffTime(tmNow, tmPrev), expectedTimeout);
                // printf("BASETIMEOUT %.4f EXTRATIME %.4f \n", baseTimeout.tv_usec / 1000.0, tmExtra.tv_usec / 1000.0);

                while (gtTime(tmExtra, baseTimeout) && burst_seq_send < BURST_SIZE && burst_seq_send < burst_seq_recv) {
                    printf("sending makeup packet %d of burst\n", burst_seq_send);
                    sendto_dbg(s_server, &pkt_buffer[burst_seq_send], sizeof(data_pkt), 0,
                            (struct sockaddr *) &send_addr, sizeof(send_addr));
                    tmExtra = diffTime(tmExtra, baseTimeout);
                    burst_seq_send++;
                }

                expectedTimeout = diffTime(baseTimeout, tmExtra);
                timeout = expectedTimeout;
                tmPrev = tmNow;
                // We are done
                if (burst_seq_send == BURST_SIZE) {
                    burst_seq_send = -1;
                }
            }
        }

        // if we are not sending at burst, we can just send when the data
        // source gives us a packet. However, if we are sending a burst,
        // we want to timeout to send at a certain rate
        if (burst_seq_send == -1) {
            timeout.tv_sec = TIMEOUT_SEC;
            timeout.tv_usec = TIMEOUT_USEC;
        }
    }
}

int setup_data_socket(bool android)
{
    int s, s2;
    socklen_t len, len2;

    struct sockaddr_un addr1, addr2;

    // Create socket that listens for connections
    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror("socket error\n");
        exit(1);
    }

    printf("Controller: Trying to connect...\n");

    const char name[] = "\0my.local.socket.address"; // fix android socket error
    if (android) {
        memset(&addr1, 0, sizeof(addr1)); // fix android socket error
        addr1.sun_family = AF_UNIX;
        memcpy(addr1.sun_path, name, sizeof(name) - 1); // fix android socket error
        len = strlen(addr1.sun_path) + sizeof(name); // fix android socket error
        addr1.sun_path[0] = 0; // fix android socket error
    } else {
        addr1.sun_family = AF_UNIX;
        strcpy(addr1.sun_path, SOCK_PATH);
        len = strlen(addr1.sun_path) + sizeof(addr1.sun_family);
    }
   
    unlink(addr1.sun_path);
    printf("Controller: before bind\n");
    if (bind(s, (struct sockaddr *)&addr1, len) == -1) {
        perror("bind");
        exit(1);
    }
    printf("Controller: after bind\n");

    if (listen(s, 5) == -1) {
        perror("listen");
        exit(1);
    }

    // Received connection
    len2 = sizeof(addr2);
    if ((s2 = accept(s, (struct sockaddr *)&addr2, &len2)) == -1) {
        perror("accept");
        exit(1);
    }

    printf("Controller: Connected to data generator.\n");
    close(s); // close the original socket
    return s2;
}

int setup_server_socket(int port, bool android)
{
    struct sockaddr_in name;

    int s_recv = socket(AF_INET, SOCK_DGRAM, 0);  /* socket for receiving (udp) */
    if (s_recv < 0) {
        perror("socket recv error\n");
        exit(1);
    }

    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(port);

    if (bind( s_recv, (struct sockaddr *)&name, sizeof(name) ) < 0 ) {
        perror("bind error\n");
        exit(1);
    }

    return s_recv;
}

struct sockaddr_in addrbyname(const char *hostname, int port)
{
    int host_num;
    struct hostent h_ent, *p_h_ent;

    struct sockaddr_in addr;

    p_h_ent = gethostbyname(hostname);
    if (p_h_ent == NULL) {
        printf("gethostbyname error.\n");
        exit(1);
    }

    memcpy( &h_ent, p_h_ent, sizeof(h_ent));
    memcpy( &host_num, h_ent.h_addr_list[0], sizeof(host_num) );

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = host_num;
    addr.sin_port = htons(port);

    return addr;
}

double estimate_change(double rate){
    return 0;
}

void stop_controller_thread() {
    kill_thread = true;
}
//if (reportedRate < rate)
// max (reportedRate - (rate - reportedRate), .5 * reportedRate)
