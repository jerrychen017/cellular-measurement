#include "net_include.h"
#include "utils.h"

void startup(int s_server, int s_data, struct sockaddr_in send_addr);
void control(int s_server, int s_data, struct sockaddr_in send_addr);

int setup_data_socket();
int setup_server_socket();
struct sockaddr_in addrbyname(char *hostname, int port);

/**
    Controller 
*/
int main(int argc, char *argv[])
{

    // args error checking
    if (argc != 3)
    {
        printf("controller usage: controller <host_address> <port>\n");
        exit(1);
    }

    // port
    int port = atoi(argv[2]);
    // address
    char *address = argv[1];

    int s_server = setup_server_socket(port);
    struct sockaddr_in send_addr = addrbyname(address, port);
    int s_data = setup_data_socket();

    startup(s_server, s_data, send_addr);
    control(s_server, s_data, send_addr);

    return 0;
}

/* Pings server to check that it is there and then starts data stream */
void startup(int s_server, int s_data, struct sockaddr_in send_addr)
{
    typed_packet pkt;
    pkt.type = LOCAL_START;
    send(s_data, &pkt, sizeof(pkt.type), 0);
}

/* Main event loop */
void control(int s_server, int s_data, struct sockaddr_in send_addr)
{
    fd_set mask;
    fd_set read_mask;
    int num;

    FD_ZERO(&mask);
    FD_SET(s_server, &mask);
    FD_SET(s_data, &mask);

    struct timeval timeout;
    data_packet data_pkt;

    int seq = 0;
    double rate = 1.0;

    for (;;) {
        read_mask = mask;
        timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = TIMEOUT_USEC;

        num = select(FD_SETSIZE, &read_mask, NULL, NULL, &timeout);

        if (num > 0) {
            if (FD_ISSET(s_data, &read_mask)) {
                int len = recv(s_data, data_pkt.data, sizeof(data_packet), 0); 
                if (len == 0) {
                    printf("data stream ended, exiting...\n");
                    close(s_data);
                    close(s_server);
                    exit(0);
                }

                data_pkt.hdr.type = NETWORK_DATA;
                data_pkt.hdr.seq_num = seq;
                data_pkt.hdr.rate = rate;

                printf("Sending to serveR \n");

                // Pass to server
                sendto(s_server, &data_pkt, sizeof(data_pkt), 0,
                        (struct sockaddr *) &send_addr, sizeof(send_addr));

                seq++;
            }
            if (FD_ISSET(s_server, &read_mask)) {

            }
        } else {
            printf("timeout?\n");
        }

    }
}

int setup_data_socket()
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

    addr1.sun_family = AF_UNIX;
    strcpy(addr1.sun_path, SOCK_PATH);
    len = strlen(addr1.sun_path) + sizeof(addr1.sun_family);
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
    len2 = sizeof(addr1);
    if ((s2 = accept(s, (struct sockaddr *)&addr2, &len2)) == -1) {
        perror("accept");
        exit(1);
    }

    printf("Connected.\n");

    return s2;
}

int setup_server_socket(int port)
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

struct sockaddr_in addrbyname(char *hostname, int port)
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
