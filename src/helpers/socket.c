#include "../../include/context.h"
#include "../../include/socket.h"
#include "openssl/err.h"
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

    return 0;

}

int create_socket(socket_context_t *socket_context) {

    if (!socket_context || !socket_context->address || !socket_context->port || !socket_context->sockaddr) {

        return -1;

    }

    if ((socket_context->socket_descriptor = socket(socket_context->sockaddr->sa_family, SOCK_STREAM, 0)) < 0) {

        return -1;

    }

    size_t sockaddr_length = 0;
    if (socket_context->sockaddr->sa_family == AF_INET) {

        sockaddr_length = sizeof(struct sockaddr_in);

    } else {

        sockaddr_length = sizeof(struct sockaddr_in6);

    }

    if (socket_context->isserver) {

        /*if (unblock_socket_file_descriptor(socket_context->socket_descriptor) == -1) {

            return -1;

        }*/

        int option = 1;
        if (setsockopt(socket_context->socket_descriptor, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option))) {

            close(socket_context->socket_descriptor);
            return -1;

        }

        if (bind(socket_context->socket_descriptor, socket_context->sockaddr, sockaddr_length) < 0) {

            close(socket_context->socket_descriptor);
            return -1;

        }

        if (listen(socket_context->socket_descriptor, 4096) < 0) {

            close(socket_context->socket_descriptor);
            return -1;

        }

    } else {

        if (connect(socket_context->socket_descriptor, socket_context->sockaddr, sockaddr_length) < 0) {

            close(socket_context->socket_descriptor);
            return -1;

        }

        if (socket_context->tls_context) {

            //rewrite later

        }

    }

    return 0;

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

    unsigned int sockaddr_length = 0;
    if (socket_context->sockaddr->sa_family == AF_INET) {

        sockaddr_length = sizeof(struct sockaddr_in);

    } else {

        sockaddr_length = sizeof(struct sockaddr_in6);

    }

    struct epoll_event event, events[4096];
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
        if ((file_descriptors = epoll_wait(epoll_descriptor, events, 4096, -1)) == -1) {

            close(socket_context->socket_descriptor);
            close(epoll_descriptor);
            return -1;

        }

        for (int i = 0; i < file_descriptors; i++) {

            /* There's a new connection! */
            int client_socket_descriptor;
            if (events[i].data.fd == socket_context->socket_descriptor) {

                struct sockaddr *clientaddr = NULL;
                clientaddr = (struct sockaddr*) malloc(sockaddr_length);
                if ((client_socket_descriptor = accept(socket_context->socket_descriptor, clientaddr, &sockaddr_length)) == -1) {

                    free(clientaddr);
                    continue;

                }

                if (unblock_socket_file_descriptor(client_socket_descriptor) == -1) {

                    free(clientaddr);
                    continue;

                }

                event.events = EPOLLIN | EPOLLET;
                event.data.fd = client_socket_descriptor;
                if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, client_socket_descriptor, &event) == -1) {

                    free(clientaddr);
                    close(client_socket_descriptor);
                    continue;

                }

                free(clientaddr);

            } else {

                client_socket_descriptor = events[i].data.fd;
                if (fork() != 0) {

                    /* Socket is inherited by the client. */
                    close(client_socket_descriptor);
                    continue; 
    
                }

                socket_dispatch_vargs_t socket_dispatch;
                memset(&socket_dispatch, 0, sizeof(socket_dispatch));
                socket_dispatch.socket_context = socket_context;
                socket_dispatch.client_sock_descriptor = client_socket_descriptor;
                handle_client(socket_dispatch);
                exit(EXIT_SUCCESS);

            }

        }

    }
    
}