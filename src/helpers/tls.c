#include "../../include/sockaddr.h"
#include "../../include/socket.h"
#include "../../include/tls.h"
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <pthread.h>
#include <unistd.h>

int cache_id;

SSL_CTX *create_ssl_server_context(char *chain_certificate_file, char *private_key_file, int cache_id) {

    if (!chain_certificate_file && !private_key_file) {

        return NULL;

    }

    SSL_CTX *context = NULL;
    if (!(context = SSL_CTX_new(TLS_server_method()))) {

        return NULL;

    }

    if(!SSL_CTX_set_min_proto_version(context, TLS1_2_VERSION)) {

        SSL_CTX_free(context);
        return NULL;

    }

    SSL_CTX_set_options(context, SSL_OP_IGNORE_UNEXPECTED_EOF | SSL_OP_NO_RENEGOTIATION | SSL_OP_CIPHER_SERVER_PREFERENCE);
    if (SSL_CTX_use_certificate_chain_file(context, chain_certificate_file) <= 0) {

        SSL_CTX_free(context);
        return NULL;

    }

    if (SSL_CTX_use_PrivateKey_file(context, private_key_file, SSL_FILETYPE_PEM) <= 0) {

        SSL_CTX_free(context);
        return NULL;

    }

    SSL_CTX_set_session_id_context(context, (void *) &cache_id, sizeof(cache_id));
    SSL_CTX_set_session_cache_mode(context, SSL_SESS_CACHE_SERVER);
    SSL_CTX_sess_set_cache_size(context, 32768);
    SSL_CTX_set_timeout(context, 1800);
    SSL_CTX_set_verify(context, SSL_VERIFY_NONE, NULL);
    return context;

}

void destroy_ssl_server_context(SSL_CTX *context) {

    if (!context) {

        return;

    }

    SSL_CTX_free(context);
    return;

}


tls_server_context_t *create_tls_server_context(char *address, uint16_t port, char *chain_certificate_file, char *private_key_file, size_t threads, void (*routine)(void *vargs)) {

    if (!address || !chain_certificate_file || !private_key_file || threads == 0 || port == 0 || !routine) {

        return NULL;

    }

    cache_id++;
    tls_server_context_t *tls_server_context = (tls_server_context_t*) malloc(sizeof(tls_server_context_t));
    if(!(tls_server_context->ssl_context = create_ssl_server_context(chain_certificate_file, private_key_file, cache_id))) {

        free(tls_server_context);
        return NULL;

    }

    if (!(tls_server_context->sockaddr = create_sockaddr(address, port, &tls_server_context->sockaddr_length))) {

        destroy_ssl_server_context(tls_server_context->ssl_context);
        free(tls_server_context);
        return NULL;

    }

    if ((tls_server_context->socket = create_socket(tls_server_context->sockaddr)) == -1) {

        destroy_ssl_server_context(tls_server_context->ssl_context);
        destroy_sockaddr(tls_server_context->sockaddr);
        free(tls_server_context);
        return NULL;

    }

    tls_server_context->threads = threads;
    tls_server_context->routine = routine;
    return tls_server_context;

}

void destroy_tls_server_context(tls_server_context_t *tls_server_context) {

    if (!tls_server_context || !tls_server_context->sockaddr || !tls_server_context->ssl_context) {

        return;

    }

    close(tls_server_context->socket);
    destroy_sockaddr(tls_server_context->sockaddr);
    destroy_ssl_server_context(tls_server_context->ssl_context);
    free(tls_server_context);
    return;

}

void *tls_server_orchestrator(void *tls_server_orchestrator_vargs) {

    tls_server_context_t *tls_server_context = (tls_server_context_t *) tls_server_orchestrator_vargs;
    thread_pool_t *thread_pool = tls_server_context->thread_pool;
    while (1) {

        if (thread_pool->halt) {

            return (void *) -1;

        }
        
        struct sockaddr client_sockaddr;
        socklen_t client_sockaddr_length = *tls_server_context->sockaddr_length;
        int client_socket;
        if ((client_socket = accept(tls_server_context->socket, &client_sockaddr, &client_sockaddr_length)) < 0) {

            continue;

        }

        BIO *client_bio = NULL;
        if (!(client_bio = BIO_new_socket(client_socket, BIO_CLOSE))) {

            close(client_socket);
            continue;

        }

        SSL *client_ssl = NULL;
        if (!(client_ssl = SSL_new(tls_server_context->ssl_context))) {

            BIO_free(client_bio);
            close(client_socket);
            continue;

        }

        SSL_set_bio(client_ssl, client_bio, client_bio);
        if (SSL_accept(client_ssl) <= 0) {

            SSL_free(client_ssl);
            close(client_socket);
            continue;

        }

        tls_worker_vargs_t *tls_worker_vargs = (tls_worker_vargs_t *) malloc(sizeof(tls_worker_vargs_t));
        tls_worker_vargs->ssl = client_ssl;
        tls_worker_vargs->bio = client_bio;
        if (thread_pool_assign_work(thread_pool, (void *) tls_server_context->routine, (void *) tls_worker_vargs) == -1) {

            continue;

        }

    }

    return (void *) -1;

}

int tls_server_listen(tls_server_context_t *tls_server_context) {

    if (!tls_server_context || !tls_server_context->sockaddr || tls_server_context->socket < 0 || !tls_server_context->ssl_context || tls_server_context->threads == 0) {

        return -1;

    }

    thread_pool_t *thread_pool = NULL;
    if (!(thread_pool = thread_pool_create(tls_server_context->threads))) {

        return -1;

    }

    tls_server_context->thread_pool = thread_pool;
    thread_pool_assign_work(thread_pool, tls_server_orchestrator, (void *) tls_server_context);
    return 0;

}

void tls_server_wait(tls_server_context_t *tls_server_context) {

    thread_pool_wait(tls_server_context->thread_pool);

}

void tls_server_destroy(tls_server_context_t *tls_server_context) {

    thread_pool_destroy(tls_server_context->thread_pool);

}