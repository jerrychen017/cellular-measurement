#ifndef CLIENT_H
#define CLIENT_H
#include <stdbool.h>
#include "net_include.h"
void start_client(const char * address, int pred_mode, bool android, struct parameters params);

#endif
