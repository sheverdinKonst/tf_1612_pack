/*
 * Copyright (C) 2009-2019 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * $Revision: 94857 $
 * $Date: 2019-01-16 13:46:55 +0800 (Wed, 16 Jan 2019) $
 *
 * Purpose : Definition for DPA
 *
 * Feature : The file includes the following modules and sub-modules
 *
 */

#include "rtk_dpa.h"
#include "rtk_gtp_fpga.h"
#include "rtk/acl.h"
#include "rtk/port.h"
#include "rtk/vlan.h"
#include "rtk/pie.h"
#include "dal/rtrpc/rtrpc_acl.h"
#include "dal/rtrpc/rtrpc_port.h"
#include "dal/rtrpc/rtrpc_pie.h"
#include "dal/rtrpc/rtrpc_vlan.h"
#ifdef RTK_DPA_CHT_DISABLE_TABLE_MOVE
#include "cht_sdn_resource.h"
#endif

//#define USING_RLT9311_DEMO_BOARD   1;
#define USING_RLT9313_DEMO_BOARD   1;

uint8       g_dpaModule_initial_status = RTK_DPA_MODULE_UNINITIALIZED;
uint32      g_teidLearn_table_activeEntry = 0;
uint32      g_ipFwd_table_activeEntry = 0;
uint32      g_appList_table_activeEntry = 0;
uint32      g_l2Fwd_table_activeEntry = 0;
uint32      g_dpa_meter_activeEntry = 0;
uint32      g_dpa_port_activeMask_low = 0;
uint32      g_dpa_port_activeMask_high = 0;

rtk_dpa_meter_t g_dpa_meter[DPA_CAPAC_METER_MAX_NUM];

/* This is a porting layer, the g_dpa_portList setting is depended on real device. */
/* The following example is used on RTL9311 Demo board */
/* Physical Port */
#ifdef USING_RLT9311_DEMO_BOARD
uint8       g_dpa_portList[] =  {  0, 1,  2,  3,  4,  5,  6 ,  7,  8,  9,
                                10, 11, 12, 13, 14, 15, 16 , 17, 18, 19,
                                20, 21, 22, 23, 24, 25, 26 , 27, 28, 29,
                                30, 31, 32, 33, 34, 35, 36 , 37, 38, 39,
                                40, 41, 42, 43, 44, 45, 46 , 47, 48, 50, 56};
#endif
/* This is a porting layer, the g_dpa_portList setting is depended on real device. */
/* The following example is used on RTL9313 Demo board */
/* Physical Port */
#ifdef USING_RLT9313_DEMO_BOARD
uint8       g_dpa_portList[] =  {  0, 8,  16,  24,  32,  40,  48 ,  50,  52,  53,
                                54, 55, 56};
#endif

int
_mac_to_uint64(rtk_mac_t *pMac, uint64_t *pValue)
{
    uint64_t        convert_value = 0, convert_value1;
    int             convert_idx;
    *pValue = 0;

    for(convert_idx = (ETHER_ADDR_LEN-1); convert_idx >= 0; convert_idx--)
    {
        convert_value1 = (uint64_t)(pMac->octet[convert_idx]);
        convert_value = (uint64_t)((convert_value * 256) + (convert_value1));
    }
    *pValue = convert_value;
    return RT_ERR_OK;
}

int
_tableName_to_rtkFlowPhase(rtk_dpa_tableName_t table, uint8_t *flowPhase)
{
    switch(table)
    {
        case RTK_DPA_TABLE_NANE_TEID_LEARN:
            *flowPhase = 0;
        break;
        case RTK_DPA_TABLE_NANE_IP_FWD:
            *flowPhase = 2;
        break;
        case RTK_DPA_TABLE_NANE_APP_LIST:
            *flowPhase = 3;
        break;
        case RTK_DPA_TABLE_NANE_L2_FWD:
            *flowPhase = 0;
        break;
        default:
            return RT_ERR_FAILED;
        break;
    }
    return RT_ERR_OK;
}

int
_rtkMask_count_outputPort_num(rtk_portmask_t *pPortMask, int *num_of_port)
{
    int num = 0;
    int scan_index;

    for(scan_index = 0; scan_index < RTK_MAX_PORT_PER_UNIT; scan_index++)
    {
        if(scan_index < (RTK_MAX_PORT_PER_UNIT/2))
        {
            if(((pPortMask->bits[0] >> scan_index) & 0x1) != 0)
                num++;
        }else{
            if((pPortMask->bits[1] >> (scan_index-(RTK_MAX_PORT_PER_UNIT/2)) & 0x1) != 0)
                num++;
        }
    }

    *num_of_port = num;
    return RT_ERR_OK;
}

int
_rtkMask_fill_instOutputPort(rtk_portmask_t *pMask, sdn_db_action_t *pOutAct, unsigned int *action_idx)
{
    int port_num = 0;
    int scan_index;
    int has_outputPort = 0;

    _rtkMask_count_outputPort_num(pMask, &port_num);

    for(scan_index = 0; scan_index < RTK_MAX_PORT_PER_UNIT; scan_index++)
    {
        if(scan_index < (RTK_MAX_PORT_PER_UNIT/2))
        {
            if(((pMask->bits[0] >> scan_index) & 0x1) != 0)
            {
                pOutAct[*action_idx].act_type = EN_OFPAT_OUTPUT;
                pOutAct[*action_idx].value.high = 0;
                pOutAct[*action_idx].value.low = RTK_DPA_PHY_TO_SDN_PORT(scan_index);
                *action_idx += 1;
                port_num--;
                has_outputPort = 1;
            }

        }else{
            if((pMask->bits[1] >> (scan_index-(RTK_MAX_PORT_PER_UNIT/2)) & 0x1) != 0)
            {
                pOutAct[*action_idx].act_type = EN_OFPAT_OUTPUT;
                pOutAct[*action_idx].value.high = 0;
                pOutAct[*action_idx].value.low = RTK_DPA_PHY_TO_SDN_PORT(scan_index);
                *action_idx += 1;
                port_num--;
                has_outputPort = 1;
            }
        }
        if(port_num == 0)
        {
            if(has_outputPort == 1)
                *action_idx -= 1;
            return RT_ERR_OK;
        }
    }

    if(has_outputPort == 1)
        *action_idx -= 1;
    return RT_ERR_OK;
}


int
_count_outputPort_num(rtk_dpa_portmask_t *pPortMask, int *num_of_port)
{
    int num = 0;
    int scan_index;

    for(scan_index = 0; scan_index < RTK_MAX_PORT_PER_UNIT; scan_index++)
    {
        if(scan_index < (RTK_MAX_PORT_PER_UNIT/2))
        {
            if(((pPortMask->portmask.bits[0] >> scan_index) & 0x1) != 0)
                num++;
        }else{
            if((pPortMask->portmask.bits[1] >> (scan_index-(RTK_MAX_PORT_PER_UNIT/2)) & 0x1) != 0)
                num++;
        }
    }
    *num_of_port = num;
    return RT_ERR_OK;
}

int
_fill_instOutputPort(rtk_dpa_portmask_t *pMask, sdn_db_action_t *pOutAct, unsigned int *action_idx)
{
    int port_num = 0;
    int scan_index;
    int has_outputPort = 0;

    _count_outputPort_num(pMask, &port_num);

    for(scan_index = 0; scan_index < RTK_MAX_PORT_PER_UNIT; scan_index++)
    {
        if(scan_index < (RTK_MAX_PORT_PER_UNIT/2))
        {
            if(((pMask->portmask.bits[0] >> scan_index) & 0x1) != 0)
            {
                pOutAct[*action_idx].act_type = EN_OFPAT_OUTPUT;
                pOutAct[*action_idx].value.high = 0;
                pOutAct[*action_idx].value.low = RTK_DPA_PHY_TO_SDN_PORT(scan_index);
                *action_idx += 1;
                port_num--;
                has_outputPort = 1;
            }

        }else{
            if((pMask->portmask.bits[1] >> (scan_index-(RTK_MAX_PORT_PER_UNIT/2)) & 0x1) != 0)
            {
                pOutAct[*action_idx].act_type = EN_OFPAT_OUTPUT;
                pOutAct[*action_idx].value.high = 0;
                pOutAct[*action_idx].value.low = RTK_DPA_PHY_TO_SDN_PORT(scan_index);
                *action_idx += 1;
                port_num--;
                has_outputPort = 1;
            }
        }
        if(port_num == 0)
        {
            if(has_outputPort == 1)
                *action_idx -= 1;
            return RT_ERR_OK;
        }
    }

    if(has_outputPort == 1)
        *action_idx -= 1;

    return RT_ERR_OK;
}

int
_dpa_meter_db_find(rtk_dpa_rateLimit_t *pRate, int32 *pMeterID)
{
    int32 meter_idx;

    for(meter_idx = 1; meter_idx < DPA_CAPAC_METER_MAX_NUM; meter_idx++)
    {
        if((g_dpa_meter[meter_idx].mode == (rtk_dpa_counter_mode_t)pRate->rate_limit_type)&&(g_dpa_meter[meter_idx].rate == pRate->rate))
        {
            *pMeterID = meter_idx;
            return RT_ERR_OK;
        }
    }
    for(meter_idx = 1; meter_idx < DPA_CAPAC_METER_MAX_NUM; meter_idx++)
    {
        if(g_dpa_meter[meter_idx].ref_count == 0)
        {
            *pMeterID = meter_idx;
            g_dpa_meter[meter_idx].mode = (rtk_dpa_counter_mode_t)pRate->rate_limit_type;
            g_dpa_meter[meter_idx].rate = pRate->rate;
            return RT_ERR_OK;
        }
    }
    return RT_ERR_FAILED;
}


int
_dpa_meter_db_add(int32 meterID)
{
    if(g_dpa_meter[meterID].ref_count == 0)
    {
        g_dpa_meter_activeEntry--;
    }
    g_dpa_meter[meterID].ref_count++;
    return RT_ERR_OK;
}

int
_dpa_meter_db_del(int32 meterID)
{
    if(g_dpa_meter[meterID].ref_count > 0)
    {
        g_dpa_meter[meterID].ref_count--;
        if(g_dpa_meter[meterID].ref_count == 0)
            g_dpa_meter_activeEntry++;
    }else{
        DPA_MODULE_MSG("\n[%s][%d] data base error!!!\n",__FUNCTION__,__LINE__);
        return RT_ERR_FAILED;
    }
    return RT_ERR_OK;
}

int
_dpa_meter_configure(int32 meterID, rtk_dpa_rateLimit_t *rate_config)
{
    int                       ret = RT_ERR_OK;
    sdn_db_meter_mod_t        *p_dbMeterMod = NULL;
    sdn_db_meter_band_t       *p_dbMeterBand = NULL;

    p_dbMeterMod = (sdn_db_meter_mod_t *)malloc(sizeof(sdn_db_meter_mod_t));
    if(p_dbMeterMod == NULL)
    {
        ret = RT_ERR_FAILED;
        goto exit;
    }
    memset(p_dbMeterMod, 0, sizeof(sdn_db_meter_mod_t));
    p_dbMeterBand = (sdn_db_meter_band_t *)malloc(sizeof(sdn_db_meter_band_t));
    if(p_dbMeterBand == NULL)
    {
        ret = RT_ERR_FAILED;
        goto exit;
    }
    memset(p_dbMeterBand, 0, sizeof(sdn_db_meter_band_t));
    p_dbMeterMod->meter_id = meterID;
    if(rate_config->rate_limit_type == RTK_DPA_RATELIMIT_TYPE_BYTE)
        p_dbMeterMod->flags = (EN_OFPMF_KBPS | EN_OFPMF_BURST);
    else
        p_dbMeterMod->flags = (EN_OFPMF_PKTPS | EN_OFPMF_BURST);
    p_dbMeterBand->rate = rate_config->rate;
    p_dbMeterBand->burst_size = rate_config->rate;
    p_dbMeterBand->type = EN_OFPMBT_DROP;
    p_dbMeterMod->bands = p_dbMeterBand;
    p_dbMeterMod->n_bands = 1;
    ret = rtk_sdn_add_meter(p_dbMeterMod);

exit:
    if(p_dbMeterMod)
        free(p_dbMeterMod);
    if(p_dbMeterBand)
        free(p_dbMeterBand);

    return ret;
}

int
_dpa_pkeSizeRange_configure(int32 index, uint32 overPktSize)
{
    int     ret = RT_ERR_OK;
    uint32  unit_id = RTK_DPA_UNIT;
    rtk_pie_rangeCheck_t rangeData;

    rangeData.type = RTK_RNGCHK_PKTLEN;
    rangeData.lower_bound = (overPktSize + 1);
    rangeData.upper_bound = RTK_DPA_PKT_SIZE_RANGE_MAX_SIZE;
    ret = rtk_pie_rangeCheck_set(unit_id, index, &rangeData);
    return ret;
}


int
_teid_learn_match_get_src_port(rtk_dpa_teid_learn_match_t *pMatch, int num_of_match, uint8 *src_port)
{
    unsigned int covert_idx;

    for(covert_idx = 0; covert_idx < num_of_match; covert_idx++)
    {
        switch(pMatch[covert_idx].type){
            case RTK_DPA_TEID_LEARN_TYPE_SRC_PORT:
                *src_port = (uint8)pMatch[covert_idx].data.src_port;
                DPA_MODULE_MSG("\n[%s][%d] pMatch[covert_idx].type = %d, *src_port = %d\n",__FUNCTION__,__LINE__,pMatch[covert_idx].type, *src_port);
                return RT_ERR_OK;
                break;
            default:
                break;
        }
    }

    *src_port = TEID_LEARN_NO_SRC_PORT;

    return RT_ERR_OK;
}


int
_teid_learn_rule_match_convert(rtk_dpa_teid_learn_match_t *pMatch, int num_of_match, sdn_db_match_field_t **ppSdn_match)
{
    unsigned int covert_idx;
    sdn_db_match_field_t *p_m;

    /* fill match field */
    p_m = (sdn_db_match_field_t *)malloc(sizeof(sdn_db_match_field_t) * num_of_match);
    if (p_m == NULL) {
        DPA_MODULE_MSG("convert match : failed to allocate memory");
        return RT_ERR_MEM_ALLOC;
    }
    memset(p_m, 0, sizeof(sdn_db_match_field_t) * num_of_match);

    for(covert_idx = 0; covert_idx < num_of_match; covert_idx++)
    {
        switch(pMatch[covert_idx].type){
            case RTK_DPA_TEID_LEARN_TYPE_TEID:
                /* ToDo */
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_RTK_EXP_GTP_TEID;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.teid;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.teid;
                break;
            case RTK_DPA_TEID_LEARN_TYPE_DIP:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_OFB_IPV4_DST;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.dip;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.dip;
                break;
            case RTK_DPA_TEID_LEARN_TYPE_SIP:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_OFB_IPV4_SRC;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.sip;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.sip;
                break;
            case RTK_DPA_TEID_LEARN_TYPE_L4_DP:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_OFB_UDP_DST;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.L4port;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.L4port;
                break;
            case RTK_DPA_TEID_LEARN_TYPE_SRC_PORT:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_OFB_IN_PORT;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.src_port;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.src_port;
                break;
            case RTK_DPA_TEID_LEARN_TYPE_IP_PROT:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_OFB_IP_PROTO;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.ip_prot;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.ip_prot;
                break;
            case RTK_DPA_TEID_LEARN_TYPE_ETH_TYPE:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_OFB_ETH_TYPE;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.eth_type;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.eth_type;
                break;
            default:
                free(p_m);
                return RT_ERR_TYPE;
        }
    }

    *ppSdn_match = p_m;

    return RT_ERR_OK;
}

