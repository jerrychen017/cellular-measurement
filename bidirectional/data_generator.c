#include "data_generator.h"
#include "net_utils.h"

static bool stop_thread = false;
void *start_generator_pthread(void *args)
{
    struct data_generator_args *received_args = (struct data_generator_args *)args;
    bool android = received_args->android;
    start_generator(android);
    return NULL;
}
void start_generator(bool android)
{

    stop_thread = false;

    socklen_t my_len, send_len;
    struct sockaddr_un my_addr = get_datagen_addr(android, &my_len);
    struct sockaddr_un send_addr = get_controller_addr(android, &send_len);
    int s = setup_unix_socket(my_addr, my_len);

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
    memset(buffer, 0, DATA_SIZE);

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
        if (stop_thread)
        {
            close(s);
            return;
        }

        read_mask = mask;

        num = select(FD_SETSIZE, &read_mask, NULL, NULL, &timeout);

        if (num > 0)
        {
            if (FD_ISSET(s, &read_mask))
            {
                int len = recv(s, &pkt, sizeof(typed_packet), 0);
                if (len <= 0)
                {
                    printf("controller disconnected, exiting\n");
                    return;
                }
                if (pkt.type == LOCAL_START)
                {
                    printf("Starting data stream\n");

                    sendto(s, &pkt, len, 0,
                           (struct sockaddr *)&send_addr, send_len);

                    start = true;
                    expectedTimeout = speed_to_interval(speed);
                }
                else if (pkt.type == LOCAL_CONTROL)
                {
                    // adjust speed
                    speed = pkt.rate;
                    if (speed <= 0)
                    {
                        perror("negative speed &\n");
                        exit(1);
                    }
                    // if (speed > MAX_SPEED) {
                    //     perror("exceed max speed\n");
                    //     exit(1);
                    // }
                    // expectedTimeout = diffTime(speed_to_interval(speed), timeout);
                }
            }
        }
        else
        {
            //timeout
            if (!start)
            {
                printf("Waiting to start\n");
            }
            else
            {
                sendto(s, buffer, DATA_SIZE, 0,
                       (struct sockaddr *)&send_addr, send_len);

                gettimeofday(&tmNow, NULL);
                if (seq != 0)
                {
                    // Account for kernel returning from select loop late
                    struct timeval baseTimeout = speed_to_interval(speed);
                    struct timeval tmExtra = diffTime(diffTime(tmNow, tmPrev), expectedTimeout);
                    while (gtTime(tmExtra, baseTimeout))
                    {
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
        if (!start)
        {
            timeout.tv_sec = TIMEOUT_SEC;
            timeout.tv_usec = TIMEOUT_USEC;
        }
        else
        {
            timeout = expectedTimeout;
        }
    }
    return;
}

void stop_data_generator_thread()
{
    stop_thread = true;
}
