#include "send_bandwidth.h"
#include "controller.h"
#include "data_generator.h"

void send_bandwidth(void * args) {
    struct send_bandwidth_args * recv_args = (struct send_bandwidth_args *) args;
    int ret; 
    int tid; // thread id for data generator
    struct data_generator_args send_args; 
    ret = start_generator(false); 
    // port
    int port = recv_args->port;
    // address
    struct sockaddr_in addr = recv_args->addr;
    char * dummy_address; 
    int dummy_port; 
    int ret = start_controller(false, dummy_address, dummy_port, addr); 
    
    send_args.android = false; 
    pthread_create(&tid, NULL, &start_generator, (void *)&send_args);
    return; 
} 