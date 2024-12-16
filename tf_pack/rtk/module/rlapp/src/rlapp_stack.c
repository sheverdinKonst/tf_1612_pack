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
#include <osal/time.h>

#include <rlapp.h>
#include <hwp/hw_profile.h>
#include <rise.h>

#include <rise_main.h>
#include <rise_util.h>
#include <rise_pkt.h>
#include <rise_rail.h>

/*
 * Symbol Definition
 */

#define RLAPP_STACK_HOTSWAP_CB_ID             6


typedef struct rlapp_stack_cb_info_s
{
    uint32      box_present[RISE_MAX_BOX];
    uint32      cnt_remove;
    uint32      cnt_insert;
    uint32      cnt_tx_sync_msg;
    uint32      cnt_tx_async_msg;
    uint32      cnt_rx_msg;
} rlapp_stack_info_t;


#define RLAPP_MAGIC                     0xEEEE0000
#define RLAPP_MAGIC_ONEHOP              0xAAAA0000
#define RLAPP_MAGIC_ACK                 0xCCCC0000
#define RLAPP_MAGIC_BCAST               0xBBBB0000
#define RLAPP_MAGIC_TOMASTER            0xDDDD0000

typedef struct rlapp_stk_msgInfo_s
{
    uint32      magic;
    uint32      sync_ack_delay;
    uint32      cnt;
    uint32      appData;
    uint8       box_id;
    uint8       master_view_unit_id;
    uint8       ret_box_id;
    uint8       ret_local_unit;
    uint8       data[0];
} rlapp_stk_msgInfo_t;




/*
 * Macro Definition
 */
#define RLAPP_STK_PKT_SIZE(data_len)     (sizeof(rise_pkt_t) + (sizeof(rise_tlv_t) * 1) + data_len)


/*
 * Data Declaration
 */
rlapp_stack_info_t     rlappStkInfo;

uint32      rlapp_rpc_pri = 3;
uint32      rlapp_stk_data = 0xCF000000;


/*
 * Function Declaration
 */
int32 rlapp_stack_send(uint32 box_id, uint32 cpu_unit, uint32 pri, rlapp_stk_msgInfo_t *rlapp_msg_info, uint8 *data, int len, uint32 is_sync);
int32 rlapp_stack_msg_send(uint32 box_id, rise_boxProperty_t *boxProperty, rlapp_stk_msgInfo_t *rlapp_msg_info, uint32 is_sync);
int32 rlapp_stack_msg_testCase_syncAckDelay(uint8 myBoxID, uint32 delay_time);
int32 rlapp_stack_msg_testCase_syncAckDelayReturnInTime(uint8 myBoxID);
int32 rlapp_stack_msg_testCase_syncAckDelayOutOfTime(uint8 myBoxID);
int32 rlapp_stack_msg_testCase_sync(uint8 myBoxID);
int32 rlapp_stack_msg_testCase_syncAsyncLoop10(uint8 myBoxID);
int32 rlapp_stack_msg_testCase_changePri(uint8 myBoxID);
int32 rlapp_stack_msg_send_test(void);



/* for linked list utilities */


void
rlapp_dump_word(char *data, int len, const char *func_name, int line)
{
    int             i, tmpLen;
    unsigned int    *pW = (unsigned int *)data;
    //char            *pB;
    int             is_trailing = 0;

    osal_printf("(%u)(%s:%u) ptr: %08X, len %d\n", osal_thread_self(), func_name, line, (unsigned int)pW, len);

    tmpLen = len / sizeof(unsigned int);

    for (i = 0; i < tmpLen; i+=4, pW+=4)
    {
        //osal_printf("i %d\n", i);
        if ((i+3) < tmpLen)
        {
            osal_printf("[%4X] %08X %08X %08X %08X\n", (i*(int)sizeof(unsigned int)),
                        *(pW+0), *(pW+1), *(pW+2), *(pW+3));
        }
        else
        {
            break;
        }
    }

    if (i < tmpLen)
    {
        osal_printf("[%4X] ", (i*(int)sizeof(unsigned int)));
        for (; i < tmpLen; i++, pW++)
        {
            osal_printf("%08X ", *pW);
        }
        is_trailing = 1;
    }


    tmpLen = len % sizeof(unsigned int);
    if (tmpLen > 0)
    {
        //pB = data;
        if (!is_trailing)
            osal_printf("[%4X] ", (i*(int)sizeof(unsigned int)));

        for (i = (len - tmpLen); i < len; i++)
            osal_printf("%02X", data[i]);

        is_trailing = 1;
    }
    if (is_trailing)
        osal_printf("\n");
}

