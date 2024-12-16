#include <fcntl.h>
#include "rtcore/rtcore.h"
#include "sys/ioctl.h"
//#include "drv/nic/common.h"
#include "private/drv/nic/nic_common.h"
#include "sdn_hal.h"
#include "rtk/acl.h"

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
    {USR_PORT_25, UNIT_ID_0, PHY_PORT_24},
    {USR_PORT_26, UNIT_ID_0, PHY_PORT_25},
    {USR_PORT_27, UNIT_ID_0, PHY_PORT_26},
    {USR_PORT_28, UNIT_ID_0, PHY_PORT_27},
    {USR_PORT_29, UNIT_ID_0, PHY_PORT_28},/*CPU port*/
   /* Do not remove end entry */
    {PORTCONF_END_VAL, PORTCONF_END_VAL, PORTCONF_END_VAL}
};

#ifdef  CONFIG_SDK_RTL9310
static int32 sdn_hal_ofactionconverttodriver(sdn_hal_apply_actions_t *of_actionbucket_p, rtk_of_actionBucket_t *hal_actionbucket_p);
#endif

static int sdn_hal_table_table0_template_bind(uint32_t unit)
{
    uint32_t    shard_template_id  = SDN_HAL_ACL_SHARED_TEMPLATE_ID;
    uint32_t    ingress_template_id= SDN_HAL_ACL_TABLE0_TEMPLATE_ID;
    uint32_t    shard_block_id     = 1;
    uint32_t    ingress_block_id   = 0;
    rtk_acl_templateIdx_t  template_idx_info;

    memset(&template_idx_info, 0, sizeof(template_idx_info));

    template_idx_info.template_id[shard_block_id] = shard_template_id;

    template_idx_info.template_id[ingress_block_id] = ingress_template_id;

//    for (ingress_block_id = OHAL_RULE_ACL_START_BLK_ID_OF_TABLE_0; ingress_block_id < OHAL_RULE_ACL_START_BLK_ID_OF_TABLE_1; ingress_block_id++)
 for (ingress_block_id = SDN_HAL_ACL_START_BLOCK_ID_OF_TABLE_0; ingress_block_id < SDN_HAL_ACL_BLOACK_SIZE; ingress_block_id++)
    {
        if (rtk_acl_templateSelector_set(unit, ingress_block_id, template_idx_info) != RT_ERR_OK)
        {
            //return RT_ERR_FAILED;
            return  -3;
        }
    }

    return RT_ERR_OK;
}

int sdn_hal_init(struct ofp_table_features tf[MAX_TABLE_NUM])
{
//FIXME:not finish
    int ret = RT_ERR_OK;
    uint32_t unit = 1;
    uint32_t block_id = SDN_HAL_ACL_START_BLOCK_ID_OF_TABLE_0;
    rtk_acl_blockGroup_t group_type = ACL_BLOCK_GROUP_4;
#if 0
    uint32_t shard_template_id  = SDN_HAL_ACL_SHARED_TEMPLATE_ID;
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
        return RT_ERR_FAILED;
    }
#endif

    if (sdn_hal_table_table0_template_bind(unit) != RT_ERR_OK)
    {
        return RT_ERR_FAILED;
    }

    /* 2. Adjust reserved ACL */

    return ret;
}

