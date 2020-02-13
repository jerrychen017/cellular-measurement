#include "include/net_include.h"
#include "utils.h"

int main(int argc, char* argv[]) {

// args error checking
if (argc != 3) {
    printf("echo_client usage: echo_client <host_address> <port>\n");
    exit(1);
}

int port = atoi(argv[2]);

// initialize starting packet and echo_packet
char init_packet[BUFF_SIZE]; 
char echo_packet[BUFF_SIZE];

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

// bind socket with port
if (bind(sk, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    perror("echo_client: bind error\n");
    exit(1);
}

// get server host address
struct hostent* server_name;
struct hostent server_name_copy;
int server_fd;
server_name = gethostbyname(argv[1]);
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
struct timeval last_time;

// send filename to rcv
sendto(sk, (char*)&init_packet, sizeof(BUFF_SIZE), 0, 
(struct sockaddr*)&echo_pac_addr, sizeof(echo_pac_addr));
// record starting time
gettimeofday(&start_time, NULL);                        
gettimeofday(&last_time, NULL);

printf("Try to establish connection with (%d.%d.%d.%d)\n",
                 (htonl(echo_pac_addr.sin_addr.s_addr) & 0xff000000) >> 24,
                 (htonl(echo_pac_addr.sin_addr.s_addr) & 0x00ff0000) >> 16,
                 (htonl(echo_pac_addr.sin_addr.s_addr) & 0x0000ff00) >> 8,
                 (htonl(echo_pac_addr.sin_addr.s_addr) & 0x000000ff));


for (;;) { 
     read_mask = mask;
     timeout.tv_sec = TIMEOUT_SEC;
     timeout.tv_usec = TIMEOUT_USEC;
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
            struct timeval diff_time = diffTime(current_time, last_time);
            double msec = diff_time.tv_sec * 1000 + ((double) diff_time.tv_usec) / 1000;
            printf("Report: RTT of a UDP packet is %f ms\n", msec);
            last_time = current_time;

            return 0; // terminate after report
        }
    } else {
        printf("Haven't heard response for over %d seconds, timeout!\n", TIMEOUT_SEC);
        fflush(0);
    }
}
return 0;
}