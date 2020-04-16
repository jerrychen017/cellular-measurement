#include "echo_client.h"

int client_bind(const char* address, int port) {

    // server socket address
    struct sockaddr_in server_addr;

    // socket both for sending and receiving
    int sk = socket(AF_INET, SOCK_DGRAM, 0);
    if (sk < 0) {
        printf("echo_client: socket error\n");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // bind socket with port
    if (bind(sk, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("echo_client: bind error\n"); // prints a warning but don't exit.
        // we can still send since it's already binded
        return 1;
    }
    return 0;
}

char * echo_send(const char* address, int port, int sequence) {
    char * out = malloc(sizeof(char) * 300);
// initialize starting packet and echo_packet
EchoPacket init_packet, echo_packet;
init_packet.type = ECHO;
init_packet.seq = sequence;

// server socket address
struct sockaddr_in server_addr;
// address of incoming packets
struct sockaddr_in echo_pac_addr;
// client socket address
struct sockaddr_in client_addr;
socklen_t client_len;

// socket both for sending and receiving
int sk = socket(AF_INET, SOCK_DGRAM, 0);
if (sk < 0) {
    perror("echo_client: socket error\n");
    exit(1);
}

server_addr.sin_family = AF_INET;
server_addr.sin_addr.s_addr = INADDR_ANY;
server_addr.sin_port = htons(port);

// binding was done in client_bind

// get server host address
struct hostent* server_name;
struct hostent server_name_copy;
int server_fd;
server_name = gethostbyname(address);
if (server_name == NULL) {
    perror("echo_client: invalid server address\n");
    exit(1);
}

memcpy(&server_name_copy, server_name, sizeof(server_name_copy));
memcpy(&server_fd, server_name_copy.h_addr_list[0], sizeof(server_fd));

// send echo_pac_addr to be server address
echo_pac_addr.sin_family = AF_INET;
echo_pac_addr.sin_addr.s_addr = server_fd;
echo_pac_addr.sin_port = htons(port);

fd_set mask;
fd_set read_mask;
int num;

FD_ZERO(&mask);
FD_SET(sk, &mask);

// timer for retransmission
struct timeval timeout;

// record starting time of transfer
struct timeval start_time;

// send init packet to rcv
sendto(sk, (EchoPacket *)&init_packet, sizeof(init_packet), 0,
(struct sockaddr*)&echo_pac_addr, sizeof(echo_pac_addr));
printf("echo packet is sent");
// record starting time
gettimeofday(&start_time, NULL);

for (;;) {
     read_mask = mask;
     timeout.tv_sec = INTERACTIVE_TIMEOUT_SEC;
     timeout.tv_usec = INTERACTIVE_TIMEOUT_USEC;
     num = select(FD_SETSIZE, &read_mask, NULL, NULL, &timeout);
    if (num > 0) {
        if (FD_ISSET(sk, &read_mask)) {
            client_len = sizeof(client_addr);
            recvfrom(sk, &echo_packet, sizeof(echo_packet), 0,
                                 (struct sockaddr*)&client_addr, &client_len);
            int server_ip = client_addr.sin_addr.s_addr;

            // record current time and report
            struct timeval current_time;
            gettimeofday(&current_time, NULL);
            struct timeval diff_time = diffTime(current_time, start_time);
            double msec = diff_time.tv_sec * 1000 + ((double) diff_time.tv_usec) / 1000;
            sprintf(out, "Report: RTT of a UDP packet is %f ms with sequence number %d\n", msec, echo_packet.seq);

            return out; // terminate after report
        }
    } else {
        sprintf(out, "Haven't heard response for over %d seconds, timeout!\n", INTERACTIVE_TIMEOUT_SEC);
        return out;
    }
}
return 0;
}