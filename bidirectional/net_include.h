#ifndef BANDWIDTH_NET_INCLUDE_H
#define BANDWIDTH_NET_INCLUDE_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <float.h>
#include <math.h>
#include <errno.h>
#include <sys/time.h>

#define TIMEOUT_SEC 1
#define TIMEOUT_USEC 0
#define PACKET_SIZE 1400

// controller constants
/**
 * Parameters we used in our protocol:
 * BURST_SIZE 10 // number of packets in increased speed burst
 * BURST_FACTOR 2
 * INTERVAL_SIZE 500 // one burst per INTERVAL_SIZE packets, should make this a multiple of BURST_SIZE
 * INTERVAL_TIME 1 // overrides and uses INTERVAL_TIME instead of INTERVAL_SIZE if it's not 0
 * MIN_SPEED 0.1   // 100 Kbps
 * MAX_SPEED 10 // 10 Mbps
 * START_SPEED 1.0 // 1.0 Mbps
 * GAMMA 10 // number of packets tolerated with measured rate below threshold
 * INSTANT_BURST 0
 * THRESHOLD 0.95
 * ALPHA 0.1
 * PRED_MODE 1 // 0 for EWMA and 1 for running average
 * USE_TCP 0 // use TCP if 1, use UDP if 0
 */

/**
 * UNIX socket for running on computers
 */
#define SOCK_CONTROLLER "/tmp/controller"
#define SOCK_DATAGEN "/tmp/datagenerator"
/**
 * UNIX socket for running on Android
 */
#define ANDROID_SOCK_DATAGEN "\0local.datagenerator"
#define ANDROID_SOCK_CONTROLLER "\0local.controller"

// Constants for interactive application
#define BUFF_SIZE 1000
#define INTERACTIVE_TIMEOUT_SEC 3
#define INTERACTIVE_TIMEOUT_USEC 0
#define PACKET_SIZE 1400
#define NAME_LENGTH 100

// SERVER_INTERACTIVE_PORT 4578
//#define SERVER_RECEIVE_PORT 4577
//#define SERVER_SEND_PORT 4576
//#define CLIENT_SEND_PORT 4577
//#define CLIENT_RECEIVE_PORT 4576

// send upload/download rates to the Android app every 200 ms
#define FEEDBACK_FREQ_USEC 200000
// printout to stdout every 1 s
#define PRINTOUT_FREQ_USEC 1000000

struct parameters
{
    int burst_size;    // number of packets in increased speed burst
    int interval_size; // one burst per INTERVAL_SIZE packets, should make this a multiple of BURST_SIZE
    int grace_period;
    int instant_burst;
    int pred_mode;
    int use_tcp; 
    double alpha;
    double threshold;
    double interval_time;
    double min_speed;
    double max_speed;
    double start_speed;
};

enum NetworkPacketType
{
    NETWORK_DATA,
    NETWORK_BURST, // data packet that is sent at a higher rate to probe for bandwidth
    NETWORK_REPORT,
    NETWORK_BURST_REPORT,
    NETWORK_START,
    NETWORK_START_ACK,
    NETWORK_STOP,
    NETWORK_BUSY,
    NETWORK_ECHO
};

enum LocalPacketType
{
    LOCAL_START,
    LOCAL_CONTROL
};

typedef struct typed_packet_
{
    int type;
    double rate;
} typed_packet;

typedef struct start_packet_
{
    int type;
    struct parameters params;
} start_packet;

#pragma pack(1)
typedef struct packet_header_
{
    int type;
    int seq_num;
    int burst_start;
    double rate;
} packet_header;
#pragma pack(0)

#pragma pack(1)
typedef struct data_packet_
{
    packet_header hdr;
    char data[PACKET_SIZE - sizeof(packet_header)];
} data_packet;
#pragma pack(0)

#endif
