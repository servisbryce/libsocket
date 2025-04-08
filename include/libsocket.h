#ifndef LIBSOCKET_H_
#define LIBSOCKET_H_

#include <stddef.h>
#include <stdint.h>

typedef struct socket_parameters_t {

    long timeout;
    char *address;
    char *certificate_file;
    char *private_key_file;
    void *(*routine)(void *routine_vargs);
    size_t maximum_threads;
    size_t stepwise_threads;
    size_t target_threads;
    uint16_t port;

} socket_parameters_t;

#endif