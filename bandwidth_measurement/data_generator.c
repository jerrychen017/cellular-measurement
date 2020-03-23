#include "net_include.h"
#include "utils.h"

#define DATA_SIZE (PACKET_SIZE - sizeof(data_header))

int setup_socket();

/**
    Data generator
*/
int main(int argc, char *argv[])
{
    // args error checking
    if (argc != 1)
    {
        printf("data_generator usage: data_generator\n");
        exit(1);
    }

    int s = setup_socket();

    // Select loop stuff
    fd_set mask;
    fd_set read_mask;

    struct timeval timeout;
    int num;

    FD_ZERO(&mask);
    FD_SET(s, &mask);

    int seq = 0;
    bool start = false;
    typed_packet pkt;
    char buffer[DATA_SIZE];
    double speed = 1.0;

    // Timeout before start is TIMEOUT_SEC

    for (;;)
    {
        read_mask = mask;

        // timeout limit
        if (!start) {
            timeout.tv_sec = TIMEOUT_SEC;
            timeout.tv_usec = TIMEOUT_USEC;
        }
        else {
            timeout = speed_to_interval(speed); 
        }

        num = select(FD_SETSIZE, &read_mask, NULL, NULL, &timeout);

        if (num > 0) {
            if (FD_ISSET(s, &read_mask))
            {
                int len = recv(s, &pkt, sizeof(typed_packet), 0);
                if (len <= 0) {
                    printf("controller disconnected, exiting\n");
                    exit(0);
                }
                if (pkt.type == LOCAL_START)
                {
                    printf("Starting data stream\n");
                    start = true;
                }
                else if (pkt.type == LOCAL_CONTROL)
                {
                    // adjust speed n
                    printf("received LOCAL_CONTROL message\n");
                    speed = pkt.rate;
                    printf("SPEED: %f\n", speed);
                    if (speed <= 0) {
                        perror("negative speed &\n");
                        exit(1);
                    }
                    if (speed > MAX_SPEED) {
                        perror("exceed max speed\n");
                        exit(1);
                    }
                }
            }
        }
        else {
            //timeout
            if (!start)
            {
                printf("Waiting to start\n");
            }
            else
            {
                send(s, buffer, DATA_SIZE , 0);
                seq++;
            }
        }

    }

    return 0;
}


int setup_socket()
{
    int s;
    int len;

    struct sockaddr_un controller;

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror("socket error\n");
        exit(1);
    }

    printf("Trying to connect...\n");

    controller.sun_family = AF_UNIX;
    strcpy(controller.sun_path, SOCK_PATH);
    len = strlen(controller.sun_path) + sizeof(controller.sun_family);
    if (connect(s, (struct sockaddr *)&controller, len) == -1)
    {
        perror("connect error\n");
        exit(1);
    }

    printf("Connected.\n");
    return s;
}
