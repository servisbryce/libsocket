#include "../../include/context.h"
#include <sys/socket.h>
#include <stdint.h>
#include <stdlib.h>

int create_socket(socket_context_t *socket_context) {

    if (!socket_context->address, !socket_context->port, !socket_context->sockaddr) {

        return -1;

    }

    if (socket_context->isserver) {

        if ((socket_context->socket_descriptor = socket(socket_context->sockaddr->sa_family, SOCK_STREAM, 0)) < 0) {

            return -1;

        }

        int option = 1;
        if (setsockopt(socket_context->socket_descriptor, SOL_SOCKET | SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option))) {

            return -1;

        }

        if (bind(socket_context->socket_descriptor, socket_context->sockaddr, sizeof(struct sockaddr_in)) < 0) {

            return -1;

        }

        if (listen(socket_context->socket_descriptor, 4096) < 0) {

            return -1;

        }

    } else {

        if ((socket_context->socket_descriptor = socket(socket_context->sockaddr->sa_family, SOCK_STREAM, 0)) < 0) {

            return -1;

        }

        if (connect(socket_context->socket_descriptor, socket_context->sockaddr, sizeof(struct sockaddr_in)) < 0) {

            return -1;

        }

        if (socket_context->tls_context) {

            if (!(socket_context->tls_context->openssl_bio = BIO_new_socket(socket_context->socket_descriptor, BIO_NOCLOSE))) {

                return -1;

            }

            if (!(socket_context->tls_context->openssl_instance = SSL_new(socket_context->tls_context->openssl_context))) {

                return -1;

            }

            /* If we're connecting to a server via domain name services, we should run some checks on the corresponding certificate to ensure everything is hardened. */
            if (socket_context->addressisdomain) {

                if (!SSL_set_tlsext_host_name(socket_context->tls_context->openssl_instance, socket_context->address)) {

                    SSL_free(socket_context->tls_context->openssl_instance);
                    return -1;

                }

                if (!SSL_set1_host(socket_context->tls_context->openssl_instance, socket_context->address)) {

                    SSL_free(socket_context->tls_context->openssl_instance);
                    return -1;

                }

            }

            int handshake_result;
            SSL_set_bio(socket_context->tls_context->openssl_instance, socket_context->tls_context->openssl_bio, socket_context->tls_context->openssl_bio);
            if ((handshake_result = SSL_connect(socket_context->tls_context->openssl_context)) == 0) {

                return -1;

            } else if (handshake_result < 0) {

                SSL_free(socket_context->tls_context->openssl_instance);

            }

        }

    }

}