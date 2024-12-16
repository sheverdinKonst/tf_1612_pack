//
// Created by sheverdin on 6/25/24.
//

#include "port_module.h"
#include "utils_lib/utils_module.h"
#include "ping.h"

portInfo_t portInfoArr[NUM_PORTS];

static void port_timeAlarm(time_h_m *timeAlarmInfo);

error_code_t port_setLinkParam(port_config_info_t *portConfig)
{
    return ERR_OK;
}

error_code_t port_setHostParam(port_config_info_t *portConfig , char *host)
{
    int ipIsValid  = ERR_IP_NOT_VALID;
    //portConfig->test = test_ping;

    strcpy(portConfig->host, host);
    printf("\n");
    checkValidIp(portConfig->host, &ipIsValid);
    return ipIsValid;
}

error_code_t port_setSpeedParam(port_config_info_t *portConfig, uint32_t max, uint32_t min)
{
    if (min > max) {
        return  ERR_SPEED_VALUE;
    }

    if (max < MIN_SPEED || max > MAX_SPEED){
        return  ERR_SPEED_VALUE;
    }
    else
    {
        portConfig->max_speed = max;
    }

    if(min < MIN_SPEED || min > MAX_SPEED){
        return ERR_SPEED_VALUE;
    }
    else
    {
         portConfig->min_speed = min;
    }
    return ERR_OK;
}

error_code_t port_setTimeParam(port_config_info_t *portConfig, char* timeUpStr, char *timeDownStr)
{
    struct tm timeInfo;
    struct tm *timeInfoPtr;
    time_t now = time(NULL);
    error_code_t errorCode = ERR_OK;
    errorCode |= isValidTime(timeUpStr, &timeInfo);

    if(errorCode == ERR_OK) {
        portConfig->timeAlarm[time_up].time_H = timeInfo.tm_hour;
        portConfig->timeAlarm[time_up].time_M = timeInfo.tm_min;

        timeInfoPtr = localtime(&now);
        timeInfoPtr->tm_hour   = portConfig->timeAlarm[time_up].time_H;
        timeInfoPtr->tm_min    = portConfig->timeAlarm[time_up].time_M;
        timeInfoPtr->tm_sec    = 0;
        portConfig->timeAlarm[time_up].targetTime = mktime(timeInfoPtr);

        if(now >= portConfig->timeAlarm[time_up].targetTime)
        {
            timeInfoPtr->tm_mday += 1;
            portConfig->timeAlarm[time_up].targetTime = mktime(timeInfoPtr);
            //printf("target Time set to next Day\n");
        }
    }

    errorCode |= isValidTime(timeDownStr, &timeInfo);

    if(errorCode == ERR_OK) {
        portConfig->timeAlarm[time_down].time_H = timeInfo.tm_hour;
        portConfig->timeAlarm[time_down].time_M = timeInfo.tm_min;
        timeInfoPtr = localtime(&now);
        timeInfoPtr->tm_hour   = portConfig->timeAlarm[time_down].time_H;
        timeInfoPtr->tm_min    =  portConfig->timeAlarm[time_down].time_M;
        timeInfoPtr->tm_sec    = 0;
        portConfig->timeAlarm[time_down].targetTime = mktime(timeInfoPtr);

        if(now >= portConfig->timeAlarm[time_down].targetTime)
        {
            timeInfoPtr->tm_mday += 1;
            portConfig->timeAlarm[time_down].targetTime = mktime(timeInfoPtr);
            //printf("target Time set to next Day\n");
        }
    }
    return errorCode;
}

portInfo_t *port_get_portInfoArr(void)
{
    return portInfoArr;
}

error_code_t port_set_PortReboot(port_reboot_info_t *portRebootInfo, uint8_t portNum)
{
    error_code_t errorCode = ERR_OK;
    void *res = memcpy(&portInfoArr[portNum].portRebootInfo, portRebootInfo, sizeof(port_reboot_info_t));
    if (res == NULL)
    {
        errorCode = ERR_NULL_OBJ;
    }

    return errorCode;
}

////////////////////////////////////////////////////////////////////
void port_runTestDisable(uint8_t portNum)
{
    //printf("runTestDisable\n");
    uint8_t index = 0;
    if (portNum > 0 && portNum <= NUM_PORTS)
    {
        index = portNum - 1;
    }
    else
    {
        return;
    }
    portInfoArr[index].portResetInfo.errorCode &= (~ERR_OK);
    portInfoArr[index].portResetInfo.errorCode |= ERR_TEST_DISABLE;
    //printf("port_runTestDisable -%d\t --  errorCode: %x\n", index, portInfoArr[index].portResetInfo.errorCode);
}

