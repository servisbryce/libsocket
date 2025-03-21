#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <stddef.h>

/* This context is optional. If this context isn't set by the user, then all traffic will be unencrypted. */
typedef struct tls_context {

    /* These are optional. If they aren't set by the user, then a cache will be automatically created with sane defaults. */
    size_t tls_cache_length;
    int tls_cache_expiry;

    /* You must set one of these variables whenever you're creating a server. If you don't, an error will be declared. For security purposes, we recommend using a chain certificate when possible. */
    char *chained_certificate_path;
    char *certificate_path;

    /* This is required. If you don't set this and you're creating a server, then an error will be declared. */
    char *private_key_path;

    /* These are optional. If they aren't set by the user, then they'll be automatically created with sane defaults. */
    long tls_options;
    int minimum_tls_version;
    int maximum_tls_version;

} tls_context_t;

/* This context is required to use this library. */
typedef struct socket_context {

    /* You shouldn't interact with these variables unless through a function in this library. */
    struct sockaddr *sockaddr;
    SSL_CTX *openssl_context;
    int socket_descriptor;

    /* This is optional. If you don't set this option, all traffic will be unencrypted. */
    tls_context_t *tls_context;

    /* These are required. If you don't set these, then an error will be declared. */
    uint16_t port;
    char *ip;

} socket_context_t;

#endif