int
_teid_learn_inst_action_convert(rtk_dpa_teid_learn_action_t *pAct, int num_of_act, uint8 src_port, rtk_dpa_operation_t op, sdn_db_instruction_t **pSdn_inst, int *pInst_num)
{
    unsigned int                    action_idx;
    unsigned int                    inst_idx;
    unsigned int                    dpa_inst_idx;
    int                             inst_num = 0;
    unsigned int                    inst_actionNum;
    int32                           meter_id = 0;
    rtk_dpa_inst_t                  inst_count;
    sdn_db_instruction_t            *p_inst = NULL;
    sdn_db_inst_write_actions_t     *p_instWriteAct = NULL;
    sdn_db_inst_goto_table_t        *p_instGotoAct = NULL;
    sdn_db_inst_meter_t             *p_instMeterAct = NULL;
    sdn_db_inst_write_metadata_t    *p_instMetadata = NULL;
    sdn_db_action_t                 *p_writeAct = NULL;
    uint8_t                          flowPhase_id;
    int                             output_port_num;
    rtk_dpa_portmask_t              dpaPortMask;

    memset(&inst_count, 0, sizeof(rtk_dpa_inst_t));

    /* Parser for
       (1) how many instructure type in these action.
       (2) each instructure owns which actions */
    for(action_idx = 0; action_idx < num_of_act; action_idx++)
    {
        switch(pAct[action_idx].action){
            case RTK_DPA_TEID_LEARN_ACTION_TYPE_OUTPUT:
                if(inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE] == 0){
                    p_instWriteAct = (sdn_db_inst_write_actions_t *)malloc(sizeof(sdn_db_inst_write_actions_t));
                    if(p_instWriteAct == NULL)
                        goto tied_learn_err;
                    inst_count.instNum[RTK_DPA_INST_TYPE_WRITE] = inst_num;
                    inst_num++;
                }
                inst_count.actionPos[RTK_DPA_INST_TYPE_WRITE][(uint8)(inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE])] = action_idx;
                _count_outputPort_num(&pAct[action_idx].data.portmask, &output_port_num);
                inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE] += output_port_num;
                break;
            case RTK_DPA_TEID_LEARN_ACTION_TYPE_DECAP:
                if(inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE] == 0){
                    p_instWriteAct = (sdn_db_inst_write_actions_t *)malloc(sizeof(sdn_db_inst_write_actions_t));
                    if(p_instWriteAct == NULL)
                        goto tied_learn_err;
                    inst_count.instNum[RTK_DPA_INST_TYPE_WRITE] = inst_num;
                    inst_num++;
                }
                inst_count.actionPos[RTK_DPA_INST_TYPE_WRITE][(uint8)(inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE])] = action_idx;
                inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE] += 2; /* Decap includes modify DMAC and output two actions */
            break;
            case RTK_DPA_TEID_LEARN_ACTION_TYPE_GOTO:
                if(inst_count.actionNum[RTK_DPA_INST_TYPE_GOTO] == 0){
                    p_instGotoAct = (sdn_db_inst_goto_table_t *)malloc(sizeof(sdn_db_inst_goto_table_t));
                    if(p_instGotoAct == NULL)
                        goto tied_learn_err;
                    inst_count.instNum[RTK_DPA_INST_TYPE_GOTO] = inst_num;
                    inst_num++;
                }
                inst_count.actionPos[RTK_DPA_INST_TYPE_GOTO][(uint8)(inst_count.actionNum[RTK_DPA_INST_TYPE_GOTO])] = action_idx;
                inst_count.actionNum[RTK_DPA_INST_TYPE_GOTO] += 1;
            break;
            case RTK_DPA_TEID_LEARN_ACTION_TYPE_RATELIMIT:
                if(inst_count.actionNum[RTK_DPA_INST_TYPE_METER] == 0){
                    inst_count.instNum[RTK_DPA_INST_TYPE_METER] = inst_num;
                    inst_num++;
                }
                p_instMeterAct = (sdn_db_inst_meter_t *)malloc(sizeof(sdn_db_inst_meter_t));
                if(p_instMeterAct == NULL)
                    goto tied_learn_err;
                _dpa_meter_db_find(&(pAct[action_idx].data.ratelimit), &meter_id);
                if((op == RTK_DPA_OPERATION_DEL) || (op == RTK_DPA_OPERATION_DEL_DEF))
                {
                    _dpa_meter_db_del(meter_id);
                    rtk_sdn_delete_meter(meter_id);
                }else{
                    _dpa_meter_db_add(meter_id);
                    _dpa_meter_configure(meter_id, &(pAct[action_idx].data.ratelimit));
                }
                p_instMeterAct->meter_id = meter_id;
                inst_count.actionPos[RTK_DPA_INST_TYPE_METER][(uint8)(inst_count.actionNum[RTK_DPA_INST_TYPE_METER])] = action_idx;
                inst_count.actionNum[RTK_DPA_INST_TYPE_METER] += 1;
            break;
            default:
                return RT_ERR_TYPE;
        }
    }

    /* For set metadata */
    if(src_port != TEID_LEARN_NO_SRC_PORT)
    {
        if(inst_count.actionNum[RTK_DPA_INST_TYPE_METADATA] == 0){
            inst_count.instNum[RTK_DPA_INST_TYPE_METADATA] = inst_num;
            inst_count.actionNum[RTK_DPA_INST_TYPE_METADATA]++;
            inst_num++;
        }
    }

    /* Allocation sdn_db_instruction_t */
    p_inst = (sdn_db_instruction_t *)malloc(sizeof(sdn_db_instruction_t) * inst_num);
    if(p_inst == NULL)
        goto tied_learn_err;
    *pInst_num = inst_num;

    /* (1) Fill instruction type
       (2) allocation action array
       (3) add action array to instructre structure */

    for(dpa_inst_idx = 0; dpa_inst_idx < RTK_DPA_INST_TYPE_END; dpa_inst_idx++)
    {
        if(inst_count.actionNum[dpa_inst_idx] == 0)
            continue;
        switch(dpa_inst_idx){
            case RTK_DPA_INST_TYPE_WRITE:
                inst_idx = inst_count.instNum[dpa_inst_idx];
                inst_actionNum = inst_count.actionNum[dpa_inst_idx];
                p_inst[inst_idx].type = EN_OFPIT_WRITE_ACTIONS;
                p_writeAct = (sdn_db_action_t *)malloc(sizeof(sdn_db_action_t)*(inst_actionNum));
                if(p_writeAct == NULL)
                    goto tied_learn_err;
                memset(p_writeAct, 0, (sizeof(sdn_db_action_t)*(inst_actionNum)));
                for(action_idx = 0; action_idx < (inst_actionNum); action_idx++)
                {
                    if(pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].action == RTK_DPA_TEID_LEARN_ACTION_TYPE_OUTPUT)
                    {
                        dpaPortMask.portmask.bits[1] = pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].data.portmask.portmask.bits[1];
                        dpaPortMask.portmask.bits[0] = pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].data.portmask.portmask.bits[0];
                        /* p_writeAct[action_idx]->oxm_type is used when action type is EN_OFPAT_SET_FIELD*/
                        _fill_instOutputPort(&dpaPortMask , p_writeAct, &action_idx);
                    }else if(pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].action == RTK_DPA_TEID_LEARN_ACTION_TYPE_DECAP)
                    {
                        p_writeAct[action_idx].oxm_type = EN_OFPXMT_OFB_ETH_DST;
                        p_writeAct[action_idx].act_type = EN_OFPAT_SET_FIELD;
                        /* Set Decap DMAC over here!!! */
                        p_writeAct[action_idx].value.high = 0;
                        rtk_gtp_fpga_compose_decap_dmac_format(RTK_GTP_DECAP_VP_ID, &(p_writeAct[action_idx].value.low));
                        action_idx++;
                        p_writeAct[action_idx].act_type = EN_OFPAT_OUTPUT;
                        p_writeAct[action_idx].value.high = 0x0;
                        p_writeAct[action_idx].value.low = DPA_FPGA_CONNECT_PORT;
                    }else{
                        goto tied_learn_err;
                    }
                }
                p_inst[inst_idx].data_p = p_instWriteAct;
                p_instWriteAct->len = (inst_actionNum);
                p_instWriteAct->action_set = p_writeAct;
                break;
            case RTK_DPA_INST_TYPE_GOTO:
                inst_idx = inst_count.instNum[dpa_inst_idx];
                p_inst[inst_idx].type = EN_OFPIT_GOTO_TABLE;
                p_inst[inst_idx].data_p = p_instGotoAct;
                _tableName_to_rtkFlowPhase(pAct[inst_count.actionPos[dpa_inst_idx][0]].data.tableName,&flowPhase_id);
                p_instGotoAct->table_id = flowPhase_id;
                DPA_MODULE_MSG("\n[%s][%d] p_instGotoAct->table_id = %d\n",__FUNCTION__,__LINE__,p_instGotoAct->table_id);
                break;
            case RTK_DPA_INST_TYPE_METADATA:
                inst_idx = inst_count.instNum[dpa_inst_idx];
                p_inst[inst_idx].type = EN_OFPIT_WRITE_METADATA;
                p_instMetadata = (sdn_db_inst_write_metadata_t *) malloc(sizeof(sdn_db_inst_write_metadata_t)*(inst_count.actionNum[dpa_inst_idx]));
                if(p_instMetadata == NULL)
                    goto tied_learn_err;
                p_inst[inst_idx].data_p = p_instMetadata;
                p_instMetadata->metadata = src_port;
                p_instMetadata->metadata_mask = 0x3f;
                break;
            case RTK_DPA_INST_TYPE_METER:
                inst_idx = inst_count.instNum[dpa_inst_idx];
                p_inst[inst_idx].type = EN_OFPIT_METER;
                p_inst[inst_idx].data_p = p_instMeterAct;
                break;
            default:
                goto tied_learn_err;
        }
    }

    *pSdn_inst = p_inst;

    return RT_ERR_OK;

tied_learn_err:

    if(p_inst != NULL)
        free(p_inst);
    if(p_instWriteAct != NULL)
        free(p_instWriteAct);
    if(p_instGotoAct != NULL)
        free(p_instGotoAct);
    if(p_writeAct != NULL)
        free(p_writeAct);
    if(p_instMeterAct != NULL)
        free(p_instMeterAct);
    if(p_instMetadata != NULL)
        free(p_instMetadata);

    return RT_ERR_MEM_ALLOC;
}



int
_ip_fwd_rule_match_convert(rtk_dpa_ip_fwd_match_t *pMatch, int num_of_match, sdn_db_match_field_t **ppSdn_match)
{
    unsigned int covert_idx;
    sdn_db_match_field_t *p_m;

    /* fill match field */
    p_m = (sdn_db_match_field_t *)malloc(sizeof(sdn_db_match_field_t) * num_of_match);
    if (p_m == NULL) {
        DPA_MODULE_MSG("convert match : failed to allocate memory");
        return RT_ERR_MEM_ALLOC;
    }
    memset(p_m, 0, sizeof(sdn_db_match_field_t) * num_of_match);

    for(covert_idx = 0; covert_idx < num_of_match; covert_idx++)
    {
        switch(pMatch[covert_idx].type){
            case RTK_DPA_IP_FWD_TYPE_DIP:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_OFB_IPV4_DST;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.dip;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.dip;
                break;
            case RTK_DPA_IP_FWD_TYPE_SRC_PORT:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_OFB_METADATA;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.src_port;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.src_port;
                break;
            default:
                free(p_m);
                return RT_ERR_TYPE;
        }
    }

    *ppSdn_match = p_m;

    return RT_ERR_OK;
}

int
_ip_fwd_inst_action_convert(rtk_dpa_ip_fwd_action_t *pAct, int num_of_act, sdn_db_instruction_t **pSdn_inst, int *pInst_num, rtk_dpa_operation_t op)
{
    unsigned int                action_idx;
    unsigned int                inst_idx;
    unsigned int                dpa_inst_idx;
    int                         inst_num = 0;
    unsigned int                inst_actionNum;
    rtk_dpa_inst_t              inst_count;
    sdn_db_instruction_t        *p_inst = NULL;
    sdn_db_inst_write_actions_t *p_instWriteAct = NULL;
    sdn_db_inst_goto_table_t    *p_instGotoAct = NULL;
    sdn_db_action_t              *p_writeAct = NULL;
    uint64_t                     mac_convert;
    rtk_gtp_fpga_vpdb_entry_sts_t       entry_status;
    int                         vp_idx = 0;
    int                         ret;
    uint8_t                     flowPhase_id;
    int                         output_port_num;
    rtk_portmask_t              portMask;
    unsigned int                vlan_id = 0;
    unsigned int                vlan_tpid = 0;

    memset(&inst_count, 0, sizeof(rtk_dpa_inst_t));

    /* Parser for
       (1) how many instructure type in these action.
       (2) each instructure owns which actions
       (3) inst_count.actionPos[TYPE][] are kept which pAct are related to
           this TYPE.
    */
    for(action_idx = 0; action_idx < num_of_act; action_idx++)
    {
        switch(pAct[action_idx].action){
            case RTK_DPA_IP_FWD_ACTION_TYPE_OUTPUT:
            case RTK_DPA_IP_FWD_ACTION_TYPE_OUTPUT_DIRECT:
                if(inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE] == 0){
                    p_instWriteAct = (sdn_db_inst_write_actions_t *)malloc(sizeof(sdn_db_inst_write_actions_t));
                    if(p_instWriteAct == NULL)
                        goto ip_fwd_err;
                    inst_count.instNum[RTK_DPA_INST_TYPE_WRITE] = inst_num;
                    inst_num++;
                }
                inst_count.actionPos[RTK_DPA_INST_TYPE_WRITE][(uint8)(inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE])] = action_idx;
                if(pAct[action_idx].action == RTK_DPA_IP_FWD_ACTION_TYPE_OUTPUT)
                {
                _rtkMask_count_outputPort_num(&pAct[action_idx].data.output.portmask, &output_port_num);
                }else if(pAct[action_idx].action == RTK_DPA_IP_FWD_ACTION_TYPE_OUTPUT_DIRECT)
                {
                    _rtkMask_count_outputPort_num(&pAct[action_idx].data.output_direct.portmask, &output_port_num);
                }else{
                    DPA_MODULE_MSG("\n[%s][%d] ERROR\n",__FUNCTION__,__LINE__);
                    goto ip_fwd_err;
                }

                DPA_MODULE_MSG("\n[%s][%d] output_port_num = %d\n",__FUNCTION__,__LINE__,output_port_num);
                inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE] += output_port_num;
                if(pAct[action_idx].action == RTK_DPA_IP_FWD_ACTION_TYPE_OUTPUT)
                {
                    /* Output includes modify DMAC, modify SMAC */
                    inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE] += 2;
                }
            break;
            case RTK_DPA_IP_FWD_ACTION_TYPE_ENCAP:
                if(inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE] == 0){
                    p_instWriteAct = (sdn_db_inst_write_actions_t *)malloc(sizeof(sdn_db_inst_write_actions_t));
                    if(p_instWriteAct == NULL)
                        goto ip_fwd_err;
                    inst_count.instNum[RTK_DPA_INST_TYPE_WRITE] = inst_num;
                    inst_num++;
                }
                inst_count.actionPos[RTK_DPA_INST_TYPE_WRITE][(uint8)(inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE])] = action_idx;

                /* Encap includes modify DMAC and output two actions */
                inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE] += 2;
            break;
            case RTK_DPA_IP_FWD_ACTION_TYPE_GOTO:
                if(inst_count.actionNum[RTK_DPA_INST_TYPE_GOTO] == 0){
                    p_instGotoAct = (sdn_db_inst_goto_table_t *)malloc(sizeof(sdn_db_inst_goto_table_t));
                    if(p_instGotoAct == NULL)
                        goto ip_fwd_err;
                    inst_count.instNum[RTK_DPA_INST_TYPE_GOTO] = inst_num;
                    inst_num++;
                }
                inst_count.actionPos[RTK_DPA_INST_TYPE_GOTO][(uint8)(inst_count.actionNum[RTK_DPA_INST_TYPE_GOTO])] = action_idx;
                inst_count.actionNum[RTK_DPA_INST_TYPE_GOTO] += 1;
            break;
            case RTK_DPA_IP_FWD_ACTION_TYPE_VLAN:
                if(inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE] == 0){
                    p_instWriteAct = (sdn_db_inst_write_actions_t *)malloc(sizeof(sdn_db_inst_write_actions_t));
                    if(p_instWriteAct == NULL)
                        goto ip_fwd_err;
                    inst_count.instNum[RTK_DPA_INST_TYPE_WRITE] = inst_num;
                    inst_num++;
                }
                inst_count.actionPos[RTK_DPA_INST_TYPE_WRITE][(uint8)(inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE])] = action_idx;
                /* For Set VLAN ID and PUSH VLAN */
                inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE] += 2;
            break;
            default:
                return RT_ERR_TYPE;
        }
    }

    /* Allocation sdn_db_instruction_t */
    p_inst = (sdn_db_instruction_t *)malloc(sizeof(sdn_db_instruction_t) * inst_num);
    if(p_inst == NULL)
        goto ip_fwd_err;

    *pInst_num = inst_num;

    /* (1) Fill instruction type
       (2) allocation action array
       (3) add action array to instructre structure */

    for(dpa_inst_idx = 0; dpa_inst_idx < RTK_DPA_INST_TYPE_END; dpa_inst_idx++)
    {
        if(inst_count.actionNum[dpa_inst_idx] == 0)
            continue;
        switch(dpa_inst_idx){
            case RTK_DPA_INST_TYPE_WRITE:
                inst_idx = inst_count.instNum[dpa_inst_idx];
                inst_actionNum = inst_count.actionNum[dpa_inst_idx];
                p_inst[inst_idx].type = EN_OFPIT_WRITE_ACTIONS;
                p_writeAct = (sdn_db_action_t *)malloc(sizeof(sdn_db_action_t)*(inst_actionNum));
                if(p_writeAct == NULL)
                    goto ip_fwd_err;
                memset(p_writeAct, 0, (sizeof(sdn_db_action_t)*(inst_actionNum)));
                for(action_idx = 0; action_idx < (inst_actionNum); action_idx++)
                {
                    if((pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].action == RTK_DPA_IP_FWD_ACTION_TYPE_OUTPUT)
                      || (pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].action == RTK_DPA_IP_FWD_ACTION_TYPE_OUTPUT_DIRECT))
                    {
                        if(pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].action == RTK_DPA_IP_FWD_ACTION_TYPE_OUTPUT)
                        {
                            p_writeAct[action_idx].oxm_type = EN_OFPXMT_OFB_ETH_DST;
                            p_writeAct[action_idx].act_type = EN_OFPAT_SET_FIELD;
                            p_writeAct[action_idx].value.high = (uint64_t)0;
                            _mac_to_uint64(&(pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].data.output.dmac),&mac_convert);
                            p_writeAct[action_idx].value.low = mac_convert;
                            action_idx++;
                            p_writeAct[action_idx].oxm_type = EN_OFPXMT_OFB_ETH_SRC;
                            p_writeAct[action_idx].act_type = EN_OFPAT_SET_FIELD;
                            p_writeAct[action_idx].value.high = (uint64_t)0;
                            _mac_to_uint64(&(pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].data.output.smac) ,&mac_convert);
                            p_writeAct[action_idx].value.low = mac_convert;
                            action_idx++;
                            portMask.bits[1] = pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].data.output.portmask.bits[1];
                            portMask.bits[0] = pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].data.output.portmask.bits[0];
                            /* p_writeAct[action_idx]->oxm_type is used when action type is EN_OFPAT_SET_FIELD*/
                            _rtkMask_fill_instOutputPort(&portMask , p_writeAct, &action_idx);
                        }else if(pAct[action_idx].action == RTK_DPA_IP_FWD_ACTION_TYPE_OUTPUT_DIRECT)
                        {
                            portMask.bits[1] = pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].data.output_direct.portmask.bits[1];
                            portMask.bits[0] = pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].data.output_direct.portmask.bits[0];
                            /* p_writeAct[action_idx]->oxm_type is used when action type is EN_OFPAT_SET_FIELD*/
                            _rtkMask_fill_instOutputPort(&portMask , p_writeAct, &action_idx);
                        }else{
                            goto ip_fwd_err;
                        }
                    }else if(pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].action == RTK_DPA_IP_FWD_ACTION_TYPE_ENCAP)
                    {
                        p_writeAct[action_idx].oxm_type = EN_OFPXMT_OFB_ETH_DST;
                        p_writeAct[action_idx].act_type = EN_OFPAT_SET_FIELD;
                        /* Set Encap DMAC over here!!! */
                        p_writeAct[action_idx].value.high = 0;
                        if((op == RTK_DPA_OPERATION_ADD)||(op == RTK_DPA_OPERATION_ADD_DEF))
                        {
                            ret = rtk_gtp_fpga_tunnelVP_db_action(&(pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].data.encap), RTK_GTP_FPGA_VPBD_ACTION_ADD, &entry_status,&vp_idx);
                            DPA_MODULE_MSG("\n[%s][%d]vp_idx = %d\n",__FUNCTION__,__LINE__,vp_idx);
                            if(ret != RT_ERR_OK)
                                goto ip_fwd_err;
                            rtk_gtp_fpga_compose_encap_dmac_format(vp_idx, &(p_writeAct[action_idx].value.low));
                         }else if((op == RTK_DPA_OPERATION_DEL) || (op == RTK_DPA_OPERATION_DEL_DEF)){
                             ret = rtk_gtp_fpga_tunnelVP_db_action(&(pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].data.encap),RTK_GTP_FPGA_VPBD_ACTION_DEL, &entry_status,&vp_idx);
                         }else
                             goto ip_fwd_err;
                        action_idx++;
                        p_writeAct[action_idx].act_type = EN_OFPAT_OUTPUT;
                        p_writeAct[action_idx].value.high = 0x0;
                        p_writeAct[action_idx].value.low = DPA_FPGA_CONNECT_PORT;
                    }else if(pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].action == RTK_DPA_IP_FWD_ACTION_TYPE_VLAN)
                    {
                        DPA_MODULE_MSG("\n[%s][%d] EN_OFPAT_PUSH_VLAN\n",__FUNCTION__,__LINE__);
                        vlan_id = pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].data.vlanTag.vlan_ID;
                        vlan_tpid = pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].data.vlanTag.tpid;
                        p_writeAct[action_idx].oxm_type = EN_OFPXMT_OFB_VLAN_VID;
                        p_writeAct[action_idx].act_type = EN_OFPAT_SET_FIELD;
                        p_writeAct[action_idx].value.high = 0;
                        p_writeAct[action_idx].value.low = vlan_id;

                        action_idx++;
                        p_writeAct[action_idx].oxm_type = EN_OFPXMT_OFB_VLAN_VID;
                        p_writeAct[action_idx].act_type = EN_OFPAT_PUSH_VLAN;
                        p_writeAct[action_idx].value.high = 0;
                        p_writeAct[action_idx].value.low = vlan_tpid;
                    }else{
                        goto ip_fwd_err;
                    }
                }
                p_inst[inst_idx].data_p = p_instWriteAct;
                p_instWriteAct->len = (inst_actionNum);
                p_instWriteAct->action_set = p_writeAct;
                break;
            case RTK_DPA_INST_TYPE_GOTO:
                inst_idx = inst_count.instNum[dpa_inst_idx];
                p_inst[inst_idx].type = EN_OFPIT_GOTO_TABLE;
                p_inst[inst_idx].data_p = p_instGotoAct;
                _tableName_to_rtkFlowPhase(pAct[inst_count.actionPos[dpa_inst_idx][0]].data.tableName,&flowPhase_id);
                p_instGotoAct->table_id = flowPhase_id;
                break;
            default:
                goto ip_fwd_err;
        }
    }

    *pSdn_inst = p_inst;

    return RT_ERR_OK;

