#include "bytes_steam.h"
// send bytes stream with the given speed
// speed n is in Mbits per second
void send_bytes_stream(int sk, struct sockaddr_in send_addr, double n)
{ 
    // n Mbits = 125000*n bytes
    // 1 / (125000*n) second = 8/n microseconds 
    // we send a byte per 8/n microsecond 
    // PACKET_SIZE bytes per 8*PACKET_SIZE/n microsend

    fd_set mask;
    fd_set read_mask;
    int num;

    FD_ZERO(&mask);
    FD_SET(sk, &mask);

    struct timeval timeout;
    int seq = 0;
    for (;;) { 
        read_mask = mask;
        // timeout limit
        timeout.tv_sec = 0;
        timeout.tv_usec = 8 * PACKET_SIZE / n;

        num = select(FD_SETSIZE, &read_mask, NULL, NULL, &timeout);

        if (num > 0) {
            // if (FD_ISSET(sk, &read_mask)) {
            // }
        } else { //timeout 
            timing.type = 0;
            timing.seq = seq;
            sendto(sk, &timing, sizeof(timing), 0, 
                (struct sockaddr*)&send_addr, sizeof(send_addr));
            seq++;
        }
    }
}