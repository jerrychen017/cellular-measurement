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

#define SOCK_PATH "/tmp/unix_socket"
// #define SOCK_PATH "my.local.socket.address" termux

// Constants for interactive application
#define PORT 9008
#define BUFF_SIZE 1000
#define INTERACTIVE_TIMEOUT_SEC 3
#define INTERACTIVE_TIMEOUT_USEC 0
#define PACKET_SIZE 1400
#define NUM_SEND 10
#define NAME_LENGTH 100

enum NetworkPacketType
{
    NETWORK_DATA,
    NETWORK_BURST,  // data packet that is sent at a higher rate to probe for bandwidth
    NETWORK_REPORT,
    NETWORK_START,
    NETWORK_START_ACK
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

typedef struct packet_header_
{
    int type;
    int seq_num;
    double rate;
} packet_header;

typedef struct data_packet_
{
    packet_header hdr;
    char data[PACKET_SIZE - sizeof(packet_header)];
} data_packet;

enum PacketType
{
    TIMING,      // 0
    REPORT,      // 1
    ECHO,        // 2
    INTERACTIVE, // 3
    CONNECT      // 4
};

enum ConnnectError
{
    NO_ERROR,
    ID_NOT_FOUND,
    MAX_USERS_REACHED
};

typedef struct Packet
{
    int type;
    char buffer[PACKET_SIZE + 8];
} Packet;

typedef struct TimingPacket
{
    int type;
    int seq;
    char buffer[PACKET_SIZE];
} TimingPacket;

typedef struct ReportPacket
{
    int type;
    int seq;
    float throughput;
} ReportPacket;

typedef struct EchoPacket
{
    int type;
    int seq;
    float x;
    float y;
    int id;
    char name[NAME_LENGTH];
} EchoPacket;

typedef struct InteractivePacket
{
    int type;
    int seq;
    float x;
    float y;
    int id;
    char name[NAME_LENGTH];
    struct timeval send_time;
    double latency;
} InteractivePacket;

typedef struct ConnectPacket
{
    int type;
    int id;
    char name[NAME_LENGTH];
    int error;
} ConnectPacket;

typedef struct User
{
    int id;
    char name[NAME_LENGTH];
    bool address_set;
    struct sockaddr_in socket_addr;
} User;

#endif