ip_fwd_err:

    if(p_inst != NULL)
        free(p_inst);
    if(p_instWriteAct != NULL)
        free(p_instWriteAct);
    if(p_writeAct != NULL)
        free(p_writeAct);
    if(p_instGotoAct != NULL)
        free(p_instGotoAct);

    return RT_ERR_MEM_ALLOC;
}

int
_app_list_rule_match_convert(rtk_dpa_app_list_match_t *pMatch, int num_of_match, sdn_db_match_field_t **ppSdn_match)
{
    unsigned int covert_idx;
    sdn_db_match_field_t *p_m;

    /* fill match field */
    p_m = (sdn_db_match_field_t *)malloc(sizeof(sdn_db_match_field_t) * num_of_match);
    if (p_m == NULL) {
        DPA_MODULE_MSG("convert match : failed to allocate memory");
        return RT_ERR_MEM_ALLOC;
    }
    memset(p_m, 0, sizeof(sdn_db_match_field_t) * num_of_match);

    for(covert_idx = 0; covert_idx < num_of_match; covert_idx++)
    {
        switch(pMatch[covert_idx].type){
            case RTK_DPA_APP_LIST_TYPE_IN_SIP:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_RTK_INNER_SIP;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.inner_sip;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.inner_sip;
                break;
            case RTK_DPA_APP_LIST_TYPE_IN_DIP:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_RTK_INNER_DIP;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.inner_dip;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.inner_dip;
                break;
            case RTK_DPA_APP_LIST_TYPE_IN_L4DP:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_RTK_INNER_L4DP;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.inner_L4port;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.inner_L4port;
                break;
            case RTK_DPA_APP_LIST_TYPE_IN_IPVER:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_RTK_INNER_IP_VER;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.inner_ip_ver;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.inner_ip_ver;
                break;
            case RTK_DPA_APP_LIST_TYPE_IP_FLAG:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_RTK_OUTER_IP_FLG;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.ip_flag;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.ip_flag;
                break;
            case RTK_DPA_APP_LIST_TYPE_OUT_IPPROT:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_OFB_IP_PROTO;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.outer_ip_proto;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.outer_ip_proto;
                break;
            case RTK_DPA_APP_LIST_TYPE_OUT_L4DP:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_OFB_UDP_DST;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.outer_L4port;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.outer_L4port;
                break;
            case RTK_DPA_APP_LIST_TYPE_ETH_TYPE:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_OFB_ETH_TYPE;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.eth_type;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.eth_type;
                break;
            case RTK_DPA_APP_LIST_TYPE_IN_IPPROT:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_RTK_INNER_IP_PROTO;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.inner_ip_proto;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.inner_ip_proto;
                break;
            case RTK_DPA_APP_LIST_TYPE_IN_TCP_FLAG:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_RTK_INNER_TCP_FLAG;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.inner_tcp_flag;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.inner_tcp_flag;
                break;
            default:
                free(p_m);
                return RT_ERR_TYPE;
        }
    }

    *ppSdn_match = p_m;

    return RT_ERR_OK;
}

int
_app_list_inst_action_convert(rtk_dpa_app_list_action_t *pAct, int num_of_act, sdn_db_instruction_t **pSdn_inst, int *pInst_num)
{
    unsigned int                 action_idx;
    unsigned int                 inst_idx;
    unsigned int                 dpa_inst_idx;
    int                          inst_num = 0;
    unsigned int                 inst_actionNum;
    rtk_dpa_inst_t               inst_count;
    sdn_db_instruction_t         *p_inst = NULL;
    sdn_db_inst_write_actions_t  *p_instWriteAct = NULL;
    sdn_db_action_t              *p_writeAct = NULL;
    int                          output_port_num;
    rtk_dpa_portmask_t           dpaPortMask;

    memset(&inst_count, 0, sizeof(rtk_dpa_inst_t));

    /* Parser for
       (1) how many instructure type in these action.
       (2) each instructure owns which actions */
    for(action_idx = 0; action_idx < num_of_act; action_idx++)
    {
        switch(pAct[action_idx].action){
            case RTK_DPA_APP_LIST_ACTION_TYPE_DECAP:
                if(inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE] == 0){
                    p_instWriteAct = (sdn_db_inst_write_actions_t *)malloc(sizeof(sdn_db_inst_write_actions_t));
                    if(p_instWriteAct == NULL)
                        goto app_list_err;
                    inst_count.instNum[RTK_DPA_INST_TYPE_WRITE] = inst_num;
                    inst_num++;
                }
                inst_count.actionPos[RTK_DPA_INST_TYPE_WRITE][(uint8)(inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE])] = action_idx;
                /* Decap includes modify DMAC and output two actions */
                inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE] += 2;
            break;
            case RTK_DPA_APP_LIST_ACTION_TYPE_OUTPUT:
                if(inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE] == 0){
                    p_instWriteAct = (sdn_db_inst_write_actions_t *)malloc(sizeof(sdn_db_inst_write_actions_t));
                    if(p_instWriteAct == NULL)
                        goto app_list_err;
                    inst_count.instNum[RTK_DPA_INST_TYPE_WRITE] = inst_num;
                    inst_num++;
                }
                inst_count.actionPos[RTK_DPA_INST_TYPE_WRITE][(uint8)(inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE])] = action_idx;
                _count_outputPort_num(&pAct[action_idx].data.portmask, &output_port_num);
                inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE] += output_port_num;

            break;
            case RTK_DPA_APP_LIST_ACTION_TYPE_POP_VLAN:
                if(inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE] == 0){
                    p_instWriteAct = (sdn_db_inst_write_actions_t *)malloc(sizeof(sdn_db_inst_write_actions_t));
                    if(p_instWriteAct == NULL)
                        goto app_list_err;
                    inst_count.instNum[RTK_DPA_INST_TYPE_WRITE] = inst_num;
                    inst_num++;
                }
                inst_count.actionPos[RTK_DPA_INST_TYPE_WRITE][(uint8)(inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE])] = action_idx;
                /* For POP VLAN */
                inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE] += 1;
            break;
            default:
                return RT_ERR_TYPE;
        }
    }

    /* Allocation sdn_db_instruction_t */
    p_inst = (sdn_db_instruction_t *)malloc(sizeof(sdn_db_instruction_t) * inst_num);
    if(p_inst == NULL)
        goto app_list_err;

    *pInst_num = inst_num;

    /* (1) Fill instruction type
       (2) allocation action array
       (3) add action array to instructre structure */

    for(dpa_inst_idx = 0; dpa_inst_idx < RTK_DPA_INST_TYPE_END; dpa_inst_idx++)
    {
        if(inst_count.actionNum[dpa_inst_idx] == 0)
            continue;
        switch(dpa_inst_idx){
            case RTK_DPA_INST_TYPE_WRITE:
                inst_idx = inst_count.instNum[dpa_inst_idx];
                inst_actionNum = inst_count.actionNum[dpa_inst_idx];
                p_inst[inst_idx].type = EN_OFPIT_WRITE_ACTIONS;
                p_writeAct = (sdn_db_action_t *)malloc(sizeof(sdn_db_action_t)*(inst_actionNum));
                if(p_writeAct == NULL)
                    goto app_list_err;
                memset(p_writeAct, 0, (sizeof(sdn_db_action_t)*(inst_actionNum)));
                for(action_idx = 0; action_idx < (inst_actionNum); action_idx++)
                {
                    if(pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].action == RTK_DPA_APP_LIST_ACTION_TYPE_OUTPUT)
                    {
                        dpaPortMask.portmask.bits[1] = pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].data.portmask.portmask.bits[1];
                        dpaPortMask.portmask.bits[0] = pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].data.portmask.portmask.bits[0];
                        /* p_writeAct[action_idx]->oxm_type is used when action type is EN_OFPAT_SET_FIELD*/
                        _fill_instOutputPort(&dpaPortMask , p_writeAct, &action_idx);
                    }else if(pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].action == RTK_DPA_APP_LIST_ACTION_TYPE_DECAP)
                    {
                        p_writeAct[action_idx].oxm_type = EN_OFPXMT_OFB_ETH_DST;
                        p_writeAct[action_idx].act_type = EN_OFPAT_SET_FIELD;
                        /* Set Decap DMAC over here!!! */
                        p_writeAct[action_idx].value.high = 0;
                        rtk_gtp_fpga_compose_decap_dmac_format(RTK_GTP_DECAP_VP_ID, &(p_writeAct[action_idx].value.low));
                        action_idx++;
                        p_writeAct[action_idx].act_type = EN_OFPAT_OUTPUT;
                        p_writeAct[action_idx].value.high = 0x0;
                        p_writeAct[action_idx].value.low = DPA_FPGA_CONNECT_PORT;
                    }else if(pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].action == RTK_DPA_APP_LIST_ACTION_TYPE_POP_VLAN)
                    {
                        p_writeAct[action_idx].act_type = EN_OFPAT_POP_VLAN;
                        p_writeAct[action_idx].value.high = 0x0;
                        p_writeAct[action_idx].value.low = 0x0;
                    }else{
                        DPA_MODULE_MSG("\n[%s][%d] ERROR!!!\n",__FUNCTION__,__LINE__);
                        goto app_list_err;
                    }
                }
                p_inst[inst_idx].data_p = p_instWriteAct;
                p_instWriteAct->len = (inst_actionNum);
                p_instWriteAct->action_set = p_writeAct;
                break;
            default:
                goto app_list_err;
        }
    }

    *pSdn_inst = p_inst;

    return RT_ERR_OK;

app_list_err:

    if(p_inst != NULL)
        free(p_inst);
    if(p_instWriteAct != NULL)
        free(p_instWriteAct);
    if(p_writeAct != NULL)
        free(p_writeAct);

    return RT_ERR_MEM_ALLOC;
}

