//
// Created by sheverdin on 6/24/24.
//

#include <time.h>
#include "utils_lib/utils_module.h"
#include "../global_includs.h"
#include "events_module.h"
#include "port_module.h"
#include "init_module.h"
#include "client_module.h"

typedef  void (*eventHandler_t)(portInfo_t *portInfo, config_type *config);

static void rebootHandler(portInfo_t *portInfo, config_type *config);
static void test_ErrorHandler(portInfo_t *portInfo, config_type *config);
static void alarmHandler(portInfo_t *portInfo);

static void printResult(void);

eventHandler_t eventHandler[MAX_AR_STATE] =
{
     NULL,
     &test_ErrorHandler,
     &rebootHandler
};

void eventsHandler(void)
{
    portInfo_t *portInfoArr = port_get_portInfoArr();
    config_type *configInfo = init_getConfig();

    for (int index = 0; index < NUM_PORTS; index++)
    {
        if (portInfoArr[index].portResetInfo.state != IDLE_STATE)
        {
            if  (eventHandler[portInfoArr[index].portResetInfo.state] == NULL)
            {
                printf("eventsHandler OBJ NOT FOUND\n");
            }
            else
            {
                eventHandler[portInfoArr[index].portResetInfo.state](&portInfoArr[index], configInfo);
            }
        }
        else if (portInfoArr[index].portResetInfo.state == IDLE_STATE)
        {
            if (portInfoArr[index].portResetInfo.event & TEST_OK)
            {
                portInfoArr[index].portResetInfo.event &= (~TEST_OK);
                portInfoArr[index].portResetInfo.state = REGULAR_STATE;
                portInfoArr[index].portResetInfo.errorCode &= (~ERR_PORT_SHUTDOWN);
            }
        }

        if (portInfoArr[index].portConfigInfo.alarm == enable)
        {
            alarmHandler(&portInfoArr[index]);
        }
    }
    uint8_t *isDebug = isDebugMode();
    if(*isDebug)
    {
        uint8_t *isRcieved = is_ReceivedMsg();
        if (*isRcieved)
            printResult();
    }
}

