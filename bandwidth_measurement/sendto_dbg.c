#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>

void *memcpy(void *dest, const void *src, size_t n);

int sendto_dbg(int s, void *buf, int len, int flags,
	       const struct sockaddr *to, int tolen )
{
    return sendto( s, buf, len, flags, to, tolen );
}
