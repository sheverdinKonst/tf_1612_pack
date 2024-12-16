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
 */

#include <fcntl.h>
#include "rtcore/rtcore.h"
#include "sys/ioctl.h"
//#include "drv/nic/common.h"
#include "private/drv/nic/nic_common.h"
#include "sdn_hal.h"
#include "common/type.h"
#include "rtk/switch.h"
#include "rtk/acl.h"
#include "rtk/port.h"
#include "ovs.h"
#include "sdn_util.h"
#include <common/util/rt_util.h>
#include "sdn_hal_common.h"

const sdn_hal_port_cfg_t sdn_hal_port_map[] =
{
    {USR_PORT_1 , UNIT_ID_0, PHY_PORT_1},
    {USR_PORT_2 , UNIT_ID_0, PHY_PORT_0},
    {USR_PORT_3 , UNIT_ID_0, PHY_PORT_3},
    {USR_PORT_4 , UNIT_ID_0, PHY_PORT_2},
    {USR_PORT_5 , UNIT_ID_0, PHY_PORT_5},
    {USR_PORT_6 , UNIT_ID_0, PHY_PORT_4},
    {USR_PORT_7 , UNIT_ID_0, PHY_PORT_7},
    {USR_PORT_8 , UNIT_ID_0, PHY_PORT_6},
    {USR_PORT_9 , UNIT_ID_0, PHY_PORT_9},
    {USR_PORT_10, UNIT_ID_0, PHY_PORT_8},
    {USR_PORT_11, UNIT_ID_0, PHY_PORT_11},
    {USR_PORT_12, UNIT_ID_0, PHY_PORT_10},
    {USR_PORT_13, UNIT_ID_0, PHY_PORT_13},
    {USR_PORT_14, UNIT_ID_0, PHY_PORT_12},
    {USR_PORT_15, UNIT_ID_0, PHY_PORT_15},
    {USR_PORT_16, UNIT_ID_0, PHY_PORT_14},
    {USR_PORT_17, UNIT_ID_0, PHY_PORT_17},
    {USR_PORT_18, UNIT_ID_0, PHY_PORT_16},
    {USR_PORT_19, UNIT_ID_0, PHY_PORT_19},
    {USR_PORT_20, UNIT_ID_0, PHY_PORT_18},
    {USR_PORT_21, UNIT_ID_0, PHY_PORT_21},
    {USR_PORT_22, UNIT_ID_0, PHY_PORT_20},
    {USR_PORT_23, UNIT_ID_0, PHY_PORT_23},
    {USR_PORT_24, UNIT_ID_0, PHY_PORT_22},
    {USR_PORT_25, UNIT_ID_0, PHY_PORT_25},
    {USR_PORT_26, UNIT_ID_0, PHY_PORT_24},
    {USR_PORT_27, UNIT_ID_0, PHY_PORT_27},
    {USR_PORT_28, UNIT_ID_0, PHY_PORT_26},
    {USR_PORT_29, UNIT_ID_0, PHY_PORT_29},
    {USR_PORT_30, UNIT_ID_0, PHY_PORT_28},
    {USR_PORT_31, UNIT_ID_0, PHY_PORT_31},
    {USR_PORT_32, UNIT_ID_0, PHY_PORT_30},
    {USR_PORT_33, UNIT_ID_0, PHY_PORT_33},
    {USR_PORT_34, UNIT_ID_0, PHY_PORT_32},
    {USR_PORT_35, UNIT_ID_0, PHY_PORT_35},
    {USR_PORT_36, UNIT_ID_0, PHY_PORT_34},
    {USR_PORT_37, UNIT_ID_0, PHY_PORT_37},
    {USR_PORT_38, UNIT_ID_0, PHY_PORT_36},
    {USR_PORT_39, UNIT_ID_0, PHY_PORT_39},
    {USR_PORT_40, UNIT_ID_0, PHY_PORT_38},
    {USR_PORT_41, UNIT_ID_0, PHY_PORT_41},
    {USR_PORT_42, UNIT_ID_0, PHY_PORT_40},
    {USR_PORT_43, UNIT_ID_0, PHY_PORT_43},
    {USR_PORT_44, UNIT_ID_0, PHY_PORT_42},
    {USR_PORT_45, UNIT_ID_0, PHY_PORT_45},
    {USR_PORT_46, UNIT_ID_0, PHY_PORT_44},
    {USR_PORT_47, UNIT_ID_0, PHY_PORT_47},
    {USR_PORT_48, UNIT_ID_0, PHY_PORT_46},
    {USR_PORT_49, UNIT_ID_0, PHY_PORT_49},
    {USR_PORT_50, UNIT_ID_0, PHY_PORT_48},
    {USR_PORT_51, UNIT_ID_0, PHY_PORT_51},
    {USR_PORT_52, UNIT_ID_0, PHY_PORT_50},
    {USR_PORT_53, UNIT_ID_0, PHY_PORT_52},
   /* Do not remove end entry */
    {PORTCONF_END_VAL, PORTCONF_END_VAL, PORTCONF_END_VAL}
};
/*
 * Data Declaration
 */
sdn_hal_portmask_t all_portMask[RTK_MAX_NUM_OF_UNIT];
uint32 unit_list[RTK_MAX_NUM_OF_UNIT];

static BOOL_T is_inport_existed = FALSE;

#ifdef  CONFIG_SDK_RTL9310
static int32 sdn_hal_ofactionconverttodriver(sdn_hal_apply_actions_t *of_actionbucket_p, rtk_of_actionBucket_t *hal_actionbucket_p);
#endif
static int32 sdn_hal_flow_IsVaild(sdn_db_flow_entry_t *flow_entry_p)
{
    SDN_PARAM_CHK(flow_entry_p == NULL, SDN_HAL_RETURN_FAILED);
    SDN_PARAM_CHK(flow_entry_p->flow_index > SDN_HAL_ACL_ENTRY_END, SDN_HAL_RETURN_FAILED);
    //SDN_PARAM_CHK(((!HAL_LIB_CHK_BIT_ON(flow_entry_p->action_data.action_mask, EN_OFPAT_OUTPUT))
    //                || (_bitmap == OHAL_NULL_ACTION_BTIMAP)) ? TRUE : FALSE, SDN_HAL_RETURN_FAILED);

    return SDN_HAL_RETURN_OK;
}

static int sdn_hal_table_table0_template_bind(uint32 unit)
{
    uint32    shard_template_id  = SDN_HAL_ACL_SHARED_TEMPLATE_ID;
    uint32    ingress_template_id= SDN_HAL_ACL_TABLE0_TEMPLATE_ID;
    uint32    shard_block_id     = 1;
    uint32    ingress_block_id   = 0;
    rtk_acl_templateIdx_t  template_idx_info;

    memset(&template_idx_info, 0, sizeof(template_idx_info));

    template_idx_info.template_id[shard_block_id] = shard_template_id;

    template_idx_info.template_id[ingress_block_id] = ingress_template_id;

//    for (ingress_block_id = OHAL_RULE_ACL_START_BLK_ID_OF_TABLE_0; ingress_block_id < OHAL_RULE_ACL_START_BLK_ID_OF_TABLE_1; ingress_block_id++)
 for (ingress_block_id = SDN_HAL_ACL_START_BLOCK_ID_OF_TABLE_0; ingress_block_id < SDN_HAL_ACL_BLOCK_SIZE; ingress_block_id++)
    {
        if (rtk_acl_templateSelector_set(unit, ingress_block_id, template_idx_info) != RT_ERR_OK)
        {
            //return SDN_HAL_RETURN_FAILED;
            return  -3;
        }
    }

    return SDN_HAL_RETURN_OK;
}

