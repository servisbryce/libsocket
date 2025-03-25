#include "../include/context.h"
#include "../include/socket.h"
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

void handle(socket_dispatch_vargs_t hi) {

    printf("hi\n");
    return;

}

void main() {

    socket_context_t *context = NULL;
    printf("%d\n", create_socket_context("127.0.0.1", 1025, true, &context));
    printf("%d\n", create_socket(context));
    perror("hi");
    socket_dispatch(context, handle);

}