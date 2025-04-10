#include "../../include/libsocket.h"
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

tls_server_context_t *create_tls_server_context(socket_parameters_t *socket_parameters) {

    if (!socket_parameters->address || !socket_parameters->certificate_file || !socket_parameters->private_key_file || !socket_parameters->routine || socket_parameters->target_threads < 1 || socket_parameters->target_threads > socket_parameters->maximum_threads || socket_parameters->port < 1) {

        return NULL;

    }

    cache_id++;
    tls_server_context_t *tls_server_context = (tls_server_context_t*) malloc(sizeof(tls_server_context_t));
    if(!(tls_server_context->ssl_context = create_ssl_server_context(socket_parameters->certificate_file, socket_parameters->private_key_file, cache_id))) {

        free(tls_server_context);
        return NULL;

    }

    if (!(tls_server_context->sockaddr = create_sockaddr(socket_parameters->address, socket_parameters->port, &tls_server_context->sockaddr_length))) {

        destroy_ssl_server_context(tls_server_context->ssl_context);
        free(tls_server_context);
        return NULL;

    }

    if ((tls_server_context->socket = create_socket(tls_server_context->sockaddr, socket_parameters->timeout)) < 0) {

        destroy_ssl_server_context(tls_server_context->ssl_context);
        destroy_sockaddr(tls_server_context->sockaddr);
        free(tls_server_context);
        return NULL;

    }

    tls_server_context->buffer_length = socket_parameters->buffer_length;
    if (tls_server_context->buffer_length == 0) {

        tls_server_context->buffer_length = 65536;

    }

    tls_server_context->target_threads = socket_parameters->target_threads;
    tls_server_context->maximum_threads = socket_parameters->maximum_threads;
    tls_server_context->stepwise_threads = socket_parameters->stepwise_threads;
    tls_server_context->routine = socket_parameters->routine;
    return tls_server_context;

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
        
        int client_socket;
        struct sockaddr client_sockaddr;
        memset(&client_sockaddr, 0, sizeof(client_sockaddr));
        if ((client_socket = accept(tls_server_context->socket, &client_sockaddr, tls_server_context->sockaddr_length)) < 0) {

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
        tls_worker_vargs->buffer_length = tls_server_context->buffer_length;
        tls_worker_vargs->immunity = true;
        tls_worker_vargs->ssl = client_ssl;
        tls_worker_vargs->bio = client_bio;
        if (thread_pool_assign_work(tls_server_context->thread_pool, (void *) tls_server_context->routine, (void *) tls_worker_vargs) == -1) {

            SSL_shutdown(client_ssl);
            SSL_free(client_ssl);
            close(client_socket);
            continue;

        }

    }

    return (void *) -1;

}

int tls_server_listen(tls_server_context_t *tls_server_context) {

    if (!tls_server_context || tls_server_context->socket < 0 || !tls_server_context->ssl_context || tls_server_context->target_threads < 1) {

        return -1;

    }

    if (!(tls_server_context->thread_pool = thread_pool_create(tls_server_context->target_threads, tls_server_context->stepwise_threads, tls_server_context->maximum_threads))) {

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

tls_data_t *tls_receive(void *tls_worker_vargs_p) {

    if (!tls_worker_vargs_p) {

        return NULL;

    }

    tls_worker_vargs_t *tls_worker_vargs = (tls_worker_vargs_t *) tls_worker_vargs_p;
    if (!tls_worker_vargs->bio || !tls_worker_vargs->ssl) {

        return NULL;

    }

    size_t truncated_length = 0;
    void *buffer = malloc(tls_worker_vargs->buffer_length);
    if (SSL_read_ex(tls_worker_vargs->ssl, buffer, tls_worker_vargs->buffer_length, &truncated_length) <= 0) {

        return NULL;

    }

    void *truncated_buffer = realloc(buffer, truncated_length);
    if (truncated_buffer != buffer) {

        free(buffer);
        buffer = NULL;

    }

    tls_data_t *tls_data = (tls_data_t *) malloc(sizeof(tls_data_t));
    tls_data->buffer_length = truncated_length;
    tls_data->buffer = buffer;
    return tls_data;

}

int tls_send(void *tls_worker_vargs_p, tls_data_t *tls_data) {

    tls_worker_vargs_t *tls_worker_vargs = (tls_worker_vargs_t *) tls_worker_vargs_p;
    if (!tls_data || !tls_worker_vargs || !tls_data->buffer || !tls_worker_vargs->bio || !tls_worker_vargs->ssl) {

        return -1;

    }

    if (SSL_write(tls_worker_vargs->ssl, tls_data->buffer, tls_data->buffer_length) <= 0) {

        return -1;

    }

    return 0;

}

int destroy_tls_data(tls_data_t *tls_data) {

    if (!tls_data) {

        return -1;

    }

    if (tls_data->buffer) {

        free(tls_data->buffer);

    }

    free(tls_data);
    return 0;

}