#ifndef SOCKADDR_H_
#define SOCKADDR_H_

#include <stdint.h>
#include <stddef.h>

struct sockaddr *create_sockaddr(char *address, uint16_t port, size_t **sockaddr_length);
int destroy_sockaddr(struct sockaddr *sockaddr);

#endif