int32
_app_list_extend_matchField_setup(void)
{
    int                             ret = RT_ERR_OK;
    rtk_pie_fieldSelector_data_t    fieldSel11_t;
    rtk_pie_fieldSelector_data_t    fieldSel12_t;
    rtk_pie_fieldSelector_data_t    fieldSel2_t;
    rtk_pie_fieldSelector_data_t    fieldSel3_t;
    rtk_pie_fieldSelector_data_t    fieldSel4_t;
    rtk_pie_fieldSelector_data_t    fieldSel5_t;
    rtk_pie_fieldSelector_data_t    fieldSel10_t;
    rtk_pie_fieldSelector_data_t    fieldSel6_t;
    rtk_pie_fieldSelector_data_t    fieldSel1_t;


    /* Configure ACL field seletor for special new add match fields */
    /* Field Seletor 5 for inner IP ver */
    memset(&fieldSel5_t, 0, sizeof(rtk_pie_fieldSelector_data_t));
    fieldSel5_t.start = FS_START_POS_L4;
    fieldSel5_t.payloadSel = PIE_FS_OUTER;
    /* FPFA cannot handle either GTP-u E,S,N flag is 1 case */
    /* Therefore, all GTP-u E,S,N flag is 0, the GTP-u header is 8 bytes */
    fieldSel5_t.offset = 4;  /* GTP-u header 8 bytes (8)/2*/
    ret = rtk_pie_fieldSelector_set(RTK_DPA_UNIT, RTK_DPA_MATCH_INNER_IP_VER_FSID, &fieldSel5_t);
    if(ret != RT_ERR_OK)
          return ret;

    /* Configure ACL field seletor for special new add match fields */
    /* Field Seletor 4 for IP Flag */
    memset(&fieldSel4_t, 0, sizeof(rtk_pie_fieldSelector_data_t));
    fieldSel4_t.start = PIE_FS_START_POS_IP_HDR;
    fieldSel4_t.payloadSel = PIE_FS_OUTER;
    fieldSel4_t.offset = 3;
    ret = rtk_pie_fieldSelector_set(RTK_DPA_UNIT, RTK_DPA_MATCH_IP_FLG_FSID, &fieldSel4_t);
    if(ret != RT_ERR_OK)
          return ret;

    /* Configure ACL field seletor for special new add match fields */
    /* Field Seletor 11 for inner SIP High */
    memset(&fieldSel11_t, 0, sizeof(rtk_pie_fieldSelector_data_t));
    fieldSel11_t.start = FS_START_POS_L4;
    fieldSel11_t.payloadSel = PIE_FS_OUTER;
    /* FPFA cannot handle either GTP-u E,S,N flag is 1 case */
    /* Therefore, all GTP-u E,S,N flag is 0, the GTP-u header is 8 bytes */
    fieldSel11_t.offset = 10;  /* GTP-u header 8 bytes; (8+12)/2*/
    ret = rtk_pie_fieldSelector_set(RTK_DPA_UNIT, RTK_DPA_MATCH_INNER_SIP_HI_FSID, &fieldSel11_t);
    if(ret != RT_ERR_OK)
        return ret;

    /* Configure ACL field seletor for special new add match fields */
    /* Field Seletor 12 for inner SIP Low */
    memset(&fieldSel12_t, 0, sizeof(rtk_pie_fieldSelector_data_t));
    fieldSel12_t.start = FS_START_POS_L4;
    fieldSel12_t.payloadSel = PIE_FS_OUTER;
    /* FPFA cannot handle either GTP-u E,S,N flag is 1 case */
    /* Therefore, all GTP-u E,S,N flag is 0, the GTP-u header is 8 bytes */
    fieldSel12_t.offset = 11;  /* GTP-u header 8 bytes; (8+14)/2*/
    ret = rtk_pie_fieldSelector_set(RTK_DPA_UNIT, RTK_DPA_MATCH_INNER_SIP_LO_FSID, &fieldSel12_t);
    if(ret != RT_ERR_OK)
          return ret;

    /* Configure ACL field seletor for special new add match fields */
    /* Field Seletor 2 for inner DIP High */
    memset(&fieldSel2_t, 0, sizeof(rtk_pie_fieldSelector_data_t));
    fieldSel2_t.start = FS_START_POS_L4;
    fieldSel2_t.payloadSel = PIE_FS_OUTER;
    /* FPFA cannot handle either GTP-u E,S,N flag is 1 case */
    /* Therefore, all GTP-u E,S,N flag is 0, the GTP-u header is 8 bytes */
    fieldSel2_t.offset = 12;  /* GTP-u header 8 bytes; (8+16)/2*/
    ret = rtk_pie_fieldSelector_set(RTK_DPA_UNIT, RTK_DPA_MATCH_INNER_DIP_HI_FSID, &fieldSel2_t);
        if(ret != RT_ERR_OK)
              return ret;

    /* Configure ACL field seletor for special new add match fields */
    /* Field Seletor 3 for inner DIP Low */
    memset(&fieldSel3_t, 0, sizeof(rtk_pie_fieldSelector_data_t));
    fieldSel3_t.start = FS_START_POS_L4;
    fieldSel3_t.payloadSel = PIE_FS_OUTER;
    /* FPFA cannot handle either GTP-u E,S,N flag is 1 case */
    /* Therefore, all GTP-u E,S,N flag is 0, the GTP-u header is 8 bytes */
    fieldSel3_t.offset = 13;  /* GTP-u header 8 bytes; (8+18)/2*/
    ret = rtk_pie_fieldSelector_set(RTK_DPA_UNIT, RTK_DPA_MATCH_INNER_DIP_LO_FSID, &fieldSel3_t);
        if(ret != RT_ERR_OK)
              return ret;

    /* Configure ACL field seletor for special new add match fields */
    /* Field Seletor 10 for inner L4DP ver */
    memset(&fieldSel10_t, 0, sizeof(rtk_pie_fieldSelector_data_t));
    fieldSel10_t.start = FS_START_POS_L4;
    fieldSel10_t.payloadSel = PIE_FS_OUTER;
    /* FPFA cannot handle either GTP-u E,S,N flag is 1 case */
    /* Therefore, all GTP-u E,S,N flag is 0, the GTP-u header is 8 bytes */
    fieldSel10_t.offset = 15;  /* GTP-u header 8 bytes; (8+20+2)/2*/
    ret = rtk_pie_fieldSelector_set(RTK_DPA_UNIT, RTK_DPA_MATCH_INNER_L4DP_FSID, &fieldSel10_t);

    /* Configure ACL field seletor for special new add match fields */
    /* Field Seletor 6 for inner IP Protocol */
    memset(&fieldSel6_t, 0, sizeof(rtk_pie_fieldSelector_data_t));
    fieldSel6_t.start = FS_START_POS_L4;
    fieldSel6_t.payloadSel = PIE_FS_OUTER;
    /* FPFA cannot handle either GTP-u E,S,N flag is 1 case */
    /* Therefore, all GTP-u E,S,N flag is 0, the GTP-u header is 8 bytes */
    fieldSel6_t.offset = 8;  /* GTP-u header 8 bytes; (8+8)/2*//* Get LSB 8 bits*/
    ret = rtk_pie_fieldSelector_set(RTK_DPA_UNIT, RTK_DPA_MATCH_INNER_IP_PROTO_FSID, &fieldSel6_t);

    /* Configure ACL field seletor for special new add match fields */
    /* Field Seletor 1 for inner IP Protocol */
    memset(&fieldSel1_t, 1, sizeof(rtk_pie_fieldSelector_data_t));
    fieldSel1_t.start = FS_START_POS_L4;
    fieldSel1_t.payloadSel = PIE_FS_OUTER;
    /* FPFA cannot handle either GTP-u E,S,N flag is 1 case */
    /* Therefore, all GTP-u E,S,N flag is 0, the GTP-u header is 8 bytes */
    fieldSel1_t.offset = 20;  /* GTP-u header 8 bytes; (8+20+12)/2*//* Get LSB 9 bits*/
    ret = rtk_pie_fieldSelector_set(RTK_DPA_UNIT, RTK_DPA_MATCH_INNER_TCP_FLAG_FSID, &fieldSel1_t);

    return ret;
}


int
_l2_fwd_rule_match_convert(rtk_dpa_l2_fwd_match_t *pMatch, int num_of_match, sdn_db_match_field_t **ppSdn_match)
{
    unsigned int                covert_idx;
    sdn_db_match_field_t        *p_m;
    uint64_t                    mac_convert;
    uint64_t                    mac_mask_convert;

    /* fill match field */
    p_m = (sdn_db_match_field_t *)malloc(sizeof(sdn_db_match_field_t) * num_of_match);
    if (p_m == NULL) {
        DPA_MODULE_MSG("convert match : failed to allocate memory");
        return RT_ERR_MEM_ALLOC;
    }
    memset(p_m, 0, sizeof(sdn_db_match_field_t) * num_of_match);

    for(covert_idx = 0; covert_idx < num_of_match; covert_idx++)
    {
        switch(pMatch[covert_idx].type){
            case RTK_DPA_L2_FWD_TYPE_GTP_MT:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_RTK_EXP_GTP_MT;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.gtp_mt;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.gtp_mt;

                break;
            case RTK_DPA_L2_FWD_TYPE_INNER_IPVER:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_RTK_L2_INNER_IP_VER;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.ip_ver;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.ip_ver;
                break;
            case RTK_DPA_L2_FWD_TYPE_TCP_FLAG:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_RTK_OUTER_TCP_FLG;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.tcp_flag;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.tcp_flag;
                break;
            case RTK_DPA_L2_FWD_TYPE_SRC_PORT:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_OFB_IN_PORT;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.src_port;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.src_port;
                break;
            case RTK_DPA_L2_FWD_TYPE_ETH_TYPE:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_OFB_ETH_TYPE;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.eth_type;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.eth_type;
                break;
            case RTK_DPA_L2_FWD_TYPE_DMAC:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_OFB_ETH_DST;
                _mac_to_uint64(&(pMatch[covert_idx].data.dmac),&mac_convert);
                p_m[covert_idx].value.low = (uint64_t)mac_convert;
                p_m[covert_idx].value.high = (uint64_t)0;
                _mac_to_uint64(&(pMatch[covert_idx].mask.dmac),&mac_mask_convert);
                p_m[covert_idx].mask.low = (uint64_t)mac_mask_convert;
                p_m[covert_idx].mask.high = (uint64_t)0;
                break;
            case RTK_DPA_L2_FWD_TYPE_IPPROT:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_OFB_IP_PROTO;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.ip_proto;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.ip_proto;
                break;
            case RTK_DPA_L2_FWD_TYPE_L4DP:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_OFB_UDP_DST;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.L4port;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.L4port;
                break;
            case RTK_DPA_L2_FWD_TYPE_VLAN_ID:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_OFB_VLAN_VID;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.vlan_id;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.vlan_id;
                break;
            case RTK_DPA_L2_FWD_TYPE_FRAGMENT_PKT:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_RTK_OUTER_FRAG_PKT;
                p_m[covert_idx].value.low = (uint64_t)pMatch[covert_idx].data.fragmented_pkt;
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)pMatch[covert_idx].mask.fragmented_pkt;
                break;
            case RTK_DPA_L2_FWD_TYPE_OVER_PKT_SIZE:
                p_m[covert_idx].oxm_class = EN_OFPXMC_OPENFLOW_BASIC;
                p_m[covert_idx].oxm_type = EN_OFPXMT_RTK_OVER_PKT_SIZE;
                _dpa_pkeSizeRange_configure(RTK_DPA_PKT_SIZE_RANGE_CHK_IDX, pMatch[covert_idx].data.overPktSize);
                /* Range chechk is used the range chkec index as match key */
                p_m[covert_idx].value.low = (uint64_t)(0x1 << RTK_DPA_PKT_SIZE_RANGE_CHK_IDX);
                p_m[covert_idx].value.high = (uint64_t)0;
                p_m[covert_idx].mask.low = (uint64_t)(0x1 << RTK_DPA_PKT_SIZE_RANGE_CHK_IDX);
                break;
            default:
                free(p_m);
                return RT_ERR_TYPE;
        }
    }

    *ppSdn_match = p_m;

    return RT_ERR_OK;
}

int
_l2_fwd_inst_action_convert(rtk_dpa_l2_fwd_action_t *pAct, int num_of_act, sdn_db_instruction_t **pSdn_inst, int *pInst_num,  uint8 src_port)
{
    unsigned int                action_idx;
    unsigned int                inst_idx;
    unsigned int                dpa_inst_idx;
    int                         inst_num = 0;
    unsigned int                inst_actionNum;
    rtk_dpa_inst_t              inst_count;
    sdn_db_instruction_t        *p_inst = NULL;
    sdn_db_inst_write_actions_t *p_instWriteAct = NULL;
    sdn_db_inst_goto_table_t    *p_instGotoAct = NULL;
    sdn_db_inst_write_metadata_t *p_instMetadata = NULL;
    sdn_db_action_t              *p_writeAct = NULL;
    uint8                       flowPhase_id;
    int                         output_port_num;
    rtk_dpa_portmask_t          dpaPortMask;

    memset(&inst_count, 0, sizeof(rtk_dpa_inst_t));

    /* Parser for
       (1) how many instructure type in these action.
       (2) each instructure owns which actions */
    for(action_idx = 0; action_idx < num_of_act; action_idx++)
    {
        switch(pAct[action_idx].action){
            case RTK_DPA_L2_FWD_ACTION_TYPE_OUTPUT:
                if(inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE] == 0){
                    p_instWriteAct = (sdn_db_inst_write_actions_t *)malloc(sizeof(sdn_db_inst_write_actions_t));
                    if(p_instWriteAct == NULL)
                        goto l2_fwd_err;
                    inst_count.instNum[RTK_DPA_INST_TYPE_WRITE] = inst_num;
                    inst_num++;
                }
                inst_count.actionPos[RTK_DPA_INST_TYPE_WRITE][(uint8)(inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE])] = action_idx;
                _count_outputPort_num(&pAct[action_idx].data.portmask, &output_port_num);
                inst_count.actionNum[RTK_DPA_INST_TYPE_WRITE] += output_port_num;
            break;
            case RTK_DPA_L2_FWD_ACTION_TYPE_GOTO:
                if(inst_count.actionNum[RTK_DPA_INST_TYPE_GOTO] == 0){
                    p_instGotoAct = (sdn_db_inst_goto_table_t *)malloc(sizeof(sdn_db_inst_goto_table_t));
                    if(p_instGotoAct == NULL)
                        goto l2_fwd_err;
                    inst_count.instNum[RTK_DPA_INST_TYPE_GOTO] = inst_num;
                    inst_num++;
                }
                inst_count.actionPos[RTK_DPA_INST_TYPE_GOTO][(uint8)(inst_count.actionNum[RTK_DPA_INST_TYPE_GOTO])] = action_idx;
                inst_count.actionNum[RTK_DPA_INST_TYPE_GOTO] += 1;
            break;
            default:
                return RT_ERR_TYPE;
        }
    }

    /* For set metadata */
    if(src_port != L2_FWD_NO_SRC_PORT)
    {
        if(inst_count.actionNum[RTK_DPA_INST_TYPE_METADATA] == 0){
            inst_count.instNum[RTK_DPA_INST_TYPE_METADATA] = inst_num;
            inst_count.actionNum[RTK_DPA_INST_TYPE_METADATA]++;
            inst_num++;
        }
    }


    /* Allocation sdn_db_instruction_t */
    p_inst = (sdn_db_instruction_t *)malloc(sizeof(sdn_db_instruction_t) * inst_num);
    if(p_inst == NULL)
        goto l2_fwd_err;

    *pInst_num = inst_num;

    /* (1) Fill instruction type
       (2) allocation action array
       (3) add action array to instructre structure */

    for(dpa_inst_idx = 0; dpa_inst_idx < RTK_DPA_INST_TYPE_END; dpa_inst_idx++)
    {
        if(inst_count.actionNum[dpa_inst_idx] == 0)
            continue;
        switch(dpa_inst_idx){
            case RTK_DPA_INST_TYPE_WRITE:
                inst_idx = inst_count.instNum[dpa_inst_idx];
                inst_actionNum = inst_count.actionNum[dpa_inst_idx];
                p_inst[inst_idx].type = EN_OFPIT_WRITE_ACTIONS;
                p_writeAct = (sdn_db_action_t *)malloc(sizeof(sdn_db_action_t)*(inst_actionNum));
                if(p_writeAct == NULL)
                    goto l2_fwd_err;
                memset(p_writeAct, 0, (sizeof(sdn_db_action_t)*(inst_actionNum)));
                for(action_idx = 0; action_idx < (inst_actionNum); action_idx++)
                {
                    if(pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].action == RTK_DPA_L2_FWD_ACTION_TYPE_OUTPUT)
                    {
                        /* p_writeAct[action_idx]->oxm_type is used when action type is EN_OFPAT_SET_FIELD*/
                        dpaPortMask.portmask.bits[1] = pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].data.portmask.portmask.bits[1];
                        dpaPortMask.portmask.bits[0] = pAct[inst_count.actionPos[dpa_inst_idx][action_idx]].data.portmask.portmask.bits[0];
                        /* p_writeAct[action_idx]->oxm_type is used when action type is EN_OFPAT_SET_FIELD*/
                        _fill_instOutputPort(&dpaPortMask , p_writeAct, &action_idx);
                    }else{
                        goto l2_fwd_err;
                    }
                }
                p_inst[inst_idx].data_p = p_instWriteAct;
                p_instWriteAct->len = (inst_actionNum);
                p_instWriteAct->action_set = p_writeAct;
                break;
            case RTK_DPA_INST_TYPE_GOTO:
                inst_idx = inst_count.instNum[dpa_inst_idx];
                p_inst[inst_idx].type = EN_OFPIT_GOTO_TABLE;
                p_inst[inst_idx].data_p = p_instGotoAct;
                _tableName_to_rtkFlowPhase(pAct[inst_count.actionPos[dpa_inst_idx][0]].data.tableName,&flowPhase_id);
                p_instGotoAct->table_id = flowPhase_id;
                break;
            case RTK_DPA_INST_TYPE_METADATA:
                inst_idx = inst_count.instNum[dpa_inst_idx];
                p_inst[inst_idx].type = EN_OFPIT_WRITE_METADATA;
                p_instMetadata = (sdn_db_inst_write_metadata_t *) malloc(sizeof(sdn_db_inst_write_metadata_t)*(inst_count.actionNum[dpa_inst_idx]));
                if(p_instMetadata == NULL)
                    goto l2_fwd_err;
                p_inst[inst_idx].data_p = p_instMetadata;
                p_instMetadata->metadata = src_port;
                p_instMetadata->metadata_mask = 0x3f;
                break;
            default:
                goto l2_fwd_err;
        }
    }

    *pSdn_inst = p_inst;

    return RT_ERR_OK;

l2_fwd_err:

    if(p_inst != NULL)
        free(p_inst);
    if(p_instWriteAct != NULL)
        free(p_instWriteAct);
    if(p_writeAct != NULL)
        free(p_writeAct);
    if(p_instGotoAct != NULL)
        free(p_instGotoAct);
    if(p_instMetadata != NULL)
        free(p_instMetadata);

    return RT_ERR_MEM_ALLOC;
}

int
_l2_fwd_match_get_src_port(rtk_dpa_l2_fwd_match_t *pMatch, int num_of_match, uint8 *src_port)
{
    unsigned int covert_idx;

    for(covert_idx = 0; covert_idx < num_of_match; covert_idx++)
    {
        switch(pMatch[covert_idx].type){
            case RTK_DPA_L2_FWD_TYPE_SRC_PORT:
                *src_port = (uint8)pMatch[covert_idx].data.src_port;
                DPA_MODULE_MSG("\n[%s][%d] pMatch[covert_idx].type = %d, *src_port = %d\n",__FUNCTION__,__LINE__,pMatch[covert_idx].type, *src_port);
                return RT_ERR_OK;
                break;
            default:
                break;
        }
    }

    *src_port = L2_FWD_NO_SRC_PORT;

    return RT_ERR_OK;
}



