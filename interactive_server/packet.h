#ifndef PACKET_H
#define PACKET_H

#define TAG_START 0
#define TAG_DATA 1
#define TAG_ACK 2
#define TAG_NACK 3
#define TAG_END 4
#define TAG_COUNTER 5 // message that contains the last counter this machine delivered 

/**
 * Tuning hyperparameters
 */
#define TABLE_SIZE 400
#define NUM_TO_SEND 10 
#define TIMEOUT_SEC 0
#define TIMEOUT_USEC 2000
#define RETRANSMIT_INTERVAL_SEC 0
#define RETRANSMIT_INTERVAL_USEC 5000
#define NUM_EXIT_SIGNALS 10 
#define CREATED_PACKETS_SIZE 40 
#define ACK_GAP 40 

struct packet {
    unsigned int tag;
    unsigned int counter;
    unsigned int machine_index; // starts from 1
    unsigned int packet_index; // starts from 1
    unsigned int random_data;
    /*
    if tag == TAG_ACK, first <num_machine> integers represent corresponding ack value
    if tag == TAG_NACK, first <num_machine> integers represent corresponding nack packet_index. 
    if the value is -1, then there's no nack for that machine.
    if tag == TAG_COUNTER, first <num_machine> integers represent last counters for each machine.
    */
    int payload[1400 / sizeof(int)];
};

#endif