/* Table Feature */
int sdn_hal_table_features_set(uint8_t n_table, const struct ofp_table_features *tf)
{
    int ret = RT_ERR_OK;

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
	              uint32_t pkt_size,
                  rtk_of_outputType_t *out_put_type,
                  uint32_t port_no)
{
   // int32 fd;
   // rtcore_ioctl_t dio;
    drv_nic_pkt_t *pPacket = NULL;
    rtk_of_outputType_t output_type = *out_put_type;
    void *pkt_pl_p = NULL;

    printf("\r\n[%s:%d] devname(%s)", __FUNCTION__, __LINE__, dev_name);
    if (pkt_p == NULL)
    {
        return RT_ERR_FAILED;
    }

    pPacket = (drv_nic_pkt_t *)malloc(sizeof(drv_nic_pkt_t));

    if (pPacket == NULL)
    {
        return RT_ERR_FAILED;
    }

    memset(pPacket, 0, sizeof(drv_nic_pkt_t));


    pkt_pl_p = malloc(pkt_size);

    if (pkt_pl_p)
    {
        free(pPacket);
        return RT_ERR_FAILED;
    }

    memcpy(pkt_pl_p, pkt_p, pkt_size);
    pPacket->length = pkt_size;
    pPacket->head   = (uint8_t *)pkt_pl_p;
    pPacket->tail   = pPacket->data + pPacket->length;
    pPacket->end    = pPacket->tail;
    if (OF_OUTPUT_TYPE_PHY_PORT == output_type)
    {
        pPacket->tx_tag.dst_port_mask = (0x00000001 << port_no);
    }
    
//#ifdef __linux__
//    if ((fd = open(RTCORE_DEV_NAME, O_RDWR)) < 0)
//    {
//        return RT_ERR_FAILED;
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
        return RT_ERR_FAILED;
    }
#endif
    
//    if (dio.ret)
//    {
//        return RT_ERR_FAILED;
//    }
    

    return RT_ERR_OK;
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
    return RT_ERR_OK;
}

int32 sdn_hal_add_group(uint32 group_id, rtk_of_groupType_t type, sdn_hal_group_table_t *entry_p)
{
    uint32 entry_id = 0;
    rtk_of_groupEntry_t group_entry;
    rtk_of_actionBucket_t drv_action_bucket_entry;
    sdn_db_apply_actions_t *action_bucket_p = NULL;

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
        return RT_ERR_FAILED;
    }

    if (sdn_hal_ofactionconverttodriver(action_bucket_p, &drv_action_bucket_entry) != RT_ERR_OK)
    {
        return RT_ERR_FAILED;
    }
#endif

    return RT_ERR_OK;
}

int32 sdn_hal_get_group(uint32 group_id, rtk_of_groupType_t *type, uint32 *bucket_num_p, sdn_hal_apply_actions_t **applied_action_list_p)
{
    if (bucket_num_p == NULL
        ||applied_action_list_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    return RT_ERR_OK;
}

int32 sdn_hal_delete_group(uint32 group_id)
{
    rtk_of_groupEntry_t group_entry;

    memset(&group_entry, 0, sizeof(group_entry));
#ifdef RTK_SDK_OK
    if (rtk_of_groupEntry_set(0, group_id, &group_entry) != RT_ERR_OK)
    {
        return RT_ERR_FAILED;
    }
#endif
    return RT_ERR_OK;
}

/* Classifier */
int sdn_hal_classifier_set(rtk_of_classifierType_t type, rtk_of_classifierData_t data)
{
//FIXME:not finish
    int ret = RT_ERR_OK;
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
        (phy_port > SDN_MAX_LOGICAL_AND_CPU_PORT_NUM))
    {
        return RT_ERR_INPUT;
    }

    for(port_index=0; port_index < _countof(sdn_hal_port_map); port_index++)
    {
        if (sdn_hal_port_map[port_index].devId != unit)
            continue;

        if (sdn_hal_port_map[port_index].phyPort == phy_port)
        {
            *logical_port_p = port_index;
            return RT_ERR_OK;
        }
    }

    return RT_ERR_FAILED;
}

