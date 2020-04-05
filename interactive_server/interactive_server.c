#include "net_include.h"

static TimingPacket timing, *recvTiming;
static ReportPacket report, *recvReport;
static EchoPacket echo, *recvEcho;
static InteractivePacket interactive, *recvInteractive;
static ConnectPacket _connect, *recvConnect;
static Packet pack, *packet = &pack;

int main(int argc, char **argv)
{

    // argc error checking
    if (argc != 3)
    {
        printf("Usage: interactive_server <port> <max-num-users>\n");
        exit(0);
    }

    int port = atoi(argv[1]);
    int max_num_users = atoi(argv[2]);      // maximum number of users
    User users[max_num_users];              // an array of users
    int users_id[max_num_users];            // an array of user id
    memset(users_id, -1, sizeof(users_id)); // initialize all user ids to -1
    int num_users = 0;                      // current number of users

    // create a socket both for sending and receiving
    int sk = socket(AF_INET, SOCK_DGRAM, 0);
    if (sk < 0)
    {
        perror("echo_server: cannot create a socket for sending and receiving\n");
        exit(1);
    }

    struct sockaddr_in sockaddr_server;
    sockaddr_server.sin_family = AF_INET;
    sockaddr_server.sin_addr.s_addr = INADDR_ANY;
    sockaddr_server.sin_port = htons(port);

    // bind the socket with PORT
    if (bind(sk, (struct sockaddr *)&sockaddr_server, sizeof(sockaddr_server)) < 0)
    {
        perror("interactive_server: cannot bind socket with server address\n");
        exit(1);
    }

    // socket address of received packet from client
    struct sockaddr_in sockaddr_client_pac;
    socklen_t sockaddr_client_pac_len;

    // packet received from client
    char init_packet[BUFF_SIZE];
    // packet to be sent from server
    char echo_packet[BUFF_SIZE];

    // wait interval for connection requests
    struct timeval idle_interval;

    fd_set mask;
    fd_set read_mask;
    int num;

    FD_ZERO(&mask);
    FD_SET(sk, &mask);

    struct timeval timeout;

    for (;;)
    {
        read_mask = mask;
        timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = TIMEOUT_USEC;

        num = select(FD_SETSIZE, &read_mask, NULL, NULL, &timeout);

        if (num > 0)
        {
            if (FD_ISSET(sk, &read_mask))
            {
                sockaddr_client_pac_len = sizeof(sockaddr_client_pac);
                recvfrom(sk, &pack, sizeof(Packet), 0, (struct sockaddr *)&sockaddr_client_pac, &sockaddr_client_pac_len);
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
                    sendto(sk, &echo, sizeof(echo), 0,
                           (struct sockaddr *)&sockaddr_client_pac, sizeof(sockaddr_client_pac));
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
                        memcpy(&(users[cur_user_id].socket_addr), &sockaddr_client_pac, sizeof(sockaddr_client_pac));
                    }

                    interactive.type = INTERACTIVE;
                    interactive.seq = recvInteractive->seq;
                    interactive.x = recvInteractive->x;
                    interactive.y = recvInteractive->y;
                    memcpy(interactive.name, recvInteractive->name, sizeof(NAME_LENGTH));
                    interactive.id = recvInteractive->id;

                    // send the interactive packet to each user
                    for (int i = 0; i < num_users; i++)
                    {
                        sendto(sk, &interactive, sizeof(interactive), 0,
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
                            sendto(sk, &_connect, sizeof(_connect), 0,
                                   (struct sockaddr *)&sockaddr_client_pac, sizeof(sockaddr_client_pac));
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
                            sendto(sk, &_connect, sizeof(_connect), 0,
                                   (struct sockaddr *)&sockaddr_client_pac, sizeof(sockaddr_client_pac));
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
                            sendto(sk, &_connect, sizeof(_connect), 0,
                                   (struct sockaddr *)&sockaddr_client_pac, sizeof(sockaddr_client_pac));
                        }
                        else
                        {
                            // invalid user id
                            _connect.type = CONNECT;
                            _connect.id = recvConnect->id;
                            _connect.error = ID_NOT_FOUND;
                            sendto(sk, &_connect, sizeof(_connect), 0,
                                   (struct sockaddr *)&sockaddr_client_pac, sizeof(sockaddr_client_pac));
                        }
                    }
                }
            }
        }
        else
        { // TIMEOUT
            printf(".");
        }
    }

    return 0;
}