int32
_l2_fwd_extend_matchField_setup(void)
{
    int                             ret = RT_ERR_OK;
    rtk_pie_fieldSelector_data_t    fieldSel8_t;
    rtk_pie_fieldSelector_data_t    fieldSel9_t;

    /* Field Seletor 9 for outer IP version */
    memset(&fieldSel9_t, 0, sizeof(rtk_pie_fieldSelector_data_t));
    fieldSel9_t.start = FS_START_POS_L4;
    fieldSel9_t.payloadSel = PIE_FS_OUTER;
    /* FPFA cannot handle either GTP-u E,S,N flag is 1 case */
    /* Therefore, all GTP-u E,S,N flag is 0, the GTP-u header is 8 bytes */
    fieldSel9_t.offset = 4;  /* GTP-u header 8 bytes (8)/2*/
    ret = rtk_pie_fieldSelector_set(RTK_DPA_UNIT, RTK_DPA_MATCH_L2_INNER_IP_VER_FSID, &fieldSel9_t);
    if(ret != RT_ERR_OK)
            return ret;

    /* Configure ACL field seletor for special new add match fields */
    /* Field Seletor 8 for GTP MT; GTP header message type = 0xFE */
    memset(&fieldSel8_t, 0, sizeof(rtk_pie_fieldSelector_data_t));
    fieldSel8_t.start = FS_START_POS_L4;
    fieldSel8_t.payloadSel = PIE_FS_OUTER;
    fieldSel8_t.offset = 0;
    ret = rtk_pie_fieldSelector_set(RTK_DPA_UNIT, RTK_DPA_MATCH_GTP_MT_FSID, &fieldSel8_t);

    return ret;
}

#define RTK_DEMO_MGMT_MIRROR_PORT       2
#define RTK_DEMO_MGMT_PORT              56
#define RTK_DEMO_INTERNAL_CPU_PORT      57

#if 0
int
rtk_demoBoard_defRules(void)
{
    rtk_dpa_operation_t             rule_op = RTK_DPA_OPERATION_ADD_DEF;
    rtk_dpa_l2_fwd_match_t         *rule_match_field_l2;
    int                             l2_match_num = 2;
    rtk_dpa_l2_fwd_action_t        *rule_action_field_l2;
    int                             l2_action_num = 1;
    int                             ret = RT_ERR_OK;

    rule_match_field_l2 = (rtk_dpa_l2_fwd_match_t *) malloc((sizeof(rtk_dpa_l2_fwd_match_t)) * l2_match_num);
    if(rule_match_field_l2 == NULL)
    {
        ret = RT_ERR_FAILED;
        got exit;
    }
    rule_action_field_l2 = (rtk_dpa_l2_fwd_action_t *) malloc((sizeof(rtk_dpa_l2_fwd_action_t)) * l2_action_num);
    if(rule_action_field_l2 == NULL)
    {
        ret = RT_ERR_FAILED;
        got exit;
    }


    memset(rule_match_field_l2, 0, sizeof((sizeof(rtk_dpa_l2_fwd_match_t)) * l2_match_num));
    memset(rule_action_field_l2, 0, sizeof((sizeof(rtk_dpa_l2_fwd_action_t)) * l2_action_num));

    rule_match_field_l2[0].type = RTK_DPA_L2_FWD_TYPE_ETH_TYPE;
    rule_match_field_l2[0].data.eth_type = 0x0806;
    rule_match_field_l2[0].mask.eth_type = 0xffff;

    rule_match_field_l2[1].type = RTK_DPA_L2_FWD_TYPE_SRC_PORT;
    rule_match_field_l2[1].data.src_port = RTK_DEMO_MGMT_PORT;
    rule_match_field_l2[1].mask.src_port = 0xff;

    rule_action_field_l2[0].action = RTK_DPA_L2_FWD_ACTION_TYPE_OUTPUT;
    rule_action_field_l2[0].data.portmask.portmask.bits[0] = 0x1;       /* Mask Port 2 & 5 */
    rule_action_field_l2[0].data.portmask.portmask.bits[1] = 0x01000000; /* Mask Port 57 */

    ret = rtk_dpa_l2_fwd_config(rule_op, rule_match_field_l2, l2_match_num, rule_action_field_l2, l2_action_num);

    rule_match_field_l2[0].type = RTK_DPA_L2_FWD_TYPE_ETH_TYPE;
    rule_match_field_l2[0].data.eth_type = 0x0806;
    rule_match_field_l2[0].mask.eth_type = 0xffff;

    rule_match_field_l2[1].type = RTK_DPA_L2_FWD_TYPE_SRC_PORT;
    rule_match_field_l2[1].data.src_port = RTK_DEMO_INTERNAL_CPU_PORT;
    rule_match_field_l2[1].mask.src_port = 0xff;

    rule_action_field_l2[0].action = RTK_DPA_L2_FWD_ACTION_TYPE_OUTPUT;
    rule_action_field_l2[0].data.portmask.portmask.bits[0] = 0x1;       /* Mask Port 2 & 5 */
    rule_action_field_l2[0].data.portmask.portmask.bits[1] = 0x01000000; /* Mask Port 57 */

    ret = rtk_dpa_l2_fwd_config(rule_op, rule_match_field_l2, l2_match_num, rule_action_field_l2, l2_action_num);

    rule_match_field_l2[0].type = RTK_DPA_L2_FWD_TYPE_IPPROT;
    rule_match_field_l2[0].data.ip_proto = 0x1;     /* ICMP */
    rule_match_field_l2[0].mask.ip_proto = 0xff;

    rule_match_field_l2[1].type = RTK_DPA_L2_FWD_TYPE_SRC_PORT;
    rule_match_field_l2[1].data.src_port = RTK_DEMO_MGMT_PORT;
    rule_match_field_l2[1].mask.src_port = 0xff;

    rule_action_field_l2[0].action = RTK_DPA_L2_FWD_ACTION_TYPE_OUTPUT;
    rule_action_field_l2[0].data.portmask.portmask.bits[0] = 0x1;        /* Mask Port 2 & 5 */
    rule_action_field_l2[0].data.portmask.portmask.bits[1] = 0x01000000;  /* Mask Port 57 */

    ret = rtk_dpa_l2_fwd_config(rule_op, rule_match_field_l2, l2_match_num, rule_action_field_l2, l2_action_num);

    rule_match_field_l2[0].type = RTK_DPA_L2_FWD_TYPE_IPPROT;
    rule_match_field_l2[0].data.ip_proto = 0x1;     /* ICMP */
    rule_match_field_l2[0].mask.ip_proto = 0xff;

    rule_match_field_l2[1].type = RTK_DPA_L2_FWD_TYPE_SRC_PORT;
    rule_match_field_l2[1].data.src_port = RTK_DEMO_INTERNAL_CPU_PORT;
    rule_match_field_l2[1].mask.src_port = 0xff;

    rule_action_field_l2[0].action = RTK_DPA_L2_FWD_ACTION_TYPE_OUTPUT;
    rule_action_field_l2[0].data.portmask.portmask.bits[0] = 0x1;
    rule_action_field_l2[0].data.portmask.portmask.bits[1] = 0x01000000;

    ret = rtk_dpa_l2_fwd_config(rule_op, rule_match_field_l2, l2_match_num, rule_action_field_l2, l2_action_num);

exit:
    if(rule_match_field_l2 != NULL)
        free(rule_match_field_l2);
    if(rule_action_field_l2 != NULL)
        free(rule_action_field_l2);

    return ret;
}
#endif
int
rtk_demoBoard_defRules(void)
{
    rtk_dpa_operation_t             rule_op = RTK_DPA_OPERATION_ADD;
    rtk_dpa_ip_fwd_match_t         *rule_match_field_ip = NULL;
    int                             ip_match_num = 2;
    rtk_dpa_ip_fwd_action_t        *rule_action_field_ip = NULL;
    int                             ip_action_num = 1;
    int                             ret = RT_ERR_OK;

    rule_match_field_ip = (rtk_dpa_ip_fwd_match_t *) malloc((sizeof(rtk_dpa_ip_fwd_match_t)) * ip_match_num);
    if(rule_match_field_ip == NULL)
        goto exit;
    rule_action_field_ip = (rtk_dpa_ip_fwd_action_t *) malloc((sizeof(rtk_dpa_ip_fwd_action_t)) * ip_action_num);
    if(rule_action_field_ip == NULL)
        goto exit;

    memset(rule_match_field_ip, 0, sizeof((sizeof(rtk_dpa_ip_fwd_match_t)) * ip_match_num));
    memset(rule_action_field_ip, 0, sizeof((sizeof(rtk_dpa_ip_fwd_action_t)) * ip_action_num));

    rule_op = RTK_DPA_OPERATION_ADD;

    rule_match_field_ip[0].type = RTK_DPA_IP_FWD_TYPE_DIP;
    rule_match_field_ip[0].data.dip = 0xc0a8780c;
    rule_match_field_ip[0].mask.dip = 0xffffff00;

    rule_match_field_ip[1].type = RTK_DPA_IP_FWD_TYPE_SRC_PORT;
    rule_match_field_ip[1].data.src_port = 0x33;
    rule_match_field_ip[1].mask.src_port = 0x33;

    rule_action_field_ip[0].action = RTK_DPA_IP_FWD_ACTION_TYPE_OUTPUT;
    rule_action_field_ip[0].data.output.dmac.octet[5] = 0xb4;
    rule_action_field_ip[0].data.output.dmac.octet[4] = 0xa5;
    rule_action_field_ip[0].data.output.dmac.octet[3] = 0xef;
    rule_action_field_ip[0].data.output.dmac.octet[2] = 0xfe;
    rule_action_field_ip[0].data.output.dmac.octet[1] = 0x4d;
    rule_action_field_ip[0].data.output.dmac.octet[0] = 0xd4;
    rule_action_field_ip[0].data.output.smac.octet[5] = 0xa0;
    rule_action_field_ip[0].data.output.smac.octet[4] = 0x36;
    rule_action_field_ip[0].data.output.smac.octet[3] = 0x9f;
    rule_action_field_ip[0].data.output.smac.octet[2] = 0xab;
    rule_action_field_ip[0].data.output.smac.octet[1] = 0x33;
    rule_action_field_ip[0].data.output.smac.octet[0] = 0xac;
    rule_action_field_ip[0].data.output.portmask.bits[0] = 0x1;
    rule_action_field_ip[0].data.output.portmask.bits[1] = 0x0;

    DPA_MODULE_MSG("\n[%s][%d] index 0 ADD\n",__FUNCTION__,__LINE__);
    ret = rtk_dpa_ip_fwd_config(rule_op, rule_match_field_ip, ip_match_num, rule_action_field_ip, ip_action_num);
    if(ret != RT_ERR_OK)
        DPA_MODULE_MSG("\n[%s][%d] index 0 Failed\n",__FUNCTION__,__LINE__);

    free(rule_match_field_ip);
    free(rule_action_field_ip);


    ip_match_num = 2;
    ip_action_num = 1;
    rule_match_field_ip = (rtk_dpa_ip_fwd_match_t *) malloc((sizeof(rtk_dpa_ip_fwd_match_t)) * ip_match_num);
    if(rule_match_field_ip == NULL)
        goto exit;
    rule_action_field_ip = (rtk_dpa_ip_fwd_action_t *) malloc((sizeof(rtk_dpa_ip_fwd_action_t)) * ip_action_num);
    if(rule_action_field_ip == NULL)
        goto exit;

    memset(rule_match_field_ip, 0, sizeof((sizeof(rtk_dpa_ip_fwd_match_t)) * ip_match_num));
    memset(rule_action_field_ip, 0, sizeof((sizeof(rtk_dpa_ip_fwd_action_t)) * ip_action_num));

    rule_op = RTK_DPA_OPERATION_ADD;

    rule_match_field_ip[0].type = RTK_DPA_IP_FWD_TYPE_DIP;
    rule_match_field_ip[0].data.dip = 0x01010101;
    rule_match_field_ip[0].mask.dip = 0x0;

    rule_match_field_ip[1].type = RTK_DPA_IP_FWD_TYPE_SRC_PORT;
    rule_match_field_ip[1].data.src_port = 0x21;
    rule_match_field_ip[1].mask.src_port = 0x21;

    rule_action_field_ip[0].action = RTK_DPA_IP_FWD_ACTION_TYPE_OUTPUT;
    rule_action_field_ip[0].data.output.dmac.octet[5] = 0x11;
    rule_action_field_ip[0].data.output.dmac.octet[4] = 0x22;
    rule_action_field_ip[0].data.output.dmac.octet[3] = 0x33;
    rule_action_field_ip[0].data.output.dmac.octet[2] = 0x44;
    rule_action_field_ip[0].data.output.dmac.octet[1] = 0x55;
    rule_action_field_ip[0].data.output.dmac.octet[0] = 0x66;
    rule_action_field_ip[0].data.output.smac.octet[5] = 0x11;
    rule_action_field_ip[0].data.output.smac.octet[4] = 0x22;
    rule_action_field_ip[0].data.output.smac.octet[3] = 0x33;
    rule_action_field_ip[0].data.output.smac.octet[2] = 0x44;
    rule_action_field_ip[0].data.output.smac.octet[1] = 0x77;
    rule_action_field_ip[0].data.output.smac.octet[0] = 0x88;
    rule_action_field_ip[0].data.output.portmask.bits[0] = 0x0;
    rule_action_field_ip[0].data.output.portmask.bits[1] = 0x00200000;


    DPA_MODULE_MSG("\n[%s][%d] index 1 ADD\n",__FUNCTION__,__LINE__);
    ret = rtk_dpa_ip_fwd_config(rule_op, rule_match_field_ip, ip_match_num, rule_action_field_ip, ip_action_num);
    if(ret != RT_ERR_OK)
        DPA_MODULE_MSG("\n[%s][%d] index 1 Failed\n",__FUNCTION__,__LINE__);

    free(rule_match_field_ip);
    free(rule_action_field_ip);


    ip_match_num = 2;
    ip_action_num = 1;
    rule_match_field_ip = (rtk_dpa_ip_fwd_match_t *) malloc((sizeof(rtk_dpa_ip_fwd_match_t)) * ip_match_num);
    if(rule_match_field_ip == NULL)
        goto exit;
    rule_action_field_ip = (rtk_dpa_ip_fwd_action_t *) malloc((sizeof(rtk_dpa_ip_fwd_action_t)) * ip_action_num);
    if(rule_action_field_ip == NULL)
        goto exit;

    memset(rule_match_field_ip, 0, sizeof((sizeof(rtk_dpa_ip_fwd_match_t)) * ip_match_num));
    memset(rule_action_field_ip, 0, sizeof((sizeof(rtk_dpa_ip_fwd_action_t)) * ip_action_num));

    rule_op = RTK_DPA_OPERATION_ADD;

    rule_match_field_ip[0].type = RTK_DPA_IP_FWD_TYPE_DIP;
    rule_match_field_ip[0].data.dip = 0x01010101;
    rule_match_field_ip[0].mask.dip = 0x0;

    rule_match_field_ip[1].type = RTK_DPA_IP_FWD_TYPE_SRC_PORT;
    rule_match_field_ip[1].data.src_port = 0x36;
    rule_match_field_ip[1].mask.src_port = 0x36;

    rule_action_field_ip[0].action = RTK_DPA_IP_FWD_ACTION_TYPE_OUTPUT;
    rule_action_field_ip[0].data.output.dmac.octet[5] = 0x11;
    rule_action_field_ip[0].data.output.dmac.octet[4] = 0x22;
    rule_action_field_ip[0].data.output.dmac.octet[3] = 0x33;
    rule_action_field_ip[0].data.output.dmac.octet[2] = 0x44;
    rule_action_field_ip[0].data.output.dmac.octet[1] = 0x55;
    rule_action_field_ip[0].data.output.dmac.octet[0] = 0x66;
    rule_action_field_ip[0].data.output.smac.octet[5] = 0x11;
    rule_action_field_ip[0].data.output.smac.octet[4] = 0x22;
    rule_action_field_ip[0].data.output.smac.octet[3] = 0x33;
    rule_action_field_ip[0].data.output.smac.octet[2] = 0x44;
    rule_action_field_ip[0].data.output.smac.octet[1] = 0x77;
    rule_action_field_ip[0].data.output.smac.octet[0] = 0x88;
    rule_action_field_ip[0].data.output.portmask.bits[0] = 0x0;
    rule_action_field_ip[0].data.output.portmask.bits[1] = 0x1;


    DPA_MODULE_MSG("\n[%s][%d] index 2 ADD\n",__FUNCTION__,__LINE__);
    ret = rtk_dpa_ip_fwd_config(rule_op, rule_match_field_ip, ip_match_num, rule_action_field_ip, ip_action_num);
    if(ret != RT_ERR_OK)
        DPA_MODULE_MSG("\n[%s][%d] index 2 Failed\n",__FUNCTION__,__LINE__);

#if 0
    free(rule_match_field_ip);
    free(rule_action_field_ip);

    ip_match_num = 1;
    ip_action_num = 1;
    rule_match_field_ip = (rtk_dpa_ip_fwd_match_t *) malloc((sizeof(rtk_dpa_ip_fwd_match_t)) * ip_match_num);
    if(rule_match_field_ip == NULL)
        goto exit;
    rule_action_field_ip = (rtk_dpa_ip_fwd_action_t *) malloc((sizeof(rtk_dpa_ip_fwd_action_t)) * ip_action_num);
    if(rule_action_field_ip == NULL)
        goto exit;

    memset(rule_match_field_ip, 0, sizeof((sizeof(rtk_dpa_ip_fwd_match_t)) * ip_match_num));
    memset(rule_action_field_ip, 0, sizeof((sizeof(rtk_dpa_ip_fwd_action_t)) * ip_action_num));

    rule_op = RTK_DPA_OPERATION_ADD;

    rule_match_field_ip[0].type = RTK_DPA_IP_FWD_TYPE_DIP;
    rule_match_field_ip[0].data.dip = 0xac14e06e;
    rule_match_field_ip[0].mask.dip = 0xffffff00;

    rule_action_field_ip[0].action = RTK_DPA_IP_FWD_ACTION_TYPE_ENCAP;
    rule_action_field_ip[0].data.encap.teid = 0x1003d08;
    rule_action_field_ip[0].data.encap.outer_dip = 0xc0a8780c;
    rule_action_field_ip[0].data.encap.outer_sip = 0xc0a87810;
    rule_action_field_ip[0].data.encap.ttl = 0;
    rule_action_field_ip[0].data.encap.diffserv = 0;
    rule_action_field_ip[0].data.encap.identification = 0;
    rule_action_field_ip[0].data.encap.outer_udp_srcPort = 0;
    rule_action_field_ip[0].data.encap.outer_udp_destPort = 0;

    DPA_MODULE_MSG("\n[%s][%d] index 3 ADD\n",__FUNCTION__,__LINE__);
    ret = rtk_dpa_ip_fwd_config(rule_op, rule_match_field_ip, 1, rule_action_field_ip, ip_action_num);
    if(ret != RT_ERR_OK)
        DPA_MODULE_MSG("\n[%s][%d] index 3 Failed\n",__FUNCTION__,__LINE__);

    free(rule_match_field_ip);
    free(rule_action_field_ip);
#endif

exit:
    if(rule_match_field_ip)
        free(rule_match_field_ip);
    if(rule_action_field_ip)
        free(rule_action_field_ip);

    return ret;
}


