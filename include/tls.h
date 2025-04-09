#ifndef TLS_H_
#define TLS_H_

#include "thread_pool.h"
#include "libsocket.h"
#include "sockaddr.h"
#include "socket.h"
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

typedef struct tls_worker_vargs {

    bool immunity;
    BIO *bio;
    SSL *ssl;

} tls_worker_vargs_t;

typedef struct tls_server_context {

    struct sockaddr *sockaddr;
    thread_pool_t *thread_pool;
    SSL_CTX *ssl_context;
    unsigned int *sockaddr_length;
    size_t target_threads;
    size_t stepwise_threads;
    size_t maximum_threads;
    void *(*routine)(void *routine_vargs);
    int socket;

} tls_server_context_t;

typedef struct tls_data {

    size_t length;
    void *buffer;

} tls_data_t;

tls_server_context_t *create_tls_server_context(socket_parameters_t *socket_parameters);
SSL_CTX *create_ssl_server_context(char *certificate_file, char *private_key_file, int cache_id);
int destroy_tls_server_context(tls_server_context_t *tls_server_context);
int destroy_ssl_server_context(SSL_CTX *context);
int tls_server_shutdown(tls_server_context_t *tls_server_context);
int tls_server_wait(tls_server_context_t *tls_server_context);
int tls_server_listen(tls_server_context_t *tls_server_context);

#endif