void port_runTestLink(uint8_t portNum)
{
    uint8_t index = 0;
    if (portNum > 0 && portNum <= NUM_PORTS)
    {
        index = portNum - 1;
    }
    else
    {
        return;
    }

    char output[OUTPUT_MAX_LENGTH];
    char cmd_getOperstate[64];
    char portStr[2] = "";
    strcpy(output, "");
    strcpy(cmd_getOperstate, "");
    strcpy(portStr, "");

    FILE *portLink              = NULL;
    char str_up[]               = "up\n";
    char path_OperstatePrefix[] = "/sys/class/net/lan\0";
    char path_OperstateSufix[]  = "/operstate\0";

    toString(portNum, portStr);
    strcpy(cmd_getOperstate, path_OperstatePrefix);
    strcat(cmd_getOperstate, portStr);
    strcat(cmd_getOperstate, path_OperstateSufix);
    FILE *fp = fopen(cmd_getOperstate, "r");
    if (fp)
    {
        fgets(output, OUTPUT_MAX_LENGTH, fp);
    }
    else
    {
        printf("Error open file\n");
    }
    fclose(fp);

    portInfoArr[index].portResetInfo.errorCode &= (~ERR_TEST_LINK);
    portInfoArr[index].portResetInfo.errorCode |= ERR_OK;

    if (strcmp(output, str_up)!=0) {
        portInfoArr[index].portResetInfo.errorCode &= (~ERR_OK);
        portInfoArr[index].portResetInfo.errorCode |= ERR_TEST_LINK;
    }
    else
    {
        if(portInfoArr[index].portConfigInfo.test == test_link)
        {
            if (portInfoArr[index].portResetInfo.state == IDLE_STATE)
            {
                portInfoArr[index].portResetInfo.event |= TEST_OK;
            }
        }
    }
}

void port_runTestPing(uint8_t portNum)
{
    uint8_t index = 0;
    if (portNum > 0 && portNum <= NUM_PORTS)
    {
        index = portNum - 1;
    }
    else
    {
        return;
    }

    const unsigned long timeout = 1500;
    unsigned long reply_time    = 0;
    int pingResult = ERR_TEST_PING;
    if (portInfoArr[index].portResetInfo.errorCode & ERR_TEST_LINK)
    {
        pingResult = ERR_TEST_PING;
    }
    else
    {
        pingResult = ping(portInfoArr[index].portConfigInfo.host, timeout, &reply_time);
    }
    portInfoArr[index].portConfigInfo.reply_time = reply_time;
    if (pingResult == ERR_TEST_PING)
    {
        //printf("Host is not available or timeOut\n");
        portInfoArr[index].portResetInfo.errorCode &= (~ERR_OK);
        portInfoArr[index].portResetInfo.errorCode |= ERR_TEST_PING;
    }
    else if (pingResult == ERR_OK)
    {
        portInfoArr[index].portConfigInfo.reply_time = reply_time;
        portInfoArr[index].portResetInfo.errorCode = ERR_OK;

        if (portInfoArr[index].portResetInfo.state == IDLE_STATE)
        {
            portInfoArr[index].portResetInfo.event |= TEST_OK;
        }

    }
}

