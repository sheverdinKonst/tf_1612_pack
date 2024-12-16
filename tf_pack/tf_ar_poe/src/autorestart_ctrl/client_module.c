//
// Created by sheverdin on 7/8/24.
//

#include "socket_lib/socket_module.h"
#include "client_module.h"
#include "parsing_command.h"

SOCKET_ERR_t socketErr;
SOCKET_ERR_t ar_ctrl_create_socket(int *uds_socket)
{
    socketErr = socket_client(uds_socket);
    return socketErr;
}

void ctrl_sendCmd(const int *sock_fd, sock_msg_t *msg)
{
    // TODO Konst need to add check if argument reboot with  port num, status without port
    ctrl_send_msg(*sock_fd,  msg->arr, sizeof(msg->arr));
}

CLIENT_STATE ctrl_getresult(const int *uds_socket, unsigned char *resetMsg)
{

    //printf("Wait for msg .......  sizeof(resetPort_U) %ld\n", sizeof(resetPort_U));
    CLIENT_STATE clientState = ctrl_receive_msg(*uds_socket, resetMsg, sizeof(resetPort_U));
    //printf("..............Msg received\n");
    return clientState;
}