static void rebootHandler(portInfo_t *portInfo, config_type *config)
{
    uint8_t portNum = portInfo->portConfigInfo.portNum;

    switch (portInfo->portRebootInfo.rebootState)
    {
        case REBOOT_STATE_IDLE:
        {
            if (portInfo->portResetInfo.event & AUTO_RESTART)
            {
                portInfo->portResetInfo.event &= (~AUTO_RESTART);
                autoResetHandler(portNum, config[MAX_RESET].value);
            }

            if (portInfo->portResetInfo.event & MANUAL_RESTART)
            {
                if (portInfo->portStateInfo.port_state == enable && portInfo->portStateInfo.poe_state == enable)
                {
                    manualResetHandler(portNum);
                    portInfo->portResetInfo.event &= (~MANUAL_RESTART);
                }
            }
        }
        break;

        case REBOOT_STATE_INIT:
        {
            portInfo->portRebootInfo.rebootState = REBOOT_STATE_POE_DOWN;
            portInfo->portRebootInfo.rebootEvent |= REBOOT_EVENT_START;
        }
        break;

        case REBOOT_STATE_POE_DOWN:
        {
            if (portInfo->portRebootInfo.rebootEvent & REBOOT_EVENT_START)
            {
                portInfo->portRebootInfo.rebootEvent &= (~REBOOT_EVENT_START);
                poe_Control(portNum, POE_DOWN);
                openlog("tf_autoresart", LOG_PID, LOG_USER);
                syslog(LOG_INFO, "REBOOT STATE POE_DOWN , port - %d", portNum);
                closelog();
                portInfo->portRebootInfo.rebootTimeStart = portInfo->portRebootInfo.rebootDelay + time(NULL);
            }
            portInfo->portRebootInfo.rebootEvent |= (time(NULL) >= portInfo->portRebootInfo.rebootTimeStart)? REBOOT_EVENT_TIMER_STOP : REBOOT_EVENT_IDLE;
            if (portInfo->portRebootInfo.rebootEvent & REBOOT_EVENT_TIMER_STOP)
            {
                openlog("tf_autoresart", LOG_PID, LOG_USER);
                syslog(LOG_INFO, "REBOOT STATE POE_DOWN , port - %d", portNum);
                closelog();
                portInfo->portRebootInfo.rebootState = REBOOT_STATE_POE_UP;
            }
        }
        break;

        case REBOOT_STATE_POE_UP:
        {
            portInfo->portRebootInfo.rebootEvent &= (~REBOOT_EVENT_TIMER_STOP);
            openlog("tf_autoresart", LOG_PID, LOG_USER);
            if (portInfo->portStateInfo.poe_state == enable && portInfo->portStateInfo.port_state == enable)
            {
                poe_Control(portNum, POE_UP);
                syslog(LOG_INFO, "POE UP - DONE, port - %d\n", portNum);
            }
            else
            {
                syslog(LOG_INFO, "POE UP - PoE in state disable %d\n", portNum);
            }
            closelog();
            portInfo->portResetInfo.state = REGULAR_STATE;
            portInfo->portRebootInfo.rebootState = REBOOT_STATE_IDLE;
            portInfo->portRebootInfo.rebootEvent |= REBOOT_EVENT_IDLE;
            portInfo->portResetInfo.errorCode &= (~ERR_REBOOTING);
            portInfo->portResetInfo.errorCode &= (~ERR_MANUAL_REBOOT);
            if (portInfo->portResetInfo.event & MANUAL_RESTART)
            {
                portInfo->portResetInfo.event &= (~MANUAL_RESTART);
                portInfo->portResetInfo.errorCode &= (~ERR_PORT_SHUTDOWN);
                getPortParam(portNum - 1, 1);
            }
        }
        break;
    }
}

static void test_ErrorHandler(portInfo_t *portInfo, config_type *config)
{
    if (portInfo->portResetInfo.errorCode & ERR_OK)
    {
        if (portInfo->portResetInfo.errorTestCount > 0)
        {
            portInfo->portResetInfo.errorTestCount--;
        }
        if (portInfo->portResetInfo.errorTestCount == 0)
        {
            portInfo->portResetInfo.errorCode &= (~ERR_PORT_SHUTDOWN);
            portInfo->portResetInfo.resetCount = 0;
        }
    }

    if (portInfo->portResetInfo.errorCode & ERR_PORT_DISABLE ||
        portInfo->portResetInfo.errorCode & ERR_TEST_DISABLE ||
        portInfo->portResetInfo.errorCode & ERR_PORT_SHUTDOWN ||
        portInfo->portResetInfo.errorCode & ERR_POE_DOWN ||
        portInfo->portResetInfo.errorCode & ERR_PoE_DISABLE)
        // do nothing
        ;
    else
    {
        if (portInfo->portResetInfo.errorCode & ERR_TEST_LINK
        || portInfo->portResetInfo.errorCode & ERR_TEST_SPEED
        || portInfo->portResetInfo.errorCode & ERR_TEST_PING)
        {
            portInfo->portResetInfo.errorTestCount++;
            if (portInfo->portResetInfo.errorTestCount >= config[TEST_NUM].value)
            {
                if (portInfo->portResetInfo.state != REBOOT_STATE)
                {
                    portInfo->portResetInfo.event |= AUTO_RESTART;
                    portInfo->portResetInfo.state = REBOOT_STATE;
                }
            }
        }
    }
}

