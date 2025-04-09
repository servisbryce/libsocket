#include "../include/tls.h"
#include <stdint.h>
#include <unistd.h>
#include <error.h>

void *routine(void *vargs) {

    tls_worker_vargs_t *a = (tls_worker_vargs_t *) vargs;
    tls_data_t *data = tls_receive(a);
    printf("%ld\n", data->buffer_length);
    printf("%p\n", data->buffer);
    printf("%s\n", (char *) data->buffer);
    destroy_tls_data(data);
    return NULL;

}

void main() {

    socket_parameters_t parameters;
    memset(&parameters, 0, sizeof(parameters));
    parameters.timeout = -1;
    parameters.address = "127.0.0.1";
    parameters.certificate_file = "cert.pem";
    parameters.private_key_file = "key.pem";
    parameters.port = 1025;
    parameters.routine = routine;
    parameters.maximum_threads = 4;
    parameters.stepwise_threads = 2;
    parameters.target_threads = 2;
    parameters.buffer_length = 4;

    tls_server_context_t *a = create_tls_server_context(&parameters);
    tls_server_listen(a);
    tls_server_wait(a);
    tls_server_shutdown(a);
    destroy_tls_server_context(a);

}