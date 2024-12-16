//
// Created by sheverdin on 10/17/23.
//

#include "dm_mainHeader.h"
#include "settings_module.h"
#include "parsing.h"

static void getCmdType(splited_line_t splitedLine, DYNAMIC_MAP_CMD *mapCmd);

static void runReboot(char* inData);
static void runReset(char* inData);
static void runSetIp(char* inData);
static void runSetMask(char* inData);
static void runSetGate(char* inData);
static void runSetDescription(char* inData);
static void runSetLocation(char* inData);
static void runSetSnmp(char* inData);
static void runSystemRestart(char* inData);
static void runNetworkRestart(char* inData);
static int checkGateWayValid(char * ip);

int locationTestFlag = 0;

static SETTINGS_INFO_t settingsInfo =
{
    .setDHCP = 0,
    .applyNetwork = 0,
    .applySystem = 0
};

const char settings_Table[SETTINGS_MAX_CMD][SET_CMD_LEN] =
{
    {"DEFAULT_ALL"},
    {"REBOOT_ALL"},
    {"IPADDRESS"},
    {"NETMASK"},
    {"GATEWAY"},
    {"SYSTEM_NAME"},
    {"SYSTEM_LOCATION"},
    {"SNMP_STATE"}
};

const char settings_commandTable[SETTINGS_MAX_CMD][SET_CMD_LEN] =
{
    {"echo 'firstboot -y && reboot'"},
    {"echo 'reboot'"},
    {"IPADDRESS"},
    {"NETMASK"},
    {"GATEWAY"},
    {"uci set system.@system[0].description="},
    {"uci set system.@system[0].location="},
    {"uci set snmpd.general.enabled="},
    {"/etc/init.d/system restart"},
    {"/etc/init.d/network restart"}
};

runSettingsCmd_f runSetingsCmd[SETTINGS_MAX_CMD] =
{
    &runReset,
    &runReboot,
    &runSetIp,
    &runSetMask,
    &runSetGate,
    &runSetDescription,
    &runSetLocation,
    &runSetSnmp,
    &runSystemRestart,
    &runNetworkRestart
};

runSettingsCmd_f *getRunSettingsCmd(void)
{
    return runSetingsCmd;
}

static void getCmdType(splited_line_t splitedLine, DYNAMIC_MAP_CMD *mapCmd)
{
    settingsInfo.applySystem = 0;
    settingsInfo.applyNetwork = 0;
    settingsInfo.setDHCP = 0;
    for (int i = 0; i < SETTINGS_MAX_CMD; i++)
    {
        if (strcmp(splitedLine.tokens[0], settings_Table[i]) == 0)
        {
            mapCmd->cmd = i;
            strncpy((char *)mapCmd->value, splitedLine.tokens[1], SETTINGS_VALUE_LENGTH);
            i = SETTINGS_MAX_CMD;
        }
    }
}

DYNAMIC_MAP_CMD *getDynamicMapCmd_ptr(uint8_t splitLineMax)
{
    DYNAMIC_MAP_CMD *mapCmd = NULL;
    mapCmd = (DYNAMIC_MAP_CMD *) malloc(splitLineMax * sizeof(DYNAMIC_MAP_CMD));
    if (mapCmd == NULL)
    {
        openlog("tfdm", LOG_PID | LOG_PERROR, LOG_USER);
        syslog(LOG_ERR, "Error to malloc memory for mapCmd\n");
        closelog();
        return (DYNAMIC_MAP_CMD*) NULL;
    }
    return mapCmd;
}

error_code_t getSettingsCmdArray(splited_line_t splitedLine, DYNAMIC_MAP_CMD *mapCmd)
{
    if(mapCmd == NULL)
    {
        openlog("tfdm", LOG_PID | LOG_PERROR, LOG_USER);
        syslog(LOG_ERR, " getSettingsCmdArray: is NULL ptr\n");
        closelog();
        return ERR_OBJ_NOT_FOUND;
    }
    else
    {
        splited_line_t splitLineCmd;
        for (int i = 0; i <= splitedLine.splitLineMax; i++)
        {
            split_line((char *) splitedLine.tokens[i], "=", &splitLineCmd);
            getCmdType(splitLineCmd ,  (mapCmd+i));
        }
    }
    return ERR_OK;
}

