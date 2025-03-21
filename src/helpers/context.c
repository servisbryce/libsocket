#include "../../include/sockaddr.h"
#include "../../include/context.h"
#include <stdint.h>
#include <stdlib.h>

int openssl_tls_cache_id = 1;

int create_socket_context(char *address, uint16_t port, bool isserver, socket_context_t **socket_context) {

    /* Validate our function inputs to ensure they're set properly. */
    if (!address || port <= 0 || !isserver) {

        return -1;

    }

    /* Create our prerequisite structures. */
    socket_context_t *result = NULL;
    struct sockaddr *sockaddr = NULL;
    if (create_sockaddr(address, &sockaddr) != 0) {

        return -1;

    }

    /* Allocate memory and load data into our structures.*/
    result = malloc(sizeof(socket_context_t));
    result->tls_context = NULL;
    result->address = address;
    result->port = port;
    result->sockaddr = sockaddr;
    result->isserver = isserver;

    /* Pass the context to the user and declare success. */
    *socket_context = result;
    return 0;
    
}

int socket_context_set_timeout(socket_context_t *socket_context, int timeout) {

    if (!socket_context) {

        return -1;

    }

    socket_context->timeout = timeout;
    return 0;

}

int create_tls_context(socket_context_t *socket_context, char *chained_certificate_path, char *certificate_path, char *private_key_path) {

    /* Validate our function inputs to ensure they're set properly. */
    if (!socket_context || ((!chained_certificate_path && !certificate_path) || (chained_certificate_path && certificate_path)) || !private_key_path) {

        return -1;

    }

    tls_context_t *tls_context = (tls_context_t *) malloc(sizeof(tls_context_t));
    tls_context->chained_certificate_path = chained_certificate_path;
    tls_context->openssl_tls_cache_id = openssl_tls_cache_id;
    tls_context->certificate_path = certificate_path;
    tls_context->private_key_path = private_key_path;
    tls_context->tls_cache_length = 32000;
    tls_context->tls_cache_expiry = 3600;
    if (socket_context->isserver) {

        tls_context->openssl_context = SSL_CTX_new(TLS_server_method());

    } else {

        tls_context->openssl_context = SSL_CTX_new(TLS_client_method());

    }

    if (!tls_context->openssl_context) {

        free(tls_context);
        return -1;

    }

    tls_context->minimum_tls_version = TLS1_2_VERSION;
    if (!SSL_CTX_set_min_proto_version(tls_context->openssl_context, tls_context->minimum_tls_version)) {

        SSL_CTX_free(tls_context->openssl_context);
        return -1;

    }

    tls_context->tls_options |= SSL_OP_IGNORE_UNEXPECTED_EOF | SSL_OP_NO_RENEGOTIATION | SSL_OP_CIPHER_SERVER_PREFERENCE;
    SSL_CTX_set_options(tls_context->openssl_context, tls_context->tls_options);
    if (tls_context->chained_certificate_path && SSL_CTX_use_certificate_chain_file(tls_context->openssl_context, tls_context->chained_certificate_path) <= 0)  {

        SSL_CTX_free(tls_context->openssl_context);
        return -1;

    }

    if (tls_context->certificate_path && SSL_CTX_use_certificate_file(tls_context->openssl_context, tls_context->certificate_path, SSL_FILETYPE_PEM) <= 0) {

        SSL_CTX_free(tls_context->openssl_context);
        return -1;

    }

    if (SSL_CTX_use_PrivateKey_file(tls_context->openssl_context, tls_context->private_key_path, SSL_FILETYPE_PEM) <= 0) {

        SSL_CTX_free(tls_context->openssl_context);
        return -1;

    }

    if (SSL_CTX_set_session_id_context(tls_context->openssl_context, (void *) &tls_context->openssl_tls_cache_id, sizeof(tls_context->openssl_tls_cache_id)) <= 0) {

        SSL_CTX_free(tls_context->openssl_context);
        return -1;

    }

    openssl_tls_cache_id++;
    SSL_CTX_sess_set_cache_size(tls_context->openssl_context, tls_context->tls_cache_length);
    if (socket_context->isserver) {

        SSL_CTX_set_session_cache_mode(tls_context->openssl_context, SSL_SESS_CACHE_SERVER);

    } else {

        SSL_CTX_set_session_cache_mode(tls_context->openssl_context, SSL_SESS_CACHE_CLIENT);

    }

    SSL_CTX_set_timeout(tls_context->openssl_context, tls_context->tls_cache_expiry);
    if (socket_context->isserver) {

        SSL_CTX_set_verify(tls_context->openssl_context, SSL_VERIFY_NONE, NULL);

    } else {

        /* Whenever we're a client, automatically deny connections from servers that use an untrustworthy certificate. This is incredibly important to ensure that man-in-the-middle attacks don't occur! */
        SSL_CTX_set_verify(tls_context->openssl_context, SSL_VERIFY_PEER, NULL);

    }

    socket_context->tls_context = tls_context;
    return 0;

}