/* Function Name:
 *      rlapp_stack_hs_cb
 * Description:
 *      Hot swap callback function which will call by RISE.
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
rlapp_stack_hs_cb(rise_hotSwap_info_t *pInform, void *pUserData)
{
    int32   ret, i;
    rlapp_stack_info_t  *pRlappStackDb = pUserData;
    rise_boxProperty_t  boxProperty;


    osal_memset(&boxProperty, 0, sizeof(rise_boxProperty_t));
    switch(pInform->event)
    {
      case RISE_INFORM_EVENT_HOT_REMOVE:
        rlapp_info("Event: HOT-REMOVE box %u\n", pInform->boxId);
        pRlappStackDb->box_present[RISE_BOX_IDX(pInform->boxId)] = FALSE;
        pRlappStackDb->cnt_remove++;
        break;
      case RISE_INFORM_EVENT_HOT_INSERT:
        rlapp_info("Event: HOT-INSERT box %u\n", pInform->boxId);
        pRlappStackDb->box_present[RISE_BOX_IDX(pInform->boxId)] = TRUE;
        pRlappStackDb->cnt_insert++;
        break;
      default:
        rlapp_dbg("Unreconized event %u box %u\n", pInform->event, pInform->boxId);
        break;

    }


    if ((ret = rise_boxProperty_get(pInform->boxId, &boxProperty)) == RT_ERR_OK)
    {
        rlapp_info("hwp_id: type %u name=%s id=%u\n", boxProperty.hwp_id.type, boxProperty.hwp_id.name, boxProperty.hwp_id.id);
        for (i = 0; i < RISE_MAX_UNIT_PER_BOX; i++)
        {
            rlapp_info(" [%u]chip_id=%x global_unit_id=%u assigedDevID=%u\n", i,
                boxProperty.chip[i].chip_id, boxProperty.chip[i].global_unit_id, boxProperty.chip[i].assigedDevID);
        }
    }
    else
    {
        if (pInform->event == RISE_INFORM_EVENT_HOT_INSERT)
        {
            rlapp_info("rise_boxProperty_get fail %X\n", ret);
        }
        else
        {
            rlapp_info("rise_boxProperty_get(%X) ok\n", ret);
        }
    }

    if (pUserData != NULL)
    {
        rlapp_dbg("user-data cnt_insert:%u cnt_remove:%u txSync:%u txAsync:%u rx:%u\n", pRlappStackDb->cnt_insert, pRlappStackDb->cnt_remove,
                pRlappStackDb->cnt_tx_sync_msg, pRlappStackDb->cnt_tx_async_msg, pRlappStackDb->cnt_rx_msg);
    }
    else
    {
        rlapp_dbg("user-data is NULL\n");
    }


    return RT_ERR_OK;
}

/* Function Name:
 *      rlapp_stack_hotswap_registration
 * Description:
 *      Register hot swap callback function to RISE.
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
rlapp_stack_hotswap_registration(void)
{
    int32   ret;
    rise_cbInfo_t       hs_cb;

    rlapp_dbg("++++++++++++++++\n");
    osal_memset(&hs_cb, 0, sizeof(rise_cbInfo_t));
    hs_cb.cb_id = RLAPP_STACK_HOTSWAP_CB_ID;
    hs_cb.cb_func = rlapp_stack_hs_cb;
    hs_cb.pUserData = &rlappStkInfo;
    if ((ret = rise_hotSwap_callback_register(&hs_cb)) != RT_ERR_OK)
    {
        rlapp_dbg("rise_hotSwap_callback_register fail:%X\n", ret);
    }

    rlapp_dbg("----------------\n");
    return RT_ERR_OK;
}

char rlapp_stk_msg_data2[] = {'A','A','A','A','B','B','B','B','C','C','C','C','D','D','D','D','E','E','E','E','F','F','F','F',
                             'G','G','G','G','H','H','H','H','I','I','I','I','J','J','J','J','K','K','K','K','L','L','L','L',};

char rlapp_stk_msg_data3[] = {'M','M','M','M','N','N','N','N','O','O','O','O','P','P','P','P','Q','Q','Q','Q','R','R','R','R'};
int rlapp_stk_msg_len2 = sizeof(rlapp_stk_msg_data2);
int rlapp_stk_msg_len3 = sizeof(rlapp_stk_msg_data3);





uint32  send_msg_test_enable = 1;
#if 0
int32
rlapp_stack_msg_send_test(void) //no msg test
{
    return RT_ERR_OK;
}
#else
#define RLAPP_STK_TLV_TYPE          0x99


int32
rlapp_stack_send(uint32 box_id, uint32 cpu_unit, uint32 pri, rlapp_stk_msgInfo_t *rlapp_msg_info, uint8 *data, int len, uint32 is_sync)
{
    int32   ret;
    uint32              flag = 0;
    uint8 *rlapp_stk_msg_data;
    int rlapp_stk_msg_len;
    uint32  total_len;
    rlapp_stk_msgInfo_t *pAppMsg;
    //rise_railMsgBuf_t      *stk_buf;
    //rise_railMsgBuf_t      *ret_stk_buf = NULL;
    rise_pkt_t              *stk_pkt;
    rise_pkt_t              *ret_stk_pkt = NULL;
    rise_railTxDest_t       dest;
    drv_nic_pkt_t           *pNicPkt, *pNicPkt_ret;

    rlapp_stk_msg_data = data;
    rlapp_stk_msg_len = len;

    total_len = sizeof(rlapp_stk_msgInfo_t) + rlapp_stk_msg_len;

    if (is_sync == TRUE)
    {
        flag |= RTSTK_PKT_FLAG_RAIL_SYNC;
    }

    //rlapp_dbg("rpc_data:%s\n", rlapp_stk_msg_data);
    rlapp_dbg("rpc_total_len:%d data_len=%d is_sync=%u data: ++\n", total_len, rlapp_stk_msg_len, is_sync);
    rlapp_dump_word(rlapp_stk_msg_data, rlapp_stk_msg_len, __FUNCTION__, __LINE__);

    if ((ret = rise_railPkt_alloc(RLAPP_STK_PKT_SIZE(total_len), &stk_pkt)) != RT_ERR_OK)
    {
        rlapp_err("rise_railPkt_alloc fail %X\n", ret);
        return RT_ERR_OK;
    }

    rlapp_msg_info->appData = rlapp_stk_data++;
    pNicPkt = stk_pkt->pDrvPkt;
    rlapp_dbg("stk_pkt nicPkt=%X stkMsg=%X cpu_unit=%u pri=%u AppData=%X ack_delay=%u\n", (uint32)pNicPkt, (uint32)stk_pkt, cpu_unit, pri,
                    rlapp_msg_info->appData, rlapp_msg_info->sync_ack_delay);

    rlapp_msg_info->magic  = RLAPP_MAGIC;
    rlapp_msg_info->box_id = box_id;
    rlapp_msg_info->master_view_unit_id = cpu_unit;
    rlapp_msg_info->ret_box_id = 0;
    rlapp_msg_info->ret_local_unit = 0;

    stk_pkt->tlv_size = sizeof(rise_tlv_t) + rlapp_stk_msg_len;
    stk_pkt->tlv_cnt = 1;
    stk_pkt->tlv->type = RLAPP_STK_TLV_TYPE;
    stk_pkt->tlv->len_of_val  = rlapp_stk_msg_len;

    pAppMsg = (rlapp_stk_msgInfo_t *)stk_pkt->tlv->value;
    osal_memcpy(pAppMsg, rlapp_msg_info, sizeof(rlapp_stk_msgInfo_t));
    osal_memcpy(pAppMsg->data, rlapp_stk_msg_data, rlapp_stk_msg_len);

    /* Dump Data */
