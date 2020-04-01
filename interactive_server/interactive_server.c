#include "include/net_include.h"

static TimingPacket timing, *recvTiming;
static ReportPacket report, *recvReport;
static EchoPacket echo, *recvEcho;
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
    int max_num_users = atoi(argv[2]);
    User users[max_num_users];
    int num_users = 0;

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
    // socket address of current client
    struct sockaddr_in sockaddr_client;

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
                    // echo.type = ECHO;
                    // echo.seq = recvEcho->seq;
                    // echo.x = recvEcho->x;
                    // echo.y = recvEcho->y;
                    // echo.name

                    // send the echo packet to each user
                    for (int i = 0; i < num_users; i++)
                    {
                        sendto(sk, &recvEcho, sizeof(recvEcho), 0,
                               (struct sockaddr *)&users[i], sizeof(users[i]));
                    }
                }
                else if (packet->type == INTERACTIVE)
                {
                    printf("received an interactive packet\n");
                    recvEcho = (EchoPacket *)packet;
                    echo.type = INTERACTIVE;
                    echo.seq = recvEcho->seq;
                    echo.x = recvEcho->x;
                    echo.y = recvEcho->y;
                    // send the echo packet to each user
                    sendto(sk, &echo, sizeof(echo), 0,
                           (struct sockaddr *)&sockaddr_client_pac, sizeof(sockaddr_client_pac));
                }
                else
                { // CONNECT packet
                    printf("received a connect packet\n");
                    recvConnect = (ConnectPacket *)packet;
                    User new_user;
                    new_user.id = recvConnect->id;
                    memcpy(new_user.name, recvConnect->name, NAME_LENGTH);
                    new_user.socket_addr = sockaddr_client_pac;
                    // add user to the sending list
                    if (num_users == max_num_users)
                    { // max number of users already
                        printf("max number of users has been reached\n");
                    }
                    else
                    {
                        users[num_users];
                        num_users++;
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
