//
// Created by sheverdin on 10/3/23.
//

#include "dm_mainHeader.h"
#include "run_CLIcmd.h"
#include "parsing.h"
#include <ctype.h>
#include <string.h>

#define COMMAND_LEN (64)

const char *delimSpace  = " ";
const char *delim1 = "/";
const char *delimDot = ".";
const char *delimDoubleDot = ":";
const char *delimEqual = "=";

static int convert_ip_to_uint8(const char *ip_str, uint8_t ipaddr[4]);
static int convert_mac_to_uint8(const char *mac_str, uint8_t macaddr[6]);

const char CommandTable[CMD_MAX][COMMAND_LEN] =
{
    {"/etc/tf_device_monitor_scripts/getNetData.lua"},     // 0
    {"/etc/tf_device_monitor_scripts/getUptime.lua"},       // 1
    {"/etc/tf_device_monitor_scripts/getBoardInfo.lua"},    // 2
    {"/etc/tf_device_monitor_scripts/getSystemInfo.lua"},   // 3
};

/// ===============================================================

error_code_t getNetInfo(SWITCH_NET_PARAM *switchNetParam, char *devName)
{
    error_code_t errorCode = ERR_OK;
    char output[OUTPUT_MAX_LENGTH];
    char cmd_getNetInfo[128] = "";
    strcpy(cmd_getNetInfo, "");
    strcat(cmd_getNetInfo, CommandTable[CMD_NET_INFO]);
    strcat(cmd_getNetInfo, " ");
    strcat(cmd_getNetInfo, devName);
    printf("cmd_getNetInfo - %s\n", cmd_getNetInfo);
    FILE *pipe_netInfo = openPipe(cmd_getNetInfo);
    uint8_t lineCount = 0;
    while (fgets(output, OUTPUT_MAX_LENGTH, pipe_netInfo))
    {
        output[strcspn(output, "\n")] = 0;
        printf("output = %s\n", output);
        if (lineCount == 0)
        {
            if (strcmp(output, "found") == 0)
            {
                lineCount++;
                continue;
            }
            else if (strcmp(output, "not_found") == 0)
            {
                printf("interface \"%s\" not presented in the system\n",  devName);
                errorCode = ERR_CONFIGURATION;
                errorCode |= ERR_NETWORK;
            }
            else
            {
                errorCode = ERR_CONFIGURATION;
            }
        }
        else if(lineCount > 0)
        {
            switch (lineCount) {
                case 1:
                    strcpy(switchNetParam->deviceName, "");
                    strncpy(switchNetParam->deviceName, output, strlen(output));
                    strcat(switchNetParam->deviceName, "\0");
                    lineCount++;
                    break;
                case 2:
                    strcpy(switchNetParam->ifName, "");
                    strncpy(switchNetParam->ifName, output, strlen(output));
                    strcat(switchNetParam->ifName, "\0");
                    lineCount++;
                    break;
                case 3:
                    convert_mac_to_uint8(output, switchNetParam->mac_addr);
                    lineCount++;
                    break;
                case 4:
                    convert_ip_to_uint8(output, switchNetParam->ip_addr);
                    lineCount++;
                    break;
                case 5:
                    convert_ip_to_uint8(output, switchNetParam->net_mask);
                    lineCount++;
                    break;
                case 6:
                    convert_ip_to_uint8(output, switchNetParam->getWay);
                    lineCount++;
                    break;
                default:
                    fprintf(stderr, "Error: Unexpected parameter %d\n", lineCount);
                    break;
            }
        }
    }
    closePipe(pipe_netInfo, (char *)CommandTable[CMD_NET_INFO]);
    printf("getNetInfo - ERROR %d\n", errorCode);
    return errorCode;
}

void setNetInfo(search_out_msg_t *searchOutMsg, SWITCH_NET_PARAM *switchNetParam)
{
    printf("setNetInfo\n");
    printf("mac_addr\n");
    for (int i = 0; i < MAC_LEN; i++)
    {
        printf("%X:", switchNetParam->mac_addr[i]);
        searchOutMsg->struct1.mac[i] = switchNetParam->mac_addr[i];
    }
    printf("\n");

    printf("ip_addr\n");
    for (int i = 0; i < IPV4_LEN; i++)
    {
        printf("%d.  -- %d. -- %d.", switchNetParam->ip_addr[i], switchNetParam->net_mask[i], switchNetParam->getWay[i]);
        searchOutMsg->struct1.ip[i] = switchNetParam->ip_addr[i];
        searchOutMsg->struct1.mask[i] = switchNetParam->net_mask[i];
        searchOutMsg->struct1.gate[i] = switchNetParam->getWay[i];
        printf("\n");
    }
    printf("\n");
}


