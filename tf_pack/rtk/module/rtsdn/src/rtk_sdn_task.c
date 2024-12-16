/*
 * Copyright (C) 2017 Realtek Semiconductor Corp, EstiNet Technologies Inc.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corp., EstiNet Technologies Inc. and/or its licensors, and only
 * be used, duplicated, modified or distributed under the authorized
 * license from Realtek and EstiNet.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER THIS LICENSE OR
 * COPYRIGHT LAW IS PROHIBITED.
 *
 * $Revision: 1 $
 * $Date: 2016-11-03 10:57:40 +0800 (Thur, 03 Nov 2016) $
 *
 * Purpose : Definition a task to get some statistic periodicly.
 *
 * Feature : The file have include the following module and sub-modules
 *            1) Task Main
 *            2) Task Initialization
 *            3) Task Destroy
 *
 */

#include <time.h>
#include <pthread.h>
#include "rtk_sdn_task.h"
#include "sdn_db.h"
#include "sdn_hal.h"

#define RTK_SDN_CHECK_IDLE_BIT_INTERVAL 1
static void stats_task_main(void);
static void rtk_sdn_flow_idle_handler(void);
static time_t start_time;
static time_t now_time;
static time_t idle_check_start;
static time_t idle_check_now;
static time_t used_time = 0;
BOOL_T ovs_terminated = TRUE;
BOOL_T ovs_is_idlehandler=FALSE;

#if defined(CONFIG_SDK_RTL9310)
static void rtk_sdn_flow_idle_handler()
{
   uint8_t  table_id        = 0;
   uint32_t unit_id         = SDN_HAL_MY_UNIT_ID();
   uint32_t flow_id         = 0;
   //uint32_t max_entry_num   = 0;
   uint32_t flow_hit_status = 0;
   sdn_db_flow_table_t *table_p = NULL;
    /*need to check flow */

   while(ovs_is_idlehandler)
   {
        if ((idle_check_now - idle_check_start) >= RTK_SDN_CHECK_IDLE_BIT_INTERVAL)
        {
            for (table_id = FT_PHASE_IGR_FT_0; table_id < FT_PHASE_END  ; table_id++)
            {
//#ifdef CONFIG_SDK_RTL9310 //L2 L3 table shall get hit bit in furture
//                IF_TABLE_COUNTER_IS_NOT_AVAILABLE(table_id)
//                {
//                    continue;
//                }
//#endif

                table_p = sdn_db_get_flow_table(table_id);


                if ((table_p == NULL)
                   || (table_p->flow_entry == NULL)
                   )
                {
                    continue;
                }

                for (flow_id = 0; flow_id < table_p->flow_count; flow_id++)
                {
                    if (table_p->flow_entry[flow_id].idle_timeout.idle_time == 0)
                    {
                        continue;
                    }

#if defined(CONFIG_SYS_RTL9310)
                    IF_TABLE_IS_L2ORL3(table_id)
                    {
                        sdn_hal_l2andl3flowentry_hitstatus_get(&table_p->flow_entry[flow_id], &flow_hit_status);
                    }
                    else
#endif
                    {
                        sdn_hal_flow_hitstatus_get(unit_id, flow_id, &flow_hit_status);
                    }

                    if (flow_hit_status)
                    {
                        time(&used_time);
                        flow_hit_status = 0;
                        if (SDN_DB_RETURN_OK != sdn_db_update_flow_entry_idletimeout(table_id, flow_id, used_time))
                        {
                            continue;
                        }
                    }
                }

            }
            time(&idle_check_start);
        }
        time(&idle_check_now);
        usleep(500);
    }
}
#endif

