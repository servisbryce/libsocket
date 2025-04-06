#ifndef SOCKET_H_
#define SOCKET_H_

#include <sys/socket.h>

int create_socket(struct sockaddr *sockaddr, long timeout);
int destroy_socket(int socket_file_descriptor);

#endif