//    rlapp_dump_word(pNicPkt->data, pNicPkt->length, __FUNCTION__, __LINE__);

    if (flag & RTSTK_PKT_FLAG_RAIL_SYNC)
        rlappStkInfo.cnt_tx_sync_msg++;
    else
        rlappStkInfo.cnt_tx_async_msg++;

    flag |= RTSTK_PKT_FLAG_UNICAST;
    osal_memset(&dest, 0, sizeof(rise_railTxDest_t));
    dest.u.unit = cpu_unit;
    stk_pkt->pri = pri;
    stk_pkt->flag = flag;
    ret = rise_railPkt_tx(&dest, stk_pkt, &ret_stk_pkt);


    rlapp_dbg("%s TX-Done ret=%X stk_pkt=%X nicPkt=%X ret_stk_pkt=%X\n",
        ((flag & RTSTK_PKT_FLAG_RAIL_SYNC)?"SYNC":"ASYNC"), ret, (uint32)stk_pkt, (uint32)stk_pkt->pDrvPkt, (uint32)ret_stk_pkt);
    if (flag & RTSTK_PKT_FLAG_RAIL_SYNC)
    {
        pNicPkt_ret = (ret_stk_pkt != NULL) ? ret_stk_pkt->pDrvPkt : NULL;
        if ((ret_stk_pkt == NULL) || (pNicPkt_ret == NULL))
        {
            rlapp_err("ret_stk_pkt %X or ret_stk_pkt->pNicPkt is NULL\n", (uint32)ret_stk_pkt);
        }
        else
        {
            rlapp_dbg("ret_stk_pkt nicPkt=%X data=%X stPkt=%X:\n", (uint32)pNicPkt_ret, (uint32)pNicPkt_ret->data, (uint32)ret_stk_pkt);
            rlapp_dump_word(pNicPkt_ret->data, pNicPkt_ret->length, __FUNCTION__, __LINE__);

            pAppMsg = (rlapp_stk_msgInfo_t *)ret_stk_pkt->tlv->value;
            rlapp_dbg("GOT magic=%X box=%u unit=%u\n", pAppMsg->magic, pAppMsg->ret_box_id, pAppMsg->ret_local_unit);
            osal_memcpy(rlapp_msg_info, pAppMsg, sizeof(rlapp_stk_msgInfo_t));
        }

    }

    //rlapp_dbg("call rise_railPkt_free stk_pkt=%X\n", (uint32)stk_pkt);
    //rise_railPkt_free(stk_pkt);
    if (ret_stk_pkt)
    {
        rlapp_dbg("call rise_railPkt_free() ret_stk_pkt=%X\n", (uint32)ret_stk_pkt);
        rise_railPkt_free(ret_stk_pkt);
    }


    if (ret != RT_ERR_OK)
    {
        if (rlappStkInfo.box_present[RISE_BOX_IDX(box_id)] == FALSE)
        {
            rlapp_dbg("Box %u not insert yet!\n", box_id);
        }
        else
        {
#if 1
            rlapp_err("ERROR occur!\n");
#else
            send_msg_test_enable = 0;
            rlapp_err("ERROR occur, STOP test!\n");
#endif
        }
    }


    rlapp_dbg("rail tx %s ret:%x --\n", ((flag&RTSTK_PKT_FLAG_RAIL_SYNC)?"SYNC":"ASYNC"), ret);

    return RT_ERR_OK;


}

int32
rlapp_stack_msg_send(uint32 box_id, rise_boxProperty_t *boxProperty, rlapp_stk_msgInfo_t *rlapp_msg_info, uint32 is_sync)
{
    int32   new_api = TRUE;
    int32   ret;
    uint32              flag = 0;
    char *rlapp_stk_msg_data;
    int rlapp_stk_msg_len;
    uint32  cpu_unit, total_len;
    rlapp_stk_msgInfo_t *pAppMsg;
    rise_railTxDest_t   dest;
    drv_nic_pkt_t       *pNicPkt, *pNicPkt_ret;

    if (box_id % 2 == 0)
    {
        rlapp_stk_msg_data = rlapp_stk_msg_data2;
        rlapp_stk_msg_len = rlapp_stk_msg_len2;
    }
    else
    {
        rlapp_stk_msg_data = rlapp_stk_msg_data3;
        rlapp_stk_msg_len = rlapp_stk_msg_len3;
    }
    total_len = sizeof(rlapp_stk_msgInfo_t) + rlapp_stk_msg_len;

    if (is_sync == TRUE)
    {
        flag |= RTSTK_PKT_FLAG_RAIL_SYNC;
    }


    rlapp_dbg("rpc_data:%s\n", rlapp_stk_msg_data);
    rlapp_dbg("rpc_total_len:%d data_len=%d\n", total_len, rlapp_stk_msg_len);

    if (new_api)
    {
        //rise_railMsgBuf_t      *stk_buf;
        //rise_railMsgBuf_t      *ret_stk_buf = NULL;
        rise_pkt_t          *stk_pkt;
        rise_pkt_t          *ret_stk_pkt = NULL;
        if ((ret = rise_railPkt_alloc(RLAPP_STK_PKT_SIZE(total_len), &stk_pkt)) != RT_ERR_OK)
        {
            rlapp_err("rise_railPkt_alloc fail %X\n", ret);
            return RT_ERR_OK;
        }

        pNicPkt = stk_pkt->pDrvPkt;
        rlapp_msg_info->appData = rlapp_stk_data++;
        cpu_unit = boxProperty->chip[0].global_unit_id;
        rlapp_dbg("stk_pkt nicPkt=%X stkMsg=%X cpu_unit=%u pri=%u appData=%X\n", (uint32)pNicPkt, (uint32)stk_pkt, cpu_unit, rlapp_rpc_pri,
                    rlapp_msg_info->appData);

        rlapp_msg_info->magic  = RLAPP_MAGIC;
        rlapp_msg_info->box_id = box_id;
        rlapp_msg_info->master_view_unit_id = cpu_unit;
        rlapp_msg_info->ret_box_id = 0;
        rlapp_msg_info->ret_local_unit = 0;


        stk_pkt->tlv_size = sizeof(rise_tlv_t) + rlapp_stk_msg_len;
        stk_pkt->tlv_cnt = 1;
        stk_pkt->tlv->type = RLAPP_STK_TLV_TYPE;
        stk_pkt->tlv->len_of_val  = rlapp_stk_msg_len;

        pAppMsg = (rlapp_stk_msgInfo_t *)stk_pkt->tlv->value;
        osal_memcpy(pAppMsg, rlapp_msg_info, sizeof(rlapp_stk_msgInfo_t));
        osal_memcpy(pAppMsg->data, rlapp_stk_msg_data, rlapp_stk_msg_len);


        rlapp_dump_word(pNicPkt->data, pNicPkt->length, __FUNCTION__, __LINE__);

        if (flag & RTSTK_PKT_FLAG_RAIL_SYNC)
            rlappStkInfo.cnt_tx_sync_msg++;
        else
            rlappStkInfo.cnt_tx_async_msg++;
        flag |= RTSTK_PKT_FLAG_UNICAST;
        dest.u.unit = cpu_unit;
        stk_pkt->pri = rlapp_rpc_pri;
        stk_pkt->flag = flag;
        ret = rise_railPkt_tx(&dest, stk_pkt, &ret_stk_pkt);

        rlapp_dbg("%s TX-Done ret=%X stk_pkt=%X nicPkt=%X stkMsg=%X ret_stk_pkt=%X\n",
            ((flag & RTSTK_PKT_FLAG_RAIL_SYNC)?"SYNC":"ASYNC"), ret, (uint32)stk_pkt, (uint32)stk_pkt->pDrvPkt, (uint32)stk_pkt,
            (uint32)ret_stk_pkt);
        if (flag & RTSTK_PKT_FLAG_RAIL_SYNC)
        {
            pNicPkt_ret = (ret_stk_pkt != NULL) ? ret_stk_pkt->pDrvPkt : NULL;
            if ((ret_stk_pkt == NULL) || (pNicPkt_ret == NULL))
            {
                rlapp_err("ret_stk_pkt or ret_stk_pkt->pNicPkt is NULL\n");
            }
            else
            {
                rlapp_dbg("ret_stk_pkt:\n");
                rlapp_dump_word(pNicPkt_ret->data, pNicPkt_ret->length, __FUNCTION__, __LINE__);

                pAppMsg = (rlapp_stk_msgInfo_t *)ret_stk_pkt->tlv->value;
                rlapp_dbg("GOT magic=%X box=%u unit=%u\n", pAppMsg->magic, pAppMsg->ret_box_id, pAppMsg->ret_local_unit);
                osal_memcpy(rlapp_msg_info, pAppMsg->data, rlapp_stk_msg_len);
            }

        }

        //rlapp_dbg("call rise_railPkt_free stk_pkt=%X\n", (uint32)stk_pkt);
        //rise_railPkt_free(stk_pkt);
        if (ret_stk_pkt)
        {
            rlapp_dbg("call rise_railPkt_free() ret_stk_pkt=%X\n", (uint32)ret_stk_pkt);
            rise_railPkt_free(ret_stk_pkt);
        }


        if (ret != RT_ERR_OK)
        {
            if (rlappStkInfo.box_present[RISE_BOX_IDX(box_id)] == FALSE)
            {
                rlapp_dbg("Box %u not insert yet!\n", box_id);
            }
            else
            {
#if 1
                rlapp_err("ERROR occur!\n");
#else
                send_msg_test_enable = 0;
                rlapp_err("ERROR occur, STOP test!\n");
#endif
            }
        }

    }
    else
    {
        rlapp_err("not support\n");
    }
    rlapp_dbg("rail tx %s ret:%x\n", ((flag&RTSTK_PKT_FLAG_RAIL_SYNC)?"SYNC":"ASYNC"), ret);

    return RT_ERR_OK;
}