/* This is a example only, This port should be customerized */
int
rtk_demoBoard_init_example(void)
{
    int             ret = RT_ERR_OK;
    uint32_t        port_listSize;
    uint32_t        port;
    int             index;
    rtk_enable_t    portEnable;

    /* RTK SDN used OTAG to check VID  */
    /* Set outer TPID entry 0*/
    rtk_vlan_tpidEntry_set(RTK_DPA_UNIT, VLAN_TAG_TYPE_OUTER, 0, RTK_DPA_CHT_TL_OUTER_VLAN_TPID);
    port_listSize = sizeof(g_dpa_portList);
    for(index = 0; index < port_listSize; index ++)
    {
        port = (uint32_t)g_dpa_portList[index];
        if(port == 56)
            continue;

        portEnable =  ENABLED;
        ret = rtk_port_adminEnable_set(RTK_DPA_UNIT, port, portEnable);
        if(ret != RT_ERR_OK)
        {
            DPA_MODULE_MSG("\n[%s][%d] port = %d\n",__FUNCTION__,__LINE__,port);
            return ret;
        }
        ret = rtk_vlan_portIgrTpid_set(RTK_DPA_UNIT,  port, OUTER_VLAN, 0x1); /* Check outer TPID entry 0*/
        if(ret != RT_ERR_OK)
        {
            DPA_MODULE_MSG("\n[%s][%d] port = %d\n",__FUNCTION__,__LINE__,port);
            return ret;
        }
    }
    return ret;
}


/* example
    rtk_dpa_teid_learn_match_t *match, *match_head=NULL, ;
    rtk_dpa_teid_learn_action_t *act, *act_head=NULL, ;
    int match_num, act_number;


    // match
    match_num = get_match_number();
    match_head = match = malloc(match_num*sizeof(rtk_dpa_teid_learn_match_t));

    str = get_next_match_string(NULL);
    while(str != NULL){

        match->type = according to str....
        match->data.dip = according to str....

        match++;
        str = get_next_match_string(str);
    }

    // action
    same above

    rtk_dpa_operation_t op = get_op_value();
    rtk_dpa_teid_learn_config(op, match_head, match_num, act_head, act_number);


*/

/* Function Name:
 *      rtk_dpa_module_init
 * Description:
 *      Initial DPA module
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Applicable:
 *      9310
 * Note:
 *
 * Changes:
 *      None
 */
int32
rtk_dpa_module_init(void)
{
    int             ret = RT_ERR_OK;
    uint32_t        port_listSize;
    uint32_t        port;
    int             index;

    g_dpaModule_initial_status      = RTK_DPA_MODULE_UNINITIALIZED;
     /* Initial DPA capacity */
    g_teidLearn_table_activeEntry   = TEID_LEARN_TABLE_MAX_ENTRY;
    g_ipFwd_table_activeEntry       = IP_FWD_TABLE_MAX_ENTRY;
    g_appList_table_activeEntry     = APP_LIST_TABLE_MAX_ENTRY;
    g_l2Fwd_table_activeEntry       = L2_FWD_TABLE_MAX_ENTRY;
    g_dpa_meter_activeEntry         = DPA_CAPAC_METER_MAX_NUM;
    g_dpa_port_activeMask_low       = DPA_CAPAC_PORT_MASK_LOW;
    g_dpa_port_activeMask_high      = DPA_CAPAC_PORT_MASK_HIGH;

    memset(g_dpa_meter, 0, (sizeof(rtk_dpa_meter_t) * DPA_CAPAC_METER_MAX_NUM));



    /* Initial RTK SDN Module */
    ret = rtk_sdn_init();
    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] rtk_sdn_init FAILED\n",__FUNCTION__,__LINE__);
        return ret;
    }

    port_listSize = sizeof(g_dpa_portList);
    for(index = 0; index < port_listSize; index ++)
    {
        port = (uint32_t)g_dpa_portList[index];
        ret = rtk_sdn_port_add((RTK_DPA_PHY_TO_SDN_PORT(port)));
        if(ret != RT_ERR_OK)
        {
            DPA_MODULE_MSG("\n[%s][%d] port = %d, index = %d\n",__FUNCTION__,__LINE__,port, index);
            return ret;
        }
    }

    ret = _l2_fwd_extend_matchField_setup();
    if(ret != RT_ERR_OK)
        return ret;

    ret = _app_list_extend_matchField_setup();
    if(ret != RT_ERR_OK)
        return ret;

    ret = rtk_gtp_fpga_module_init();
    if(ret != RT_ERR_OK)
        return ret;

    /* Shoule be customerized */
    ret = rtk_demoBoard_init_example();
    if(ret != RT_ERR_OK)
        return ret;

    /* By CHT TL request; APP_LIST default table miss action is change to OF_TBLMISS_ACT_EXEC_ACTION_SET */
    ret = rtk_sdn_set_table_miss_action(RTK_DPA_TABLE_ID_APP_LIST, OF_TBLMISS_ACT_EXEC_ACTION_SET);
    if(ret != RT_ERR_OK)
        return ret;

    g_dpaModule_initial_status      = RTK_DPA_MODULE_INITIALIZED;

#ifdef ENABLE_DEFAULT_RULES
    ret = rtk_demoBoard_defRules();
    if(ret != RT_ERR_OK)
        return ret;
#endif
    return ret;
}


/* Function Name:
 *      rtk_dpa_teid_learn_config
 * Description:
 *      Add/Add as default/Delete entry into table
 * Input:
 *      op              - table entry operation
 *      pMatch          - entry's match field array
 *      num_of_match    - total match field
 *      pAct            - entry's action field array
 *      num_of_act      - total action field
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Applicable:
 *      9310
 * Note:
 *
 * Changes:
 *      None
 */
int32
rtk_dpa_teid_learn_config(rtk_dpa_operation_t op, rtk_dpa_teid_learn_match_t *pMatch, int num_of_match, rtk_dpa_teid_learn_action_t *pAct, int num_of_act)
{
#ifdef SOFTWARE_SIMULATE
    switch(op){
        case RTK_DPA_OPERATION_ADD_DEF:
            DPA_MODULE_DBG("a default entry was added\n");
            break;
        case RTK_DPA_OPERATION_ADD:
            DPA_MODULE_DBG("a entry was added\n");
            break;
        case RTK_DPA_OPERATION_DEL:
            DPA_MODULE_DBG("a entry was deleted\n");
            break;
        case RTK_DPA_OPERATION_FIND:
            DPA_MODULE_DBG("a entry was found\n");
            break;
        default:
            break;
    }

    return RT_ERR_OK;
#else
    int ret;
    int inst_num;
    uint8 match_srcPort;
    sdn_db_flow_entry_t *pFlow;

    RTK_DPA_MODULE_INIT_CHECK(g_dpaModule_initial_status);
    RTK_DPA_CAPACITY_CHECK(g_teidLearn_table_activeEntry, TEID_LEARN_TABLE_MAX_ENTRY);

    pFlow = (sdn_db_flow_entry_t *) malloc(sizeof(sdn_db_flow_entry_t));
    if(pFlow == NULL)
    {
        return RT_ERR_FAILED;
    }
    ret = _teid_learn_match_get_src_port(pMatch, num_of_match, &match_srcPort);
    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] SRC PORT get FAILED!!!\n",__FUNCTION__,__LINE__);
        free(pFlow);
        return RT_ERR_FAILED;
    }

    ret = _teid_learn_rule_match_convert(pMatch, num_of_match, &(pFlow->match_field_p));
    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] Match convert FAILED!!!\n",__FUNCTION__,__LINE__);
        free(pFlow);
        return RT_ERR_FAILED;
    }

    DPA_MODULE_MSG("\n[%s][%d] num_of_match = %d\n",__FUNCTION__,__LINE__,num_of_match);

    ret = _teid_learn_inst_action_convert(pAct, num_of_act, match_srcPort, op, &(pFlow->instruction_p), &inst_num);
    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] Instruction convert FAILED!!!\n",__FUNCTION__,__LINE__);
        free(pFlow);
        return RT_ERR_FAILED;
    }

    DPA_MODULE_MSG("\n[%s][%d] inst_num = %d\n",__FUNCTION__,__LINE__,inst_num);

    pFlow->table_id = RTK_DPA_TABLE_ID_TEID_LREAD;
    pFlow->len_match = num_of_match;
    pFlow->len_inst = inst_num;

    switch(op){
        case RTK_DPA_OPERATION_ADD_DEF:
            DPA_MODULE_MSG("\n[%s][%d] RTK_DPA_OPERATION_ADD_DEF\n",__FUNCTION__,__LINE__);
            pFlow->priority = TEID_LEARN_ADD_DEF_PRIORITY;
#ifdef RTK_DPA_CHT_DISABLE_TABLE_MOVE
            ret = cht_sdn_add_flow(pFlow);
#else
            ret = rtk_sdn_add_flow(pFlow);
#endif
            if(ret == RT_ERR_OK)
                g_teidLearn_table_activeEntry--;
            break;
        case RTK_DPA_OPERATION_ADD:
            DPA_MODULE_MSG("\n[%s][%d] RTK_DPA_OPERATION_ADD\n",__FUNCTION__,__LINE__);
            pFlow->priority = TEID_LEARN_ADD_PRIORITY;
#ifdef RTK_DPA_CHT_DISABLE_TABLE_MOVE
            ret = cht_sdn_add_flow(pFlow);
#else
            ret = rtk_sdn_add_flow(pFlow);
#endif
            if(ret == RT_ERR_OK)
                g_teidLearn_table_activeEntry--;
            break;
        case RTK_DPA_OPERATION_DEL:
            DPA_MODULE_MSG("\n[%s][%d] RTK_DPA_OPERATION_DEL\n",__FUNCTION__,__LINE__);
#ifdef RTK_DPA_CHT_DISABLE_TABLE_MOVE
            ret = cht_sdn_delete_flow(pFlow->table_id, TEID_LEARN_ADD_PRIORITY, pFlow->len_match, pFlow->match_field_p);
#else
            ret = rtk_sdn_delete_flow(pFlow->table_id, TEID_LEARN_ADD_PRIORITY, pFlow->len_match, pFlow->match_field_p);
#endif
            if(ret == RT_ERR_OK)
                g_teidLearn_table_activeEntry++;
            break;
        case RTK_DPA_OPERATION_DEL_DEF:
            DPA_MODULE_MSG("\n[%s][%d] RTK_DPA_OPERATION_DEL_DEF\n",__FUNCTION__,__LINE__);
#ifdef RTK_DPA_CHT_DISABLE_TABLE_MOVE
            ret = cht_sdn_delete_flow(pFlow->table_id, TEID_LEARN_ADD_DEF_PRIORITY, pFlow->len_match, pFlow->match_field_p);
#else
            ret = rtk_sdn_delete_flow(pFlow->table_id, TEID_LEARN_ADD_DEF_PRIORITY, pFlow->len_match, pFlow->match_field_p);
#endif

            if(ret == RT_ERR_OK)
                g_teidLearn_table_activeEntry++;
            break;
        case RTK_DPA_OPERATION_FIND:
            break;
        default:
            break;
    }

    if(ret != RT_ERR_OK)
    {
        return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
#endif //SOFTWARE_SIMMULATE
}


/* Function Name:
 *      rtk_dpa_teid_learn_find
 * Description:
 *      find the entry from TEID LEARN table
 * Input:
 *      pMatch          - entry's match field array
 *      num_of_match    - total match field
 *      rule_priority   - TEID_LEARN_ADD_DEF_PRIORITY/TEID_LEARN_ADD_PRIORITY
 * Output:
 *      pEntryIndex     - This output parameter is vaild when the result is TURE.
 *      pResult         - TRUE/FALSE
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Applicable:
 *      9310
 * Note:
 *
 * Changes:
 *      None
 */
int32
rtk_dpa_teid_learn_find(rtk_dpa_teid_learn_match_t *pMatch, int num_of_match, int rule_priority, int *pEntryIndex, int *pResult)
{

#ifdef SOFTWARE_SIMULATE
    *pResult = TRUE;
#else
    int                     ret;
    sdn_db_flow_entry_t     *pFlow;
    uint32                  entry_idx = 0;
    uint32                  entry_found = 0;

    RTK_DPA_MODULE_INIT_CHECK(g_dpaModule_initial_status);


    pFlow = (sdn_db_flow_entry_t *) malloc(sizeof(sdn_db_flow_entry_t));

    if(pFlow == NULL)
    {
        return RT_ERR_FAILED;
    }

    ret = _teid_learn_rule_match_convert(pMatch, num_of_match, &(pFlow->match_field_p));
    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] FAILED!!!\n",__FUNCTION__,__LINE__);
        free(pFlow);
        return RT_ERR_FAILED;
    }
    pFlow->table_id = RTK_DPA_TABLE_ID_TEID_LREAD;
    pFlow->len_match = num_of_match;
    pFlow->priority = rule_priority;

#ifdef RTK_DPA_CHT_DISABLE_TABLE_MOVE
    ret = cht_sdn_flow_entry_search(pFlow->table_id, pFlow->priority, pFlow->len_match, (pFlow->match_field_p), &entry_idx, &entry_found);
#else
    ret = rtk_sdn_flow_entry_search(pFlow->table_id, pFlow->priority, pFlow->len_match, (pFlow->match_field_p), &entry_idx, &entry_found);
#endif

    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] FAILED!!!\n",__FUNCTION__,__LINE__);
        free(pFlow);
        return RT_ERR_FAILED;
    }

    *pResult = (int)entry_found;
    if(entry_found == TRUE)
    {
        *pEntryIndex = (int)entry_idx;
    }
    free(pFlow);
#endif //SOFTWARE_SIMMULATE
    return RT_ERR_OK;

}

/* Function Name:
 *      rtk_dpa_ip_fwd_config
 * Description:
 *      Add/Add as default/Delete entry into table
 * Input:
 *      op              - table entry operation
 *      pMatch          - entry's match field array
 *      num_of_match    - total match field
 *      pAct            - entry's action field array
 *      num_of_act      - total action field
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Applicable:
 *      9310
 * Note:
 *
 * Changes:
 *      None
 */
