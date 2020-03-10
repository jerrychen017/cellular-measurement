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

#include <errno.h>

#include <sys/time.h>

#define PORT 9008
#define BUFF_SIZE 1000
#define TIMEOUT_SEC 5
#define TIMEOUT_USEC 0
#define PACKET_SIZE 1400
#define NUM_SEND 100
#define NUM_REPROT 5 // number of report packets the server sends to client

#define SOCK_PATH "/tmp/unix_socket"

enum NetworkPacketType
{
    NETWORK_REPORT,
    NETWORK_START,
    NETWORK_PING,
    NETWORK_PONG,
    NETWORK_DATA
};

enum LocalPacketType
{
    LOCAL_START,
    LOCAL_CONTROL
};