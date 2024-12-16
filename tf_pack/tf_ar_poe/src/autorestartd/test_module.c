//
// Created by sheverdin on 6/24/24.
//

#include "thread_lib/thread_module.h"
#include "utils_lib/utils_module.h"
#include "../global_includs.h"
#include "test_module.h"
#include "port_module.h"

static  error_code_t test_main(void);

pthread_t threadsMain[NUM_PORTS];

typedef void *(*runTest_t)(port_config_info_t *portConfigInfo);

static void* runTestDisable(port_config_info_t *portConfigInfo);
static void* runTestLink(port_config_info_t *portConfigInfo);
static void* runTestPing(port_config_info_t *portConfigInfo);
static void* runTestSpeed(port_config_info_t *portConfigInfo);

runTest_t runTest[test_max] =
{
    &runTestDisable,
    &runTestLink,
    &runTestPing,
    &runTestSpeed,
};

void testHandler(void)
{
    error_code_t errorCode = ERR_OK;
    errorCode |= test_main();
}

static  error_code_t test_main(void)
{
    error_code_t errorCode = ERR_OK;
    portInfo_t *portInfoArr = port_get_portInfoArr();

    for (int index = 0; index < NUM_PORTS; index++)
    {
        get_PortAndPoeState(index + 1);
        if (runTest[portInfoArr[index].portConfigInfo.test] == NULL)
        {
            printf("RUN TEST OBJ NOT FOUND");
        }
        else
        {
            if (pthread_create((pthread_t*)&threadsMain[index], NULL, (void *(*)(void *)) runTest[portInfoArr[index].portConfigInfo.test],
                               (uint8_t *)&portInfoArr[index].portConfigInfo))
            {
                perror("pthread_create:  \n") ;
                errorCode |= ERR_CREATE_THREAD;
                return ERR_CREATE_THREAD;
            }
        }
    }
    errorCode = ERR_OK;

    for (int port = 0; port < NUM_PORTS; port++)
    {
        int status = pthread_join(threadsMain[port], NULL);
        if (status != 0)
        {
            printf("ERR_JOIN_THREAD - port %d\n", port);
            errorCode |= ERR_JOIN_THREAD;
            return ERR_JOIN_THREAD;
        }
    }
    return ERR_OK;
}

static void* runTestDisable(port_config_info_t *portConfigInfo)
{
    port_runTestDisable(portConfigInfo->portNum);
    return NULL;
}

static void* runTestLink(port_config_info_t *portConfigInfo)
{
    port_runTestLink(portConfigInfo->portNum);
    return NULL;
}

static void* runTestPing(port_config_info_t *portConfigInfo)
{
    port_runTestLink(portConfigInfo->portNum);
    port_runTestPing(portConfigInfo->portNum);
     return NULL;
}

static void*  runTestSpeed(port_config_info_t *portConfigInfo)
{
    port_runTestLink(portConfigInfo->portNum);
    port_runTestSpeed(portConfigInfo->portNum);
    return NULL;
}


