#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <netdb.h>

/* 

    This automatically determines the socket address for whatever domain or internet address you're requesting.

    The user is responsible for dictating what port is being used whenever they interact with the structure outside of this function as this function is designed to be address protocol agnostic.

*/

int create_sockaddr(char *address, struct sockaddr **sockaddr) {

    /* Determine the address of the host that we're discovering. */
    struct addrinfo *response = NULL;
    struct addrinfo hints;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(address, NULL, &hints, &response) != 0) {

        return -1;

    }

    /* Construct the socket address and pass the data to the user. */
    struct sockaddr result = *response->ai_addr;
    *sockaddr = &result;

    /* Return our memory back to the user and declare success. */
    freeaddrinfo(response);
    return 0;

}