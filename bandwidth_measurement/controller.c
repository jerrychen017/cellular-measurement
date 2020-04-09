#include "controller.h"
#include "logger.h"

int start_controller(const char* address, int port, bool android)
{
    sendto_dbg_init(0);
    
    int s_server = setup_server_socket(port, android);
    struct sockaddr_in send_addr = addrbyname(address, port);
    int s_data = setup_data_socket(android);

    startup(s_server, s_data, send_addr);
    control(s_server, s_data, send_addr);

    return 0;
}

/* Pings data generator to check that it is there and then starts data stream */
void startup(int s_server, int s_data, struct sockaddr_in send_addr)
{
    typed_packet pkt;
    pkt.type = LOCAL_START;
    send(s_data, &pkt, sizeof(pkt.type), 0);
}




/* Main event loop */
void control(int s_server, int s_data, struct sockaddr_in send_addr)
{
    static char feedbackBuf[256];

    struct sockaddr_in server_addr;
    socklen_t server_addr_len;

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
                    exit(0);
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
                        gettimeofday(&tmPrev, NULL);
                        timeout = speed_to_interval(0);
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
                    typed_packet pkt;
                    pkt.type = LOCAL_CONTROL;
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

                    sprintf(feedbackBuf, "adjusted rate to %.4f\n", reportedRate);
                    sendMessage(feedbackBuf, strlen(feedbackBuf));

                    pkt.rate = recv_pkt.hdr.rate >= MAX_SPEED ? MAX_SPEED : reportedRate;
                    send(s_data, &pkt, sizeof(pkt), 0);
                    rate = pkt.rate;
                }
            }
        } else {
            // timeout
            if (burst_seq_send == -1) {
                printf("timeout?\n");
            }
            else {  
                if (burst_seq_send >= burst_seq_recv && burst_seq_recv != -1) {
                    printf("Error: burst trying to send seq not received %d\n", burst_seq_send);
                    exit(1);
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
                // printf("EXPECTEDTIMEOUT %.4f \n", expectedTimeout.tv_usec / 1000.0);
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

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror("socket error\n");
        exit(1);
    }

    printf("Trying to connect...\n");

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
    if (bind(s, (struct sockaddr *)&addr1, len) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(s, 5) == -1) {
        perror("listen");
        exit(1);
    }

    printf("Waiting for a connection...\n");

    if (android) {
        const char name2[] = "\0my2.local.socket.address"; // fix android socket error
        memcpy(addr2.sun_path, name2, sizeof(name2) - 1); // fix android socket error
        len2 = strlen(addr2.sun_path) + sizeof(name); // fix android socket error
        addr2.sun_path[0] = 0; // fix android socket error
    } else {
        len2 = sizeof(addr1);
    }
    
    if ((s2 = accept(s, (struct sockaddr *)&addr2, &len2)) == -1) {
        perror("accept");
        exit(1);
    }

    printf("Connected.\n");

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

    // bind is done separaltely when running on the android side
    if (!android && bind( s_recv, (struct sockaddr *)&name, sizeof(name) ) < 0 ) {
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

//if (reportedRate < rate)
// max (reportedRate - (rate - reportedRate), .5 * reportedRate)
