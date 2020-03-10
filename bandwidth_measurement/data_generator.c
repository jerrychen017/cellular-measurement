#include "net_include.h"

/**
Data generator
*/
int main(int argc, char *argv[])
{
    // args error checking
    if (argc != 2)
    {
        printf("data_generator usage: data_generator <speed>\n");
        exit(1);
    }
    int n = atoi(argv[1]);

    // n Mbits = 125000*n bytes
    // 1 / (125000*n) second = 8/n microseconds
    // we send a byte per 8/n microsecond
    // PACKET_SIZE bytes per 8*PACKET_SIZE/n microsend

    int s,
        t, len;
    struct sockaddr_un controller;
    char str[100];

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    printf("Trying to connect...\n");

    controller.sun_family = AF_UNIX;
    strcpy(controller.sun_path, SOCK_PATH);
    len = strlen(controller.sun_path) + sizeof(controller.sun_family);
    if (connect(s, (struct sockaddr *)&controller, len) == -1)
    {
        perror("connect");
        exit(1);
    }

    printf("Connected.\n");

    fd_set mask;
    fd_set read_mask;
    int num;

    FD_ZERO(&mask);
    FD_SET(s, &mask);

    struct timeval timeout;
    int seq = 0;

    bool start = false;
    char data[1400];
    for (;;)
    {
        read_mask = mask;
        // timeout limit
        timeout.tv_sec = 0;
        timeout.tv_usec = 8 * PACKET_SIZE / n;

        num = select(FD_SETSIZE, &read_mask, NULL, NULL, &timeout);

        int local_packet_type;

        if (num > 0)
        {
            if (FD_ISSET(s, &read_mask))
            {
                recv(s, &local_packet_type, sizeof(int), 0);
                if (local_packet_type == LOCAL_START)
                {
                    start = true;
                }
                else if (local_packet_type == LOCAL_CONTROL)
                {
                    // adjust speed n
                    printf("received LOCAL_CONTROL message\n");
                }
            }
        }
        else
        { //timeout
            if (!start)
            {
                printf("Waiting to start\n");
            }
            else
            {
                sendto(s, &data, sizeof(data), 0,
                       (struct sockaddr *)&controller, sizeof(controller));
                seq++;
            }
        }
    }

    return 0;
}
