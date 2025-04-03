#ifndef LIBSOCKET_H_
#define LIBSOCKET_H_

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <sys/socket.h>

typedef struct tls_server_context {

    struct sockaddr *sockaddr;
    SSL_CTX *ssl_context;
    size_t *sockaddr_length;
    size_t threads;
    void (*routine)(void *routine_vargs);
    int socket;

} tls_server_context_t;

typedef struct client_context {



} client_context_t;

#endif