int32
rtk_dpa_ip_fwd_config(rtk_dpa_operation_t op, rtk_dpa_ip_fwd_match_t *pMatch, int num_of_match, rtk_dpa_ip_fwd_action_t *pAct, int num_of_act)
{
#ifdef SOFTWARE_SIMULATE
    switch(op){
        case RTK_DPA_OPERATION_ADD_DEF:
            DPA_MODULE_MSG("a default entry was added\n");
            break;
        case RTK_DPA_OPERATION_ADD:
            DPA_MODULE_MSG("a entry was added\n");
            break;
        case RTK_DPA_OPERATION_DEL:
            DPA_MODULE_MSG("a entry was deleted\n");
            break;
        case RTK_DPA_OPERATION_FIND:
            DPA_MODULE_MSG("a entry was found\n");
            break;
        default:
            break;
    }

    return RT_ERR_OK;
#else
    int ret;
    int inst_num;
    sdn_db_flow_entry_t *pFlow;

    RTK_DPA_MODULE_INIT_CHECK(g_dpaModule_initial_status);
    RTK_DPA_CAPACITY_CHECK(g_ipFwd_table_activeEntry, IP_FWD_TABLE_MAX_ENTRY);

    pFlow = (sdn_db_flow_entry_t *) malloc(sizeof(sdn_db_flow_entry_t));
    if(pFlow == NULL)
    {
        return RT_ERR_FAILED;
    }

    ret = _ip_fwd_rule_match_convert(pMatch, num_of_match, &(pFlow->match_field_p));
    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] Match convert FAILED!!!\n",__FUNCTION__,__LINE__);
        free(pFlow);
        return RT_ERR_FAILED;
    }

    DPA_MODULE_MSG("\n[%s][%d] num_of_match = %d\n",__FUNCTION__,__LINE__,num_of_match);

    ret = _ip_fwd_inst_action_convert(pAct, num_of_act, &(pFlow->instruction_p), &inst_num, op);
    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] Instruction convert FAILED!!!\n",__FUNCTION__,__LINE__);
        free(pFlow);
        return RT_ERR_FAILED;
    }

    DPA_MODULE_MSG("\n[%s][%d] inst_num = %d\n",__FUNCTION__,__LINE__,inst_num);

    pFlow->table_id = RTK_DPA_TABLE_ID_IP_FWD;
    pFlow->len_match = num_of_match;
    pFlow->len_inst = inst_num;

    switch(op){
        case RTK_DPA_OPERATION_ADD_DEF:
            DPA_MODULE_MSG("\n[%s][%d] RTK_DPA_OPERATION_ADD_DEF\n",__FUNCTION__,__LINE__);
            pFlow->priority = IP_FWD_ADD_DEF_PRIORITY;
            ret = rtk_sdn_add_flow(pFlow);
            if(ret == RT_ERR_OK)
                g_ipFwd_table_activeEntry--;
            break;
        case RTK_DPA_OPERATION_ADD:
            DPA_MODULE_MSG("\n[%s][%d] RTK_DPA_OPERATION_ADD\n",__FUNCTION__,__LINE__);
            pFlow->priority = IP_FWD_ADD_PRIORITY;
            ret = rtk_sdn_add_flow(pFlow);
            if(ret == RT_ERR_OK)
                g_ipFwd_table_activeEntry--;
            break;
        case RTK_DPA_OPERATION_DEL:
            DPA_MODULE_MSG("\n[%s][%d] RTK_DPA_OPERATION_DEL\n",__FUNCTION__,__LINE__);
            ret = rtk_sdn_delete_flow(pFlow->table_id, IP_FWD_ADD_PRIORITY, pFlow->len_match, pFlow->match_field_p);
            if(ret == RT_ERR_OK)
                g_ipFwd_table_activeEntry++;
            break;
        case RTK_DPA_OPERATION_DEL_DEF:
            DPA_MODULE_MSG("\n[%s][%d] RTK_DPA_OPERATION_DEL_DEF\n",__FUNCTION__,__LINE__);
            ret = rtk_sdn_delete_flow(pFlow->table_id, IP_FWD_ADD_DEF_PRIORITY, pFlow->len_match, pFlow->match_field_p);
            if(ret == RT_ERR_OK)
                g_ipFwd_table_activeEntry++;
            break;
        case RTK_DPA_OPERATION_FIND:
            break;
        default:
            break;
    }

    if(ret != RT_ERR_OK)
    {
        return RT_ERR_FAILED;
    }
    return RT_ERR_OK;
#endif //SOFTWARE_SIMMULATE
}


/* Function Name:
 *      rtk_dpa_ip_fwd_find
 * Description:
 *      find the entry from IP FWD table.
 * Input:
 *      pMatch          - entry's match field array
 *      num_of_match    - total match field
 *      rule_priority   - IP_FWD_ADD_DEF_PRIORITY/IP_FWD_ADD_PRIORITY
 * Output:
 *      pEntryIndex     - This output parameter is vaild when the result is TURE.
 *      pResult         - TRUE/FALSE
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Applicable:
 *      9310
 * Note:
 *
 * Changes:
 *      None
 */
int32
rtk_dpa_ip_fwd_find(rtk_dpa_ip_fwd_match_t *pMatch, int num_of_match, int rule_priority, int *pEntryIndex, int *pResult)
{

#ifdef SOFTWARE_SIMULATE
    *pResult = TRUE;
#else
    int                     ret;
    sdn_db_flow_entry_t     *pFlow;
    uint32                  entry_idx = 0;
    uint32                  entry_found = 0;

    RTK_DPA_MODULE_INIT_CHECK(g_dpaModule_initial_status);

    pFlow = (sdn_db_flow_entry_t *) malloc(sizeof(sdn_db_flow_entry_t));

    if(pFlow == NULL)
    {
        return RT_ERR_FAILED;
    }

    ret = _ip_fwd_rule_match_convert(pMatch, num_of_match, &(pFlow->match_field_p));
    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] FAILED!!!\n",__FUNCTION__,__LINE__);
        free(pFlow);
        return RT_ERR_FAILED;
    }
    pFlow->table_id = RTK_DPA_TABLE_ID_IP_FWD;
    pFlow->len_match = num_of_match;
    pFlow->priority = rule_priority;

    ret = rtk_sdn_flow_entry_search(pFlow->table_id, pFlow->priority, pFlow->len_match, (pFlow->match_field_p), &entry_idx, &entry_found);

    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] FAILED!!!\n",__FUNCTION__,__LINE__);
        free(pFlow);
        return RT_ERR_FAILED;
    }

    *pResult = (int)entry_found;
    if(entry_found == TRUE)
    {
        *pEntryIndex = (int)entry_idx;
    }
    free(pFlow);

#endif //SOFTWARE_SIMMULATE

    return RT_ERR_OK;

}

/* Function Name:
 *      rtk_dpa_app_list_config
 * Description:
 *      Add/Add as default/Delete entry into table
 * Input:
 *      op              - table entry operation
 *      pMatch          - entry's match field array
 *      num_of_match    - total match field
 *      pAct            - entry's action field array
 *      num_of_act      - total action field
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Applicable:
 *      9310
 * Note:
 *
 * Changes:
 *      None
 */
int32
rtk_dpa_app_list_config(rtk_dpa_operation_t op, rtk_dpa_app_list_match_t *pMatch, int num_of_match, rtk_dpa_app_list_action_t *pAct, int num_of_act)
{
#ifdef SOFTWARE_SIMULATE
        switch(op){
            case RTK_DPA_OPERATION_ADD_DEF:
                DPA_MODULE_DBG("a default entry was added\n");
                break;
            case RTK_DPA_OPERATION_ADD:
                DPA_MODULE_DBG("a entry was added\n");
                break;
            case RTK_DPA_OPERATION_DEL:
                DPA_MODULE_DBG("a entry was deleted\n");
                break;
            case RTK_DPA_OPERATION_FIND:
                DPA_MODULE_DBG("a entry was found\n");
                break;
            default:
                break;
        }

        return RT_ERR_OK;
#else
        int ret;
        int inst_num;
        sdn_db_flow_entry_t *pFlow;

        RTK_DPA_MODULE_INIT_CHECK(g_dpaModule_initial_status);
        RTK_DPA_CAPACITY_CHECK(g_appList_table_activeEntry, APP_LIST_TABLE_MAX_ENTRY);

        pFlow = (sdn_db_flow_entry_t *) malloc(sizeof(sdn_db_flow_entry_t));
        if(pFlow == NULL)
        {
            return RT_ERR_FAILED;
        }
        ret = _app_list_rule_match_convert(pMatch, num_of_match, &(pFlow->match_field_p));
        if(ret != RT_ERR_OK)
        {
            DPA_MODULE_MSG("\n[%s][%d] Match convert FAILED!!!\n",__FUNCTION__,__LINE__);
            free(pFlow);
            return RT_ERR_FAILED;
        }

        DPA_MODULE_MSG("\n[%s][%d] num_of_match = %d\n",__FUNCTION__,__LINE__,num_of_match);

        ret = _app_list_inst_action_convert(pAct, num_of_act, &(pFlow->instruction_p), &inst_num);
        if(ret != RT_ERR_OK)
        {
            DPA_MODULE_MSG("\n[%s][%d] Instruction convert FAILED!!!\n",__FUNCTION__,__LINE__);
            free(pFlow);
            return RT_ERR_FAILED;
        }

        DPA_MODULE_MSG("\n[%s][%d] inst_num = %d\n",__FUNCTION__,__LINE__,inst_num);

        pFlow->table_id = RTK_DPA_TABLE_ID_APP_LIST;
        pFlow->len_match = num_of_match;
        pFlow->len_inst = inst_num;

        switch(op){
            case RTK_DPA_OPERATION_ADD_DEF:
                DPA_MODULE_MSG("\n[%s][%d] RTK_DPA_OPERATION_ADD_DEF\n",__FUNCTION__,__LINE__);
                pFlow->priority = APP_LIST_ADD_DEF_PRIORITY;
                ret = rtk_sdn_add_flow(pFlow);
                if(ret == RT_ERR_OK)
                    g_appList_table_activeEntry--;
                break;
            case RTK_DPA_OPERATION_ADD:
                DPA_MODULE_MSG("\n[%s][%d] RTK_DPA_OPERATION_ADD\n",__FUNCTION__,__LINE__);
                pFlow->priority = APP_LIST_ADD_PRIORITY;
                ret = rtk_sdn_add_flow(pFlow);
                if(ret == RT_ERR_OK)
                    g_appList_table_activeEntry--;
                break;
            case RTK_DPA_OPERATION_DEL:
                DPA_MODULE_MSG("\n[%s][%d] RTK_DPA_OPERATION_DEL\n",__FUNCTION__,__LINE__);
                ret = rtk_sdn_delete_flow(pFlow->table_id, APP_LIST_ADD_PRIORITY, pFlow->len_match, pFlow->match_field_p);
                if(ret == RT_ERR_OK)
                    g_appList_table_activeEntry++;
                break;
            case RTK_DPA_OPERATION_DEL_DEF:
                DPA_MODULE_MSG("\n[%s][%d] RTK_DPA_OPERATION_DEL_DEF\n",__FUNCTION__,__LINE__);
                ret = rtk_sdn_delete_flow(pFlow->table_id, APP_LIST_ADD_DEF_PRIORITY, pFlow->len_match, pFlow->match_field_p);
                if(ret == RT_ERR_OK)
                    g_appList_table_activeEntry++;
                break;
            case RTK_DPA_OPERATION_FIND:
                break;
            default:
                break;
        }

        if(ret != RT_ERR_OK)
        {
            return RT_ERR_FAILED;
        }

        return RT_ERR_OK;
#endif //SOFTWARE_SIMMULATE
}


/* Function Name:
 *      rtk_dpa_app_list_find
 * Description:
 *      find the entry from APP LIST table
 * Input:
 *      pMatch          - entry's match field array
 *      num_of_match    - total match field
 *      rule_priority   - APP_LIST_ADD_DEF_PRIORITY/APP_LIST_ADD_PRIORITY
 * Output:
 *      pEntryIndex     - This output parameter is vaild when the result is TURE.
 *      pResult         - TRUE/FALSE
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Applicable:
 *      9310
 * Note:
 *
 * Changes:
 *      None
 */
int32
rtk_dpa_app_list_find(rtk_dpa_app_list_match_t *pMatch, int num_of_match, int rule_priority, int *pEntryIndex, int *pResult)
{

#ifdef SOFTWARE_SIMULATE

    *pResult = TRUE;
#else
    int                     ret;
    sdn_db_flow_entry_t     *pFlow;
    uint32                  entry_idx = 0;
    uint32                  entry_found = 0;

    RTK_DPA_MODULE_INIT_CHECK(g_dpaModule_initial_status);

    pFlow = (sdn_db_flow_entry_t *) malloc(sizeof(sdn_db_flow_entry_t));

    if(pFlow == NULL)
    {
        return RT_ERR_FAILED;
    }

    ret = _app_list_rule_match_convert(pMatch, num_of_match, &(pFlow->match_field_p));
    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] FAILED!!!\n",__FUNCTION__,__LINE__);
        free(pFlow);
        return RT_ERR_FAILED;
    }
    pFlow->table_id = RTK_DPA_TABLE_ID_APP_LIST;
    pFlow->len_match = num_of_match;
    pFlow->priority = rule_priority;

    ret = rtk_sdn_flow_entry_search(pFlow->table_id, pFlow->priority, pFlow->len_match, (pFlow->match_field_p), &entry_idx, &entry_found);

    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] FAILED!!!\n",__FUNCTION__,__LINE__);
        free(pFlow);
        return RT_ERR_FAILED;
    }

    *pResult = (int)entry_found;
    if(entry_found == TRUE)
    {
        *pEntryIndex = (int)entry_idx;
    }
    free(pFlow);
#endif //SOFTWARE_SIMMULATE

    return RT_ERR_OK;

}

/* Function Name:
 *      rtk_dpa_l2_fwd_config
 * Description:
 *      Add/Add as default/Delete entry into table
 * Input:
 *      op              - table entry operation
 *      pMatch           - entry's match field array
 *      num_of_match    - total match field
 *      pAct             - entry's action field array
 *      num_of_act      - total action field
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Applicable:
 *      9310
 * Note:
 *
 * Changes:
 *      None
 */
int32
rtk_dpa_l2_fwd_config(rtk_dpa_operation_t op, rtk_dpa_l2_fwd_match_t *pMatch, int num_of_match, rtk_dpa_l2_fwd_action_t *pAct, int num_of_act)
{
#ifdef SOFTWARE_SIMULATE
    switch(op){
        case RTK_DPA_OPERATION_ADD_DEF:
            DPA_MODULE_MSG("a default entry was added\n");
            break;
        case RTK_DPA_OPERATION_ADD:
            DPA_MODULE_MSG("a entry was added\n");
            break;
        case RTK_DPA_OPERATION_DEL:
            DPA_MODULE_MSG("a entry was deleted\n");
            break;
        case RTK_DPA_OPERATION_FIND:
            DPA_MODULE_MSG("a entry was found\n");
            break;
        default:
            break;
    }

    return RT_ERR_OK;
#else
    int ret;
    int inst_num;
    uint8 match_srcPort;
    sdn_db_flow_entry_t *pFlow;

    RTK_DPA_MODULE_INIT_CHECK(g_dpaModule_initial_status);
    RTK_DPA_CAPACITY_CHECK(g_l2Fwd_table_activeEntry, L2_FWD_TABLE_MAX_ENTRY);

    pFlow = (sdn_db_flow_entry_t *) malloc(sizeof(sdn_db_flow_entry_t));
    if(pFlow == NULL)
    {
        return RT_ERR_FAILED;
    }

    ret = _l2_fwd_match_get_src_port(pMatch, num_of_match, &match_srcPort);
    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] SRC PORT get FAILED!!!\n",__FUNCTION__,__LINE__);
        free(pFlow);
        return RT_ERR_FAILED;
    }

    ret = _l2_fwd_rule_match_convert(pMatch, num_of_match, &(pFlow->match_field_p));
    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] Match convert FAILED!!!\n",__FUNCTION__,__LINE__);
        free(pFlow);
        return RT_ERR_FAILED;
    }

    DPA_MODULE_MSG("\n[%s][%d] num_of_match = %d\n",__FUNCTION__,__LINE__,num_of_match);

    ret = _l2_fwd_inst_action_convert(pAct, num_of_act, &(pFlow->instruction_p), &inst_num, match_srcPort);
    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] Instruction convert FAILED!!!\n",__FUNCTION__,__LINE__);
        free(pFlow);
        return RT_ERR_FAILED;
    }

    DPA_MODULE_MSG("\n[%s][%d] inst_num = %d\n",__FUNCTION__,__LINE__,inst_num);

    pFlow->table_id = RTK_DPA_TABLE_ID_L2_FWD;
    pFlow->len_match = num_of_match;
    pFlow->len_inst = inst_num;

    switch(op){
        case RTK_DPA_OPERATION_ADD_DEF:
            DPA_MODULE_MSG("\n[%s][%d] RTK_DPA_OPERATION_ADD_DEF\n",__FUNCTION__,__LINE__);
            pFlow->priority = L2_FWD_ADD_DEF_PRIORITY;
#ifdef RTK_DPA_CHT_DISABLE_TABLE_MOVE
            ret = cht_sdn_add_flow(pFlow);
#else
            ret = rtk_sdn_add_flow(pFlow);
#endif
            if(ret == RT_ERR_OK)
                g_l2Fwd_table_activeEntry--;
            break;
        case RTK_DPA_OPERATION_ADD:
            DPA_MODULE_MSG("\n[%s][%d] RTK_DPA_OPERATION_ADD\n",__FUNCTION__,__LINE__);
            pFlow->priority = L2_FWD_ADD_PRIORITY;
#ifdef RTK_DPA_CHT_DISABLE_TABLE_MOVE
            ret = cht_sdn_add_flow(pFlow);
#else
            ret = rtk_sdn_add_flow(pFlow);
#endif
            if(ret == RT_ERR_OK)
                g_l2Fwd_table_activeEntry--;
            break;
        case RTK_DPA_OPERATION_DEL:
            DPA_MODULE_MSG("\n[%s][%d] RTK_DPA_OPERATION_DEL\n",__FUNCTION__,__LINE__);
#ifdef RTK_DPA_CHT_DISABLE_TABLE_MOVE
            ret = cht_sdn_delete_flow(pFlow->table_id, L2_FWD_ADD_PRIORITY, pFlow->len_match, pFlow->match_field_p);
#else
            ret = rtk_sdn_delete_flow(pFlow->table_id, L2_FWD_ADD_PRIORITY, pFlow->len_match, pFlow->match_field_p);
#endif
            if(ret == RT_ERR_OK)
                g_l2Fwd_table_activeEntry++;
            break;
        case RTK_DPA_OPERATION_DEL_DEF:
            DPA_MODULE_MSG("\n[%s][%d] RTK_DPA_OPERATION_DEL_DEF\n",__FUNCTION__,__LINE__);
#ifdef RTK_DPA_CHT_DISABLE_TABLE_MOVE
            ret = cht_sdn_delete_flow(pFlow->table_id, L2_FWD_ADD_DEF_PRIORITY, pFlow->len_match, pFlow->match_field_p);
#else
            ret = rtk_sdn_delete_flow(pFlow->table_id, L2_FWD_ADD_DEF_PRIORITY, pFlow->len_match, pFlow->match_field_p);
#endif
            if(ret == RT_ERR_OK)
                g_l2Fwd_table_activeEntry++;
            break;
        case RTK_DPA_OPERATION_FIND:
            break;
        default:
            break;
    }

    if(ret != RT_ERR_OK)
    {
        return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
#endif //SOFTWARE_SIMMULATE
}