void getTimeInfo(search_out_msg_t *searchOutMsg)
{
    char output[OUTPUT_MAX_LENGTH];
    printf(">>> getTimeInfo\n");
    openlog("tfdm", LOG_PID, LOG_USER);
    syslog(LOG_INFO, " -- getTimeInfo");
    uint8_t  uptimeFlag = 0;
    FILE *time_pipe = openPipe(CommandTable[CMD_TIME_INFO]);
    printf("cmd = %s\n", CommandTable[CMD_TIME_INFO]);
    uint8_t lineCount = 0;
    while (fgets(output, OUTPUT_MAX_LENGTH, time_pipe))
    {
        uptimeFlag = 1;
        lineCount ++;
        if (lineCount == 1)
        {
            syslog(LOG_INFO, "Uptime output: %s", output);
            uint32_t d_uptime = strtol(output, NULL, 10);
            syslog(LOG_INFO, "Uptime : %d", d_uptime);
            searchOutMsg->struct1.uptime[0] = (d_uptime) & (0xFF);
            searchOutMsg->struct1.uptime[1] = (d_uptime >> 8 * 1) & (0xFF);
            searchOutMsg->struct1.uptime[2] = (d_uptime >> 8 * 2) & (0xFF);
            searchOutMsg->struct1.uptime[3] = (d_uptime >> 8 * 3) & (0xFF);
            syslog(LOG_INFO, "struct1.uptime 1: %x", searchOutMsg->struct1.uptime[0]);
            syslog(LOG_INFO, "struct1.uptime 2: %x", searchOutMsg->struct1.uptime[1]);
            syslog(LOG_INFO, "struct1.uptime 3: %x", searchOutMsg->struct1.uptime[2]);
            syslog(LOG_INFO, "struct1.uptime 4: %x", searchOutMsg->struct1.uptime[3]);
        }
        else {
            syslog(LOG_ERR, "In system more than 1 Uptime - count: %d", lineCount);
        }
    }
    if(!uptimeFlag)
    {
        syslog(LOG_ERR, "Uptime not found");
    }
    closelog();
    closePipe(time_pipe, (char *)CommandTable[CMD_TIME_INFO]);
}

void getBoardInfo(search_out_msg_t *searchOutMsg)
{
    openlog("tfdm", LOG_PID, LOG_USER);
    syslog(LOG_INFO, " -- getBoardInfo");
    char output[OUTPUT_MAX_LENGTH];
    char model[]        = "model";
    char version[]      = "version";
    uint8_t modelFlag   = 0;
    uint8_t versionFlag = 0;
    splited_line_t splitLineBoard;
    uint8_t lineCount = 0;
    uint8_t fw_version_count = FW_VERSION_MAXBIT;
    FILE *board_pipe = openPipe(CommandTable[CMD_BOARD_INFO]);
    syslog(LOG_INFO, "CMD_BOARD_INFO %s", CommandTable[CMD_BOARD_INFO]);
    while (fgets(output, OUTPUT_MAX_LENGTH, board_pipe) != NULL)
    {
        syslog(LOG_INFO, "model type : %s", output);
        output[strcspn(output, "\n")] = 0;
        lineCount++;
        if (lineCount == 1)
        {
            uint8_t modelType = strtol(output, NULL, 10);
            if (modelType > 0) {
                modelFlag = 1;
                searchOutMsg->struct1.dev_type = modelType;
                syslog(LOG_INFO, "model: %d", searchOutMsg->struct1.dev_type);
            }
            else
            {
                searchOutMsg->struct1.dev_type = 0;
                syslog(LOG_ERR, "Model not found");
            }
        }
        else if (lineCount >= 2 && lineCount <= 4)
        {
            versionFlag = 1;
            uint8_t fwv = strtol(output, NULL, 10);
            searchOutMsg->struct1.firmware[fw_version_count] = (fwv) & (0xFF);
            syslog(LOG_INFO, "searchOutMsg->struct1.firmware[lineCount - 2]  %d -- line count - %d", searchOutMsg->struct1.firmware[fw_version_count], fw_version_count);
            if (fw_version_count > 0) {
                fw_version_count--;
            }
        }
        else if (lineCount > 4) {
            versionFlag = 0;
            syslog(LOG_INFO, "System Info not corrected");
        }
    }
    closelog();
    closePipe(board_pipe, (char *)CommandTable[CMD_BOARD_INFO]);
    if(!modelFlag)
    {
        openlog("tfdm", LOG_PID | LOG_PERROR, LOG_USER);
        syslog(LOG_NOTICE, "Error: model not found");
        closelog();
    }
    if(!versionFlag)
    {
        openlog("tfdm", LOG_PID | LOG_PERROR, LOG_USER);
        syslog(LOG_NOTICE, "Error: version not found");
        closelog();
    }
}

