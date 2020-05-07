#ifndef RECEIVE_BANDWIDTH
#define RECEIVE_BANDWIDTH

#include "net_include.h"
#include "bandwidth_utils.h"

struct recv_bandwidth_args
{
    int sk;
    struct sockaddr_in expected_addr;
    struct parameters params;
    bool android;
};

/**
 * Breaks out receive_bandwidth() select for loop and stops the thread
 */
void stop_receiving_thread();

/**
 * Calls receive_bandwidth() with given arguments. Used to match signatures for pthread_create.
 */
void stop_tcp_recv_thread();

/**
 * Receives data packets from the controller (sender) and sends reports packets to the sender.
 */
void receive_bandwidth(int sk, struct sockaddr_in expected_addr, struct parameters params, bool android);

/**
 * Receives bandwidth data stream over TCP on the server side and send back report packets to the controller
 */
void server_receive_bandwidth_tcp(int sk);

/**
 * Receives bandwidth data stream over TCP on the client side and send back report packets to the controller
 */
void client_receive_bandwidth_tcp(int sk);

/**
 * Breaks out TCP receive bandwidth select loop and stops the thread
 */
void *receive_bandwidth_pthread(void *);

#endif
