#include "net_utils.h"

struct sockaddr_un get_datagen_addr(bool android, socklen_t *len)
{
    struct sockaddr_un addr;

    if (android) {
        const char name[] = ANDROID_SOCK_DATAGEN; // fix android socket error
        memset(&addr, 0, sizeof(addr)); // fix android socket error
        addr.sun_family = AF_UNIX;
        memcpy(addr.sun_path, name, sizeof(name) - 1); // fix android socket error
        *len = strlen(addr.sun_path) + sizeof(name); // fix android socket error
        addr.sun_path[0] = 0; // fix android socket error
    } else {
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, SOCK_DATAGEN);
        *len = strlen(addr.sun_path) + sizeof(addr.sun_family);
    }

    return addr;
}

struct sockaddr_un get_controller_addr(bool android, socklen_t *len)
{
    struct sockaddr_un addr;

    if (android) {
        const char name[] = ANDROID_SOCK_CONTROLLER; // fix android socket error
        memset(&addr, 0, sizeof(addr)); // fix android socket error
        addr.sun_family = AF_UNIX;
        memcpy(addr.sun_path, name, sizeof(name) - 1); // fix android socket error
        *len = strlen(addr.sun_path) + sizeof(name); // fix android socket error
        addr.sun_path[0] = 0; // fix android socket error
    } else {
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, SOCK_CONTROLER);
        *len = strlen(addr.sun_path) + sizeof(addr.sun_family);
    }

    return addr;
}

/* Setup unix socket, binds/connects to addr */
int setup_unix_socket(struct sockaddr_un *send_addr, struct sockaddr_un *recv_addr,
        socklen_t send_len, socklen_t recv_len)
{
    int s;
    struct sockaddr_un controller;

    if ((s = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
        perror("socket error\n");
        exit(1);
    }

    if (bind(s, (struct sockaddr *) &recv_addr, send_len) == -1) {
        perror("bind");
        exit(1);
    }

    if (connect(s, (struct sockaddr *) &send_addr, recv_len) == -1) {
        perror("connect");
        exit(1);
    }

    return s;
}

struct sockaddr_in addrbyname(const char *hostname, int port)
{
    int host_num;
    struct hostent h_ent, *p_h_ent;

    struct sockaddr_in addr;

    p_h_ent = gethostbyname(hostname);
    if (p_h_ent == NULL) {
        printf("gethostbyname error.\n");
        exit(1);
    }

    memcpy( &h_ent, p_h_ent, sizeof(h_ent));
    memcpy( &host_num, h_ent.h_addr_list[0], sizeof(host_num) );

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = host_num;
    addr.sin_port = htons(port);

    return addr;
}

