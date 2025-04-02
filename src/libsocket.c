#include "../include/thread_pool.h"
#include "../include/libsocket.h"
#include "../include/sockaddr.h"
#include "../include/socket.h"
#include "../include/tls.h"
#include <stdint.h>
#include <unistd.h>

int cache_id = 0;

tls_server_context_t *create_tls_server_context(char *address, uint16_t port, char *chain_certificate_file, char *private_key_file) {

    cache_id++;
    tls_server_context_t *tls_server_context = (tls_server_context_t*) malloc(sizeof(tls_server_context_t));
    if(!(tls_server_context->ssl_context = create_ssl_server_context(chain_certificate_file, private_key_file, cache_id))) {

        free(tls_server_context);
        return NULL;

    }

    if (!(tls_server_context->sockaddr = create_sockaddr(address, port))) {

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

    return tls_server_context;

}

void main() {

    create_tls_server_context("127.0.0.1", 1000, "cert.pem", "key.pem");
    getchar();

}