int sdn_hal_init(struct ofp_table_features tf[MAX_TABLE_NUM])
{
//FIXME:not finish
    int ret = SDN_HAL_RETURN_OK;
    uint32 unit = 1;
    uint32 block_id = SDN_HAL_ACL_START_BLOCK_ID_OF_TABLE_0;
    rtk_acl_blockGroup_t group_type = ACL_BLOCK_GROUP_4;
#if 0
    uint32 shard_template_id  = SDN_HAL_ACL_SHARED_TEMPLATE_ID;
    rtk_acl_template_t  shared_template_info;
#endif

    /* 1. Init Chip ACL block,partition,flow table,execution set,group table */
    if ((ret = rtk_acl_blockGroupEnable_set(unit, block_id, group_type, ENABLED)) != RT_ERR_OK)
    {
        return ret;
    }

    if ((ret = rtk_acl_blockPwrEnable_set(unit, block_id, ENABLED)) != RT_ERR_OK)
    {
        return ret;
    }
#if 0
    if ((ret = rtk_acl_blockLookupEnable_set(unit, block_id, ENABLED)) != RT_ERR_OK)
    {
        return ret;
    }
#endif
#if 0
    shared_template_info.field[0]  = TMPLTE_FIELD_SMAC0;
    shared_template_info.field[1]  = TMPLTE_FIELD_SMAC1;
    shared_template_info.field[2]  = TMPLTE_FIELD_SMAC2;
    shared_template_info.field[3]  = TMPLTE_FIELD_DMAC0;
    shared_template_info.field[4]  = TMPLTE_FIELD_DMAC1;
    shared_template_info.field[5]  = TMPLTE_FIELD_DMAC2;
    shared_template_info.field[6]  = TMPLTE_FIELD_OTAG;
    shared_template_info.field[7]  = TMPLTE_FIELD_L4_DPORT;
    shared_template_info.field[8]  = TMPLTE_FIELD_SIP0;
    shared_template_info.field[9]  = TMPLTE_FIELD_SIP1;
    shared_template_info.field[10] = TMPLTE_FIELD_DIP0;
    shared_template_info.field[11] = TMPLTE_FIELD_DIP1;

    if (rtk_acl_template_set(unit, shard_template_id, &shared_template_info) != RT_ERR_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }
#endif

    if (sdn_hal_table_table0_template_bind(unit) != SDN_HAL_RETURN_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    /* 2. Adjust reserved ACL */

    return ret;
}

/* Table Feature */
int sdn_hal_table_features_set(uint8 n_table, const struct ofp_table_features *tf)
{
    int ret = SDN_HAL_RETURN_OK;

    /* 1. Apply new features to chip */
    /* TODO:Need to implement */

    return ret;
}

/* Tx packet
    OF_OUTPUT_TYPE_NONE,                 disable output action
    OF_OUTPUT_TYPE_PHY_PORT_EX_SRC_PORT, physical port and excludes source port
    OF_OUTPUT_TYPE_PHY_PORT,             physical port
    OF_OUTPUT_TYPE_TRK_PORT_EX_SRC_PORT, trunk port and excludes source port
    OF_OUTPUT_TYPE_TRK_PORT,             trunk port
    OF_OUTPUT_TYPE_MULTI_EGR_PORT,       multiple egress ports and excludes source port
    OF_OUTPUT_TYPE_IN_PORT,              packet's ingress port
    OF_OUTPUT_TYPE_FLOOD,                FLOOD reserved port(all ports with VLAN, trunk, spanning tree filter and excludes source port)
    OF_OUTPUT_TYPE_LB,                   NORMAL reserved port(loopback to traditional pipeline for forwarding)
    OF_OUTPUT_TYPE_TUNNEL,               tunnel interface
    OF_OUTPUT_TYPE_FAILOVER,             failover to the first link up port
*/
int sdn_hal_pkt_tx(char *dev_name,
                  void *pkt_p,
	              uint32 pkt_size,
                  rtk_of_outputType_t *out_put_type,
                  uint32 port_no)
{
   // int32 fd;
   // rtcore_ioctl_t dio;
    drv_nic_pkt_t *pPacket = NULL;
    rtk_of_outputType_t output_type = *out_put_type;
    void *pkt_pl_p = NULL;

    printf("\r\n[%s:%d] devname(%s)", __FUNCTION__, __LINE__, dev_name);
    if (pkt_p == NULL)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    pPacket = (drv_nic_pkt_t *)malloc(sizeof(drv_nic_pkt_t));

    if (pPacket == NULL)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    memset(pPacket, 0, sizeof(drv_nic_pkt_t));


    pkt_pl_p = malloc(pkt_size);

    if (pkt_pl_p)
    {
        free(pPacket);
        return SDN_HAL_RETURN_FAILED;
    }

    memcpy(pkt_pl_p, pkt_p, pkt_size);
    pPacket->length = pkt_size;
    pPacket->head   = (uint8 *)pkt_pl_p;
    pPacket->tail   = pPacket->data + pPacket->length;
    pPacket->end    = pPacket->tail;
    if (OF_OUTPUT_TYPE_PHY_PORT == output_type)
    {
        pPacket->tx_tag.dst_port_mask = (0x00000001 << port_no);
    }

//#ifdef __linux__
//    if ((fd = open(RTCORE_DEV_NAME, O_RDWR)) < 0)
//    {
//        return SDN_HAL_RETURN_FAILED;
//    }
//#endif

    //memset(&dio, 0, sizeof(dio));

    //dio.data[0] = 0;

    /* If BP_FLTR_2 is 0b1, then bypass the following checking:
     (1) Egress spanning-tree port state in blocking/listening or learning
     (2)Egress VLAN filtering
     */
    /* herro added
     tx priority
     */
    pPacket->tx_tag.as_priority = 1;
    pPacket->tx_tag.priority    = 6;
#if defined(CONFIG_SDK_RTL8380) || defined(CONFIG_SDK_RTL8390)
    pPacket->tx_tag.bp_fltr1 = 1;
    pPacket->tx_tag.bp_fltr2 = 1;
#endif

#if defined(CONFIG_SDK_RTL9300) || defined(CONFIG_SDK_RTL9310)
    pPacket->tx_tag.bp_fltr = 1;
#endif

//#ifdef __APPLE__
//    dio.data[1] = (unsigned long)pPacket;
//#elif __linux__
//    dio.data[1] = (uint32)pPacket;
//    ioctl(fd, RTCORE_NIC_PKT_TX, &dio);
//#endif

#ifdef __linux__
//    close(fd);

    if(RT_ERR_OK != drv_nic_pkt_tx_toKernel(0, pPacket))
    {
        return SDN_HAL_RETURN_FAILED;
    }
#endif

//    if (dio.ret)
//    {
//        return SDN_HAL_RETURN_FAILED;
//    }


    return SDN_HAL_RETURN_OK;
}

#ifdef  CONFIG_SDK_RTL9310
/*
 rtk_enable_t 				    cp_ttl_inward;

 rtk_enable_t 				    pop_vlan;

 rtk_enable_t 				    pop_mpls;
 rtk_of_actBucketPopMplsType_t   pop_mpls_type;

 rtk_enable_t 				    push_mpls;
 rtk_of_actBucketPushMpls_t	    push_mpls_data;

 rtk_enable_t 				    push_vlan;
 rtk_of_actBucketPushVlan_t	    push_vlan_data;

 rtk_enable_t 				    cp_ttl_outward;

 rtk_enable_t 				    dec_mpls_ttl;

 rtk_enable_t 				    dec_ip_ttl;

 rtk_of_actBucketSF0_t	        set_field_0_data;
 rtk_of_actBucketSF1_t	        set_field_1_data;
 rtk_of_actBucketSF2_t	        set_field_2_data;

 rtk_enable_t 				    set_queue;
 uint32						    qid;

 rtk_of_actBucketOutput_t	    output_data;

	EN_OFPAT_OUTPUT			= 0,
	EN_OFPAT_COPY_TTL_OUT		= 11,
	EN_OFPAT_COPY_TTL_IN		= 12,
	EN_OFPAT_SET_MPLS_TTL		= 15,
	EN_OFPAT_DEC_MPLS_TTL		= 16,
	EN_OFPAT_PUSH_VLAN		= 17,
	EN_OFPAT_POP_VLAN		= 18,
	EN_OFPAT_PUSH_MPLS		= 19,
	EN_OFPAT_POP_MPLS		= 20,
	EN_OFPAT_SET_QUEUE		= 21,
	EN_OFPAT_GROUP			= 22,
	EN_OFPAT_SET_NW_TTL		= 23,
	EN_OFPAT_DEC_NW_TTL		= 24,
	EN_OFPAT_SET_FIELD		= 25,
	EN_OFPAT_PUSH_PBB		= 26,
	EN_OFPAT_POP_PBB		= 27,
*/

static int32 sdn_hal_ofactionconverttodriver(sdn_hal_apply_actions_t *of_actionbucket_p, rtk_of_actionBucket_t *hal_actionbucket_p)
{
    uint32 action_id   = 0;
    uint32 action_type = 0xff;
    uint32 oxm_type    = 0xff;
    uint32 total_out_ports = 0;

    if (of_actionbucket_p == NULL
        || hal_actionbucket_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    for (action_id = 0; action_id < of_actionbucket_p->action_len; action_id++)
    {
        action_type = of_actionbucket_p->action_p[action_id].act_type;
        switch(action_type)
        {
            case EN_OFPAT_PUSH_VLAN:
                hal_actionbucket_p->push_vlan                    = ENABLED;
                hal_actionbucket_p->push_vlan_data.etherType_idx = (uint32)of_actionbucket_p->action_p[action_id].data.low;
                break;
            case EN_OFPAT_COPY_TTL_IN:
                hal_actionbucket_p->cp_ttl_inward = ENABLED;
                break;
            case EN_OFPAT_COPY_TTL_OUT:
                hal_actionbucket_p->cp_ttl_outward = ENABLED;
                break;
            case EN_OFPAT_SET_QUEUE:
                hal_actionbucket_p->set_queue = ENABLED;
                hal_actionbucket_p->qid       = (uint32)of_actionbucket_p->action_p[action_id].data.low;
                break;
            case EN_OFPAT_DEC_MPLS_TTL:
                hal_actionbucket_p->dec_mpls_ttl = ENABLED;
                break;
            case EN_OFPAT_PUSH_MPLS:
                hal_actionbucket_p->push_mpls    = ENABLED;
                hal_actionbucket_p->push_mpls_data.etherType = (uint32)of_actionbucket_p->action_p[action_id].data.low;
                break;
            case EN_OFPAT_OUTPUT:
            {
                if (total_out_ports == 0)
                {
                    hal_actionbucket_p->output_data.port = (uint32)of_actionbucket_p->action_p[action_id].data.low;
                }
                else
                {

                }
                total_out_ports++;
            }
                break;
            case EN_OFPAT_SET_FIELD:
            {
                oxm_type = of_actionbucket_p->action_p[action_id].oxm_type;
                switch (oxm_type)
                {
                    case EN_OFPXMT_OFB_IN_PORT:
                    default:
                        ;
                }
                break;
            }
            default:
                ;
        }
    }
    return SDN_HAL_RETURN_OK;
}

int32 sdn_hal_add_group(uint32 group_id, rtk_of_groupType_t type, sdn_hal_group_table_t *entry_p)
{
    uint32 entry_id = 0;
    rtk_of_groupEntry_t group_entry;
    rtk_of_actionBucket_t drv_action_bucket_entry;
    sdn_hal_apply_actions_t  *action_bucket_p = NULL;
    //sdn_db_apply_actions_t *action_bucket_p = NULL;

    if (entry_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    entry_id = group_id;
    memset(&group_entry, 0, sizeof(group_entry));
    memset(&drv_action_bucket_entry, 0, sizeof(drv_action_bucket_entry));

    action_bucket_p = entry_p->action_buket_p;

    group_entry.bucket_id  = action_bucket_p->bucket_id;
    group_entry.type       = type;
    group_entry.bucket_num = entry_p->action_buket_num;

#ifdef RTK_SDK_OK
    if (rtk_of_groupEntry_set(0, entry_id, &group_entry) != RT_ERR_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    if (sdn_hal_ofactionconverttodriver(action_bucket_p, &drv_action_bucket_entry) != RT_ERR_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }
#endif

    return SDN_HAL_RETURN_OK;
}

int32 sdn_hal_get_group(uint32 group_id, rtk_of_groupType_t *type, uint32 *bucket_num_p, sdn_hal_apply_actions_t **applied_action_list_p)
{
    if (bucket_num_p == NULL
        ||applied_action_list_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    return SDN_HAL_RETURN_OK;
}

int32 sdn_hal_delete_group(uint32 group_id)
{
    rtk_of_groupEntry_t group_entry;

    memset(&group_entry, 0, sizeof(group_entry));
#ifdef RTK_SDK_OK
    if (rtk_of_groupEntry_set(0, group_id, &group_entry) != RT_ERR_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }
#endif
    return SDN_HAL_RETURN_OK;
}

/* Classifier */
int sdn_hal_classifier_set(rtk_of_classifierType_t type, rtk_of_classifierData_t data)
{
//FIXME:not finish
    int ret = SDN_HAL_RETURN_OK;
    int unit = 1;

    /* 1. Apply counter to chip */
    ret = rtk_of_classifier_set(unit, type, data);

    return ret;
}
#endif

int32 sdn_hal_phyPort2LogicalPort(uint32 unit, uint32 phy_port, uint32 *logical_port_p)
{
    uint32 port_index = 0;

    if (logical_port_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    if ((phy_port < PHY_PORT_1) ||
        (phy_port > PHY_PORT_53))
    {
        return SDN_HAL_RETURN_ERR_INPUT;
    }

    for(port_index = 0; port_index < _countof(sdn_hal_port_map); port_index++)
    {
        if (sdn_hal_port_map[port_index].devId != unit)
            continue;

        if (sdn_hal_port_map[port_index].phyPort == phy_port)
        {
            *logical_port_p = port_index;
            return SDN_HAL_RETURN_OK;
        }
    }

    return SDN_HAL_RETURN_FAILED;
}

int32 sdn_hal_logicalPort2PhyPort(uint32 logical_port, uint32 *phy_port_p)
{
    int32 i = 0;
    if (phy_port_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    if ((logical_port < 0))
    {
        return SDN_HAL_RETURN_ERR_INPUT;
    }

    for(i = 0; i < _countof(sdn_hal_port_map); i++)
    {
        if ((sdn_hal_port_map[i].usrPort == logical_port))
        {
            *phy_port_p = sdn_hal_port_map[i].phyPort;
            return SDN_HAL_RETURN_OK;
        }
    }

    return SDN_HAL_RETURN_FAILED;
}

int32 sdn_hal_phyPort2UserPort(uint32 unit, uint32 phy_port, uint32 *user_port_p)
{
    uint32 port_index = 0;

    if (user_port_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    if (phy_port > RTK_MAX_PORT_PER_UNIT)
    {
        return SDN_HAL_RETURN_ERR_INPUT;
    }

    for(port_index = 0; port_index < _countof(sdn_hal_port_map); port_index++)
    {
        if (sdn_hal_port_map[port_index].devId != unit)
            continue;

        if (sdn_hal_port_map[port_index].phyPort == phy_port)
        {
            *user_port_p = sdn_hal_port_map[port_index].usrPort;
            return SDN_HAL_RETURN_OK;
        }
    }

    return SDN_HAL_RETURN_FAILED;
}

int32 sdn_hal_userPort2PhyPort(uint32 user_port, uint32 *phy_port_p)
{
    int32 i = 0;
    if (phy_port_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    if ((user_port < 0))
    {
        return SDN_HAL_RETURN_ERR_INPUT;
    }

    for(i = 0; i < _countof(sdn_hal_port_map); i++)
    {
        if ((sdn_hal_port_map[i].usrPort == user_port))
        {
            *phy_port_p = sdn_hal_port_map[i].phyPort;
            return SDN_HAL_RETURN_OK;
        }
    }

    return SDN_HAL_RETURN_FAILED;

}

int32 sdn_hal_logicalPort2UserPort(uint32 unit, uint32 logical_port, uint32 *user_port_p)
{
    if (user_port_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    if ((logical_port < 0) ||
        (logical_port > _countof(sdn_hal_port_map)))
    {
        return SDN_HAL_RETURN_ERR_INPUT;
    }

    if (sdn_hal_port_map[logical_port].devId != unit)
        return SDN_HAL_RETURN_FAILED;

    if (sdn_hal_port_map[logical_port].usrPort != PORTCONF_END_VAL)
    {
            *user_port_p = sdn_hal_port_map[logical_port].usrPort;
            return SDN_HAL_RETURN_OK;
    }

    return SDN_HAL_RETURN_FAILED;
}

int32 sdn_hal_userPort2LogicalPort(uint32 unit, uint32 user_port, uint32 *logical_port_p)
{
    uint32 port_index = 0;

    if (logical_port_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    if ((user_port < USR_PORT_1) ||
        (user_port > USR_PORT_53))
    {
        return SDN_HAL_RETURN_ERR_INPUT;
    }

    for(port_index = 0; port_index < _countof(sdn_hal_port_map); port_index++)
    {
        if (sdn_hal_port_map[port_index].devId != unit)
            continue;

        if (sdn_hal_port_map[port_index].usrPort == user_port)
        {
            *logical_port_p = port_index;
            return SDN_HAL_RETURN_OK;
        }
    }

    return SDN_HAL_RETURN_FAILED;
}

int32 sdn_hal_logicalPortList2PhyPortList(uint32 unit, rtk_portmask_t *lport_list_p, rtk_portmask_t *phyport_list_p)
{
    uint32 port_id = 0;
    uint32 phy_port_id = 0;

    SDN_PARAM_CHK(lport_list_p == NULL, SDN_HAL_RETURN_NULL_DATA);
    SDN_PARAM_CHK(phyport_list_p == NULL, SDN_HAL_RETURN_NULL_DATA);

    for(port_id = USR_PORT_1; port_id <= USR_PORT_54; port_id++)
    {
        if(PORTMASK_IS_PORT_SET(*lport_list_p, port_id))
        {
            if(SDN_HAL_RETURN_OK != sdn_hal_logicalPort2PhyPort(port_id, &phy_port_id))
            {
                return SDN_HAL_RETURN_FAILED;
            }

            PORTMASK_PORT_SET(*phyport_list_p, phy_port_id);
        }
        else
        {
            if(SDN_HAL_RETURN_OK != sdn_hal_logicalPort2PhyPort(port_id, &phy_port_id))
            {
                return SDN_HAL_RETURN_FAILED;
            }

            PORTMASK_PORT_CLEAR(*phyport_list_p, phy_port_id);
        }
    }

    return SDN_HAL_RETURN_FAILED;

}


/* Counter */
int32 sdn_hal_flow_entry_counter_apply(uint8 table_id,
                                     uint32 flow_id,
                                     uint32 count_id,
                                     uint32_t counter_flag)
{
    /* 1. Apply counter to chip */

#if 1
    /*Brandon:
        1.need to convert table_id to phase.
        2. flow_id convert to physical entry_idx.
        3. count_id doesn't be used by sdk  ,  why not  ????!!!
    */
    int ret = SDN_HAL_RETURN_OK;
    int unit = 0;

    CHECK_TABLE_ID_IS_VALID(table_id);

    if(( ret = rtk_of_flowCntMode_set(unit, (rtk_of_flowtable_phase_t) table_id, (rtk_of_flow_id_t) flow_id, OF_FLOW_CNTMODE_PACKET_CNT_AND_BYTE_CNT)) != 0)
    {
        printf("[%s][%d] fail to get flow count mode, ret=[%d]. \n", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    return ret;

#endif

}

int sdn_hal_flow_entry_counter_delete(uint8 table_id,
                                      uint32 flow_id,
                                      uint32 count_id)
{
    /* 1. Delete counter to chip */

    /*Brandon: don't know which sdk API. */
    /*full match flow table alway apply counter, if del binded counter id shall delete flow db only. */

}

int sdn_hal_flow_entry_counter_clear(uint8 table_id,
                                     uint32 flow_id,
                                     uint32 count_id)
{
    /* 1. Clear counter to chip */

#if 1
    int ret = SDN_HAL_RETURN_OK;
    int unit = 0;
    rtk_of_flowCntMode_t  cnt_mode;
    /*Brandon: counter type need transfer from upper layer.*/

   CHECK_TABLE_ID_IS_VALID(table_id);

    ret = rtk_of_flowCntMode_get(unit, (rtk_of_flowtable_phase_t) table_id, (rtk_of_flow_id_t) flow_id, &cnt_mode);

    if (ret != RT_ERR_OK)
    {
        printf("[%s][%d] fail to get flow count mode, ret=[%d]. \n", __FUNCTION__, __LINE__, ret);
        return ret;
    }


    switch (cnt_mode)
    {
        case OF_FLOW_CNTMODE_PACKET_CNT_AND_BYTE_CNT:
            if(( ret = rtk_of_flowCnt_clear(unit , (rtk_of_flowtable_phase_t) table_id, (rtk_of_flow_id_t)  flow_id,
                                            OF_FLOW_CNT_TYPE_PACKET) ) != RT_ERR_OK)
            {
                break;
            }

            if(( ret = rtk_of_flowCnt_clear(unit , (rtk_of_flowtable_phase_t) table_id, (rtk_of_flow_id_t)  flow_id,
                                            OF_FLOW_CNT_TYPE_BYTE) ) != RT_ERR_OK)
            {
                break;
            }
            break;

        case  OF_FLOW_CNTMODE_PACKET_CNT_AND_PACKET_CNT_TH:
            if(( ret = rtk_of_flowCnt_clear(unit , (rtk_of_flowtable_phase_t) table_id, (rtk_of_flow_id_t)  flow_id,
                                            OF_FLOW_CNT_TYPE_PACKET) ) != RT_ERR_OK)
            {
                break;
            }
            break;
        case OF_FLOW_CNTMODE_BYTE_CNT_AND_BYTE_CNT_TH:
            if((ret = rtk_of_flowCnt_clear(unit , (rtk_of_flowtable_phase_t) table_id, (rtk_of_flow_id_t)  flow_id,
                                         OF_FLOW_CNT_TYPE_BYTE)) != RT_ERR_OK)
            {
                break;
            }
            break;
        default:
            break;
    }



    if (ret != RT_ERR_OK)
    {
        printf("[%s][%d] fail to get flow count , ret=[%d]. \n", __FUNCTION__, __LINE__, ret);
        return SDN_HAL_RETURN_FAILED;
    }

    return SDN_HAL_RETURN_OK;
#endif

}

int sdn_hal_flow_table_stats_get(uint8 table_id, rtk_of_flowTblCntType_t type, uint32 *pCnt)
{
    /* 1. Get counter of flow table from chip */

#if 1  /* Brandon add: */

    int ret = 0;
    int unit = 0;

    if (pCnt == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    CHECK_TABLE_ID_IS_VALID(table_id);

    if (( ret = rtk_of_flowTblCnt_get(unit, (rtk_of_flowtable_phase_t) table_id, type, pCnt)) != 0)
    {
        printf("[%s][%d] fail to get flow count mode, ret=[%d]. \n", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    return ret;

#endif

}

#if 1   //Brandon modify
int sdn_hal_flow_stats_get(uint8 table_id,
    uint16 priority,
    uint8 match_num,
    ofp_match *match,
    uint64_t *packet_count,
    uint64_t *byte_count)
{
    int unit = 0;
    rtk_of_flowCntMode_t  counter_mode = OF_FLOW_CNTMODE_END;
    /* 1. Get flow counter from chip */
    /*
    1. table_id convert to phase
    2. according to match, convert match to flow_id. (maybe refer to sdn_db)

    int32 rtk_of_flowCnt_get(uint32 unit, rtk_of_flowtable_phase_t phase, rtk_of_flow_id_t  entry_idx,
                                                rtk_of_flowCntType_t type, uint64 *pCnt);
    */

    if (match == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    CHECK_TABLE_ID_IS_VALID(table_id);

    if (RT_ERR_OK != rtk_of_flowCntMode_get(unit, (rtk_of_flowtable_phase_t) table_id, (rtk_of_flow_id_t) priority, &counter_mode))
    {
        return RT_ERR_FAILED;
    }

    switch(counter_mode)
    {
        case OF_FLOW_CNTMODE_PACKET_CNT_AND_BYTE_CNT:
        break;

        case OF_FLOW_CNTMODE_PACKET_CNT_AND_PACKET_CNT_TH:
        break;

        case OF_FLOW_CNTMODE_BYTE_CNT_AND_BYTE_CNT_TH:
        break;

        OF_FLOW_CNTMODE_DISABLE:
        default:
             *packet_count = 0;
             *byte_count   = 0;
        ;
    }
}

#else
int sdn_hal_flow_counter_get(uint8 table_id,
    uint16 priority,
    uint8 match_num,
    ofp_match *match,
    uint64_t *packet_count,
    uint64_t *byte_count)
{
    /* 1. Get flow counter from chip */
}

#endif

#if 1  //Brandon add:


int sdn_hal_flow_stats_periodic_get(uint8 table_id,
    uint32 flow_id,
    uint64_t *packet_count,
    uint64_t *byte_count)
{

    int ret = -1;
    int unit = 0;
    int i = 0;
    rtk_of_flowCntMode_t cnt_mode = OF_FLOW_CNTMODE_DISABLE;
    uint64 count = ~0;

    if (packet_count == NULL)
        return RT_ERR_NULL_POINTER;

    if (byte_count == NULL)
        return RT_ERR_NULL_POINTER;


    /* 1. Get flow counter from chip */

    /*
    1. table_id convert to phase
    2. flow_id convert to entry_idx
    */

/*
    ret = rtk_of_flowCntMode_get(unit, (rtk_of_flowtable_phase_t) table_id, (rtk_of_flow_id_t) flow_id, &cnt_mode);

    if (ret != 0)
    {
        printf("[%s][%d] fail to get flow count mode, ret=[%d]. \n", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    switch (cnt_mode)
    {
        case OF_FLOW_CNTMODE_PACKET_CNT_AND_BYTE_CNT:
            if(( ret = rtk_of_flowCnt_get(unit , (rtk_of_flowtable_phase_t) table_id, (rtk_of_flow_id_t)  flow_id,
                                          OF_FLOW_CNT_TYPE_PACKET, &count) ) != 0)
                break;
            *packet_count = count;
            count = ~0;
            if((ret = rtk_of_flowCnt_get(unit , (rtk_of_flowtable_phase_t) table_id, (rtk_of_flow_id_t)  flow_id,
                                         OF_FLOW_CNT_TYPE_BYTE, &count)) != 0)
                break;
            *byte_count = count;
            break;

        case  OF_FLOW_CNTMODE_PACKET_CNT_AND_PACKET_CNT_TH:
            if(( ret = rtk_of_flowCnt_get(unit , (rtk_of_flowtable_phase_t) table_id, (rtk_of_flow_id_t)  flow_id,
                                          OF_FLOW_CNT_TYPE_PACKET, &count)) != 0)
                break;
            *packet_count = count;
            break;

        case OF_FLOW_CNTMODE_BYTE_CNT_AND_BYTE_CNT_TH:
            if(( ret = rtk_of_flowCnt_get(unit , (rtk_of_flowtable_phase_t) table_id, (rtk_of_flow_id_t)  flow_id,
                                          OF_FLOW_CNT_TYPE_BYTE, &count)) != 0)
                break;
            *byte_count = count;
            break;
        default:
            break;
    }
*/

    if (ret != 0)
    {
        printf("[%s][%d] fail to get flow count , ret=[%d]. \n", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    return 0;
}

int sdn_hal_port_stats_get(uint32 port_no, hal_port_stats_t *pPort_stats_data )
{
    int ret = -1;
    int unit = 0;
    rtk_stat_port_cntr_t port_counter;
    uint32 phy_port;

    sdn_hal_usrPort_unit_get(port_no, &unit);

    if(SDN_HAL_RETURN_OK != (ret = sdn_hal_userPort2PhyPort(port_no, &phy_port)))
    {
        return ret;
    }

    if (pPort_stats_data == NULL)
        return RT_ERR_NULL_POINTER;

    memset(&port_counter, 0x0, sizeof(rtk_stat_port_cntr_t));

    if ((ret = rtk_stat_port_getAll(unit, phy_port, &port_counter)) == 0)
    {
        /* OHAL_COUNTER_PORT_TYPE_PACKETS_RECEIVED */
        pPort_stats_data->rx_packets = port_counter.ifInUcastPkts;

        /* OHAL_COUNTER_PORT_TYPE_PACKETS_TRANSMITTED */
        pPort_stats_data->tx_packets = port_counter.ifOutUcastPkts;

        /* OHAL_COUNTER_PORT_TYPE_BYTES_RECEIVED */
        pPort_stats_data->rx_bytes	= port_counter.ifInOctets;

        /* OHAL_COUNTER_PORT_TYPE_BYTES_TRANSMITTED */
        pPort_stats_data->tx_bytes	= port_counter.ifOutOctets;

        /* OHAL_COUNTER_PORT_TYPE_DROPS_RECEIVED */
        pPort_stats_data->rx_drops	= port_counter.dot1dTpPortInDiscards;

        /* OHAL_COUNTER_PORT_TYPE_DROP_TRANSMITTED */
        pPort_stats_data->tx_drops	= 0;				/* TODO: ?? */

        /* OHAL_COUNTER_PORT_TYPE_ERRORS_RECEIVED */
        pPort_stats_data->rx_errors	 = port_counter.etherStatsRxUndersizePkts;
        pPort_stats_data->rx_errors	+= port_counter.etherStatsRxOversizePkts;
        pPort_stats_data->rx_errors	+= port_counter.etherStatsDropEvents;
        pPort_stats_data->rx_errors	+= port_counter.etherStatsCRCAlignErrors;
        pPort_stats_data->rx_errors	+= port_counter.dot1dTpPortInDiscards;

        /* OHAL_COUNTER_PORT_TYPE_ERRORS_TRANSMITTED */
        pPort_stats_data->tx_errors	 = port_counter.ifOutDiscards;
        pPort_stats_data->tx_errors	+= port_counter.dot3StatsExcessiveCollisions;

        #if defined(CONFIG_SDK_RTL8390) || defined(CONFIG_SDK_RTL9300)||defined(CONFIG_SDK_RTL9310)
        pPort_stats_data->tx_errors	+= port_counter.txEtherStatsJabbers;
        #endif

        pPort_stats_data->tx_errors	+= port_counter.dot3StatsLateCollisions;
        pPort_stats_data->tx_errors	+= port_counter.dot3StatsDeferredTransmissions;

        /* OHAL_COUNTER_PORT_TYPE_FRAME_ALIGNMENT_ERRORS */
        pPort_stats_data->rx_frame_align_err	= port_counter.etherStatsCRCAlignErrors;

        /* OHAL_COUNTER_PORT_TYPE_OVERRUN_ERRORS */
        pPort_stats_data->rx_overrun_errors = 0;			/* TBD */

        /* OHAL_COUNTER_PORT_TYPE_CRC_ERRORS */
        pPort_stats_data->rx_crc_errors	= port_counter.etherStatsCRCAlignErrors;

        /* OHAL_COUNTER_PORT_TYPE_COLLISIONS */
        pPort_stats_data->collisions	= port_counter.dot3StatsLateCollisions;

        /*Brandon comment: refer to  A.3.5.6 Port Statistics.  ofproto.c-->append_port_stat( )
                                            duration(msec)=systime-port_created_time   Ex. 300123 msec    , precision= msec
                                            So,
                                            duration_secs  = 300456 msec/1000                      =300 sec
                                            duration_nsecs =(300456 msec%1000)*1000*1000=456000000 nsec

                                            duration expression= 300.456000000 sec
        */

        /* OHAL_COUNTER_PORT_TYPE_DURATION_SECS */
        /* TBD: using system time to instead.  */

        /* OHAL_COUNTER_PORT_TYPE_DUATION_NSECS */
        /* TBD: using system time to instead */

        printf("\n[%s-%d] logic_port(%d) phy_port(%d)   \
                                        rx_packets(%d) tx_packets(%d) rx_bytes(%d) \
                                        tx_btye(%d) rx_frame_align(%d) rx_over_run(%d) rx_crc_error(%d)",
                __FUNCTION__,
                __LINE__,
                port_no,
                phy_port,
                pPort_stats_data->rx_packets,
                pPort_stats_data->tx_packets,
                pPort_stats_data->rx_bytes,
                pPort_stats_data->tx_bytes,
                pPort_stats_data->rx_frame_align_err,
                pPort_stats_data->rx_overrun_errors,
                pPort_stats_data->rx_crc_errors
               );
    }


    return ret;
}

#endif

int32 sdn_hal_usrPort_unit_get(uint32 port_no, uint32_t *unit)
{
    uint32 port_index = 0;

    if ((port_no < USR_PORT_1) ||
        (port_no > USR_PORT_END))
    {
        return SDN_HAL_RETURN_ERR_INPUT;
    }

    for(port_index = 0; port_index < _countof(sdn_hal_port_map); port_index++)
    {
        if (sdn_hal_port_map[port_index].usrPort == port_no)
        {
            *unit = sdn_hal_port_map[port_index].devId;
            return SDN_HAL_RETURN_OK;
        }
    }

    return SDN_HAL_RETURN_FAILED;
}
int32 sdn_hal_portLinkChange_get(sdn_hal_port_linkChange_t *change)
{
    rtk_port_linkChange_t rtk_linkChange;
    uint32_t unit;

    memset(&rtk_linkChange, 0, sizeof(rtk_port_linkChange_t));
    rtk_ovs_portLinkChange_get(&rtk_linkChange);

    FOR_EACH_UNIT(unit)
    {
        memcpy(&change->portmask[unit], &rtk_linkChange.portmask[unit], sizeof(rtk_portmask_t));
    }

    return SDN_HAL_RETURN_OK;
}
int32 sdn_hal_port_state_get(uint32_t port_no, uint32_t *port_state)
{
    uint32_t unit;
    uint32_t ret = 0;
    uint32_t phy_port = 0;
    uint32_t link_status;

    sdn_hal_usrPort_unit_get(port_no, &unit);

    if(SDN_HAL_RETURN_OK != (ret = sdn_hal_userPort2PhyPort(port_no, &phy_port)))
    {
        return ret;
    }

    ret = rtk_port_link_get(unit, phy_port, &link_status);
    if (RT_ERR_OK != ret)
    {
        return SDN_HAL_RETURN_FAILED;
    }
    switch(link_status)
    {
        case PORT_LINKUP:
            *port_state = 0;
        case PORT_LINKDOWN:
            *port_state = EN_OFPPS_LINK_DOWN;
        default:
            return SDN_HAL_RETURN_FAILED;
    }
    return SDN_HAL_RETURN_OK;
}

int32 sdn_hal_port_max_get(void)
{
    uint32 maxPort = 0;
    uint32 i = 0;

    for(i = 0; i < _countof(sdn_hal_port_map); i++)
    {
        if ((sdn_hal_port_map[i].usrPort > maxPort) && (sdn_hal_port_map[i].usrPort != PORTCONF_END_VAL))
            maxPort = sdn_hal_port_map[i].usrPort;
    }

    return maxPort;
}

int32 sdn_hal_port_feature_get(uint32 port_no, sdn_db_port_feature_t *pFeature)
{
    uint32 phy_port = 0;
    uint32 unit = 0;
    uint32 ret = SDN_HAL_RETURN_FAILED;
    rtk_port_speed_t port_speed = 0;
    rtk_port_duplex_t port_duplex = 0;
    rtk_port_media_t link_media = 0;
    rtk_port_linkStatus_t status = 0;
    sdn_db_port_feature_t port_feature;
    rtk_port_phy_ability_t port_ability;

    SDN_PARAM_CHK(!sdn_hal_port_exist(port_no), SDN_HAL_RETURN_FAILED);

    sdn_hal_usrPort_unit_get(port_no, &unit);

    osal_memset(&port_feature, 0, sizeof(sdn_db_port_feature_t));

    if(SDN_HAL_RETURN_OK != (ret = sdn_hal_userPort2PhyPort(port_no, &phy_port)))
    {
        return ret;
    }

    if(RT_ERR_OK != (ret = rtk_port_speedDuplex_get(unit, phy_port, &port_speed, &port_duplex)))
    {
        return ret;
    }

    if(RT_ERR_OK != (ret = rtk_port_linkMedia_get(unit, phy_port, &status, &link_media)))
    {
        return ret;
    }

    if(RT_ERR_OK != (ret = rtk_port_phyAutoNegoAbility_get(unit, phy_port, &port_ability)))
    {
        return ret;
    }
    if (status == PORT_LINKUP)
    {
        switch (port_speed)
        {
            case PORT_SPEED_10M:
                port_feature.curr |= ((port_duplex == PORT_HALF_DUPLEX)?(EN_OFPPF_10MB_HD):(EN_OFPPF_10MB_FD));
                break;
            case PORT_SPEED_100M:
                port_feature.curr |= ((port_duplex == PORT_HALF_DUPLEX)?(EN_OFPPF_100MB_HD):(EN_OFPPF_100MB_FD));
                break;
            case PORT_SPEED_1000M:
                port_feature.curr |= ((port_duplex == PORT_HALF_DUPLEX)?(EN_OFPPF_1GB_HD):(EN_OFPPF_1GB_FD));
                break;
            case PORT_SPEED_10G:
                port_feature.curr |= EN_OFPPF_10GB_FD;
                break;
            case PORT_SPEED_500M:
            case PORT_SPEED_2G:
            case PORT_SPEED_2_5G:
            case PORT_SPEED_5G:
                port_feature.curr |= EN_OFPPF_OTHER;
                break;
            default:
                return SDN_HAL_RETURN_FAILED;
        }

        switch (link_media)
        {
            case PORT_MEDIA_COPPER:
                port_feature.curr |= EN_OFPPF_COPPER;
                break;
            case PORT_MEDIA_FIBER:
                port_feature.curr |= EN_OFPPF_FIBER;
                break;
            default:
                return SDN_HAL_RETURN_FAILED;
        }
    }
    else
    {
        port_feature.curr = 0;
    }

    port_feature.curr |= (EN_OFPPF_COPPER);
    port_feature.supported |= (port_ability.Half_10 ? (EN_OFPPF_10MB_HD):0);
    port_feature.supported |= (port_ability.Full_10 ? (EN_OFPPF_10MB_FD):0);
    port_feature.supported |= (port_ability.Half_100 ? (EN_OFPPF_100MB_HD):0);
    port_feature.supported |= (port_ability.Full_100 ? (EN_OFPPF_100MB_FD):0);
    port_feature.supported |= (port_ability.Half_1000 ? (EN_OFPPF_1GB_FD):0);
    port_feature.supported |= (port_ability.Full_1000 ? (EN_OFPPF_1GB_HD):0);
#if (defined(CONFIG_SDK_RTL8390) || defined(CONFIG_SDK_RTL9300))
    port_feature.supported |= (port_ability.Full_10G ? (EN_OFPPF_10GB_FD):0);
#endif
#if defined(CONFIG_SDK_RTL9310)
    port_feature.supported |= (port_ability.adv_10GBase_T ? (EN_OFPPF_10GB_FD):0);
#endif
    pFeature->supported = port_feature.supported;
    pFeature->curr = port_feature.curr;
    pFeature->peer = 0;
    pFeature->advertised = 0;

    return 0;
}
int32 sdn_hal_port_config_set(uint32 port_no, sdn_hal_port_config_t *port_config)
{
    uint32 phy_port = 0;
    uint32 unit = 0;
    uint32 ret = 0;
    rtk_enable_t port_admin_status = ENABLED;

    SDN_PARAM_CHK(!sdn_hal_port_exist(port_no), SDN_HAL_RETURN_FAILED);

    sdn_hal_usrPort_unit_get(port_no, &unit);

    if(SDN_HAL_RETURN_OK != (ret = sdn_hal_userPort2PhyPort(port_no, &phy_port)))
    {
        return ret;
    }

    if (port_config->config_mask & EN_OFPPC_PORT_DOWN)
    {
        if (RT_ERR_OK != rtk_port_adminEnable_set(unit, phy_port, (port_config->config & EN_OFPPC_PORT_DOWN)))
        {
            return SDN_HAL_RETURN_FAILED;
        }
    }

    if (port_config->config_mask & EN_OFPPC_NO_RECV)
    {
        if (RT_ERR_OK !=  rtk_port_rxEnable_set(unit, phy_port, (port_config->config & EN_OFPPC_NO_RECV)))
        {
            return SDN_HAL_RETURN_FAILED;
        }
    }

    if (port_config->config_mask & EN_OFPPC_NO_FWD)
    {
        if (RT_ERR_OK != rtk_port_txEnable_set(unit, phy_port, (port_config->config & EN_OFPPC_NO_FWD)))
        {
            return SDN_HAL_RETURN_FAILED;
        }
    }

    return SDN_HAL_RETURN_OK;
}

int32 sdn_hal_port_exist(uint32 port_no)
{
    uint32 unit = 0;
    uint32 phyPort = 0;

    sdn_hal_usrPort_unit_get(port_no, &unit);

    if(SDN_HAL_RETURN_OK != sdn_hal_userPort2PhyPort(port_no, &phyPort))
    {
        return 0;
    }

    if(RTK_PORTMASK_IS_PORT_SET(all_portMask[unit], phyPort))
        return 1;

    return 0;
}

int32 sdn_hal_port_add(uint32 port_no)
{
    uint32 unit = 0;
    uint32 phyPort = 0;
    sdn_hal_usrPort_unit_get(port_no, &unit);

    if(SDN_HAL_RETURN_OK != sdn_hal_userPort2PhyPort(port_no, &phyPort))
    {
        return SDN_HAL_RETURN_FAILED;
    }
    /*TODO*//*when mgmt port turn into openflow port, clean acl setup for this mgmt port.*/
    return SDN_HAL_RETURN_OK;
}

int32 sdn_hal_port_delete(uint32 port_no)
{
    uint32 unit = 0;
    uint32 phyPort = 0;
    sdn_hal_usrPort_unit_get(port_no, &unit);

    if(SDN_HAL_RETURN_OK != sdn_hal_userPort2PhyPort(port_no, &phyPort))
    {
        return SDN_HAL_RETURN_FAILED;
    }
    return SDN_HAL_RETURN_OK;
}

int32 sdn_hal_port_init(void)
{
    rtk_switch_devInfo_t dev_info;
    sdn_hal_portmask_t   sdn_hal_physicalPortMask;


#if 1 /*This unit init procedure will move to sdn_hal_init when sdn_hal_init is done.*/
    int i;

    for(i = 0; i < RTK_MAX_NUM_OF_UNIT; i++)
    {
        unit_list[i] = SDN_HAL_INVALID_UNIT;
    }
    for(i = 0; i < _countof(sdn_hal_port_map); i++)
    {
        if((sdn_hal_port_map[i].devId != PORTCONF_END_VAL) && (unit_list[sdn_hal_port_map[i].devId] == SDN_HAL_INVALID_UNIT))
            unit_list[sdn_hal_port_map[i].devId] = 1;
    }

#endif

    /*Get exist portmask from chip.*/
    FOR_EACH_UNIT(i)
    {
        if(RT_ERR_OK != rtk_switch_deviceInfo_get(i, &dev_info))
        {
            return SDN_HAL_RETURN_FAILED;
        }
        all_portMask[i] = dev_info.all.portmask;
        memset(&sdn_hal_physicalPortMask, 0, sizeof(sdn_hal_portmask_t));
    }

    /*Register port link change call back.*/
    return SDN_HAL_RETURN_OK;
}

int32 sdn_hal_eth_addr_get(uint32 port_no, uint8 mac_addr[ETH_ADDR_LEN])
{
    uint32 ret;
    uint32 unit = 0;
    uint32 phyPort = 0;
    rtk_mac_t mac;

    sdn_hal_usrPort_unit_get(port_no, &unit);

    if(RT_ERR_OK != rtk_switch_mgmtMacAddr_get(unit, &mac))
    {
        return SDN_HAL_RETURN_FAILED;
    }

    if ((mac.octet[ETHER_ADDR_LEN - 1] + port_no + 1) > 0xFF)
    {
        if (0xFF == mac.octet[ETHER_ADDR_LEN - 2])
            mac.octet[ETHER_ADDR_LEN - 3] += 1;
        mac.octet[ETHER_ADDR_LEN - 2] += 1;
    }
    mac.octet[ETHER_ADDR_LEN - 1] += (port_no + 1);
    //mac_addr = mac;
    osal_memcpy(mac_addr, mac.octet, ETH_ADDR_LEN);

    return SDN_HAL_RETURN_OK;

}

int32 sdn_hal_switch_features_get(sdn_db_switch_features_t *features)
{
    uint32 unit = 0;
    unit = SDN_HAL_MY_UNIT_ID();

    features->virtual_table_exist = true;
    features->n_tables = 2;
    features->capabilities = SDN_HAL_RULE_SWITCH_FLOW_STATS
                                | SDN_HAL_RULE_SWITCH_PORT_STATS
                                | SDN_HAL_RULE_SWITCH_TABLE_STATS
                                | SDN_HAL_RULE_SWITCH_PORT_BLOCKED;

    return SDN_HAL_RETURN_OK;
}

//int32 sdn_hal_flow_entry_add(struct ofp_flow_mod *fm, uint8 match_num, uint8 inst_num, struct ofp_instruction_header *inst)
int32 sdn_hal_flow_entry_add(sdn_db_flow_entry_t *flow_entry_p)
{
    uint32 ret = SDN_HAL_RETURN_FAILED;
    uint32 unit = 0;
    rtk_acl_operation_t entry_op;
    rtk_acl_action_t    acl_action;

    memset(&entry_op, 0, sizeof(entry_op));
    memset(&acl_action, 0 , sizeof(acl_action));

    unit = SDN_HAL_MY_UNIT_ID();

    if ((ret = sdn_hal_flow_IsVaild(flow_entry_p)) != SDN_HAL_RETURN_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }
#ifdef RTK_HAL
    if (_sdn_hal_flow_aclField_fill(flow_entry_p) != SDN_HAL_RETURN_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    if (_sdn_hal_flow_aclAction_fill(flow_entry_p, &acl_action)!=  SDN_HAL_RETURN_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }
    entry_op.aggr_1 = DISABLED;
    entry_op.aggr_2 = DISABLED;
    entry_op.reverse= DISABLED;

//TODO:convert openflow flow id to asic acl id
    if (RT_ERR_OK != rtk_acl_ruleOperation_set(unit, (rtk_acl_phase_t)table_id, (rtk_acl_id_t)asic_id,  &entry_op))
    {
        return SDN_HAL_RETURN_FAILED;
    }

    if (RT_ERR_OK != rtk_acl_ruleAction_set(unit, (rtk_acl_phase_t)table_id, (rtk_acl_id_t)asic_id, &acl_action))
    {
        return SDN_HAL_RETURN_FAILED;
    }

    if (RT_ERR_OK != rtk_acl_ruleValidate_set(unit, (rtk_acl_phase_t)table_id, (rtk_acl_id_t)asic_id, ENABLED))
    {
        return SDN_HAL_RETURN_FAILED;
    }
#endif
    return SDN_HAL_RETURN_OK;

}


//int32 sdn_hal_flow_entry_modify(struct ofp_flow_mod *fm, uint8 match_num, uint8 inst_num, struct ofp_instruction_header *inst)
int32 sdn_hal_flow_entry_modify(sdn_db_flow_entry_t *flow_entry_p)
{
    uint32 ret = SDN_HAL_RETURN_FAILED;
    uint32 unit = 0;
    rtk_acl_operation_t entry_op;
    rtk_acl_action_t acl_action;
    rtk_acl_phase_t table_id = 0;

    if ((ret = sdn_hal_flow_IsVaild(flow_entry_p)) != SDN_HAL_RETURN_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    memset(&entry_op, 0, sizeof(entry_op));
    memset(&acl_action, 0, sizeof(acl_action));

    unit = SDN_HAL_MY_UNIT_ID();
    table_id = sdn_hal_rule_get_flow_table(flow_entry_p->flow_index);

#ifdef RTK_HAL
    if (_sdn_hal_flow_aclField_fill(flow_entry_p) != SDN_HAL_RETURN_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }


    if (_sdn_hal_flow_aclAction_fill(flow_entry_p, &acl_action)!=  SDN_HAL_RETURN_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }
    entry_op.aggr_1 = DISABLED;
    entry_op.aggr_2 = DISABLED;
    entry_op.reverse= DISABLED;

//TODO:covert openflow id to asic acl id
    if (RT_ERR_OK != rtk_acl_ruleOperation_set(unit, table_id, (rtk_acl_id_t)asic_id,  &entry_op))
    {
        return SDN_HAL_RETURN_FAILED;
    }

    if (RT_ERR_OK != rtk_acl_ruleAction_set(unit, table_id, (rtk_acl_id_t)asic_id, &acl_action))
    {
        return SDN_HAL_RETURN_FAILED;
    }

    if (RT_ERR_OK != rtk_acl_ruleValidate_set(unit, table_id, (rtk_acl_id_t)asic_id, ENABLED))
    {
        return SDN_HAL_RETURN_FAILED;
    }
#endif
    return SDN_HAL_RETURN_OK;


}
//int32 sdn_hal_flow_entry_delete(struct ofp_flow_delete *fm, uint8 match_num)
int32 sdn_hal_flow_entry_delete(sdn_db_flow_entry_t *flow_entry_p)
{
    uint32 ret = SDN_HAL_RETURN_FAILED;
    uint32 unit = 0;
    uint32 asic_id = 0;
    rtk_acl_phase_t table_id = 0;
    rtk_acl_clear_t del_acl_idx;

    if ((ret = sdn_hal_flow_IsVaild(flow_entry_p)) != SDN_HAL_RETURN_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    unit = SDN_HAL_MY_UNIT_ID();
    table_id = sdn_hal_rule_get_flow_table(flow_entry_p->flow_index);

    if(SDN_HAL_RETURN_OK != sdn_hal_flowId_asicId_transform(table_id, flow_entry_p->flow_index, &asic_id))
    {
        return SDN_HAL_RETURN_FAILED;
    }

    memset(&del_acl_idx, 0, sizeof(del_acl_idx));
    del_acl_idx.start_idx = asic_id;
    del_acl_idx.end_idx   = asic_id;

//TODO:covert openflow id to asic acl id
    if ( RT_ERR_OK != rtk_acl_rule_del(unit, table_id, &del_acl_idx))
    {
        return SDN_HAL_RETURN_FAILED;
    }

#ifdef RTK_HAL
    if (flow_entry_p->action_data.action_mask &  SDN_HAL_RULE_FLOW_ACTION_RL)
    {
        if ( RT_ERR_OK != rtk_acl_meterMode_set(unit , meterBlkIdx, (rtk_acl_meterMode_t)OHAL_RULE_MAX_METER_FLAGS))
        {
            return SDN_HAL_RETURN_FAILED;
        }
    }

    if (sdn_hal_flow_IsWildcardFlowEntry(flow_entry_p))
    {
        /* Enable ARP BCAST flow entry */
        if (RT_ERR_OK != rtk_acl_ruleValidate_set(unit, (rtk_acl_phase_t)OHAL_RULE_GET_FLOW_TABLE_NUM(ARP_BCAST_ASIC_IDX), (rtk_acl_id_t)ARP_BCAST_ASIC_IDX, ENABLED))
        {
            return SDN_HAL_RETURN_FAILED;
        }
    }
#endif
    return SDN_HAL_RETURN_OK;
}
int32 sdn_hal_rule_get_flow_table(uint32 entryId)
{
    if(entryId < (SDN_HAL_ACL_INGRESS_EGRESS_CUT_LINE * SDN_HAL_ACL_ENTRY_SIZE_PER_BLOCK))
        return ACL_PHASE_VACL;
    else
        return ACL_PHASE_IACL;
}
int32 sdn_hal_flowId_asicId_transform(uint32 table_id, uint32 flow_id, uint32 *asic_id)
{
    switch(table_id)
    {
        case ACL_PHASE_VACL:
            if (flow_id < SDN_HAL_RULE_TABLE0_START_ID || flow_id > SDN_HAL_RULE_TABLE0_END_ID)
            {
                return SDN_HAL_RETURN_FAILED;
            }
            break;
        case ACL_PHASE_IACL:
            if (flow_id < SDN_HAL_RULE_TABLE0_END_ID || flow_id > SDN_HAL_ACL_ENTRY_END)
            {
                return SDN_HAL_RETURN_FAILED;
            }

        default:
            return SDN_HAL_RETURN_FAILED;
    }
    *asic_id = flow_id;
    return SDN_HAL_RETURN_OK;
}

static int32 _sdn_hal_flow_aclField_fill(sdn_hal_rule_flow_mgmt_t *flow_entry_p)
{
    uint32 fields_idx = 0;
    uint32 asic_idx   = 0;
    uint32 unit_id    = 0;
    uint32 phy_port   = 0xff;
    uint32 ret_val    = SDN_HAL_RETURN_FAILED;
    uint16 mask16     = 0xffff;
    uint16 port_no    = 0;
    uint8 template_mask = 0xff;
    uint8 template_id = 0xff;
    uint8 ip_dscp_flag = FALSE;
    BOOL_T  is_default_template = FALSE;
    BOOL_T  is_shared_template  = FALSE;

    rtk_acl_phase_t table_id = 0;

    unit_id = SDN_HAL_MY_UNIT_ID();

    is_inport_existed = FALSE;

    is_default_template = (flow_entry_p->chosen_template_id == 0) ? TRUE : FALSE;
    DBG_SDN("\r\n[%s][%d] Brandon test ,  is_default_template=[%s] \r\n",__FUNCTION__, __LINE__,
                         (is_default_template) ? "TRUE":"FALSE");

    table_id = sdn_hal_rule_get_flow_table(flow_entry_p->flow_id);

    if(SDN_HAL_RETURN_OK != sdn_hal_flowId_asicId_transform(table_id, flow_entry_p->flow_id, &asic_idx))
    {
        return SDN_HAL_RETURN_FAILED;
    }

    rtk_acl_ruleEntryField_write(unit_id, table_id, (rtk_acl_id_t)asic_idx, USER_FIELD_TEMPLATE_ID,
        (uint8 *)&flow_entry_p->chosen_template_id,
        &template_mask);

    fields_idx = 0;

    while (fields_idx < EN_OFPXMT_OFB_MAX)
    {
        if (!HAL_LIB_CHK_BIT_ON(flow_entry_p->match_fields.match_fields_bitmap, fields_idx))
        {
           fields_idx++;
           continue;
        }
        DBG_SDN("\r\n[%s-%d]set unit(%d), phase(%d), entry(%d), field(%d)\r\n", __FUNCTION__, __LINE__,
                                                                    (uint32)unit_id,
                                                                    (uint32)table_id,
                                                                    flow_entry_p->flow_id, fields_idx);
        mask16 = 0xffff;
        switch((EN_OFP_OXM_OFB_MATCH_FIELDS_T)fields_idx)
        {
            case EN_OFPXMT_OFB_IN_PORT:
                sdn_hal_logicalPort2PhyPort(flow_entry_p->match_fields.field_data.l_port, &phy_port);
                is_inport_existed = TRUE;
                SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_id, (rtk_acl_id_t)asic_idx,
                                                 USER_FIELD_SPN, (uint8*)&phy_port, (uint8 *)&mask16);
            break;

            case EN_OFPXMT_OFB_IN_PHY_PORT:
                phy_port = flow_entry_p->match_fields.field_data.l_port;
                SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_id, (rtk_acl_id_t)asic_idx,
                                                 USER_FIELD_SPN, (uint8*)&phy_port, (uint8 *)&mask16);
                break;

            case EN_OFPXMT_OFB_ETH_DST:
                DBG_SDN("\r\n[%s-%d]dst mac:%02x:%02x:%02x:%02x:%02x:%02x \r\n", __FUNCTION__, __LINE__,
                        flow_entry_p->match_fields.field_data.dst_mac[0],
                        flow_entry_p->match_fields.field_data.dst_mac[1],
                        flow_entry_p->match_fields.field_data.dst_mac[2],
                        flow_entry_p->match_fields.field_data.dst_mac[3],
                        flow_entry_p->match_fields.field_data.dst_mac[4],
                        flow_entry_p->match_fields.field_data.dst_mac[5]);
                SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_id, (rtk_acl_id_t)asic_idx,
                                                 USER_FIELD_DMAC, flow_entry_p->match_fields.field_data.dst_mac, flow_entry_p->match_fields.field_data.dst_mac_mask);
                break;

            case EN_OFPXMT_OFB_ETH_SRC:
                DBG_SDN("\r\n[%s-%d]src mac:%02x:%02x:%02x:%02x:%02x:%02x \r\n",  __FUNCTION__, __LINE__,
                        flow_entry_p->match_fields.field_data.src_mac[0],
                        flow_entry_p->match_fields.field_data.src_mac[1],
                        flow_entry_p->match_fields.field_data.src_mac[2],
                        flow_entry_p->match_fields.field_data.src_mac[3],
                        flow_entry_p->match_fields.field_data.src_mac[4],
                        flow_entry_p->match_fields.field_data.src_mac[5]);

                DBG_SDN("\r\n[%s-%d]src mask:%02x:%02x:%02x:%02x:%02x:%02x \r\n",  __FUNCTION__, __LINE__,
                        flow_entry_p->match_fields.field_data.src_mac_mask[0],
                        flow_entry_p->match_fields.field_data.src_mac_mask[1],
                        flow_entry_p->match_fields.field_data.src_mac_mask[2],
                        flow_entry_p->match_fields.field_data.src_mac_mask[3],
                        flow_entry_p->match_fields.field_data.src_mac_mask[4],
                        flow_entry_p->match_fields.field_data.src_mac_mask[5]);

                        SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_id, (rtk_acl_id_t)asic_idx,
                                USER_FIELD_SMAC, flow_entry_p->match_fields.field_data.src_mac, flow_entry_p->match_fields.field_data.src_mac_mask);
                break;

            case EN_OFPXMT_OFB_ETH_TYPE:
            {
                 uint16 eth_type = flow_entry_p->match_fields.field_data.eth_type_val;
                 DBG_SDN("\r\n[%s-%d] eth_type:%02x\r\n", __FUNCTION__, __LINE__, eth_type);
                 SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_id, (rtk_acl_id_t)asic_idx,
                                                 USER_FIELD_ETHERTYPE, (uint8 *)&eth_type, (uint8 *)&mask16);
            }
                 break;

            case EN_OFPXMT_OFB_VLAN_VID:
                //Brandon modify for RTK8382M:
                 DBG_SDN("\r\n[%s-%d]Make sure that  is_default_template=[%s]  \r\n",__FUNCTION__, __LINE__,
                                      (is_default_template) ? "TRUE":"FALSE");

                 DBG_SDN("\r\n[%s-%d]  svid=[%d]\r\n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.svid);
                 DBG_SDN("\r\n[%s-%d]  cvid=[%d]\r\n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.cvid);

                 SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_id,
                                    (rtk_acl_id_t)asic_idx,
                                    USER_FIELD_OTAG_VID,
                                    (uint8 *)&flow_entry_p->match_fields.field_data.svid,
                                    (uint8 *)&mask16);
                break;
            case EN_OFPXMT_OFB_VLAN_PCP:
                DBG_SDN("\r\n[%s-%d] pcp_value=[%d]\r\n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.pcp_value);

                SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_id,
                                    (rtk_acl_id_t)asic_idx,
                                    USER_FIELD_OTAG_PRI,
                                    &flow_entry_p->match_fields.field_data.pcp_value, &flow_entry_p->match_fields.field_data.pcp_mask);
                break;

#if 0 /*unsupport now*/
            case OHAL_RULE_VLAN_IVID:
                 OHAL_LIB_APPLY_FIELD_VALUE(unit_id, (rtk_acl_phase_t) table_id, (rtk_acl_id_t) flow_entry_p->flow_id,
                                                 USER_FIELD_ITAG_VID, (uint8 *)&flow_entry_p->match_fields.field_data.cvid, (uint8 *)&mask16);
                break;


            case OHAL_RULE_VLAN_ID_RANGE0:
                if (flow_entry_p->match_fields.field_data.vlan_rang_id > OHAL_RULE_ACL_VLAN_RANGE_NUMBER_PER_TEMPLATE)
                {
                    OHAL_LIB_PRINT_MSG("out of vid(%d) range", flow_entry_p->match_fields.field_data.vlan_rang_id);
                }

                OHAL_LIB_APPLY_FIELD_VALUE(unit_id, (rtk_acl_phase_t) table_id, (rtk_acl_id_t) flow_entry_p->flow_id,
                                                USER_FIELD_VID_RANGE0, (uint8 *)&flow_entry_p->match_fields.field_data.vlan_rang_id, (uint8 *)&flow_entry_p->match_fields.field_data.vlan_rang_id);
                break;
#endif
            case EN_OFPXMT_OFB_IP_DSCP:
                mask16 = 0xfc00;
                ip_dscp_flag = TRUE;

                DBG_SDN("\r\n[%s-%d] dscp=[%d] \r\n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.dscp );

                flow_entry_p->match_fields.field_data.dscp <<= 2;
                DBG_SDN("\r\n[%s-%d] dscp<<=2=[%d] \r\n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.dscp );
                SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_id, (rtk_acl_id_t)asic_idx,
                                                 USER_FIELD_IP4TOS_IP6TC, &flow_entry_p->match_fields.field_data.dscp, (uint8 *)&mask16);

                break;

            case EN_OFPXMT_OFB_IP_PROTO:
                mask16 = 0xff00;
                SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_id, (rtk_acl_id_t)asic_idx,
                                                 USER_FIELD_IP4PROTO_IP6NH, &flow_entry_p->match_fields.field_data.protocol_val, (uint8 *)&mask16);
                break;

            case EN_OFPXMT_OFB_ARP_OP:
                mask16 = 0xffff;
                SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_id, (rtk_acl_id_t)asic_idx,
                                                 USER_FIELD_ARPOPCODE, (uint8 *)&flow_entry_p->match_fields.field_data.arp_op, (uint8 *)&mask16);
                break;

            case EN_OFPXMT_OFB_IP_ECN:
                if (ip_dscp_flag ==FALSE) //Brandon
                {
                    mask16 = 0x0300;
                    SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_id, (rtk_acl_id_t)asic_idx,
                                                 USER_FIELD_IP4TOS_IP6TC, &flow_entry_p->match_fields.field_data.ip_ecn, (uint8 *)&mask16);
                }
                else
                {
                    // EN_OFPXMT_OFB_IP_ECN is scan after EN_OFPXMT_OFB_IP_DSCP,
                    //So, if DSCP has value, we must assemble DSCP and ECN to be a TOS ,and then set it into chip.
                    mask16 = 0xff00;
                    DBG_SDN("\r\n[%s-%d] dscp=[%d] \r\n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.dscp );

                    flow_entry_p->match_fields.field_data.dscp = flow_entry_p->match_fields.field_data.dscp |flow_entry_p->match_fields.field_data.ip_ecn;
                    DBG_SDN("\r\n[%s-%d] dscp | ecn=[%d] \r\n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.dscp );

                    SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_id, (rtk_acl_id_t)asic_idx,
                    USER_FIELD_IP4TOS_IP6TC, &flow_entry_p->match_fields.field_data.dscp, (uint8 *)&mask16);
                }
                break;
            case EN_OFPXMT_OFB_IPV4_SRC:
            case EN_OFPXMT_OFB_IPV6_SRC:
                if (fields_idx == EN_OFPXMT_OFB_IPV4_SRC)
                {
                    uint8 *ipaddr_p =  (uint8*)&flow_entry_p->match_fields.field_data.src_ipv4_addr;
                    DBG_SDN("src ip:%d.%d.%d.%d \r\n", ipaddr_p[0],
                                                       ipaddr_p[1],
                                                       ipaddr_p[2],
                                                       ipaddr_p[3]);
                    SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_id, (rtk_acl_id_t)asic_idx,
                                                     USER_FIELD_IP4_SIP, (uint8 *)&flow_entry_p->match_fields.field_data.src_ipv4_addr, (uint8 *)&flow_entry_p->match_fields.field_data.src_ipv4_mask);
                }
                else
                {
                    SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_id, (rtk_acl_id_t)asic_idx,
                                                    USER_FIELD_IP6_SIP, flow_entry_p->match_fields.field_data.ipv6_src_addr, flow_entry_p->match_fields.field_data.ipv6_src_mask);
                }
                break;

            case EN_OFPXMT_OFB_IPV4_DST:
            case EN_OFPXMT_OFB_IPV6_DST:
                if (fields_idx == EN_OFPXMT_OFB_IPV4_DST)
                {
                     uint8 *ipaddr_p =  (uint8*)&flow_entry_p->match_fields.field_data.dst_ipv4_addr;
                     DBG_SDN("dst ip:%d.%d.%d.%d \r\n", ipaddr_p[0],
                                                        ipaddr_p[1],
                                                        ipaddr_p[2],
                                                        ipaddr_p[3]);
                     SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_id, (rtk_acl_id_t)asic_idx,
                                                     USER_FIELD_IP4_DIP, (uint8 *)&flow_entry_p->match_fields.field_data.dst_ipv4_addr, (uint8 *)&flow_entry_p->match_fields.field_data.dst_ipv4_mask);
                }
                else
                {
                     SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_id, (rtk_acl_id_t)asic_idx,
                                                     USER_FIELD_IP6_DIP, flow_entry_p->match_fields.field_data.ipv6_dst_addr, flow_entry_p->match_fields.field_data.ipv6_dst_mask);
                }
                break;

            case EN_OFPXMT_OFB_TCP_SRC:
            case EN_OFPXMT_OFB_UDP_SRC:
                mask16 = 0xffff;
                port_no =  ((fields_idx == EN_OFPXMT_OFB_TCP_SRC) ? flow_entry_p->match_fields.field_data.tcp_src_port_no :
                                                                     flow_entry_p->match_fields.field_data.udp_src_port_no );
                DBG_SDN(" \n[%s-%d] source port no=[%d] \n", __FUNCTION__, __LINE__,port_no);

                SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_id, (rtk_acl_id_t)asic_idx,
                                                 USER_FIELD_L4_SRC_PORT, (uint8 *)&port_no, (uint8 *)&mask16);
                break;

            case EN_OFPXMT_OFB_TCP_DST:
            case EN_OFPXMT_OFB_UDP_DST:
                mask16 = 0xffff;
                port_no =  ((fields_idx == EN_OFPXMT_OFB_TCP_DST) ? flow_entry_p->match_fields.field_data.tcp_dst_port_no :
                                                                     flow_entry_p->match_fields.field_data.udp_dst_port_no );
                DBG_SDN(" \n[%s-%d] destination port no=[%d] \n", __FUNCTION__, __LINE__,port_no);

                SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_id, (rtk_acl_id_t)asic_idx,
                                                 USER_FIELD_L4_DST_PORT, (uint8 *)&port_no, (uint8 *)&mask16);
                break;

            case EN_OFPXMT_OFB_ICMPV4_TYPE:
                mask16 = 0xffff;
                DBG_SDN(" \n[%s-%d] icmp type=[%d] \n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.icmp_type);

                SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_id, (rtk_acl_id_t)asic_idx,
                                                 USER_FIELD_ICMP_TYPE, &flow_entry_p->match_fields.field_data.icmp_type, (uint8 *)&mask16);
                break;
            case EN_OFPXMT_OFB_ICMPV4_CODE:
                mask16 = 0xffff;
                DBG_SDN(" \n[%s-%d] icmp code=[%d] \n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.icmp_code);

                SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_id, (rtk_acl_id_t)asic_idx,
                                                     USER_FIELD_ICMP_CODE, &flow_entry_p->match_fields.field_data.icmp_code, (uint8 *)&mask16);
                break;

            default:
                break;
        }
        fields_idx++;
    }

    return SDN_HAL_RETURN_OK;
}

static int32 _sdn_hal_flow_ingressAction_assign(uint32 action_type,
    sdn_hal_rule_flow_mgmt_t  *flow_entry_p,
    sdn_hal_lib_action_t *ingress_action_p)
{

    uint32 cpu_port = 0;
    uint32 phy_port = 0xffff;
    uint32 unit_id  = 0;
    //sdn_hal_meter_entry_t meter_entry;
    SDN_PARAM_CHK(flow_entry_p == NULL, SDN_HAL_RETURN_OK);
    SDN_PARAM_CHK(ingress_action_p == NULL, SDN_HAL_RETURN_OK);

    sdn_hal_lib_iAction_t *action = (sdn_hal_lib_iAction_t *)&ingress_action_p->vact;
    DBG_SDN("[%s-%d] table0 action(%d)\r\n", __FUNCTION__, __LINE__, action_type);

    unit_id = SDN_HAL_MY_UNIT_ID();
    /* insert ingress acl action*/

    switch(action_type)
    {
#ifdef RTK_CODE

        case EN_OFPAT_OUTPUT:
        {
            rtk_portmask_t phy_port_list;
            uint32 logical_port_id = 0;
            int32  flood_port_mask_id = -1;
            DBG_SDN("[%s-%d] output to %s\r\n", __FUNCTION__, __LINE__, flow_entry_p->action_data.port_list_is_used == TRUE ? "multi-port" :"single-port");

            /*Brandon add the below logic, if don't do this, tagged packet in, and then untagged packet out.

                tagged packet     + outer tag status(tagged) +only output---->tagged packet
                untagged packet + outer tag status(tagged) +only output---->untagged packet
            */
#if 0
           action->egrTagStat_en            = ENABLED;
           action->egrTagStat_data.otag_sts = ACL_ACTION_TAG_STS_KEEP_CONTENT;
#endif /*need to fix*/

            if (flow_entry_p->action_data.is_normal_output == TRUE)
            {
                action->fwd_data.fwd_type = (rtk_acl_igrActionFwdType_t)ACL_ACTION_FWD_PERMIT;
            }

            if (flow_entry_p->action_data.port_list_is_used == TRUE)
            {
    	        memset(&phy_port_list, 0, sizeof(phy_port_list));

        		if (SDN_HAL_RETURN_OK != sdn_hal_logicalPortList2PhyPortList(unit_id, &flow_entry_p->action_data.port_list, &phy_port_list))
                {
        		    DBG_SDN("[%s-%d] convert logic port to phy port failed\r\n", __FUNCTION__, __LINE__);
                    return SDN_HAL_RETURN_FAILED;
                }

                DBG_SDN("\r\n[%s-%d] output to  flow_entry_p->action_data.port_list.bits=[0x%x],  phy_port_list.bits=[0x%x]\r\n",
                    __FUNCTION__, __LINE__, flow_entry_p->action_data.port_list.bits[0] , phy_port_list.bits[0]);

                if (rtk_l2_mcastFwdIndex_alloc(unit_id, &flood_port_mask_id) != RT_ERR_OK
                    || rtk_l2_mcastFwdPortmaskEntry_set(unit_id, flood_port_mask_id, &phy_port_list) != RT_ERR_OK)
                {
                    DBG_SDN("[%s-%d] out port set failed\r\n", __FUNCTION__, __LINE__);
                    return SDN_HAL_RETURN_FAILED;
                }
                action->fwd_data.fwd_info = flood_port_mask_id;
                action->fwd_data.fwd_type =  (flow_entry_p->action_data.is_normal_output == TRUE ?  (rtk_acl_igrActionFwdType_t)ACL_ACTION_FWD_COPY_TO_PORTMASK  : (rtk_acl_igrActionFwdType_t)ACL_ACTION_FWD_REDIRECT_TO_PORTMASK);
                DBG_SDN("\r\n[%s-%d] output to port list(%x)\r\n", __FUNCTION__, __LINE__,
                                                                    action->fwd_data.info.copy_redirect_portMsk.fwd_idx);
            }
            else
            {
                sdn_hal_userPort2PhyPort(unit_id, flow_entry_p->action_data.port_no, &phy_port);

                rtk_port_cpuPortId_get(unit_id, &cpu_port);
                if (phy_port == cpu_port)  //PHY_PORT_28= CPU port,     for RTK8382M
                {
                    action->fwd_data.info.copy_redirect_port.cpu_tag = 1;    /* force packet to carry CPU tag 1: force carry; 0: follow other modules */
                }

                action->fwd_en            = ENABLED;
                //action->fwd_data.info.copy_redirect_port.force = 1;
                action->fwd_data.fwd_info = phy_port; /*todo: assign the specified port number*/
        		action->fwd_data.fwd_type = (flow_entry_p->action_data.is_normal_output == TRUE ?  (rtk_acl_igrActionFwdType_t)ACL_ACTION_FWD_COPY_TO_PORTID :  (rtk_acl_igrActionFwdType_t)ACL_ACTION_FWD_REDIRECT_TO_PORTID );
                //action->fwd_data.info.copy_redirect_port.fwd_port_id = phy_port;  //Brandon ,For RTL 8380;
                DBG_SDN("\r\n[%s-%d] output to type(%d) %s (%d) %s (%d)\r\n", __FUNCTION__, __LINE__,
                                                                      action->fwd_data.fwd_type,
                                                                      "logical port",
                                                                      flow_entry_p->action_data.port_no,
                                                                      "phy port",
                                                                      action->fwd_data.info.copy_redirect_port.fwd_port_id);
            }

            //for pure sdn to add port in sorc-port isolation
            {
                rtk_portmask_t dst_phy_port_list;
                uint32       src_phy_port = 0;
                uint32       dst_phy_port = 0;

                rtk_port_cpuPortId_get(unit_id, &cpu_port);

                memset(&dst_phy_port_list, 0, sizeof(dst_phy_port_list));

                if (is_inport_existed == TRUE)
                {
                    sdn_hal_userPort2PhyPort(unit_id, flow_entry_p->match_fields.field_data.l_port, &src_phy_port);
                }

                if (flow_entry_p->action_data.port_list_is_used && is_inport_existed == TRUE )
                {
                    rtk_port_isolation_get(unit_id, src_phy_port, &dst_phy_port_list);
                    PORTMASK_PORT_SET(dst_phy_port_list.bits, src_phy_port);
                    PORTMASK_PORT_SET(dst_phy_port_list.bits, cpu_port);
                    for (dst_phy_port = PHY_PORT_0; dst_phy_port < PHY_PORT_49; dst_phy_port++)
                    {
                        if (PORTMASK_IS_PORT_SET(phy_port_list.bits, dst_phy_port))
                        {
                            PORTMASK_PORT_SET(dst_phy_port_list.bits, dst_phy_port);
                        }
                    }
                    rtk_port_isolation_set(unit_id, src_phy_port, &dst_phy_port_list);
                }
                else if (flow_entry_p->action_data.port_list_is_used != TRUE  && is_inport_existed == TRUE)
                {
                    rtk_port_isolation_get(unit_id, src_phy_port, &dst_phy_port_list);
                    PORTMASK_PORT_SET(dst_phy_port_list.bits, src_phy_port);
                    PORTMASK_PORT_SET(dst_phy_port_list.bits, phy_port);
                    PORTMASK_PORT_SET(dst_phy_port_list.bits, cpu_port);
                    rtk_port_isolation_set(unit_id,  src_phy_port, &dst_phy_port_list);
                }
                else if ( is_inport_existed == FALSE)
                {
                    for (src_phy_port = PHY_PORT_0; src_phy_port < PHY_PORT_49; src_phy_port++)
                    {
                        memset(&dst_phy_port_list, 0, sizeof(dst_phy_port_list));
                        rtk_port_isolation_get(unit_id, src_phy_port, &dst_phy_port_list);
                        if (flow_entry_p->action_data.port_list_is_used == TRUE)
                        {
                            for (dst_phy_port = PHY_PORT_0; dst_phy_port < PHY_PORT_49; dst_phy_port++)
                            {
                                if (PORTMASK_IS_PORT_SET(phy_port_list.bits, dst_phy_port))
                                {
                                    PORTMASK_PORT_SET(dst_phy_port_list.bits, dst_phy_port);
                                }
                            }
                            rtk_port_isolation_set(unit_id,  src_phy_port, &dst_phy_port_list);
                        }
                        else
                        {
                            rtk_port_isolation_get(unit_id, src_phy_port, &dst_phy_port_list);
                            PORTMASK_PORT_SET(dst_phy_port_list.bits, src_phy_port);
                            PORTMASK_PORT_SET(dst_phy_port_list.bits, phy_port);
                            rtk_port_isolation_set(unit_id,  src_phy_port, &dst_phy_port_list);
                        }
                    }
                }

            }
        }
        break;


        case EN_OFPAT_POP_VLAN:
            /* Generally, all of the packets in the switch has only one tag.
            The following three statements is to modify outer vlan.

            action->outer_vlan_assign_en            = ENABLED;
            action->outer_vlan_data.vid_assign_type = ACL_IGR_ACTION_OVLAN_ASSIGN_NEW_VID;
            action->outer_vlan_data.vid_value       = flow_entry_p->action_data.cvid;
            */
           action->egrTagStat_en            = ENABLED;
           action->egrTagStat_data.otag_sts = ACL_ACTION_TAG_STS_UNTAG;  //Pop packet has only one tag, so untag output.

            printf("[%s][%d] Brandon test  svid=[%d]  cvid=[%d]\n  ",__FUNCTION__,__LINE__,flow_entry_p->action_data.svid,
                      flow_entry_p->action_data.cvid);
        break;

        case EN_OFPAT_PUSH_VLAN:
             /* Generally, all of the packets in the switch has only one tag ,except MPLS...double tag, it has its own MPLS PUSH VLAN.
                 In this case, push vlan and set vlan field does the same action that is just to modify outer tag.
            */
            action->outer_vlan_assign_en            = ENABLED;
            action->outer_vlan_data.vid_assign_type = ACL_IGR_ACTION_OVLAN_ASSIGN_NEW_VID;
            action->outer_vlan_data.vid_value       = flow_entry_p->action_data.svid;
            action->egrTagStat_en                   = ENABLED;
            action->egrTagStat_data.otag_sts        = ACL_ACTION_TAG_STS_TAG;
            printf("[%s][%d] Brandon test  svid=[%d]  cvid=[%d]\n  ",__FUNCTION__,__LINE__,flow_entry_p->action_data.svid,
            flow_entry_p->action_data.cvid);
        break;

        case EN_OFPAT_SET_FIELD:
            /* Generally, all of the packets in the switch has only one tag ,except MPLS...double tag, it has its own MPLS PUSH VLAN.
            In this case, push vlan and set vlan field does the same action that is just to modify outer tag.
            */

            printf("[%s][%d] Brandon test,  flow_entry_p->action_data.ofp_oxm_type=[%d]\n  ",__FUNCTION__,__LINE__,
                        flow_entry_p->action_data.ofp_oxm_type);
            printf("[%s][%d] Brandon test  svid=[%d]  cvid=[%d]\n  ",__FUNCTION__,__LINE__,flow_entry_p->action_data.svid,
                        flow_entry_p->action_data.cvid);

	    if (flow_entry_p->action_data.ofp_oxm_type == EN_OFPXMT_OFB_VLAN_VID)
            {
                /* Brandon:  all templates doesn't support ethertype field, so don't check it.
                if (flow_entry_p->action_data.eth_type != 0x8100  && flow_entry_p->action_data.eth_type != 0x88a8)
                {
                    printf("[%s][%d] Brandon test  eth_type=[0x%x]\n  ",__FUNCTION__,__LINE__,flow_entry_p->action_data.eth_type);

                    return SDN_HAL_RETURN_FAILED;
                }
                */
                action->outer_vlan_assign_en            = ENABLED;
                action->outer_vlan_data.vid_assign_type = ACL_IGR_ACTION_OVLAN_ASSIGN_NEW_VID;
                action->outer_vlan_data.vid_value       = flow_entry_p->action_data.svid;
                action->egrTagStat_en            = ENABLED;
                action->egrTagStat_data.otag_sts = ACL_ACTION_TAG_STS_TAG;
                printf("[%s][%d] Brandon test  svid=[%d]  cvid=[%d]\n  ",__FUNCTION__,__LINE__,flow_entry_p->action_data.svid,
                flow_entry_p->action_data.cvid);
            }


	    if (flow_entry_p->action_data.ofp_oxm_type == EN_OFPXMT_OFB_VLAN_PCP)
            {
                action->rmk_en            = ENABLED;
                action->rmk_data.rmk_act = ACL_ACTION_REMARK_OUTER_USER_PRI;
                action->rmk_data.rmk_info= flow_entry_p->action_data.remark_val;

/*Brandon add the below logic, if don't do this, tagged packet in, and then untagged packet out.

        tagged packet     + outer tag status(tagged) +only output---->tagged packet
        untagged packet + outer tag status(tagged) +only output---->untagged packet
*/

               action->egrTagStat_en            = ENABLED;
               action->egrTagStat_data.otag_sts = ACL_ACTION_TAG_STS_TAG;
                printf("[%s][%d] Brandon test  VLAN_PCP_value=[%d] \n  ",__FUNCTION__,__LINE__,flow_entry_p->action_data.remark_val);
            }else if (flow_entry_p->action_data.ofp_oxm_type == EN_OFPXMT_OFB_IP_DSCP)
            { //RTK8382M: no support  set EN_OFPXMT_OFB_IP_ECN
                action->rmk_en            = ENABLED;
                action->rmk_data.rmk_act = ACL_ACTION_REMARK_DSCP;
                action->rmk_data.rmk_info= flow_entry_p->action_data.remark_val;
                printf("[%s][%d] Brandon test  REMARK_DSCP_value=[%d] \n  ",__FUNCTION__,__LINE__,flow_entry_p->action_data.remark_val);

            }

	    break;
#ifdef OHAL_ACTION_SUPPORT
        case OHAL_RULE_FLOW_ACTION_PERMIT:
            action->fwd_en            = ENABLED;
            action->fwd_data.fwd_type = (rtk_acl_igrActionFwdType_t)ACL_ACTION_FWD_PERMIT;
        break;
        case OHAL_RULE_FLOW_ACTION_DROP:
            action->fwd_en            = ENABLED;
            action->fwd_data.fwd_type = (rtk_acl_igrActionFwdType_t)ACL_ACTION_FWD_DROP;
        break;

        case OHAL_RULE_FLOW_ACTION_TRAP:
        case OHAL_RULE_FLOW_ACTION_PORTSHUTDOWN:
            rtk_port_cpuPortId_get(0, &cpu_port);

            action->fwd_data.fwd_info = cpu_port;

            if (OHAL_LIB_CHK_BIT_ON(flow_entry_p->action_data.action_mask, OHAL_RULE_FLOW_ACTION_BYPASS_STP))
            {
                action->bypass_en           = ENABLED;
                action->bypass_data.igr_stp = ENABLED;
            }

            if (OHAL_LIB_CHK_BIT_ON(flow_entry_p->action_data.action_mask, OHAL_RULE_FLOW_ACTION_BYPASS_RATECTRL))
            {
                action->bypass_en          = ENABLED;
                action->bypass_data.ibc_sc = ENABLED;
            }
        break;

        case OHAL_RULE_FLOW_ACTION_COPYTOCPU:

            rtk_port_cpuPortId_get(0, &cpu_port);

            action->fwd_en            = ENABLED;
            action->fwd_data.fwd_info = cpu_port;
            action->fwd_data.fwd_type = (rtk_acl_igrActionFwdType_t)ACL_ACTION_FWD_COPY_TO_PORTID;

            if (OHAL_LIB_CHK_BIT_ON(flow_entry_p->action_data.action_mask, OHAL_RULE_FLOW_ACTION_SKIPIGRVLANFILTER))
            {
                action->bypass_en      = ENABLED;
                action->bypass_data.all = ENABLED;
            }
        break;

        case OHAL_RULE_FLOW_ACTION_FLOOD_IN_VLAN:
            action->fwd_en            = ENABLED;
            action->fwd_data.fwd_info = 1; /*TODO check assign value*/
            action->fwd_data.fwd_type = (rtk_acl_igrActionFwdType_t)ACL_ACTION_FWD_REDIRECT_TO_PORTID;
        break;

        case OHAL_RULE_FLOW_ACTION_MIRROR:
            action->mirror_en                  = ENABLED;
            action->mirror_data.mirror_set_idx = flow_entry_p->action_data.mirror_id; /* todo: assign a mirror index*/
        break;
        case OHAL_RULE_FLOW_ACTION_RL:
        {
            uint32 asic_id = 0;
            /*TODO: insert egress acl rate limit profile*/
             memset(&meter_entry, 0, sizeof(meter_entry));
             meter_entry.type      = OHAL_METER_TYPE_DROP;
             meter_entry.index    = flow_entry_p->meter_id;
             meter_entry.rate_type = OHAL_METER_RATE_TYPE_BYTE;
           if (ohal_flow_ConvertMeterRateTo16KBps(flow_entry_p->action_data.rate_limit_data.mode,
                                                  flow_entry_p->action_data.rate_limit_data.rate,
                                                  &meter_entry.rate) != OHAL_RULE_ERROR_RETURN_OK)
           {
           OHAL_LIB_RETURN_VALUE(OHAL_RULE_ERROR_RETURN_ADD_METER_ENTRY_FAILED);
           }

            if (OHAL_RULE_ERROR_RETURN_OK != ohal_meter_entry_is_exited(&meter_entry))
            {
                if (RT_ERR_OK != ohal_meter_create(0, &meter_entry))
                {
                    return SDN_HAL_RETURN_FAILED;
                }
            }

            OHAL_FLOW_ID_TO_ASIC_ID(OHAL_RULE_GET_FLOW_TABLE_NUM(flow_entry_p->flow_id), flow_entry_p->flow_id, &asic_id);

            if (RT_ERR_OK != ohal_meter_flow_attach(0, OHAL_RULE_GET_FLOW_TABLE_NUM(flow_entry_p->flow_id), asic_id, flow_entry_p->meter_id))
            {
                return SDN_HAL_RETURN_FAILED;
            }
        action->meter_data.meter_idx = flow_entry_p->meter_id;
        action->meter_en             = ENABLED;
        }
        break;

        case OHAL_RULE_FLOW_ACTION_PRIO:
            action->pri_en       = ENABLED;
            action->pri_data.pri = flow_entry_p->action_data.pri;
        break;

        case OHAL_RULE_FLOW_ACTION_CVID:
            action->inner_vlan_assign_en            = ENABLED;
            action->inner_vlan_data.vid_assign_type = (rtk_acl_igrActionIVlanAssignType_t)ACL_IGR_ACTION_OVLAN_ASSIGN_NEW_VID;
            action->inner_vlan_data.vid_value       = flow_entry_p->action_data.cvid;
        break;

        case OHAL_RULE_FLOW_ACTION_SKIPIGRVLANFILTER:
            action->bypass_en       = ENABLED;
            action->bypass_data.all = ENABLED;
        break;

        case OHAL_RULE_FLOW_ACTION_BYPASS_STP:
            action->bypass_en          = ENABLED;
            action->bypass_data.ibc_sc = ENABLED;
        break;

        case OHAL_RULE_FLOW_ACTION_BYPASS_RATECTRL:
            action->bypass_en          = ENABLED;
            action->bypass_data.ibc_sc = ENABLED;
        break;

        case OHAL_RULE_FLOW_ACTION_BYPASS_IGRDROP:
            action->bypass_en       = ENABLED;
            action->bypass_data.all = ENABLED;
        break;
#endif
#endif
        default:
            DBG_SDN("[%s-%d] table0 unknow action\r\n", __FUNCTION__, __LINE__);
        break;

    }

    if (flow_entry_p->counter_data.is_apply_counter == TRUE)
    {
#if defined(CONFIG_SDK_RTL9300)
         action->stat_data.stat_idx = flow_entry_p->counter_data.counter_id;
         action->stat_data.stat_type= (rtk_acl_statType_t)flow_entry_p->counter_data.counter_type;
#endif
         action->stat_en            = ENABLED;
    }
    else
    {
#if defined(CONFIG_SDK_RTL9300)
         action->stat_data.stat_type= (rtk_acl_statType_t)flow_entry_p->counter_data.counter_type; /* STAT_TYPE_BYTE_BASED_64BIT*/;
#endif
         action->stat_en            = DISABLED;
    }


    return SDN_HAL_RETURN_OK;
}

static int32 _sdn_hal_flow_egressAction_assign(uint32 action_type,
                                                        sdn_hal_rule_flow_mgmt_t *flow_entry_p,
                                                        sdn_hal_lib_action_t  *egress_action_p)
{
#if 0
    //uint32 cpu_port = 0;
    uint32 phy_port = 0xffff;
    uint32 unit_id  = 0;
    OHAL_METER_ENTRY_S   meter_entry;;
    OHAL_LIB_CHK_ISNULL(flow_entry_p);
    OHAL_LIB_CHK_ISNULL(egress_action_p);
    OHAL_LIB_EGRESS_ACTION_T *action = &egress_action_p->egr_acl;

    ohal_lib_unitid_get(&unit_id);
    DBG_SDN("[%s-%d] table1 action(%d)\r\n", __FUNCTION__, __LINE__, action_type);
    switch(action_type)
    {
#if 0
        case OHAL_RULE_FLOW_ACTION_PERMIT:
            action->fwd_data.fwd_info = (rtk_acl_egrActionFwdType_t)ACL_ACTION_FWD_PERMIT;
            action->fwd_data.fwd_type = (rtk_acl_egrActionFwdType_t)ACL_ACTION_FWD_PERMIT;
        break;
#endif
        case OHAL_RULE_FLOW_ACTION_DROP:
            action->fwd_en            = ENABLED;
            action->fwd_data.fwd_type = (rtk_acl_egrActionFwdType_t)ACL_ACTION_FWD_DROP;
        break;
#ifdef OHAL_ACTION_SUPPORT
        case OHAL_RULE_FLOW_ACTION_FLOOD_IN_VLAN:
            if (OHAL_LIB_CHK_BIT_ON(flow_entry_p->action_data.action_mask, OHAL_RULE_FLOW_ACTION_COPYTOCPU)
                || OHAL_LIB_CHK_BIT_ON(flow_entry_p->action_data.action_mask, OHAL_RULE_FLOW_ACTION_COPYTOCPU))
            {
                action->fwd_en            = ENABLED;
                action->fwd_data.fwd_type = (rtk_acl_egrActionFwdType_t)ACL_ACTION_FWD_REDIRECT_TO_PORTMASK;
                action->fwd_data.fwd_info = 1 /*TOTO assign forward index*/;
            }
        break;

        case OHAL_RULE_FLOW_ACTION_TRAP:
        case OHAL_RULE_FLOW_ACTION_PORTSHUTDOWN:
            rtk_port_cpuPortId_get(0, &cpu_port);

            action->fwd_en            = ENABLED;
            action->fwd_data.fwd_info = cpu_port;
            action->fwd_data.fwd_type = (rtk_acl_egrActionFwdType_t)ACL_ACTION_FWD_REDIRECT_TO_PORTID;
        break;

        case OHAL_RULE_FLOW_ACTION_COPYTOCPU:
            rtk_port_cpuPortId_get(0, &cpu_port);
            action->fwd_en            = ENABLED;
            action->fwd_data.fwd_info = cpu_port;
            action->fwd_data.fwd_type = (rtk_acl_egrActionFwdType_t)ACL_ACTION_FWD_COPY_TO_PORTID;
        break;
#endif
        case EN_OFPAT_OUTPUT:
        {
            rtk_portmask_t phy_port_list;
            uint32 logical_port_id = 0;
            INT32_T  flood_port_mask_id = -1;
            DBG_SDN("[%s-%d] output to %s\r\n", __FUNCTION__, __LINE__, flow_entry_p->action_data.port_list_is_used == TRUE ? "multi-port" :"single-port");
            if (flow_entry_p->action_data.port_list_is_used == TRUE)
            {
	        memset(&phy_port_list, 0, sizeof(phy_port_list));

		if (OHAL_RULE_ERROR_RETURN_OK != ohal_lib_logicalPortList2phyPortList(unit_id, &flow_entry_p->action_data.port_list, &phy_port_list))
                {
                    return SDN_HAL_RETURN_FAILED;
		}

                if (rtk_l2_mcastFwdIndex_alloc(unit_id, &flood_port_mask_id) != RT_ERR_OK
                    || rtk_l2_mcastFwdPortmask_set(unit_id, flood_port_mask_id, &phy_port_list, FALSE) != RT_ERR_OK)
                {
		    return SDN_HAL_RETURN_FAILED;
                }

		action->fwd_en            = ENABLED;
                action->fwd_data.fwd_info = flood_port_mask_id;
                action->fwd_data.fwd_type = (rtk_acl_egrActionFwdType_t)ACL_ACTION_FWD_REDIRECT_TO_PORTMASK;
                DBG_SDN("\r\n[%s-%d] output to port list(%x)\r\n", __FUNCTION__, __LINE__,
                                                             action->fwd_data.fwd_info);
            }
	    else
            {
                ohal_lib_logicalPort2phyPort(unit_id, flow_entry_p->action_data.port_no, &phy_port);
                action->fwd_en            = ENABLED;
                action->fwd_data.fwd_info = phy_port; /*todo: assign the specified port number*/
                action->fwd_data.fwd_type = (rtk_acl_egrActionFwdType_t)ACL_ACTION_FWD_REDIRECT_TO_PORTID;
                DBG_SDN("\r\n[%s-%d] output to %s (%d)\r\n", __FUNCTION__, __LINE__,
                                                            "phy port",
                                                             action->fwd_data.fwd_info);
            }
        }
        break;
#ifdef OHAL_ACTION_SUPPORT
        case OHAL_RULE_FLOW_ACTION_MIRROR:
            action->mirror_en                  = ENABLED;
            action->mirror_data.mirror_type    = ACL_ACTION_MIRROR_ORIGINAL;
            action->mirror_data.mirror_set_idx = flow_entry_p->action_data.mirror_id;
        break;
#endif

        case OHAL_RULE_FLOW_ACTION_RL:
        {
            uint32 asic_id = 0;
            /*TODO: insert egress acl rate limit profile
             */
             memset(&meter_entry, 0, sizeof(meter_entry));
	     meter_entry.index    = flow_entry_p->meter_id;
             meter_entry.type      = OHAL_METER_TYPE_DROP;
             meter_entry.rate_type = OHAL_METER_RATE_TYPE_BYTE;
             if (ohal_flow_ConvertMeterRateTo16KBps(flow_entry_p->action_data.rate_limit_data.mode,
                                                                                                flow_entry_p->action_data.rate_limit_data.rate,
                                                                                                &meter_entry.rate) != OHAL_RULE_ERROR_RETURN_OK)
             {
                 OHAL_LIB_RETURN_VALUE(OHAL_RULE_ERROR_RETURN_ADD_METER_ENTRY_FAILED);
             }

             if (OHAL_RULE_ERROR_RETURN_OK != ohal_meter_entry_is_exited(&meter_entry))
             {
                 if (RT_ERR_OK != ohal_meter_create(0, &meter_entry))
                 {
                      return SDN_HAL_RETURN_FAILED;
                 }
            }

             OHAL_FLOW_ID_TO_ASIC_ID(OHAL_RULE_GET_FLOW_TABLE_NUM(flow_entry_p->flow_id), flow_entry_p->flow_id, &asic_id);

             if (RT_ERR_OK != ohal_meter_flow_attach(0, OHAL_RULE_GET_FLOW_TABLE_NUM(flow_entry_p->flow_id), asic_id, flow_entry_p->meter_id))
             {
                 return SDN_HAL_RETURN_FAILED;
             }

             action->meter_data.meter_idx = flow_entry_p->meter_id;
	     action->meter_en             = ENABLED;
        }
        break;
#ifdef OHAL_ACTION_SUPPORT
        case OHAL_RULE_FLOW_ACTION_DSCPREMARK:
            action->rmk_en            = ENABLED;
            action->rmk_data.rmk_type = ACL_ACTION_REMARK_DSCP;
            action->rmk_data.rmk_value= flow_entry_p->action_data.remark_val;
        break;

        case OHAL_RULE_FLOW_ACTION_OUTERREMARK:
            action->rmk_en            = ENABLED;
            action->rmk_data.rmk_type = ACL_ACTION_REMARK_OUTER_USER_PRI;
            action->rmk_data.rmk_value= flow_entry_p->action_data.remark_val;
        break;
#endif

        case OHAL_RULE_FLOW_ACTION_CVID:
            action->inner_vlan_assign_en            = ENABLED;
            action->inner_vlan_data.vid_assign_type = ACL_EGR_ACTION_IVLAN_ASSIGN_NEW_VID;
            action->inner_vlan_data.vid_value       = flow_entry_p->action_data.cvid;
        break;

        case EN_OFPAT_POP_VLAN:
            action->outer_vlan_assign_en            = ENABLED;
            action->outer_vlan_data.vid_assign_type = ACL_EGR_ACTION_OVLAN_ASSIGN_NEW_VID;
            action->outer_vlan_data.vid_value       = flow_entry_p->action_data.svid;
        break;

        case EN_OFPAT_PUSH_VLAN:
            action->tag_sts_en            = ENABLED;
            action->tag_sts_data.itag_sts = ACL_ACTION_TAG_STS_TAG;
            action->inner_vlan_assign_en            = ENABLED;
            action->inner_vlan_data.vid_assign_type = ACL_EGR_ACTION_IVLAN_ASSIGN_NEW_VID;
            action->inner_vlan_data.vid_value       = flow_entry_p->action_data.cvid;
        break;

        case EN_OFPAT_SET_FIELD:
	    if (flow_entry_p->action_data.ofp_oxm_type == EN_OFPXMT_OFB_VLAN_VID)
            {
                if (flow_entry_p->action_data.eth_type != 0x81000  && flow_entry_p->action_data.eth_type != 0x88a8)
                {
                    return SDN_HAL_RETURN_FAILED;
                }

                action->tag_sts_en            = ENABLED;
                action->tag_sts_data.itag_sts = ACL_ACTION_TAG_STS_TAG;
                action->inner_vlan_assign_en            = ENABLED;
                action->inner_vlan_data.vid_assign_type = ACL_EGR_ACTION_IVLAN_ASSIGN_NEW_VID;
                action->inner_vlan_data.vid_value       = flow_entry_p->action_data.cvid;
            }
	    else if (flow_entry_p->action_data.ofp_oxm_type == EN_OFPXMT_OFB_IP_DSCP)
            {
                action->rmk_en            = ENABLED;
                action->rmk_data.rmk_type = ACL_ACTION_REMARK_DSCP;
                action->rmk_data.rmk_value= flow_entry_p->action_data.remark_val;
            }
	    else if (flow_entry_p->action_data.ofp_oxm_type == EN_OFPXMT_OFB_VLAN_PCP)
            {
                action->rmk_en            = ENABLED;
                action->rmk_data.rmk_type = ACL_ACTION_REMARK_OUTER_USER_PRI;
                action->rmk_data.rmk_value= flow_entry_p->action_data.remark_val;
            }
	break;
#if 0 /*unsupported*/
        case OHAL_RULE_FLOW_ACTION_CUNTAG:
            action->tag_sts_en            = ENABLED;
            action->tag_sts_data.itag_sts = ACL_ACTION_TAG_STS_UNTAG;
        break;

        case OHAL_RULE_FLOW_ACTION_STAG:
            action->tag_sts_en            = ENABLED;
            action->tag_sts_data.otag_sts = ACL_ACTION_TAG_STS_TAG;
        break;

        case OHAL_RULE_FLOW_ACTION_SUNTAG:
            action->tag_sts_en = ENABLED;
            action->tag_sts_data.otag_sts = ACL_ACTION_TAG_STS_UNTAG;
        break;
#endif
        default:
	    DBG_SDN("[%s-%d] table1 unknow action\r\n", __FUNCTION__, __LINE__);
            ;

    }

    if (flow_entry_p->counter_data.is_apply_counter == TRUE)
    {
         action->stat_data.stat_idx = flow_entry_p->counter_data.counter_id;
         action->stat_data.stat_type= (rtk_acl_statType_t)flow_entry_p->counter_data.counter_type;
         action->stat_en            = ENABLED;
    }
    else
    {
         action->stat_data.stat_type= (rtk_acl_statType_t)flow_entry_p->counter_data.counter_type; //STAT_TYPE_BYTE_BASED_64BIT;
         action->stat_en            = DISABLED;
    }

    OHAL_LIB_RETURN_VALUE(OHAL_RULE_ERROR_RETURN_OK);
#endif
}

int32 _sdn_hal_flow_aclAction_fill(sdn_hal_rule_flow_mgmt_t *flow_entry_p, sdn_hal_lib_action_t *acl_action_p)
{
    uint32 action_index = 0;
    rtk_acl_phase_t phase;

    SDN_PARAM_CHK(acl_action_p == NULL, SDN_HAL_RETURN_OK);

    phase = sdn_hal_rule_get_flow_table(flow_entry_p->flow_id);

    DBG_SDN("[%s-%d] action bitmask(%x)\r\n", __FUNCTION__, __LINE__, flow_entry_p->action_data.action_mask);

    for (action_index = 0; action_index < EN_OFPAT_STD_MAX; action_index++)
    {
        /* TODO : fill action to action_p*/
        if (!HAL_LIB_CHK_BIT_ON(flow_entry_p->action_data.action_mask, action_index))
        {
            continue;
        }
        switch(phase)
        {
            case ACL_PHASE_VACL:
                DBG_SDN("[%s-%d] table0 action set\r\n", __FUNCTION__, __LINE__);
                _sdn_hal_flow_ingressAction_assign (action_index, flow_entry_p, acl_action_p);
                break;
            case ACL_PHASE_IACL:
                DBG_SDN("[%s-%d] table1 action set\r\n", __FUNCTION__, __LINE__);
                _sdn_hal_flow_egressAction_assign (action_index, flow_entry_p, acl_action_p);
                break;
            default:
                break;
            /*TODO : table */
        }
    }

    return SDN_HAL_RETURN_OK;
}

int32 _meter_transform(sdn_hal_rule_meter_mgmt_t *meter_data, const ofp_meter_mod_t *meter)
{
	meter_data->meter_id = SDN_HAL_METER_INDEX(meter->meter_id);
    EN_OFP_METER_FLAGS_T   config_counter_mode = EN_OFPMF_STATS;
#if 0
    if (opal_to_hoal_get_systemcountermode(&config_counter_mode) != OPAL_RETURN_OK
       || (meter->flags != config_counter_mode)
       )
    {
        return SDN_HAL_RETURN_NOT_SUPPORTED;
    }
#endif
	switch(meter->flags)
	{
		case EN_OFPMF_KBPS:
		{
			meter_data->meter_flags = SDN_HAL_RULE_METER_FLAG_KBPS;
			break;
		}
		case EN_OFPMF_PKTPS:
		{
			meter_data->meter_flags = SDN_HAL_RULE_METER_FLAG_PKTPS;
			break;
		}
		default:
		{
			/* Others are unsupported */
			return SDN_HAL_RETURN_NOT_SUPPORTED;
		}
	}

	if(meter->n_bands == 0)
	{
		return SDN_HAL_RETURN_INVALID_PARAM;
	}

	switch(meter->bands[0].type)
	{
		case EN_OFPMBT_DROP:
		{
			meter_data->meter_band_type = SDN_HAL_RULE_METER_BAND_DROP;
			break;
		}
		case EN_OFPMBT_DSCP_REMARK:
		{
			meter_data->meter_band_type = SDN_HAL_RULE_METER_BAND_REMARK;
			break;
		}
		default:
		{
			/* Others are unsupported */
			return SDN_HAL_RETURN_NOT_SUPPORTED;
		}
	}
	meter_data->meter_rate = meter->bands[0].rate;
	meter_data->meter_burst_size = meter->bands[0].burst_size;

	return SDN_HAL_RETURN_OK;
}

int32 sdn_hal_meter_entry_add(ofp_meter_mod_t *meter_entry)
{
    uint32 ret = SDN_HAL_RETURN_FAILED;
    uint32 unit = SDN_HAL_MY_UNIT_ID();
    sdn_hal_rule_meter_mgmt_t meter_data;
    rtk_acl_meterEntry_t meter;

    if (meter_entry->meter_id >= SDN_HAL_RTK_METER_SIZE)
        return SDN_HAL_RETURN_FAILED;

    if(SDN_HAL_RETURN_OK != _meter_transform(&meter_data, meter_entry))
        return SDN_HAL_RETURN_FAILED;

    if (meter_data.meter_band_type != EN_OFPMBT_DROP)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    memset(&meter, 0x0, sizeof(rtk_acl_meterEntry_t));

    if ((ret = rtk_pie_meterEntry_get(unit, meter_data.meter_id, &meter)) == RT_ERR_OK && meter.type == METER_TYPE_INVALID)
    {
        meter.type = METER_TYPE_DLB;
        meter.color_aware    = 0;
        meter.lb0_rate = meter_data.meter_rate;    /* rate granularity is 16Kbps, or 1pps */
        meter.lb1_rate = meter_data.meter_rate;    /* rate granularity is 16Kbps, or 1pps */
        ret = rtk_pie_meterEntry_set(unit, meter_data.meter_id, &meter);
    }

    return SDN_HAL_RETURN_OK;

}
int32 sdn_hal_meter_entry_del(uint32 meter_id)
{
    uint32 ret = SDN_HAL_RETURN_FAILED;
    uint32 unit = 0;
    rtk_acl_meterEntry_t meter;

    unit = SDN_HAL_MY_UNIT_ID();

    if (meter_id < SDN_HAL_RTK_METER_SIZE)
    {
        memset(&meter, 0x0, sizeof(rtk_acl_meterEntry_t));
        ret = rtk_pie_meterEntry_set(unit, meter_id, &meter);
    }

    return SDN_HAL_RETURN_OK;
}
int32 sdn_hal_meter_entry_modify(ofp_meter_mod_t *meter_entry)
{
    uint32 ret = SDN_HAL_RETURN_FAILED;
    uint32 unit = SDN_HAL_MY_UNIT_ID();
    sdn_hal_rule_meter_mgmt_t meter_data;
    rtk_acl_meterEntry_t meter;

    if (meter_entry->meter_id >= SDN_HAL_RTK_METER_SIZE)
        return SDN_HAL_RETURN_FAILED;

    if(SDN_HAL_RETURN_OK != _meter_transform(&meter_data, meter_entry))
        return SDN_HAL_RETURN_FAILED;

    if (meter_data.meter_band_type != EN_OFPMBT_DROP)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    memset(&meter, 0x0, sizeof(rtk_acl_meterEntry_t));

    if ((ret = rtk_pie_meterEntry_get(unit, meter_data.meter_id, &meter)) == RT_ERR_OK && meter.type == METER_TYPE_INVALID)
    {
        meter.type = METER_TYPE_DLB;
        meter.color_aware = 0;
        meter.lb0_rate = meter_data.meter_rate;    /* rate granularity is 16Kbps, or 1pps */
        meter.lb1_rate = meter_data.meter_rate;    /* rate granularity is 16Kbps, or 1pps */
        ret = rtk_pie_meterEntry_set(unit, meter_data.meter_id, &meter);
    }

    return SDN_HAL_RETURN_OK;
}
int32 sdn_hal_meter_table_features_get(sdn_db_meter_table_features_t *feature)
{
    uint32 unit = SDN_HAL_MY_UNIT_ID();

    feature->max_meters = SDN_HAL_RTK_METER_SIZE;
    feature->max_bands  = 1;
    feature->band_types = 1;
    feature->max_colors = 0;

    return SDN_HAL_RETURN_OK;
}