void getSystemInfo(search_out_msg_t *searchOutMsg)
{
    char output[OUTPUT_MAX_LENGTH];
    uint8_t locationFlag    = 0;
    uint8_t descriptionFlag = 0;

    FILE *sys_pipe = openPipe(CommandTable[CMD_SYSTEM_INFO]);
    uint8_t lineCount = 0;
    openlog("tfdm", LOG_PID , LOG_USER);
    syslog(LOG_INFO, " -- getSystemInfo");
    while (fgets(output, OUTPUT_MAX_LENGTH, sys_pipe))
    {
        lineCount++;
        output[strcspn(output, "\n")] = 0;
        if (lineCount == 1) {
            locationFlag = 1;

            for (int i = 0; i< MAX_LENGTH; i++ )
            {
                searchOutMsg->struct1.dev_loc[i] = '\0';
            }
            strncpy(searchOutMsg->struct1.dev_loc, output, strlen(output));
            searchOutMsg->struct1.dev_loc[MAX_LENGTH-1] = (uint8_t) 0x7F;
            syslog(LOG_INFO, "Location: %s", searchOutMsg->struct1.dev_loc);
        }
        else if (lineCount == 2) {
            descriptionFlag = 1;
            for (int i = 0; i< MAX_LENGTH; i++ )
            {
                searchOutMsg->struct1.dev_descr[i] = '\0';
            }
            strncpy(searchOutMsg->struct1.dev_descr,output, strlen(output));
            syslog(LOG_INFO, "Description: %s", searchOutMsg->struct1.dev_descr);
        }
        else if (lineCount > 2) {
            syslog(LOG_INFO, "System Info more than 2 Line, count %d", lineCount);
        }
    }
    closelog();
    closePipe(sys_pipe, (char *)CommandTable[CMD_SYSTEM_INFO]);

    if(!locationFlag)
    {
        openlog("tfdm", LOG_PID | LOG_PERROR, LOG_USER);
        syslog(LOG_NOTICE, "Error: Location not found");
        closelog();
    }
    if(!descriptionFlag)
    {
        openlog("tfdm", LOG_PID | LOG_PERROR, LOG_USER);
        syslog(LOG_NOTICE, "Error: Description not found");
        closelog();
    }
}

FILE *openPipe(const char *cmdStr)
{
    FILE *pipe = popen(cmdStr, "r");
    if (!pipe)
    {
        openlog("tfdm", LOG_PID | LOG_PERROR, LOG_USER);
        syslog(LOG_ERR, "Error to run command %s", cmdStr);
        closelog();
    }
    return pipe;
}

void closePipe(FILE *pipe, char *str)
{
    int status = pclose(pipe);
    if (!status) 
	{
        return;
    }
    else
	{
        printf(" -->err closePipe %s\n", str);
        openlog("tfdm", LOG_PID | LOG_PERROR, LOG_USER);
        syslog(LOG_ERR, "Error close pipe, cmd  %s, status %d\n", str, status);
        closelog();
    }
}

static int convert_ip_to_uint8(const char *ip_str, uint8_t ipaddr[4])
{
    char *token;
    char *ip_copy = strdup(ip_str);
    int index = 0;
    token = strtok(ip_copy, ".");
    while (token != NULL && index < 4) {
        int num = atoi(token);
        if (num < 0 || num > 255) {
            free(ip_copy);
            return -1;
        }

        ipaddr[index++] = (uint8_t) num;
        token = strtok(NULL, ".");
    }
    free(ip_copy);
    return (index == 4) ? 0 : -1;
}

static int convert_mac_to_uint8(const char *mac_str, uint8_t macaddr[6])
{
    int res = sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
              &macaddr[0], &macaddr[1], &macaddr[2],
              &macaddr[3], &macaddr[4], &macaddr[5]);
    if(res != 6)
    {
        printf("error to convert MAC\n");
    }
    return res;
}

const char *getCommandTable_line(cmd_type_e cmd)
{
    return CommandTable[cmd];
}

