#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>

int create_socket(struct sockaddr *sockaddr) {

    if (!sockaddr) {

        return -1;

    }

    int socket_file_descriptor;
    if ((socket_file_descriptor = socket(sockaddr->sa_family, SOCK_STREAM, 0)) == -1) {

        return -1;

    }

    size_t sockaddr_length = sizeof(struct sockaddr_in6);
    if (sockaddr->sa_family == AF_INET) {

        sockaddr_length = sizeof(struct sockaddr_in);

    }

    int socket_options = SO_REUSEADDR | SO_REUSEPORT;
    if (setsockopt(socket_file_descriptor, SOL_SOCKET, socket_options, &socket_options, sizeof(socket_options)) == -1) {

        printf("hi\n");
        close(socket_file_descriptor);
        return -1;

    }

    if (bind(socket_file_descriptor, sockaddr, sockaddr_length) == -1) {

        close(socket_file_descriptor);
        return -1;

    }

    if (listen(socket_file_descriptor, 4096) == -1) {

        close(socket_file_descriptor);
        return -1;

    }

    return socket_file_descriptor;

}

void destroy_socket(int socket_file_descriptor) {

    if (socket_file_descriptor < 0) {

        return;

    }

    close(socket_file_descriptor);
    return;

}