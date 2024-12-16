//
// Created by sheverdin on 6/24/24.
//

#include "utils_lib/utils_module.h"
#include "init_module.h"
#include "port_module.h"

const char testTable[test_max][WORD_LENGTH] =
{
    {"disable"},    //  0
    {"link"},       //  1
    {"ping"},       //  2
    {"speed"},      //  3
};

const char boolTable[2][8] =
{
    {"disable"},    //  0
    {"enable"},     //  1
};

static error_code_t setDisable(port_config_info_t * portConfig, char * jsonData);
static error_code_t setLink(port_config_info_t * portConfig,    char * jsonData);
static error_code_t setHost(port_config_info_t * portConfig, char * jsonData);
static error_code_t setSpeed(port_config_info_t * portConfig,   char * jsonData);
static error_code_t setTime(port_config_info_t * portConfig, char * jsonData);

static void setConfigZeroValue (port_config_info_t *portConfig,  uint8_t isReboot);
static void setResetZeroValue(port_reset_info_t *portReset, uint8_t isReboot);

static TYPE_OF_TEST_t getCmdKey(const char * str);
static bool_t getAlarmSate(const char *str);

typedef error_code_t (*setTestConfig_t)(port_config_info_t * portConfig, char * jsonData);
setTestConfig_t setTestConfig[test_max] =
{
    &setDisable,
    &setLink,
    &setHost,
    &setSpeed,
};

config_type configOptionsDefault[MAX_PARM] = {
{
        .optionName = "test_delay",
        .value    = 3,
    },
{
        .optionName = "test_num",
        .value = 10,
    },
{
        .optionName = "max_reset",
        .value = 3,
    },
{
        .optionName = "main_time",
        .value      = 0
    }
};

config_type init_configOptions[MAX_PARM] = {
{
        .optionName = "test_delay",
        .value    = 0,
    },
{
        .optionName = "test_num",
        .value = 0,
    },
{
        .optionName = "max_reset",
        .value = 0,
    },
{
        .optionName = "main_time",
        .value = 0,
    }
};

void getConfig(void)
{
    FILE *pipeConfig = NULL;
    char str_cmd[64];
    char output[64];
    char res[256];

    strcpy(str_cmd, "");
    strcpy(output, "");
    strcpy(res, "");

    char str_ubusCall[] = "ubus call uci get '{\"config\":\"tf_autorestart\", \"section\":\"ar_demon\"}'";
    strcpy(str_cmd, str_ubusCall);
    pipeConfig = openPipe(str_cmd);
    while (fgets(output, 64, pipeConfig))
    {
        strcat(res, output);
    }
    closePipe(pipeConfig, str_cmd);
    for (int i = 0; i < MAX_PARM - 1; i++)
    {
        char value_str[64];
        getValueFromJson(res, init_configOptions[i].optionName, value_str);
        init_configOptions[i].value = strtol(value_str, NULL, 10);
    }

    init_configOptions[TEST_DELAY].value = (init_configOptions[TEST_DELAY].value > 60)?configOptionsDefault[TEST_DELAY].value:init_configOptions[TEST_DELAY].value;
    init_configOptions[TEST_NUM].value = (init_configOptions[TEST_NUM].value > 60)?configOptionsDefault[TEST_NUM].value:init_configOptions[TEST_NUM].value;
    init_configOptions[MAX_RESET].value = (init_configOptions[MAX_RESET].value > 60)?configOptionsDefault[MAX_RESET].value:init_configOptions[MAX_RESET].value;
}

void getPortsParam(void)
{
    for (int index = 0; index < NUM_PORTS; index++)
    {
         getPortParam(index, 0);
    }
}

void rebootParamInit(void)
{
    port_reboot_info_t  portRebootInfo;
    for (int port = 0; port < NUM_PORTS; ++port)
    {
        portRebootInfo.rebootState     = REBOOT_STATE_IDLE;
        portRebootInfo.rebootEvent     = REBOOT_EVENT_IDLE;
        portRebootInfo.rebootDelay     = 15;
        portRebootInfo.rebootTimeStart = 0;
        port_set_PortReboot(&portRebootInfo, port);
    }
}

static TYPE_OF_TEST_t getCmdKey(const char *str)
{
    TYPE_OF_TEST_t cmdKey = test_max;
    for (int i = 0; i < test_max; i++)
    {
        if (strcmp(str, testTable[i]) == 0)
        {
            if (i == 0)
            {
                cmdKey = test_disable;
                i = test_max;
            } else if (i == 1)
            {
                cmdKey = test_link;
                i = test_max;
            } else if (i == 2)
            {
                cmdKey = test_ping;
                i = test_max;
            } else if (i == 3)
            {
                cmdKey = test_speed;
                i = test_max;
            }
        }
    }
    return cmdKey;
}

