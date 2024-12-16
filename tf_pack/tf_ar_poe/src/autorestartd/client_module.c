//
// Created by sheverdin on 6/24/24.
//

#include "string.h"
#include "utils_lib/utils_module.h"
#include "client_module.h"
#include "port_module.h"

uint8_t isRecivedMsg = 0;
resetPort_U resetPortBuf;

typedef void (*cmdHandler_t) (int fd, uint8_t port);

static void cmd_sendStatus(int fd, uint8_t port);
static void cmd_rebootHandler(int fd, uint8_t port);
static void d_sendCmd(const int *sock_fd, resetPort_U *msg);

cmdHandler_t cmdHandler[max_cmd] =
{
    NULL,
    &cmd_sendStatus,
    &cmd_rebootHandler
};

CLIENT_STATE clientHandler(int fd)
{
    isRecivedMsg = 0;
    sock_msg_t clientMsg;
    CLIENT_STATE clientState = SOCKET_IDLE;
    clientMsg.msg_s.cmdType   = 0,
    clientMsg.msg_s.port      = 0;
    clientState = d_receive_msg(fd, clientMsg.arr);
    if (clientState == SOCKET_READ_OK)
    {
        isRecivedMsg = 1;
        if (cmdHandler[clientMsg.msg_s.cmdType] == NULL)
        {
            printf("OBJECT NOT FOUND\n");
        }
        else
        {
            cmdHandler[clientMsg.msg_s.cmdType](fd, clientMsg.msg_s.port);
        }
    }
    return clientState;
}

uint8_t *is_ReceivedMsg(void)
{
    return &isRecivedMsg;
}

void set_notRecivedMsg(void)
{
    isRecivedMsg = 0;
}

static void cmd_sendStatus(int fd, uint8_t port)
{
    portInfo_t *portInfoArr = port_get_portInfoArr();
    for (int portNum = 0; portNum < NUM_PORTS; portNum++)
    {
       resetPortBuf.resetPort[portNum].errorCode     = portInfoArr[portNum].portResetInfo.errorCode;
       resetPortBuf.resetPort[portNum].portNum       = portInfoArr[portNum].portConfigInfo.portNum;
       resetPortBuf.resetPort[portNum].resetCount    = portInfoArr[portNum].portResetInfo.totalResetCount;
       resetPortBuf.resetPort[portNum].testType      = portInfoArr[portNum].portConfigInfo.test;
       resetPortBuf.resetPort[portNum].status        = 0;
       strcpy(resetPortBuf.resetPort[portNum].timeStr, "");
       strcat(resetPortBuf.resetPort[portNum].timeStr, portInfoArr[portNum].portConfigInfo.timeStr);
       strcat(resetPortBuf.resetPort[portNum].timeStr, "\0");
    }

    //uint8_t *isDebug = isDebugMode();
    //if(*isDebug)
    //{
    //    for (int portNum = 0; portNum < NUM_PORTS; portNum++)
    //    {
    //        printf("========== portInfoArr ============= \n");
    //
    //        printf("errorCode       = %d\n", portInfoArr[portNum].portResetInfo.errorCode);
    //        printf("portNum         = %d\n", portInfoArr[portNum].portConfigInfo.portNum);
    //        printf("totalResetCount = %d\n", portInfoArr[portNum].portResetInfo.totalResetCount);
    //        printf("test            = %d\n", portInfoArr[portNum].portConfigInfo.test);
    //        printf("timeStr         = %s\n", portInfoArr[portNum].portConfigInfo.timeStr);
    //        printf("-------------- resetPortBuf ----------------\n");
    //        printf("errorCode       = %d\n", resetPortBuf.resetPort[portNum].errorCode);
    //        printf("portNum         = %d\n", resetPortBuf.resetPort[portNum].portNum);
    //        printf("totalResetCount = %d\n", resetPortBuf.resetPort[portNum].resetCount);
    //        printf("test            = %d\n", resetPortBuf.resetPort[portNum].testType);
    //        printf("timeStr         = %s\n", resetPortBuf.resetPort[portNum].timeStr);
    //    }
    //}
    //printf("size of resetPortBuf = %ld\n", sizeof(resetPortBuf));
    //printf("size of resetPort_U = %ld\n", sizeof(resetPort_U));
    d_sendCmd(&fd,  &resetPortBuf);
}

static void cmd_rebootHandler(int fd, uint8_t port)
{

    portInfo_t *portInfoArr = port_get_portInfoArr();
    resetPortBuf.resetPort[port-1].status = 0;

    if (portInfoArr[port - 1].portResetInfo.state != REBOOT_STATE)
    {
        if (portInfoArr[port - 1].portStateInfo.poe_state == enable
            && portInfoArr[port - 1].portStateInfo.port_state == enable)
        {
            portInfoArr[port - 1].portResetInfo.state = REBOOT_STATE;
            portInfoArr[port - 1].portResetInfo.event |= MANUAL_RESTART;
            openlog("tf_autoresart", LOG_PID, LOG_USER);
            syslog(LOG_INFO, "cmd manual reboot: port - %d", port);
            closelog();
            resetPortBuf.resetPort[port - 1].status = 1;
        }
        else
        {
            resetPortBuf.resetPort[port - 1].status = 3;
        }
    }
    else
    {
        portInfoArr[port - 1].portResetInfo.event &= (~MANUAL_RESTART);
        openlog("tf_autoresart", LOG_PID, LOG_USER);
        syslog(LOG_INFO, "Manual reboot disable, port - %d in rebooting sate", port);
        closelog();
        resetPortBuf.resetPort[port-1].status = 2;
    }
    resetPortBuf.resetPort[port - 1].errorCode  = portInfoArr[port - 1].portResetInfo.errorCode;
    resetPortBuf.resetPort[port - 1].portNum    = portInfoArr[port - 1].portConfigInfo.portNum;
    resetPortBuf.resetPort[port - 1].resetCount = portInfoArr[port - 1].portResetInfo.totalResetCount;
    resetPortBuf.resetPort[port - 1].testType   = portInfoArr[port - 1].portConfigInfo.test;
    strcpy(resetPortBuf.resetPort[port - 1].timeStr, "");
    strcat(resetPortBuf.resetPort[port - 1].timeStr, portInfoArr[port - 1].portConfigInfo.timeStr);
    strcat(resetPortBuf.resetPort[port - 1].timeStr, "\0");

    d_sendCmd(&fd,  &resetPortBuf);
}

static void d_sendCmd(const int *sock_fd, resetPort_U *msg)
{
    d_send_msg(*sock_fd,  msg->arr, sizeof(msg->arr));
}