#ifndef INTERACTIVE_NET_INCLUDE_H
#define INTERACTIVE_NET_INCLUDE_H

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
#include <unistd.h>
#include <time.h>
#include <float.h>

#include <errno.h>

#include <sys/time.h>

#define PORT 9008
#define BUFF_SIZE 1000
#define INTERACTIVE_TIMEOUT_SEC 3
#define INTERACTIVE_TIMEOUT_USEC 0
#define PACKET_SIZE 1400
#define NUM_SEND 10
#define NAME_LENGTH 100

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
