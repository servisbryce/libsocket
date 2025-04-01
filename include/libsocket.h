#ifndef LIBSOCKET_H_
#define LIBSOCKET_H_

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

typedef struct tls_server_context {

    SSL_CTX *ssl_context;
    int socket;
    SSL *ssl:
    BIO *bio;

} tls_server_context_t;

typedef struct client_context {



} client_context_t;

#endif