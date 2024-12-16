//
// Created by sheverdin on 10/17/23.
//

#ifndef DM_CLIENT_SETTINGS_MODULE_H
#define DM_CLIENT_SETTINGS_MODULE_H

#include "dm_mainHeader.h"

#define SET_CMD_LEN             (128)

typedef enum
{
    SETTINGS_DEFAULT_ALL        = 0,
    SETTINGS_REBOOT_ALL         = 1,
    SETTINGS_IPADDRESS          = 2,
    SETTINGS_NETMASK            = 3,
    SETTINGS_GATEWAY            = 4,
    SETTINGS_SYSTEM_NAME        = 5,
    SETTINGS_SYSTEM_LOCATION    = 6,
    SETTINGS_SNMP               = 7,
    SETTINGS_SYS_RELOAD         = 8,
    SETTINGS_NET_RELOAD         = 9,
    SETTINGS_MAX_CMD            = 10
}SETTINGS_CMD;

typedef struct
{
    uint8_t setDHCP;
    uint8_t applyNetwork;
    uint8_t applySystem;
}SETTINGS_INFO_t;

typedef struct
{
    SETTINGS_CMD cmd;
    char  value[SETTINGS_VALUE_LENGTH];
}DYNAMIC_MAP_CMD;

typedef void (*runSettingsCmd_f) (char* value);

DYNAMIC_MAP_CMD *getDynamicMapCmd_ptr(uint8_t splitLineMax);
error_code_t getSettingsCmdArray(splited_line_t splitedLine, DYNAMIC_MAP_CMD *mapCmd);
error_code_t settings_handler(DYNAMIC_MAP_CMD *settingsCmdMap, uint8_t splitLineMax);
void setLocationTestFlag(int flag);
SETTINGS_INFO_t *getSettingInfo(void);
runSettingsCmd_f *getRunSettingsCmd(void);

#endif //DM_CLIENT_SETTINGS_MODULE_H
