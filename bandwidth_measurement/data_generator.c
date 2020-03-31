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
    struct timeval expectedTimeout;
    int num;

    int seq = 0;
    bool start = false;
    typed_packet pkt;
    char buffer[DATA_SIZE];

    struct timeval tmPrev;
    struct timeval tmNow;
    double speed = 1.0;

    FD_ZERO(&mask);
    FD_SET(s, &mask);

    // initial timeout is 1 sec
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = TIMEOUT_USEC;

    for (;;)
    {
        read_mask = mask;

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
		    expectedTimeout = speed_to_interval(speed);
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
		    expectedTimeout = diffTime(speed_to_interval(speed), timeout);
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

                gettimeofday(&tmNow, NULL);
                if (seq != 0) {
                    // Account for kernel returning from select loop late
                    struct timeval actualTimeout = diffTime(tmNow, tmPrev);
                    struct timeval baseTimeout = speed_to_interval(speed); 
                    expectedTimeout = diffTime(baseTimeout, diffTime(actualTimeout, expectedTimeout));
                    /*printf("timeout base %.4f, expected %.4f ms\n", 
                        baseTimeout.tv_usec / 1000.0,
                        expectedTimeout.tv_usec / 1000.0);*/
                }
                tmPrev = tmNow;
                seq++;
            }
        }

        // timeout limit
        if (!start) {
            timeout.tv_sec = TIMEOUT_SEC;
            timeout.tv_usec = TIMEOUT_USEC;
        } else {
            timeout = expectedTimeout;
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
