//
// Created by sheverdin on 10/3/23.
//

#ifndef DM_CLIENT_DM_MAINHEADER_H
#define DM_CLIENT_DM_MAINHEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>

#define FW_VERSION_RESOLUTION   (4)
#define FW_VERSION_MAXBIT       (FW_VERSION_RESOLUTION -2)
#define DM_UDP_PORT             (6123)
#define PORT_NUM                (16)
#define MAC_LEN                 (6)
#define IPV4_LEN                (4)
#define VER_LEN                 (4)
#define MD5_LEN                 (32)
#define MAX_LENGTH              (128)
#define MAX_BUFFER_SIZE         (1550)
#define SETTINGS_MD5_LEN        (32)
#define SETTINGS_MSG_LEN        (1024)
#define SETTINGS_VALUE_LENGTH   (128)
#define OUTPUT_MAX_LENGTH       (128)


#define GLOBAL_TEST

#ifdef GLOBAL_TEST
    #define IP_TEST
    #define MASK_TEST
    #define GATE_TEST
    #define MAC_TEST
    #define FIRMWARE_TEST
    #define DEV_DESCR_TEST
    #define DEV_TYPE_TEST
    #define DEV_LOCATION_TEST
    #define UPTIME_TEST
#endif

typedef enum
{
    ERR_OK                     = 0,
    ERR_AUTHENTICATION         = 1,
    ERR_CONFIGURATION          = 2,
    ERR_OBJ_NOT_FOUND          = 3,
    ERR_UNAVAILABLE_RESOURCE   = 4,
    ERR_NETWORK                = 5,
    INFO_MAC_NOT_IDENTICAL     = 6,
}error_code_t;

typedef enum
{
    pass_mac = 0x001,
    pass_md5 = 0x002
}pass_id_e;

typedef enum
{
    request_find_device = 0xE0,
    ack_find_device     = 0xE1,
    request_set_config  = 0xE2,
    ack_to_set_config   = 0xE3,
    request_to_update   = 0xE4,
    ack_to_update       = 0xE5,
    max_msg_type        = 0xE6
}msg_type_e;

typedef enum
{
    type_idle        = 0,
    type_searchMsg   = 1,
    type_settingsMsg = 2,
    type_reload      = 3,
    type_maxMsg      = 4
}TYPE_MSG_e;

typedef struct
{
    uint8_t mac[MAC_LEN];
    uint8_t ip[IPV4_LEN];
}search_mac_entry_t;

typedef union
{
    struct
    {
        uint8_t msg_type;                       //  done
        uint8_t dev_type;                       //  done
        uint8_t ip[IPV4_LEN];                   //  done
        uint8_t mac[MAC_LEN];                   //  done
        char dev_descr[MAX_LENGTH];             //  done
        char dev_loc[MAX_LENGTH];               //  done
        uint8_t uptime[4];                      //  done
        uint8_t firmware[FW_VERSION_RESOLUTION];                    //  done
        uint8_t mask[IPV4_LEN];                 //  done
        uint8_t gate[IPV4_LEN];                 //  done
        search_mac_entry_t mac_entry[PORT_NUM];
    }struct1;
    uint8_t buff[444];
}search_out_msg_t;

typedef union
{
    struct
    {
        uint8_t msg_type;
        char mac[MAC_LEN];
        char pass_md5[MD5_LEN];
        char msg[1024];
    }settings_struct;
    char buff[1+6+32+1024];
}sett_msg_t;

typedef struct
{
    char **tokens;
    uint8_t splitLineMax;
}splited_line_t;

typedef struct
{
    sett_msg_t settMsg;
    size_t len;
}SET_SETTINGS_MSG;

typedef enum
{
    psw_2G              = 1,
    psw_2G_UPS          = 2,
    psw_2G_plus         = 3,
    psw_1G4F            = 4,
    psw_2G4F            = 5,
    PSW_2G6F_plus       = 6,
    PSW_2G8F_plus       = 7,
    PSW_1G4F_UPS        = 8,
    PSW_2G4F_UPS        = 9,
    PSW_2G_plus_UPS_Box = 10,
    MAX_DEVICE_TYPE
}device_type_e;

typedef enum
{
    CMD_NET_INFO             = 0,
    CMD_TIME_INFO            = 1,
    CMD_BOARD_INFO           = 2,
    CMD_SYSTEM_INFO          = 3,
    CMD_MAX                 = 4
}cmd_type_e;

#endif //DM_CLIENT_DM_MAINHEADER_H
