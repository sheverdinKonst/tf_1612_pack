//
// Created by sheverdin on 6/24/24.
//

#include "ar_mainApp.h"
#include "thread_lib/thread_module.h"
#include "utils_lib/utils_module.h"

#include "client_module.h"
#include "events_module.h"
#include "test_module.h"
#include "init_module.h"
#include "port_module.h"

static void* runTestHandler(void *args);
static void* runComHandler(void *args);
static void* runEventsHandler(void *args);

#define THREADS_NUM (3)

mainThreads_t ar_mainThreads[] =
{
    &runTestHandler,
    &runComHandler,
    &runEventsHandler
};

void ar_mainInit(int argc, char **argv)
{
    if (argc == 2){
        if (argv[1][0] == '-')
        {
            setTestMode(argv[1][1]);
        }
    }

    uint8_t *isDebug = isDebugMode();
    if(*isDebug)
    {
        printf("tf_autorestart VERSION: %s\n", VERSION);
    }

    getConfig();
    getPortsParam();
    rebootParamInit();
    runTimer();
}

void ar_mainApp(void)
{
    uint8_t p_err = createThread(THREADS_NUM, ar_mainThreads);
    if (p_err != 0)
    {
        printf("ERROR Create thread\n");
    }
    p_err = 0;
    p_err = joinThread(THREADS_NUM);
    if (p_err != 0)
    {
        printf("ERROR Join thread\n");
    }
}

static void* runTestHandler(void *args)
{
    config_type configOptions[MAX_PARM];
    for (int i = 0; i < MAX_PARM; i++)
    {
        getConfigOptions(&configOptions[i], i);
    }
    while (1)
    {
        testHandler();
        sleep(configOptions[TEST_DELAY].value);
    }
    return NULL;
}

static void* runComHandler(void *args)
{
    int sock_fd, client_fd;
    socket_asyncServer(&sock_fd);
    CLIENT_STATE clientState = SOCKET_READ_OK;

    while (1)
    {
        accept_socket(sock_fd, &client_fd);
        if (client_fd == -1)
        {
            continue;
        }
        clientState = clientHandler(client_fd);
    }
    return NULL;
}

static void* runEventsHandler(void *args)
{
    config_type configOptions[MAX_PARM];
    for (int i = 0; i < MAX_PARM; i++)
    {
        getConfigOptions(&configOptions[i], i);
    }
    while (1)
    {
        eventsHandler();
        sleep(configOptions[TEST_DELAY].value);
    }
   return NULL;
}







