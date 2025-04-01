#ifndef TLS_H_
#define TLS_H_

SSL_CTX *create_tls_server_context(char *chain_certificate_file, char *certificate_file, char *private_key_file, int cache_id);
void destroy_tls_context(SSL_CTX *context);

#endif