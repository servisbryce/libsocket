#ifndef SOCKET_H_
#define SOCKET_H_

#include <sys/socket.h>

int create_socket(struct sockaddr *sockaddr);
void destroy_socket(int socket_file_descriptor);

#endif