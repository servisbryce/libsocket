#include "../../include/sockaddr.h"
#include "../../include/socket.h"
#include "../../include/tls.h"
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <pthread.h>
#include <unistd.h>

int cache_id;

SSL_CTX *create_ssl_server_context(char *certificate_file, char *private_key_file, int cache_id) {

    if (!certificate_file && !private_key_file) {

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
    if (SSL_CTX_use_certificate_chain_file(context, certificate_file) <= 0) {

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

int destroy_ssl_server_context(SSL_CTX *context) {

    if (!context) {

        return -1;

    }

    SSL_CTX_free(context);
    return 0;

}


tls_server_context_t *create_tls_server_context(char *address, uint16_t port, char *certificate_file, char *private_key_file, size_t threads, long timeout, void (*routine)(void *vargs)) {

    if (!address || !certificate_file || !private_key_file || threads < 1 || port < 1 || !routine) {

        return NULL;

    }

    cache_id++;
    tls_server_context_t *tls_server_context = (tls_server_context_t*) malloc(sizeof(tls_server_context_t));
    if(!(tls_server_context->ssl_context = create_ssl_server_context(certificate_file, private_key_file, cache_id))) {

        free(tls_server_context);
        return NULL;

    }

    if (!(tls_server_context->sockaddr = create_sockaddr(address, port, &tls_server_context->sockaddr_length))) {

        destroy_ssl_server_context(tls_server_context->ssl_context);
        free(tls_server_context);
        return NULL;

    }

    if ((tls_server_context->socket = create_socket(tls_server_context->sockaddr, timeout)) < 0) {

        destroy_ssl_server_context(tls_server_context->ssl_context);
        destroy_sockaddr(tls_server_context->sockaddr);
        free(tls_server_context);
        return NULL;

    }

    tls_server_context->threads = threads;
    tls_server_context->routine = routine;
    return tls_server_context;

}

int tls_server_set_routine(tls_server_context_t *tls_server_context, void (*routine)(void *vargs)) {

    if (!tls_server_context || !routine) {

        return -1;

    }

    tls_server_context->routine = routine;
    return 0;

}

int tls_server_set_threads(tls_server_context_t *tls_server_context, size_t threads) {

    if (!tls_server_context || !tls_server_context->thread_pool || threads < 1) {

        return -1;

    }

    if (thread_pool_destroy(tls_server_context->thread_pool) < 0) {

        return -1;

    }

    tls_server_context->thread_pool = NULL;
    tls_server_context->threads = threads;
    if (!(tls_server_context->thread_pool = thread_pool_create(tls_server_context->threads))) {

        return -1;

    }

    return 0;

}

int destroy_tls_server_context(tls_server_context_t *tls_server_context) {

    if (!tls_server_context || tls_server_context->socket < 0 || !tls_server_context->sockaddr || !tls_server_context->ssl_context) {

        return -1;

    }

    if (close(tls_server_context->socket) < 0) {

        return -1;

    }

    if (destroy_sockaddr(tls_server_context->sockaddr) < 0) {

        return -1;

    }

    if (destroy_ssl_server_context(tls_server_context->ssl_context) < 0) {

        return -1;

    }

    free(tls_server_context);
    return 0;

}

void *tls_server_orchestrator(void *tls_server_orchestrator_vargs) {

    if (!tls_server_orchestrator_vargs) {

        return (void *) -1;

    }

    tls_server_context_t *tls_server_context = (tls_server_context_t *) tls_server_orchestrator_vargs;
    while (1) {

        if (tls_server_context->thread_pool->halt) {

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
        if (thread_pool_assign_work(tls_server_context->thread_pool, (void *) tls_server_context->routine, (void *) tls_worker_vargs) == -1) {

            continue;

        }

    }

    return (void *) -1;

}

int tls_server_listen(tls_server_context_t *tls_server_context) {

    if (!tls_server_context || tls_server_context->socket < 0 || !tls_server_context->ssl_context || tls_server_context->threads < 1) {

        return -1;

    }

    if (!(tls_server_context->thread_pool = thread_pool_create(tls_server_context->threads))) {

        return -1;

    }

    if (thread_pool_assign_work(tls_server_context->thread_pool, tls_server_orchestrator, (void *) tls_server_context) < 0) {

        return -1;

    }

    return 0;

}

int tls_server_wait(tls_server_context_t *tls_server_context) {

    if (!tls_server_context || !tls_server_context->thread_pool) {

        return -1;

    }

    if (thread_pool_wait(tls_server_context->thread_pool) < 0) {

        return -1;

    }

    return 0;

}

int tls_server_shutdown(tls_server_context_t *tls_server_context) {

    if (!tls_server_context || !tls_server_context->thread_pool) {

        return -1;

    }

    if (thread_pool_destroy(tls_server_context->thread_pool) < 0) {

        return -1;

    }

    return 0;

}