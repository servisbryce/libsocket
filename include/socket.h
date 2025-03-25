#ifndef SOCKADDR_H
#define SOCKADDR_H

#include "context.h"

typedef struct socket_dispatch_vargs {

    socket_context_t *socket_context;
    int client_sock_descriptor;

} socket_dispatch_vargs_t;

#endif