/* Function Name:
 *      rtk_dpa_l2_fwd_find
 * Description:
 *      find the entry from L2 FWD table.
 * Input:
 *      pMatch          - entry's match field array
 *      num_of_match    - total match field
 *      rule_priority   - L2_FWD_ADD_DEF_PRIORITY/L2_FWD_ADD_PRIORITY
 * Output:
 *      pEntryIndex     - This output parameter is vaild when the result is TURE.
 *      pResult         - TRUE/FALSE
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Applicable:
 *      9310
 * Note:
 *
 * Changes:
 *      None
 */
int32
rtk_dpa_l2_fwd_find(rtk_dpa_l2_fwd_match_t *pMatch, int num_of_match, int rule_priority, int *pEntryIndex, int *pResult)
{

#ifdef SOFTWARE_SIMULATE
    *pResult = TRUE;
#else
    int                     ret;
    sdn_db_flow_entry_t     *pFlow;
    uint32                  entry_idx = 0;
    uint32                  entry_found = 0;

    RTK_DPA_MODULE_INIT_CHECK(g_dpaModule_initial_status);

    pFlow = (sdn_db_flow_entry_t *) malloc(sizeof(sdn_db_flow_entry_t));


    if(pFlow == NULL)
    {
        return RT_ERR_FAILED;
    }

    ret = _l2_fwd_rule_match_convert(pMatch, num_of_match, &(pFlow->match_field_p));
    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] FAILED!!!\n",__FUNCTION__,__LINE__);
        free(pFlow);
        return RT_ERR_FAILED;
    }
    pFlow->table_id = RTK_DPA_TABLE_ID_L2_FWD;
    pFlow->len_match = num_of_match;
    pFlow->priority = rule_priority;
#ifdef RTK_DPA_CHT_DISABLE_TABLE_MOVE
    ret = cht_sdn_flow_entry_search(pFlow->table_id, pFlow->priority, pFlow->len_match, (pFlow->match_field_p), &entry_idx, &entry_found);
#else
    ret = rtk_sdn_flow_entry_search(pFlow->table_id, pFlow->priority, pFlow->len_match, (pFlow->match_field_p), &entry_idx, &entry_found);
#endif
    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] FAILED!!!\n",__FUNCTION__,__LINE__);
        free(pFlow);
        return RT_ERR_FAILED;
    }

    *pResult = (int)entry_found;
    if(entry_found == TRUE)
    {
        *pEntryIndex = (int)entry_idx;
    }
    free(pFlow);

#endif //SOFTWARE_SIMMULATE
    return RT_ERR_OK;

}

/* Function Name:
 *      rtk_dpa_capacity_get
 * Description:
 *      Get the DPA MAX capacity.
 * Input:
 *      tableName       - specific table
 * Output:
 *      pActiveNum      - active enrty number of specific table.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Applicable:
 *      9310
 * Note:
 *
 * Changes:
 *      None
 */
int32
rtk_dpa_table_activeEntryNum_get(rtk_dpa_tableName_t tableName, int *pActiveNum)
{

#ifdef SOFTWARE_SIMULATE
    *pActiveNum = 1000;
    return RT_ERR_OK;
#else
    RTK_DPA_MODULE_INIT_CHECK(g_dpaModule_initial_status);

    switch(tableName){
    case RTK_DPA_TABLE_NANE_TEID_LEARN:
        *pActiveNum = g_teidLearn_table_activeEntry;
        break;
    case RTK_DPA_TABLE_NANE_IP_FWD:
        *pActiveNum = g_ipFwd_table_activeEntry;
        break;
    case RTK_DPA_TABLE_NANE_APP_LIST:
        *pActiveNum = g_appList_table_activeEntry;
        break;
    case RTK_DPA_TABLE_NANE_L2_FWD:
        *pActiveNum = g_l2Fwd_table_activeEntry;
        break;
    default:
        return RT_ERR_INPUT;
    }
    return RT_ERR_OK;
#endif //SOFTWARE_SIMMULATE

}


/* Function Name:
 *      rtk_dpa_capacity_get
 * Description:
 *      Get DPA capacity.
 * Input:
 *      None
 * Output:
 *      pDpaCapacity      - content DPA capacity.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Applicable:
 *      9310
 * Note:
 *
 * Changes:
 *      None
 */
int32
rtk_dpa_capacity_get(rtk_dpa_capacity_t *pDpaCapacity)
{

#ifdef SOFTWARE_SIMULATE
    pDpaCapacity->teidLearn_table_max = 1000;
    pDpaCapacity->l2Fwd_table_max = 1000;
    pDpaCapacity->ipFwd_table_max = 1000;
    pDpaCapacity->appList_table_max = 1000;
    pDpaCapacity->ue_num_max = 1000;
    pDpaCapacity->meter_max = 800;
    pDpaCapacity->max_port_num = 12;
    pDpaCapacity->portMask_low = 0xfff;
    pDpaCapacity->portMask_high = 0x0;
#else
    RTK_DPA_MODULE_INIT_CHECK(g_dpaModule_initial_status);

    pDpaCapacity->teidLearn_table_max = TEID_LEARN_TABLE_MAX_ENTRY;
    pDpaCapacity->l2Fwd_table_max = IP_FWD_TABLE_MAX_ENTRY;
    pDpaCapacity->ipFwd_table_max = APP_LIST_TABLE_MAX_ENTRY;
    pDpaCapacity->appList_table_max = L2_FWD_TABLE_MAX_ENTRY;
    pDpaCapacity->ue_num_max = DPA_CAPAC_UE_MAX_NUM;
    pDpaCapacity->meter_max = DPA_CAPAC_METER_MAX_NUM;
    pDpaCapacity->max_port_num = DPA_CAPAC_PORT_MAX_NUM;
    pDpaCapacity->portMask_low = DPA_CAPAC_PORT_MASK_LOW;
    pDpaCapacity->portMask_high = DPA_CAPAC_PORT_MASK_HIGH;
#endif //SOFTWARE_SIMMULATE

    return RT_ERR_OK;

}

/* Function Name:
 *      rtk_dpa_ueDownlinkCount_get
 * Description:
 *      Get table entry's counter value.
 * Input:
 *      pMatch          - entry's match field array
 *      num_of_match    - total match field
 * Output:
 *      pPacketCount    - entry packet counter value.
 *      pByteCount      - entry byte counter value.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Applicable:
 *      9310
 * Note:
 *      None
 * Changes:
 *      None
 */
int32
rtk_dpa_ueDownlinkCount_get(rtk_dpa_teid_learn_match_t *pMatch, int num_of_match, uint64_t *pPacketCount, uint64_t *pByteCount)
{

#ifdef SOFTWARE_SIMULATE
    *pPacketCount = 512;
    *pByteCount = 512*1024;
#else
    int                     ret;
    sdn_db_flow_entry_t     *pFlow;
    uint64_t                packet_count = 0;
    uint64_t                byte_count = 0;

    RTK_DPA_MODULE_INIT_CHECK(g_dpaModule_initial_status);

    pFlow = (sdn_db_flow_entry_t *) malloc(sizeof(sdn_db_flow_entry_t));

    if(pFlow == NULL)
    {
        return RT_ERR_FAILED;
    }

    ret = _teid_learn_rule_match_convert(pMatch, num_of_match, &(pFlow->match_field_p));
    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] Failed!\n",__FUNCTION__,__LINE__);
        free(pFlow);
        return RT_ERR_FAILED;
    }
    pFlow->table_id = RTK_DPA_TABLE_ID_TEID_LREAD;
    pFlow->len_match = num_of_match;
    pFlow->priority = TEID_LEARN_ADD_PRIORITY;

    ret = rtk_sdn_get_flow_counter(pFlow->table_id, pFlow->priority, pFlow->len_match, (pFlow->match_field_p), &packet_count, &byte_count);

    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] Get Counter Failed !\n",__FUNCTION__,__LINE__);
        free(pFlow);
        return RT_ERR_FAILED;
    }

    *pPacketCount = packet_count;
    *pByteCount = byte_count;

    free(pFlow);
#endif //SOFTWARE_SIMMULATE

    return RT_ERR_OK;
}


/* Function Name:
 *      rtk_dpa_ueUplinkCount_get
 * Description:
 *      Get table entry's counter value.
 * Input:
 *      pMatch          - entry's match field array
 *      num_of_match    - total match field
 * Output:
 *      pPacketCount    - entry packet counter value.
 *      pByteCount      - entry byte counter value.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Applicable:
 *      9310
 * Note:
 *      None
 * Changes:
 *      None
 */
int32
rtk_dpa_ueUplinkCount_get(rtk_dpa_app_list_match_t *pMatch, int num_of_match, uint64_t *pPacketCount, uint64_t *pByteCount)
{

#ifdef SOFTWARE_SIMULATE
    *pPacketCount = 512;
    *pByteCount = 512*1024;
#else
    int                     ret;
    sdn_db_flow_entry_t     *pFlow;
    uint64_t                packet_count = 0;
    uint64_t                byte_count = 0;

    RTK_DPA_MODULE_INIT_CHECK(g_dpaModule_initial_status);

    pFlow = (sdn_db_flow_entry_t *) malloc(sizeof(sdn_db_flow_entry_t));

    if(pFlow == NULL)
    {
        return RT_ERR_FAILED;
    }

    ret = _app_list_rule_match_convert(pMatch, num_of_match, &(pFlow->match_field_p));
    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] Failed!\n",__FUNCTION__,__LINE__);
        free(pFlow);
        return RT_ERR_FAILED;
    }
    pFlow->table_id = RTK_DPA_TABLE_ID_APP_LIST;
    pFlow->len_match = num_of_match;
    pFlow->priority = APP_LIST_ADD_PRIORITY;

    ret = rtk_sdn_get_flow_counter(pFlow->table_id, pFlow->priority, pFlow->len_match, (pFlow->match_field_p), &packet_count, &byte_count);

    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] Get Counter Failed !\n",__FUNCTION__,__LINE__);
        free(pFlow);
        return RT_ERR_FAILED;
    }

    *pPacketCount = packet_count;
    *pByteCount = byte_count;

    free(pFlow);
#endif //SOFTWARE_SIMMULATE

    return RT_ERR_OK;
}


/* Function Name:
 *      rtk_dpa_tableEntry_hitStatus_clear
 * Description:
 *      Clear table entry's hit status.
 * Input:
 *      tableName       - specific table.
 *      entryIndex      - entry index.
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Applicable:
 *      9310
 * Note:
 *
 * Changes:
 *      None
 */
int32
rtk_dpa_tableEntry_hitStatus_clear(rtk_dpa_tableName_t tableName, int entryIndex)
{
#ifdef SOFTWARE_SIMULATE

    return RT_ERR_OK;
#else
    int         ret = RT_ERR_FAILED;
    uint8_t     table_id = tableName;
    uint32_t    entry_idx = entryIndex;
    uint32_t    hitSts = 0;

    RTK_DPA_MODULE_INIT_CHECK(g_dpaModule_initial_status);


    if(table_id == RTK_DPA_TABLE_NANE_TEID_LEARN)
    {
        ret = sdn_hal_flow_hitstatus_get(table_id, entry_idx, &hitSts);
    }else{
        ret = RT_ERR_INPUT;
    }
    return ret;
#endif //SOFTWARE_SIMMULATE

}


/* Function Name:
 *      rtk_dpa_tableEntry_hitStatus_get
 * Description:
 *      Get table entry's hit status.
 * Input:
 *      tableName       - specific table.
 *      entryIndex      - entry index.
 * Output:
 *      pHitSts         - entry hit result.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Applicable:
 *      9310
 * Note:
 *      Once the Hit status is HIT,
 *      The rtk_dpa_tableEntry_hitStatus_clear() is used to clear entry hit status.
 * Changes:
 *      None
 */
int32
rtk_dpa_tableEntry_hitStatus_get(rtk_dpa_tableName_t tableName, int entryIndex, rtk_dpa_entry_hit_status_t *pHitSts)
{
#ifdef SOFTWARE_SIMULATE
    *pHitSts = RTK_DPA_ENTRY_HIT_STATUS_HIT;
    return RT_ERR_OK;
#else
    int         ret = RT_ERR_FAILED;
    uint8_t     table_id = tableName;
    uint32_t    entry_idx = entryIndex;
    uint32_t    hitSts = 0;

    RTK_DPA_MODULE_INIT_CHECK(g_dpaModule_initial_status);

    DPA_MODULE_MSG("\n[%s][%d]\n",__FUNCTION__,__LINE__);

    if(table_id == RTK_DPA_TABLE_NANE_TEID_LEARN)
    {
        ret = sdn_hal_flow_hitstatus_get(table_id, entry_idx, &hitSts);
        if(ret == SDN_HAL_RETURN_OK)
            ret = RT_ERR_OK;
        if(hitSts == 0)
            *pHitSts = RTK_DPA_ENTRY_HIT_STATUS_MISS;
        else
            *pHitSts = RTK_DPA_ENTRY_HIT_STATUS_HIT;

    }else{
        ret = RT_ERR_INPUT;
    }
    return ret;
#endif //SOFTWARE_SIMMULATE



}

/* Function Name:
 *      rtk_dpa_status_get
 * Description:
 *      Get DPA current status.
 * Input:
 *      None
 * Output:
 *      pDpaStatus    - DPA status
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Applicable:
 *      9310
 * Note:
 *      None
 * Changes:
 *      None
 */
int32
rtk_dpa_status_get(rtk_dpa_status_t *pDpaStatus)
{
#ifdef SOFTWARE_SIMULATE
    *pDpaStatus = RTK_DPA_STATUS_READY;
#else
    if(g_dpaModule_initial_status == RTK_SDN_MODULE_INITIALIZED)
        *pDpaStatus = RTK_DPA_STATUS_READY;
    else
        *pDpaStatus = RTK_DPA_STATUS_INIT_FAILED;
#endif //SOFTWARE_SIMMULATE

    return RT_ERR_OK;
}


/* Function Name:
 *      rtk_dpa_factoryDefault_config
 * Description:
 *      Set the DPA to factory default.
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Applicable:
 *      9310
 * Note:
 *
 * Changes:
 *      None
 */
int32
rtk_dpa_factoryDefault_config(void)
{
    int32   ret;
#ifdef SOFTWARE_SIMULATE
    ret = RT_ERR_OK;
#else
    RTK_DPA_MODULE_INIT_CHECK(g_dpaModule_initial_status);

    /* Need to Remove all active rules */
    ret = RT_ERR_OK;

#endif //SOFTWARE_SIMMULATE

    return ret;

}

/* Function Name:
 *      rtk_dpa_tableMissAction_set
 * Description:
 *      Configure the table miss action.
 * Input:
 *      tableName       - specific table.
 *      tbl_missAct     - table miss action.
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Applicable:
 *      9310
 * Note:
 *      For RTL9310, RTK_DPA_TABLE_NANE_TEID_LEARN and RTK_DPA_TABLE_NANE_L2_FWD
 *      are the same table in chip.
 * Changes:
 *      None
 */
int32
rtk_dpa_tableMissAction_set(rtk_dpa_tableName_t tableName, rtk_dpa_tblMissAct_t tbl_missAct)
{
    int32   ret = RT_ERR_FAILED;
    uint8   table_idx;
    rtk_of_tblMissAct_t of_tbl_missAct;

    switch(tableName){
        case RTK_DPA_TABLE_NANE_TEID_LEARN:
            table_idx = RTK_DPA_TABLE_ID_TEID_LREAD;
            break;
        case RTK_DPA_TABLE_NANE_IP_FWD:
            table_idx = RTK_DPA_TABLE_ID_IP_FWD;
            break;
        case RTK_DPA_TABLE_NANE_APP_LIST:
            table_idx = RTK_DPA_TABLE_ID_APP_LIST;
            break;
        case RTK_DPA_TABLE_NANE_L2_FWD:
            table_idx = RTK_DPA_TABLE_ID_L2_FWD;
            break;
        default:
            DPA_MODULE_MSG("[%s] Table Name Error! \n",__FUNCTION__);
            return ret;
    }

    switch(tbl_missAct){
        case RTK_DPA_TBLMISS_ACT_DROP:
            of_tbl_missAct = OF_TBLMISS_ACT_DROP;
            break;
        case RTK_DPA_TBLMISS_ACT_TRAP:
            of_tbl_missAct = OF_TBLMISS_ACT_TRAP;
            break;
        case RTK_DPA_TBLMISS_ACT_FORWARD_NEXT_TBL:
            of_tbl_missAct = OF_TBLMISS_ACT_FORWARD_NEXT_TBL;
            break;
        case RTK_DPA_TBLMISS_ACT_EXEC_ACTION_SET:
            of_tbl_missAct = OF_TBLMISS_ACT_EXEC_ACTION_SET;
            break;
        default:
            DPA_MODULE_MSG("[%s] Table Miss Action Error! \n",__FUNCTION__);
            return ret;
    }

    ret = rtk_sdn_set_table_miss_action(table_idx, of_tbl_missAct);
    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("[%s] Failed! \n",__FUNCTION__);
    }

    return ret;
}