static bool_t getAlarmSate(const char *str)
{
    bool_t cmdKey = max_bool;
    for (int i = 0; i < max_bool; i++)
    {
        if (strcmp(str, boolTable[i]) == 0)
        {
            if (i == 0)
            {
                cmdKey = disable;
                i = max_bool;
            } else if (i == 1)
            {
                cmdKey = enable;
                i = max_bool;
            }
        }
    }
    return cmdKey;
}

void getPortParam(uint8_t index, uint8_t isReboot)
{
    char portStr[2];
    char res[256];
    strcpy(res, "");
    strcpy(portStr, "");
    error_code_t errorCode;
    uint8_t port = index + 1;
    portInfo_t *portInfo = port_get_portInfoArr();
    portInfo[index].portConfigInfo.portNum = port;
    char output[OUTPUT_MAX_LENGTH];
    FILE *pipeTypeOfTest = NULL;

    toString(port, portStr);

    char str_TypeOfTestPrefix[] = "ubus call uci get '{\"config\":\"tf_autorestart\", \"section\":\"lan";
    char cmd_getTypeOfTest[128]  = "";
    char str_TypeOfTestSuffix[] = "\"}'";

    strcpy(cmd_getTypeOfTest, str_TypeOfTestPrefix);
    strcat(cmd_getTypeOfTest, portStr);
    strcat(cmd_getTypeOfTest, str_TypeOfTestSuffix);

    pipeTypeOfTest = openPipe(cmd_getTypeOfTest);

    while (fgets(output, OUTPUT_MAX_LENGTH, pipeTypeOfTest))
    {
        strcat(res, output);
    }
    closePipe(pipeTypeOfTest, cmd_getTypeOfTest);

    char value_state_str[16];
    char value_alarm_str[16];
    strcpy(value_state_str, "");
    strcpy(value_alarm_str, "");
    getValueFromJson(res, "state", value_state_str);
    getValueFromJson(res, "alarm", value_alarm_str);

    portInfo[index].portConfigInfo.test = test_disable;
    TYPE_OF_TEST_t typeOfTest = getCmdKey(value_state_str);
    bool_t alarmState         = getAlarmSate(value_alarm_str);
    portInfo[index].portConfigInfo.alarm = alarmState;
    setConfigZeroValue(&portInfo[index].portConfigInfo, isReboot);
    setResetZeroValue(&portInfo[index].portResetInfo, isReboot);

    if (typeOfTest >= test_max)
    {
        portInfo[index].portResetInfo.errorCode &= (~ERR_OK);
        portInfo[index].portResetInfo.errorCode |= ERR_TEST_TYPE;
    }
    else if (typeOfTest == test_disable)
    {
        portInfo[index].portResetInfo.errorCode &= (~ERR_OK);
        portInfo[index].portResetInfo.errorCode |= ERR_TEST_DISABLE;
    }
    else
    {
        portInfo[index].portConfigInfo.test    = typeOfTest;
        portInfo[index].portConfigInfo.portNum = port;
        if (setTestConfig[portInfo[index].portConfigInfo.test] == NULL)
        {
            printf("setTestConfig OBJ NOT FOUND\n");
        }
        else
        {
            portInfo[index].portResetInfo.errorCode = setTestConfig[portInfo[index].portConfigInfo.test](&portInfo[index].portConfigInfo, res);
        }
    }
    if (portInfo[index].portConfigInfo.alarm == enable)
    {
        errorCode = setTime(&portInfo[index].portConfigInfo, res);
        if (errorCode & ERR_NULL_OBJ)
        {
            errorCode &= (~ERR_NULL_OBJ);
            portInfo[index].portConfigInfo.alarm = disable;
        }
    }
    set_errorHandler(index);
}

static error_code_t setDisable(port_config_info_t *portConfig, char *jsonData)
{
    //printf("set port Disable Param -- port = %d\n", portConfig->portNum);
    return ERR_PORT_DISABLE;
}

static error_code_t setLink(port_config_info_t *portConfig, char *jsonData)
{
    //printf("set setLink -- port = %d\n", portConfig->portNum);
    //error_code_t errorCode = port_setLinkParam(portConfig);
    return ERR_OK;
}

