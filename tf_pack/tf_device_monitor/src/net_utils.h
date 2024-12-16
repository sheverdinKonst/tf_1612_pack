//
// Created by sheverdin on 10/3/23.
//

#ifndef UNTITLED_NET_UTILS_H
#define UNTITLED_NET_UTILS_H

#include "dm_mainHeader.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>

#define PORT (6123)

typedef struct
{
    struct sockaddr_in client_addr;
    int socket_fd;
    socklen_t addr_len ;
}SOCKET_PARAM_t;

typedef struct
{
    char ifName[16];
    char deviceName[16];
    char mngtDevice[16];
}CONFIG_PARAM_t;

uint8_t net_socketInit(void);
uint8_t net_getCommand(void);
uint8_t net_sendMsg(void);
char *getBufferAddress(void);
void getBuffer(uint8_t *settingsBuffer, uint32_t index);
void getBufferLen(size_t *len);
void get_devName(char *DevName);
void set_DevName(char *inputName);
void fillNetInfo(search_out_msg_t *searchOutMsg);
#endif //UNTITLED_NET_UTILS_H
