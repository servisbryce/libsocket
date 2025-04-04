#ifndef TLS_H_
#define TLS_H_

#include "thread_pool.h"
#include "sockaddr.h"
#include "socket.h"
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

typedef struct tls_worker_vargs {

    BIO *bio;
    SSL *ssl;

} tls_worker_vargs_t;

typedef struct tls_server_context {

    struct sockaddr *sockaddr;
    thread_pool_t *thread_pool;
    SSL_CTX *ssl_context;
    size_t *sockaddr_length;
    size_t threads;
    void (*routine)(void *routine_vargs);
    int socket;

} tls_server_context_t;

typedef struct client_context {



} client_context_t;

tls_server_context_t *create_tls_server_context(char *address, uint16_t port, char *chain_certificate_file, char *private_key_file, size_t threads, void (*routine)(void *vargs));
SSL_CTX *create_ssl_server_context(char *chain_certificate_file, char *private_key_file, int cache_id);
void destroy_tls_server_context(tls_server_context_t *tls_server_context);
void destroy_ssl_server_context(SSL_CTX *context);
void tls_server_destroy(tls_server_context_t *tls_server_context);
void tls_server_wait(tls_server_context_t *tls_server_context);
int tls_server_listen(tls_server_context_t *tls_server_context);

#endif