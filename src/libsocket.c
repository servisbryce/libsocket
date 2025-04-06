#include "../include/tls.h"
#include <stdint.h>
#include <unistd.h>
#include <error.h>

void *routine(void *arg) {

    tls_worker_vargs_t *a = (tls_worker_vargs_t *) arg;
    SSL_write(a->ssl, "hi", 3);
    return NULL;

}

void main() {

    tls_server_context_t *a = create_tls_server_context("127.0.0.1", 1025, "cert.pem", "key.pem", 5, 10, (void *) routine);
    tls_server_listen(a);
    tls_server_decrement_threads(a, 4);
    tls_server_wait(a);
    tls_server_shutdown(a);
    destroy_tls_server_context(a);

}