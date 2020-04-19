#include "net_include.h"
#include "bandwidth_utils.h"
#include "sendto_dbg.h"

#define ALPHA 0.1  // closer to 0 is smoother, closer to 1 is quicker reaction (90 packets ~ 1Mbps) 0.2/0.1 for regular
#define THRESHOLD 0.95 // percent drop threshold

static TimingPacket timing, *recvTiming;
static ReportPacket report, *recvReport;
static EchoPacket echo, *recvEcho;
static InteractivePacket interactive, *recvInteractive;
static ConnectPacket _connect, *recvConnect;
static Packet pack, *packet = &pack;

int setup_server_socket(int port);
void receive(int s_bw, int s_inter, int predMode, int max_num_users);

int main(int argc, char **argv)
{

    // argc error checking
    if (argc != 5)
    {
        printf("Usage: echo_server <portBandwidth> <portInteractive> <EWMA/RunningAvg> <max-num-users> \n");
        exit(0);
    }
    // get separate ports for bandwidth and interactive applications
    int portBandwidth = atoi(argv[1]);
    int portInteractive = atoi(argv[2]);
    // toggle Prediction mode 0 for EMWA and 1 for RunningAvg
    int predMode = atoi(argv[3]);
    int max_num_users = atoi(argv[4]);

    // Bandwidth application socket
    int s_bw = setup_server_socket(portBandwidth);
    int s_inter = setup_server_socket(portInteractive);
    receive(s_bw, s_inter, predMode, max_num_users);

    return 0;
}


