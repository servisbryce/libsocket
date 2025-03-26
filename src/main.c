#include "../include/context.h"
#include "../include/socket.h"
#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>

void handle(socket_dispatch_vargs_t hi) {

    SSL *ssl = SSL_new(hi.socket_context->tls_context->openssl_context);
    SSL_set_fd(ssl, hi.client_sock_descriptor);
    if (SSL_accept(ssl) <= 0) {

    }
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(hi.client_sock_descriptor);
    return;

}

void main() {

    socket_context_t *context = NULL;
    printf("%d\n", create_socket_context("127.0.0.1", 1025, true, &context));
    printf("%d\n", create_tls_context(context, NULL, "cert.pem", "key.pem"));
    printf("%d\n", create_socket(context));
    socket_dispatch(context, handle);

}