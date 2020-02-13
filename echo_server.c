#include "include/net_include.h"
#include "utils.h"

int main(int argc, char** argv) {

// argc error checking
if (argc != 2) {
    printf("Usage: echo_server <port>\n");
    exit(0);
}

int port = atoi(argv[1]);

// create a socket both for sending and receiving
int sk = socket(AF_INET, SOCK_DGRAM, 0);
if (sk < 0) {
    perror("echo_server: cannot create a socket for sending and receiving\n");
    exit(1);
}

struct sockaddr_in sockaddr_server;
sockaddr_server.sin_family = AF_INET;
sockaddr_server.sin_addr.s_addr = INADDR_ANY;
sockaddr_server.sin_port = htons(port);

// bind the socket with PORT
if (bind(sk, (struct sockaddr*)&sockaddr_server, sizeof(sockaddr_server)) < 0) {
    perror("echo_server: cannot bind socket with server address");
    exit(1);
}

fd_set read_mask;
fd_set mask;
FD_ZERO(&mask);
FD_SET(sk, &mask);


// socket address of received packet from client
struct sockaddr_in sockaddr_client_pac;
socklen_t sockaddr_client_pac_len;
// socket address of current client
struct sockaddr_in sockaddr_client;

// packet received from client
char init_packet[BUFF_SIZE];
// packet to be sent from server
char echo_packet[BUFF_SIZE];

// record starting time of transfer   
struct timeval start_time;
struct timeval last_time;

// wait interval for connection requests
struct timeval idle_interval;

for (;;) {
    read_mask = mask;
    idle_interval.tv_sec = 10;
    idle_interval.tv_usec = 0;
    int num = select(FD_SETSIZE, &read_mask, NULL, NULL, &idle_interval);

    if (num > 0) {
        if (FD_ISSET(sk, &read_mask)) {
        sockaddr_client_pac_len = sizeof(sockaddr_client_pac);
        // receive packet from client and store its address
        recvfrom(sk, &init_packet, sizeof(init_packet), 0,
                (struct sockaddr*) &sockaddr_client_pac, &sockaddr_client_pac_len);
        int client_ip = sockaddr_client_pac.sin_addr.s_addr;

        printf("Echo packet sending to address (%d.%d.%d.%d)\n",
                 (htonl(sockaddr_client_pac.sin_addr.s_addr) & 0xff000000) >> 24,
                 (htonl(sockaddr_client_pac.sin_addr.s_addr) & 0x00ff0000) >> 16,
                 (htonl(sockaddr_client_pac.sin_addr.s_addr) & 0x0000ff00) >> 8,
                 (htonl(sockaddr_client_pac.sin_addr.s_addr) & 0x000000ff));
        // send back echo packet
        sendto(sk, (char *) &echo_packet, sizeof(echo_packet), 0,
                            (struct sockaddr*) &sockaddr_client_pac, sizeof(sockaddr_client_pac));

        }
    } else {
        printf(".");
        fflush(0);
    }
}

return 0; 
}