void port_runTestSpeed(uint8_t portNum)
{
    uint32_t delta_time = 0;
    uint32_t rx_speed_Kbit = 0;
    uint32_t rx_delta_bit = 0;
    uint8_t index = 0;
    if (portNum > 0 && portNum <= NUM_PORTS)
    {
        index = portNum - 1;
    }
    else
    {
        return;
    }

    if (portInfoArr[index].portResetInfo.errorCode & ERR_TEST_LINK)
    {
        delta_time = 0;
    }
    else
    {
        char output[OUTPUT_MAX_LENGTH];
        char portStr[2];
        char cmd_get_rx_byte[64];
        strcpy(output, "");
        strcpy(portStr, "");
        strcpy(cmd_get_rx_byte, "");

        FILE *portSpeed = NULL;
        char path_rx_bytePrefix[] = "cat /sys/class/net/lan";
        char path_rx_byteSufix[] = "/statistics/rx_bytes";

        toString(portNum, portStr);
        strcpy(cmd_get_rx_byte, path_rx_bytePrefix);
        strcat(cmd_get_rx_byte, portStr);
        strcat(cmd_get_rx_byte, path_rx_byteSufix);
        //printf("cmd_get_rx_byte = %s\n", cmd_get_rx_byte);

        portSpeed = openPipe(cmd_get_rx_byte);

        fgets(output, OUTPUT_MAX_LENGTH, portSpeed);
        closePipe(portSpeed, cmd_get_rx_byte);
        //printf("rx byte =  %s", output);
        portInfoArr[index].portResetInfo.errorCode &= (~ERR_TEST_SPEED);
        portInfoArr[index].portResetInfo.errorCode |= ERR_OK;

        portInfoArr[index].portConfigInfo.rx_byte_current = (int32_t) strtol(output, NULL, 10);
        portInfoArr[index].portConfigInfo.rx_delta_byte =
                portInfoArr[index].portConfigInfo.rx_byte_current - portInfoArr[index].portConfigInfo.rx_byte_prev;
        //printf("RX_Byte:  current = %d - prev = %d - delta %d\n",portInfoArr[index].portConfigInfo.rx_byte_current, portInfoArr[index].portConfigInfo.rx_byte_prev, portInfoArr[index].portConfigInfo.rx_delta_byte);
        portInfoArr[index].portConfigInfo.rx_byte_prev = portInfoArr[index].portConfigInfo.rx_byte_current;

        portInfoArr[index].portConfigInfo.time_current = time(NULL);
        delta_time =
                portInfoArr[index].portConfigInfo.time_current - portInfoArr[index].portConfigInfo.time_prev;
        //printf("Time: current = %d - prev = %d - delta %d\n", portInfoArr[index].portConfigInfo.time_current, portInfoArr[index].portConfigInfo.time_prev, delta_time);
        portInfoArr[index].portConfigInfo.time_prev = portInfoArr[index].portConfigInfo.time_current;

        rx_delta_bit = portInfoArr[index].portConfigInfo.rx_delta_byte * 8;
        rx_speed_Kbit = rx_delta_bit;
    }

    if (delta_time == 0) {
        portInfoArr[index].portResetInfo.errorCode &= (~ERR_OK);
        portInfoArr[index].portResetInfo.errorCode |= ERR_TEST_SPEED;
    }
    else
    {
        rx_speed_Kbit = rx_delta_bit / (delta_time);
        rx_speed_Kbit = rx_speed_Kbit/1000;
        //printf("speed [KBit/sec] = %d\n", rx_speed_Kbit);

        if ((rx_speed_Kbit < portInfoArr[index].portConfigInfo.min_speed) ||
            (rx_speed_Kbit > portInfoArr[index].portConfigInfo.max_speed))
        {
            portInfoArr[index].portResetInfo.errorCode &= (~ERR_OK);
            portInfoArr[index].portResetInfo.errorCode |= ERR_TEST_SPEED;
        }
        else
        {
            if (portInfoArr[index].portResetInfo.state == IDLE_STATE)
            {
                portInfoArr[index].portResetInfo.event |= TEST_OK;
            }
        }
    }
    portInfoArr[index].portConfigInfo.rx_speed_Kbit = rx_speed_Kbit;
}

void port_runTestTime(uint8_t portNum)
{
    uint8_t index = 0;
    if (portNum > 0 && portNum <= NUM_PORTS)
    {
        index = portNum - 1;
    }
    else
    {
        return;
    }
    port_timeAlarm(&portInfoArr[index].portConfigInfo.timeAlarm[time_up]);
    port_timeAlarm(&portInfoArr[index].portConfigInfo.timeAlarm[time_down]);
}

