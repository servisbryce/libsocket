#include "../include/thread_pool.h"
#include "../include/libsocket.h"
#include "../include/sockaddr.h"
#include "../include/socket.h"
#include "../include/tls.h"
#include <stdint.h>
#include <unistd.h>
#include <error.h>

int cache_id = 0;

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

int tls_server_listen(tls_server_context_t *tls_server_context) {

    if (!tls_server_context || !tls_server_context->sockaddr || tls_server_context->socket < 0 || !tls_server_context->ssl_context || tls_server_context->threads == 0) {

        return -1;

    }

    thread_pool_t *thread_pool = NULL;
    if (!(thread_pool = thread_pool_create(tls_server_context->threads))) {

        return -1;

    }

    while (1) {

        struct sockaddr client_sockaddr;
        socklen_t client_sockaddr_length = *tls_server_context->sockaddr_length;
        int client_socket;
        if ((client_socket = ac cept(tls_server_context->socket, &client_sockaddr, &client_sockaddr_length)) < 0) {

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

        tls_worker_vargs_t *vargs = (tls_worker_vargs_t *) malloc(sizeof(tls_worker_vargs_t));
        vargs->ssl = client_ssl;
        vargs->bio = client_bio;
        if (thread_pool_assign_work(thread_pool, tls_server_context->routine, (void *) vargs) == -1) {

            continue;

        }

    }

}

void routine(void *arg) {

    tls_worker_vargs_t *a = (tls_worker_vargs_t *) arg;
    SSL_write(a->ssl, "hi", 3);
    SSL_shutdown(a->ssl);
    SSL_free(a->ssl);

}

void main() {

    tls_server_context_t *a = create_tls_server_context("127.0.0.1", 1025, "cert.pem", "key.pem", 32, routine);
    tls_server_listen(a);
    destroy_tls_server_context(a);

}