int32 msg_send_cnt = 0;



int32
rlapp_stack_msg_testCase_forceSend(uint8 dstBoxID, uint8 cpu_unit, uint32 delay_time, uint32 is_sync)
{
    int32 ret;
    rise_boxRole_t      box_role;
    rlapp_stk_msgInfo_t rlapp_msg_info;
    uint32              pri = 3;
    uint8 data[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
                      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, };


    if ((ret = rise_boxRole_get(&box_role)) != RT_ERR_OK)
    {
        rlapp_err("rise_boxRole_get fail! ret=%X\n", ret);
        return ret;
    }

    if (box_role == RISE_BOXROLE_SLAVE)
    {
        return RT_ERR_OK;
    }

    if (delay_time==500)
    {
        pri = 8;
    }

    rlapp_info("TEST: Send Message to BOX %u unit %u ++\n", dstBoxID, cpu_unit);
    osal_memset(&rlapp_msg_info, 0, sizeof(rlapp_stk_msgInfo_t));
    rlapp_msg_info.sync_ack_delay = delay_time; /* ms */
    rlapp_stack_send(dstBoxID, cpu_unit, pri/*pri*/, &rlapp_msg_info, data, 32, is_sync);
    rlapp_info("TEST: Send Message to BOX %u unit %u %s Magic=%X cnt=%u--\n", dstBoxID, cpu_unit,
            ((is_sync)?"SYNC":"ASYNC"), rlapp_msg_info.magic, rlapp_msg_info.cnt);

    return RT_ERR_OK;
}

int32
rlapp_stack_msg_oneHop(uint32 unit, uint32 port)
{
    rlapp_stk_msgInfo_t rlapp_msg_info;
    uint32              pri = 9;
    uint32              rlapp_stk_msg_len;
    int32               ret;
    rlapp_stk_msgInfo_t *pAppMsg;
    //rise_railMsgBuf_t      *stk_buf;
    //rise_railMsgBuf_t      *ret_stk_buf = NULL;
    rise_pkt_t      *stk_pkt;
    rise_pkt_t      *ret_stk_pkt = NULL;
    rise_railTxDest_t   dest;
    drv_nic_pkt_t       *pNicPkt;


    osal_memset(&dest, 0, sizeof(rise_railTxDest_t));
    dest.u.stPort.unit = unit;
    dest.u.stPort.port = port;

    rlapp_stk_msg_len = sizeof(rlapp_stk_msgInfo_t);
    if ((ret = rise_railPkt_alloc(RLAPP_STK_PKT_SIZE(rlapp_stk_msg_len), &stk_pkt)) != RT_ERR_OK)
    {
        rlapp_err("rise_railPkt_alloc fail %X\n", ret);
        return RT_ERR_OK;
    }

    pNicPkt = stk_pkt->pDrvPkt;
    rlapp_msg_info.appData = rlapp_stk_data++;
    rlapp_dbg("ONEHOP TX stk_pkt nicPkt=%X stkMsg=%X unit=%u port=%u pri=%u AppData=%X +++\n", (uint32)pNicPkt, (uint32)stk_pkt, unit, port, pri,
                    rlapp_msg_info.appData);
    osal_memset(&rlapp_msg_info, 0, sizeof(rlapp_stk_msgInfo_t));
    rlapp_msg_info.magic  = RLAPP_MAGIC_ONEHOP;

    stk_pkt->tlv_size = sizeof(rise_tlv_t) + rlapp_stk_msg_len;
    stk_pkt->tlv_cnt = 1;
    stk_pkt->tlv->type = RLAPP_STK_TLV_TYPE;
    stk_pkt->tlv->len_of_val  = rlapp_stk_msg_len;

    pAppMsg = (rlapp_stk_msgInfo_t *)stk_pkt->tlv->value;
    osal_memcpy(pAppMsg, &rlapp_msg_info, sizeof(rlapp_stk_msgInfo_t));
    //osal_memcpy(pAppMsg->data, rlapp_stk_msg_data, rlapp_stk_msg_len);

    stk_pkt->pri = pri;
    stk_pkt->flag = RTSTK_PKT_FLAG_BYHOP;
    ret = rise_railPkt_tx(&dest, stk_pkt, &ret_stk_pkt);

    rlapp_dbg("ONEHOP TX-Done ret=%X retBuf=%X \n", ret, (uint32)ret_stk_pkt);
    if (ret_stk_pkt != NULL)
    {
        pAppMsg = (rlapp_stk_msgInfo_t *)ret_stk_pkt->tlv->value;
        rlapp_dbg("GOT magic=%X box=%u unit=%u. free it.\n", pAppMsg->magic, pAppMsg->ret_box_id, pAppMsg->ret_local_unit);
        rise_railPkt_free(ret_stk_pkt);
    }

    //rlapp_dbg("call rise_railPkt_free stk_pkt=%X \n", (uint32)stk_pkt);
    //rise_railPkt_free(stk_pkt);


    rlapp_dbg("ONEHOP TX-Fihish ---\n");
    return RT_ERR_OK;
}



