//
// Created by sheverdin on 6/24/24.
//

#ifndef TF_SOCKET_MODULE_H
#define TF_SOCKET_MODULE_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>

#define SOCKET_PATH "/var/run/ar_uds.sock"
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

typedef enum
{
    SOCKET_ERR_CREATE  = 0x01,
    SOCKET_ERR_CONNECT = 0x02,
    SOCKET_ERR_READ    = 0x04,
    SOCKET_ERR_OK      = 0x08
}SOCKET_ERR_t;

SOCKET_ERR_t socket_client(int *socket_fd);
void socket_asyncServer(int *socket_fd);
void select_socket(fd_set *read_fds, int sock1, int sock2);
void accept_socket(int server_fd, int *client_fd);
void send_msg(int socket, unsigned char *msg, uint16_t size);
void ctrl_send_msg(int socket, unsigned char *msg, uint16_t size);
void d_send_msg(int socket, unsigned char *msg, uint16_t size);

CLIENT_STATE receive_msg(int socket_fd, unsigned char *msg);
CLIENT_STATE d_receive_msg(int socket_fd, unsigned char *rec_msg);
CLIENT_STATE ctrl_receive_msg(int socket_fd, unsigned char *rec_msg,  uint16_t nbytes);

#endif //TF_SOCKET_MODULE_H
