#ifndef SENDTO_DBG
#define SENDTO_DBG

#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>

int 	sendto_dbg(int s, const char *buf, int len, int flags,
		   const struct sockaddr *to, int tolen);

void 	sendto_dbg_init(int percent);

#endif 