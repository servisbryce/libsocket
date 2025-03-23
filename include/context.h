#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <stdbool.h>
#include <stddef.h>

/* This context is optional. If this context isn't set by the user, then all traffic will be unencrypted. */
typedef struct tls_context {

    /* You shouldn't interact with these variables unless through a function in this library. */
    SSL_CTX *openssl_context;
    int openssl_tls_cache_id;
    BIO *openssl_bio;

    /* You shouldn't interact with this. This is for client-mode only. */
    SSL *openssl_instance;

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
    bool addressIsDomain;
    int socket_descriptor;

    /* This is optional. If you don't set this option, all traffic will be unencrypted. */
    tls_context_t *tls_context;

    /* This is optional. If you don't set this option, keepalives will be disabled (you want them most of the time!). */
    int timeout;

    /* These are required. If you don't set these, then an error will be declared. You shouldn't ever change these after you've created your context, create a new context instead. */
    uint16_t port;
    char *address;
    bool isserver;

} socket_context_t;

int create_socket_context(char *address, uint16_t port, bool isserver, socket_context_t **socket_context);
int socket_context_set_timeout(socket_context_t *socket_context, int timeout);
int create_tls_context(socket_context_t *socket_context, char *chained_certificate_path, char *certificate_path, char *private_key_path);
int tls_context_set_cache_length(socket_context_t *socket_context, size_t tls_context_cache_length);
int tls_context_set_cache_expiry(socket_context_t *socket_context, int expiry);
int tls_context_set_options(socket_context_t *socket_context, long options);
int tls_context_append_options(socket_context_t *socket_context, long options);
int tls_context_set_minimum_version(socket_context_t *socket_context, int minimum_version);
int tls_context_set_maximum_version(socket_context_t *socket_context, int maximum_version);
void free_socket_context(socket_context_t **socket_context);
void free_tls_context(tls_context_t *tls_context);

#endif