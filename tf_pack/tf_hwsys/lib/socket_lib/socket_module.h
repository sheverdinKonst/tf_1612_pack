//
// Created by sheverdin on 3/19/24.
//

#ifndef TF_HWSYS_SOCKET_MODULE_H
#define TF_HWSYS_SOCKET_MODULE_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <poll.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>

#define SOCKET_PATH "/var/run/hwsys-uds.sock"
#define MAX_CLIENTS 10

void socket_test(void);

typedef enum
{
    SOCKET_DISCONNECTED = 0,
    SOCKET_READ_ERROR   = 1,
    SOCKET_READ_OK      = 2,
    SOCKET_RESET        = 3,
    SOCKET_IDLE         = 4
}CLIENT_STATE;

void socket_client(int *sock_fd);
void socket_asyncServer(int *socket_fd);
void select_socket(fd_set *read_fds, int sock1, int sock2);
void accept_socket(int server_fd, int *client_fd);
void send_msg(int socket, unsigned char *msg, uint8_t size);
CLIENT_STATE receive_msg(int socket_fd, unsigned char *msg);





#endif //TF_HWSYS_SOCKET_MODULE_H