int32
rlapp_stack_msg_bcast(void)
{
    rlapp_stk_msgInfo_t rlapp_msg_info;
    uint32              pri = 1;
    uint32              rlapp_stk_msg_len;
    int32               ret;
    rlapp_stk_msgInfo_t *pAppMsg;
//    rise_railMsgBuf_t      *stk_buf;
//    rise_railMsgBuf_t      *ret_stk_buf = NULL;
    rise_pkt_t      *stk_pkt;
    rise_pkt_t      *ret_stk_pkt = NULL;
    rise_railTxDest_t   dest;
    drv_nic_pkt_t       *pNicPkt;

    osal_memset(&dest, 0, sizeof(rise_railTxDest_t));

    rlapp_stk_msg_len = sizeof(rlapp_stk_msgInfo_t);
    if ((ret = rise_railPkt_alloc(RLAPP_STK_PKT_SIZE(rlapp_stk_msg_len), &stk_pkt)) != RT_ERR_OK)
    {
        rlapp_err("rise_railPkt_alloc fail %X\n", ret);
        return RT_ERR_OK;
    }

    pNicPkt = stk_pkt->pDrvPkt;
    rlapp_msg_info.appData = rlapp_stk_data++;
    rlapp_dbg("BCAST TX stk_pkt nicPkt=%X stkMsg=%X pri=%u AppData=%X +++\n", (uint32)pNicPkt, (uint32)stk_pkt, pri,
                    rlapp_msg_info.appData);
    osal_memset(&rlapp_msg_info, 0, sizeof(rlapp_stk_msgInfo_t));
    rlapp_msg_info.magic  = RLAPP_MAGIC_BCAST;

    stk_pkt->tlv_size = sizeof(rise_tlv_t) + rlapp_stk_msg_len;
    stk_pkt->tlv_cnt = 1;
    stk_pkt->tlv->type = RLAPP_STK_TLV_TYPE;
    stk_pkt->tlv->len_of_val  = rlapp_stk_msg_len;

    pAppMsg = (rlapp_stk_msgInfo_t *)stk_pkt->tlv->value;
    osal_memcpy(pAppMsg, &rlapp_msg_info, sizeof(rlapp_stk_msgInfo_t));
    //osal_memcpy(pAppMsg->data, rlapp_stk_msg_data, rlapp_stk_msg_len);

    stk_pkt->pri = pri;
    stk_pkt->flag = RTSTK_PKT_FLAG_BROADCAST;
    ret = rise_railPkt_tx(&dest, stk_pkt, &ret_stk_pkt);

    rlapp_dbg("BCAST TX-Done ret=%X retBuf=%X \n", ret, (uint32)ret_stk_pkt);
    if (ret_stk_pkt != NULL)
    {
        pAppMsg = (rlapp_stk_msgInfo_t *)ret_stk_pkt->tlv->value;
        rlapp_dbg("GOT magic=%X box=%u unit=%u. free it.\n", pAppMsg->magic, pAppMsg->ret_box_id, pAppMsg->ret_local_unit);
        rise_railPkt_free(ret_stk_pkt);
    }

    //rlapp_dbg("call rise_railPkt_free stk_pkt=%X \n", (uint32)stk_pkt);
    //rise_railPkt_free(stk_pkt);


    rlapp_dbg("BCAST TX-Fihish ---\n");
    return RT_ERR_OK;
}




int32
rlapp_stack_msg_sendToMaster(uint32 delay_time, uint32 is_sync)
{
    rlapp_stk_msgInfo_t rlapp_msg_info;
    uint32              pri = 27;
    uint32              unit = 0;
    uint32              rlapp_stk_msg_len;
    int32               ret;
    rlapp_stk_msgInfo_t *pAppMsg;
    //rise_railMsgBuf_t      *stk_buf;
    //rise_railMsgBuf_t      *ret_stk_buf = NULL;
    rise_pkt_t      *stk_pkt;
    rise_pkt_t      *ret_stk_pkt = NULL;
    rise_railTxDest_t   dest;
    drv_nic_pkt_t       *pNicPkt;


    osal_memset(&dest, 0, sizeof(rise_railTxDest_t));
    dest.u.unit = 0;

    rlapp_stk_msg_len = sizeof(rlapp_stk_msgInfo_t);
    if ((ret = rise_railPkt_alloc(RLAPP_STK_PKT_SIZE(rlapp_stk_msg_len), &stk_pkt)) != RT_ERR_OK)
    {
        rlapp_err("rise_railPkt_alloc fail %X\n", ret);
        return RT_ERR_OK;
    }

    pNicPkt = stk_pkt->pDrvPkt;
    rlapp_msg_info.appData = rlapp_stk_data++;
    rlapp_dbg("toMaster TX stk_pkt nicPkt=%X stkMsg=%X unit=%u pri=%u AppData=%X MAGIC=%X +++\n", (uint32)pNicPkt, (uint32)stk_pkt, unit, pri,
                    rlapp_msg_info.appData, RLAPP_MAGIC_TOMASTER);
    osal_memset(&rlapp_msg_info, 0, sizeof(rlapp_stk_msgInfo_t));
    rlapp_msg_info.magic  = RLAPP_MAGIC_TOMASTER;

    stk_pkt->tlv_size = sizeof(rise_tlv_t) + rlapp_stk_msg_len;
    stk_pkt->tlv_cnt = 1;
    stk_pkt->tlv->type = RLAPP_STK_TLV_TYPE;
    stk_pkt->tlv->len_of_val  = rlapp_stk_msg_len;

    pAppMsg = (rlapp_stk_msgInfo_t *)stk_pkt->tlv->value;
    osal_memcpy(pAppMsg, &rlapp_msg_info, sizeof(rlapp_stk_msgInfo_t));
    //osal_memcpy(pAppMsg->data, rlapp_stk_msg_data, rlapp_stk_msg_len);

    stk_pkt->pri = pri;
    stk_pkt->flag = RTSTK_PKT_FLAG_UNICAST;
    if (is_sync)
        stk_pkt->flag |= RTSTK_PKT_FLAG_RAIL_SYNC;
    ret = rise_railPkt_tx(&dest, stk_pkt, &ret_stk_pkt);

    rlapp_dbg("toMaster TX-Done ret=%X retBuf=%X \n", ret, (uint32)ret_stk_pkt);
    if (ret_stk_pkt != NULL)
    {
        pAppMsg = (rlapp_stk_msgInfo_t *)ret_stk_pkt->tlv->value;
        rlapp_dbg("GOT magic=%X box=%u unit=%u. free it.\n", pAppMsg->magic, pAppMsg->ret_box_id, pAppMsg->ret_local_unit);
        rise_railPkt_free(ret_stk_pkt);
    }

    //rlapp_dbg("call rise_railPkt_free stk_pkt=%X \n", (uint32)stk_pkt);
    //rise_railPkt_free(stk_pkt);


    rlapp_dbg("toMaster TX-Fihish ---\n");
    return RT_ERR_OK;
}

