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
#define BURST_SIZE 10 // number of packets in increased speed burst
#define BURST_FACTOR 2
#define INTERVAL_SIZE 100 // one burst per INTERVAL_SIZE packets, should make this a multiple of BURST_SIZE
#define MIN_SPEED 0.1   // 100 Kbps
#define MAX_SPEED 10 // 10 Mbps
#define START_SPEED 1.0 // 1.0 Mbps

#define SOCK_PATH "my.local.socket.address"
enum NetworkPacketType
{
    NETWORK_DATA,
    NETWORK_BURST,  // data packet that is sent at a higher rate to probe for bandwidth
    NETWORK_REPORT,
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
    char data[PACKET_SIZE - sizeof(int)- sizeof(double)];
} typed_packet;

typedef struct data_header_
{
    int type;
    int seq_num;
    double rate;
} data_header;

typedef struct data_packet_
{
    data_header hdr;
    char data[PACKET_SIZE - sizeof(data_header)];
} data_packet;
#endif