static error_code_t setHost(port_config_info_t *portConfig, char *jsonData)
{
    //printf("set setHost -- port = %d\n", portConfig->portNum);
    char host[16];
    error_code_t errorCode;
    strcpy(host, "0.0.0.0");
    int res = getData_formJson(jsonData, "host", host);
    if (res ^ 8) {
        printf("error: json get ip addr - 0x%x\n", res);
        errorCode = ERR_IP_NOT_VALID;
    }
    else
    {
        errorCode = port_setHostParam(portConfig, host);
    }
    return errorCode;
}

static error_code_t setSpeed(port_config_info_t *portConfig, char *jsonData)
{
    //printf("set setSpeed -- port = %d\n", portConfig->portNum);
    char value_str[64];
    error_code_t errorCode;
    strcpy(value_str, "");
    uint16_t speed_max = 0;
    uint16_t speed_min = 0;

    int res = getData_formJson(jsonData, "max_speed", value_str);
    if (res ^ 8) {
        printf("error: json get speed max - 0x%x\n", res);
        errorCode = ERR_SPEED_VALUE;
    }
    else
    {
        speed_max = strtol(value_str, NULL, 10);
    }

    res = getData_formJson(jsonData, "min_speed", value_str);
    if (res ^ 8) {
        printf("error: json get speed min - 0x%x\n", res);
        errorCode = ERR_SPEED_VALUE;
    }
    else
    {
        speed_min = strtol(value_str, NULL, 10);
    }

    if (errorCode != ERR_SPEED_VALUE)
    {
        errorCode = port_setSpeedParam(portConfig, speed_max, speed_min);
    }
    return errorCode;
}

static error_code_t setTime(port_config_info_t *portConfig, char *jsonData)
{
    char value_str[64];
    char timeUp[5];
    char timeDown[5];
    error_code_t errorCode = ERR_OK;
    strcpy(value_str, "");
    strcpy(timeUp, "");
    strcpy(timeDown, "");
    uint8_t  status = 1;
    int res = getData_formJson(jsonData, "timeUp", value_str);
    if (res ^ 8) {
        status = 0;
        printf("error: json get time Up - 0x%x\n", res);
    }
    else
    {
        strcpy(timeUp, value_str);
    }
    strcpy(value_str, "");
    res = getData_formJson(jsonData, "timeDown", value_str);
    if (res ^ 8 ) {
        status = 0;
        printf("error: json get time Down - 0x%x\n", res);
    }
    else
    {
        strcpy(timeDown, value_str);
    }

    if (status)
    {
         errorCode =  port_setTimeParam(portConfig, timeUp, timeDown);
    }
    else
    {
        openlog("tf_autoresart", LOG_PID, LOG_USER);
        syslog(LOG_INFO, "autorestart can not run - config not corrected: port - %d", portConfig->portNum);
        closelog();
        printf("error: autorestart can not run - config not corrected -- port %d\n", portConfig->portNum);
        errorCode |= ERR_TIME_NOT_VALID;
    }
    return errorCode;
}

static void setConfigZeroValue(port_config_info_t *portConfig,  uint8_t isReboot)
{

    portConfig->min_speed       = 0;
    portConfig->max_speed       = 0;
    portConfig->rx_delta_byte   = 0;
    portConfig->rx_byte_current = 0;
    portConfig->rx_byte_prev    = 0;
    portConfig->time_current    = 0;
    portConfig->rx_speed_Kbit   = 0;

    portConfig->time_current = time(NULL);
    if (!isReboot)
    {
        strcpy(portConfig->host, "0.0.0.0");
        portConfig->timeAlarm[time_down].status = disable;
        portConfig->timeAlarm[time_up].status   = disable;
        portConfig->timeAlarm[time_down].time_H = 25;
        portConfig->timeAlarm[time_down].time_M = 61;
        portConfig->timeAlarm[time_up].time_H = 27;
        portConfig->timeAlarm[time_up].time_M = 62;
        strcpy(portConfig->timeStr, "\0");
    }
}

static void setResetZeroValue(port_reset_info_t *portReset, uint8_t isReboot)
{
    portReset->errorCode        = ERR_OK;
    if (!isReboot)
    {
        portReset->totalResetCount = 0;
    }
    portReset->resetCount       = 0;
    portReset->errorTestCount   = 0;
    portReset->state            = REGULAR_STATE;
    portReset->event            = IDLE_EVENT;
}

void runTimer()
{
    init_configOptions[MAIN_TIME].value = time(NULL) + init_configOptions[TEST_DELAY].value;
}

void getConfigOptions(config_type *configOptions, uint8_t index)
{
    memcpy(configOptions, &init_configOptions[index], sizeof(config_type));
}

config_type* init_getConfig(void)
{
    return init_configOptions;
}
