#ifndef SOCKADDR_H
#define SOCKADDR_H

#include <sys/socket.h>
#include <stdint.h>

int create_sockaddr(char *address, uint16_t port, struct sockaddr **sockaddr);

#endif