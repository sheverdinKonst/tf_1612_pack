//
// Created by sheverdin on 10/11/23.
//

#ifndef DM_CLIENT_STATE_HANDLER_H
#define DM_CLIENT_STATE_HANDLER_H

#include "dm_mainHeader.h"

#define CLIENT_FILLMSG_NUM_OF_CALLBACKS  (5)
#define CLIENT_SENDMSG_NUM_OF_CALLBACKS  (type_maxMsg)
#define SETTING_MSG_INDEX                (39)

typedef enum
{
    CLIENT_SATE_IDLE        = 0,
    CLIENT_SATE_INIT        = 1,
    CLIENT_STATE_WAIT       = 2,
    CLIENT_STATE_FILL_MSG   = 3,
    CLIENT_STATE_SEND       = 4,
    CLIENT_STATE_SETTINGS   = 5,
    CLIENT_STATE_RELOAD     = 6,
    CLIENT_STATE_MAX        = 7
}CLIENT_STATE_e;

typedef enum
{
    CLIENT_EVENT_IDLE       = 0x0000,
    CLIENT_EVENT_INIT       = 0x0001,
    CLIENT_EVENT_FIND       = 0x0002,
    CLIENT_EVENT_SET        = 0x0004,
    CLIENT_EVENT_MSG_READY  = 0x0008,
    CLIENT_EVENT_SETTINGS   = 0x0010,
    CLIENT_EVENT_UPDATE     = 0x0020,
    CLIENT_EVENT_NET_RELOAD = 0x0040,
    CLIENT_EVENT_SYS_RELOAD = 0x0080,
    CLIENT_EVENT_MAX        = 0x0100,
}CLIENT_EVENT_e;

typedef struct
{
    CLIENT_STATE_e state;
    CLIENT_EVENT_e event;
    TYPE_MSG_e     msgType;
    uint32_t       count;
}clientInfo_t;

typedef void (*event_FillMsg_Callback_t)(search_out_msg_t *outMsg);
typedef error_code_t (*event_SendMsg_Callback_t)(void);

error_code_t client_app(void);
void getMainBuff(uint8_t *mainBuff);
void setMainTestFlag(uint8_t value);

#endif //DM_CLIENT_STATE_HANDLER_H
