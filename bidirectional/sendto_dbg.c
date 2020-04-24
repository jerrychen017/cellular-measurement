#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>

#include "net_include.h"
#include "bandwidth_utils.h"
//static struct timeval sendTimes[BURST_SIZE];
static int mySeq = 0;

int sendto_dbg(int s, void *buf, int len, int flags,
	       const struct sockaddr *to, int tolen )
{
    /*
    gettimeofday(&sendTimes[mySeq % BURST_SIZE], NULL);

    if (mySeq >= BURST_SIZE - 1) {
        double rate = interval_to_speed(diffTime(sendTimes[mySeq % BURST_SIZE],
                sendTimes[(mySeq + 1) % BURST_SIZE]), BURST_SIZE - 1);
//        printf("Actual speed %.4f\n", rate);
    }
    mySeq++;
    */

    return sendto( s, buf, len, flags, to, tolen );
}
