#ifndef SOCKADDR_H_
#define SOCKADDR_H_

#include <stdint.h>

struct sockaddr *create_sockaddr(char *address, uint16_t port);
void destroy_sockaddr(struct sockaddr *sockaddr);

#endif