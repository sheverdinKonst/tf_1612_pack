//
// Created by sheverdin on 7/8/24.
//

#ifndef TF_AUTORESTART_CLIENT_MODULE_H
#define TF_AUTORESTART_CLIENT_MODULE_H

#include "../global_includs.h"
#include "socket_lib/socket_module.h"

SOCKET_ERR_t ar_ctrl_create_socket(int *uds_socket);
void ctrl_sendCmd(const int *sock_fd, sock_msg_t *msg);
CLIENT_STATE ctrl_getresult(const int *uds_socket, unsigned char *resetMsg);

#endif //TF_AUTORESTART_CLIENT_MODULE_H
