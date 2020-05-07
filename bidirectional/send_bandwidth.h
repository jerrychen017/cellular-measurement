#ifndef SEND_BANDWIDTH
#define SEND_BANDWIDTH
#include "net_include.h"
struct send_bandwidth_args
{
    struct sockaddr_in addr;
    int sk;
    bool android;
    struct parameters params;
};

/**
 * Calls send_bandwidth() with given arguments. Used to match signatures for pthread_create.
 */
void *send_bandwidth_pthread(void *arg);

/**
 * Starts data generator on a new thread and runs controller.
 */
void send_bandwidth(struct sockaddr_in addr, int sk, bool android, struct parameters params);

/**
 * Sends data stream to the receiver over TCP on the client side
 */
void client_send_bandwidth_tcp(int sk);

/**
 * Calls server_send_bandwidth_tcp() with given arguments. Used to match signatures for pthread_create.
 */
void * server_send_bandwidth_tcp_pthread(void * args);

/**
 * Sends data stream to the receiver over TCP on the server side
 */
void server_send_bandwidth_tcp(int sk);

/**
 * Breaks out client_send_bandwidth_tcp() select for loop and stops the thread
 */
void stop_tcp_send_thread();

#endif
