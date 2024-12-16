//
// Created by sheverdin on 10/3/23.
//

#include <pthread.h>
#include <arpa/inet.h>
#include "net_utils.h"
#include "run_CLIcmd.h"

static void error(char *message);
static uint8_t net_closeSocket(int socket);
static void test_natData(char * devName);

SWITCH_NET_PARAM switchNetParam;
SOCKET_PARAM_t socket_rcv =
{
    .addr_len = sizeof(socket_rcv.client_addr),
};

char buffer[MAX_BUFFER_SIZE];
ssize_t  bufferLen = 0;

CONFIG_PARAM_t configParam;

uint8_t net_socketInit(void)
{
    error_code_t errorCode = getNetInfo(&switchNetParam, configParam.deviceName);
    test_natData(configParam.deviceName);

    printf(">>>>>>>>>>>>> net_socketInit\n");
    memset(&socket_rcv.client_addr, 0, socket_rcv.addr_len);
    socket_rcv.client_addr.sin_family = AF_INET;
    socket_rcv.client_addr.sin_port = htons(PORT);
    socket_rcv.client_addr.sin_addr.s_addr =  htonl(INADDR_ANY);

    if ((socket_rcv.socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("error create socket: ");
        syslog(LOG_ERR, "Socket creation failed");
        return ERR_NETWORK;
    }
    int reuse = 1;
    if (setsockopt(socket_rcv.socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        perror("error SO_REUSEADDRt: ");
        syslog(LOG_ERR, "Setting SO_REUSEADDR option failed");
        close(socket_rcv.socket_fd);
        return ERR_NETWORK;
    }

    printf("bind to dev - %s\n", configParam.deviceName);
    openlog("tfdm", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "socket init -- bind to dev - %s", configParam.deviceName);
    closelog();
    if (setsockopt(socket_rcv.socket_fd, SOL_SOCKET, SO_BINDTODEVICE, configParam.deviceName, strlen(configParam.deviceName)) < 0)
    {
        perror("error SO_BINDTODEVICE: ");
        return ERR_NETWORK;
    }

    if (bind(socket_rcv.socket_fd, (const struct sockaddr*)&socket_rcv.client_addr, socket_rcv.addr_len) < 0)
    {
        perror("error bind socket: ");
        syslog(LOG_ERR, "Binding failed");
        close(socket_rcv.socket_fd);
        return ERR_NETWORK;
    }

    openlog("tfdm", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "Client listening on port %d...\n", PORT);
    closelog();
    printf("net_socketInit - ERROR %d\n", errorCode);
    return errorCode;
}

uint8_t net_getCommand(void)
{
    printf("net_getCommand\n");
    uint8_t msgType;
    uint8_t senderIP[IPV4_LEN] = {0,0,0,0};
    bufferLen = recvfrom(socket_rcv.socket_fd, (char*)buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*)&socket_rcv.client_addr, &socket_rcv.addr_len);
    senderIP[3] = (socket_rcv.client_addr.sin_addr.s_addr)&(0xFF);
    senderIP[2] = (socket_rcv.client_addr.sin_addr.s_addr>>8*1)&(0xFF);
    senderIP[1] = (socket_rcv.client_addr.sin_addr.s_addr>>8*2)&(0xFF);
    senderIP[0] = (socket_rcv.client_addr.sin_addr.s_addr>>8*3)&(0xFF);

    if (bufferLen < 0)
    {
        perror("error recvfrom: ");
    }
    buffer[bufferLen] = '\0';
    msgType = buffer[0]&0xFF;
    openlog("tfdm", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "Client listening on port %d...\n", PORT);
    printf("Received Msg from: %d.%d.%d.%d -- msg Type: %X\n", senderIP[0], senderIP[1], senderIP[2], senderIP[3], msgType);
    return msgType;
}

char *getBufferAddress(void)
{
    return buffer;
}

void getBuffer(uint8_t *settingsBuffer, uint32_t index)
{
    if(bufferLen > 0)
    {
        memcpy(settingsBuffer, buffer+index, bufferLen-index);
    }
}

void getBufferLen(size_t *len)
{
    *len = bufferLen-39;
}

uint8_t net_sendMsg(void)
{
    int broadcastEnable = 1;
    struct sockaddr_in client_addr_1;
    int sockSend_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockSend_fd < 0)
    {
        perror("error create sockSend_fd: ");
        openlog("tfdm", LOG_PID | LOG_PERROR, LOG_USER);
        syslog(LOG_ERR, "Failed to create socket Send");
        closelog();
        return ERR_NETWORK;
    }

    if (setsockopt(sockSend_fd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0)
    {
        perror("error SO_BROADCAST: ");
        openlog("tfdm", LOG_PID | LOG_PERROR, LOG_USER);
        syslog(LOG_ERR, "Failed to enable broadcast");
        closelog();
        return ERR_NETWORK;
    }

    printf("bind to dev - %s\n", configParam.deviceName);
    openlog("tfdm", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "bind to dev - %s", configParam.deviceName);
    closelog();
    if (setsockopt(sockSend_fd, SOL_SOCKET, SO_BINDTODEVICE, configParam.deviceName, strlen(configParam.deviceName)) < 0)
    {
        perror("send msg error SO_BINDTODEVICE: ");
        openlog("tfdm", LOG_PID | LOG_PERROR, LOG_USER);
        syslog(LOG_ERR, "send msg error SO_BINDTODEVICE: ");
        closelog();
        return ERR_NETWORK;
    }

    memset(&client_addr_1, 0, sizeof(client_addr_1));
    client_addr_1.sin_family = AF_INET;
    client_addr_1.sin_port = htons(PORT);
    client_addr_1.sin_addr.s_addr = inet_addr("255.255.255.255");

    ssize_t sendSize = 0;
    sendSize = sendto(sockSend_fd, buffer, bufferLen, 0, (struct sockaddr *)&client_addr_1, sizeof(client_addr_1));
    closelog();

    if (sendSize < 0)
    {
        perror("error send to: ");
        openlog("tfdm", LOG_PID | LOG_PERROR, LOG_USER);
        syslog(LOG_ERR, "Error send massage");
        closelog();
        return ERR_NETWORK;
    }
    else
    {
        printf("massage was send successful\n");
        openlog("tfdm", LOG_PID, LOG_USER);
        syslog(LOG_INFO, "massage was send successful");
        closelog();
    }

    net_closeSocket(sockSend_fd);
    return EXIT_SUCCESS;
}

static uint8_t net_closeSocket(int socket)
{
    if (close(socket) < 0)
    {
        syslog(LOG_ERR, "Error socket close");
        return (ERR_NETWORK);
    }
    return EXIT_SUCCESS;
}

static void printError(char *message)
{
    perror(message);
    //exit(EXIT_FAILURE);
}

static void test_natData(char *devName)
{
    printf("ifName: %s\n", switchNetParam.ifName);
    printf("deviceName: %s\n", switchNetParam.deviceName);
    printf("MAC: ");
    for (int i = 0; i < 6; i++) {
        printf("%X:", switchNetParam.mac_addr[i]);
    }
    printf("\nIP: ");
    for (int i = 0; i < 4; i++) {
        printf("%d.", switchNetParam.ip_addr[i]);
    }
    printf("\nMASK: ");
    for (int i = 0; i < 4; i++) {
        printf("%d.", switchNetParam.net_mask[i]);
    }
    printf("\nGATEWAY: ");
    for (int i = 0; i < 4; i++) {
        printf("%d.", switchNetParam.getWay[i]);
    }
    printf("\n");
}

void set_DevName(char *inputName)
{
    strcpy(configParam.deviceName,"");
    strcpy(configParam.deviceName, inputName);
    strcat(configParam.deviceName, "\0");
    printf("configParam.deviceName = %s -- len = %lu\n", configParam.deviceName, strlen(configParam.deviceName));
}

void get_devName(char *DevName)
{
    strcpy(DevName,"");
    strcpy(DevName, configParam.deviceName);
    strcat(DevName, "\0");
    printf("DevName = %s -- len = %lu\n",DevName, strlen(DevName));
}

void fillNetInfo(search_out_msg_t *searchOutMsg)
{
    printf("fillNetInfo\n");
    setNetInfo(searchOutMsg, &switchNetParam);
}
