//
// Created by sheverdin on 3/21/24.
//

#include <signal.h>
#include "main_app.h"
#include "client_comm.h"
#include "events_lib/events_handlers.h"
#include "utils_lib/utils_module.h"

static void getDemonParam(void);
static void mainHandler(int i2c_fd);

static void* runMainHandler(void *args);
static void* runComHandler(void *args);
static void* runEventsHandler(void *args);
static void eventsHandler();
static void do_gpio_reset(void);


uint8_t ResetState = 0;
CRITICAL_EVENT_t criticalEvent = CRITICAL_IDLE;

struct timer events_timer;

mainThreads_t mainThreads[] =
{
    &runMainHandler,
    &runComHandler,
    &runEventsHandler
};

app_param appParam = {
    .debugMode            = 1,
    .delayHiPriority      = 3,
    .delayLowPriority     = 30,
    .timeAlarm  = 0
};

void setResetState(uint8_t resetState)
{
    if (resetState == 1 || resetState == 0){
        ResetState = resetState;
    }
    else
    {
        printf("Error reset State value\n");
    }
}

uint32_t pause_sec = 0;

void setPause(uint32_t sec)
{
    pause_sec = sec;
    //printf("SECOND %d\n", pause_sec);
}

void mainInit(int argc, char **argv)
{
    if (argc >= 2){
        if (argv[1][0] == '-'){
            //printf("argv[1][1] == \'-\' \n");
            setTestMode(argv[1][1]);
        }

        //printf("argv[2] = %s\n", argv[2]);
        //uint32_t pauseSec = (uint32_t)strtol(argv[2],NULL, 10);

        //printf("pauseSec = %d\n", pauseSec);
        //setPause(pauseSec);
    }
    if(isDebugMode())
    {
        printf("tf_hwsys_demon VERSION: %s\n", VERSION);
    }

    getDemonParam();
    runTimer(&appParam.timeAlarm ,appParam.delay);
    int len2cLenMsg = i2c_get_MsgSize();
    //printf("len i2c Len Msg = %d\n", len2cLenMsg);
}

void mainApp(void)
{
   // printf(" --------------- >>> Main App HW_SYS_D\n");

    uint8_t p_err = createThread(THREADS_NUM, mainThreads);
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

static void* runMainHandler(void *args)
{
    int i2c_fd;
    open_i2c(&i2c_fd);
    while (1)
    {
        mainHandler(i2c_fd);
    }
    return EXIT_SUCCESS;
}

static void mainHandler(int i2c_fd)
{
    //printf("================ mainHandler =================\n");
    static uint8_t errorCode = 0;
    static uint8_t errorCount = 0;
    static uint8_t prevResetCount = 0;
    gpio_param_t gpio_param;

    if(read_i2c(&i2c_fd) == EXIT_FAILURE)
         errorCount++;
    else
         errorCount = 0;

    if (errorCount > MAX_I2C_ERROR)
    {
        errorCount = 0;
        gpio_resetIoMcu();
    }

    //test();
}

static void* runComHandler(void *args)
{
    int sock_fd, client_fd;
    socket_asyncServer(&sock_fd);
    CLIENT_STATE clientState = SOCKET_READ_OK;
    static CRITICAL_EVENT_t com_prevResetState = reBoot;

    while (1)
    {
        //printf("before accept\n");
        accept_socket(sock_fd, &client_fd);
        //printf("after accept\n");
        if (client_fd == -1)
        {
            continue;
        }
        //fprintf(stderr, "accept_socket\n");
        clientState = clientHandler(client_fd);
        //fprintf(stderr, "MAIN_APP clientState = %d\n", clientState);

        if (clientState == SOCKET_RESET)
        {
            //printf("clientState == SOCKET_RESET\n");
            //gpio_resetHandler();
        }
    }
}

static void* runEventsHandler(void *args)
{
    uint8_t  crit_pause_sec = 3;
    timer_set(&events_timer,crit_pause_sec);
    while (1)
   {
       eventsHandler();
   }
    return EXIT_SUCCESS;
}

void eventsHandler()
{
    if(timer_expired(&events_timer)) {
        i2c_events_handler();
        timer_reset(&events_timer);
    }
    sleep(1);
}

static void getDemonParam(void)
{
    printf("get demon param\n");
}






