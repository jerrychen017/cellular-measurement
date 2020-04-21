#ifndef BANDWIDTH_DATA_GENERATOR_H
#define BANDWIDTH_DATA_GENERATOR_H
#include "net_include.h"
#include "bandwidth_utils.h"

#define DATA_SIZE (PACKET_SIZE - sizeof(packet_header))

/**
 * start_generator is called in android ndk to run data generator 
 */
void start_generator(bool android);
void stop_data_generator_thread();

#endif