//////////////////////////////////////////////////////////////
AR_STATE get_PortAndPoeState(uint8_t portNum)
{
    //printf("get_PortAndPoeState -- %d \n", portNum);
    uint8_t index = 0;
    if (portNum > 0 && portNum <= NUM_PORTS)
    {
        index = portNum - 1;
    }
    else
    {
        return IDLE_STATE;
    }

    AR_STATE  ar_state = REGULAR_STATE;
    char output[OUTPUT_MAX_LENGTH];
    char cmdPortState[64];
    char portStr[2];
    strcpy(output, "");
	strcpy(cmdPortState, "");
    strcpy(portStr, "");
    char *s_enable         = "enable";
    char *str1             = "ubus call uci get '{\"config\":\"port\", \"section\":\"lan";
    char *str2             =  "\"}'";
    toString(portNum, portStr);

    strcpy(cmdPortState, str1);
    strcat(cmdPortState, portStr);
    strcat(cmdPortState, str2);

    //printf("cmdPortState ============== :  %s\n", cmdPortState);

    FILE *pipePortState = openPipe(cmdPortState);
    char  res[256];
    strcpy(res, "");
    while (fgets(output, OUTPUT_MAX_LENGTH, pipePortState))
    {
        strcat(res, output);
    }
    closePipe(pipePortState, cmdPortState);

    char state_str[32];
    char poe_str[32];
	strcpy(state_str, "");
	strcpy(poe_str, "");
    getData_formJson(res, "state", state_str);
    getData_formJson(res, "poe", poe_str);

    //printf("state_str = %s\t poe_str = %s\n", state_str, poe_str);
    //printf("state_str len  = %lu\t poe_str len = %lu\n", strlen(state_str), strlen(poe_str));

    if (strcmp (state_str, s_enable)==0)
    {
        portInfoArr[index].portStateInfo.port_state = enable;
    }
    else
    {
        //printf("Error: state_str false - %s -- %d\n", poe_str, portNum);
        portInfoArr[index].portStateInfo.port_state = disable;
        portInfoArr[index].portResetInfo.errorCode &= (~ERR_OK);
        portInfoArr[index].portResetInfo.errorCode |= ERR_PORT_DISABLE;
        portInfoArr[index].portResetInfo.state = IDLE_STATE;
        ar_state = portInfoArr[index].portResetInfo.state;
    }

    if (strcmp (poe_str, s_enable)==0)
    {
        //printf("Error: poe_str true = %s \n", poe_str);
        portInfoArr[index].portStateInfo.poe_state = enable;
    }
    else
    {
        //printf("Error: poe_str false - %s -- %d  \n", poe_str, portNum);
        portInfoArr[index].portStateInfo.poe_state = disable;
        portInfoArr[index].portResetInfo.errorCode &= (~ERR_OK);
        portInfoArr[index].portResetInfo.errorCode |= ERR_PoE_DISABLE;
        portInfoArr[index].portResetInfo.state = IDLE_STATE;
        ar_state = portInfoArr[index].portResetInfo.state;
    }
    return ar_state;
}

static void port_timeAlarm(time_h_m *timeAlarmInfo)
{
    time_t now = time(NULL);
    struct tm *timeInfo = localtime(&now);
    timeInfo->tm_hour   = timeAlarmInfo->time_H;
    timeInfo->tm_min    = timeAlarmInfo->time_M;
    timeInfo->tm_sec    = 0;
    now = time(NULL);
    if (now >= timeAlarmInfo->targetTime) {
        timeInfo = localtime(&now);
        timeInfo->tm_mday += 1; // add one day
        timeAlarmInfo->targetTime = mktime(timeInfo);
        timeAlarmInfo->remainTime = 0;
        timeAlarmInfo->status = enable;
    }
    else
    {
        timeAlarmInfo->remainTime = timeAlarmInfo->targetTime - now;
    }
}

void poe_Control(uint8_t port, POE_CONTROL poeControl)
{
    FILE *pipe_poeRestart = NULL;
    char output[OUTPUT_MAX_LENGTH];
    char cmdPortRestart[64];
    char portStr[2];
    strcpy(output, "");
    strcpy(cmdPortRestart, "");
    strcpy(portStr, "");

    char str1[]             = "ubus call poe manage '{\"port\":\"lan"; //   1\", \"enable\":true}'";
    char strTrue[]          = "\", \"enable\":true}'";
    char strFalse[]         = "\", \"enable\":false}'";

    toString(port, portStr);

    if(poeControl & POE_DOWN)
    {
        strcpy(cmdPortRestart, str1);
        strcat(cmdPortRestart, portStr);
        strcat(cmdPortRestart, strFalse);
        pipe_poeRestart = openPipe(cmdPortRestart);
        closePipe(pipe_poeRestart, cmdPortRestart);
        openlog("tf_autoresart", LOG_PID, LOG_USER);
        syslog(LOG_INFO, "POE DOWN, port - %d", port);
        closelog();
    }

    if(poeControl & POE_UP)
    {
        //printf("REBOOT_STATE_POE_UP\n");
        strcpy(cmdPortRestart, "");
        strcpy(cmdPortRestart, str1);
        strcat(cmdPortRestart, portStr);
        strcat(cmdPortRestart, strTrue);
        pipe_poeRestart = openPipe(cmdPortRestart);
        closePipe(pipe_poeRestart, cmdPortRestart);
        openlog("tf_autoresart", LOG_PID, LOG_USER);
        syslog(LOG_INFO, "POE UP, port - %d ", port);
        closelog();
    }
}