static void stats_task_main()
{
    hal_port_stats_t port_stats_data;
    uint8_t           table_id = 0;
    //int ret = RT_ERR_OK;
    uint16_t flow_id = 0;
    uint32_t counter_id = 0;
    uint32_t port_no = 0;
    uint32 table_cnt     = 0;
    uint64 flow_pkt_cnt     = 0;
    uint64 flow_byte_cnt     = 0;
    rtk_of_flowTblCntType_t table_cnt_type = 0;

    while(!ovs_terminated)
    {
        if ( (now_time - start_time) >= 1)  /* time unit is second*/
        {

        #ifdef  CONFIG_SDK_RTL9310
            for (table_id = FT_PHASE_IGR_FT_0; table_id < FT_PHASE_END  ; table_id++)
        #else
            for (table_id = 0; table_id < HAL_TABLE_NUM  ; table_id++)
        #endif
            {

                #ifdef CONFIG_SDK_RTL9310
                IF_TABLE_COUNTER_IS_NOT_AVAILABLE(table_id)
                {
                    continue;
                }
                #endif

                for (table_cnt_type = OF_FLOW_TBL_CNT_TYPE_LOOKUP; table_cnt_type < OF_FLOW_TBL_CNT_TYPE_END; table_cnt_type++)
                {
                    table_cnt = 0;
                    /*get packet_lookups */
                    if (sdn_hal_flow_table_stats_get(table_id, table_cnt_type, &table_cnt) == SDN_HAL_RETURN_OK)
                    {
                        //SDN_HAL_MSG("[%s:%d] table(%d) type(%s) counter(%d) \n", __FUNCTION__, __LINE__, table_id, table_cnt_type == OF_FLOW_TBL_CNT_TYPE_LOOKUP ? "look_up" :"match", table_cnt);
                        sdn_db_set_flow_table_stats(table_id, table_cnt_type, (uint64_t)table_cnt);
                    }
                    else
                    {
                        /*Fail, to fill invalid value.*/
                        sdn_db_set_flow_table_stats(table_id, table_cnt_type,  ~0);
                    }
                }

                for (counter_id = 0; counter_id < HAL_RULE_COUNTER_SIZE  ; counter_id++)
                {
                    /*Brandon:
                        1. because counter number is less than flow_id's. So looping by counter has more performance.
                        2. get flow entries which has assigned a counter.
                        3. And ,directly get from hal, maybe no need to  check if flow_id is binding a counter.
                    */
                    if (sdn_db_get_flow_by_counter_id(table_id, counter_id , &flow_id) != SDN_DB_RETURN_OK)
                    {
                        continue;
                    }

                    if (sdn_hal_flow_stats_periodic_get(table_id, flow_id, &flow_pkt_cnt, &flow_byte_cnt) == SDN_HAL_RETURN_OK)
                    {
                        sdn_db_set_flow_stats(table_id, flow_id, counter_id, flow_pkt_cnt, flow_byte_cnt);
                    }
                    else
                    {
                        /*Fail, to fill invalid value.*/
                        sdn_db_set_flow_stats(table_id, flow_id, counter_id,  ~(0),  ~(0));
                    }
                }
            }

            for (port_no = USR_PORT_1; port_no < sdn_db_port_max_get()/*USR_PORT_END*/  ; port_no++)
            {
                /* get all kinds of count from MIB
                       if success, save data from MIB.
                       if fail, save zero value.
                */
                memset(&port_stats_data, 0x0, sizeof(hal_port_stats_t));
                sdn_hal_port_stats_get(port_no, &port_stats_data);
                sdn_db_set_port_stats(port_no, &port_stats_data);
            }

            time(&start_time); // update start_time
        }
        time(&now_time);
        usleep(1000);
    }
}

int rtk_sdn_task_init(void)
{
    pthread_t th_id;
    pthread_t idle_th_id;
    int ret = RT_ERR_FAILED;

    /*pthread_attr_t th_attr;
        int stack_size = 37768; */
    time(&start_time);
    time(&now_time);
    time(&idle_check_start);
    time(&idle_check_now);
    /*
    if ( 0 != pthread_attr_init(&th_attr))
    {
        SDN_HAL_MSG("\r\n[%s-%d] thread attr is created failed \r\n", __FUNCTION__, __LINE__);
        return OPAL_RETURN_GENERAL_ERROR;
    }

    pthread_attr_setstacksize(&th_attr, stack_size);
    */
    ovs_terminated = FALSE;
    ovs_is_idlehandler = TRUE;

    if ( 0 !=  pthread_create(&th_id, NULL, (void *) stats_task_main, NULL))
    {
        SDN_HAL_MSG("\n[%s-%d] thread is created failed \r\n", __FUNCTION__, __LINE__);
        return ret;
    }

    if ( 0 !=  pthread_create(&idle_th_id, NULL, (void *) rtk_sdn_flow_idle_handler, NULL))
    {
        SDN_HAL_MSG("\n[%s-%d] thread is created failed \r\n", __FUNCTION__, __LINE__);
        return ret;
    }

    SDN_HAL_MSG("\n[%s-%d] init thread is created \r\n", __FUNCTION__, __LINE__);

    return RT_ERR_OK;
}

void rtk_sdn_task_destroy(void)
{
    ovs_terminated     = TRUE;
    ovs_is_idlehandler = FALSE;
}


