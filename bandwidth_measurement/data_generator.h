#ifndef BANDWIDTH_DATA_GENERATOR_H
#define BANDWIDTH_DATA_GENERATOR_H
#include "net_include.h"
#include "bandwidth_utils.h"

#define DATA_SIZE (PACKET_SIZE - sizeof(data_header))

int setup_socket();

/**
 * start_generator is called in android ndk to run data generator 
 */
int start_generator();

#endif
