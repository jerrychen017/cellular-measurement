#include "interactive_client.h"
// server socket address
static struct sockaddr_in server_addr;
// address of incoming packets
static struct sockaddr_in echo_pac_addr;
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

static EchoPacket packet_send;

// send CONNECT packet to interactive server
int interactive_connect(int id, const char name[NAME_LENGTH])
{
    // initialize starting packet and echo_packet
    ConnectPacket connect_packet;
    connect_packet.type = 4; // CONNECT
    connect_packet.id = id;
    memcpy(connect_packet.name, name, NAME_LENGTH);

    // initalize packet_send
    packet_send.type = 2; // ECHO
    packet_send.id = id;
    memcpy(packet_send.name, name, NAME_LENGTH);

    // send init packet to rcv
    sendto(sk, (ConnectPacket *)&connect_packet, sizeof(connect_packet), 0,
           (struct sockaddr *)&echo_pac_addr, sizeof(echo_pac_addr));
    return 0;
}

int send_interactive_packet(int seq_num, float x, float y)
{

    // initialize starting packet and echo_packet
    packet_send.seq = seq_num;
    packet_send.x = x;
    packet_send.y = y;

    // send init packet to rcv
    sendto(sk, (EchoPacket *)&packet_send, sizeof(packet_send), 0,
           (struct sockaddr *)&echo_pac_addr, sizeof(echo_pac_addr));
    printf("interactive packet is sent\n");
    return 0;
}

/**
 * receive an interactive packet with a certain sequence number
 * @return EchoPacket
 */
EchoPacket receive_interactive_packet()
{
    float *coord = malloc(3 * sizeof(float));
    // initialize packet to be received
    EchoPacket received_packet;

    read_mask = mask;

    num = select(FD_SETSIZE, &read_mask, NULL, NULL, NULL);
    if (num > 0)
    {
        if (FD_ISSET(sk, &read_mask))
        {
            client_len = sizeof(client_addr);
            recvfrom(sk, &received_packet, sizeof(received_packet), 0,
                     (struct sockaddr *)&client_addr, &client_len);

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
    echo_pac_addr.sin_family = AF_INET;
    echo_pac_addr.sin_addr.s_addr = server_fd;
    echo_pac_addr.sin_port = htons(port);

    FD_ZERO(&mask);
    FD_SET(sk, &mask);
}
