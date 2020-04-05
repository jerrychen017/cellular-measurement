#include "interactive_client.h"
// server socket address
static struct sockaddr_in server_addr;
// address of incoming packets
static struct sockaddr_in interactive_pac_addr;
// client socket address
static struct sockaddr_in client_addr;
static socklen_t client_len;
static int sk;
// get server host address
static struct hostent *server_name;
static struct hostent server_name_copy;
static int server_fd;

static fd_set mask;
static fd_set read_mask;
static int num;
static struct timeval now;
static struct timeval latency;

static InteractivePacket packet_send;

// send CONNECT packet to interactive server
// returns an id
int interactive_connect(const char name[NAME_LENGTH])
{
    // initialize starting packet and echo_packet
    ConnectPacket connect_packet;
    connect_packet.type = 4; // CONNECT
    connect_packet.id = -1;
    memcpy(connect_packet.name, name, NAME_LENGTH);

    // send CONNECT packet to server
    sendto(sk, (ConnectPacket *)&connect_packet, sizeof(connect_packet), 0,
           (struct sockaddr *)&interactive_pac_addr, sizeof(interactive_pac_addr));

    // receive CONNECT packet from server
    // initialize packet to be received
    ConnectPacket received_packet;
    read_mask = mask;
    struct timeval timeout;
    for (;;)
    {
        timeout.tv_sec = INTERACTIVE_TIMEOUT_SEC;
        timeout.tv_usec = INTERACTIVE_TIMEOUT_USEC;
        num = select(FD_SETSIZE, &read_mask, NULL, NULL, &timeout);
        if (num > 0)
        {
            if (FD_ISSET(sk, &read_mask))
            {
                client_len = sizeof(client_addr);
                recvfrom(sk, &received_packet, sizeof(received_packet), 0,
                         (struct sockaddr *)&client_addr, &client_len);
                int received_error = received_packet.error;
                switch (received_error)
                {
                case NO_ERROR:
                    // initalize packet_send
                    packet_send.type = 3; // INTERACTIVE
                    packet_send.id = received_packet.id;
                    memcpy(packet_send.name, name, NAME_LENGTH);
                    return received_packet.id;
                    break;
                case ID_NOT_FOUND:
                    return -1;
                    break;
                case MAX_USERS_REACHED:
                    return -2;
                    break;
                default:
                    break;
                }
            }
        }
        else
        {
            printf("interactive_connect: timeout!");
            // send CONNECT packet to server again
            sendto(sk, (ConnectPacket *)&connect_packet, sizeof(connect_packet), 0,
                   (struct sockaddr *)&interactive_pac_addr, sizeof(interactive_pac_addr));
        }
    }
}

int send_interactive_packet(int seq_num, float x, float y)
{
    // initialize starting packet and echo_packet
    packet_send.seq = seq_num;
    packet_send.x = x;
    packet_send.y = y;
    gettimeofday(&packet_send.send_time, NULL);
    // send init packet to rcv
    sendto(sk, (EchoPacket *)&packet_send, sizeof(packet_send), 0,
           (struct sockaddr *)&interactive_pac_addr, sizeof(interactive_pac_addr));
    printf("interactive packet is sent\n");
    return 0;
}

/**
 * receive an interactive packet with a certain sequence number
 * @return InteractivePacket
 */
InteractivePacket receive_interactive_packet()
{
    // initialize packet to be received
    InteractivePacket received_packet;

    read_mask = mask;

    num = select(FD_SETSIZE, &read_mask, NULL, NULL, NULL);
    if (num > 0)
    {
        if (FD_ISSET(sk, &read_mask))
        {
            client_len = sizeof(client_addr);
            recvfrom(sk, &received_packet, sizeof(received_packet), 0,
                     (struct sockaddr *)&client_addr, &client_len);

            gettimeofday(&now, NULL);
            latency = diffTime(now, received_packet.send_time);
            received_packet.latency = latency.tv_usec / 1000 + latency.tv_sec * 1000;
            return received_packet;
        }
    }
    else
    {
        printf("receive_interactive_packet timeout error!");
        received_packet.seq = -1;
        return received_packet;
    }
}

void init_socket(const char *address, int port)
{

    // socket both for sending and receiving
    sk = socket(AF_INET, SOCK_DGRAM, 0);
    if (sk < 0)
    {
        perror("echo_client: socket error\n");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // binding was done in client_bind

    server_name = gethostbyname(address);
    if (server_name == NULL)
    {
        perror("echo_client: invalid server address\n");
        return;
    }

    memcpy(&server_name_copy, server_name, sizeof(server_name_copy));
    memcpy(&server_fd, server_name_copy.h_addr_list[0], sizeof(server_fd));

    // send echo_pac_addr to be server address
    interactive_pac_addr.sin_family = AF_INET;
    interactive_pac_addr.sin_addr.s_addr = server_fd;
    interactive_pac_addr.sin_port = htons(port);

    FD_ZERO(&mask);
    FD_SET(sk, &mask);
}
