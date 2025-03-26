#include "../include/context.h"
#include "../include/socket.h"
#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>

void handle(socket_dispatch_vargs_t hi) {

    close(hi.client_sock_descriptor);
    return;

}

void main() {

    socket_context_t *context = NULL;
    printf("%d\n", create_socket_context("127.0.0.1", 1025, true, &context));
    struct sockaddr_in *bad = (struct sockaddr_in*) context->sockaddr;

    printf("%d\n", bad->sin_port);
    printf("%d\n", create_socket(context));
    perror("hi");
    socket_dispatch(context, handle);

}