void receive(int s_bw, int s_inter, int predMode, int max_num_users)
{
    User users[max_num_users];              // an array of users
    int users_id[max_num_users];            // an array of user id
    memset(users_id, -1, sizeof(users_id)); // initialize all user ids to -1
    int num_users = 0;                      // current number of users

    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);

    // Select loop
    fd_set mask;
    fd_set read_mask;
    struct timeval timeout;
    int num;
    int len;

    // state variables
    // Current rate the server expectes to receive at
    double curRate = 0;
    // Rate that ewma predicts
    double ewmaRate = 0;
    // only use measurements after we have received WINDOW_SIZE packets at current rate
    int numAtCurRate = 0;
    // number of packets below threshold
    int numBelowThreshold = 0; 

    // next expected seq number
    int seq = 0;
    // Arrival time of last BURST_SIZE packets
    struct timeval arrivals[BURST_SIZE];
    // whether we have received a packet in window
    int received[BURST_SIZE];

    // analogous to above for bursts
    int burstSeq = 0;
    // first burst packet received
    int bFirst = 0;
    struct timeval barrivals[BURST_SIZE];
    int breceived[BURST_SIZE];



    memset(received, 0, sizeof(received));
    memset(breceived, 0, sizeof(breceived));


    // variables to store caclulations in
    double calculated_speed;
    struct timeval tm_diff;

    // packet buffers
    data_packet data_pkt;
    packet_header report_pkt;
    packet_header ack_pkt;

    // Add file descriptors to fdset
    FD_ZERO(&mask);
    FD_SET(s_bw, &mask);
    FD_SET(s_inter, &mask);

    for (;;) {
        read_mask = mask;
        timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = TIMEOUT_USEC;

        num = select(FD_SETSIZE, &read_mask, NULL, NULL, &timeout);

        if (num > 0) {
            if (FD_ISSET(s_bw, &read_mask)) {
                len = recvfrom(s_bw, &data_pkt, sizeof(data_packet), 0,
                               (struct sockaddr *) &from_addr, &from_len);
                if (len < 0) {
                    perror("socket error");
                    exit(1);
                }
                printf("received %d bytes, seq %d at rate %f\n", len, data_pkt.hdr.seq_num, data_pkt.hdr.rate );

                // When we receive a new START message, reset the server
                if (data_pkt.hdr.type == NETWORK_START) {
                    seq = 0;
                    burstSeq = 0;
                    bFirst = 0;

                    memset(received, 0, sizeof(received));
                    memset(breceived, 0, sizeof(breceived));

                    ack_pkt.type = NETWORK_START_ACK;
                    ack_pkt.rate = 0;
                    ack_pkt.seq_num = 0;

                    sendto_dbg(s_bw, &ack_pkt, sizeof(packet_header), 0,
                            (struct sockaddr *) &from_addr, from_len);
                    continue;
                }


                double expectedRate = data_pkt.hdr.rate;
                int currSeq = data_pkt.hdr.seq_num;

                // keep track of how many packets we've received at the current rate
                if (expectedRate != curRate) {
                    curRate = expectedRate;
                    numAtCurRate = 0;
                }
                numAtCurRate++;

                // out of order packet, drop
                if (currSeq < seq) {
                    continue;
                }


                // mark packets up to received seq as not received
                for (; seq < currSeq; seq++) {
                    received[seq % BURST_SIZE] = 0;
                }
                // mark current pacekt as received and record arrival time
                int i = currSeq % BURST_SIZE;
                received[i] = 1;
                gettimeofday(&arrivals[i], NULL);

                if (data_pkt.hdr.type == NETWORK_BURST){
                    // first burst packet we have received
                    if (burstSeq == 0) {
                        memset(breceived, 0, sizeof(breceived));
                    }

                    // Calculate index within burst
                    int bursti = (currSeq % INTERVAL_SIZE) - (INTERVAL_SIZE - BURST_SIZE);

                    if (bursti >= burstSeq) {
                        if (burstSeq == 0) bFirst = bursti;
                        gettimeofday(&barrivals[bursti], NULL);
                        breceived[i] = 1;
                        burstSeq = bursti + 1;
                    }
                }
                else {
                    // burst just finished, send report
                    if (burstSeq != 0) {
                        tm_diff = diffTime(barrivals[burstSeq - 1], barrivals[bFirst]);
                        if (burstSeq - 1 != bFirst) {
                            calculated_speed = interval_to_speed(tm_diff, (burstSeq - 1) - bFirst);
                            printf("Burst calculated speed of %.4f Mbps\n", calculated_speed);
                            report_pkt.type = NETWORK_BURST_REPORT;
                            report_pkt.rate = calculated_speed;
                            report_pkt.seq_num = currSeq;

                            sendto_dbg(s_bw, &report_pkt, sizeof(report_pkt), 0,
                                       (struct sockaddr *) &from_addr, from_len);
                            burstSeq = 0;
                        }
                    }

                    // First assume rate is as expected (if we are unable to calculate because we
                    // haven't received enough, we don't want to do anything)
                    double calcRate = expectedRate;

                    // EWMA
                    if (predMode == 0 && numAtCurRate >= 2) {
                        if (!received[(currSeq - 1) % BURST_SIZE]) continue;

                        tm_diff = diffTime(arrivals[currSeq % BURST_SIZE], arrivals[(currSeq - 1) % BURST_SIZE]);
                        calculated_speed = interval_to_speed(tm_diff, 1);
                        ewmaRate = (ALPHA * calculated_speed) + (1 - ALPHA) * ewmaRate;
                        printf("Computed sending rate of %.4f Mbps\n", ewmaRate);
                        calcRate = ewmaRate;

                    }
                    // Running Avg
                    if (predMode == 1 && numAtCurRate >= BURST_SIZE) {
                        if (!received[(currSeq + 1) % BURST_SIZE]) continue;

                        // Wrap around average of packets
                        tm_diff = diffTime(arrivals[currSeq % BURST_SIZE], arrivals[(currSeq + 1) % BURST_SIZE]);
                        calculated_speed = interval_to_speed(tm_diff, BURST_SIZE - 1);
                        calcRate = calculated_speed; //Figure out threshold
                        printf("Computed sending rate of %.4f Mbps\n", calcRate);
                    }

                    // Send report packet if we are under 90 percent of expected rate
                    if (calcRate <= THRESHOLD * expectedRate) {
                        if (numBelowThreshold == GRACE_PERIOD) {
                            report_pkt.type = NETWORK_REPORT;
                            report_pkt.rate = calcRate;
                            report_pkt.seq_num = currSeq;
                            sendto_dbg(s_bw, &report_pkt, sizeof(report_pkt), 0,
                                   (struct sockaddr *) &from_addr, from_len);
                            printf("Computed rate %.4f below threshold, actual rate %.4f\n", calcRate, expectedRate);
                            numBelowThreshold = 0; 
                        } else {
                            numBelowThreshold++; 
                        }
                    } else {
                        // reset numBelowThreshold when received non-delayed packet within the grace period
                        if (numBelowThreshold != 0) {
                            printf("num threshold is %d\n", numBelowThreshold);
                        }
                        numBelowThreshold = 0; 
                    }
                }

                // increment seq
                seq = currSeq + 1;
            }

            if (FD_ISSET(s_inter, &read_mask))
            {
                from_len = sizeof(from_addr);
                recvfrom(s_inter, &pack, sizeof(Packet), 0, (struct sockaddr *)&from_addr, &from_len);
                if (packet->type == TIMING)
                { // Timing packet
                    // recvTiming = (TimingPacket *)packet;
                    printf("received a timing packet\n");
                }
                else if (packet->type == REPORT)
                { // Report packet
                    recvReport = (ReportPacket *)packet;
                    printf("received a report packet\n");
                }
                else if (packet->type == ECHO)
                { // Echo packet
                    printf("received an echo packet\n");
                    recvEcho = (EchoPacket *)packet;
                    echo.type = ECHO;
                    echo.seq = recvEcho->seq;
                    // send the echo packet to each user
                    sendto(s_inter, &echo, sizeof(echo), 0,
                           (struct sockaddr *)&from_addr, sizeof(from_addr));
                }
                else if (packet->type == INTERACTIVE)
                {
                    // printf("received an interactive packet\n");
                    recvInteractive = (InteractivePacket *)packet;
                    int cur_user_id = recvInteractive->id;
                    // check if the address of the user is set
                    if (!users[cur_user_id].address_set)
                    {
                        users[cur_user_id].address_set = true;
                        memcpy(&(users[cur_user_id].socket_addr), &from_addr, sizeof(from_addr));
                    }

                    interactive.type = INTERACTIVE;
                    interactive.seq = recvInteractive->seq;
                    interactive.x = recvInteractive->x;
                    interactive.y = recvInteractive->y;
                    interactive.send_time = recvInteractive->send_time;
                    memcpy(interactive.name, recvInteractive->name, sizeof(NAME_LENGTH));
                    interactive.id = recvInteractive->id;

                    // send the interactive packet to each user
                    for (int i = 0; i < num_users; i++)
                    {
                        sendto(s_inter, &interactive, sizeof(interactive), 0,
                               (struct sockaddr *)&(users[i].socket_addr), sizeof(users[i].socket_addr));
                        // printf("sent an interactive packet with name <%s>, id <%d>, coord_x <%f>, coord_y <%f>\n", users[i].name, users[i].id, interactive.x, interactive.y);
                    }
                }
                else
                { // CONNECT packet
                    printf("received a connect packet\n");
                    recvConnect = (ConnectPacket *)packet;
                    int cur_user_id = recvConnect->id; // this user's id
                    // user id is -1 if it has not been assigned by the server.
                    if (cur_user_id == -1)
                    { // new user
                        if (num_users == max_num_users)
                        { // maximum number of users reached
                            // send MAX_USERS_REACHED error to client
                            _connect.type = CONNECT;
                            _connect.id = -1;
                            _connect.error = MAX_USERS_REACHED;
                            sendto(s_inter, &_connect, sizeof(_connect), 0,
                                   (struct sockaddr *)&from_addr, sizeof(from_addr));
                        }
                        else
                        {
                            User new_user;
                            new_user.id = num_users; // assign new user with an id
                            memcpy(new_user.name, recvConnect->name, NAME_LENGTH);
                            new_user.address_set = false; // set address when first receive an interactive packet ffrom the user
                            // send an CONNECT packet with id to user
                            _connect.type = CONNECT;
                            _connect.id = num_users;
                            users_id[num_users] = num_users; // put id in users_id array
                            _connect.error = NO_ERROR;
                            sendto(s_inter, &_connect, sizeof(_connect), 0,
                                   (struct sockaddr *)&from_addr, sizeof(from_addr));
                            num_users++;
                        }
                    }
                    else
                    { // existing user
                        // check if the user id is in the users id array
                        bool exist = false;
                        for (int i = 0; i < num_users; i++)
                        {
                            if (users_id[i] == cur_user_id)
                            {
                                exist = true;
                            }
                        }
                        if (exist)
                        {
                            // send a connect message with the same user id to the client
                            _connect.type = CONNECT;
                            _connect.id = recvConnect->id;
                            sendto(s_inter, &_connect, sizeof(_connect), 0,
                                   (struct sockaddr *)&from_addr, sizeof(from_addr));
                        }
                        else
                        {
                            // invalid user id
                            _connect.type = CONNECT;
                            _connect.id = recvConnect->id;
                            _connect.error = ID_NOT_FOUND;
                            sendto(s_inter, &_connect, sizeof(_connect), 0,
                                   (struct sockaddr *)&from_addr, sizeof(from_addr));
                        }
                    }
                }
            }
        } else {
            printf(".");
            fflush(0);
        }
    }
}

int setup_server_socket(int port)
{
    struct sockaddr_in name;

    int s = socket(AF_INET, SOCK_DGRAM, 0);  /* socket for receiving (udp) */
    if (s < 0) {
        perror("socket error\n");
        exit(1);
    }

    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(port);

    if (bind( s, (struct sockaddr *)&name, sizeof(name) ) < 0 ) {
        perror("bind error\n");
        exit(1);
    }

    return s;
}