int32
rlapp_stack_msg_testCase_syncAckDelay(uint8 myBoxID, uint32 delay_time)
{
    rise_boxProperty_t  boxProperty;
    uint32 send_box_id, send_cnt, is_sync;
    uint32  loop_cnt;
    rlapp_stk_msgInfo_t rlapp_msg_info;

    send_cnt = 0;
    loop_cnt = 1;
    do {
        for (send_box_id = 1 ; send_box_id <= 16; send_box_id++)
        {
            if (send_box_id == myBoxID)
                continue;
            if (rise_boxProperty_get(send_box_id, &boxProperty) != RT_ERR_OK)
                continue;

            rlapp_info("I am Master. Send RPC to box %u. unit %u ---------------------------------------\n", send_box_id, boxProperty.chip[0].global_unit_id);
            send_cnt++;
            is_sync = TRUE;

            osal_memset(&rlapp_msg_info, 0, sizeof(rlapp_stk_msgInfo_t));

            if (send_box_id % 2 == 0)
            {
                rlapp_msg_info.sync_ack_delay = delay_time; /* ms */
            }
            else
            {
                rlapp_msg_info.sync_ack_delay = 0;
            }

            rlapp_stack_msg_send(send_box_id, &boxProperty, &rlapp_msg_info, is_sync);
            msg_send_cnt++;

            rlapp_info("             Send RPC to box %u. unit %u --------------------------------------- Done\n", send_box_id, boxProperty.chip[0].global_unit_id);
        }
        loop_cnt--;
    } while(loop_cnt > 0);

    if (send_cnt == 0)
    {
        rlapp_info("I am Master. No box found\n");
    }
    return RT_ERR_OK;
}

int32
rlapp_stack_msg_testCase_syncAckDelayReturnInTime(uint8 myBoxID)
{
    rlapp_dbg("TEST:sync message retry and got replied\n");
    return rlapp_stack_msg_testCase_syncAckDelay(myBoxID, 1000/* ms*/);
}


int32
rlapp_stack_msg_testCase_syncAckDelayOutOfTime(uint8 myBoxID)
{
    rlapp_dbg("TEST:sync message retry but timeout then got replied\n");
    return rlapp_stack_msg_testCase_syncAckDelay(myBoxID, 3000/* ms*/);
}



int32
rlapp_stack_msg_testCase_sync(uint8 myBoxID)
{
    uint32              cpu_arr_idx;
    rise_boxProperty_t  boxProperty;
    uint32 send_box_id, send_cnt, is_sync;
    uint32  loop_cnt;
    rlapp_stk_msgInfo_t rlapp_msg_info;

    send_cnt = 0;
    loop_cnt = 1;
    do {
        for (send_box_id = 1 ; send_box_id <= 16; send_box_id++)
        {
            if (send_box_id == myBoxID)
                continue;
            if (rise_boxProperty_get(send_box_id, &boxProperty) != RT_ERR_OK)
                continue;

            rlapp_info("I am Master. Send RPC to box %u. unit %u\n", send_box_id, boxProperty.chip[0].global_unit_id);
            cpu_arr_idx = send_box_id - 2;
            send_cnt++;
            is_sync = TRUE;

            osal_memset(&rlapp_msg_info, 0, sizeof(rlapp_stk_msgInfo_t));

            rlapp_stack_msg_send(send_box_id, &boxProperty, &rlapp_msg_info, is_sync);

        }
        loop_cnt--;
    } while(loop_cnt > 0);

    if (send_cnt == 0)
    {
        rlapp_info("I am Master. No box found\n");
    }
    return RT_ERR_OK;
}

int32
rlapp_stack_msg_testCase_syncAsyncLoop10(uint8 myBoxID)
{
    uint32              cpu_arr_idx;
    rise_boxProperty_t  boxProperty;
    uint32 send_box_id, send_cnt, is_sync;
    uint32  loop_cnt;
    rlapp_stk_msgInfo_t rlapp_msg_info;

    send_cnt = 0;
    loop_cnt = 10;
    do {
        for (send_box_id = 1 ; send_box_id <= 16; send_box_id++)
        {
            if (send_box_id == myBoxID)
                continue;
            if (rise_boxProperty_get(send_box_id, &boxProperty) != RT_ERR_OK)
                continue;

            rlapp_info("I am Master. Send RPC to box %u. unit %u\n", send_box_id, boxProperty.chip[0].global_unit_id);
            cpu_arr_idx = send_box_id - 2;
            send_cnt++;

            if (send_box_id % 2 == 0)
            {
                is_sync = FALSE;
            }
            else
            {
                is_sync = TRUE;
            }

            osal_memset(&rlapp_msg_info, 0, sizeof(rlapp_stk_msgInfo_t));

            rlapp_stack_msg_send(send_box_id, &boxProperty, &rlapp_msg_info, is_sync);

        }
        loop_cnt--;
    } while(loop_cnt > 0);

    if (send_cnt == 0)
    {
        rlapp_info("I am Master. No box found\n");
    }
    return RT_ERR_OK;
}


int32
rlapp_stack_msg_testCase_changePri(uint8 myBoxID)
{
    uint32              cpu_arr_idx;
    rise_boxProperty_t  boxProperty;
    uint32 send_box_id, send_cnt, is_sync;
    uint32  loop_cnt;
    rlapp_stk_msgInfo_t rlapp_msg_info;

    send_cnt = 0;
    loop_cnt = 1;
    do {
        for (send_box_id = 1 ; send_box_id <= 16; send_box_id++)
        {
            if (send_box_id == myBoxID)
                continue;
            if (rise_boxProperty_get(send_box_id, &boxProperty) != RT_ERR_OK)
                continue;

            rlapp_info("I am Master. Send RPC to box %u. unit %u\n", send_box_id, boxProperty.chip[0].global_unit_id);
            cpu_arr_idx = send_box_id - 2;
            send_cnt++;
            is_sync = TRUE;

            osal_memset(&rlapp_msg_info, 0, sizeof(rlapp_stk_msgInfo_t));

            rlapp_rpc_pri = (rlapp_rpc_pri + 1) % RISE_MAX_PRI_QUEUE;
            rlapp_stack_msg_send(send_box_id, &boxProperty, &rlapp_msg_info, is_sync);

        }
        loop_cnt--;
    } while(loop_cnt > 0);

    if (send_cnt == 0)
    {
        rlapp_info("I am Master. No box found\n");
    }
    return RT_ERR_OK;
}

