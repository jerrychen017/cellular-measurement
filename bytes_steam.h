#ifndef BYTES_STREAM_H
#define BYTES_STREAM_H
#include "include/net_include.h"
#include "utils.h"

void send_bytes_stream(int sk, struct sockaddr_in send_addr, double n);

#endif 