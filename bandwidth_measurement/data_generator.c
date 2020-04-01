#include "data_generator.h"

int start_generator(bool android) {
    int s = setup_socket(android);

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
		            // expectedTimeout = diffTime(speed_to_interval(speed), timeout);
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
                    struct timeval baseTimeout = speed_to_interval(speed); 
                    struct timeval tmExtra = diffTime(diffTime(tmNow, tmPrev), expectedTimeout);
                    while (gtTime(tmExtra, baseTimeout)) {
                        seq++;
                        send(s, buffer, DATA_SIZE, 0);
                        tmExtra = diffTime(tmExtra, baseTimeout);
                    }

                    expectedTimeout = diffTime(baseTimeout, tmExtra);
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


int setup_socket(bool android)
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

    if (android) {
        memset(&controller, 0, sizeof(controller)); // fix android connect error 
        controller.sun_family = AF_UNIX;
        const char name[] = "\0my.local.socket.address"; // fix android connect error 
        memcpy(controller.sun_path, name, sizeof(name) - 1); // fix android connect error 
        len = strlen(controller.sun_path) + sizeof(name); // fix android connect error 
        controller.sun_path[0] = 0; // fix android connect error 
    } else {
        controller.sun_family = AF_UNIX;
        strcpy(controller.sun_path, SOCK_PATH);
        len = strlen(controller.sun_path) + sizeof(controller.sun_family);
    }

    if (connect(s, (struct sockaddr *)&controller, len) == -1)
    {
        perror("connect error\n");
        exit(1);
    }

    printf("Connected.\n");
    return s;
}