static void printResult(void)
{
    portInfo_t *portInfoArr = port_get_portInfoArr();

    static uint8_t testCount = 0;
    static uint8_t errorCode = 0;

    for (int port = 0; port < NUM_PORTS; port++)
    {
        printf("-----------------  portConfigInfo[ %d ] - \n", port + 1);
        printf("portNum     = %d\n", portInfoArr[port].portConfigInfo.portNum);
        printf("test        = %d\n", portInfoArr[port].portConfigInfo.test);
        printf("errorCode   = %x\n", portInfoArr[port].portResetInfo.errorCode);
        printf("\n-----------------  \n");
        printf("\t\ttimeStr               = %s\n", portInfoArr[port].portConfigInfo.timeStr);
        printf("\t\talarm down        = %d\n", portInfoArr[port].portConfigInfo.timeAlarm[time_down].status);
        printf("\t\ttime remain down  = %d\n", portInfoArr[port].portConfigInfo.timeAlarm[time_down].remainTime);
        printf("\t\talarm up          = %d\n", portInfoArr[port].portConfigInfo.timeAlarm[time_up].status);
        printf("\t\ttime rimain up    = %d\n", portInfoArr[port].portConfigInfo.timeAlarm[time_up].remainTime);
        printf("\n-----------------  \n");

        if (portInfoArr[port].portConfigInfo.test == test_disable)
        {
            printf("test disable -- \n");
        } else if (portInfoArr[port].portConfigInfo.test == test_ping)
        {
            printf("reply_time      = %d\n", portInfoArr[port].portConfigInfo.reply_time);
        } else if (portInfoArr[port].portConfigInfo.test == test_speed)
        {
            printf("rx_speed_Kbit   = %d\n", portInfoArr[port].portConfigInfo.rx_speed_Kbit);
            printf("rx_delta        = %d\n", portInfoArr[port].portConfigInfo.rx_delta_byte);
        }

        printf("test Count        = %d\n", portInfoArr[port].portResetInfo.errorTestCount);
        printf("reset Count       = %d\n", portInfoArr[port].portResetInfo.resetCount);
        printf("total Reset Count = %d\n", portInfoArr[port].portResetInfo.totalResetCount);
        printf("reset event       = %X\n", portInfoArr[port].portResetInfo.event);
        printf("reset state       = %d\n", portInfoArr[port].portResetInfo.state);
        printf("\n*********************************************\n");
        printf("reboot Event      = %x\n", portInfoArr[port].portRebootInfo.rebootEvent);
        printf("reboot State      = %d\n", portInfoArr[port].portRebootInfo.rebootState);
        printf("reboot TimeStart  = %d\n", portInfoArr[port].portRebootInfo.rebootTimeStart);
        printf("reboot Delay      = %d\n", portInfoArr[port].portRebootInfo.rebootDelay);
    }
    set_notRecivedMsg();
}
static void alarmHandler(portInfo_t *portInfo)
{
    port_runTestTime(portInfo->portConfigInfo.portNum);

    if (portInfo->portResetInfo.state != REBOOT_STATE)
    {
        if (portInfo->portConfigInfo.timeAlarm[time_down].status == enable)
        {
            portInfo->portConfigInfo.timeAlarm[time_down].status = disable;
            portInfo->portResetInfo.errorCode |= ERR_POE_DOWN;
            poe_Control(portInfo->portConfigInfo.portNum, POE_DOWN);
            portInfo->portResetInfo.totalResetCount++;
            portInfo->portConfigInfo.poe_state = 0;
            char  timeDateStr[32];
            strcpy(timeDateStr, "");
            getTimeDate(timeDateStr);
            strcpy(portInfo->portConfigInfo.timeStr, "");
            strcpy(portInfo->portConfigInfo.timeStr, timeDateStr);
        }

        if (portInfo->portConfigInfo.timeAlarm[time_up].status == enable)
        {
            portInfo->portConfigInfo.timeAlarm[time_up].status = disable;
            poe_Control(portInfo->portConfigInfo.portNum, POE_UP);
            portInfo->portResetInfo.errorCode &= (~ERR_POE_DOWN);
            portInfo->portConfigInfo.poe_state = 1;
        }
    }
}


