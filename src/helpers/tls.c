#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

SSL_CTX *create_tls_server_context(char *chain_certificate_file, char *certificate_file, char *private_key_file, int cache_id) {

    if ((!chain_certificate_file && !certificate_file) || (chain_certificate_file && certificate_file) || !private_key_file) {

        return NULL;

    }

    SSL_CTX *context = NULL;
    if (!(context = SSL_CTX_new(TLS_server_method()))) {

        return NULL;

    }

    if(!SSL_CTX_set_min_proto_version(context, TLS1_2_VERSION)) {

        SSL_CTX_free(context);
        return NULL;

    }

    SSL_CTX_set_options(context, SSL_OP_IGNORE_UNEXPECTED_EOF | SSL_OP_NO_RENEGOTIATION | SSL_OP_CIPHER_SERVER_PREFERENCE);
    if (chain_certificate_file) {

        if (SSL_CTX_use_certificate_chain_file(context, chain_certificate_file) <= 0) {

            SSL_CTX_free(context);
            return NULL;

        }

    } else {

        if (SSL_CTX_use_certificate_file(context, certificate_file, SSL_FILETYPE_PEM) <= 0) {

            SSL_CTX_free(context);
            return NULL;

        }

    }

    if (SSL_CTX_use_PrivateKey_file(context, private_key_file, SSL_FILETYPE_PEM) <= 0) {

        SSL_CTX_free(context);
        return NULL;

    }

    SSL_set_session_id_context(context, (void *) &cache_id, sizeof(cache_id));
    SSL_CTX_set_session_cache_mode(context, SSL_SESS_CACHE_SERVER);
    SSL_CTX_sess_set_cache_size(context, 32768);
    SSL_CTX_set_timeout(context, 1800);
    SSL_CTX_set_verify(context, SSL_VERIFY_NONE, NULL);
    return context;

}

void destroy_tls_context(SSL_CTX *context) {

    if (!context) {

        return;

    }

    SSL_CTX_free(context);
    return;

}