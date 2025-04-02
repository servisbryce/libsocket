#include <sys/socket.h>
#include <sys/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>

struct sockaddr *create_sockaddr(char *address, uint16_t port, size_t *sockaddr_length) {

    if (!address || port <= 0) {

        return NULL;

    }

    struct addrinfo *response = NULL;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    if (getaddrinfo(address, NULL, &hints, &response) != 0) {

        return NULL;

    }

    size_t sockaddr_len = sizeof(struct sockaddr_in6);
    if (response->ai_addr->sa_family == AF_INET) {

        sockaddr_len = sizeof(struct sockaddr_in);

    }

    struct sockaddr *sockaddr = (struct sockaddr*) malloc(sockaddr_len);
    memcpy(sockaddr, response->ai_addr, sockaddr_len);
    sockaddr->sa_family = response->ai_family;
    sockaddr_length = &sockaddr_len;
    freeaddrinfo(response);
    return sockaddr;

}

void destroy_sockaddr(struct sockaddr *sockaddr) {

    if (!sockaddr) {

        return;

    }

    free(sockaddr);
    return;

}