int32 sdn_hal_logicalPort2PhyPort(uint32 unit, uint32 logical_port, uint32 *phy_port_p)
{
    if (phy_port_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    if ((logical_port < 0) ||
        (logical_port >= _countof(sdn_hal_port_map)))
    {
        return RT_ERR_INPUT;
    }

    if (sdn_hal_port_map[logical_port].devId != unit)
        return RT_ERR_FAILED;

    if (sdn_hal_port_map[logical_port].phyPort != PORTCONF_END_VAL)
    {
        *phy_port_p = sdn_hal_port_map[logical_port].phyPort;
        return RT_ERR_OK;
    }

    return RT_ERR_FAILED;
}

int32 sdn_hal_phyPort2UserPort(uint32 unit, uint32 phy_port, uint32 *user_port_p)
{
    uint32 port_index = 0;

    if (user_port_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    if ((phy_port < PHY_PORT_1) ||
        (phy_port > SDN_MAX_LOGICAL_AND_CPU_PORT_NUM))
    {
        return RT_ERR_INPUT;
    }

    for(port_index=0; port_index < _countof(sdn_hal_port_map); port_index++)
    {
        if (sdn_hal_port_map[port_index].devId != unit)
            continue;

        if (sdn_hal_port_map[port_index].phyPort == phy_port)
        {
            *user_port_p = sdn_hal_port_map[port_index].usrPort;
            return RT_ERR_OK;
        }
    }

    return RT_ERR_FAILED;
}

int32 sdn_hal_userPort2PhyPort(uint32 unit, uint32 user_port, uint32 *phy_port_p)
{
    uint32 port_index = 0;

    if (phy_port_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    if ((user_port < USR_PORT_1) ||
        (user_port > SDN_MAX_LOGICAL_AND_CPU_PORT_NUM))
    {
        return RT_ERR_INPUT;
    }

    for(port_index=0; port_index < _countof(sdn_hal_port_map); port_index++)
    {
        if (sdn_hal_port_map[port_index].devId != unit)
            continue;

        if (sdn_hal_port_map[port_index].usrPort == user_port)
        {
            *phy_port_p = sdn_hal_port_map[port_index].phyPort;
            return RT_ERR_OK;
        }
    }

    return RT_ERR_FAILED;
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
        return RT_ERR_INPUT;
    }

    if (sdn_hal_port_map[logical_port].devId != unit)
        return RT_ERR_FAILED;

    if (sdn_hal_port_map[logical_port].usrPort != PORTCONF_END_VAL)
    {
            *user_port_p = sdn_hal_port_map[logical_port].usrPort;
            return RT_ERR_OK;
    }

    return RT_ERR_FAILED;
}

int32 sdn_hal_userPort2LogicalPort(uint32 unit, uint32 user_port, uint32 *logical_port_p)
{
    uint32 port_index = 0;

    if (logical_port_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    if ((user_port < USR_PORT_1) ||
        (user_port > SDN_MAX_LOGICAL_AND_CPU_PORT_NUM))
    {
        return RT_ERR_INPUT;
    }

    for(port_index=0; port_index < _countof(sdn_hal_port_map); port_index++)
    {
        if (sdn_hal_port_map[port_index].devId != unit)
            continue;

        if (sdn_hal_port_map[port_index].usrPort == user_port)
        {
            *logical_port_p = port_index;
            return RT_ERR_OK;
        }
    }

    return RT_ERR_FAILED;
}



/* Counter */
int sdn_hal_flow_entry_counter_apply(uint8_t table_id,
                                     uint32_t flow_id,
                                     uint32_t count_id,
                                     rtk_of_flowCntMode_t mode)
{
    /* 1. Apply counter to chip */

#if 1
    /*Brandon:
        1.need to convert table_id to phase.
        2. flow_id convert to physical entry_idx.
        3. count_id doesn't be used by sdk  ,  why not  ????!!!
    */
    int ret = RT_ERR_OK;
    int unit = 0;

/*
    if(( ret = rtk_of_flowCntMode_set(unit, (rtk_of_flowtable_phase_t) table_id, (rtk_of_flow_id_t) flow_id, mode)) != 0)
    {
        printf("[%s][%d] fail to get flow count mode, ret=[%d]. \n", __FUNCTION__, __LINE__, ret);
        return ret;
    }
*/

    return ret;

#endif

}

int sdn_hal_flow_entry_counter_delete(uint8_t table_id,
                                      uint32_t flow_id,
                                      uint32_t count_id)
{
    /* 1. Delete counter to chip */

    /*Brandon: don't know which sdk API. */

}

int sdn_hal_flow_entry_counter_clear(uint8_t table_id,
                                     uint32_t flow_id,
                                     uint32_t count_id)
{
    /* 1. Clear counter to chip */

#if 1
    int ret = RT_ERR_OK;
    int unit = 0;
    rtk_of_flowCntMode_t  cnt_mode;
    /*Brandon: counter type need transfer from upper layer.*/

/*
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
                break;

            if((ret = rtk_of_flowCnt_get(unit , (rtk_of_flowtable_phase_t) table_id, (rtk_of_flow_id_t)  flow_id,
                                         OF_FLOW_CNT_TYPE_BYTE)) != RT_ERR_OK)
                break;

            break;

        case  OF_FLOW_CNTMODE_PACKET_CNT_AND_PACKET_CNT_TH:
            if(( ret = rtk_of_flowCnt_clear(unit , (rtk_of_flowtable_phase_t) table_id, (rtk_of_flow_id_t)  flow_id,
                                            OF_FLOW_CNT_TYPE_PACKET) ) != RT_ERR_OK)
                break;
            break;
        case OF_FLOW_CNTMODE_BYTE_CNT_AND_BYTE_CNT_TH:
            if((ret = rtk_of_flowCnt_get(unit , (rtk_of_flowtable_phase_t) table_id, (rtk_of_flow_id_t)  flow_id,
                                         OF_FLOW_CNT_TYPE_BYTE)) != RT_ERR_OK)
                break;

            break;
        default:
            break;
    }

*/

    if (ret != RT_ERR_OK)
    {
        printf("[%s][%d] fail to get flow count , ret=[%d]. \n", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    return ret;
#endif

}

int sdn_hal_flow_table_stats_get(uint8_t table_id, rtk_of_flowTblCntType_t type, uint32 *pCnt)
{
    /* 1. Get counter of flow table from chip */

#if 1  /* Brandon add: */

    int ret = 0;
    int unit = 0;
    
    if (pCnt == NULL)
        return RT_ERR_NULL_POINTER;

/*
    if (( ret = rtk_of_flowTblCnt_get(unit, (rtk_of_flowtable_phase_t) table_id, type, pCnt)) != 0)
    {
        printf("[%s][%d] fail to get flow count mode, ret=[%d]. \n", __FUNCTION__, __LINE__, ret);
        return ret;
    }
*/
    return ret;

#endif

}

#if 1   //Brandon modify
int sdn_hal_flow_stats_get(uint8_t table_id,
		                   uint16_t priority,
		                   uint8_t match_num,
		                   ofp_match *match,
		                   uint64_t *packet_count,
		                   uint64_t *byte_count)
{
    /* 1. Get flow counter from chip */

    /*
    1. table_id convert to phase
    2. according to match, convert match to flow_id. (maybe refer to sdn_db)

    int32 rtk_of_flowCnt_get(uint32 unit, rtk_of_flowtable_phase_t phase, rtk_of_flow_id_t  entry_idx,
                                                rtk_of_flowCntType_t type, uint64 *pCnt);
    */

    if (match == NULL)
        return RT_ERR_NULL_POINTER;
    
}

#else
int sdn_hal_flow_counter_get(uint8_t table_id,
                             uint16_t priority,
                             uint8_t match_num,
                             ofp_match *match,
                             uint64_t *packet_count,
                             uint64_t *byte_count)
{
    /* 1. Get flow counter from chip */
}

#endif

#if 1  //Brandon add:

int sdn_hal_flow_stats_periodic_get(uint8_t table_id,
                                    uint32_t flow_id,
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


int sdn_hal_port_stats_get(uint32_t port_no, hal_port_stats_t *pPort_stats_data )
{

    /* call rtk_stat_port_getAll()
    need convert port_no(user_port) to phy_port !
    */
    int ret = -1;
    int unit = 0;
    rtk_stat_port_cntr_t port_counter;
    uint32_t phy_port;

    if (pPort_stats_data == NULL)
        return RT_ERR_NULL_POINTER;

    memset(&port_counter, 0x0, sizeof(rtk_stat_port_cntr_t));

    if ((ret = rtk_stat_port_getAll(unit, port_no, &port_counter)) == 0)
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