error_code_t settings_handler(DYNAMIC_MAP_CMD *settingsCmdMap, uint8_t splitLineMax)
{
    if (settingsCmdMap == NULL)
    {
        return ERR_OBJ_NOT_FOUND;
    }
    else
    {
        SETTINGS_CMD cmdIndex;
        openlog("tddm", LOG_PID, LOG_USER);
        for(int i = 0; i <= splitLineMax; i++)
        {
            cmdIndex = settingsCmdMap[i].cmd;
            runSetingsCmd[cmdIndex](settingsCmdMap[i].value);
        }
        if (settingsInfo.applySystem == 1)
        {
            syslog(LOG_INFO, "SETTINGS_SYS_RELOAD");
            runSetingsCmd[SETTINGS_SYS_RELOAD](" ");
        }
        if (settingsInfo.applyNetwork == 1)
        {
            syslog(LOG_INFO, "SETTINGS_NET_RELOAD");
            runSetingsCmd[SETTINGS_NET_RELOAD](" ");
        }
        if(settingsInfo.setDHCP == 1)
        {
            //TODO konst
        }
    }
    return ERR_OK;
}

static void runReset(char* inData)
{
    char value[128];
    extractValue(inData, value);
    FILE *pipe = openPipe(settings_commandTable[SETTINGS_DEFAULT_ALL]);
}

static void runReboot(char* inData)
{
    char value[128];
    extractValue(inData, value);
    FILE *pipe = openPipe(settings_commandTable[SETTINGS_REBOOT_ALL]);
}

static void runSetDhcp(const uint8_t* inData)
{
    settingsInfo.setDHCP = 1;
    openPipe("uci del network.lan.ipaddr");
    openPipe("uci del network.lan.netmask");
    openPipe("uci del network.lan.gateway");
    openPipe("uci set network.lan.proto='dhcp'");
}

static void runSetIp(char* inData)
{
    char addedData[128];
    char output[128]; //"network.lan.proto='dhcp'";

    FILE *pipe = openPipe("uci show network.lan.proto");
    while (fgets(output, OUTPUT_MAX_LENGTH, pipe) != NULL)
    {
        char mainCommand[128] = "uci set network.lan.ipaddr=";
        replaceSymbols(inData, '[', '\'');
        replaceSymbols(inData, ']', '\'');

        strcpy(addedData, inData);
        mainCommand[strcspn(mainCommand, "\n")] = '\0';
        addedData[strcspn(addedData, "\n")] = '\0';
        strcat(mainCommand, addedData);
        openPipe("uci set network.lan.proto='static'");
        openPipe(mainCommand);
        openlog("tfdm", LOG_PID, LOG_USER);
        syslog(LOG_INFO, "set IP: %s", mainCommand);
        closelog();
        settingsInfo.applyNetwork = 1;
    }
}

static void runSetMask(char* inData)
{
    char addedData[128];
    char mainCommand[128] = "uci set network.lan.netmask=";
    replaceSymbols(inData, '[', '\'');
    replaceSymbols(inData, ']', '\'');

    strcpy(addedData, inData);
    mainCommand[strcspn(mainCommand, "\n")] = '\0';
    addedData[strcspn(addedData, "\n")] = '\0';
    strcat(mainCommand, addedData);
    openPipe(mainCommand);
    openlog("tfdm", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "set Mask Command: %s", mainCommand);
    closelog();
    settingsInfo.applyNetwork = 1;
}

static void runSetGate(char* inData)
{
    char tempData[SETTINGS_VALUE_LENGTH];
    strcpy(tempData, inData);
    int validGateWay = checkGateWayValid(tempData);
    if(!validGateWay)
    {
        char addedData[128];
        char mainCommand[128] = "uci set network.lan.gateway=";
        replaceSymbols(inData, '[', '\'');
        replaceSymbols(inData, ']', '\'');
        strcpy(addedData,  inData);
        mainCommand[strcspn(mainCommand, "\n")] = '\0';
        addedData[strcspn((char *) addedData, "\n")] = '\0';
        strcat(mainCommand, addedData);
        openPipe(mainCommand);
        openPipe("uci commit network");

        openlog("tfdm", LOG_PID, LOG_USER);
        syslog(LOG_INFO, "set Gate Way: %s", mainCommand);
        closelog();
    }
    else
    {
        printf("GateWay is FF.FF.FF.FF \n");
        openPipe("uci del network.lan.gateway");
        openPipe("uci commit network");
    }
    settingsInfo.applyNetwork = 1;
}

