#ifndef CLIENT_H
#define CLIENT_H
#include <stdbool.h>
#include "net_include.h"

/**
 * CLI start client function. Sends START packet to the server with all parameters and wait for ACKs.
 * Once connected with the server, run send_bandwidth() in a new thread and call receive_bandwidth()
 * @param address server address
 * @param params parameters
 */
void start_client(const char * address, struct parameters params);

#endif
