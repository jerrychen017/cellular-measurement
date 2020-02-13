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

#include <errno.h>

#include <sys/time.h>

#define PORT 9008
#define BUFF_SIZE 1000
#define TIMEOUT_SEC 2
#define TIMEOUT_USEC 0
