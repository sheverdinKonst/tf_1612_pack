//
// Created by sheverdin on 10/3/23.
//

#ifndef DM_CLIENT_RUN_CLICMD_H
#define DM_CLIENT_RUN_CLICMD_H

#include "dm_mainHeader.h"

#define COMMAND_MAX_LENGTH 100

#define DM_TOK_DELIM "\t\r\n\a"

typedef struct
{
    char ifName[16];
    char deviceName[16];
    uint8_t mac_addr[6];
    uint8_t ip_addr[IPV4_LEN];
    uint8_t net_mask[IPV4_LEN];
    uint8_t getWay[IPV4_LEN];
}SWITCH_NET_PARAM;

error_code_t getNetInfo(SWITCH_NET_PARAM *switchNetParam, char *devName);
void setNetInfo(search_out_msg_t *searchOutMsg, SWITCH_NET_PARAM *switchNetParam);
void getTimeInfo(search_out_msg_t    *searchOutMsg);
void getBoardInfo(search_out_msg_t   *searchOutMsg);
void getSystemInfo(search_out_msg_t  *searchOutMsg);

const char * getCommandTable_line(cmd_type_e cmd);


#endif //DM_CLIENT_RUN_CLICMD_H
