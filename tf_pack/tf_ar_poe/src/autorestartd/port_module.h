//
// Created by sheverdin on 6/25/24.
//

#ifndef TF_AR_POE_PORT_MODULE_H
#define TF_AR_POE_PORT_MODULE_H

#include "../global_includs.h"

typedef enum
{
    time_up         = 0,
    time_down       = 1,
    time_alarm_max  = 2
}TIME_ALARM_INDEX_e;

typedef struct
{
    bool_t  status;
    uint8_t time_H;
    uint8_t time_M;
    uint32_t targetTime;
    uint32_t remainTime;
}time_h_m;

typedef enum
{
    IDLE_STATE       = 0,
    REGULAR_STATE    = 1,
    REBOOT_STATE     = 2,
    MAX_AR_STATE
}AR_STATE;

typedef enum
{
    IDLE_EVENT       = 0x00,
    TEST_OK          = 0x01,
    AUTO_RESTART     = 0x02,
    MANUAL_RESTART   = 0x04,
}AR_EVENT;

typedef enum
{
    REBOOT_STATE_IDLE        = 0,
    REBOOT_STATE_INIT        = 1,
    REBOOT_STATE_POE_DOWN    = 2,
    REBOOT_STATE_POE_UP      = 3
}REBOOT_STATE_e;

typedef enum
{
    REBOOT_EVENT_IDLE         = 0x00,
    REBOOT_EVENT_START        = 0x01,
    REBOOT_EVENT_TIMER_STOP   = 0x02,
}REBOOT_EVENT_e;

typedef struct
{
    char            host[16];
    time_h_m        timeAlarm[time_alarm_max];
    uint32_t        min_speed;
    uint32_t        max_speed;
    uint32_t        reply_time;
    uint32_t        rx_byte_prev;
    uint32_t        rx_byte_current;
    uint32_t        rx_delta_byte;
    uint32_t        rx_speed_Kbit;
    uint32_t        time_current;
    uint32_t        time_prev;
    uint32_t        tx_byte_prev;
    uint32_t        tx_byte_current;
    uint32_t        tx_delta;
    bool_t          alarm;
    uint8_t         portNum;
    TYPE_OF_TEST_t  test;
    uint8_t         poe_state;
    char            timeStr[32];
}port_config_info_t;

typedef struct {
    uint8_t         resetCount;
    uint8_t         totalResetCount;
    uint8_t         errorTestCount;
    error_code_t    errorCode;
    AR_EVENT        event;
    AR_STATE        state;
}port_reset_info_t;

typedef struct
{
    REBOOT_STATE_e  rebootState;
    REBOOT_EVENT_e  rebootEvent;
    uint32_t        rebootDelay;
    uint32_t        rebootTimeStart;
}port_reboot_info_t;

typedef struct {
    bool_t         linkState;
    bool_t         port_state;
    bool_t         poe_state;
}port_state_info_t;

typedef struct
{
    port_config_info_t portConfigInfo;
    port_reset_info_t  portResetInfo;
    port_state_info_t  portStateInfo;
    port_reboot_info_t portRebootInfo;
}portInfo_t;

typedef enum
{
    POE_UP      = 0x01,
    POE_DOWN    = 0x02,
    POE_RESTART = 0x03
}POE_CONTROL;

error_code_t port_setLinkParam(port_config_info_t *portConfig);
error_code_t port_setHostParam(port_config_info_t *portConfig, char * host);
error_code_t port_setSpeedParam(port_config_info_t *portConfig, uint32_t max, uint32_t min);
error_code_t port_setTimeParam(port_config_info_t *portConfig, char* timeUp, char *timeDown);

error_code_t port_set_PortReboot(port_reboot_info_t *portRebootInfo, uint8_t portNum);

portInfo_t *port_get_portInfoArr(void);
AR_STATE get_PortAndPoeState(uint8_t portNum);

void autoResetHandler(uint8_t portNum, uint8_t maxReset);
void manualResetHandler(uint8_t portINum);

void port_runTestDisable(uint8_t portNum);
void port_runTestLink(uint8_t portNum);
void port_runTestPing(uint8_t portNum);
void port_runTestSpeed(uint8_t portNum);
void port_runTestTime(uint8_t portNum);

void rebootParamInit(void);
void poe_Control(uint8_t port, POE_CONTROL poeControl);
void set_errorHandler(uint8_t index);

#endif //TF_AR_POE_PORT_MODULE_H
