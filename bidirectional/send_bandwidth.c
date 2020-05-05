#include "send_bandwidth.h"
#include "controller.h"
#include "data_generator.h"
#include "net_utils.h"

void *send_bandwidth_pthread(void *args)
{
    struct send_bandwidth_args *recv_args = (struct send_bandwidth_args *)args;
    send_bandwidth(recv_args->addr, recv_args->sk, recv_args->android, recv_args->params);

    return NULL;
}

void send_bandwidth(struct sockaddr_in addr, int sk, bool android, struct parameters params)
{
    struct data_generator_args send_args;
    pthread_t tid; // thread id for data generator

    send_args.android = android;
    pthread_create(&tid, NULL, &start_generator_pthread, (void *)&send_args);

    start_controller(android, addr, sk, params);
    pthread_join(tid, NULL);
    return;
}

void send_bandwidth_tcp(int sk, bool android){
    char buffer[PACKET_SIZE];
    for(;;) {
        send(sk, buffer, PACKET_SIZE, 0);
        printf("sent tcp ");
    }
}