int32
rlapp_stack_msg_send_test(void)
{
    int32   ret;
    uint8               myBoxID = 0;
    rise_boxRole_t      box_role;

    if (send_msg_test_enable == 0)
    {
        return RT_ERR_OK;
    }

    if (rise_myBoxID_get(&myBoxID) != RT_ERR_OK)
    {
        rlapp_info("unable to get my box ID.\n");
    }
    else
    {
        rlapp_info("my box ID is %u.\n", myBoxID);
    }

    if ((ret = rise_boxRole_get(&box_role)) != RT_ERR_OK)
    {
        rlapp_err("rise_boxRole_get fail! ret=%X\n", ret);
        return ret;
    }

    if (box_role == RISE_BOXROLE_SLAVE)
    {
        rlapp_info("I am Slave. no RPC.\n");
        return RT_ERR_OK;
    }

#if 0
    if (msg_send_cnt >= 1)
    {
        return RT_ERR_OK;
    }
#endif

    rlapp_stack_msg_testCase_sync(myBoxID);
    //rlapp_stack_msg_testCase_syncAsyncLoop10(myBoxID);
    //rlapp_stack_msg_testCase_syncAckDelayReturnInTime(myBoxID);
    //rlapp_stack_msg_testCase_syncAckDelayOutOfTime(myBoxID);
    //rlapp_stack_msg_testCase_changePri(myBoxID);

    return RT_ERR_OK;
}
#endif

char rlapp_stk_msg_data_reply[]= {'W','W','X','X','Y','Y','Z','Z'};
int rlapp_stk_msg_data_reply_len = sizeof(rlapp_stk_msg_data_reply);

char    traffic_type_ucast[] = "UCAST";
char    traffic_type_bcast[] = "BCAST";
char    traffic_type_onehop[] = "ONE_HOP";
char    traffic_type_unknown[] = "UNKNOWN";

rise_rxRetCode_t
rlapp_stack_msg_rx(rise_railRx_t *msg, rise_pkt_t **pRetPkt, void *pUserData)
{
    uint8               myBoxID = 0;
    //rise_pkt_t     *pStkPkt = (rise_pkt_t *)msg->pNicPkt->data;
    rise_pkt_t     *pStkPkt = msg->pStkPkt;
    drv_nic_pkt_t       *pNicPkt = msg->pStkPkt->pDrvPkt;
    rlapp_stk_msgInfo_t *pAppMsg;
    char        *traffic;

    rlappStkInfo.cnt_rx_msg++;

    pAppMsg = (rlapp_stk_msgInfo_t *)pStkPkt->tlv->value;

    if (pStkPkt->flag & RTSTK_PKT_FLAG_UNICAST)
        traffic = traffic_type_ucast;
    else if (pStkPkt->flag & RTSTK_PKT_FLAG_BYHOP)
        traffic = traffic_type_onehop;
    else if (pStkPkt->flag & RTSTK_PKT_FLAG_BROADCAST)
        traffic = traffic_type_bcast;
    else
        traffic = traffic_type_unknown;

    rlapp_info("RX STK unit %u port %u nickPkt=%X flag=%X Traffic=%s.%s TLV: cnt=%u size=%u type=%X len=%u magic=%X AppData=%X delay=%u\n",
            (uint32)msg->stPort.unit, (uint32)msg->stPort.port, (uint32)pNicPkt,
            pStkPkt->flag, traffic, ((pStkPkt->flag & RTSTK_PKT_FLAG_RAIL_SYNC)?"SYNC":"ASYNC"),
            pStkPkt->tlv_cnt, pStkPkt->tlv_size, pStkPkt->tlv->type, pStkPkt->tlv->len_of_val,
            pAppMsg->magic, pAppMsg->appData, pAppMsg->sync_ack_delay);

    if (pAppMsg->sync_ack_delay != 0)
    {
        osal_time_mdelay(pAppMsg->sync_ack_delay);
    }

    if (rise_myBoxID_get(&myBoxID) != RT_ERR_OK)
    {
        rlapp_info("unable to get my box ID.\n");
    }

    rlapp_info("Set reply info\n");
    pAppMsg->magic          = RLAPP_MAGIC_ACK;
    pAppMsg->cnt            = rlappStkInfo.cnt_rx_msg;
    pAppMsg->ret_box_id     = myBoxID;
    pAppMsg->ret_local_unit = msg->stPort.unit;

//    if (pStkPkt->tlv->type != RLAPP_STK_TLV_TYPE)
//        return RISE_RX_MSG_NOT_HANDLED;

    *pRetPkt = msg->pStkPkt;

    return RISE_RX_MSG_HANDLED;
}

//#include <hal/common/halctrl.h>
//extern void rtstk_rail_debug_dump(void);//DEBUG
void
rlapp_stack_wait_rise_ready(void)
{
    int32   ret;
    rise_boxRole_t      box_role;
    rise_railRxCbInfo_t     rxCbInfo;
    rise_boxProperty_t      bp;

#if 0
//    rlapp_info("sleep 10sec (No Wait RISE Ready)\n");
//    osal_time_mdelay(60*1000);
#else
    {
        uint32  boxID_list;
        uint32  box_id;
        rlapp_info("Wait RISE Ready............................................\n");
        ret = rise_application_kickoff(&boxID_list);

//rlapp_info("DEBUG DUMP:\n");
//rtstk_rail_debug_dump();//DEBUG

        rlapp_info("Got RISE Ready! ret=0x%X boxIdList=0x%X............................\n", ret, boxID_list);
        for (box_id = 1; box_id <= RISE_MAX_BOX; box_id++)
        {
            if (boxID_list & (0x1 << box_id))
            {
                rlappStkInfo.box_present[RISE_BOX_IDX(box_id)] = TRUE;
                rlapp_info("box %u is in the stack!\n", box_id);
                osal_memset(&bp, 0, sizeof(rise_boxProperty_t));
                rlapp_info("chip_id=%d g_unit_id=%u devId=%u\n", bp.chip[0].chip_id, bp.chip[0].global_unit_id, bp.chip[0].assigedDevID);
            }
        }

    }
#endif

    if ((ret = rise_boxRole_get(&box_role)) != RT_ERR_OK)
    {
        rlapp_err("rise_boxRole_get fail! ret=%X\n", ret);
        return;
    }

#if 0
    {
        int i;
        for (i = 0; i < RTK_MAX_NUM_OF_UNIT; i++)
        {
            rlapp_info("u%d=dev%u\n", i, HAL_UNIT_TO_DEV_ID(i));
        }
    }
#endif

    if (box_role == RISE_BOXROLE_SLAVE)
    {
        rlapp_info("I am Slave.\n");
    }
    else if (box_role == RISE_BOXROLE_MASTER)
    {
        rlapp_info("I am Master.\n");
    }

    rlapp_info("Regster RPC RX callback.\n");

    osal_memset(&rxCbInfo, 0, sizeof(rise_railRxCbInfo_t));
    rxCbInfo.rxCb_id = 5;
    rxCbInfo.rxCb_func = rlapp_stack_msg_rx;
    rxCbInfo.pUserData = NULL;
    rise_railRxCallback_register(&rxCbInfo);

}


