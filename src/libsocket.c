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
    tls_server_context->ssl_context = create_ssl_server_context(chain_certificate_file, private_key_file, cache_id);
    tls_server_context->sockaddr = create_sockaddr(address, port);
    tls_server_context->socket = create_socket(tls_server_context->sockaddr);

}