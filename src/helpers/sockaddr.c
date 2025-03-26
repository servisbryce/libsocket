#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>

/* 

    This automatically determines the socket address for whatever domain or internet address you're requesting.

    The user is responsible for dictating what port is being used whenever they interact with the structure outside of this function as this function is designed to be address protocol agnostic.

    This function returns a status code. Any value that isn't zero is an error declaration.

*/

int create_sockaddr(char *address, uint16_t port, struct sockaddr **sockaddr) {

    /* Determine the address of the host that we're discovering. */
    struct addrinfo *response = NULL;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(address, NULL, &hints, &response) != 0) {

        return -1;

    }

    /* Construct the socket address and pass the data to the user. */
    size_t length = 0;
    if (response->ai_addr->sa_family == AF_INET) {

        length = sizeof(struct sockaddr_in);

    } else {

        length = sizeof(struct sockaddr_in6);

    }

    struct sockaddr *sockaddr_response = (struct sockaddr*) malloc(length);
    sockaddr_response = (struct sockaddr*) memcpy(sockaddr_response, response->ai_addr, length);
    if (response->ai_addr->sa_family == AF_INET) {

        struct sockaddr_in *sockaddr_in_response = (struct sockaddr_in*) sockaddr_response;
        sockaddr_in_response->sin_port = htons(port);
        sockaddr_response = (struct sockaddr*) sockaddr_in_response;

    } else {

        struct sockaddr_in6 *sockaddr_in6_response = (struct sockaddr_in6*) sockaddr_response;
        sockaddr_in6_response->sin6_port = htons(port);
        sockaddr_response = (struct sockaddr*) sockaddr_in6_response;

    }

    *sockaddr = sockaddr_response;

    /* Return our memory back to the user and declare success. */
    freeaddrinfo(response);
    return 0;

}