#ifndef TLS_H_
#define TLS_H_

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

typedef struct tls_worker_vargs {

    BIO *bio;
    SSL *ssl;

} tls_worker_vargs_t;

SSL_CTX *create_ssl_server_context(char *chain_certificate_file, char *private_key_file, int cache_id);
void destroy_ssl_server_context(SSL_CTX *context);

#endif