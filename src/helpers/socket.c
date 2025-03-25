#include "../../include/context.h"
#include "../../include/socket.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int unblock_socket_file_descriptor(int socket) {

    int flags;
    if ((flags = fcntl(socket, F_GETFL, 0)) == -1) {
    
        close(socket);
        return -1;
    
    }    

    flags |= O_NONBLOCK;
    if (fcntl(socket, F_SETFL, flags) == -1) {

        close(socket);
        return -1;

    }

}

int create_socket(socket_context_t *socket_context) {

    if (!socket_context || !socket_context->address || !socket_context->port || !socket_context->sockaddr) {

        return -1;

    }

    if ((socket_context->socket_descriptor = socket(socket_context->sockaddr->sa_family, SOCK_STREAM, 0)) < 0) {

        return -1;

    }

    if (socket_context->isserver) {

        if (unblock_socket_file_descriptor(socket_context->socket_descriptor) == -1) {

            return -1;

        }

        int option = 1;
        if (setsockopt(socket_context->socket_descriptor, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option))) {

            close(socket_context->socket_descriptor);
            return -1;

        }

        if (bind(socket_context->socket_descriptor, socket_context->sockaddr, sizeof(struct sockaddr_in)) < 0) {

            close(socket_context->socket_descriptor);
            return -1;

        }

        if (listen(socket_context->socket_descriptor, 4096) < 0) {

            close(socket_context->socket_descriptor);
            return -1;

        }

    } else {

        if (connect(socket_context->socket_descriptor, socket_context->sockaddr, sizeof(struct sockaddr_in)) < 0) {

            close(socket_context->socket_descriptor);
            return -1;

        }

        if (socket_context->tls_context) {

            if (!(socket_context->tls_context->openssl_bio = BIO_new_socket(socket_context->socket_descriptor, BIO_NOCLOSE))) {

                close(socket_context->socket_descriptor);
                return -1;

            }

            if (!(socket_context->tls_context->openssl_instance = SSL_new(socket_context->tls_context->openssl_context))) {

                BIO_free(socket_context->tls_context->openssl_bio);
                close(socket_context->socket_descriptor);
                return -1;

            }

            /* If we're connecting to a server via domain name services, we should run some checks on the corresponding certificate to ensure everything is hardened. */
            if (socket_context->addressisdomain) {

                if (!SSL_set_tlsext_host_name(socket_context->tls_context->openssl_instance, socket_context->address)) {

                    BIO_free(socket_context->tls_context->openssl_bio);
                    SSL_free(socket_context->tls_context->openssl_instance);
                    close(socket_context->socket_descriptor);
                    return -1;

                }

                if (!SSL_set1_host(socket_context->tls_context->openssl_instance, socket_context->address)) {

                    BIO_free(socket_context->tls_context->openssl_bio);
                    SSL_free(socket_context->tls_context->openssl_instance);
                    close(socket_context->socket_descriptor);
                    return -1;

                }

            }

            int handshake_result;
            SSL_set_bio(socket_context->tls_context->openssl_instance, socket_context->tls_context->openssl_bio, socket_context->tls_context->openssl_bio);
            if ((handshake_result = SSL_connect(socket_context->tls_context->openssl_instance)) < 1) {

                SSL_free(socket_context->tls_context->openssl_instance);
                close(socket_context->socket_descriptor);
                return -1;

            }

        }

    }

}

int socket_dispatch(socket_context_t *socket_context, void (*handle_client)(socket_dispatch_vargs_t) ) {

    if (!socket_context || !socket_context->isserver) {

        return -1;

    }

    int epoll_descriptor;
    if ((epoll_descriptor = epoll_create1(0)) == -1) {

        close(socket_context->socket_descriptor);
        return -1;

    }

    struct epoll_event event, events[32];
    memset(&event, 0, sizeof(struct epoll_event));
    memset(&events, 0, sizeof(struct epoll_event));
    event.events = EPOLLIN;
    event.data.fd = socket_context->socket_descriptor;
    if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, socket_context->socket_descriptor, &event) == -1) {

        close(socket_context->socket_descriptor);
        close(epoll_descriptor);
        return -1;

    }

    while (1) {

        int file_descriptors;
        if ((file_descriptors = epoll_wait(epoll_descriptor, events, 32, -1)) == -1) {

            close(socket_context->socket_descriptor);
            close(epoll_descriptor);
            return -1;

        }

        for (int i = 0; i < file_descriptors; i++) {

            /* There's a new connection! */
            int client_socket_descriptor;
            unsigned int sockaddr_length = sizeof(struct sockaddr_in);
            if (events[i].data.fd == socket_context->socket_descriptor) {

                struct sockaddr_in6 clientaddr;
                if (socket_context->sockaddr->sa_family == AF_INET) {

                    if (client_socket_descriptor = accept(socket_context->socket_descriptor, (struct sockaddr *) &clientaddr, &sockaddr_length)) {

                        continue;

                    }

                } else {

                    if (client_socket_descriptor = accept(socket_context->socket_descriptor, (struct sockaddr *) &clientaddr, &sockaddr_length)) {

                        continue;

                    }

                }

                if (unblock_socket_file_descriptor(client_socket_descriptor) == -1) {

                    continue;

                }

                event.events = EPOLLIN | EPOLLET;
                event.data.fd = client_socket_descriptor;
                if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, client_socket_descriptor, &event) == -1) {

                    close(client_socket_descriptor);
                    continue;

                }

                if (socket_context->tls_context) {

                    BIO *bio;
                    if (!(bio = BIO_new_socket(client_socket_descriptor, BIO_NOCLOSE))) {

                        close(client_socket_descriptor);
                        continue;

                    }

                    SSL *ssl;
                    if (!(ssl = SSL_new(socket_context->tls_context->openssl_context))) {

                        close(client_socket_descriptor);
                        continue;

                    }

                    /* Perform the handshake. */
                    SSL_set_bio(ssl, bio, bio);
                    if (SSL_accept(ssl) <= 0) {

                        SSL_free(ssl);
                        continue;

                    }

                }

            } else {

                client_socket_descriptor = events[i].data.fd;
                socket_dispatch_vargs_t socket_dispatch;
                memset(&socket_dispatch, 0, sizeof(socket_dispatch));
                socket_dispatch.socket_context = socket_context;
                socket_dispatch.client_sock_descriptor = client_socket_descriptor;
                pthread_t thread;
                pthread_create(&thread, NULL, (void*) handle_client, (void *)&socket_dispatch);
                pthread_detach(thread);

            }

        }

    }
    
}