#if 0
void
_rail_test_dump(void)
{
    rtstk_util_listNode_t        *pNode;
    rise_railRxCbInfo_t   *pRxCbInfo;

        osal_printf("pStkRailRxCbRoot dump\n");
        for (pNode = pStkRailRxCbRoot->head; pNode != NULL; pNode = pNode->next)
        {
            if ((pRxCbInfo = pNode->p) == NULL)
            {
                osal_printf("pRxCbInfo is NULL\n");
            }
            osal_printf("(%X)id=%u\n", (uint32)pRxCbInfo, pRxCbInfo->rxCb_id);
        }
}

void
_rail_test(void)
{
    rise_railRxCbInfo_t   rxCbInfo;

    rxCbInfo.rxCb_id = 5;
    rxCbInfo.rxCb_func = NULL;
    rxCbInfo.pUserData = NULL;
    rise_railRxCallback_register(&rxCbInfo);

    rxCbInfo.rxCb_id = 9;
    rxCbInfo.rxCb_func = NULL;
    rxCbInfo.pUserData = NULL;
    rise_railRxCallback_register(&rxCbInfo);

    rxCbInfo.rxCb_id = 7;
    rxCbInfo.rxCb_func = NULL;
    rxCbInfo.pUserData = NULL;
    rise_railRxCallback_register(&rxCbInfo);

    rxCbInfo.rxCb_id = 3;
    rxCbInfo.rxCb_func = NULL;
    rxCbInfo.pUserData = NULL;
    rise_railRxCallback_register(&rxCbInfo);

    rxCbInfo.rxCb_id = 9;
    rxCbInfo.rxCb_func = NULL;
    rxCbInfo.pUserData = NULL;
    rise_railRxCallback_register(&rxCbInfo);

    _rail_test_dump();

    osal_printf("................................del 9\n");
    rise_railRxCallback_unregister(9);
    _rail_test_dump();

    osal_printf("................................del 9\n");
    rise_railRxCallback_unregister(9);
    _rail_test_dump();

    osal_printf("................................del 7\n");
    rise_railRxCallback_unregister(7);
    _rail_test_dump();

    osal_printf("................................del 5\n");
    rise_railRxCallback_unregister(5);
    _rail_test_dump();

    osal_printf("................................del 3\n");
    rise_railRxCallback_unregister(3);
    _rail_test_dump();


}
#endif

int32
rlapp_portLinkChange_callback(int32 unit, rtk_port_t port, rtk_port_linkStatus_t new_linkStatus)
{
    rlapp_info("U%u P%u new_link=%d LinkChange_%s \n", unit, port, new_linkStatus, (new_linkStatus==PORT_LINKDOWN)?"DOWN":"UP");
    return RT_ERR_OK;
}

void
rlapp_linkChange_register(void)
{
    rtk_port_linkMon_register(rlapp_portLinkChange_callback);
}


void
rlapp_stack_continue_send(void)
{
    rise_boxRole_t      box_role;
uint32  test_cnt = 0;

    while(1)
    {
        osal_time_usleep(1000*1000);
        if ((rise_boxRole_get(&box_role) != RT_ERR_OK))
        {
            osal_time_usleep(10*1000);
            continue;
        }

        if ((box_role == RISE_BOXROLE_SLAVE))
        {
#if 1 //Slave Send Message test
            rlapp_stack_msg_sendToMaster(0/*delay_time*/, FALSE/*is_sync*/);

            osal_time_usleep(10*1000);
            rlapp_stack_msg_sendToMaster(0/*delay_time*/, TRUE/*is_sync*/);
            test_cnt++;
            if (test_cnt >= 5)
                break;
#else
            osal_time_usleep(10*1000);
            continue;
#endif
        }
        else
        {
#if 1 //Master Send Message test
            uint32  target_box = 2;

            if (rlappStkInfo.box_present[RISE_BOX_IDX(target_box)] == FALSE)
            {
                osal_time_usleep(10*1000);
                continue;
            }

    test_cnt++;

            rlapp_stack_msg_bcast();

            osal_time_usleep(1000*1000);
            rlapp_stack_msg_oneHop(0 /* unit */, 24/* port */);

            osal_time_usleep(1000*1000);
            rlapp_stack_msg_testCase_forceSend(target_box /*dstBoxID*/, 1/*cpu_unit*/, 0/*delay_time*/, TRUE/*is_sync*/);

            osal_time_usleep(1000*1000);
            rlapp_stack_msg_testCase_forceSend(target_box /*dstBoxID*/, 2/*cpu_unit*/, 0/*delay_time*/, FALSE/*is_sync*/);

    if (test_cnt >= 10)
        break;
#endif
        }
    }
}

void
rlapp_stack_continue_send2(void)
{
    uint32  target_box = 2;
    rise_boxRole_t      box_role;

    while(1)
    {
        osal_time_usleep(100*1000);
        if ((rise_boxRole_get(&box_role) != RT_ERR_OK) || (box_role == RISE_BOXROLE_SLAVE))
        {
            osal_time_usleep(10*1000);
            continue;
        }

        if (rlappStkInfo.box_present[RISE_BOX_IDX(target_box)] == FALSE)
        {
            osal_time_usleep(10*1000);
            continue;
        }
        //osal_printf("!");
        rlapp_stack_msg_testCase_forceSend(target_box /*dstBoxID*/, 1/*cpu_unit*/, 500/*delay_time*/, TRUE/*is_sync*/);
    }

}


/* Function Name:
 *      rlapp_stack_init
 * Description:
 *      Initial Real-APP stack module
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
rlapp_stack_init(void)
{

    rlapp_dbg("++++++++++++++++\n");

    osal_memset(&rlappStkInfo, 0, sizeof(rlapp_stack_info_t));
    rlapp_stack_hotswap_registration();
    rlapp_linkChange_register();


    rlapp_dbg(">>>>>>>>>>>>>>>>>>>osal_atomic_t size=%u\n", sizeof(osal_atomic_t));

    rlapp_dbg("----------------\n");

    return RT_ERR_OK;
}

