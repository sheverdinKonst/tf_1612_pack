//
// Created by sheverdin on 6/24/24.
//

#ifndef TF_AR_POE_GLOBAL_INCLUDE_H
#define TF_AR_POE_GLOBAL_INCLUDE_H

#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

#define VERSION             ("1.0.0")
#define NUM_PORTS           (8)
#define OUTPUT_MAX_LENGTH   (64)
#define WORD_LENGTH         (10)

#define MIN_SPEED           (10)
#define MAX_SPEED           (1000000)

typedef enum
{
    disable  = 0,
    enable   = 1,
    max_bool = 2
}bool_t;

typedef enum
{
    test_disable                = 0,
    test_link                   = 1,
    test_ping                   = 2,
    test_speed                  = 3,
    test_max                    = 4
}TYPE_OF_TEST_t;

typedef enum
{
    ERR_PORT_DISABLE            = 0x00001,
    ERR_TEST_TYPE               = 0x00002,
    ERR_TEST_LINK               = 0x00004,
    ERR_TEST_PING               = 0x00008,
    ERR_TEST_SPEED              = 0x00010,
    ERR_TIME_ALARM              = 0x00020,
    ERR_TEST_DISABLE            = 0x00040,
    ERR_POE_DOWN                = 0x00080,
    ERR_UNAVAILABLE_RESOURCE     = 0x00100,
    ERR_NULL_OBJ                = 0x00200,
    ERR_CREATE_THREAD           = 0x00400,
    ERR_JOIN_THREAD             = 0x00800,
    ERR_IP_NOT_VALID            = 0x01000,
    ERR_PoE_DISABLE             = 0x02000,
    ERR_TIME_NOT_VALID          = 0x04000,
    ERR_SPEED_VALUE             = 0x08000,
    ERR_OK                     = 0x10000,
    ERR_PORT_SHUTDOWN           = 0x20000,
    ERR_MANUAL_REBOOT           = 0x40000,
    ERR_REBOOTING               = 0x80000
}error_code_t;

typedef enum
{
    CTRL_ERROR_OK       = 0x01,
    CTRL_ERROR_DEBUG    = 0x02,
    CTRL_ERROR_INACTIVE = 0x04,
    CTRL_ERROR_SOCKET   = 0x08,
    CTRL_ERROR_ARG_1    = 0x10,
    CTRL_ERROR_ARG_2    = 0x20,
    CTRL_ERROR_ARG_3    = 0x40,
}AR_CTRL_ERROR;


typedef enum
{
    cmd_idle    = 0,
    cmd_status  = 1,
    cmd_reboot  = 2,
    cmd_error   = 3,
    max_cmd     = 4
}CMD_TYPE_e;

typedef union
{
    struct
    {
        CMD_TYPE_e cmdType;
        uint8_t port;
    }msg_s;
    unsigned char arr[5];
}sock_msg_t;

typedef struct
{
    error_code_t errorCode;
    uint8_t status;
    uint8_t portNum;
    uint8_t resetCount;
    uint32_t time;
    TYPE_OF_TEST_t testType;
    char timeStr[32];
}resetPort_t;

typedef  union
{
    struct {
        AR_CTRL_ERROR arCtrlError;
        resetPort_t resetPort[NUM_PORTS];
    };
    unsigned char arr[52*NUM_PORTS];
}resetPort_U;

#endif //TF_AR_POE_GLOBAL_INCLUDE_H