static void runSetDescription(char* inData)
{
    size_t len = strlen(inData);
    if(len>0)
    {
        char addedData[128];
        char mainCommand[128];
        replaceSymbols(inData, '[', '\'');
        replaceSymbols( inData, ']', '\'');

        strcpy(addedData, inData);
        strcpy(mainCommand, settings_commandTable[SETTINGS_SYSTEM_NAME]);
        mainCommand[strcspn(mainCommand, "\n")] = '\0';
        if (!locationTestFlag)
        {
            addedData[strcspn(addedData, "\n")] = '\0';
        }
        strcat(mainCommand, addedData);
        openPipe(mainCommand);
        openPipe("uci commit system");
        openlog("tfdm", LOG_PID, LOG_USER);
        syslog(LOG_INFO, "set Description: %s", mainCommand);
        closelog();
        settingsInfo.applySystem = 1;
    }
}

static void runSetLocation(char* inData)
{
    size_t len = strlen(inData);
    if(len>0)
    {
        char addedData[128];
        char mainCommand[128];
        replaceSymbols(inData, '[', '\'');
        replaceSymbols( inData, ']', '\'');

        strcpy(addedData, inData);
        strcpy(mainCommand, settings_commandTable[SETTINGS_SYSTEM_LOCATION]);
        mainCommand[strcspn(mainCommand, "\n")] = '\0';
        if (!locationTestFlag)
        {
            addedData[strcspn(addedData, "\n")] = '\0';
        }
        strcat(mainCommand, addedData);
        strcat(mainCommand, "\0");
        openPipe(mainCommand);
        openPipe("uci commit system");
        openlog("tfdm", LOG_PID, LOG_USER);
        syslog(LOG_INFO, "set Location: %s", mainCommand);
        closelog();
        settingsInfo.applySystem = 1;
    }
}

void setLocationTestFlag(int flag)
{
    locationTestFlag = flag;
}

SETTINGS_INFO_t *getSettingInfo(void)
{
    return &settingsInfo;
}

static void runSetSnmp(char* inData)
{
    size_t len = strlen(inData);
    if(len>0)
    {
        char addedData[128];
        char mainCommand[128];

        replaceSymbols(inData, '[', '\'');
        replaceSymbols(inData, ']', '\'');

        strcpy(addedData, inData);
        strcpy(mainCommand,  settings_commandTable[SETTINGS_SNMP]);
        mainCommand[strcspn(mainCommand, "\n")] = '\0';
        addedData[strcspn(addedData, "\n")] = '\0';
        strcat(mainCommand, addedData);
        openPipe(mainCommand);
        openPipe("uci commit snmpd");
        openlog("tfdm", LOG_PID, LOG_USER);
        syslog(LOG_INFO, "set SNMP: %s", mainCommand);
        closelog();
        openPipe("/etc/init.d/snmpd reload");
        settingsInfo.applyNetwork = 1;
    }
}

static void runSystemRestart(char* inData)
{
    openPipe(settings_commandTable[SETTINGS_SYS_RELOAD]);
}

static void runNetworkRestart(char* inData)
{
    openlog("tfdm", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "network reload");
    closelog();
    openPipe(settings_commandTable[SETTINGS_NET_RELOAD]);
}

int checkGateWayValid(char *ip)
{
    size_t len = strlen(ip);
    replaceSymbols(ip, '.', ' ');
    replaceSymbols(ip, '[', ' ');
    replaceSymbols(ip, ']', ' ');
    int gatewayFlag = 0;
    splited_line_t splitLine;
    split_line(ip, " ", &splitLine);

    if (splitLine.splitLineMax != 3)
    {
        gatewayFlag = 5;
    }
    else
    {
        for (int i = 0; i <= splitLine.splitLineMax ; i++)
        {
            if (atoi(splitLine.tokens[i]) == 0xFF)
            {
                printf("gatewayFlag = %d\n", gatewayFlag);
                gatewayFlag++;
            }
        }
    }

    return gatewayFlag;
}