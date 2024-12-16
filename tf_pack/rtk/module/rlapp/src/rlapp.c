/*
 * Copyright (C) 2018 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 *
 * Purpose : Real-APP for simulate customer application code.
 *
 */


/*
 * Include Files
 */
#include <common/rt_autoconf.h>
#include <common/type.h>
#include <common/rt_type.h>
#include <common/rt_error.h>

#include <osal/lib.h>
#include <osal/print.h>
#include <osal/thread.h>
#include <osal/sem.h>
#include <osal/time.h>
#include <rlapp.h>
#include <rlapp_stack.h>


/*
 * Symbol Definition
 */
#define RLAPP_TASK_EVENT_TIMER      (0x1 << 0)
#define RLAPP_TASK_EVENT_EXIT       (0x1 << 1)

typedef struct rlapp_task_event_s
{
    uint32          bits;
} rlapp_task_event_t;




/*
 * Data Declaration
 */
uint32  rlapp_task_sem = 0;
uint32  rlapp_task_scan_interval_us = (3 * 1000000);
rlapp_task_event_t  g_rlapp_task_event = { .bits = 0 };

/*
 * Macro Declaration
 */
#define RLAPP_TASK_EVENT_SET(_e)        (g_rlapp_task_event.bits |= (_e))
#define RLAPP_TASK_EVENT_CLEAR(_e)      (g_rlapp_task_event.bits &= ~(_e))
#define RLAPP_TASK_EVENT(_e)            (g_rlapp_task_event.bits & (_e))


/*
 * Function Declaration
 */




void
rlapp_task_wait_event(void)
{
    if (osal_sem_take(rlapp_task_sem, rlapp_task_scan_interval_us) != RT_ERR_OK)
        RLAPP_TASK_EVENT_SET(RLAPP_TASK_EVENT_TIMER);
}


void
rlapp_task(void *p)
{
    rlapp_stack_wait_rise_ready();

    rlapp_info("rlapp_task %d start", osal_thread_self());
    while(1)
    {
        rlapp_task_wait_event();
        if (RLAPP_TASK_EVENT(RLAPP_TASK_EVENT_EXIT))
        {
            RLAPP_TASK_EVENT_CLEAR(RLAPP_TASK_EVENT_EXIT);
            break;
        }

        if (RLAPP_TASK_EVENT(RLAPP_TASK_EVENT_TIMER))
        {
//            rlapp_stack_msg_send_test();
            RLAPP_TASK_EVENT_CLEAR(RLAPP_TASK_EVENT_TIMER);
        }
    }

    osal_memset(&g_rlapp_task_event, 0, sizeof(rlapp_task_event_t));


    return;
}



void
rlapp_task2(void *p)
{
    rlapp_stack_wait_rise_ready();

    rlapp_info("rlapp_task2 %d start\n", osal_thread_self());

    rlapp_stack_continue_send();

    rlapp_info("rlapp_task2 %d finish\n", osal_thread_self());




    return;
}


void
rlapp_task3(void *p)
{
    rlapp_stack_wait_rise_ready();

    rlapp_info("rlapp_task3 %d start\n", osal_thread_self());
    rlapp_stack_continue_send2();
    rlapp_info("rlapp_task3 %d finish\n", osal_thread_self());



    return;
}



/* Function Name:
 *      rlapp_init
 * Description:
 *      Initial Real-APP
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *
 */
int32
rlapp_init(void)
{
    rlapp_task_sem = osal_sem_create(0);
    if (0 == rlapp_task_sem)
    {
        rlapp_err("create sem fail!!");
    }

    rlapp_stack_init();


    if((osal_thread_t)NULL == osal_thread_create("Real-APP task", 0, 100, rlapp_task, 0))
    {
        rlapp_dbg("create fail!!");
    }

    if((osal_thread_t)NULL == osal_thread_create("Real-APP task2", 0, 100, rlapp_task2, 0))
    {
        rlapp_dbg("create fail!!");
    }

#if 0
    if((osal_thread_t)NULL == osal_thread_create("Real-APP task3", 0, 100, rlapp_task3, 0))
    {
        rlapp_dbg("create fail!!");
    }
#endif

    return RT_ERR_OK;
}



