#ifndef BANDWIDTH_DATA_GENERATOR_H
#define BANDWIDTH_DATA_GENERATOR_H
#include "net_include.h"
#include "bandwidth_utils.h"

#define DATA_SIZE (PACKET_SIZE - sizeof(packet_header))

/**
 * Used to pass parameters through pthread_create.
 */
struct data_generator_args {
    bool android; 
};

/**
 * Calls start_generator with given arguments. Used in pthread_create.
 */
void * start_generator_pthread(void * args);

/**
 * starts data generator
 */
void start_generator(bool android);

/**
 * Breaks out data generator select loop and stops data generator thread
 */
void stop_data_generator_thread();

#endif