void autoResetHandler(uint8_t portNum, uint8_t maxReset)
{
    uint8_t index = 0;
    if (portNum > 0 && portNum <= NUM_PORTS)
    {
        index = portNum - 1;
    }
    else
    {
        return;
    }

    char  timeDateStr[32];
    strcpy(timeDateStr, "");
    getTimeDate(timeDateStr);
    strcpy(portInfoArr[index].portConfigInfo.timeStr, "");
    strcpy(portInfoArr[index].portConfigInfo.timeStr, timeDateStr);
    portInfoArr[index].portResetInfo.errorCode &= (~ERR_OK);

    if (portInfoArr[index].portResetInfo.resetCount >= maxReset)
    {
        portInfoArr[index].portResetInfo.errorCode &= (~ERR_REBOOTING);
        portInfoArr[index].portResetInfo.errorCode |= ERR_PORT_SHUTDOWN;
        portInfoArr[index].portRebootInfo.rebootState =  REBOOT_STATE_IDLE;
        portInfoArr[index].portResetInfo.state = IDLE_STATE;
        syslog(LOG_INFO, "Reboot disable after max reset, port - %d", portNum);
        closelog();
        set_errorHandler(index);
    }
    else if (portInfoArr[index].portResetInfo.resetCount < maxReset)
    {
        openlog("tf_autorestart", LOG_PID, LOG_USER);
        syslog(LOG_INFO, "auto reset port = %d", portNum);
        syslog(LOG_INFO, "total reset count = %d", portInfoArr[index].portResetInfo.totalResetCount);
        closelog();
        portInfoArr[index].portResetInfo.errorTestCount = 0;
        portInfoArr[index].portResetInfo.resetCount++;
        portInfoArr[index].portResetInfo.totalResetCount++;
        portInfoArr[index].portResetInfo.errorCode |= ERR_REBOOTING;
        portInfoArr[index].portRebootInfo.rebootState = REBOOT_STATE_INIT;
    }
}

void manualResetHandler(uint8_t portNum)
{
    uint8_t index = 0;
    if (portNum > 0 && portNum <= NUM_PORTS)
    {
        index = portNum - 1;
    }
    else
    {
        return;
    }

    openlog("tf_autorestart", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "run manual reset - port = %d", portInfoArr[index].portConfigInfo.portNum);
    closelog();

    char  timeDateStr[32];
    strcpy(timeDateStr, "");
    getTimeDate(timeDateStr);

    strcpy(portInfoArr[index].portConfigInfo.timeStr, "");
    strcpy(portInfoArr[index].portConfigInfo.timeStr, timeDateStr);

    portInfoArr[index].portResetInfo.errorCode &= (~ERR_OK);
    portInfoArr[index].portResetInfo.errorCode &= (~ERR_PORT_SHUTDOWN);

    portInfoArr[index].portResetInfo.errorCode |= ERR_MANUAL_REBOOT;
    portInfoArr[index].portResetInfo.totalResetCount++;
    portInfoArr[index].portResetInfo.resetCount     = 0;
    portInfoArr[index].portResetInfo.errorTestCount = 0;
    portInfoArr[index].portRebootInfo.rebootState = REBOOT_STATE_INIT;
}

void set_errorHandler(uint8_t index)
{
    if (
        portInfoArr[index].portResetInfo.errorCode      & ERR_IP_NOT_VALID
        || portInfoArr[index].portResetInfo.errorCode   & ERR_TIME_NOT_VALID
        || portInfoArr[index].portResetInfo.errorCode   & ERR_SPEED_VALUE
        || portInfoArr[index].portResetInfo.errorCode   & ERR_PORT_DISABLE
        )
    {
        printf("portConfigInfo.test = test_disable - port - %d\n", portInfoArr[index].portConfigInfo.portNum);
        portInfoArr[index].portConfigInfo.test = test_disable;
    }
}