int tls_context_set_cache_length(socket_context_t *socket_context, size_t tls_context_cache_length) {

    if (!socket_context || !socket_context->tls_context || !socket_context->tls_context->openssl_context) {

        return -1;

    }

    socket_context->tls_context->tls_cache_length = tls_context_cache_length;
    SSL_CTX_sess_set_cache_size(socket_context->tls_context->openssl_context, socket_context->tls_context->tls_cache_length);
    return 0;

}

int tls_context_set_cache_expiry(socket_context_t *socket_context, int expiry) {

    if (!socket_context || !socket_context->tls_context || !socket_context->tls_context->openssl_context) {

        return -1;

    }

    socket_context->tls_context->tls_cache_expiry = expiry;
    SSL_CTX_set_timeout(socket_context->tls_context->openssl_context, socket_context->tls_context->tls_cache_expiry);
    return 0;

}

int tls_context_set_options(socket_context_t *socket_context, long options) {

    if (!socket_context || !socket_context->tls_context || !socket_context->tls_context->openssl_context) {

        return -1;

    }

    socket_context->tls_context->tls_options = options;
    SSL_CTX_set_options(socket_context->tls_context->openssl_context, socket_context->tls_context->tls_options);
    return 0;

}

int tls_context_append_options(socket_context_t *socket_context, long options) {

    if (!socket_context || !socket_context->tls_context || !socket_context->tls_context->openssl_context || !socket_context->tls_context->tls_options) {

        return -1;

    }

    socket_context->tls_context->tls_options |= options;
    SSL_CTX_set_options(socket_context->tls_context->openssl_context, socket_context->tls_context->tls_options);
    return 0;

}

int tls_context_set_minimum_version(socket_context_t *socket_context, int minimum_version) {

    if (!socket_context || !socket_context->tls_context || !socket_context->tls_context->openssl_context) {

        return -1;

    }

    socket_context->tls_context->minimum_tls_version = minimum_version;
    SSL_CTX_set_min_proto_version(socket_context->tls_context->openssl_context, minimum_version);
    return 0;

}

int tls_context_set_maximum_version(socket_context_t *socket_context, int maximum_version) {

    if (!socket_context || !socket_context->tls_context || !socket_context->tls_context->openssl_context) {

        return -1;

    }

    socket_context->tls_context->minimum_tls_version = maximum_version;
    SSL_CTX_set_max_proto_version(socket_context->tls_context->openssl_context, maximum_version);
    return 0;

}

void free_socket_context(socket_context_t **socket_context) {

    /* Validate our function inputs to ensure they're set properly. */
    if (!(*socket_context)) {

        return;

    }

    /* Free the TLS context, if it exists. */
    if ((*socket_context)->tls_context) {

        free_tls_context((*socket_context)->tls_context);

    }

    /* Free the context and set it to null. */
    free((*socket_context));
    socket_context = NULL;
    return;

}



void free_tls_context(tls_context_t *tls_context) {

    /* Validate our function inputs to ensure they're set properly. */
    if (!tls_context) {

        return;

    }

    /* Free our OpenSSL data if it exists. */
    if (tls_context->openssl_context) {

        SSL_CTX_free(tls_context->openssl_context);

    }

    if (tls_context->openssl_bio) {

        BIO_free(tls_context->openssl_bio);

    }

    /* Free the context and set it to null. */
    free(tls_context);
    tls_context = NULL;
    return;

}

