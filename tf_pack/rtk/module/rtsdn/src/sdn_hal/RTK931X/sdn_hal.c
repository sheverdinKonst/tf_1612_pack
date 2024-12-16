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
#include "sys/ioctl.h"
#include "private/drv/nic/nic_common.h"
#include "sdn_hal.h"
#include "common/type.h"
#include "rtk/switch.h"
#include "rtk/acl.h"
#include "rtk/port.h"
#include "rtk/vlan.h"
//#ifdef of_table_features
#include "ovs.h"
//#endif
#include "dal/rtrpc/rtrpc_switch.h"
#include "dal/rtrpc/rtrpc_acl.h"
#include "dal/rtrpc/rtrpc_port.h"
#ifdef of_table_features
#include "rtrpc_ovs.h"
#endif
#include "dal/rtrpc/rtrpc_pie.h"
#include "dal/rtrpc/rtrpc_vlan.h"
#include "dal/rtrpc/rtrpc_rate.h"
#include "dal/rtrpc/rtrpc_counter.h"
#include "sdn_util.h"
#include "sdn_db.h"
#include <common/util/rt_util.h>
#include "sdn_hal_common.h"
#include <rtk/openflow.h>
#ifdef CONFIG_SYS_LIB_PKT_IN_OUT
#include <private/drv/nic/nic_pkt2usr.h>
#endif

#define SDN_HAL_GET_ASIC_GROUPID(_id) ( ((_id >= 1) ? _id - 1 : 0) )

#define RTK_SDN_931X_CPU_PORT         (PHY_PORT_56 + 1)  /* Mapping phyical to SDN USER */

sdn_hal_flow_table_feature_t sdn_hal_table_feature[SDN_HAL_MAX_TABLE_NUM + 1];
uint32   cpu_port_id = SDN_HAL_CPU_PORT; /*todo: need to get from chip */
uint32_t g_total_actions_bucket_num = 0;
sdn_hal_tpid_t g_tpid[SDN_HAL_TPID_NUM_MAX];
uint32 l3_cam_logic_id[SDN_HAL_INGRESS_L3_TABLE2_CAM_RULE_NUM + 1];
uint32 l3_cam_flow_count = 0;
sdn_hal_portmask_t   org_isolate_port_1;
sdn_hal_portmask_t   org_isolate_port_2;

const sdn_hal_port_cfg_t sdn_hal_port_map[] =
{
    {USR_PORT_1 , UNIT_ID_0, PHY_PORT_0},
    {USR_PORT_2 , UNIT_ID_0, PHY_PORT_1},
    {USR_PORT_3 , UNIT_ID_0, PHY_PORT_2},
    {USR_PORT_4 , UNIT_ID_0, PHY_PORT_3},
    {USR_PORT_5 , UNIT_ID_0, PHY_PORT_4},
    {USR_PORT_6 , UNIT_ID_0, PHY_PORT_5},
    {USR_PORT_7 , UNIT_ID_0, PHY_PORT_6},
    {USR_PORT_8 , UNIT_ID_0, PHY_PORT_7},
    {USR_PORT_9 , UNIT_ID_0, PHY_PORT_8},
    {USR_PORT_10, UNIT_ID_0, PHY_PORT_9},
    {USR_PORT_11, UNIT_ID_0, PHY_PORT_10},
    {USR_PORT_12, UNIT_ID_0, PHY_PORT_11},
    {USR_PORT_13, UNIT_ID_0, PHY_PORT_12},
    {USR_PORT_14, UNIT_ID_0, PHY_PORT_13},
    {USR_PORT_15, UNIT_ID_0, PHY_PORT_14},
    {USR_PORT_16, UNIT_ID_0, PHY_PORT_15},
    {USR_PORT_17, UNIT_ID_0, PHY_PORT_16},
    {USR_PORT_18, UNIT_ID_0, PHY_PORT_17},
    {USR_PORT_19, UNIT_ID_0, PHY_PORT_18},
    {USR_PORT_20, UNIT_ID_0, PHY_PORT_19},
    {USR_PORT_21, UNIT_ID_0, PHY_PORT_20},
    {USR_PORT_22, UNIT_ID_0, PHY_PORT_21},
    {USR_PORT_23, UNIT_ID_0, PHY_PORT_22},
    {USR_PORT_24, UNIT_ID_0, PHY_PORT_23},
    {USR_PORT_25, UNIT_ID_0, PHY_PORT_24},
    {USR_PORT_26, UNIT_ID_0, PHY_PORT_25},
    {USR_PORT_27, UNIT_ID_0, PHY_PORT_26},
    {USR_PORT_28, UNIT_ID_0, PHY_PORT_27},
    {USR_PORT_29, UNIT_ID_0, PHY_PORT_28},
    {USR_PORT_30, UNIT_ID_0, PHY_PORT_29},
    {USR_PORT_31, UNIT_ID_0, PHY_PORT_30},
    {USR_PORT_32, UNIT_ID_0, PHY_PORT_31},
    {USR_PORT_33, UNIT_ID_0, PHY_PORT_32},
    {USR_PORT_34, UNIT_ID_0, PHY_PORT_33},
    {USR_PORT_35, UNIT_ID_0, PHY_PORT_34},
    {USR_PORT_36, UNIT_ID_0, PHY_PORT_35},
    {USR_PORT_37, UNIT_ID_0, PHY_PORT_36},
    {USR_PORT_38, UNIT_ID_0, PHY_PORT_37},
    {USR_PORT_39, UNIT_ID_0, PHY_PORT_38},
    {USR_PORT_40, UNIT_ID_0, PHY_PORT_39},
    {USR_PORT_41, UNIT_ID_0, PHY_PORT_40},
    {USR_PORT_42, UNIT_ID_0, PHY_PORT_41},
    {USR_PORT_43, UNIT_ID_0, PHY_PORT_42},
    {USR_PORT_44, UNIT_ID_0, PHY_PORT_43},
    {USR_PORT_45, UNIT_ID_0, PHY_PORT_44},
    {USR_PORT_46, UNIT_ID_0, PHY_PORT_45},
    {USR_PORT_47, UNIT_ID_0, PHY_PORT_46},
    {USR_PORT_48, UNIT_ID_0, PHY_PORT_47},
    {USR_PORT_49, UNIT_ID_0, PHY_PORT_48},
    {USR_PORT_50, UNIT_ID_0, PHY_PORT_49},
    {USR_PORT_51, UNIT_ID_0, PHY_PORT_50},
    {USR_PORT_52, UNIT_ID_0, PHY_PORT_51},
    {USR_PORT_53, UNIT_ID_0, PHY_PORT_52},
    {USR_PORT_54, UNIT_ID_0, PHY_PORT_53},
    {USR_PORT_55, UNIT_ID_0, PHY_PORT_54},
    {USR_PORT_56, UNIT_ID_0, PHY_PORT_55},
    {RTK_SDN_931X_CPU_PORT, UNIT_ID_0, PHY_PORT_56},
   /* Do not remove end entry */
    {PORTCONF_END_VAL, PORTCONF_END_VAL, PORTCONF_END_VAL}
};

/*
 * Data Declaration
 */
sdn_hal_portmask_t all_portMask[RTK_MAX_NUM_OF_UNIT];
uint32 unit_list[RTK_MAX_NUM_OF_UNIT];
//new group code
static sdn_hal_group_feature_t g_sdn_hal_group_feature[] =
 {
    {
        .group_type = EN_OFPGT_ALL,
        .actions_feature = (EN_OFPAT_OUTPUT | EN_OFPAT_PUSH_VLAN | EN_OFPAT_POP_VLAN | EN_OFPAT_PUSH_MPLS | EN_OFPAT_POP_MPLS | EN_OFPAT_SET_QUEUE | EN_OFPAT_GROUP | EN_OFPAT_DEC_MPLS_TTL | EN_OFPAT_DEC_NW_TTL | EN_OFPAT_SET_FIELD ),
    },
    {
        .group_type = EN_OFPGT_SELECT,
        .actions_feature = (EN_OFPAT_OUTPUT | EN_OFPAT_PUSH_VLAN | EN_OFPAT_POP_VLAN | EN_OFPAT_PUSH_MPLS | EN_OFPAT_POP_MPLS | EN_OFPAT_SET_QUEUE | EN_OFPAT_GROUP | EN_OFPAT_DEC_MPLS_TTL | EN_OFPAT_DEC_NW_TTL | EN_OFPAT_SET_FIELD ),
    },
 };
//new group code

static BOOL_T is_inport_existed = FALSE;
static sdn_hal_table_supported_field_t sdn_hal_table_match_field_table[] =
{
    {
        .fields = EN_OFPXMT_OFB_IN_PORT,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3)),
    },
    {
        .fields = EN_OFPXMT_OFB_IN_PHY_PORT,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3)),
    },
    {
        .fields = EN_OFPXMT_OFB_METADATA,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_ETH_DST,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(1) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_ETH_SRC,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(1) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_ETH_TYPE,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_VLAN_VID,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(1) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_VLAN_PCP,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_IP_DSCP,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_IP_ECN,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_IP_PROTO,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_IPV4_SRC,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(1) | SDN_HAL_TABLE(2) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),

    },
    {
        .fields = EN_OFPXMT_OFB_IPV4_DST,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(1) | SDN_HAL_TABLE(2) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_TCP_SRC,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(1) | SDN_HAL_TABLE(2) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_TCP_DST,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(1) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_UDP_SRC,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(1) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_UDP_DST,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(1) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_SCTP_SRC,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_SCTP_DST,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_ICMPV4_TYPE,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_ICMPV4_CODE,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_ARP_OP,
        .table_id_bitmap = SDN_HAL_TABLE_ID_BITMAP_NONE,
    },
    {
        .fields = EN_OFPXMT_OFB_ARP_SPA,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_ARP_TPA,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_ARP_SHA,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_ARP_THA,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_IPV6_SRC,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_IPV6_DST,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_IPV6_FLABEL,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_ICMPV6_TYPE,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_ICMPV6_CODE,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .fields = EN_OFPXMT_OFB_IPV6_ND_TARGET,
        .table_id_bitmap = SDN_HAL_TABLE_ID_BITMAP_NONE,
    },
    {
        .fields = EN_OFPXMT_OFB_IPV6_ND_SLL,
        .table_id_bitmap = SDN_HAL_TABLE_ID_BITMAP_NONE,
    },
    {
        .fields = EN_OFPXMT_OFB_IPV6_ND_TLL,
        .table_id_bitmap = SDN_HAL_TABLE_ID_BITMAP_NONE,
    },
    {
        .fields = EN_OFPXMT_OFB_MPLS_LABEL,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3)),
    },
    {
        .fields = EN_OFPXMT_OFB_MPLS_TC,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3)),
    },
    {
        .fields = EN_OFPXMT_OFB_MPLS_BOS,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3)),
    },
    {
        .fields = EN_OFPXMT_OFB_PBB_ISID,
        .table_id_bitmap = SDN_HAL_TABLE_ID_BITMAP_NONE,
    },
    {
        .fields = EN_OFPXMT_OFB_TUNNEL_ID,
        .table_id_bitmap = SDN_HAL_TABLE_ID_BITMAP_NONE,
    },
    {
        .fields = EN_OFPXMT_OFB_IPV6_EXTHDR,
        .table_id_bitmap = SDN_HAL_TABLE_ID_BITMAP_NONE,
    },
    /*
    * Realtek defined
    */
    {
        .fields = EN_OFPXMT_RTK_EXP_GTP_TEID,
        .table_id_bitmap = (SDN_HAL_TABLE(0)),
    },
    {
        .fields = EN_OFPXMT_RTK_L2_INNER_IP_VER,
        .table_id_bitmap = (SDN_HAL_TABLE(0)),
    },
    {
        .fields = EN_OFPXMT_RTK_OUTER_TCP_FLG,
        .table_id_bitmap = (SDN_HAL_TABLE(0)),
    },
    {
        .fields = EN_OFPXMT_RTK_EXP_GTP_MT,
        .table_id_bitmap = (SDN_HAL_TABLE(0)),
    },
    {
        .fields = EN_OFPXMT_RTK_OUTER_IP_FLG,
        .table_id_bitmap = (SDN_HAL_TABLE(3)),
    },
    {
        .fields = EN_OFPXMT_RTK_INNER_IP_VER,
        .table_id_bitmap = (SDN_HAL_TABLE(3)),
    },
    {
        .fields = EN_OFPXMT_RTK_INNER_L4DP,
        .table_id_bitmap = (SDN_HAL_TABLE(3)),
    },
    {
        .fields = EN_OFPXMT_RTK_INNER_SIP,
        .table_id_bitmap = (SDN_HAL_TABLE(3)),
    },
    {
        .fields = EN_OFPXMT_RTK_INNER_DIP,
        .table_id_bitmap = (SDN_HAL_TABLE(3)),
    },
    {
        .fields = EN_OFPXMT_RTK_OUTER_FRAG_PKT,
        .table_id_bitmap = (SDN_HAL_TABLE(0)),
    },
    {
        .fields = EN_OFPXMT_RTK_INNER_IP_PROTO,
        .table_id_bitmap = (SDN_HAL_TABLE(3)),
    },
    {
        .fields = EN_OFPXMT_RTK_INNER_TCP_FLAG,
        .table_id_bitmap = (SDN_HAL_TABLE(3)),
    },
    {
        .fields = EN_OFPXMT_RTK_OVER_PKT_SIZE,
        .table_id_bitmap = (SDN_HAL_TABLE(0)),
    },
    /*
    * Estinet defined
    */
    {
        .fields = EN_OFPXMT_EXP_VM_TAG,
        .table_id_bitmap = SDN_HAL_TABLE_ID_BITMAP_NONE,
    },
    {
        .fields = EN_OFPXMT_EXP_VM_ID,
        .table_id_bitmap = SDN_HAL_TABLE_ID_BITMAP_NONE,
    },
    {
        .fields = EN_OFPXMT_EXP_VM_SEGMENT_ID,
        .table_id_bitmap = SDN_HAL_TABLE_ID_BITMAP_NONE,
    },
};

static sdn_hal_table_supported_action_t sdn_hal_table_action_table[] =
{
    {
        .actions         = EN_OFPAT_OUTPUT,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(1) | SDN_HAL_TABLE(2) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .actions         = EN_OFPAT_PUSH_VLAN,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(1) | SDN_HAL_TABLE(2) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .actions         = EN_OFPAT_POP_VLAN,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(1) | SDN_HAL_TABLE(2) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .actions         = EN_OFPAT_SET_FIELD, /*need to check.*/
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(1) | SDN_HAL_TABLE(2) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .actions         = EN_OFPAT_COPY_TTL_OUT,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3)),
    },
    {
        .actions         = EN_OFPAT_COPY_TTL_IN,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3)),
    },
    {
        .actions         = EN_OFPAT_SET_MPLS_TTL,
        .table_id_bitmap = SDN_HAL_TABLE_ID_BITMAP_NONE,
    },
    {
        .actions         = EN_OFPAT_DEC_MPLS_TTL,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3)),
    },
    {
        .actions         = EN_OFPAT_PUSH_MPLS,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3)),
    },
    {
        .actions         = EN_OFPAT_POP_MPLS,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3)),
    },
    {
        .actions         = EN_OFPAT_SET_QUEUE,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(1) | SDN_HAL_TABLE(2) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .actions         = EN_OFPAT_GROUP,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(1) | SDN_HAL_TABLE(2) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .actions         = EN_OFPAT_SET_NW_TTL,
        .table_id_bitmap = SDN_HAL_TABLE_ID_BITMAP_NONE,
    },
    {
        .actions         = EN_OFPAT_DEC_NW_TTL,
        .table_id_bitmap = SDN_HAL_TABLE_ID_BITMAP_NONE,
    },
    {
        .actions         = EN_OFPAT_PUSH_PBB,
        .table_id_bitmap = SDN_HAL_TABLE_ID_BITMAP_NONE,
    },
    {
        .actions         = EN_OFPAT_POP_PBB,
        .table_id_bitmap = SDN_HAL_TABLE_ID_BITMAP_NONE,
    },
    /*
    * EstiNet defined
    */
    {
        .actions         = EN_OFPAT_EXP_PUSH_8021BR,//NA
        .table_id_bitmap = SDN_HAL_TABLE_ID_BITMAP_NONE,
    },
    {
        .actions         = EN_OFPAT_EXP_POP_8021BR,//NA
        .table_id_bitmap = SDN_HAL_TABLE_ID_BITMAP_NONE,
    },
    {
        .actions         = EN_OFPAT_EXP_PUSH_NIV,//NA
        .table_id_bitmap = SDN_HAL_TABLE_ID_BITMAP_NONE,
    },
    {
        .actions         = EN_OFPAT_EXP_POP_NIV,//NA
        .table_id_bitmap = SDN_HAL_TABLE_ID_BITMAP_NONE,
    },
    {
        .actions         = EN_OFPAT_EXP_PUSH_8021QBG,//NA
        .table_id_bitmap = SDN_HAL_TABLE_ID_BITMAP_NONE,
    },
    {
        .actions         = EN_OFPAT_EXP_POP_8021QBG,//NA
        .table_id_bitmap = SDN_HAL_TABLE_ID_BITMAP_NONE,
    },
};

static sdn_hal_table_supported_set_field_t sdn_hal_table_setfield_table[] =
{
    {
        .set_table_field  = EN_OFPXMT_OFB_VLAN_VID,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(1) | SDN_HAL_TABLE(2) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .set_table_field  = EN_OFPXMT_OFB_IP_DSCP,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(1) | SDN_HAL_TABLE(2) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .set_table_field  = EN_OFPXMT_OFB_VLAN_PCP,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(1) | SDN_HAL_TABLE(2) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    }
};

static sdn_hal_table_supported_inst_t sdn_hal_table_inst_table[]=
{
    {
        .inst_action     = EN_OFPIT_GOTO_TABLE,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(1) | SDN_HAL_TABLE(2) | SDN_HAL_TABLE(3)),
    },
    {
        .inst_action     = EN_OFPIT_WRITE_METADATA,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(1) | SDN_HAL_TABLE(2) | SDN_HAL_TABLE(3)),
    },
    {
        .inst_action     = EN_OFPIT_WRITE_ACTIONS,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(1) | SDN_HAL_TABLE(2) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .inst_action     = EN_OFPIT_APPLY_ACTIONS,
        .table_id_bitmap = SDN_HAL_TABLE_ID_BITMAP_NONE,
    },
    {
        .inst_action     = EN_OFPIT_METER,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .inst_action     = EN_OFPIT_CLEAR_ACTIONS,
        .table_id_bitmap = (SDN_HAL_TABLE(0) | SDN_HAL_TABLE(1) | SDN_HAL_TABLE(2) | SDN_HAL_TABLE(3) | SDN_HAL_TABLE(4)),
    },
    {
        .inst_action     = EN_OFPIT_MAX,
        .table_id_bitmap = SDN_HAL_TABLE_ID_BITMAP_NONE,
    },
};

static sdn_hal_queue_stat_t queue_stat_id[] = {
    {
        .out_pkt_queue_id = TX_QUEUE_0_OUT_PKTS_INDEX,
        .out_pkt_drop_id  = TX_QUEUE_0_DROP_PKTS_INDEX,
    },
    {
        .out_pkt_queue_id = TX_QUEUE_1_OUT_PKTS_INDEX,
        .out_pkt_drop_id  = TX_QUEUE_1_DROP_PKTS_INDEX,
    },
    {
        .out_pkt_queue_id = TX_QUEUE_2_OUT_PKTS_INDEX,
        .out_pkt_drop_id  = TX_QUEUE_2_DROP_PKTS_INDEX,
    },
    {
        .out_pkt_queue_id = TX_QUEUE_3_OUT_PKTS_INDEX,
        .out_pkt_drop_id  = TX_QUEUE_3_DROP_PKTS_INDEX,
    },
    {
        .out_pkt_queue_id = TX_QUEUE_4_OUT_PKTS_INDEX,
        .out_pkt_drop_id  = TX_QUEUE_4_DROP_PKTS_INDEX,
    },
    {
        .out_pkt_queue_id = TX_QUEUE_5_OUT_PKTS_INDEX,
        .out_pkt_drop_id  = TX_QUEUE_5_DROP_PKTS_INDEX,
    },
    {
        .out_pkt_queue_id = TX_QUEUE_6_OUT_PKTS_INDEX,
        .out_pkt_drop_id  = TX_QUEUE_6_DROP_PKTS_INDEX,
    },
    {
        .out_pkt_queue_id = TX_QUEUE_7_OUT_PKTS_INDEX,
        .out_pkt_drop_id  = TX_QUEUE_7_DROP_PKTS_INDEX,
    },
};
static int32 sdn_hal_local_destroy_qos(uint32_t phy_port);

int32
_sdn_hal_l3Cam_is_used(sdn_hal_rule_flow_mgmt_t *flow_entry_p)
{
    uint64 match_fields_bitmap = 0;
    uint8 *dip_v6_mask_p = NULL;
    uint8 *sip_v6_mask_p = NULL;

    match_fields_bitmap = flow_entry_p->match_fields.match_fields_bitmap;
    dip_v6_mask_p = (uint8*)&flow_entry_p->match_fields.field_data.ipv6_dst_mask;
    sip_v6_mask_p = (uint8*)&flow_entry_p->match_fields.field_data.ipv6_src_mask;

    if((HAL_LIB_CHK_BIT_ON(match_fields_bitmap, EN_OFPXMT_OFB_IPV6_DST) && !ipv6IsAllOnes_ret(dip_v6_mask_p))
        || (HAL_LIB_CHK_BIT_ON(match_fields_bitmap, EN_OFPXMT_OFB_IPV6_SRC) && !ipv6IsAllOnes_ret(sip_v6_mask_p))
        || (HAL_LIB_CHK_BIT_ON(match_fields_bitmap, EN_OFPXMT_OFB_IPV4_DST) && !ipv4IsAllOnes_ret(flow_entry_p->match_fields.field_data.dst_ipv4_mask))
        || (HAL_LIB_CHK_BIT_ON(match_fields_bitmap, EN_OFPXMT_OFB_IPV4_SRC) && !ipv4IsAllOnes_ret(flow_entry_p->match_fields.field_data.src_ipv4_mask)))
    {
        return TRUE;
    }

    return FALSE;
}
static int32 sdn_hal_table_userdefined_template_init(uint32 unit)
{
    rtk_pie_template_t  template_info;

    /* user template 0
     */
    memset(&template_info, 0, sizeof(rtk_pie_template_t));
#ifndef CONFIG_FLOW_AG
    template_info.field[0]  = TMPLTE_FIELD_SIP0;
    template_info.field[1]  = TMPLTE_FIELD_SIP1;
    template_info.field[2]  = TMPLTE_FIELD_DSAP_SSAP;
    template_info.field[3]  = TMPLTE_FIELD_SNAP_OUI;
    template_info.field[4]  = TMPLTE_FIELD_DIP0;
    template_info.field[5]  = TMPLTE_FIELD_DIP1;
    template_info.field[6]  = TMPLTE_FIELD_ETHERTYPE;
    template_info.field[7]  = TMPLTE_FIELD_PKT_INFO;
    template_info.field[8]  = TMPLTE_FIELD_IP_TOS_PROTO;
    template_info.field[9]  = TMPLTE_FIELD_L4_DPORT;
    template_info.field[10] = TMPLTE_FIELD_SPM0;
    template_info.field[11] = TMPLTE_FIELD_SPM1;
    template_info.field[12] = TMPLTE_FIELD_SPM2;
    template_info.field[13] = TMPLTE_FIELD_SPM3;
#else
    template_info.field[0]  = TMPLTE_FIELD_SIP0;
    template_info.field[1]  = TMPLTE_FIELD_SIP1;
    template_info.field[2]  = TMPLTE_FIELD_DSAP_SSAP;
    template_info.field[3]  = TMPLTE_FIELD_SNAP_OUI;
    template_info.field[4]  = TMPLTE_FIELD_DIP0;
    template_info.field[5]  = TMPLTE_FIELD_DIP1;
    template_info.field[6]  = TMPLTE_FIELD_DMAC0;
    template_info.field[7]  = TMPLTE_FIELD_DMAC1;
    template_info.field[8]  = TMPLTE_FIELD_DMAC2;
    template_info.field[9]  = TMPLTE_FIELD_L4_DPORT;
    template_info.field[10] = TMPLTE_FIELD_OTAG;
    template_info.field[11] = TMPLTE_FIELD_IP_TOS_PROTO;
    template_info.field[12] = TMPLTE_FIELD_ETHERTYPE;
#endif
    if (rtk_pie_template_set(unit, SDN_HAL_RULE_USER_TEMPLATE_ID_0, &template_info) != RT_ERR_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    /* user template 1
     */
    memset(&template_info, 0, sizeof(rtk_pie_template_t));
    template_info.field[0]  = TMPLTE_FIELD_DMAC0;
    template_info.field[1]  = TMPLTE_FIELD_DMAC1;
    template_info.field[2]  = TMPLTE_FIELD_DMAC2;
    template_info.field[3]  = TMPLTE_FIELD_OTAG;
    template_info.field[4]  = TMPLTE_FIELD_FIELD_SELECTOR_8;
    template_info.field[5]  = TMPLTE_FIELD_FIELD_SELECTOR_9;
    template_info.field[6]  = TMPLTE_FIELD_PKT_INFO;
    template_info.field[7]  = TMPLTE_FIELD_L4_DPORT;
    template_info.field[8]  = TMPLTE_FIELD_ETHERTYPE;
    template_info.field[9]  = TMPLTE_FIELD_TCP_INFO;
    template_info.field[10] = TMPLTE_FIELD_IP_TOS_PROTO;
    template_info.field[11] = TMPLTE_FIELD_SPM1;
    template_info.field[12] = TMPLTE_FIELD_RANGE_CHK;
    template_info.field[13] = TMPLTE_FIELD_L34_HEADER;

    if (rtk_pie_template_set(unit, SDN_HAL_RULE_USER_TEMPLATE_ID_1, &template_info) != RT_ERR_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    /* user template 2
     */
    memset(&template_info, 0, sizeof(rtk_pie_template_t));
    template_info.field[0]  = TMPLTE_FIELD_SMAC0;
    template_info.field[1]  = TMPLTE_FIELD_SMAC1;
    template_info.field[2]  = TMPLTE_FIELD_SMAC2;
    template_info.field[3]  = TMPLTE_FIELD_ETHERTYPE;
    template_info.field[4]  = TMPLTE_FIELD_OTAG;
    template_info.field[5]  = TMPLTE_FIELD_ITAG;
    template_info.field[6]  = TMPLTE_FIELD_DIP0;
    template_info.field[7]  = TMPLTE_FIELD_DIP1;
    template_info.field[8]  = TMPLTE_FIELD_L4_SPORT;
    template_info.field[9]  = TMPLTE_FIELD_L4_DPORT;
    template_info.field[10] = TMPLTE_FIELD_DPM0;
    template_info.field[11] = TMPLTE_FIELD_DPM1;
    template_info.field[12] = TMPLTE_FIELD_DPM2;
    template_info.field[13] = TMPLTE_FIELD_DPM3;

    if (rtk_pie_template_set(unit, SDN_HAL_RULE_USER_TEMPLATE_ID_2, &template_info) != RT_ERR_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    /* user template 3
     */
    memset(&template_info, 0, sizeof(rtk_pie_template_t));
    template_info.field[0]  = TMPLTE_FIELD_FIELD_SELECTOR_2;
    template_info.field[1]  = TMPLTE_FIELD_L4_DPORT;
    template_info.field[2]  = TMPLTE_FIELD_FIELD_SELECTOR_6;
    template_info.field[3]  = TMPLTE_FIELD_FIELD_SELECTOR_3;
    template_info.field[4]  = TMPLTE_FIELD_FIELD_SELECTOR_4;
    template_info.field[5]  = TMPLTE_FIELD_FIELD_SELECTOR_5;
    template_info.field[6]  = TMPLTE_FIELD_FIELD_SELECTOR_10;
    template_info.field[7]  = TMPLTE_FIELD_FIELD_SELECTOR_11;
    template_info.field[8]  = TMPLTE_FIELD_FIELD_SELECTOR_12;
    template_info.field[9]  = TMPLTE_FIELD_FIELD_SELECTOR_1;
    template_info.field[10] = TMPLTE_FIELD_IP_TOS_PROTO;
    template_info.field[11] = TMPLTE_FIELD_ETHERTYPE;
    template_info.field[12] = TMPLTE_FIELD_PKT_INFO;

    if (rtk_pie_template_set(unit, SDN_HAL_RULE_USER_TEMPLATE_ID_3, &template_info) != RT_ERR_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    /* user template 4
     */
    memset(&template_info, 0, sizeof(rtk_pie_template_t));
    template_info.field[0]  = TMPLTE_FIELD_DIP0;
    template_info.field[1]  = TMPLTE_FIELD_DIP1;
    template_info.field[2]  = TMPLTE_FIELD_DIP2;
    template_info.field[3]  = TMPLTE_FIELD_DIP3;
    template_info.field[4]  = TMPLTE_FIELD_DIP4;
    template_info.field[5]  = TMPLTE_FIELD_DIP5;
    template_info.field[6]  = TMPLTE_FIELD_DIP6;
    template_info.field[7]  = TMPLTE_FIELD_DIP7;
    template_info.field[8]  = TMPLTE_FIELD_META_DATA;
    template_info.field[9]  = TMPLTE_FIELD_SNAP_OUI;
    template_info.field[10] = TMPLTE_FIELD_IP_TOS_PROTO;
    template_info.field[11] = TMPLTE_FIELD_ETHERTYPE;
    template_info.field[12] = TMPLTE_FIELD_PKT_INFO;

    if (rtk_pie_template_set(unit, SDN_HAL_RULE_USER_TEMPLATE_ID_4, &template_info) != RT_ERR_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    return SDN_HAL_RETURN_OK;
}

static int32 sdn_hal_table_table0_init(uint32 unit)
{
    uint32 block_id;
    uint32 group_id = 0;
    uint32 logic_id = 0;
    int32  ret = 0;

    for (block_id = SDN_HAL_TABLE0_BLOCK_ID_START ; block_id <= SDN_HAL_TABLE0_BLOCK_ID_END; block_id++)
    {
        rtk_pie_phase_set(unit, block_id, PIE_PHASE_IGR_FLOW_TABLE_0);
        logic_id = block_id - SDN_HAL_TABLE0_BLOCK_ID_START;
        if ((ret = rtk_pie_blockGrouping_set(unit, block_id, group_id, logic_id)) != RT_ERR_OK )
        {
            return SDN_HAL_RETURN_FAILED;
        }
        if ((ret = rtk_pie_blockLookupEnable_set(unit, block_id, ENABLED)) != RT_ERR_OK )
        {
            return SDN_HAL_RETURN_FAILED;
        }
    }
    return SDN_HAL_RETURN_OK;
}

static int32 sdn_hal_table_table3_init(uint32 unit)
{
    uint32 block_id;
    uint32 group_id = 2;
    uint32 logic_id = 0;
    int32  ret = 0;

    for (block_id = SDN_HAL_TABLE3_BLOCK_ID_START ; block_id <= SDN_HAL_TABLE3_BLOCK_ID_END; block_id++)
    {
        rtk_pie_phase_set(unit, block_id, PIE_PHASE_IGR_FLOW_TABLE_3);
        logic_id = block_id - SDN_HAL_TABLE3_BLOCK_ID_START;
        if ((ret = rtk_pie_blockGrouping_set(unit, block_id, group_id, logic_id)) != RT_ERR_OK )
        {
            return SDN_HAL_RETURN_FAILED;
        }
        if ((ret = rtk_pie_blockLookupEnable_set(unit, block_id, ENABLED)) != RT_ERR_OK )
        {
            return SDN_HAL_RETURN_FAILED;
        }
    }
    return SDN_HAL_RETURN_OK;

}

static int32 sdn_hal_table_table4_init(uint32 unit)
{
    uint32 block_id;
    uint32 group_id = 3;
    uint32 logic_id = 0;
    int32  ret = 0;

    for (block_id = SDN_HAL_TABLE4_BLOCK_ID_START ; block_id <= SDN_HAL_TABLE4_BLOCK_ID_END; block_id++)
    {
        rtk_pie_phase_set(unit, block_id, PIE_PHASE_EGR_FLOW_TABLE_0);
        logic_id = block_id - SDN_HAL_TABLE4_BLOCK_ID_START;
        if ((ret = rtk_pie_blockGrouping_set(unit, block_id, group_id, logic_id)) != RT_ERR_OK )
        {
            return SDN_HAL_RETURN_FAILED;
        }
        if ((ret = rtk_pie_blockLookupEnable_set(unit, block_id, ENABLED)) != RT_ERR_OK )
        {
            return SDN_HAL_RETURN_FAILED;
        }

    }
    return SDN_HAL_RETURN_OK;

}

int32 sdn_hal_table_init(uint32 unit)
{
    int32 ret = SDN_HAL_RETURN_OK;
    uint32 table_id = 0;
    rtk_of_flowtable_phase_t phase;



    if ((ret = sdn_hal_table_table0_init(unit)) != SDN_HAL_RETURN_OK)
    {
        DBG_SDN("[%s-%d] table 0 init failed....\r\n", __FUNCTION__, __LINE__);
        return ret;
    }

    if ((ret = sdn_hal_table_table3_init(unit)) != SDN_HAL_RETURN_OK)
    {
        DBG_SDN("[%s-%d] table 3 init failed....\r\n", __FUNCTION__, __LINE__);
        return ret;
    }

    if ((ret = sdn_hal_table_table4_init(unit)) != SDN_HAL_RETURN_OK)
    {
        DBG_SDN("[%s-%d] table 4 init failed....\r\n", __FUNCTION__, __LINE__);
        return ret;
    }

    for(table_id = 0; table_id < SDN_HAL_MAX_TABLE_NUM; table_id++)
    {
        phase = sdn_hal_rule_get_flow_table(table_id);
        if (phase != FT_PHASE_EGR_FT_0)
            sdn_hal_table_miss_action_set(table_id, OF_TBLMISS_ACT_DROP);
    }
    return  ret;
}

static int32  sdn_hal_table_features_init(uint32 unit)
{
    uint32 table_id = 0;
    uint32 field_id = 0;
    uint32 flow_entry_max[SDN_HAL_MAX_TABLE_NUM] = {
        SDN_HAL_INGRESS_FULL_MATCH_TABLE0_RULE_NUM,
        SDN_HAL_INGRESS_L2_TABLE1_RULE_NUM,
        SDN_HAL_INGRESS_L3_TABLE2_RULE_NUM,
        SDN_HAL_INGRESS_FULL_MATCH_TABLE3_RULE_NUM,
        SDN_HAL_EGRESS_TABLE0_RULE_NUM,
    };

    //match field & wildcard
    for(table_id = 0; table_id < SDN_HAL_MAX_TABLE_NUM; table_id++)
    {
        sdn_hal_flow_table_feature_t *table_feature_p = &sdn_hal_table_feature[table_id];

        for (field_id = 0; field_id < _countof(sdn_hal_table_match_field_table); field_id++)
        {
            if(HAL_LIB_CHK_BIT_ON(sdn_hal_table_match_field_table[field_id].table_id_bitmap, table_id))
            {
                HAL_LIB_BIT_ON(table_feature_p->match_fields_bit_map, sdn_hal_table_match_field_table[field_id].fields);
                HAL_LIB_BIT_ON(table_feature_p->wildcard_fields_bit_map, sdn_hal_table_match_field_table[field_id].fields);
            }
        }

        for(field_id = 0; field_id < _countof(sdn_hal_table_action_table); field_id++)
        {
            if(HAL_LIB_CHK_BIT_ON(sdn_hal_table_action_table[field_id].table_id_bitmap, table_id))
            {
                HAL_LIB_BIT_ON(table_feature_p->action_bit_map, sdn_hal_table_action_table[field_id].actions);
            }
        }

         for(field_id = 0; field_id < _countof(sdn_hal_table_setfield_table); field_id++)
         {
             if(HAL_LIB_CHK_BIT_ON(sdn_hal_table_setfield_table[field_id].table_id_bitmap, table_id))
             {
                HAL_LIB_BIT_ON(table_feature_p->set_table_fields_bit_map, sdn_hal_table_setfield_table[field_id].set_table_field);
            }
         }

         for(field_id = 0; field_id < _countof(sdn_hal_table_inst_table); field_id++)
         {
             if(HAL_LIB_CHK_BIT_ON(sdn_hal_table_inst_table[field_id].table_id_bitmap, table_id))
             {
                HAL_LIB_BIT_ON(table_feature_p->instruction_bit_map, sdn_hal_table_inst_table[field_id].inst_action);
            }
         }
         table_feature_p->max_entry_nbr = flow_entry_max[table_id];

    }

    if(sdn_hal_table_userdefined_template_init(unit) != SDN_HAL_RETURN_OK)
    {
        DBG_SDN("[%s-%d] sdn_hal_table_userdefined_template_init failed....\r\n", __FUNCTION__, __LINE__);
        return SDN_HAL_RETURN_FAILED;
    }

    if (sdn_hal_table_table_template_bind(unit) != SDN_HAL_RETURN_OK)
    {
        DBG_SDN("[%s-%d] sdn_hal_table_userdefined_template_init failed....\r\n", __FUNCTION__, __LINE__);
        return SDN_HAL_RETURN_FAILED;
    }

    return  SDN_HAL_RETURN_OK;
}

#ifdef RTK_SDK_OK
#ifdef  CONFIG_SDK_RTL9310
static int32 sdn_hal_ofactionconverttodriver(sdn_hal_apply_actions_t *of_actionbucket_p, rtk_of_actionBucket_t *hal_actionbucket_p);
#endif
#endif
static int32 sdn_hal_flow_IsVaild(sdn_db_flow_entry_t *flow_entry_p)
{
    SDN_PARAM_CHK(flow_entry_p == NULL, SDN_HAL_RETURN_FAILED);
    SDN_PARAM_CHK(flow_entry_p->flow_index > sdn_hal_table_feature[flow_entry_p->table_id].max_entry_nbr, SDN_HAL_RETURN_FAILED);

    return SDN_HAL_RETURN_OK;
}

int32 _sdn_hal_logicalId_to_physicalId(rtk_of_flowtable_phase_t phase, uint32 logical_id, uint32 *physical_id)
{
    if(FT_PHASE_IGR_FT_2 == phase)
    {
        /*ipv4*/
        *physical_id = SDN_HAL_INGRESS_L3_DIV_FACTOR * logical_id;
    }
    else if(FT_PHASE_IGR_FT_3 == phase || FT_PHASE_IGR_FT_0 == phase)
    {
#ifdef CONFIG_FLOW_AG
        *physical_id = SDN_HAL_AG_ENTRY_LENGTH * logical_id;
#else
        *physical_id = logical_id;
#endif
    }
    else
    {
        *physical_id = logical_id;
    }
    return SDN_HAL_RETURN_OK;
}

int32 _sdn_hal_physicalId_to_logicalId(rtk_of_flowtable_phase_t phase, uint32 physical_id, uint32 *logical_id)
{
    if(FT_PHASE_IGR_FT_2 == phase)
    {
        /*ipv4*/
        *logical_id = (physical_id/SDN_HAL_INGRESS_L3_DIV_FACTOR);
    }
    else if(FT_PHASE_IGR_FT_3 == phase || FT_PHASE_IGR_FT_0 == phase)
    {
#ifdef CONFIG_FLOW_AG
        *logical_id = (physical_id/SDN_HAL_AG_ENTRY_LENGTH);
#else
        *logical_id = physical_id;
#endif
    }
    else
    {
        *logical_id = physical_id;
    }
    return SDN_HAL_RETURN_OK;
}

#ifdef CONFIG_FLOW_AG
static int32 sdn_hal_flow_is_aggregatable(uint32 table_id, uint64 match_fields_bitmap)
{
    uint8 i;
    uint32 table_phase = sdn_hal_rule_get_flow_table(table_id);

    /*Only FT0 and FT3 may support Flow aggregation*/
    if(table_phase != FT_PHASE_IGR_FT_0 && table_phase != FT_PHASE_IGR_FT_3)
        return FALSE;

    for(i = 0; i < SDN_HAL_MAX_NUM_BLOCK_TEMPLATE; i++)
    {
        match_fields_bitmap = match_fields_bitmap & ~(sdn_hal_table_feature[table_id].template_info.template_bitmap[i]);
    }
    if(0 == match_fields_bitmap)
        return TRUE;
    return FALSE;
}
#endif
static int32 sdn_hal_templateId_chosen(uint32 table_id, uint8 *template_id, uint64 match_fields_bitmap)
{
    uint8 i;

    for(i = 0; i < SDN_HAL_MAX_NUM_BLOCK_TEMPLATE; i++)
    {
        DBG_SDN("template_bitmap[%d] = %llu, match_fields_bitmap%llu\n",
            i, sdn_hal_table_feature[table_id].template_info.template_bitmap[i],
            match_fields_bitmap);
        if(0 == (match_fields_bitmap & ~(sdn_hal_table_feature[table_id].template_info.template_bitmap[i])))
        {
            *template_id = i;
            return SDN_HAL_RETURN_OK;
        }
    }
    return SDN_HAL_RETURN_FAILED;

}
static void sdn_hal_template_bitmap_fill(sdn_hal_tempate_info_t *templat_info)
{
    uint32 i, j;
    bool l4_sport = FALSE;
    bool l4_dport = FALSE;
    bool tcp_info = FALSE;
    bool flow_lable = FALSE;
    bool l34_header = FALSE;
    bool dip2 = FALSE;
    bool gtp_high = FALSE;
    bool gtp_low = FALSE;

    for(i = 0; i < SDN_HAL_MAX_NUM_BLOCK_TEMPLATE; i++)
    {
        for(j = 0; j < SDN_HAL_MAX_NUM_TEMPLATE_FIELD; j++)
        {
            if (templat_info->template[i].field[j] == TMPLTE_FIELD_NONE)
                continue;
 //           DBG_SDN("templat_info->template[%d].field[%d] = %d\n", i, j, templat_info->template[i].field[j]);

            switch(templat_info->template[i].field[j])
            {
                case TMPLTE_FIELD_PKT_INFO:
                {
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_IN_PORT);
                    break;
                }
                case TMPLTE_FIELD_DMAC0:
                case TMPLTE_FIELD_DMAC1:
                case TMPLTE_FIELD_DMAC2:
                {
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_ETH_DST);
                    break;
                }
                case TMPLTE_FIELD_SMAC0:
                case TMPLTE_FIELD_SMAC1:
                case TMPLTE_FIELD_SMAC2:
                {
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_ETH_SRC);
                    break;
                }
                case TMPLTE_FIELD_OTAG:
                {
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_VLAN_VID);
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_VLAN_PCP);
                    break;
                }
                case TMPLTE_FIELD_META_DATA:
                {
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_METADATA);
                    break;
                }
                case TMPLTE_FIELD_ETHERTYPE:
                {
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_ETH_TYPE);
                    break;
                }
                case TMPLTE_FIELD_IP_TOS_PROTO:
                {
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_IP_DSCP);
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_IP_ECN);
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_IP_PROTO);
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_ARP_OP);
                    break;
                }

                case TMPLTE_FIELD_SIP0:
                case TMPLTE_FIELD_SIP1:
                {
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_IPV4_SRC);
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_ARP_SPA);
                    break;
                }
                case TMPLTE_FIELD_SIP2:
                case TMPLTE_FIELD_SIP3:
                case TMPLTE_FIELD_SIP4:
                case TMPLTE_FIELD_SIP5:
                case TMPLTE_FIELD_SIP6:
                case TMPLTE_FIELD_SIP7:
                {
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_IPV6_SRC);
                    break;
                }
                case TMPLTE_FIELD_DIP0:
                case TMPLTE_FIELD_DIP1:
                {
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_IPV4_DST);
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_ARP_TPA);
                    break;
                }
                case TMPLTE_FIELD_DIP2:
                case TMPLTE_FIELD_DIP3:
                case TMPLTE_FIELD_DIP4:
                case TMPLTE_FIELD_DIP5:
                case TMPLTE_FIELD_DIP6:
                case TMPLTE_FIELD_DIP7:
                {
                    if (templat_info->template[i].field[j] == TMPLTE_FIELD_DIP2)
                        dip2 = TRUE;

                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_IPV6_DST);
                    break;
                }
                case TMPLTE_FIELD_L4_SPORT:
                {
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_TCP_SRC);
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_UDP_SRC);
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_SCTP_SRC);
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_ICMPV4_TYPE);
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_ICMPV4_CODE);
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_ICMPV6_TYPE);
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_ICMPV6_CODE);
                    l4_sport = TRUE;
                    break;
                }
                case TMPLTE_FIELD_L4_DPORT:
                {
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_TCP_DST);
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_UDP_DST);
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_SCTP_DST);
                    l4_dport = TRUE;
                    break;
                }
                case TMPLTE_FIELD_FLOW_LABEL:
                case TMPLTE_FIELD_TCP_INFO:
                {

                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_IPV6_FLABEL);
                    tcp_info = TRUE;
                    flow_lable = TRUE;
                    /* For DPA */
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_RTK_OUTER_TCP_FLG);
                    break;
                }
                case TMPLTE_FIELD_FIRST_MPLS1:
                case TMPLTE_FIELD_FIRST_MPLS2:
                {

                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_MPLS_LABEL);
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_MPLS_BOS);
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_MPLS_TC);
                    break;
                }
                case TMPLTE_FIELD_L34_HEADER:
                {
                    l34_header = TRUE;
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_RTK_OUTER_FRAG_PKT);
                    break;
                }
                case TMPLTE_FIELD_DSAP_SSAP:
                {
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_RTK_EXP_GTP_TEID);
                    gtp_low = TRUE;
                    break;
                }
                case TMPLTE_FIELD_SNAP_OUI:
                {
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_RTK_EXP_GTP_TEID);
                    gtp_high = TRUE;
                    break;
                }
                case TMPLTE_FIELD_FIELD_SELECTOR_9:
                {
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_RTK_L2_INNER_IP_VER);
                    break;
                }
                case TMPLTE_FIELD_FIELD_SELECTOR_8:
                {
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_RTK_EXP_GTP_MT);
                    break;
                }
                case TMPLTE_FIELD_FIELD_SELECTOR_4:
                {
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_RTK_OUTER_IP_FLG);
                    break;
                }
                case TMPLTE_FIELD_FIELD_SELECTOR_5:
                {
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_RTK_INNER_IP_VER);
                    break;
                }
                case TMPLTE_FIELD_FIELD_SELECTOR_10:
                {
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_RTK_INNER_L4DP);
                    break;
                }
                case TMPLTE_FIELD_FIELD_SELECTOR_11:
                case TMPLTE_FIELD_FIELD_SELECTOR_12:
                {
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_RTK_INNER_SIP);
                    break;
                }
                case TMPLTE_FIELD_FIELD_SELECTOR_2:
                case TMPLTE_FIELD_FIELD_SELECTOR_3:
                {
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_RTK_INNER_DIP);
                    break;
                }
                case TMPLTE_FIELD_FIELD_SELECTOR_6:
                {
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_RTK_INNER_IP_PROTO);
                    break;
                }
                case TMPLTE_FIELD_FIELD_SELECTOR_1:
                {
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_RTK_INNER_TCP_FLAG);
                    break;
                }
                case TMPLTE_FIELD_RANGE_CHK:
                {
                    templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_RTK_OVER_PKT_SIZE);
                    break;
                }
                default:
                    break;
            }
        }

        if(tcp_info && l4_sport && l4_dport)
            templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_ARP_SHA);
        if(l34_header && flow_lable && dip2)
            templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_ARP_THA);
        if(gtp_high && gtp_low)
            templat_info->template_bitmap[i] |= (1ULL << EN_OFPXMT_OFB_TUNNEL_ID);

        DBG_SDN("templat_info->template_bitmap[%d] = %llu\n", i, templat_info->template_bitmap[i]);
    }
}
static int sdn_hal_table_table0_template_bind(uint32 unit)
{
    uint32 block_id = 0;
    uint32 i;
    sdn_hal_table_feature[0].template_info.template_id.template_id[0] = SDN_HAL_RULE_USER_TEMPLATE_ID_0;
    sdn_hal_table_feature[0].template_info.template_id.template_id[1] = SDN_HAL_RULE_USER_TEMPLATE_ID_1;

    for(i = 0; i < SDN_HAL_MAX_NUM_BLOCK_TEMPLATE; i++)
    {
        rtk_pie_template_get(unit,
            sdn_hal_table_feature[0].template_info.template_id.template_id[i],
            &sdn_hal_table_feature[0].template_info.template[i]);
    }

    sdn_hal_template_bitmap_fill(&sdn_hal_table_feature[0].template_info);

    for (block_id = 0; block_id < SDN_HAL_TABLE0_BLOCK_SIZE; block_id++)
    {
        if (rtk_of_ftTemplateSelector_set(unit, FT_PHASE_IGR_FT_0, block_id, sdn_hal_table_feature[0].template_info.template_id) != RT_ERR_OK)
        {
            //return SDN_HAL_RETURN_FAILED;
            return  -3;
        }
    }

    return SDN_HAL_RETURN_OK;
}

static int sdn_hal_table_table1_template_bind(uint32 unit)
{
    rtk_of_l2FlowTblMatchField_t field;

    field = OF_L2_FT_MATCH_FIELD_VID_SA_SA;
    if(RT_ERR_OK != rtk_of_l2FlowTblMatchField_set(unit, field))
        return SDN_HAL_RETURN_FAILED;

    return SDN_HAL_RETURN_OK;
}

static int sdn_hal_table_table2_template_bind(uint32 unit)
{
    rtk_of_l3CamFlowTblMatchField_t  l3_camField;
    rtk_of_l3HashFlowTblMatchField_t l3_hashField;

    l3_camField = OF_L3_CAM_FT_MATCH_FIELD_MD_SIP_DIP_MD_DIP;
    if(RT_ERR_OK != rtk_of_l3CamFlowTblMatchField_set(unit, l3_camField))
        return SDN_HAL_RETURN_FAILED;

    l3_hashField = OF_L3_HASH_FT_MATCH_FIELD_SIP_DIP;
    if(RT_ERR_OK != rtk_of_l3HashFlowTblMatchField_set(unit, l3_hashField))
        return SDN_HAL_RETURN_FAILED;

    return SDN_HAL_RETURN_OK;
}

static int sdn_hal_table_table3_template_bind(uint32 unit)
{
    uint32 block_id   = 0;
    uint32 i;

    sdn_hal_table_feature[3].template_info.template_id.template_id[0] = SDN_HAL_RULE_USER_TEMPLATE_ID_3;
    sdn_hal_table_feature[3].template_info.template_id.template_id[1] = SDN_HAL_RULE_USER_TEMPLATE_ID_4;

    for(i = 0; i < SDN_HAL_MAX_NUM_BLOCK_TEMPLATE; i++)
    {
        rtk_pie_template_get(unit,
            sdn_hal_table_feature[3].template_info.template_id.template_id[i],
            &sdn_hal_table_feature[3].template_info.template[i]);
    }

    sdn_hal_template_bitmap_fill(&sdn_hal_table_feature[3].template_info);

    for (block_id = 0; block_id < SDN_HAL_TABLE3_BLOCK_SIZE; block_id++)
    {
        if (rtk_of_ftTemplateSelector_set(unit, FT_PHASE_IGR_FT_3, block_id, sdn_hal_table_feature[3].template_info.template_id) != RT_ERR_OK)
        {
            //return SDN_HAL_RETURN_FAILED;
            return  -3;
        }
    }

    return SDN_HAL_RETURN_OK;
}

static int sdn_hal_table_table4_template_bind(uint32 unit)
{
    uint32 block_id = 0;
    uint32 i = 0;
    sdn_hal_table_feature[4].template_info.template_id.template_id[0] = SDN_HAL_RULE_USER_TEMPLATE_ID_3;
    sdn_hal_table_feature[4].template_info.template_id.template_id[1] = SDN_HAL_RULE_USER_TEMPLATE_ID_4;

    for(i = 0; i < SDN_HAL_MAX_NUM_BLOCK_TEMPLATE; i++)
    {
        rtk_pie_template_get(unit,
            sdn_hal_table_feature[4].template_info.template_id.template_id[i],
            &sdn_hal_table_feature[4].template_info.template[i]);
    }

    sdn_hal_template_bitmap_fill(&sdn_hal_table_feature[4].template_info);

    for (block_id = 0; block_id < SDN_HAL_TABLE4_BLOCK_SIZE; block_id++)
    {
        if (rtk_of_ftTemplateSelector_set(unit, FT_PHASE_EGR_FT_0, block_id, sdn_hal_table_feature[4].template_info.template_id) != RT_ERR_OK)
        {
            //return SDN_HAL_RETURN_FAILED;
            return  -3;
        }
    }

    return SDN_HAL_RETURN_OK;
}
int32 sdn_hal_table_table_template_bind(uint32 unit)
{
    if (sdn_hal_table_table0_template_bind(unit) != SDN_HAL_RETURN_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    if (sdn_hal_table_table1_template_bind(unit) != SDN_HAL_RETURN_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    if (sdn_hal_table_table2_template_bind(unit) != SDN_HAL_RETURN_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    if (sdn_hal_table_table3_template_bind(unit) != SDN_HAL_RETURN_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    if (sdn_hal_table_table4_template_bind(unit) != SDN_HAL_RETURN_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    return SDN_HAL_RETURN_OK;
}
int sdn_hal_init(struct ofp_table_features tf[MAX_TABLE_NUM])
{
    int ret = SDN_HAL_RETURN_OK;
    uint32 unit = 0;

    /* 1. Init Chip ACL block,partition,flow table,execution set,group table */

    if(sdn_hal_table_init(unit) != SDN_HAL_RETURN_OK)
    {
        DBG_SDN("[%s-%d] table init failed....\r\n", __FUNCTION__, __LINE__);
        return SDN_HAL_RETURN_FAILED;
    }

    if(sdn_hal_table_features_init(unit) != SDN_HAL_RETURN_OK)
    {
        DBG_SDN("[%s-%d] table features init failed....\r\n", __FUNCTION__, __LINE__);
        return SDN_HAL_RETURN_FAILED;
    }
    memset(&g_tpid, 0, sizeof(sdn_hal_tpid_t) * SDN_HAL_TPID_NUM_MAX);

    g_total_actions_bucket_num = 0;

    memset(&l3_cam_logic_id, 0, sizeof(uint32) * SDN_HAL_INGRESS_L3_TABLE2_CAM_RULE_NUM);
    return ret;
}
int32 sdn_hal_table_features_get(uint32 table_id, sdn_hal_flow_table_feature_t *table_features_p)
{
    rtk_of_flowtable_phase_t phase;

    if (SDN_HAL_MAX_TABLE_NUM <= table_features_p->flow_table_id)
        return SDN_HAL_RETURN_FAILED;

    phase = sdn_hal_rule_get_flow_table(table_id);

    table_features_p->match_fields_bit_map     = sdn_hal_table_feature[phase].match_fields_bit_map;
    table_features_p->wildcard_fields_bit_map  = sdn_hal_table_feature[phase].wildcard_fields_bit_map;
    table_features_p->action_bit_map           = sdn_hal_table_feature[phase].action_bit_map;
    table_features_p->set_table_fields_bit_map = sdn_hal_table_feature[phase].set_table_fields_bit_map;
    table_features_p->instruction_bit_map      = sdn_hal_table_feature[phase].instruction_bit_map;
    table_features_p->max_entry_nbr            = sdn_hal_table_feature[phase].max_entry_nbr;
    /*todo*/
#if 0
    table_features_p->max_actions;
    table_features_p->max_match_fields;
    table_features_p->next_table_id;
#endif
    return SDN_HAL_RETURN_OK;
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
    int rc = SDN_HAL_RETURN_OK;
    drv_nic_pkt_t pkt_full_data;
    drv_nic_pkt_t *pPacket = &pkt_full_data;//NULL;
    uint8_t pkt_data[2048] = { 0 };

    void *pkt_pl_p = (void *)pkt_data; //NULL;

    DBG_SDN("\r\n[%s:%d] devname(%s) \r\n", __FUNCTION__, __LINE__, dev_name);
    if (pkt_p == NULL)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    //pPacket = (drv_nic_pkt_t *)malloc(sizeof(drv_nic_pkt_t));

    if (pPacket == NULL)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    memset(pPacket, 0, sizeof(drv_nic_pkt_t));


    //pkt_pl_p = malloc(pkt_size);

    //if (pkt_pl_p == NULL)
    if (pkt_size > sizeof(pkt_data))
    {
        pPacket = NULL;//free(pPacket);
        return SDN_HAL_RETURN_FAILED;
    }

    memcpy(pkt_pl_p, pkt_p, pkt_size);
    pPacket->length = pkt_size;
    pPacket->head   = (uint8 *)pkt_pl_p;
    pPacket->data   = (uint8 *)pPacket->head;
    pPacket->tail   = pPacket->data + pPacket->length;
    pPacket->end    = pPacket->tail;
    pPacket->tx_tag.fwd_type = NIC_FWD_TYPE_PHYISCAL;
    pPacket->as_txtag = 1;

    //if (OF_OUTPUT_TYPE_PHY_PORT == output_type)
    if (port_no < 32 )
    {
        pPacket->tx_tag.dst_port_mask = (0x00000001 << port_no);
    }
    else
    {
        pPacket->tx_tag.dst_port_mask_1 = (0x00000001 << port_no);
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
    pPacket->tx_tag.bp_fltr       = 1;
    pPacket->tx_tag.ori_tagif_en  = 1;
    pPacket->tx_tag.ori_otagif    = 0;
    pPacket->tx_tag.fwd_type      = NIC_FWD_TYPE_PHYISCAL;
#endif

//#ifdef __APPLE__
//    dio.data[1] = (unsigned long)pPacket;
//#elif __linux__
//    dio.data[1] = (uint32)pPacket;
//    ioctl(fd, RTCORE_NIC_PKT_TX, &dio);
//#endif

//    close(fd);
#ifdef CONFIG_SYS_LIB_PKT_IN_OUT
    rc = drv_nic_pkt_tx_toKernel(SDN_HAL_MY_UNIT_ID(), pPacket);
#endif
    if (rc != RT_ERR_OK)
    {
        rc = SDN_HAL_RETURN_FAILED;
    }
    else
    {
        DBG_SDN("\r\n[%s:%d] tx to port(%d) ok\r\n", __FUNCTION__, __LINE__, port_no);
        rc = SDN_HAL_RETURN_OK;
    }

//    if (dio.ret)
//    {
//        return SDN_HAL_RETURN_FAILED;
//    }

    if (pkt_pl_p)
    {
        //free(pkt_pl_p);
        pkt_pl_p = NULL;
    }

    if (pPacket)
    {
        //free(pPacket);
        pPacket = NULL;
    }

    return rc;
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

#ifdef RTK_SDK_OK
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
#endif
int32 sdn_hal_add_group(uint32 group_id, rtk_of_groupType_t type, sdn_hal_group_table_t *entry_p)
{
    rtk_of_groupEntry_t group_entry;
    rtk_of_actionBucket_t drv_action_bucket_entry;
    sdn_hal_apply_actions_t  *action_bucket_p = NULL;
    //sdn_db_apply_actions_t *action_bucket_p = NULL;

    if (entry_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }
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

//    for(port_id = USR_PORT_1; port_id <= USR_PORT_54; port_id++)
    for(port_id = USR_PORT_1; port_id <= RTK_SDN_931X_CPU_PORT; port_id++) /*Modify for MEC DPA*/
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

    return SDN_HAL_RETURN_OK;

}


/* Counter */
int32 sdn_hal_flow_entry_counter_apply(uint8 table_id,
                                     uint32 flow_id,
                                     uint32 count_id,
                                     uint32_t counter_flag)
{
    /* 1. Apply counter to chip */
    /*Brandon:
        1.need to convert table_id to phase.
        2. flow_id convert to physical entry_idx.
        3. count_id doesn't be used by sdk  ,  why not  ????!!!
    */
    rtk_of_flowtable_phase_t phase;
    int ret = SDN_HAL_RETURN_OK;
    int unit = 0;
    uint32 physical_id;

    phase = sdn_hal_rule_get_flow_table(table_id);
    _sdn_hal_logicalId_to_physicalId(phase, flow_id, &physical_id);

    if(phase == FT_PHASE_IGR_FT_1 || phase == FT_PHASE_IGR_FT_2)
        return SDN_HAL_RETURN_OK;

    if(( ret = rtk_of_flowCntMode_set(unit, phase, (rtk_of_flow_id_t) physical_id, OF_FLOW_CNTMODE_PACKET_CNT_AND_BYTE_CNT)) != RT_ERR_OK)
    {
        DBG_SDN("[%s][%d] fail to get flow count mode, ret=[%d]. \n", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    return SDN_HAL_RETURN_OK;
}

int sdn_hal_flow_entry_counter_delete(uint8 table_id,
                                      uint32 flow_id,
                                      uint32 count_id)
{
    /* 1. Delete counter to chip */

    /*Brandon: don't know which sdk API. */
    /*full match flow table alway apply counter, if del binded counter id shall delete flow db only. */
    return SDN_HAL_RETURN_OK; /*Add by MSP*/
}

int sdn_hal_flow_entry_counter_clear(uint8 table_id,
                                     uint32 flow_id,
                                     uint32 count_id)
{
    /* 1. Clear counter to chip */

    int ret = SDN_HAL_RETURN_OK;
    int unit = 0;
    rtk_of_flowCntMode_t  cnt_mode;
    rtk_of_flowtable_phase_t table_phase;
    uint32 physical_id;

    table_phase = sdn_hal_rule_get_flow_table(table_id);
    _sdn_hal_logicalId_to_physicalId(table_phase, flow_id, &physical_id);

    if(table_phase == FT_PHASE_IGR_FT_1 || table_phase == FT_PHASE_IGR_FT_2)
        return SDN_HAL_RETURN_OK;

    ret = rtk_of_flowCntMode_get(unit, table_phase, (rtk_of_flow_id_t) physical_id, &cnt_mode);

    if (ret != RT_ERR_OK)
    {
        DBG_SDN("[%s][%d] fail to get flow count mode, ret=[%d]. \n", __FUNCTION__, __LINE__, ret);
        return ret;
    }


    switch (cnt_mode)
    {
        case OF_FLOW_CNTMODE_PACKET_CNT_AND_BYTE_CNT:
            if(( ret = rtk_of_flowCnt_clear(unit , table_phase, (rtk_of_flow_id_t)  physical_id,
                                            OF_FLOW_CNT_TYPE_PACKET) ) != RT_ERR_OK)
            {
                break;
            }

            if(( ret = rtk_of_flowCnt_clear(unit , table_phase, (rtk_of_flow_id_t)  physical_id,
                                            OF_FLOW_CNT_TYPE_BYTE) ) != RT_ERR_OK)
            {
                break;
            }
            break;

        case  OF_FLOW_CNTMODE_PACKET_CNT_AND_PACKET_CNT_TH:
            if(( ret = rtk_of_flowCnt_clear(unit , table_phase, (rtk_of_flow_id_t)  physical_id,
                                            OF_FLOW_CNT_TYPE_PACKET) ) != RT_ERR_OK)
            {
                break;
            }
            break;
        case OF_FLOW_CNTMODE_BYTE_CNT_AND_BYTE_CNT_TH:
            if((ret = rtk_of_flowCnt_clear(unit , table_phase, (rtk_of_flow_id_t)  physical_id,
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
        DBG_SDN("[%s][%d] fail to get flow count , ret=[%d]. \n", __FUNCTION__, __LINE__, ret);
        return SDN_HAL_RETURN_FAILED;
    }

    return SDN_HAL_RETURN_OK;
}

/* Enhance by MEC DPA */
int sdn_hal_flow_entry_counter_get(uint8 table_id,
                                     uint32 flow_id,
                                     uint64 *pPacketCnt,
                                     uint64 *pByteCnt)
{
    /* 1. Clear counter to chip */

    int ret = SDN_HAL_RETURN_OK;
    int unit = 0;
    rtk_of_flowCntMode_t  cnt_mode;
    rtk_of_flowtable_phase_t table_phase;
    uint32 physical_id;

    table_phase = sdn_hal_rule_get_flow_table(table_id);
    _sdn_hal_logicalId_to_physicalId(table_phase, flow_id, &physical_id);

    if(table_phase == FT_PHASE_IGR_FT_1 || table_phase == FT_PHASE_IGR_FT_2)
        return SDN_HAL_RETURN_OK;

    ret = rtk_of_flowCntMode_get(unit, table_phase, (rtk_of_flow_id_t) physical_id, &cnt_mode);

    if (ret != RT_ERR_OK)
    {
        DBG_SDN("[%s][%d] fail to get flow count mode, ret=[%d]. \n", __FUNCTION__, __LINE__, ret);
        return ret;
    }


    switch (cnt_mode)
    {
        case OF_FLOW_CNTMODE_PACKET_CNT_AND_BYTE_CNT:
            if(( ret = rtk_of_flowCnt_get(unit , table_phase, (rtk_of_flow_id_t)  physical_id,
                                            OF_FLOW_CNT_TYPE_PACKET,pPacketCnt)) != RT_ERR_OK)
            {
                break;
            }

            ret = rtk_of_flowCnt_get(unit , table_phase, (rtk_of_flow_id_t)  physical_id, OF_FLOW_CNT_TYPE_BYTE, pByteCnt);

            break;

        case  OF_FLOW_CNTMODE_PACKET_CNT_AND_PACKET_CNT_TH:
            break;
        case OF_FLOW_CNTMODE_BYTE_CNT_AND_BYTE_CNT_TH:
            break;
        default:
            break;
    }

    if (ret != RT_ERR_OK)
    {
        DBG_SDN("[%s][%d] fail to get flow count , ret=[%d]. \n", __FUNCTION__, __LINE__, ret);
        return SDN_HAL_RETURN_FAILED;
    }

    return SDN_HAL_RETURN_OK;
}

int sdn_hal_flow_table_stats_get(uint8 table_id, rtk_of_flowTblCntType_t type, uint32 *pCnt)
{
    /* 1. Get counter of flow table from chip */

#if 1  /* Brandon add: */
    int unit = 0;
    uint32_t table_count = 0;
    rtk_of_flowtable_phase_t phase;

    phase = sdn_hal_rule_get_flow_table(table_id);
    if (pCnt == NULL)
    {
        return SDN_HAL_RETURN_NULL_DATA;
    }

    CHECK_TABLE_ID_IS_VALID(table_id);

    if (rtk_of_flowTblCnt_get(unit, phase, type, &table_count) != RT_ERR_OK)
    {
        DBG_SDN("[%s][%d] fail to get flow count mode, ret=[%d]. \n", __FUNCTION__, __LINE__, SDN_HAL_RETURN_FAILED);
        return SDN_HAL_RETURN_FAILED;
    }

    *pCnt = table_count;

//    DBG_SDN("[%s:%d] table(%d) type(%s) counter(%d) \n", __FUNCTION__, __LINE__, table_id, (type == OF_FLOW_TBL_CNT_TYPE_LOOKUP ? "look_up" : "match"), *pCnt);

    return SDN_HAL_RETURN_OK;

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
    rtk_of_flowtable_phase_t phase;

    phase = sdn_hal_rule_get_flow_table(table_id);
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

    if (RT_ERR_OK != rtk_of_flowCntMode_get(unit, phase, (rtk_of_flow_id_t) priority, &counter_mode))
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

        case OF_FLOW_CNTMODE_DISABLE:
        default:
             *packet_count = 0;
             *byte_count   = 0;
        ;
    }
    return SDN_HAL_RETURN_OK; /*Add by MSP*/
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
    rtk_of_flowCntMode_t cnt_mode = OF_FLOW_CNTMODE_DISABLE;
    uint64 count = ~0;
    rtk_of_flowtable_phase_t phase;
    uint32 physical_id;

    phase = sdn_hal_rule_get_flow_table(table_id);
    _sdn_hal_logicalId_to_physicalId(phase, flow_id, &physical_id);

    if (packet_count == NULL)
        return RT_ERR_NULL_POINTER;

    if (byte_count == NULL)
        return RT_ERR_NULL_POINTER;


    /* 1. Get flow counter from chip */

    /*
    1. table_id convert to phase
    2. flow_id convert to entry_idx
    */


    ret = rtk_of_flowCntMode_get(unit, phase, (rtk_of_flow_id_t)physical_id, &cnt_mode);

    if (ret != RT_ERR_OK)
    {
        DBG_SDN("[%s][%d] fail to get flow count mode, ret=[%d]. \n", __FUNCTION__, __LINE__, ret);
        return SDN_HAL_RETURN_FAILED;
    }

    switch (cnt_mode)
    {
        case OF_FLOW_CNTMODE_PACKET_CNT_AND_BYTE_CNT:
            if(( ret = rtk_of_flowCnt_get(unit , phase, (rtk_of_flow_id_t)  physical_id,
                                          OF_FLOW_CNT_TYPE_PACKET, &count) ) != 0)
                break;
            *packet_count = count;
            count = ~0;
            if((ret = rtk_of_flowCnt_get(unit , phase, (rtk_of_flow_id_t)  physical_id,
                                         OF_FLOW_CNT_TYPE_BYTE, &count)) != 0)
                break;
            *byte_count = count;
            break;

        case  OF_FLOW_CNTMODE_PACKET_CNT_AND_PACKET_CNT_TH:
            if(( ret = rtk_of_flowCnt_get(unit , phase, (rtk_of_flow_id_t)  physical_id,
                                          OF_FLOW_CNT_TYPE_PACKET, &count)) != 0)
                break;
            *packet_count = count;
            break;

        case OF_FLOW_CNTMODE_BYTE_CNT_AND_BYTE_CNT_TH:
            if(( ret = rtk_of_flowCnt_get(unit , phase, (rtk_of_flow_id_t)  physical_id,
                                          OF_FLOW_CNT_TYPE_BYTE, &count)) != 0)
                break;
            *byte_count = count;
            break;
        default:
            break;
    }

    if (ret != RT_ERR_OK)
    {
        DBG_SDN("[%s][%d] fail to get flow count , ret=[%d]. \n", __FUNCTION__, __LINE__, ret);
        return SDN_HAL_RETURN_FAILED;
    }

    return SDN_HAL_RETURN_OK;
}

int sdn_hal_port_stats_get(uint32 port_no, hal_port_stats_t *pPort_stats_data )
{
    int ret = -1;
    uint32_t unit = 0;

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
        pPort_stats_data->rx_drops      += port_counter.rxMacIPGShortDrop;
        pPort_stats_data->rx_drops      += port_counter.etherStatsRxUndersizeDropPkts;

        /* OHAL_COUNTER_PORT_TYPE_DROP_TRANSMITTED */
        pPort_stats_data->tx_drops	 =  port_counter.egrQueue0DropPkts;				/* TODO: ?? */
        pPort_stats_data->tx_drops      +=  port_counter.egrQueue1DropPkts;
        pPort_stats_data->tx_drops      +=  port_counter.egrQueue2DropPkts;
        pPort_stats_data->tx_drops      +=  port_counter.egrQueue3DropPkts;
        pPort_stats_data->tx_drops      +=  port_counter.egrQueue4DropPkts;
        pPort_stats_data->tx_drops      +=  port_counter.egrQueue5DropPkts;
        pPort_stats_data->tx_drops      +=  port_counter.egrQueue6DropPkts;
        pPort_stats_data->tx_drops      +=  port_counter.egrQueue7DropPkts;

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
        pPort_stats_data->rx_crc_errors	 = port_counter.etherStatsCRCAlignErrors;
        pPort_stats_data->rx_crc_errors += port_counter.rxEtherStatsCRCAlignErrors;

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
#if 0
        DBG_SDN("\n[%s-%d] logic_port(%d) phy_port(%d)   \
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

#endif
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
    uint32_t unit = 0;

    memset(&rtk_linkChange, 0, sizeof(rtk_port_linkChange_t));
    //rtk_ovs_portLinkChange_get(unit, &rtk_linkChange);

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
            break;
        case PORT_LINKDOWN:
            *port_state = EN_OFPPS_LINK_DOWN;
            break;
        default:
            return SDN_HAL_RETURN_FAILED;
    }
    return SDN_HAL_RETURN_OK;
}
int32 sdn_hal_cpu_port_get(void)
{
    rtk_switch_devInfo_t devInfo;
    uint32_t unit;
    unit = SDN_HAL_MY_UNIT_ID();

    memset(&devInfo, 0, sizeof(rtk_switch_devInfo_t));

    rtk_switch_deviceInfo_get(unit, &devInfo);

    return devInfo.cpuPort;
}

int32 sdn_hal_port_max_get(void)
{
    uint32 maxPort = 0;
    uint32 i = 0;

    for(i = 0; i < _countof(sdn_hal_port_map); i++)
    {
        if ((sdn_hal_port_map[i].usrPort > maxPort) && (sdn_hal_port_map[i].usrPort != PORTCONF_END_VAL))
            maxPort++;//maxPort = sdn_hal_port_map[i].usrPort;
    }

    maxPort++;

    //DBG_SDN("[%s:%d] max port(%d) \n", __FUNCTION__, __LINE__, maxPort);
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
    rtk_enable_t state = ENABLED;

    SDN_PARAM_CHK(!sdn_hal_port_exist(port_no), SDN_HAL_RETURN_FAILED);

    sdn_hal_usrPort_unit_get(port_no, &unit);

    if(SDN_HAL_RETURN_OK != (ret = sdn_hal_userPort2PhyPort(port_no, &phy_port)))
    {
        return ret;
    }

    if (port_config->config_mask & EN_OFPPC_PORT_DOWN)
    {
        if(port_config->config & EN_OFPPC_PORT_DOWN)
            state = DISABLED;
        else
            state = ENABLED;

        if (RT_ERR_OK != rtk_port_adminEnable_set(unit, phy_port, state))
        {
            return SDN_HAL_RETURN_FAILED;
        }
    }

    if (port_config->config_mask & EN_OFPPC_NO_RECV)
    {
        if(port_config->config & EN_OFPPC_NO_RECV)
            state = DISABLED;
        else
            state = ENABLED;
        if (RT_ERR_OK !=  rtk_port_rxEnable_set(unit, phy_port, state))
        {
            return SDN_HAL_RETURN_FAILED;
        }
    }

    if (port_config->config_mask & EN_OFPPC_NO_FWD)
    {
        if(port_config->config & EN_OFPPC_NO_FWD)
            state = DISABLED;
        else
            state = ENABLED;
        if (RT_ERR_OK != rtk_port_txEnable_set(unit, phy_port, state))
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
    rtk_of_classifierData_t classifier;

    if(port_no == cpu_port_id)
            return SDN_HAL_RETURN_OK;

    sdn_hal_usrPort_unit_get(port_no, &unit);

    if(SDN_HAL_RETURN_OK != sdn_hal_userPort2PhyPort(port_no, &phyPort))
    {
        return SDN_HAL_RETURN_FAILED;
    }

    classifier.enable = ENABLED;
    classifier.port = phyPort;
    /*openflow port doesn't go to the acl pipeline.*/
    if(RT_ERR_OK != rtk_of_classifier_set(unit, OF_CLASSIFIER_TYPE_PORT, classifier))
        return SDN_HAL_RETURN_FAILED;

    return SDN_HAL_RETURN_OK;
}

int32 sdn_hal_port_delete(uint32 port_no)
{
    uint32 unit = 0;
    uint32 phyPort = 0;
    rtk_of_classifierData_t classifier;
    sdn_hal_usrPort_unit_get(port_no, &unit);

    if(SDN_HAL_RETURN_OK != sdn_hal_userPort2PhyPort(port_no, &phyPort))
    {
        return SDN_HAL_RETURN_FAILED;
    }
    classifier.enable = ENABLED;
    classifier.port = phyPort;

    if(RT_ERR_OK != rtk_of_classifier_set(unit, OF_CLASSIFIER_TYPE_PORT, classifier))
        return SDN_HAL_RETURN_FAILED;

    return SDN_HAL_RETURN_OK;
}

int32 sdn_hal_destroy(void)
{
    rtk_switch_devInfo_t dev;
    uint32 unit = SDN_HAL_MY_UNIT_ID();
    uint32 port;
    rtk_of_classifierData_t data;

    memset(&data, 0, sizeof(rtk_of_classifierData_t));
    rtk_switch_deviceInfo_get(unit, &dev);
    rtk_port_isolation_set(unit, SDN_HAL_MGMT_PORT_1, &org_isolate_port_1);
    rtk_port_isolation_set(unit, SDN_HAL_MGMT_PORT_2, &org_isolate_port_2);
    rtk_port_isolation_set(unit, dev.cpuPort, &dev.all.portmask);

    RTK_PORTMASK_SCAN(dev.all.portmask, port)
    {
        data.port = port;
        data.enable = DISABLED;

        rtk_of_classifier_get(SDN_HAL_MY_UNIT_ID() , OF_CLASSIFIER_TYPE_PORT, &data);

        if (data.enable != ENABLED)
        {
            continue;
        }

        sdn_hal_local_destroy_qos(data.port);

        rtk_of_classifier_set(unit, OF_CLASSIFIER_TYPE_PORT, data);
    }
    return SDN_HAL_RETURN_OK; /*Add by MSP*/
}

int32 sdn_hal_port_init(void)
{
    rtk_switch_devInfo_t dev_info;
    sdn_hal_portmask_t   sdn_hal_physicalPortMask;
    sdn_hal_portmask_t   isolate_port;
    rtk_switch_devInfo_t dev;

    memset(&isolate_port, 0, sizeof(sdn_hal_portmask_t));
    memset(&org_isolate_port_1, 0, sizeof(sdn_hal_portmask_t));
    memset(&org_isolate_port_2, 0, sizeof(sdn_hal_portmask_t));
    memset(&dev, 0, sizeof(rtk_switch_devInfo_t));

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

    return SDN_HAL_RETURN_OK;
}

int32 sdn_hal_eth_addr_get(uint32 port_no, uint8 mac_addr[ETH_ADDR_LEN])
{
    uint32 unit = 0;
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
    features->virtual_table_exist = false;
    features->n_tables = HAL_TABLE_NUM;
    features->capabilities = SDN_HAL_RULE_SWITCH_FLOW_STATS
                                | SDN_HAL_RULE_SWITCH_PORT_STATS
                                | SDN_HAL_RULE_SWITCH_TABLE_STATS
                                | SDN_HAL_RULE_SWITCH_PORT_BLOCKED;

    return SDN_HAL_RETURN_OK;
}
int32 sdn_hal_flow_entry_move(uint8 table_id, uint32 flow_index_dst, uint32 flow_index_src, uint32 total_flows)
{
    rtk_of_flowMove_t move_data;
    rtk_of_flowtable_phase_t table_phase;
    uint32  i = 0;
    uint32 unit_id = 0;
    uint32 physical_idx;

    table_phase = sdn_hal_rule_get_flow_table(table_id);

    switch(table_phase)
    {
        case FT_PHASE_IGR_FT_1:
            return SDN_HAL_RETURN_OK;
        case FT_PHASE_IGR_FT_2:
        {
             /*move backward*/
            if (flow_index_dst > flow_index_src)
            {
                for( i = 0; i < l3_cam_flow_count; i++)
                {
                    if(flow_index_src <= l3_cam_logic_id[i])
                    {
                        l3_cam_logic_id[i] += (flow_index_dst - flow_index_src);
                    }
                }
            }
            else
            {
                for( i = 0; i < l3_cam_flow_count; i++)
                {
                    if(flow_index_src <= l3_cam_logic_id[i])
                    {
                        l3_cam_logic_id[i] -= (flow_index_src - flow_index_dst);
                    }
                }
            }
            break;
        }
        case FT_PHASE_IGR_FT_0:
        case FT_PHASE_IGR_FT_3:
        case FT_PHASE_EGR_FT_0:
        {
            memset(&move_data, 0, sizeof(rtk_of_flowMove_t));
            _sdn_hal_logicalId_to_physicalId(table_phase, flow_index_src, &physical_idx);
            move_data.move_from = physical_idx;

            _sdn_hal_logicalId_to_physicalId(table_phase, flow_index_dst, &physical_idx);
            move_data.move_to = physical_idx;
            if(FT_PHASE_EGR_FT_0 == table_phase)
                move_data.length = total_flows;
            else
            {
#ifdef CONFIG_FLOW_AG
                move_data.length = SDN_HAL_AG_ENTRY_LENGTH * total_flows;
#else
                move_data.length = total_flows;
#endif
            }

            /* Check again, anyway (Checking the last index is just enough) */
            if(move_data.move_from == move_data.move_to)
            {
                return SDN_HAL_RETURN_OK;
            }
            if(total_flows == 0)
            {
                return SDN_HAL_RETURN_FAILED;
            }

            if(RT_ERR_OK != rtk_of_flowEntry_move(unit_id, table_phase, &move_data))
            {
                return SDN_HAL_RETURN_FAILED;
            }
            break;
        }
        default:
            return SDN_HAL_RETURN_FAILED;
    }

    return SDN_HAL_RETURN_OK;
}

static int match_transform(sdn_hal_rule_flow_mgmt_t *flow_data, const sdn_db_flow_entry_t *flow)
{
    uint32  i;
    sdn_hal_rule_flow_match_fields_t    *sdn_hal_match = NULL;
    sdn_db_match_field_t                *sdn_db_match = NULL;

    sdn_hal_match = &flow_data->match_fields;

    if (flow->len_match == 0)
    {
        sdn_hal_match->field_data.l_port = 0Xffff;
    }

    for(i = 0; i < flow->len_match; i++)
    {
        sdn_db_match = &flow->match_field_p[i];
        if(sdn_db_match->oxm_class != EN_OFPXMC_OPENFLOW_BASIC)
        {
            continue;
        }
        DBG_SDN("sdn_db_match->oxm_type = %d\n", sdn_db_match->oxm_type);
        switch(sdn_db_match->oxm_type)
        {
            case EN_OFPXMT_OFB_IN_PORT:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                sdn_hal_match->field_data.l_port = (uint16)((sdn_db_match->value.low));
                break;
            }
            case EN_OFPXMT_OFB_METADATA:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                sdn_hal_match->field_data.metadata = (uint16)((sdn_db_match->value.low));
                sdn_hal_match->field_data.metadata_mask = (uint16)((sdn_db_match->mask.low));
                break;
            }
            case EN_OFPXMT_OFB_ETH_DST:
            case EN_OFPXMT_OFB_ARP_THA:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                /* Copy lowest 6 bytes */
                memcpy(sdn_hal_match->field_data.dst_mac, ((char*)&sdn_db_match->value.low) + 2, OFP_ETH_ALEN);
                /* Mask */
                memcpy(sdn_hal_match->field_data.dst_mac_mask, ((char*)&sdn_db_match->mask.low) + 2, OFP_ETH_ALEN);
                break;
            }
            case EN_OFPXMT_OFB_ETH_SRC:
            case EN_OFPXMT_OFB_ARP_SHA:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                /* Copy lowest 6 bytes */
                memcpy(sdn_hal_match->field_data.src_mac, ((char*)&sdn_db_match->value.low) + 2, OFP_ETH_ALEN);
                /* Mask */
                memcpy(sdn_hal_match->field_data.src_mac_mask, ((char*)&sdn_db_match->mask.low) + 2, OFP_ETH_ALEN);
                break;
            }
            case EN_OFPXMT_OFB_ETH_TYPE:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                sdn_hal_match->field_data.eth_type_val = (uint32)sdn_db_match->value.low;
                break;
            }
            case EN_OFPXMT_OFB_VLAN_VID:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                sdn_hal_match->field_data.svid = (uint32)sdn_db_match->value.low;
                /* Mask? */
                break;
            }
            case EN_OFPXMT_OFB_VLAN_PCP:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                sdn_hal_match->field_data.pcp_value = (uint8)sdn_db_match->value.low;
                sdn_hal_match->field_data.pcp_mask = 0x07;
                break;
            }
            case EN_OFPXMT_OFB_IP_DSCP:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                sdn_hal_match->field_data.dscp = (uint8)sdn_db_match->value.low;
                break;
            }
            case EN_OFPXMT_OFB_IP_ECN:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                sdn_hal_match->field_data.ip_ecn = (uint8)sdn_db_match->value.low;
                break;
            }
            case EN_OFPXMT_OFB_IP_PROTO:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                sdn_hal_match->field_data.protocol_val = (uint8)sdn_db_match->value.low;
                break;
            }
            case EN_OFPXMT_OFB_ARP_OP:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                sdn_hal_match->field_data.arp_op = (uint32)sdn_db_match->value.low;
                break;
            }
            case EN_OFPXMT_OFB_IPV4_SRC:
            case EN_OFPXMT_OFB_ARP_SPA:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                sdn_hal_match->field_data.src_ipv4_addr = (uint32)sdn_db_match->value.low;
                /* Mask */
                sdn_hal_match->field_data.src_ipv4_mask = (uint32)sdn_db_match->mask.low;
                break;
            }
            case EN_OFPXMT_OFB_IPV4_DST:
            case EN_OFPXMT_OFB_ARP_TPA:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                sdn_hal_match->field_data.dst_ipv4_addr = (uint32)sdn_db_match->value.low;
                /* Mask */
                sdn_hal_match->field_data.dst_ipv4_mask = (uint32)sdn_db_match->mask.low;
                break;
            }
            case EN_OFPXMT_OFB_TCP_SRC:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                sdn_hal_match->field_data.tcp_src_port_no = (uint32)sdn_db_match->value.low;
                break;
            }
            case EN_OFPXMT_OFB_TCP_DST:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                sdn_hal_match->field_data.tcp_dst_port_no = (uint32)sdn_db_match->value.low;
                break;
            }
            case EN_OFPXMT_OFB_UDP_SRC:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                sdn_hal_match->field_data.udp_src_port_no = (uint32)sdn_db_match->value.low;
                break;
            }
            case EN_OFPXMT_OFB_UDP_DST:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                sdn_hal_match->field_data.udp_dst_port_no = (uint32)sdn_db_match->value.low;
                break;
            }
            case EN_OFPXMT_OFB_SCTP_SRC:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                sdn_hal_match->field_data.sctp_src_port_no = (uint32)sdn_db_match->value.low;
                break;
            }
            case EN_OFPXMT_OFB_SCTP_DST:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                sdn_hal_match->field_data.sctp_dst_port_no = (uint32)sdn_db_match->value.low;
                break;
            }
            case EN_OFPXMT_OFB_ICMPV4_TYPE:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                sdn_hal_match->field_data.icmp_type = (uint8)sdn_db_match->value.low;
                break;
            }
            case EN_OFPXMT_OFB_ICMPV4_CODE:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                sdn_hal_match->field_data.icmp_code = (uint8)sdn_db_match->value.low;
                break;
            }
            case EN_OFPXMT_OFB_IPV6_SRC:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                memcpy(sdn_hal_match->field_data.ipv6_src_addr, &sdn_db_match->value.high, SDN_HAL_RULE_MAX_IPV6_ADDR_LEN);
                /* Mask */
                memcpy(sdn_hal_match->field_data.ipv6_src_mask, &sdn_db_match->mask.high, SDN_HAL_RULE_MAX_IPV6_ADDR_LEN);
                break;
            }
            case EN_OFPXMT_OFB_IPV6_DST:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                memcpy(sdn_hal_match->field_data.ipv6_dst_addr, &sdn_db_match->value.high, SDN_HAL_RULE_MAX_IPV6_ADDR_LEN);
                /* Mask */
                memcpy(sdn_hal_match->field_data.ipv6_dst_mask, &sdn_db_match->mask.high, SDN_HAL_RULE_MAX_IPV6_ADDR_LEN);
                break;
            }
            case EN_OFPXMT_OFB_TUNNEL_ID:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                sdn_hal_match->field_data.tunnel_id = (uint32)sdn_db_match->value.low;
                /* Mask */
                sdn_hal_match->field_data.tunnel_id_mask = (uint32)sdn_db_match->mask.low;
                break;
            }
            case EN_OFPXMT_RTK_EXP_GTP_TEID:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                /* Value */
                sdn_hal_match->field_data.gtp_teid = (uint32)sdn_db_match->value.low;
                /* Mask */
                sdn_hal_match->field_data.gtp_teid_mask = (uint32)sdn_db_match->mask.low;
                break;
            }
            case EN_OFPXMT_RTK_L2_INNER_IP_VER:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                /* Value */
                sdn_hal_match->field_data.outer_ip_ver = (uint32)sdn_db_match->value.low;
                /* Mask */
                sdn_hal_match->field_data.outer_ip_ver_mask = (uint32)sdn_db_match->mask.low;
                break;
            }
            case EN_OFPXMT_RTK_OUTER_TCP_FLG:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                /* Value */
                sdn_hal_match->field_data.outer_tcp_flag = (uint8)sdn_db_match->value.low;
                /* Mask */
                sdn_hal_match->field_data.outer_tcp_flag_mask = (uint8)sdn_db_match->mask.low;
                break;
            }
            case EN_OFPXMT_RTK_EXP_GTP_MT:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                /* Value */
                sdn_hal_match->field_data.gtp_mt = (uint8)sdn_db_match->value.low;
                /* Mask */
                sdn_hal_match->field_data.gtp_mt_mask = (uint8)sdn_db_match->mask.low;
                break;
            }
            case EN_OFPXMT_RTK_OUTER_IP_FLG:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                /* Value */
                sdn_hal_match->field_data.outer_ip_flag = (uint8)sdn_db_match->value.low;
                /* Mask */
                sdn_hal_match->field_data.outer_ip_flag_mask = (uint8)sdn_db_match->mask.low;
                break;
            }
            case EN_OFPXMT_RTK_INNER_IP_VER:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                /* Value */
                sdn_hal_match->field_data.inner_ip_ver = (uint32)sdn_db_match->value.low;
                /* Mask */
                sdn_hal_match->field_data.inner_ip_ver_mask = (uint32)sdn_db_match->mask.low;
                break;
            }
            case EN_OFPXMT_RTK_INNER_L4DP:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                /* Value */
                sdn_hal_match->field_data.inner_l4dp = (uint32)sdn_db_match->value.low;
                /* Mask */
                sdn_hal_match->field_data.inner_l4dp_mask = (uint32)sdn_db_match->mask.low;
                break;
            }
            case EN_OFPXMT_RTK_INNER_SIP:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                /* Value */
                sdn_hal_match->field_data.inner_sip = (uint32)sdn_db_match->value.low;
                /* Mask */
                sdn_hal_match->field_data.inner_sip_mask = (uint32)sdn_db_match->mask.low;
                break;
            }
            case EN_OFPXMT_RTK_INNER_DIP:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                /* Value */
                sdn_hal_match->field_data.inner_dip = (uint32)sdn_db_match->value.low;
                /* Mask */
                sdn_hal_match->field_data.inner_dip_mask = (uint32)sdn_db_match->mask.low;
                break;
            }
            case EN_OFPXMT_RTK_OUTER_FRAG_PKT:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                /* Value */
                sdn_hal_match->field_data.outer_frag_pkt = (uint8)sdn_db_match->value.low;
                /* Mask */
                sdn_hal_match->field_data.outer_frag_pkt_mask = (uint8)sdn_db_match->mask.low;
                break;
            }
            case EN_OFPXMT_RTK_INNER_IP_PROTO:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                /* Value */
                sdn_hal_match->field_data.inner_ip_proto = (uint8)sdn_db_match->value.low;
                /* Mask */
                sdn_hal_match->field_data.inner_ip_proto_mask = (uint8)sdn_db_match->mask.low;
                break;
            }
            case EN_OFPXMT_RTK_INNER_TCP_FLAG:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                /* Value */
                sdn_hal_match->field_data.inner_tcp_flag = (uint8)sdn_db_match->value.low;
                /* Mask */
                sdn_hal_match->field_data.inner_tcp_flag_mask = (uint8)sdn_db_match->mask.low;
                break;
            }
            case EN_OFPXMT_RTK_OVER_PKT_SIZE:
            {
                sdn_hal_match->match_fields_bitmap |= (1ULL << (sdn_db_match->oxm_type));
                /* Value */
                sdn_hal_match->field_data.range_chk_index = (uint16)sdn_db_match->value.low;
                /* Mask */
                sdn_hal_match->field_data.range_chk_index_mask = (uint16)sdn_db_match->mask.low;
                break;
            }
            default:
            {
                /* Others are unsupported */
                break;
            }
        }
    }
#ifdef CONFIG_FLOW_AG
    flow_data->flow_ag = sdn_hal_flow_is_aggregatable(flow->table_id, sdn_hal_match->match_fields_bitmap);
    if(flow_data->flow_ag)
    {
        for(i = 0; i < SDN_HAL_MAX_NUM_BLOCK_TEMPLATE; i++)
        {
            sdn_hal_match->match_fields_bitmap_agg[i] = sdn_hal_match->match_fields_bitmap
                & (sdn_hal_table_feature[flow->table_id].template_info.template_bitmap[i]);
        }
    }
#endif
    return SDN_HAL_RETURN_OK;
}

static int32 action_transform(sdn_hal_rule_flow_mgmt_t *flow_data, uint32 len_action, sdn_db_action_t *sdn_db_action)
{
    uint32 i;
    uint32 port;
    uint32 total_ports = 0;

    for(i = 0; i < len_action; i++)
    {
        switch(sdn_db_action[i].act_type)
        {
            case EN_OFPAT_OUTPUT:
            {
                flow_data->action_data.action_mask |= (1 << sdn_db_action[i].act_type);

                if (EN_OFPP_NORMAL == (uint32)sdn_db_action[i].value.low)
                {
                    flow_data->action_data.is_normal_output = TRUE;
                }
                else if (EN_OFPP_IN_PORT == (uint32)sdn_db_action[i].value.low)
                {
                    flow_data->action_data.phy_in_port = TRUE;
                }
                else if (EN_OFPP_FLOOD == (uint32)sdn_db_action[i].value.low)
                {
                    flow_data->action_data.flood_port = TRUE;
                }
                else if(EN_OFPP_CONTROLLER == (uint32)sdn_db_action[i].value.low)
                {
                    flow_data->action_data.controller_port = TRUE;
                }
                else if(EN_OFPP_ALL == (uint32)sdn_db_action[i].value.low)
                {
                    /*EN_OFPP_ALL excludes ingress port and EN_OFPPC_NO_FWD, we think it' also excluding controller port(cpu port).*/
                    for(port = USR_PORT_1; port <= sdn_hal_port_max_get(); port++)
                    {
                        if (port != flow_data->match_fields.field_data.l_port)
                        {
                                SDN_HAL_LIB_PORT_MASK_ADD(flow_data->action_data.port_list.bits, port);
                                //DBG_SDN("[%s][%d] output user port list.bits[0]=[0x%x].\n",__FUNCTION__,__LINE__, flow_data->action_data.port_list.bits[0]);
                        }
                    }
                    DBG_SDN("[%s][%d] output user port list.bits[0]=[0x%x] user port list.bits[1]=[0x%x].\n",__FUNCTION__,__LINE__,
                                flow_data->action_data.port_list.bits[0], flow_data->action_data.port_list.bits[1]);

                    flow_data->action_data.port_list_is_used = TRUE;
                }
                else if((total_ports == 0) && (uint32)sdn_db_action[i].value.low <= sdn_hal_port_max_get())
                {
                    flow_data->action_data.port_no = (uint32)(sdn_db_action[i].value.low);
                }
                else
                {
                    if((uint32)sdn_db_action[i].value.low > sdn_hal_port_max_get())
                        return SDN_HAL_RETURN_BAD_PORT;

                    flow_data->action_data.port_list_is_used = TRUE;
                    /* In this case the original output port is not included
                     * in the port list. Now add it */
                    if(total_ports == 1)
                    {
                        SDN_HAL_LIB_PORT_MASK_ADD(flow_data->action_data.port_list.bits, flow_data->action_data.port_no);
                    }
                    SDN_HAL_LIB_PORT_MASK_ADD(flow_data->action_data.port_list.bits, (uint32)(sdn_db_action[i].value.low));
                }
                total_ports++;
                break;
            }
            case EN_OFPAT_PUSH_VLAN:
            {
                flow_data->action_data.action_mask |= (1 << sdn_db_action[i].act_type);
                flow_data->action_data.eth_type = (uint16)sdn_db_action[i].value.low;
                break;
            }
            case EN_OFPAT_PUSH_MPLS:
            {
                flow_data->action_data.action_mask |= (1 << sdn_db_action[i].act_type);
                flow_data->action_data.eth_type = (uint16)sdn_db_action[i].value.low;
                break;
            }
            case EN_OFPAT_POP_VLAN:
            case EN_OFPAT_DEC_MPLS_TTL:
            case EN_OFPAT_DEC_NW_TTL:
            case EN_OFPAT_COPY_TTL_OUT:
            case EN_OFPAT_COPY_TTL_IN:
            {
                flow_data->action_data.action_mask |= (1 << sdn_db_action[i].act_type);
                break;
            }
            case EN_OFPAT_SET_QUEUE:
            {
                flow_data->action_data.action_mask |= (1 << sdn_db_action[i].act_type);
                flow_data->action_data.qid = (uint32)sdn_db_action[i].value.low;
                break;
            }
            case EN_OFPAT_SET_MPLS_TTL:
            {
                flow_data->action_data.action_mask |= (1 << sdn_db_action[i].act_type);
                flow_data->action_data.ttl = (uint8)sdn_db_action[i].value.low;
                break;
            }
            case EN_OFPAT_SET_NW_TTL:
            {
                flow_data->action_data.action_mask |= (1 << sdn_db_action[i].act_type);
                flow_data->action_data.ttl = (uint8)sdn_db_action[i].value.low;
                break;
            }
            case EN_OFPAT_GROUP :
            {
                flow_data->action_data.action_mask |= (1 << sdn_db_action[i].act_type);
                flow_data->action_data.group_id = (uint8)sdn_db_action[i].value.low;
                break;
            }
            case EN_OFPAT_METER :
            {
                flow_data->instruction_mask |= (1 << sdn_db_action[i].act_type);
                flow_data->meter_id = (uint8)sdn_db_action[i].value.low;
                break;
            }
            case EN_OFPAT_SET_FIELD:
            {
                flow_data->action_data.action_mask |= (1 << sdn_db_action[i].act_type);
                flow_data->action_data.ofp_oxm_type = sdn_db_action[i].oxm_type;
                /*todo: Each set field action can set all oxm type at most one times.*/
                switch(sdn_db_action[i].oxm_type)
                {
                    case EN_OFPXMT_OFB_VLAN_VID:
                    {
                        flow_data->action_data.svid = (uint16)sdn_db_action[i].value.low;
                        flow_data->action_data.set_svid = TRUE;
                        break;
                    }
                    case EN_OFPXMT_OFB_IP_DSCP:
                    {
                        flow_data->action_data.remark_val = (uint8)sdn_db_action[i].value.low;
                        flow_data->action_data.set_remark_val = TRUE;
                        break;
                    }
                    case EN_OFPXMT_OFB_VLAN_PCP:
                    {
                        flow_data->action_data.pri = (uint8)sdn_db_action[i].value.low;
                        flow_data->action_data.set_pri = TRUE;
                        break;
                    }
                    case EN_OFPXMT_OFB_TCP_SRC:
                    case EN_OFPXMT_OFB_UDP_SRC:
                    case EN_OFPXMT_OFB_SCTP_SRC:
                    {
                        flow_data->action_data.l4_src_port = (uint16)sdn_db_action[i].value.low;
                        flow_data->action_data.set_l4_src_port = TRUE;
                        break;
                    }
                    case EN_OFPXMT_OFB_TCP_DST:
                    case EN_OFPXMT_OFB_UDP_DST:
                    case EN_OFPXMT_OFB_SCTP_DST:
                    {
                        flow_data->action_data.l4_dst_port = (uint16)sdn_db_action[i].value.low;
                        flow_data->action_data.set_l4_dst_port = TRUE;
                        break;
                    }
                    case EN_OFPXMT_OFB_IPV4_SRC:
                    {
                        flow_data->action_data.src_ip = (uint32)sdn_db_action[i].value.low;
                        flow_data->action_data.set_src_ip = TRUE;
                        break;
                    }
                    case EN_OFPXMT_OFB_IPV4_DST:
                    {
                        flow_data->action_data.dst_ip = (uint32)sdn_db_action[i].value.low;
                        flow_data->action_data.set_dst_ip = TRUE;
                        break;
                    }
                    case EN_OFPXMT_OFB_ETH_SRC:
                    {
                        memcpy(flow_data->action_data.src_mac, ((char*)&sdn_db_action[i].value.low) + 2, OFP_ETH_ALEN);
                        flow_data->action_data.set_src_mac = TRUE;
                        break;
                    }
                    case EN_OFPXMT_OFB_ETH_DST:
                    {
                        memcpy(flow_data->action_data.dst_mac, ((char*)&sdn_db_action[i].value.low) + 2, OFP_ETH_ALEN);
                        flow_data->action_data.set_dst_mac = TRUE;
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
                break;
            }
            default:
            {
                /* Others are unsupported */
                break;
            }

        }
    }

    return SDN_HAL_RETURN_OK;
}
static int32 assign_counter_mode(const sdn_db_flow_entry_t *flow)
{
    uint32 unit_id;
    uint32 ret = SDN_HAL_RETURN_FAILED;
    uint32 flags;
    rtk_of_flowtable_phase_t table_phase;
    rtk_of_flowCntMode_t counter_mode;

    unit_id = SDN_HAL_MY_UNIT_ID();
    table_phase = sdn_hal_rule_get_flow_table(flow->table_id);
    flags = flow->flags;

    if(table_phase == FT_PHASE_IGR_FT_1 || table_phase == FT_PHASE_IGR_FT_2)
        return SDN_HAL_RETURN_OK;

    if((flags & EN_OFPFF_NO_PKT_COUNTS) && (flags & EN_OFPFF_NO_BYT_COUNTS))
    {
        counter_mode = OF_FLOW_CNTMODE_DISABLE;
    }
    else if(!(flags & EN_OFPFF_NO_PKT_COUNTS) && (flags & EN_OFPFF_NO_BYT_COUNTS))
    {
        counter_mode = OF_FLOW_CNTMODE_PACKET_CNT_AND_PACKET_CNT_TH;
    }
    else if ((flags & EN_OFPFF_NO_PKT_COUNTS) && !(flags & EN_OFPFF_NO_BYT_COUNTS))
    {
        counter_mode = OF_FLOW_CNTMODE_BYTE_CNT_AND_BYTE_CNT_TH;
    }
    else
    {
        counter_mode = OF_FLOW_CNTMODE_PACKET_CNT_AND_BYTE_CNT;
    }

    if(RT_ERR_OK != (ret = rtk_of_flowCntMode_set(unit_id, table_phase, flow->flow_index, counter_mode)))
        return ret;

    return SDN_HAL_RETURN_OK;
}

static int32 inst_transform(sdn_hal_rule_flow_mgmt_t *flow_data, const sdn_db_flow_entry_t *flow)
{

    uint32  i;
    uint32  len_action = 0;
    int32   rc;
    sdn_db_instruction_t  *sdn_db_inst = NULL;
    sdn_db_action_t *sdn_db_action = NULL;
    for(i = 0; i < flow->len_inst; i++)
    {
        sdn_db_inst = &flow->instruction_p[i];
        switch(sdn_db_inst->type)
        {

            case EN_OFPIT_WRITE_ACTIONS:
            {
                sdn_db_inst_write_actions_t *inst_write_action = NULL;
                if (sdn_db_inst->data_p)
                {
                    inst_write_action = (sdn_db_inst_write_actions_t*)sdn_db_inst->data_p;
                    sdn_db_action = inst_write_action->action_set;
                    len_action = inst_write_action->len;
                    rc = action_transform(flow_data, len_action, sdn_db_action);

                    if (rc != SDN_HAL_RETURN_OK)
                    {
                        DBG_SDN("[%s][%d] action_transform fail, rc=[%d]\n",__FUNCTION__,__LINE__,rc);
                        return rc;
                    }
                    flow_data->instruction_mask |= (1 << sdn_db_inst->type);
                }
                break;
            }
            case EN_OFPIT_APPLY_ACTIONS:
            {
                sdn_db_inst_apply_actions_t *inst_apply_action = NULL;
                if (sdn_db_inst->data_p)
                {
                    inst_apply_action = (sdn_db_inst_apply_actions_t*)sdn_db_inst->data_p;
                    sdn_db_action = inst_apply_action->action_set;
                    len_action = inst_apply_action->len;
                    rc = action_transform(flow_data, len_action, sdn_db_action);

                    if (rc != SDN_HAL_RETURN_OK)
                    {
                        DBG_SDN("[%s][%d] action_transform fail, rc=[%d]\n",__FUNCTION__,__LINE__,rc);
                        return rc;
                    }
                    flow_data->instruction_mask |= (1 << sdn_db_inst->type);
                }
                break;
            }

            case EN_OFPIT_METER:
            {
                sdn_db_inst_meter_t *inst_meter;

                /* phku, 2014/10/31, tell the caller this flow wants to use a meter
                 * (in OpenFlow meter_id can not be 0, the initial value just means no need to assign a meter)
                 */
                inst_meter = (sdn_db_inst_meter_t*)sdn_db_inst->data_p;
                flow_data->meter_id = inst_meter->meter_id;
                flow_data->instruction_mask |= (1 << sdn_db_inst->type);
                break;
            }
            case EN_OFPIT_GOTO_TABLE:
            {
                sdn_db_inst_goto_table_t *inst_goto;
                inst_goto = (sdn_db_inst_goto_table_t*)sdn_db_inst->data_p;
                flow_data->goto_table_id = inst_goto->table_id;
                flow_data->instruction_mask |= (1 << sdn_db_inst->type);
                break;
            }
            case EN_OFPIT_CLEAR_ACTIONS:
            {
                sdn_db_inst_clear_actions_t *clear_action;
                clear_action = (sdn_db_inst_clear_actions_t*)sdn_db_inst->data_p;
                flow_data->clear_action = clear_action->clear_enable;
                flow_data->instruction_mask |= (1 << sdn_db_inst->type);
                break;
            }
            case EN_OFPIT_WRITE_METADATA:
            {
                sdn_db_inst_write_metadata_t *inst_metaData;
                inst_metaData = (sdn_db_inst_write_metadata_t*)sdn_db_inst->data_p;
                flow_data->metadata = inst_metaData->metadata;
                flow_data->metadata_mask = inst_metaData->metadata_mask;
                flow_data->instruction_mask |= (1 << sdn_db_inst->type);
                break;
            }
            default:
            {
                DBG_SDN("[%s][%d] Unknown Instruction!\n",__FUNCTION__,__LINE__);
                return SDN_HAL_RETURN_FAILED;
                /* Others are unsupported */
                break;
            }

        }
    }

    return SDN_HAL_RETURN_OK;
}

//int32 sdn_hal_flow_entry_add(struct ofp_flow_mod *fm, uint8 match_num, uint8 inst_num, struct ofp_instruction_header *inst)
int32 sdn_hal_flow_entry_add(sdn_db_flow_entry_t *flow_entry_p)
{
    int32 ret = SDN_HAL_RETURN_FAILED;
    rtk_of_flowtable_phase_t phase;
    sdn_hal_flow_t flow;
    sdn_hal_rule_flow_mgmt_t flow_data;

    memset(&flow_data, 0, sizeof(sdn_hal_rule_flow_mgmt_t));
    memset(&flow, 0, sizeof(sdn_hal_flow_t));

    match_transform(&flow_data, flow_entry_p);
    inst_transform(&flow_data, flow_entry_p);

    flow_data.table_id = flow_entry_p->table_id;
    phase = sdn_hal_rule_get_flow_table(flow_entry_p->table_id);
    _sdn_hal_logicalId_to_physicalId(phase, flow_entry_p->flow_index, &flow_data.flow_id);

    if ((ret = sdn_hal_flow_IsVaild(flow_entry_p)) != SDN_HAL_RETURN_OK)
    {
        DBG_SDN("sdn_hal_flow_IsVaild failed! ret = %d  line:%d\n", ret, __LINE__);
        return SDN_HAL_RETURN_FAILED;
    }

    if ((ret = sdn_hal_flow_instruction_fill(phase, &flow_data, &flow)) != SDN_HAL_RETURN_OK)
    {
        DBG_SDN("sdn_hal_flow_instruction_fill failed! ret = %d  line:%d\n", ret, __LINE__);
        return ret;
    }

    if ((ret = sdn_hal_flow_matchField_fill(phase, &flow_data, &flow)) != SDN_HAL_RETURN_OK)
    {
        DBG_SDN("sdn_hal_flow_matchField_fill failed! ret = %d  line:%d\n", ret, __LINE__);
        return ret;
    }

    if ((ret = sdn_hal_flow_action_fill(phase, &flow_data, &flow)) != SDN_HAL_RETURN_OK)
    {
        DBG_SDN("sdn_hal_flow_action_fill failed! ret = %d  line:%d\n", ret, __LINE__);
        return ret;
    }

    if ((ret = assign_counter_mode(flow_entry_p)) !=  SDN_HAL_RETURN_OK)
    {
        DBG_SDN("assign_counter_mode failed! ret = %d  line:%d\n", ret, __LINE__);
        return SDN_HAL_RETURN_FAILED;
    }

    if ((ret = sdn_hal_flow_set(phase, &flow_data, &flow)) != SDN_HAL_RETURN_OK)
    {
        DBG_SDN("sdn_hal_flow_set failed! ret = %d  line:%d\n", ret, __LINE__);
        return SDN_HAL_RETURN_FAILED;
    }

    return SDN_HAL_RETURN_OK;

}

int32
_sdn_hal_l3_cam_entry_del(uint32 unit, sdn_hal_rule_flow_mgmt_t *flow_data)
{
    uint32 i, j;
    uint32 l3_physical_id;
    uint32 l3_logical_id;
    uint32 l3_logical_found = 0;
    rtk_of_flowMove_t l3_cam_move;
    int32   ret = RT_ERR_OK;

    memset(&l3_cam_move, 0, sizeof(rtk_of_flowMove_t));
    l3_physical_id = flow_data->flow_id;
    _sdn_hal_physicalId_to_logicalId(FT_PHASE_IGR_FT_2, l3_physical_id , &l3_logical_id);

    for( i = 0; i < l3_cam_flow_count; i++)
    {
        if(l3_logical_id == l3_cam_logic_id[i])
        {
            l3_logical_found = 1;
            break;
        }
    }

    if(l3_logical_found == 0)
        return SDN_HAL_RETURN_FAILED;

    if ( RT_ERR_OK != rtk_of_l3CamFlowEntry_del(unit, l3_physical_id))
    {
        return SDN_HAL_RETURN_FAILED;
    }

    /*move database.*/
    if ((l3_logical_id == l3_cam_logic_id[l3_cam_flow_count - 1]))
    {
        l3_cam_logic_id[l3_cam_flow_count - 1] = 0;
    }
    else
    {
        /*Compare logical flow index to find an appropriate position.*/
        for( i = 0; i < l3_cam_flow_count; i++)
        {
            if(l3_logical_id == l3_cam_logic_id[i])
            {
                /*physical flow entry move.*/
                l3_cam_move.length = (l3_cam_flow_count - l3_logical_id - 1) * SDN_HAL_INGRESS_L3_DIV_FACTOR;
                l3_cam_move.move_from = (l3_logical_id + 1) * SDN_HAL_INGRESS_L3_DIV_FACTOR;
                l3_cam_move.move_to = (l3_logical_id) * SDN_HAL_INGRESS_L3_DIV_FACTOR;
                SDN_HAL_MSG("\n[%s] length = %d, from = %d, to = %d\n",__FUNCTION__,l3_cam_move.length,l3_cam_move.move_from,l3_cam_move.move_to);
                ret = rtk_of_l3CamFlowEntry_move(unit, &l3_cam_move);
                if (RT_ERR_OK != ret)
                {
                    SDN_HAL_MSG("\n[%s] rtk_of_l3CamFlowEntry_move() ret = 0x%08x\n",__FUNCTION__,ret);
                    return SDN_HAL_RETURN_FAILED;
                }

                for(j = i; j < l3_cam_flow_count; j++)
                {
                    l3_cam_logic_id[j] = l3_cam_logic_id[j + 1];
                }
                break;
            }
        }
    }

    l3_cam_flow_count--;
    return SDN_HAL_RETURN_OK; /*Add by MSP*/
}
int32 sdn_hal_flow_disable(uint32 unit, rtk_of_flowtable_phase_t phase, sdn_hal_rule_flow_mgmt_t *flow_data)
{
    sdn_hal_flow_t flow;
    uint32 flow_id;
    uint32 j = 0;

    memset(&flow, 0, sizeof(sdn_hal_flow_t));

    switch(phase)
    {
        case FT_PHASE_IGR_FT_0:
        case FT_PHASE_IGR_FT_3:
        {
#ifndef CONFIG_FLOW_AG
            flow_id = flow_data->flow_id;
#else
            for(j = 0; j < SDN_HAL_MAX_NUM_BLOCK_TEMPLATE; j++)
            {
#endif
                flow_id = flow_data->flow_id + j;
                if (rtk_of_flowEntryValidate_set(unit, phase, flow_id, DISABLED) != RT_ERR_OK)
                {
                    DBG_SDN("rtk_of_flowEntryValidate_set failed\n");
                    return SDN_HAL_RETURN_FAILED;
                }
#ifndef CONFIG_FLOW_AG
#else
            }
#endif
            break;
        }
        case FT_PHASE_EGR_FT_0:
        {
            if (rtk_of_flowEntryValidate_set(unit, phase, flow_data->flow_id, DISABLED) != RT_ERR_OK)
            {
                DBG_SDN("rtk_of_flowEntryValidate_set failed\n");
                return SDN_HAL_RETURN_FAILED;
            }
            break;
        }
        case FT_PHASE_IGR_FT_1:
        {
            if (sdn_hal_flow_matchField_fill(phase, flow_data, &flow) != SDN_HAL_RETURN_OK)
            {
                DBG_SDN("sdn_hal_flow_matchField_fill failed\n");
                return SDN_HAL_RETURN_FAILED;
            }

            if (rtk_of_l2FlowEntry_del(unit, &flow.ft1_flow) != RT_ERR_OK)
            {
                DBG_SDN("rtk_of_l2FlowEntry_del failed\n");
                return SDN_HAL_RETURN_FAILED;
            }
            break;
        }
        case FT_PHASE_IGR_FT_2:
        {
            if(_sdn_hal_l3Cam_is_used(flow_data))
            {
                if(SDN_HAL_RETURN_OK != _sdn_hal_l3_cam_entry_del(unit, flow_data))
                    return SDN_HAL_RETURN_FAILED;
            }
            else
            {
                sdn_hal_flow_matchField_fill(phase, flow_data, &flow);

                if ( RT_ERR_OK != rtk_of_l3HashFlowEntry_del(unit, &flow.ft2_flow.ft2_hash_flow))
                {
                    return SDN_HAL_RETURN_FAILED;
                }
            }
            break;
        }
        default:
            break;
    }
    return SDN_HAL_RETURN_OK;
}


//int32 sdn_hal_flow_entry_modify(struct ofp_flow_mod *fm, uint8 match_num, uint8 inst_num, struct ofp_instruction_header *inst)
int32 sdn_hal_flow_entry_modify(sdn_db_flow_entry_t *flow_entry_p)
{
    int32 ret = SDN_HAL_RETURN_FAILED;
    uint32 unit = 0;
    sdn_hal_flow_t flow;
    rtk_of_flowtable_phase_t phase;
    sdn_hal_rule_flow_mgmt_t flow_data;

    memset(&flow_data, 0, sizeof(sdn_hal_rule_flow_mgmt_t));
    memset(&flow, 0, sizeof(sdn_hal_flow_t));

    match_transform(&flow_data, flow_entry_p);
    inst_transform(&flow_data, flow_entry_p);

    unit = SDN_HAL_MY_UNIT_ID();
    flow_data.table_id = flow_entry_p->table_id;
    phase = sdn_hal_rule_get_flow_table(flow_entry_p->table_id);
    _sdn_hal_logicalId_to_physicalId(phase, flow_entry_p->flow_index, &flow_data.flow_id);

    if ((ret = sdn_hal_flow_IsVaild(flow_entry_p)) != SDN_HAL_RETURN_OK)
    {
        DBG_SDN("sdn_hal_flow_IsVaild check failed\n");
        return SDN_HAL_RETURN_FAILED;
    }

    if (sdn_hal_flow_disable(unit, phase, &flow_data) != SDN_HAL_RETURN_OK)
    {
        DBG_SDN("sdn_hal_flow_disable failed\n");
        return SDN_HAL_RETURN_FAILED;
    }

    if (sdn_hal_flow_instruction_fill(phase, &flow_data, &flow) != SDN_HAL_RETURN_OK)
    {
        DBG_SDN("sdn_hal_flow_instruction_fill failed\n");
        return SDN_HAL_RETURN_FAILED;
    }

    if (sdn_hal_flow_matchField_fill(phase, &flow_data, &flow) != SDN_HAL_RETURN_OK)
    {
        DBG_SDN("sdn_hal_flow_matchField_fill failed\n");
        return SDN_HAL_RETURN_FAILED;
    }

    if (sdn_hal_flow_action_fill(phase, &flow_data, &flow) !=  SDN_HAL_RETURN_OK)
    {
        DBG_SDN("sdn_hal_flow_action_fill failed\n");
        return SDN_HAL_RETURN_FAILED;
    }

    if (assign_counter_mode(flow_entry_p) !=  SDN_HAL_RETURN_OK)
    {
        DBG_SDN("assign_counter_mode failed\n");
        return SDN_HAL_RETURN_FAILED;
    }

    if (sdn_hal_flow_set(phase, &flow_data, &flow) !=  SDN_HAL_RETURN_OK)
    {
        DBG_SDN("sdn_hal_flow_set failed\n");
        return SDN_HAL_RETURN_FAILED;
    }

    return SDN_HAL_RETURN_OK;
}

int32 _sdn_hal_flow_entry_delete(uint32 unit, rtk_of_flowtable_phase_t phase, sdn_hal_rule_flow_mgmt_t *flow_data)
{
    rtk_of_flowClear_t del_flow_idx;
    sdn_hal_flow_t flow;

    memset(&del_flow_idx, 0, sizeof(del_flow_idx));
    memset(&flow, 0, sizeof(sdn_hal_flow_t));
    del_flow_idx.start_idx = flow_data->flow_id;
    del_flow_idx.end_idx   = flow_data->flow_id;

    switch(phase)
    {
        case FT_PHASE_IGR_FT_0:
        case FT_PHASE_IGR_FT_3:
        {
#ifdef CONFIG_FLOW_AG
            del_flow_idx.end_idx   = flow_data->flow_id + 1;
#endif
            if ( RT_ERR_OK != rtk_of_flowEntry_del(unit, phase, &del_flow_idx))
            {
                return SDN_HAL_RETURN_FAILED;
            }
            break;
        }
        case FT_PHASE_EGR_FT_0:
        {
            if ( RT_ERR_OK != rtk_of_flowEntry_del(unit, phase, &del_flow_idx))
            {
                return SDN_HAL_RETURN_FAILED;
            }
            break;
        }
        case FT_PHASE_IGR_FT_1:
        {
            sdn_hal_flow_matchField_fill(phase, flow_data, &flow);

            if ( RT_ERR_OK != rtk_of_l2FlowEntry_del(unit, &flow.ft1_flow))
            {
                return SDN_HAL_RETURN_FAILED;
            }
            break;
        }
        case FT_PHASE_IGR_FT_2:
        {
            if(_sdn_hal_l3Cam_is_used(flow_data))
            {
                if(SDN_HAL_RETURN_OK != _sdn_hal_l3_cam_entry_del(unit, flow_data))
                    return SDN_HAL_RETURN_FAILED;
            }
            else
            {
                sdn_hal_flow_matchField_fill(phase, flow_data, &flow);

                if ( RT_ERR_OK != rtk_of_l3HashFlowEntry_del(unit, &flow.ft2_flow.ft2_hash_flow))
                {
                    return SDN_HAL_RETURN_FAILED;
                }
            }
            break;
        }
        default:
        {
            break;
        }
    }
    return SDN_HAL_RETURN_OK;
}


//int32 sdn_hal_flow_entry_delete(struct ofp_flow_delete *fm, uint8 match_num)
int32 sdn_hal_flow_entry_delete(sdn_db_flow_entry_t *flow_entry_p)
{
    uint32 ret = SDN_HAL_RETURN_FAILED;
    uint32 unit = 0;
    rtk_of_flowtable_phase_t phase = 0;
    sdn_hal_rule_flow_mgmt_t flow_data;

    if ((ret = sdn_hal_flow_IsVaild(flow_entry_p)) != SDN_HAL_RETURN_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }
    memset(&flow_data, 0, sizeof(sdn_hal_rule_flow_mgmt_t));
    match_transform(&flow_data, flow_entry_p);

    unit = SDN_HAL_MY_UNIT_ID();
    flow_data.table_id = flow_entry_p->table_id;
    phase = sdn_hal_rule_get_flow_table(flow_entry_p->table_id);
    _sdn_hal_logicalId_to_physicalId(phase, flow_entry_p->flow_index, &flow_data.flow_id);

    if ( SDN_HAL_RETURN_OK != _sdn_hal_flow_entry_delete(unit, phase, &flow_data))
    {
        return SDN_HAL_RETURN_FAILED;
    }

    return SDN_HAL_RETURN_OK;
}
int32 sdn_hal_rule_get_flow_table(uint32 tableId)
{
    switch (tableId)
    {
        case 0:
            return FT_PHASE_IGR_FT_0;
            break;
        case 1:
            return FT_PHASE_IGR_FT_1;
            break;
        case 2:
            return FT_PHASE_IGR_FT_2;
            break;
        case 3:
            return FT_PHASE_IGR_FT_3;
            break;
        case 4:
            return FT_PHASE_EGR_FT_0;
            break;
        default:
            return FT_PHASE_END;
            break;
    }
}

static int32 _sdn_hal_flow_ft0_matchField_fill(sdn_hal_rule_flow_mgmt_t *flow_entry_p, sdn_hal_flow_t *flow)
{
    uint32 fields_idx           = 0;
    uint32 unit_id              = 0;
    uint32 get_port             = 0xff;
    uint8  phy_in_port          = 0;
    uint16 mask16               = 0xffff;
    uint8  mask8                = 0xff;
    uint16 port_no              = 0;
    uint16 metadata             = 0;
    uint8 template_mask         = 0xf;
    uint8 template_id           = 0xff;

    uint32 teid                 = 0;
    uint32 mask32               = 0xffffffff;
    uint16 field_sel16          = 0;
    uint16 field_sel16_mask     = 0xffff;
    uint8  field_sel8           = 0;
    uint8  field_sel8_mask      = 0xff;


    rtk_of_flow_id_t flow_id = 0;

    rtk_of_flowtable_phase_t table_phase;

    unit_id = SDN_HAL_MY_UNIT_ID();

    is_inport_existed = FALSE;

#ifndef CONFIG_FLOW_AG
    flow_id = flow_entry_p->flow_id;
#else
    int i;
    for(i = 0; i < SDN_HAL_MAX_NUM_BLOCK_TEMPLATE; i++)
    {
        flow_id = flow_entry_p->flow_id + i;
        flow_entry_p->match_fields.match_fields_bitmap = flow_entry_p->match_fields.match_fields_bitmap_agg[i];
#endif
        if(SDN_HAL_RETURN_OK != sdn_hal_templateId_chosen(flow_entry_p->table_id,
                                         &template_id,
                                         flow_entry_p->match_fields.match_fields_bitmap))
        {
            DBG_SDN("sdn_hal_templateId_chosen failed!\n");
            return SDN_HAL_RETURN_FAILED;
        }

        table_phase = sdn_hal_rule_get_flow_table(flow_entry_p->table_id);

        DBG_SDN("flow_entry_p->chosen_template_id %d flow_id%d, table_phase%d ret%d\n", template_id, flow_id, table_phase, rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_TEMPLATE_ID,
            &template_id,
            &template_mask));

        if(RT_ERR_OK != rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_TEMPLATE_ID,
            &template_id,
            &template_mask))
        {
            DBG_SDN("rtk_of_flowEntryField_write template write failed!\n");
            return SDN_HAL_RETURN_FAILED;
        }


        fields_idx = 0;

        while (fields_idx < EN_OFPXMT_RTK_EXP_MAX) /* Modify for MEC DPA */
//        while (fields_idx < EN_OFPXMT_OFB_MAX)
        {
            if (!HAL_LIB_CHK_BIT_ON(flow_entry_p->match_fields.match_fields_bitmap, fields_idx))
            {
               fields_idx++;
               continue;
            }
            DBG_SDN("\r\n[%s-%d]set unit(%d), phase(%d), entry(%d), field(%d)\r\n", __FUNCTION__, __LINE__,
                                                                        (uint32)unit_id,
                                                                        (uint32)table_phase,
                                                                        flow_entry_p->flow_id, fields_idx);
            mask16 = 0xffff;
            switch((EN_OFP_OXM_OFB_MATCH_FIELDS_T)fields_idx)
            {
                case EN_OFPXMT_OFB_IN_PORT:
                {
                    sdn_hal_logicalPort2PhyPort(flow_entry_p->match_fields.field_data.l_port, &get_port);
                    phy_in_port = get_port;
                    is_inport_existed = TRUE;
//                    SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_phase, flow_id,
//                                                     MATCH_FIELD_IN_PHY_PORT, (uint8 *)&phy_in_port, (uint8 *)&mask8);
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_IN_PHY_PORT, (uint8 *)&phy_in_port, (uint8 *)&mask8);
                    break;
                }
                case EN_OFPXMT_OFB_IN_PHY_PORT:
                {
                    phy_in_port = flow_entry_p->match_fields.field_data.l_port;
//                    SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_phase, flow_id,
//                                                     MATCH_FIELD_IN_PHY_PORT, (uint8 *)&phy_in_port, (uint8 *)&mask8);
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_IN_PHY_PORT, (uint8 *)&phy_in_port, (uint8 *)&mask8);
                    break;
                }
                case EN_OFPXMT_OFB_METADATA:
                {
                    metadata = flow_entry_p->match_fields.field_data.metadata;
//                    SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_phase, flow_id,
//                                                     MATCH_FIELD_METADATA, (uint8 *)&metadata, (uint8 *)&mask16);
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_METADATA, (uint8 *)&metadata, (uint8 *)&mask16);
                    break;
                }
                case EN_OFPXMT_OFB_ARP_THA:
                {
                    DBG_SDN("\r\n[%s-%d]dst mac:%02x:%02x:%02x:%02x:%02x:%02x \r\n", __FUNCTION__, __LINE__,
                            flow_entry_p->match_fields.field_data.dst_mac[0],
                            flow_entry_p->match_fields.field_data.dst_mac[1],
                            flow_entry_p->match_fields.field_data.dst_mac[2],
                            flow_entry_p->match_fields.field_data.dst_mac[3],
                            flow_entry_p->match_fields.field_data.dst_mac[4],
                            flow_entry_p->match_fields.field_data.dst_mac[5]);
//                    SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_phase, flow_id,
//                                                     MATCH_FIELD_ARP_THA, flow_entry_p->match_fields.field_data.dst_mac, flow_entry_p->match_fields.field_data.dst_mac_mask);
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_ARP_THA, flow_entry_p->match_fields.field_data.dst_mac, flow_entry_p->match_fields.field_data.dst_mac_mask);
                    break;
                }
                case EN_OFPXMT_OFB_ETH_DST:
                {
                    DBG_SDN("\r\n[%s-%d]dst mac:%02x:%02x:%02x:%02x:%02x:%02x \r\n", __FUNCTION__, __LINE__,
                            flow_entry_p->match_fields.field_data.dst_mac[0],
                            flow_entry_p->match_fields.field_data.dst_mac[1],
                            flow_entry_p->match_fields.field_data.dst_mac[2],
                            flow_entry_p->match_fields.field_data.dst_mac[3],
                            flow_entry_p->match_fields.field_data.dst_mac[4],
                            flow_entry_p->match_fields.field_data.dst_mac[5]);
//                    SDN_HAL_APPLY_FIELD_VALUE(unit_id, table_phase, flow_id,
//                                                     MATCH_FIELD_ETH_DST, flow_entry_p->match_fields.field_data.dst_mac, flow_entry_p->match_fields.field_data.dst_mac_mask);
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_ETH_DST, flow_entry_p->match_fields.field_data.dst_mac, flow_entry_p->match_fields.field_data.dst_mac_mask);
                    break;
                }
                case EN_OFPXMT_OFB_ARP_SHA:
                {
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

                            rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_ARP_SHA, flow_entry_p->match_fields.field_data.src_mac, flow_entry_p->match_fields.field_data.src_mac_mask);
                    break;
                }
                case EN_OFPXMT_OFB_ETH_SRC:
                {
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

                            rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                    MATCH_FIELD_ETH_SRC, flow_entry_p->match_fields.field_data.src_mac, flow_entry_p->match_fields.field_data.src_mac_mask);
                    break;
                }
                case EN_OFPXMT_OFB_ETH_TYPE:
                {
                     uint16 eth_type = flow_entry_p->match_fields.field_data.eth_type_val;
                     DBG_SDN("\r\n[%s-%d] eth_type:%02x\r\n", __FUNCTION__, __LINE__, eth_type);
                     rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                     MATCH_FIELD_ETH_TYPE, (uint8 *)&eth_type, (uint8 *)&mask16);

                     break;
                }
                case EN_OFPXMT_OFB_VLAN_VID:
                {
                    DBG_SDN("\r\n[%s-%d]Make sure that  is_default_template=[%s]  \r\n",__FUNCTION__, __LINE__,
                                          (is_default_template) ? "TRUE":"FALSE");

                     DBG_SDN("\r\n[%s-%d]  svid=[%d]\r\n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.svid);
                     DBG_SDN("\r\n[%s-%d]  cvid=[%d]\r\n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.cvid);
                    if(flow_entry_p->match_fields.field_data.svid == 0)
                    {
                        rtk_of_flowEntryField_write(unit_id, table_phase,
                                        flow_id,
                                        MATCH_FIELD_OTAG_EXIST,
                                        (uint8 *)&flow_entry_p->match_fields.field_data.svid,
                                        (uint8 *)&mask16);
                    }
                    else
                    {
                        rtk_of_flowEntryField_write(unit_id, table_phase,
                                        flow_id,
                                        MATCH_FIELD_OTAG_VID,
                                        (uint8 *)&flow_entry_p->match_fields.field_data.svid,
                                        (uint8 *)&mask16);
                    }
                    break;
                }
                case EN_OFPXMT_OFB_VLAN_PCP:
                {
                    DBG_SDN("\r\n[%s-%d] pcp_value=[%d]\r\n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.pcp_value);

                    rtk_of_flowEntryField_write(unit_id, table_phase,
                                        flow_id,
                                        MATCH_FIELD_OTAG_PCP,
                                        &flow_entry_p->match_fields.field_data.pcp_value, &flow_entry_p->match_fields.field_data.pcp_mask);
                    break;
                }
                case EN_OFPXMT_OFB_IP_PROTO:
                {
                    uint8 mask8 = 0xff;
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                     MATCH_FIELD_IP_PROTO, &flow_entry_p->match_fields.field_data.protocol_val, &mask8);
                    break;
                }
                case EN_OFPXMT_OFB_ARP_OP:
                {
                    mask16 = 0xffff;
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                     MATCH_FIELD_ARP_OP, (uint8 *)&flow_entry_p->match_fields.field_data.arp_op, (uint8 *)&mask16);
                    break;
                }
                case EN_OFPXMT_OFB_IP_ECN:
                {
                    mask8 = 0x3;
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                 MATCH_FIELD_IP_ECN, (uint8 *)&flow_entry_p->match_fields.field_data.ip_ecn, (uint8 *)&mask8);
                    break;
                }
                case EN_OFPXMT_OFB_IP_DSCP:
                {
                    mask8=0x3f;
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                    MATCH_FIELD_IP_DSCP, (uint8 *)&flow_entry_p->match_fields.field_data.dscp, (uint8 *)&mask8);

                    break;
                }
                case EN_OFPXMT_OFB_IPV4_SRC:
                case EN_OFPXMT_OFB_IPV6_SRC:
                {
                    if (fields_idx == EN_OFPXMT_OFB_IPV4_SRC)
                    {
                        rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                         MATCH_FIELD_IPV4_SRC, (uint8 *)&flow_entry_p->match_fields.field_data.src_ipv4_addr, (uint8 *)&flow_entry_p->match_fields.field_data.src_ipv4_mask);
                    }
                    else
                    {
                        rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                        MATCH_FIELD_IPV6_SRC, flow_entry_p->match_fields.field_data.ipv6_src_addr, flow_entry_p->match_fields.field_data.ipv6_src_mask);
                    }
                    break;
                }
                case EN_OFPXMT_OFB_IPV4_DST:
                case EN_OFPXMT_OFB_IPV6_DST:
                {
                    if (fields_idx == EN_OFPXMT_OFB_IPV4_DST)
                    {
                         rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                         MATCH_FIELD_IPV4_DST, (uint8 *)&flow_entry_p->match_fields.field_data.dst_ipv4_addr, (uint8 *)&flow_entry_p->match_fields.field_data.dst_ipv4_mask);
                    }
                    else
                    {
                         rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                         MATCH_FIELD_IPV6_DST, flow_entry_p->match_fields.field_data.ipv6_dst_addr, flow_entry_p->match_fields.field_data.ipv6_dst_mask);
                    }
                    break;
                }
                case EN_OFPXMT_OFB_SCTP_SRC:
                case EN_OFPXMT_OFB_TCP_SRC:
                case EN_OFPXMT_OFB_UDP_SRC:
                {
                    mask16 = 0xffff;

                    if(fields_idx == EN_OFPXMT_OFB_TCP_SRC)
                    {
                        port_no = flow_entry_p->match_fields.field_data.tcp_src_port_no;
                    }
                    else if (fields_idx == EN_OFPXMT_OFB_UDP_SRC)
                    {
                        port_no = flow_entry_p->match_fields.field_data.udp_src_port_no;
                    }
                    else
                    {
                        port_no = flow_entry_p->match_fields.field_data.sctp_src_port_no;
                    }
                    DBG_SDN(" \n[%s-%d] source port no=[%d] \n", __FUNCTION__, __LINE__,port_no);

                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                     MATCH_FIELD_L4_SRC_PORT, (uint8 *)&port_no, (uint8 *)&mask16);
                    break;
                }
                case EN_OFPXMT_OFB_SCTP_DST:
                case EN_OFPXMT_OFB_TCP_DST:
                case EN_OFPXMT_OFB_UDP_DST:
                {
                    mask16 = 0xffff;
                    if(fields_idx == EN_OFPXMT_OFB_TCP_DST)
                    {
                        port_no = flow_entry_p->match_fields.field_data.tcp_dst_port_no;
                    }
                    else if (fields_idx == EN_OFPXMT_OFB_UDP_DST)
                    {
                        port_no = flow_entry_p->match_fields.field_data.udp_dst_port_no;
                    }
                    else
                    {
                        port_no = flow_entry_p->match_fields.field_data.sctp_dst_port_no;
                    }

                    DBG_SDN(" \n[%s-%d] destination port no=[%d] \n", __FUNCTION__, __LINE__,port_no);

                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                     MATCH_FIELD_L4_DST_PORT, (uint8 *)&port_no, (uint8 *)&mask16);
                    break;
                }
                case EN_OFPXMT_OFB_ICMPV4_TYPE:
                case EN_OFPXMT_OFB_ICMPV6_TYPE:
                {
                    mask16 = 0xffff;
                    DBG_SDN(" \n[%s-%d] icmp type=[%d] \n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.icmp_type);

                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                     MATCH_FIELD_ICMP_TYPE, &flow_entry_p->match_fields.field_data.icmp_type, (uint8 *)&mask16);
                    break;
                }
                case EN_OFPXMT_OFB_ICMPV4_CODE:
                case EN_OFPXMT_OFB_ICMPV6_CODE:
                {
                    mask16 = 0xffff;
                    DBG_SDN(" \n[%s-%d] icmp code=[%d] \n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.icmp_code);

                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                         MATCH_FIELD_ICMP_CODE, &flow_entry_p->match_fields.field_data.icmp_code, (uint8 *)&mask16);
                    break;
                }
                case EN_OFPXMT_OFB_TUNNEL_ID:
                {
#ifdef CONFIG_SDN_HAL_DBG
                    uint8 *tunnel_id_p =  (uint8*)&flow_entry_p->match_fields.field_data.tunnel_id;
                    DBG_SDN("tunnel id: 0x%x\r\n", flow_entry_p->match_fields.field_data.tunnel_id);
#endif
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                    MATCH_FIELD_GTP_TEID, (uint8 *)&flow_entry_p->match_fields.field_data.tunnel_id, (uint8 *)&flow_entry_p->match_fields.field_data.tunnel_id_mask);
                    break;
                }
                case EN_OFPXMT_RTK_EXP_GTP_TEID:
                {
                    teid = flow_entry_p->match_fields.field_data.gtp_teid;
                    mask32 = flow_entry_p->match_fields.field_data.gtp_teid_mask;
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_GTP_TEID, (uint8 *)&teid, (uint8 *)&mask32);
                    DBG_SDN("\n[%s][%d] EN_OFPXMT_RTK_EXP_GTP_TEID, teid = 0x%08x, mask32 = 0x%08x\n",__FUNCTION__,__LINE__,teid,mask32);
                    break;
                }
                case EN_OFPXMT_RTK_L2_INNER_IP_VER:
                {
                    field_sel16 = flow_entry_p->match_fields.field_data.outer_ip_ver;
                    field_sel16_mask = flow_entry_p->match_fields.field_data.outer_ip_ver_mask;
                    field_sel16 = (field_sel16 << RTK_DPA_INNER_IP_VER_OFFSET);
                    field_sel16_mask = (field_sel16_mask << RTK_DPA_INNER_IP_VER_OFFSET);
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_FIELD_SELECTOR9, (uint8 *)&field_sel16, (uint8 *)&field_sel16_mask);
                    break;
                }
                case EN_OFPXMT_RTK_OUTER_TCP_FLG:
                {
                    field_sel8 = (uint8)flow_entry_p->match_fields.field_data.outer_tcp_flag;
                    field_sel8_mask = (uint8)flow_entry_p->match_fields.field_data.outer_tcp_flag_mask;
                    field_sel8 = (field_sel8 & (uint8)RTK_DPA_OUTER_TCP_FLAG_MASK);
                    field_sel8_mask = (field_sel8_mask & (uint8)RTK_DPA_OUTER_TCP_FLAG_MASK);
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_TCP_FLAG, (uint8 *)&field_sel8, (uint8 *)&field_sel8_mask);
                    DBG_SDN("\n[%s][%d] EN_OFPXMT_RTK_OUTER_TCP_FLG, field_sel = 0x%08x, field_sel_mask = 0x%08x\n",__FUNCTION__,__LINE__,field_sel8,field_sel8_mask);
                    break;
                }
                case EN_OFPXMT_RTK_EXP_GTP_MT:
                {
                    field_sel16 = (uint16)flow_entry_p->match_fields.field_data.gtp_mt;
                    field_sel16_mask = (uint16)flow_entry_p->match_fields.field_data.gtp_mt_mask;
                    field_sel16 = (uint16)(field_sel16);
                    field_sel16_mask = (uint16)(RTK_DPA_GTP_MT_VALUE_MASK);
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_FIELD_SELECTOR8, (uint8 *)&field_sel16, (uint8 *)&field_sel16_mask);
                    break;
                }
                case EN_OFPXMT_RTK_OUTER_FRAG_PKT:
                {
                    field_sel8 = (uint8)flow_entry_p->match_fields.field_data.outer_frag_pkt;
                    field_sel8_mask = (uint8)flow_entry_p->match_fields.field_data.outer_frag_pkt_mask;
                    field_sel8 = (field_sel8 & (uint8)RTK_DPA_OUTER_TCP_FLAG_MASK);
                    field_sel8_mask = (field_sel8_mask & (uint8)RTK_DPA_OUTER_TCP_FLAG_MASK);
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_IP_FRAG, (uint8 *)&field_sel8, (uint8 *)&field_sel8_mask);
                    break;
                }
                case EN_OFPXMT_RTK_OVER_PKT_SIZE:
                {
                    field_sel16 = (uint16)flow_entry_p->match_fields.field_data.range_chk_index;
                    field_sel16_mask = (uint16)flow_entry_p->match_fields.field_data.range_chk_index_mask;
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_LEN_RANGE, (uint8 *)&field_sel16, (uint8 *)&field_sel16_mask);
                    break;
                }
                default:
                    return SDN_HAL_RETURN_FAILED;
            }
            fields_idx++;
        }
#ifndef CONFIG_FLOW_AG
#else
    }
#endif
    return SDN_HAL_RETURN_OK;
}

int32 _sdn_hal_flow_ft1_matchFieldValid_chosen(uint32 unit, sdn_hal_rule_flow_mgmt_t *flow_entry_p, rtk_of_l2FlowTblMatchField_t field, rtk_of_l2FlowEntryType_t *type)
{
    uint64 match_template_bitmap0 = 0;
    uint64 match_template_bitmap1 = 0;
    uint64 match_fields_bitmap = 0;

    match_fields_bitmap = flow_entry_p->match_fields.match_fields_bitmap;
    /*Metadata is an option match field.*/
    HAL_LIB_BIT_OFF(match_fields_bitmap, EN_OFPXMT_OFB_METADATA);

    switch(field)
    {
        case OF_L2_FT_MATCH_FIELD_VID_SA_SA:
        {
            /*(VID, SMAC)*/
            HAL_LIB_BIT_ON(match_template_bitmap0, EN_OFPXMT_OFB_ETH_SRC);
            HAL_LIB_BIT_ON(match_template_bitmap0, EN_OFPXMT_OFB_VLAN_VID);
            /*(0, SMAC)*/
            HAL_LIB_BIT_ON(match_template_bitmap1, EN_OFPXMT_OFB_ETH_SRC);
            if (match_fields_bitmap == match_template_bitmap0
                || match_fields_bitmap == match_template_bitmap1)
            {
                *type = (match_fields_bitmap == match_template_bitmap0) ? \
                (OF_L2_FLOW_ENTRY_TYPE_VID_SA) : (OF_L2_FLOW_ENTRY_TYPE_SA);
                return SDN_HAL_RETURN_OK;
            }
            break;
        }
        case OF_L2_FT_MATCH_FIELD_VID_SA_VID_DA:
        {
            /*(VID, SMAC)*/
            HAL_LIB_BIT_ON(match_template_bitmap0, EN_OFPXMT_OFB_ETH_SRC);
            HAL_LIB_BIT_ON(match_template_bitmap0, EN_OFPXMT_OFB_VLAN_VID);
            /*(VID, DMAC)*/
            HAL_LIB_BIT_ON(match_template_bitmap1, EN_OFPXMT_OFB_ETH_DST);
            HAL_LIB_BIT_ON(match_template_bitmap1, EN_OFPXMT_OFB_VLAN_VID);
            if (match_fields_bitmap == match_template_bitmap0
                || match_fields_bitmap == match_template_bitmap1)
            {
                *type = (match_fields_bitmap == match_template_bitmap0) ? \
                (OF_L2_FLOW_ENTRY_TYPE_VID_SA) : (OF_L2_FLOW_ENTRY_TYPE_VID_DA);
                return SDN_HAL_RETURN_OK;
            }
            break;
        }
        case OF_L2_FT_MATCH_FIELD_VID_SA_DA:
        {
            /*(VID, SMAC)*/
            HAL_LIB_BIT_ON(match_template_bitmap0, EN_OFPXMT_OFB_ETH_SRC);
            HAL_LIB_BIT_ON(match_template_bitmap0, EN_OFPXMT_OFB_VLAN_VID);
            /*(0, DMAC)*/
            HAL_LIB_BIT_ON(match_template_bitmap1, EN_OFPXMT_OFB_ETH_DST);
            if (match_fields_bitmap == match_template_bitmap0
                || match_fields_bitmap == match_template_bitmap1)
            {
                *type = (match_fields_bitmap == match_template_bitmap0) ? \
                (OF_L2_FLOW_ENTRY_TYPE_VID_SA) : (OF_L2_FLOW_ENTRY_TYPE_DA);
                return SDN_HAL_RETURN_OK;
            }
            break;
        }
        case OF_L2_FT_MATCH_FIELD_SA_VID_DA:
        {
            /*(0, SMAC)*/
            HAL_LIB_BIT_ON(match_template_bitmap0, EN_OFPXMT_OFB_ETH_SRC);

            /*(VID, DMAC)*/
            HAL_LIB_BIT_ON(match_template_bitmap1, EN_OFPXMT_OFB_VLAN_VID);
            HAL_LIB_BIT_ON(match_template_bitmap1, EN_OFPXMT_OFB_ETH_DST);
            if (match_fields_bitmap == match_template_bitmap0
                || match_fields_bitmap == match_template_bitmap1)
            {
                *type = (match_fields_bitmap == match_template_bitmap0) ? \
                (OF_L2_FLOW_ENTRY_TYPE_SA) : (OF_L2_FLOW_ENTRY_TYPE_VID_DA);
                return SDN_HAL_RETURN_OK;
            }
            break;
        }
        case OF_L2_FT_MATCH_FIELD_SA_DA:
        {
            /*(0, SMAC)*/
            HAL_LIB_BIT_ON(match_template_bitmap0, EN_OFPXMT_OFB_ETH_SRC);
            /*(0, DMAC)*/
            HAL_LIB_BIT_ON(match_template_bitmap1, EN_OFPXMT_OFB_ETH_DST);
            if (match_fields_bitmap == match_template_bitmap0
                || match_fields_bitmap == match_template_bitmap1)
            {
                *type = (match_fields_bitmap == match_template_bitmap0) ? \
                (OF_L2_FLOW_ENTRY_TYPE_SA) : (OF_L2_FLOW_ENTRY_TYPE_DA);
                return SDN_HAL_RETURN_OK;
            }
            break;
        }
        case OF_L2_FT_MATCH_FIELD_VID_DA_DA:
        {
            /*(VID, DMAC)*/
            HAL_LIB_BIT_ON(match_template_bitmap0, EN_OFPXMT_OFB_VLAN_VID);
            HAL_LIB_BIT_ON(match_template_bitmap0, EN_OFPXMT_OFB_ETH_DST);
            /*(0, DMAC)*/
            HAL_LIB_BIT_ON(match_template_bitmap1, EN_OFPXMT_OFB_ETH_DST);
            if (match_fields_bitmap == match_template_bitmap0
                || match_fields_bitmap == match_template_bitmap1)
            {
                *type = (match_fields_bitmap == match_template_bitmap0) ? \
                (OF_L2_FLOW_ENTRY_TYPE_VID_DA) : (OF_L2_FLOW_ENTRY_TYPE_DA);
                return SDN_HAL_RETURN_OK;
            }
            break;
        }
        case OF_L2_FT_MATCH_FIELD_DA_SA_VID_SA:
        {
            /*(DMAC, SMAC)*/
            HAL_LIB_BIT_ON(match_template_bitmap0, EN_OFPXMT_OFB_ETH_SRC);
            HAL_LIB_BIT_ON(match_template_bitmap0, EN_OFPXMT_OFB_ETH_DST);
            /*(VID, SMAC)*/
            HAL_LIB_BIT_ON(match_template_bitmap1, EN_OFPXMT_OFB_ETH_SRC);
            HAL_LIB_BIT_ON(match_template_bitmap1, EN_OFPXMT_OFB_VLAN_VID);
            if (match_fields_bitmap == match_template_bitmap0
                || match_fields_bitmap == match_template_bitmap1)
            {
                *type = (match_fields_bitmap == match_template_bitmap0) ? \
                (OF_L2_FLOW_ENTRY_TYPE_SA_DA) : (OF_L2_FLOW_ENTRY_TYPE_VID_SA);
                return SDN_HAL_RETURN_OK;
            }
            break;
        }
        case OF_L2_FT_MATCH_FIELD_DA_SA_SA:
        {
            /*(DMAC, SMAC)*/
            HAL_LIB_BIT_ON(match_template_bitmap0, EN_OFPXMT_OFB_ETH_SRC);
            HAL_LIB_BIT_ON(match_template_bitmap0, EN_OFPXMT_OFB_ETH_DST);
            /*(0, SMAC)*/
            HAL_LIB_BIT_ON(match_template_bitmap1, EN_OFPXMT_OFB_ETH_SRC);
            if (match_fields_bitmap == match_template_bitmap0
                || match_fields_bitmap == match_template_bitmap1)
            {
                *type = (match_fields_bitmap == match_template_bitmap0) ? \
                (OF_L2_FLOW_ENTRY_TYPE_SA_DA) : (OF_L2_FLOW_ENTRY_TYPE_SA);
                return SDN_HAL_RETURN_OK;
            }
            break;
        }
        case OF_L2_FT_MATCH_FIELD_DA_SA_VID_DA:
        {
            /*(DMAC, SMAC)*/
            HAL_LIB_BIT_ON(match_template_bitmap0, EN_OFPXMT_OFB_ETH_SRC);
            HAL_LIB_BIT_ON(match_template_bitmap0, EN_OFPXMT_OFB_ETH_DST);

            /*(VID, DMAC)*/
            HAL_LIB_BIT_ON(match_template_bitmap1, EN_OFPXMT_OFB_ETH_DST);
            HAL_LIB_BIT_ON(match_template_bitmap1, EN_OFPXMT_OFB_VLAN_VID);
            if (match_fields_bitmap == match_template_bitmap0
                || match_fields_bitmap == match_template_bitmap1)
            {
                *type = (match_fields_bitmap == match_template_bitmap0) ? \
                (OF_L2_FLOW_ENTRY_TYPE_SA_DA) : (OF_L2_FLOW_ENTRY_TYPE_VID_DA);
                return SDN_HAL_RETURN_OK;
            }
            break;
        }
        case OF_L2_FT_MATCH_FIELD_DA_SA_DA:
        {
            /*(DMAC, SMAC)*/
            HAL_LIB_BIT_ON(match_template_bitmap0, EN_OFPXMT_OFB_ETH_SRC);
            HAL_LIB_BIT_ON(match_template_bitmap0, EN_OFPXMT_OFB_ETH_DST);

            /*(0, DMAC)*/
            HAL_LIB_BIT_ON(match_template_bitmap1, EN_OFPXMT_OFB_ETH_DST);
            if (match_fields_bitmap == match_template_bitmap0
                || match_fields_bitmap == match_template_bitmap1)
            {
                *type = (match_fields_bitmap == match_template_bitmap0) ? \
                (OF_L2_FLOW_ENTRY_TYPE_SA_DA) : (OF_L2_FLOW_ENTRY_TYPE_DA);
                return SDN_HAL_RETURN_OK;
            }
            break;
        }
        case OF_L2_FT_MATCH_FIELD_FIVE_TUPLE:
        {
            /*(IP PROTOCOL,L4_DPORT,L4_SPORT,DIP,SIP)*/
            if ((HAL_LIB_CHK_BIT_ON(match_fields_bitmap, EN_OFPXMT_OFB_TCP_SRC)
                && HAL_LIB_CHK_BIT_ON(match_fields_bitmap, EN_OFPXMT_OFB_TCP_DST))
                || (HAL_LIB_CHK_BIT_ON(match_fields_bitmap, EN_OFPXMT_OFB_UDP_SRC)
                && HAL_LIB_CHK_BIT_ON(match_fields_bitmap, EN_OFPXMT_OFB_UDP_DST))
                || (HAL_LIB_CHK_BIT_ON(match_fields_bitmap, EN_OFPXMT_OFB_SCTP_SRC)
                && HAL_LIB_CHK_BIT_ON(match_fields_bitmap, EN_OFPXMT_OFB_UDP_DST)))
            {
                HAL_LIB_BIT_OFF(match_fields_bitmap, EN_OFPXMT_OFB_TCP_SRC);
                HAL_LIB_BIT_OFF(match_fields_bitmap, EN_OFPXMT_OFB_TCP_DST);
                HAL_LIB_BIT_OFF(match_fields_bitmap, EN_OFPXMT_OFB_UDP_SRC);
                HAL_LIB_BIT_OFF(match_fields_bitmap, EN_OFPXMT_OFB_UDP_DST);
                HAL_LIB_BIT_OFF(match_fields_bitmap, EN_OFPXMT_OFB_SCTP_SRC);
                HAL_LIB_BIT_OFF(match_fields_bitmap, EN_OFPXMT_OFB_SCTP_DST);
            }
            HAL_LIB_BIT_ON(match_template_bitmap0, EN_OFPXMT_OFB_ETH_TYPE);
            HAL_LIB_BIT_ON(match_template_bitmap0, EN_OFPXMT_OFB_IP_PROTO);
            HAL_LIB_BIT_ON(match_template_bitmap0, EN_OFPXMT_OFB_IPV4_SRC);
            HAL_LIB_BIT_ON(match_template_bitmap0, EN_OFPXMT_OFB_IPV4_DST);

            /*(VID,L4_DPORT,L4_SPORT,DIP,SIP)*/
            HAL_LIB_BIT_ON(match_template_bitmap1, EN_OFPXMT_OFB_ETH_TYPE);
            HAL_LIB_BIT_ON(match_template_bitmap1, EN_OFPXMT_OFB_VLAN_VID);
            HAL_LIB_BIT_ON(match_template_bitmap1, EN_OFPXMT_OFB_IPV4_SRC);
            HAL_LIB_BIT_ON(match_template_bitmap1, EN_OFPXMT_OFB_IPV4_DST);

            if (match_fields_bitmap == match_template_bitmap0
                || match_fields_bitmap == match_template_bitmap1)
            {
                *type = (match_fields_bitmap == match_template_bitmap0) ? \
                (OF_L2_FLOW_ENTRY_TYPE_5_TUPLE_IPPROTO) : (OF_L2_FLOW_ENTRY_TYPE_5_TUPLE_VID);
                return SDN_HAL_RETURN_OK;
            }
            break;
        }
        default:
            return SDN_HAL_RETURN_FAILED;
    }

    return SDN_HAL_RETURN_FAILED;
}

static int32 _sdn_hal_flow_ft1_matchField_fill(sdn_hal_rule_flow_mgmt_t *flow_entry_p, sdn_hal_flow_t *flow)
{
    uint32 fields_idx = 0;
    uint32 unit_id    = 0;
    rtk_of_l2FlowTblMatchField_t field;
    rtk_of_l2FlowEntryType_t type;

    unit_id = SDN_HAL_MY_UNIT_ID();
    fields_idx = 0;

    if(RT_ERR_OK != rtk_of_l2FlowTblMatchField_get(unit_id, &field))
        return SDN_HAL_RETURN_FAILED;

    if(SDN_HAL_RETURN_OK != _sdn_hal_flow_ft1_matchFieldValid_chosen(unit_id, flow_entry_p, field, &type))
        return SDN_HAL_RETURN_FAILED;

    flow->ft1_flow.type = type;
    while (fields_idx < EN_OFPXMT_RTK_EXP_MAX) /* For DPA */
//    while (fields_idx < EN_OFPXMT_OFB_MAX)
    {
        if (!HAL_LIB_CHK_BIT_ON(flow_entry_p->match_fields.match_fields_bitmap, fields_idx))
        {
           fields_idx++;
           continue;
        }
        DBG_SDN("\r\n[%s-%d]set unit(%d), phase(%d), entry(%d), field(%d)\r\n", __FUNCTION__, __LINE__,
                                                                    (uint32)unit_id,
                                                                    (uint32)table_phase,
                                                                    flow_entry_p->flow_id, fields_idx);
        switch((EN_OFP_OXM_OFB_MATCH_FIELDS_T)fields_idx)
        {
            case EN_OFPXMT_OFB_ETH_DST:
            {
                if(macIsAllOnes_ret(flow_entry_p->match_fields.field_data.dst_mac_mask))
                {
                    DBG_SDN("\r\n[%s-%d]dst mac:%02x:%02x:%02x:%02x:%02x:%02x \r\n", __FUNCTION__, __LINE__,
                            flow_entry_p->match_fields.field_data.dst_mac[0],
                            flow_entry_p->match_fields.field_data.dst_mac[1],
                            flow_entry_p->match_fields.field_data.dst_mac[2],
                            flow_entry_p->match_fields.field_data.dst_mac[3],
                            flow_entry_p->match_fields.field_data.dst_mac[4],
                            flow_entry_p->match_fields.field_data.dst_mac[5]);
                    memcpy(&flow->ft1_flow.matchfield.vidMac.dmac.octet, &flow_entry_p->match_fields.field_data.dst_mac, sizeof(rtk_mac_t));
                }
                else
                    return SDN_HAL_RETURN_UNSUPPORTED_MASK;
                break;
            }
            case EN_OFPXMT_OFB_ETH_SRC:
            {
                if(macIsAllOnes_ret(flow_entry_p->match_fields.field_data.src_mac_mask))
                {
                    DBG_SDN("\r\n[%s-%d]src mac:%02x:%02x:%02x:%02x:%02x:%02x \r\n",  __FUNCTION__, __LINE__,
                            flow_entry_p->match_fields.field_data.src_mac[0],
                            flow_entry_p->match_fields.field_data.src_mac[1],
                            flow_entry_p->match_fields.field_data.src_mac[2],
                            flow_entry_p->match_fields.field_data.src_mac[3],
                            flow_entry_p->match_fields.field_data.src_mac[4],
                            flow_entry_p->match_fields.field_data.src_mac[5]);
                    memcpy(&flow->ft1_flow.matchfield.vidMac.smac.octet, &flow_entry_p->match_fields.field_data.src_mac, sizeof(rtk_mac_t));
                }
                else
                {
                    return SDN_HAL_RETURN_UNSUPPORTED_MASK;
                }
                break;
            }
            case EN_OFPXMT_OFB_VLAN_VID:
            {
                DBG_SDN("\r\n[%s-%d]  svid=[%d]\r\n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.svid);

                if (field == OF_L2_FT_MATCH_FIELD_FIVE_TUPLE)
                    flow->ft1_flow.matchfield.fiveTp.fifthData.vid = flow_entry_p->match_fields.field_data.svid;
                else
                    flow->ft1_flow.matchfield.vidMac.vid = flow_entry_p->match_fields.field_data.svid;

                break;
            }
            case EN_OFPXMT_OFB_IP_PROTO:
            {
                flow->ft1_flow.matchfield.fiveTp.fifthData.ipproto = flow_entry_p->match_fields.field_data.protocol_val;
                break;
            }
            case EN_OFPXMT_OFB_IPV4_SRC:
            {
#ifdef CONFIG_SDN_HAL_DBG
                uint8 *ipaddr_p =  (uint8*)&flow_entry_p->match_fields.field_data.src_ipv4_addr;
                DBG_SDN("src ip:%d.%d.%d.%d \r\n", ipaddr_p[0],
                                                   ipaddr_p[1],
                                                   ipaddr_p[2],
                                                   ipaddr_p[3]);
#endif
                flow->ft1_flow.matchfield.fiveTp.sip = flow_entry_p->match_fields.field_data.src_ipv4_addr;
                break;
            }
            case EN_OFPXMT_OFB_IPV4_DST:
            {
#ifdef CONFIG_SDN_HAL_DBG
                uint8 *ipaddr_p =  (uint8*)&flow_entry_p->match_fields.field_data.src_ipv4_addr;
                DBG_SDN("dst ip:%d.%d.%d.%d \r\n", ipaddr_p[0],
                                                ipaddr_p[1],
                                                ipaddr_p[2],
                                                ipaddr_p[3]);
#endif
                flow->ft1_flow.matchfield.fiveTp.dip = flow_entry_p->match_fields.field_data.dst_ipv4_addr;
                break;
            }
            case EN_OFPXMT_OFB_SCTP_SRC:
            case EN_OFPXMT_OFB_TCP_SRC:
            case EN_OFPXMT_OFB_UDP_SRC:
            {
                if (fields_idx == EN_OFPXMT_OFB_TCP_SRC)
                {
                    flow->ft1_flow.matchfield.fiveTp.l4_sport = flow_entry_p->match_fields.field_data.tcp_src_port_no;
                }
                else if(fields_idx == EN_OFPXMT_OFB_UDP_SRC)
                {
                    flow->ft1_flow.matchfield.fiveTp.l4_sport = flow_entry_p->match_fields.field_data.udp_src_port_no;
                }
                else
                {
                    flow->ft1_flow.matchfield.fiveTp.l4_sport = flow_entry_p->match_fields.field_data.sctp_src_port_no;
                }
                break;
            }
            case EN_OFPXMT_OFB_SCTP_DST:
            case EN_OFPXMT_OFB_TCP_DST:
            case EN_OFPXMT_OFB_UDP_DST:
            {
                if (fields_idx == EN_OFPXMT_OFB_TCP_DST)
                {
                    flow->ft1_flow.matchfield.fiveTp.l4_dport = flow_entry_p->match_fields.field_data.tcp_dst_port_no;
                }
                else if(fields_idx == EN_OFPXMT_OFB_UDP_DST)
                {
                    flow->ft1_flow.matchfield.fiveTp.l4_dport = flow_entry_p->match_fields.field_data.udp_dst_port_no;
                }
                else
                {
                    flow->ft1_flow.matchfield.fiveTp.l4_dport = flow_entry_p->match_fields.field_data.sctp_dst_port_no;
                }

                break;
            }
            case EN_OFPXMT_OFB_ETH_TYPE:
            {
                break;
            }
            case EN_OFPXMT_OFB_METADATA:
            {
                if(flow_entry_p->match_fields.field_data.metadata != flow_entry_p->match_fields.field_data.metadata_mask)
                    return SDN_HAL_RETURN_FAILED;
                flow->ft1_flow.matchfield.vidMac.metadata = flow_entry_p->match_fields.field_data.metadata;
                flow->ft1_flow.flags = RTK_OF_FLAG_FLOWENTRY_MD_CMP;
                break;
            }
            default:
                return SDN_HAL_RETURN_FAILED;
        }
        fields_idx++;
    }

    return SDN_HAL_RETURN_OK;

}
int32 _sdn_hal_flow_ft2_matchFieldValid_chosen(uint32 unit,
    sdn_hal_rule_flow_mgmt_t *flow_entry_p,
    sdn_hal_flow_t *flow,
    rtk_of_l3FlowEntryType_t *type)
{
    rtk_of_l3CamFlowTblMatchField_t  cam_field;
    rtk_of_l3HashFlowTblMatchField_t hash_field;
    rtk_of_l3FlowEntryType_t select_type;
    uint64 match_fields_bitmap = 0;
    match_fields_bitmap = flow_entry_p->match_fields.match_fields_bitmap;
    /*Select l3 flow entry type.*/
    if((HAL_LIB_CHK_BIT_ON(match_fields_bitmap, EN_OFPXMT_OFB_IPV6_SRC)
        || HAL_LIB_CHK_BIT_ON(match_fields_bitmap, EN_OFPXMT_OFB_IPV4_SRC))
        && (!HAL_LIB_CHK_BIT_ON(match_fields_bitmap, EN_OFPXMT_OFB_IPV4_DST)
        && !HAL_LIB_CHK_BIT_ON(match_fields_bitmap, EN_OFPXMT_OFB_IPV6_DST)))
    {
        if(HAL_LIB_CHK_BIT_ON(match_fields_bitmap, EN_OFPXMT_OFB_IPV6_SRC))
            select_type = OF_L3_FLOW_ENTRY_TYPE_IP6_SIP;
        else
            select_type = OF_L3_FLOW_ENTRY_TYPE_IP4_SIP;
    }
    else if((!HAL_LIB_CHK_BIT_ON(match_fields_bitmap, EN_OFPXMT_OFB_IPV6_SRC)
        && !HAL_LIB_CHK_BIT_ON(match_fields_bitmap, EN_OFPXMT_OFB_IPV4_SRC))
        && (HAL_LIB_CHK_BIT_ON(match_fields_bitmap, EN_OFPXMT_OFB_IPV4_DST)
        || HAL_LIB_CHK_BIT_ON(match_fields_bitmap, EN_OFPXMT_OFB_IPV6_DST)))
    {
        if(HAL_LIB_CHK_BIT_ON(match_fields_bitmap, EN_OFPXMT_OFB_IPV6_DST))
            select_type = OF_L3_FLOW_ENTRY_TYPE_IP6_DIP;
        else
            select_type = OF_L3_FLOW_ENTRY_TYPE_IP4_DIP;
    }
    else
    {
        if(HAL_LIB_CHK_BIT_ON(match_fields_bitmap, EN_OFPXMT_OFB_IPV6_SRC)
            || HAL_LIB_CHK_BIT_ON(match_fields_bitmap, EN_OFPXMT_OFB_IPV6_DST))
            select_type = OF_L3_FLOW_ENTRY_TYPE_IP6_SIP_DIP;
        else
            select_type = OF_L3_FLOW_ENTRY_TYPE_IP4_SIP_DIP;
    }
    //DBG_SDN("%s : line%d, !ipv6IsAllOnes_ret(dip_v6_mask_p) = %d, !ipv6IsAllOnes_ret(sip_v6_mask_p) = %d, !ipv4IsAllOnes_ret(dst_ipv4_mask)%d, !ipv4IsAllOnes_ret(src_ipv4_mask)%d\n", __FUNCTION__, __LINE__, !ipv6IsAllOnes_ret(dip_v6_mask_p), !ipv6IsAllOnes_ret(sip_v6_mask_p), !ipv4IsAllOnes_ret(flow_entry_p->match_fields.field_data.dst_ipv4_mask), !ipv4IsAllOnes_ret(flow_entry_p->match_fields.field_data.src_ipv4_mask));

    /*check wheather type is valid.*/
    if(_sdn_hal_l3Cam_is_used(flow_entry_p))
    {
        if(RT_ERR_OK != rtk_of_l3CamFlowTblMatchField_get(unit, &cam_field))
            return SDN_HAL_RETURN_FAILED;

        switch (cam_field)
        {
            case OF_L3_CAM_FT_MATCH_FIELD_MD_SIP_MD_DIP:
            {
                if(select_type == OF_L3_FLOW_ENTRY_TYPE_IP6_SIP_DIP
                    || select_type == OF_L3_FLOW_ENTRY_TYPE_IP4_SIP_DIP)
                    return SDN_HAL_RETURN_FAILED;
                break;
            }
            case OF_L3_CAM_FT_MATCH_FIELD_MD_SIP_DIP_MD_SIP:
            case OF_L3_CAM_FT_MATCH_FIELD_MD_SIP_DIP_MD_DIP:
            {
                /*Don't need to check.*/
                break;
            }
            default:
                return SDN_HAL_RETURN_FAILED;
        }
        flow->ft2_flow.ft2_cam_flow.type = select_type;
    }
    else
    {
        if(RT_ERR_OK != rtk_of_l3HashFlowTblMatchField_get(unit, &hash_field))
            return SDN_HAL_RETURN_FAILED;

        switch (hash_field)
        {
            case OF_L3_HASH_FT_MATCH_FIELD_SIP_DIP:
            {
                if(select_type == OF_L3_FLOW_ENTRY_TYPE_IP6_SIP_DIP
                    || select_type == OF_L3_FLOW_ENTRY_TYPE_IP4_SIP_DIP)
                    return SDN_HAL_RETURN_FAILED;
                break;
            }
            case OF_L3_HASH_FT_MATCH_FIELD_SIP_DIP_SIP:
            {
                if(select_type == OF_L3_FLOW_ENTRY_TYPE_IP4_DIP
                    || select_type == OF_L3_FLOW_ENTRY_TYPE_IP6_DIP)
                    return SDN_HAL_RETURN_FAILED;
                break;
            }
            case OF_L3_HASH_FT_MATCH_FIELD_SIP_DIP_DIP:
            {
                if(select_type == OF_L3_FLOW_ENTRY_TYPE_IP4_SIP
                    || select_type == OF_L3_FLOW_ENTRY_TYPE_IP6_SIP)
                    return SDN_HAL_RETURN_FAILED;
                break;
            }
            default:
                return SDN_HAL_RETURN_FAILED;
        }
        flow->ft2_flow.ft2_hash_flow.type = select_type;
    }

    *type = select_type;

    return SDN_HAL_RETURN_OK;
}

static int32 _sdn_hal_flow_ft2_matchField_fill(sdn_hal_rule_flow_mgmt_t *flow_entry_p, sdn_hal_flow_t *flow)
{
    uint32 fields_idx = 0;
    uint32 unit_id    = 0;
    rtk_of_l3FlowEntryType_t type;

    unit_id = SDN_HAL_MY_UNIT_ID();
    fields_idx = 0;

    if(SDN_HAL_RETURN_OK != _sdn_hal_flow_ft2_matchFieldValid_chosen(unit_id, flow_entry_p, flow, &type))
        return SDN_HAL_RETURN_FAILED;

    fields_idx = 0;

    while (fields_idx < EN_OFPXMT_RTK_EXP_MAX) /* For MEC DPA */
//    while (fields_idx < EN_OFPXMT_OFB_MAX)
    {
        if (!HAL_LIB_CHK_BIT_ON(flow_entry_p->match_fields.match_fields_bitmap, fields_idx))
        {
           fields_idx++;
           continue;
        }
        DBG_SDN("\r\n[%s-%d]set unit(%d), phase(%d), entry(%d), field(%d)\r\n", __FUNCTION__, __LINE__,
                                                                    (uint32)unit_id,
                                                                    (uint32)table_phase,
                                                                    flow_entry_p->flow_id, fields_idx);
        switch((EN_OFP_OXM_OFB_MATCH_FIELDS_T)fields_idx)
        {
            case EN_OFPXMT_OFB_IPV4_SRC:
            {
#ifdef CONFIG_SDN_HAL_DBG
                uint8 *ipaddr_p =  (uint8*)&flow_entry_p->match_fields.field_data.src_ipv4_addr;
                DBG_SDN("src ip:%d.%d.%d.%d \r\n", ipaddr_p[0],
                                                   ipaddr_p[1],
                                                   ipaddr_p[2],
                                                   ipaddr_p[3]);
#endif
                if(_sdn_hal_l3Cam_is_used(flow_entry_p))
                {
                    flow->ft2_flow.ft2_cam_flow.sip.ipv4 = flow_entry_p->match_fields.field_data.src_ipv4_addr;
                    flow->ft2_flow.ft2_cam_flow.sip_msk.ipv4 = flow_entry_p->match_fields.field_data.src_ipv4_mask;
                }
                else
                {
                    flow->ft2_flow.ft2_hash_flow.sip.ipv4 = flow_entry_p->match_fields.field_data.src_ipv4_addr;
                }
                break;
            }
            case EN_OFPXMT_OFB_IPV4_DST:
            {
#ifdef CONFIG_SDN_HAL_DBG
                 uint8 *ipaddr_p =  (uint8*)&flow_entry_p->match_fields.field_data.dst_ipv4_addr;
                 DBG_SDN("dst ip:%d.%d.%d.%d \r\n", ipaddr_p[0],
                                                    ipaddr_p[1],
                                                    ipaddr_p[2],
                                                    ipaddr_p[3]);
#endif
                if(_sdn_hal_l3Cam_is_used(flow_entry_p))
                {
                    flow->ft2_flow.ft2_cam_flow.dip.ipv4 = flow_entry_p->match_fields.field_data.dst_ipv4_addr;
                    flow->ft2_flow.ft2_cam_flow.dip_msk.ipv4 = flow_entry_p->match_fields.field_data.dst_ipv4_mask;
                }
                else
                {
                    flow->ft2_flow.ft2_hash_flow.dip.ipv4 = flow_entry_p->match_fields.field_data.dst_ipv4_addr;
                }

                break;
            }
            case EN_OFPXMT_OFB_IPV6_SRC:
            {
                if(_sdn_hal_l3Cam_is_used(flow_entry_p))
                {
                    memcpy(&flow->ft2_flow.ft2_cam_flow.sip.ipv6,
                        &flow_entry_p->match_fields.field_data.ipv6_src_addr,
                        sizeof(rtk_ipv6_addr_t));
                    memcpy(&flow->ft2_flow.ft2_cam_flow.sip_msk.ipv6,
                        &flow_entry_p->match_fields.field_data.ipv6_src_mask,
                        sizeof(rtk_ipv6_addr_t));
                }
                else
                {
                    memcpy(&flow->ft2_flow.ft2_hash_flow.sip.ipv6,
                        &flow_entry_p->match_fields.field_data.ipv6_src_addr,
                        sizeof(rtk_ipv6_addr_t));
                }
                break;
            }
            case EN_OFPXMT_OFB_IPV6_DST:
            {
                if(_sdn_hal_l3Cam_is_used(flow_entry_p))
                {
                    memcpy(&flow->ft2_flow.ft2_cam_flow.dip.ipv6,
                        &flow_entry_p->match_fields.field_data.ipv6_dst_addr,
                        sizeof(rtk_ipv6_addr_t));
                    memcpy(&flow->ft2_flow.ft2_cam_flow.dip_msk.ipv6,
                        &flow_entry_p->match_fields.field_data.ipv6_dst_mask,
                        sizeof(rtk_ipv6_addr_t));
                }
                else
                {
                    memcpy(&flow->ft2_flow.ft2_hash_flow.dip.ipv6,
                        &flow_entry_p->match_fields.field_data.ipv6_dst_addr,
                        sizeof(rtk_ipv6_addr_t));
                }

                break;
            }
            case EN_OFPXMT_OFB_ETH_TYPE:
            {
                break;
            }
            case EN_OFPXMT_OFB_METADATA:
            {
                if(!_sdn_hal_l3Cam_is_used(flow_entry_p))
                {
                    flow->ft2_flow.ft2_hash_flow.metadata = flow_entry_p->match_fields.field_data.metadata;
                    flow->ft2_flow.ft2_hash_flow.flags = RTK_OF_FLAG_FLOWENTRY_MD_CMP;

                }
                else
                {
                    flow->ft2_flow.ft2_cam_flow.metadata = flow_entry_p->match_fields.field_data.metadata;
                    flow->ft2_flow.ft2_cam_flow.metadata_msk = flow_entry_p->match_fields.field_data.metadata_mask;
                    flow->ft2_flow.ft2_cam_flow.flags = RTK_OF_FLAG_FLOWENTRY_MD_CMP;
                }
                break;
            }
            default:
                return SDN_HAL_RETURN_FAILED;
        }
        fields_idx++;
    }

    return SDN_HAL_RETURN_OK;

}
static int32 _sdn_hal_flow_ft3_matchField_fill(sdn_hal_rule_flow_mgmt_t *flow_entry_p, sdn_hal_flow_t *flow)
{
    uint32 fields_idx           = 0;
    uint32 unit_id              = 0;
    uint32 get_port             = 0xff;
    uint8  phy_in_port          = 0;
    uint16 mask16               = 0xffff;
    uint16 port_no              = 0;
    uint16 metadata             = 0;
    uint8 template_mask         = 0xf;
    uint8 template_id           = 0;
    rtk_of_flow_id_t flow_id    = 0;
    rtk_of_flowtable_phase_t table_phase;

    uint32 field_sel            = 0;
    uint32 field_sel_mask       = 0xffff;
    uint16 field_sel16_hi       = 0;
    uint16 field_sel16_hi_mask  = 0xffff;
    uint16 field_sel16_lo       = 0;
    uint16 field_sel16_lo_mask  = 0xffff;

    uint16 field_sel16          = 0;
    uint16 field_sel16_mask     = 0xffff;
    uint16 field_sel8           = 0;
    uint16 field_sel8_mask      = 0xff;

    int     ret = 0;

    unit_id = SDN_HAL_MY_UNIT_ID();

    is_inport_existed = FALSE;
#ifndef CONFIG_FLOW_AG
    flow_id = flow_entry_p->flow_id;
#else
    int i;
    for(i = 0; i < SDN_HAL_MAX_NUM_BLOCK_TEMPLATE; i++)
    {
        flow_id = flow_entry_p->flow_id + i;
        flow_entry_p->match_fields.match_fields_bitmap = flow_entry_p->match_fields.match_fields_bitmap_agg[i];
#endif
        if(SDN_HAL_RETURN_OK != sdn_hal_templateId_chosen(flow_entry_p->table_id,
                                         &template_id,
                                         flow_entry_p->match_fields.match_fields_bitmap))
        {
            DBG_SDN("\n[%s][%d]\n",__FUNCTION__,__LINE__);
            return SDN_HAL_RETURN_FAILED;
        }

        table_phase = sdn_hal_rule_get_flow_table(flow_entry_p->table_id);

        rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_TEMPLATE_ID,
            &template_id,
            &template_mask);

        fields_idx = 0;
        while (fields_idx < EN_OFPXMT_RTK_EXP_MAX) /* For DPA */
//        while (fields_idx < EN_OFPXMT_OFB_MAX)
        {
            if (!HAL_LIB_CHK_BIT_ON(flow_entry_p->match_fields.match_fields_bitmap, fields_idx))
            {
               fields_idx++;
               continue;
            }
            DBG_SDN("\r\n[%s-%d]set unit(%d), phase(%d), entry(%d), field(%d)\r\n", __FUNCTION__, __LINE__,
                                                                        (uint32)unit_id,
                                                                        (uint32)table_phase,
                                                                        flow_entry_p->flow_id, fields_idx);
            mask16 = 0xffff;
            switch((EN_OFP_OXM_OFB_MATCH_FIELDS_T)fields_idx)
            {
                case EN_OFPXMT_OFB_IN_PORT:
                {
                    sdn_hal_logicalPort2PhyPort(flow_entry_p->match_fields.field_data.l_port, &get_port);
                    phy_in_port = get_port;
                    is_inport_existed = TRUE;
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                     MATCH_FIELD_IN_PHY_PORT, (uint8 *)&phy_in_port, (uint8 *)&mask16);
                    break;
                }
                case EN_OFPXMT_OFB_IN_PHY_PORT:
                {
                    phy_in_port = flow_entry_p->match_fields.field_data.l_port;
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                     MATCH_FIELD_IN_PHY_PORT, (uint8 *)&phy_in_port, (uint8 *)&mask16);
                    break;
                }
                case EN_OFPXMT_OFB_METADATA:
                {
                    metadata = flow_entry_p->match_fields.field_data.metadata;
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                     MATCH_FIELD_METADATA, (uint8 *)&metadata, (uint8 *)&mask16);
                    break;
                }
                case EN_OFPXMT_OFB_ARP_THA:
                {
                    DBG_SDN("\r\n[%s-%d]dst mac:%02x:%02x:%02x:%02x:%02x:%02x \r\n", __FUNCTION__, __LINE__,
                            flow_entry_p->match_fields.field_data.dst_mac[0],
                            flow_entry_p->match_fields.field_data.dst_mac[1],
                            flow_entry_p->match_fields.field_data.dst_mac[2],
                            flow_entry_p->match_fields.field_data.dst_mac[3],
                            flow_entry_p->match_fields.field_data.dst_mac[4],
                            flow_entry_p->match_fields.field_data.dst_mac[5]);
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                     MATCH_FIELD_ARP_THA, flow_entry_p->match_fields.field_data.dst_mac, flow_entry_p->match_fields.field_data.dst_mac_mask);
                    break;
                }
                case EN_OFPXMT_OFB_ETH_DST:
                {
                    DBG_SDN("\r\n[%s-%d]dst mac:%02x:%02x:%02x:%02x:%02x:%02x \r\n", __FUNCTION__, __LINE__,
                            flow_entry_p->match_fields.field_data.dst_mac[0],
                            flow_entry_p->match_fields.field_data.dst_mac[1],
                            flow_entry_p->match_fields.field_data.dst_mac[2],
                            flow_entry_p->match_fields.field_data.dst_mac[3],
                            flow_entry_p->match_fields.field_data.dst_mac[4],
                            flow_entry_p->match_fields.field_data.dst_mac[5]);
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                     MATCH_FIELD_ETH_DST, flow_entry_p->match_fields.field_data.dst_mac, flow_entry_p->match_fields.field_data.dst_mac_mask);
                    break;
                }
                case EN_OFPXMT_OFB_ARP_SHA:
                {
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

                            rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                    MATCH_FIELD_ARP_SHA, flow_entry_p->match_fields.field_data.src_mac, flow_entry_p->match_fields.field_data.src_mac_mask);
                    break;
                }
                case EN_OFPXMT_OFB_ETH_SRC:
                {
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

                            rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                    MATCH_FIELD_ETH_SRC, flow_entry_p->match_fields.field_data.src_mac, flow_entry_p->match_fields.field_data.src_mac_mask);
                    break;
                }
                case EN_OFPXMT_OFB_ETH_TYPE:
                {
                     uint16 eth_type = flow_entry_p->match_fields.field_data.eth_type_val;
                     DBG_SDN("\r\n[%s-%d] eth_type:%02x\r\n", __FUNCTION__, __LINE__, eth_type);
                     rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                     MATCH_FIELD_ETH_TYPE, (uint8 *)&eth_type, (uint8 *)&mask16);
                     break;
                }
                case EN_OFPXMT_OFB_VLAN_VID:
                {
                     DBG_SDN("\r\n[%s-%d]Make sure that  is_default_template=[%s]  \r\n",__FUNCTION__, __LINE__,
                                          (is_default_template) ? "TRUE":"FALSE");

                     DBG_SDN("\r\n[%s-%d]  svid=[%d]\r\n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.svid);
                     DBG_SDN("\r\n[%s-%d]  cvid=[%d]\r\n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.cvid);

                    if(flow_entry_p->match_fields.field_data.svid == 0)
                    {
                        rtk_of_flowEntryField_write(unit_id, table_phase,
                                        flow_id,
                                        MATCH_FIELD_OTAG_EXIST,
                                        (uint8 *)&flow_entry_p->match_fields.field_data.svid,
                                        (uint8 *)&mask16);
                    }
                    else
                    {
                        rtk_of_flowEntryField_write(unit_id, table_phase,
                                        flow_id,
                                        MATCH_FIELD_OTAG_VID,
                                        (uint8 *)&flow_entry_p->match_fields.field_data.svid,
                                        (uint8 *)&mask16);
                    }
                    break;
                }
                case EN_OFPXMT_OFB_VLAN_PCP:
                {
                    DBG_SDN("\r\n[%s-%d] pcp_value=[%d]\r\n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.pcp_value);

                    rtk_of_flowEntryField_write(unit_id, table_phase,
                                        flow_id,
                                        MATCH_FIELD_OTAG_PCP,
                                        &flow_entry_p->match_fields.field_data.pcp_value, &flow_entry_p->match_fields.field_data.pcp_mask);
                    break;
                }
                case EN_OFPXMT_OFB_IP_DSCP:
                {
                    uint8 mask8 = 0x3f;

                    DBG_SDN("\r\n[%s-%d] dscp=[%d] \r\n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.dscp);
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                     MATCH_FIELD_IP_DSCP, &flow_entry_p->match_fields.field_data.dscp, &mask8);

                    break;
                }
                case EN_OFPXMT_OFB_IP_PROTO:
                {
                    uint8 mask8 = 0xff;
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                     MATCH_FIELD_IP_PROTO, &flow_entry_p->match_fields.field_data.protocol_val, &mask8);
                    break;
                }
                case EN_OFPXMT_OFB_ARP_OP:
                {
                    mask16 = 0xffff;
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                     MATCH_FIELD_ARP_OP, (uint8 *)&flow_entry_p->match_fields.field_data.arp_op, (uint8 *)&mask16);
                    break;
                }
                case EN_OFPXMT_OFB_IP_ECN:
                {
                    uint8 mask8 = 0x3;
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                 MATCH_FIELD_IP_ECN, (uint8 *)&flow_entry_p->match_fields.field_data.ip_ecn, (uint8 *)&mask8);
                    break;
                }
                case EN_OFPXMT_OFB_IPV4_SRC:
                case EN_OFPXMT_OFB_IPV6_SRC:
                {
                    if (fields_idx == EN_OFPXMT_OFB_IPV4_SRC)
                    {
#ifdef CONFIG_SDN_HAL_DBG
                        uint8 *ipaddr_p =  (uint8*)&flow_entry_p->match_fields.field_data.src_ipv4_addr;
                        DBG_SDN("src ip:%d.%d.%d.%d \r\n", ipaddr_p[0],
                                                           ipaddr_p[1],
                                                           ipaddr_p[2],
                                                           ipaddr_p[3]);
#endif
                        rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                         MATCH_FIELD_IPV4_SRC, (uint8 *)&flow_entry_p->match_fields.field_data.src_ipv4_addr, (uint8 *)&flow_entry_p->match_fields.field_data.src_ipv4_mask);
                    }
                    else
                    {
                        rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                        MATCH_FIELD_IPV6_SRC, flow_entry_p->match_fields.field_data.ipv6_src_addr, flow_entry_p->match_fields.field_data.ipv6_src_mask);
                    }
                    break;
                }
                case EN_OFPXMT_OFB_IPV4_DST:
                case EN_OFPXMT_OFB_IPV6_DST:
                {
                    if (fields_idx == EN_OFPXMT_OFB_IPV4_DST)
                    {
#ifdef CONFIG_SDN_HAL_DBG
                         uint8 *ipaddr_p =  (uint8*)&flow_entry_p->match_fields.field_data.dst_ipv4_addr;
                         DBG_SDN("dst ip:%d.%d.%d.%d \r\n", ipaddr_p[0],
                                                            ipaddr_p[1],
                                                            ipaddr_p[2],
                                                            ipaddr_p[3]);
#endif
                         rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                         MATCH_FIELD_IPV4_DST, (uint8 *)&flow_entry_p->match_fields.field_data.dst_ipv4_addr, (uint8 *)&flow_entry_p->match_fields.field_data.dst_ipv4_mask);
                    }
                    else
                    {
                         rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                         MATCH_FIELD_IPV6_DST, flow_entry_p->match_fields.field_data.ipv6_dst_addr, flow_entry_p->match_fields.field_data.ipv6_dst_mask);
                    }
                    break;
                }
                case EN_OFPXMT_OFB_SCTP_SRC:
                case EN_OFPXMT_OFB_TCP_SRC:
                case EN_OFPXMT_OFB_UDP_SRC:
                {
                    mask16 = 0xffff;

                    if(fields_idx == EN_OFPXMT_OFB_TCP_SRC)
                    {
                        port_no = flow_entry_p->match_fields.field_data.tcp_src_port_no;
                    }
                    else if (fields_idx == EN_OFPXMT_OFB_UDP_SRC)
                    {
                        port_no = flow_entry_p->match_fields.field_data.udp_src_port_no;
                    }
                    else
                    {
                        port_no = flow_entry_p->match_fields.field_data.sctp_src_port_no;
                    }
                    DBG_SDN(" \n[%s-%d] source port no=[%d] \n", __FUNCTION__, __LINE__,port_no);

                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                     MATCH_FIELD_L4_SRC_PORT, (uint8 *)&port_no, (uint8 *)&mask16);
                    break;
                }
                case EN_OFPXMT_OFB_SCTP_DST:
                case EN_OFPXMT_OFB_TCP_DST:
                case EN_OFPXMT_OFB_UDP_DST:
                {
                    mask16 = 0xffff;
                    if(fields_idx == EN_OFPXMT_OFB_TCP_DST)
                    {
                        port_no = flow_entry_p->match_fields.field_data.tcp_dst_port_no;
                    }
                    else if (fields_idx == EN_OFPXMT_OFB_UDP_DST)
                    {
                        port_no = flow_entry_p->match_fields.field_data.udp_dst_port_no;
                    }
                    else
                    {
                        port_no = flow_entry_p->match_fields.field_data.sctp_dst_port_no;
                    }

                    DBG_SDN(" \n[%s-%d] destination port no=[%d] \n", __FUNCTION__, __LINE__,port_no);

                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                     MATCH_FIELD_L4_DST_PORT, (uint8 *)&port_no, (uint8 *)&mask16);
                    break;
                }
                case EN_OFPXMT_OFB_ICMPV4_TYPE:
                case EN_OFPXMT_OFB_ICMPV6_TYPE:
                {
                    mask16 = 0xffff;
                    DBG_SDN(" \n[%s-%d] icmp type=[%d] \n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.icmp_type);

                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                     MATCH_FIELD_ICMP_TYPE, &flow_entry_p->match_fields.field_data.icmp_type, (uint8 *)&mask16);
                    break;
                }
                case EN_OFPXMT_OFB_ICMPV4_CODE:
                case EN_OFPXMT_OFB_ICMPV6_CODE:
                {
                    mask16 = 0xffff;
                    DBG_SDN(" \n[%s-%d] icmp code=[%d] \n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.icmp_code);

                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                         MATCH_FIELD_ICMP_CODE, &flow_entry_p->match_fields.field_data.icmp_code, (uint8 *)&mask16);
                    break;
                }
                case EN_OFPXMT_OFB_TUNNEL_ID:
                {
#ifdef CONFIG_SDN_HAL_DBG
                    uint8 *tunnel_id_p =  (uint8*)&flow_entry_p->match_fields.field_data.tunnel_id;
                    DBG_SDN("tunnel id: 0x%x\r\n", flow_entry_p->match_fields.field_data.tunnel_id);
#endif
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                    MATCH_FIELD_GTP_TEID, (uint8 *)&flow_entry_p->match_fields.field_data.tunnel_id, (uint8 *)&flow_entry_p->match_fields.field_data.tunnel_id_mask);
                    break;
                }
                case EN_OFPXMT_RTK_OUTER_IP_FLG:
                {
                    field_sel16 = (uint16)flow_entry_p->match_fields.field_data.outer_ip_flag;
                    field_sel16_mask = (uint16)flow_entry_p->match_fields.field_data.outer_ip_flag_mask;
                    field_sel16 = (field_sel16 << RTK_DPA_OUTER_IP_FLAG_OFFSET);
                    field_sel16_mask = (field_sel16_mask << RTK_DPA_OUTER_IP_FLAG_OFFSET);
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_FIELD_SELECTOR4, (uint8 *)&field_sel16, (uint8 *)&field_sel16_mask);
                    break;
                }
                case EN_OFPXMT_RTK_INNER_IP_VER:
                {
                    field_sel16 = flow_entry_p->match_fields.field_data.inner_ip_ver;
                    field_sel16_mask = flow_entry_p->match_fields.field_data.inner_ip_ver_mask;
                    field_sel16 = (field_sel16 << RTK_DPA_INNER_IP_VER_OFFSET);
                    field_sel16_mask = (field_sel16_mask << RTK_DPA_INNER_IP_VER_OFFSET);
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_FIELD_SELECTOR5, (uint8 *)&field_sel16, (uint8 *)&field_sel16_mask);
                    break;
                }
                case EN_OFPXMT_RTK_INNER_L4DP:
                {
                    field_sel16 = (uint16)flow_entry_p->match_fields.field_data.inner_l4dp;
                    field_sel16_mask = (uint16)flow_entry_p->match_fields.field_data.inner_l4dp_mask;
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_FIELD_SELECTOR10, (uint8 *)&field_sel16, (uint8 *)&field_sel16_mask);
                    break;
                }
                case EN_OFPXMT_RTK_INNER_SIP:
                {
                    field_sel = flow_entry_p->match_fields.field_data.inner_sip;
                    field_sel_mask = flow_entry_p->match_fields.field_data.inner_sip_mask;
                    field_sel16_hi = (uint16)(field_sel >> 16);
                    field_sel16_hi_mask = (uint16)(field_sel_mask >> 16);
                    ret = rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_FIELD_SELECTOR11, (uint8 *)&field_sel16_hi, (uint8 *)&field_sel16_hi_mask); /*Inner SPI High*/
                    if(ret != 0)
                        DBG_SDN("\n[%s][%d]MATCH_FIELD_FIELD_SELECTOR0 ERROR!!!\n",__FUNCTION__,__LINE__);
                    field_sel16_lo = (uint16)(field_sel & 0x0000ffff);
                    field_sel16_lo_mask = (uint16)(field_sel_mask & 0x0000ffff);
                    ret = rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_FIELD_SELECTOR12, (uint8 *)&field_sel16_lo, (uint8 *)&field_sel16_lo_mask); /*Inner SPI Low*/
                    if(ret != 0)
                        DBG_SDN("\n[%s][%d]MATCH_FIELD_FIELD_SELECTOR1 ERROR!!!\n",__FUNCTION__,__LINE__);
                    break;
                }
                case EN_OFPXMT_RTK_INNER_DIP:
                {
                    field_sel = flow_entry_p->match_fields.field_data.inner_dip;
                    field_sel_mask = flow_entry_p->match_fields.field_data.inner_dip_mask;
                    field_sel16_hi = (uint16)(field_sel >> 16);
                    field_sel16_hi_mask = (uint16)(field_sel_mask >> 16);
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_FIELD_SELECTOR2, (uint8 *)&field_sel16_hi, (uint8 *)&field_sel16_hi_mask); /*Inner SPI High*/
                    field_sel16_lo = (uint16)(field_sel & 0x0000ffff);
                    field_sel16_lo_mask = (uint16)(field_sel_mask & 0x0000ffff);
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_FIELD_SELECTOR3, (uint8 *)&field_sel16_lo, (uint8 *)&field_sel16_lo_mask); /*Inner SPI Low*/
                    break;
                }
                case EN_OFPXMT_RTK_INNER_IP_PROTO:
                {
                    field_sel8 = (uint8)flow_entry_p->match_fields.field_data.inner_ip_proto;
                    field_sel8_mask = (uint8)flow_entry_p->match_fields.field_data.inner_ip_proto_mask;
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_FIELD_SELECTOR6, (uint8 *)&field_sel8, (uint8 *)&field_sel8_mask);
                    break;
                }
                case EN_OFPXMT_RTK_INNER_TCP_FLAG:
                {
                    field_sel16 = (uint16)flow_entry_p->match_fields.field_data.inner_tcp_flag;
                    field_sel16_mask = (uint16)flow_entry_p->match_fields.field_data.inner_tcp_flag_mask;
                    field_sel16_mask = (field_sel16_mask & (uint16)RTK_DPA_INNER_TCP_FLAG_MASK);
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_FIELD_SELECTOR1, (uint8 *)&field_sel16, (uint8 *)&field_sel16_mask);
                    break;
                }
                default:
                    return SDN_HAL_RETURN_FAILED;
            }
            fields_idx++;
        }
#ifndef CONFIG_FLOW_AG
#else
    }
#endif
    return SDN_HAL_RETURN_OK;
}
static int32 _sdn_hal_flow_ft4_matchField_fill(sdn_hal_rule_flow_mgmt_t *flow_entry_p, sdn_hal_flow_t *flow)
{
    uint32 fields_idx = 0;
    uint32 unit_id    = 0;
    uint16 mask16     = 0xffff;
    uint16 port_no    = 0;
    uint8 template_mask = 0xf;
    uint8 template_id = 0;
    rtk_of_flow_id_t flow_id = 0;

    rtk_of_flowtable_phase_t table_phase;

    unit_id = SDN_HAL_MY_UNIT_ID();

    is_inport_existed = FALSE;
    if(SDN_HAL_RETURN_OK != sdn_hal_templateId_chosen(flow_entry_p->table_id,
                                     &template_id,
                                     flow_entry_p->match_fields.match_fields_bitmap))
    {
        return SDN_HAL_RETURN_FAILED;
    }

    table_phase = sdn_hal_rule_get_flow_table(flow_entry_p->table_id);
    flow_id = flow_entry_p->flow_id;

    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id, MATCH_FIELD_TEMPLATE_ID,
        &template_id,
        &template_mask);

    fields_idx = 0;
    while (fields_idx < EN_OFPXMT_RTK_EXP_MAX) /* Modify by MEC DPA*/
//    while (fields_idx < EN_OFPXMT_OFB_MAX)
    {
        if (!HAL_LIB_CHK_BIT_ON(flow_entry_p->match_fields.match_fields_bitmap, fields_idx))
        {
           fields_idx++;
           continue;
        }
        DBG_SDN("\r\n[%s-%d]set unit(%d), phase(%d), entry(%d), field(%d)\r\n", __FUNCTION__, __LINE__,
                                                                    (uint32)unit_id,
                                                                    (uint32)table_phase,
                                                                    flow_entry_p->flow_id, fields_idx);
        mask16 = 0xffff;
        switch((EN_OFP_OXM_OFB_MATCH_FIELDS_T)fields_idx)
        {
            case EN_OFPXMT_OFB_ETH_DST:
            {
                DBG_SDN("\r\n[%s-%d]dst mac:%02x:%02x:%02x:%02x:%02x:%02x \r\n", __FUNCTION__, __LINE__,
                        flow_entry_p->match_fields.field_data.dst_mac[0],
                        flow_entry_p->match_fields.field_data.dst_mac[1],
                        flow_entry_p->match_fields.field_data.dst_mac[2],
                        flow_entry_p->match_fields.field_data.dst_mac[3],
                        flow_entry_p->match_fields.field_data.dst_mac[4],
                        flow_entry_p->match_fields.field_data.dst_mac[5]);
                rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                 MATCH_FIELD_ETH_DST, flow_entry_p->match_fields.field_data.dst_mac, flow_entry_p->match_fields.field_data.dst_mac_mask);
                break;
            }
            case EN_OFPXMT_OFB_ARP_THA:
            {
                DBG_SDN("\r\n[%s-%d]dst mac:%02x:%02x:%02x:%02x:%02x:%02x \r\n", __FUNCTION__, __LINE__,
                        flow_entry_p->match_fields.field_data.dst_mac[0],
                        flow_entry_p->match_fields.field_data.dst_mac[1],
                        flow_entry_p->match_fields.field_data.dst_mac[2],
                        flow_entry_p->match_fields.field_data.dst_mac[3],
                        flow_entry_p->match_fields.field_data.dst_mac[4],
                        flow_entry_p->match_fields.field_data.dst_mac[5]);
                rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                 MATCH_FIELD_ARP_THA, flow_entry_p->match_fields.field_data.dst_mac, flow_entry_p->match_fields.field_data.dst_mac_mask);
                break;
            }
            case EN_OFPXMT_OFB_ETH_SRC:
            {
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

                        rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                MATCH_FIELD_ETH_SRC, flow_entry_p->match_fields.field_data.src_mac, flow_entry_p->match_fields.field_data.src_mac_mask);
                break;
            }
            case EN_OFPXMT_OFB_ARP_SHA:
            {
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

                        rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                MATCH_FIELD_ARP_SHA, flow_entry_p->match_fields.field_data.src_mac, flow_entry_p->match_fields.field_data.src_mac_mask);
                break;
            }
            case EN_OFPXMT_OFB_ETH_TYPE:
            {
                mask16 = 0xffff;
                uint16 eth_type = flow_entry_p->match_fields.field_data.eth_type_val;
                DBG_SDN("\r\n[%s-%d] eth_type:%02x\r\n", __FUNCTION__, __LINE__, eth_type);
                rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                             MATCH_FIELD_ETH_TYPE, (uint8 *)&eth_type, (uint8 *)&mask16);
                break;
            }
            case EN_OFPXMT_OFB_VLAN_VID:
            {
                mask16 = 0xffff;
                DBG_SDN("\r\n[%s-%d]Make sure that  is_default_template=[%s]  \r\n",__FUNCTION__, __LINE__,
                                  (is_default_template) ? "TRUE":"FALSE");

                DBG_SDN("\r\n[%s-%d]  svid=[%d]\r\n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.svid);
                DBG_SDN("\r\n[%s-%d]  cvid=[%d]\r\n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.cvid);
                if(flow_entry_p->match_fields.field_data.svid == 0)
                {
                    rtk_of_flowEntryField_write(unit_id, table_phase,
                                    flow_id,
                                    MATCH_FIELD_OTAG_EXIST,
                                    (uint8 *)&flow_entry_p->match_fields.field_data.svid,
                                    (uint8 *)&mask16);
                }
                else
                {
                    rtk_of_flowEntryField_write(unit_id, table_phase,
                                    flow_id,
                                    MATCH_FIELD_OTAG_VID,
                                    (uint8 *)&flow_entry_p->match_fields.field_data.svid,
                                    (uint8 *)&mask16);
                }
                break;
            }
            case EN_OFPXMT_OFB_VLAN_PCP:
            {
                DBG_SDN("\r\n[%s-%d] pcp_value=[%d]\r\n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.pcp_value);

                rtk_of_flowEntryField_write(unit_id, table_phase,
                                    flow_id,
                                    MATCH_FIELD_OTAG_PCP,
                                    &flow_entry_p->match_fields.field_data.pcp_value, &flow_entry_p->match_fields.field_data.pcp_mask);
                break;
            }
            case EN_OFPXMT_OFB_IP_DSCP:
            {
                uint8 mask8 = 0x3f;

                DBG_SDN("\r\n[%s-%d] dscp=[%d] \r\n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.dscp );
                rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                 MATCH_FIELD_IP_DSCP, &flow_entry_p->match_fields.field_data.dscp, &mask8);
                break;
            }
            case EN_OFPXMT_OFB_IP_PROTO:
            {
                uint8 mask8 = 0xff;
                rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                 MATCH_FIELD_IP_PROTO, &flow_entry_p->match_fields.field_data.protocol_val, &mask8);
                break;
            }
            case EN_OFPXMT_OFB_ARP_OP:
            {
                mask16 = 0xffff;
                rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                 MATCH_FIELD_ARP_OP, (uint8 *)&flow_entry_p->match_fields.field_data.arp_op, (uint8 *)&mask16);
                break;
            }
            case EN_OFPXMT_OFB_IP_ECN:
            {
                uint8 mask8 = 0x3;
                rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                             MATCH_FIELD_IP_ECN, (uint8 *)&flow_entry_p->match_fields.field_data.ip_ecn, (uint8 *)&mask8);
                break;
            }
            case EN_OFPXMT_OFB_IPV4_SRC:
            case EN_OFPXMT_OFB_IPV6_SRC:
            {
                if (fields_idx == EN_OFPXMT_OFB_IPV4_SRC)
                {
#ifdef CONFIG_SDN_HAL_DBG
                    uint8 *ipaddr_p =  (uint8*)&flow_entry_p->match_fields.field_data.src_ipv4_addr;
                    DBG_SDN("src ip:%d.%d.%d.%d \r\n", ipaddr_p[0],
                                                       ipaddr_p[1],
                                                       ipaddr_p[2],
                                                       ipaddr_p[3]);
#endif
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                     MATCH_FIELD_IPV4_SRC, (uint8 *)&flow_entry_p->match_fields.field_data.src_ipv4_addr, (uint8 *)&flow_entry_p->match_fields.field_data.src_ipv4_mask);
                }
                else
                {
                    rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                    MATCH_FIELD_IPV6_SRC, flow_entry_p->match_fields.field_data.ipv6_src_addr, flow_entry_p->match_fields.field_data.ipv6_src_mask);
                }
                break;
            }
            case EN_OFPXMT_OFB_IPV4_DST:
            case EN_OFPXMT_OFB_IPV6_DST:
            {
                if (fields_idx == EN_OFPXMT_OFB_IPV4_DST)
                {
#ifdef CONFIG_SDN_HAL_DBG
                     uint8 *ipaddr_p =  (uint8*)&flow_entry_p->match_fields.field_data.dst_ipv4_addr;
                     DBG_SDN("dst ip:%d.%d.%d.%d \r\n", ipaddr_p[0],
                                                        ipaddr_p[1],
                                                        ipaddr_p[2],
                                                        ipaddr_p[3]);
#endif
                     rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                     MATCH_FIELD_IPV4_DST, (uint8 *)&flow_entry_p->match_fields.field_data.dst_ipv4_addr, (uint8 *)&flow_entry_p->match_fields.field_data.dst_ipv4_mask);
                }
                else
                {
                     rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                     MATCH_FIELD_IPV6_DST, flow_entry_p->match_fields.field_data.ipv6_dst_addr, flow_entry_p->match_fields.field_data.ipv6_dst_mask);
                }
                break;
            }
            case EN_OFPXMT_OFB_SCTP_SRC:
            case EN_OFPXMT_OFB_TCP_SRC:
            case EN_OFPXMT_OFB_UDP_SRC:
            {
                mask16 = 0xffff;
                if(fields_idx == EN_OFPXMT_OFB_TCP_SRC)
                {
                    port_no = flow_entry_p->match_fields.field_data.tcp_src_port_no;
                }
                else if (fields_idx == EN_OFPXMT_OFB_UDP_SRC)
                {
                    port_no = flow_entry_p->match_fields.field_data.udp_src_port_no;
                }
                else
                {
                    port_no = flow_entry_p->match_fields.field_data.sctp_src_port_no;
                }
                DBG_SDN(" \n[%s-%d] source port no=[%d] \n", __FUNCTION__, __LINE__,port_no);

                rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                 MATCH_FIELD_L4_SRC_PORT, (uint8 *)&port_no, (uint8 *)&mask16);
                break;
            }
            case EN_OFPXMT_OFB_SCTP_DST:
            case EN_OFPXMT_OFB_TCP_DST:
            case EN_OFPXMT_OFB_UDP_DST:
            {
                mask16 = 0xffff;
                if(fields_idx == EN_OFPXMT_OFB_TCP_DST)
                {
                    port_no = flow_entry_p->match_fields.field_data.tcp_dst_port_no;
                }
                else if (fields_idx == EN_OFPXMT_OFB_UDP_DST)
                {
                    port_no = flow_entry_p->match_fields.field_data.udp_dst_port_no;
                }
                else
                {
                    port_no = flow_entry_p->match_fields.field_data.sctp_dst_port_no;
                }
                DBG_SDN(" \n[%s-%d] destination port no=[%d] \n", __FUNCTION__, __LINE__,port_no);

                rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                 MATCH_FIELD_L4_DST_PORT, (uint8 *)&port_no, (uint8 *)&mask16);
                break;
            }
            case EN_OFPXMT_OFB_ICMPV4_TYPE:
            case EN_OFPXMT_OFB_ICMPV6_TYPE:
            {
                mask16 = 0xffff;
                DBG_SDN(" \n[%s-%d] icmp type=[%d] \n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.icmp_type);

                rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                 MATCH_FIELD_ICMP_TYPE, &flow_entry_p->match_fields.field_data.icmp_type, (uint8 *)&mask16);
                break;
            }
            case EN_OFPXMT_OFB_ICMPV4_CODE:
            case EN_OFPXMT_OFB_ICMPV6_CODE:
            {
                mask16 = 0xffff;
                DBG_SDN(" \n[%s-%d] icmp code=[%d] \n", __FUNCTION__, __LINE__, flow_entry_p->match_fields.field_data.icmp_code);

                rtk_of_flowEntryField_write(unit_id, table_phase, flow_id,
                                                     MATCH_FIELD_ICMP_CODE, &flow_entry_p->match_fields.field_data.icmp_code, (uint8 *)&mask16);
                break;
            }
            default:
                return SDN_HAL_RETURN_FAILED;
        }
        fields_idx++;
    }

    return SDN_HAL_RETURN_OK;
}

static int32 _sdn_hal_flow_Ft0IngressAction_assign(uint32 action_type,
    sdn_hal_rule_flow_mgmt_t  *flow_entry_p,
    sdn_hal_flow_t *flow_p)
{
    uint32 phy_port = 0xffff;
    uint32 unit_id  = 0;
    uint32 tpid_idx = 0;
    SDN_PARAM_CHK(flow_entry_p == NULL, SDN_HAL_RETURN_FAILED);
    SDN_PARAM_CHK(flow_p == NULL, SDN_HAL_RETURN_FAILED);

    DBG_SDN("[%s-%d] table 0 action(%d)\r\n", __FUNCTION__, __LINE__, action_type);

    unit_id = SDN_HAL_MY_UNIT_ID();
    switch(action_type)
    {
        case EN_OFPAT_OUTPUT:
        {
            rtk_portmask_t phy_port_list;
            if (flow_entry_p->action_data.port_list_is_used == TRUE)
            {
                memset(&phy_port_list, 0, sizeof(phy_port_list));
                if (SDN_HAL_RETURN_OK != sdn_hal_logicalPortList2PhyPortList(unit_id, &flow_entry_p->action_data.port_list, &phy_port_list))
                {
                    DBG_SDN("[%s-%d] convert logic port to phy port failed\r\n", __FUNCTION__, __LINE__);
                    return SDN_HAL_RETURN_BAD_PORT;
                }

                if(flow_entry_p->action_data.controller_port == TRUE)
                {
                    sdn_hal_logicalPort2PhyPort(SDN_HAL_CPU_PORT, &phy_port);
                    SDN_HAL_LIB_PORT_MASK_ADD(phy_port_list.bits, phy_port);
                }

                flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.output_data.type = OF_OUTPUT_TYPE_MULTI_EGR_PORT;
                flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.output_data.portmask = phy_port_list;
            }
            else
            {
                if(flow_entry_p->action_data.controller_port == TRUE)
                {
                    flow_entry_p->action_data.port_no = SDN_HAL_CPU_PORT;
                    if (SDN_HAL_RETURN_OK != sdn_hal_logicalPort2PhyPort(flow_entry_p->action_data.port_no, &phy_port))
                    {
                        DBG_SDN("[%s-%d] convert logic port to phy port failed\r\n", __FUNCTION__, __LINE__);
                        return SDN_HAL_RETURN_BAD_PORT;
                    }
                    flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.output_data.type = OF_OUTPUT_TYPE_PHY_PORT;
                    flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.output_data.port = phy_port;
                }
                else if (flow_entry_p->action_data.is_normal_output == TRUE)
                {
                    flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.output_data.type = OF_OUTPUT_TYPE_LB;
                }
                else if (flow_entry_p->action_data.phy_in_port == TRUE)
                {
                    flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.output_data.type = OF_OUTPUT_TYPE_IN_PORT;
                }
                else if (flow_entry_p->action_data.flood_port == TRUE)
                {
                    flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.output_data.type = OF_OUTPUT_TYPE_FLOOD;
                }
                else
                {
                    if (SDN_HAL_RETURN_OK != sdn_hal_logicalPort2PhyPort(flow_entry_p->action_data.port_no, &phy_port))
                    {
                        DBG_SDN("[%s-%d] convert logic port to phy port failed\r\n", __FUNCTION__, __LINE__);
                        return SDN_HAL_RETURN_BAD_PORT;
                    }
                    flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.output_data.type = OF_OUTPUT_TYPE_PHY_PORT;
                    flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.output_data.port = phy_port;
                }
            }
            break;
        }
        case EN_OFPAT_COPY_TTL_OUT:
        {
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.cp_ttl_outward = ENABLED;
            break;
        }
        case EN_OFPAT_COPY_TTL_IN:
        {
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.cp_ttl_inward = ENABLED;
            break;
        }
        case EN_OFPAT_SET_MPLS_TTL:
        {
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.set_field_0_data.field_type = OF_IGR_FT0_SF_TYPE0_MPLS_TTL;
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.set_field_0_data.field_data = flow_entry_p->action_data.ttl;
            break;
        }
        case EN_OFPAT_DEC_MPLS_TTL:
        {
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.dec_mpls_ttl = ENABLED;
            break;
        }
        case EN_OFPAT_PUSH_VLAN:
        {
            for(tpid_idx = 0; tpid_idx < SDN_HAL_TPID_NUM_MAX; tpid_idx++)
            {
                /*Search for the exist entry.*/
                if ((g_tpid[tpid_idx].validate == true)
                    && (g_tpid[tpid_idx].ether_type == flow_entry_p->action_data.eth_type))
                {
                    flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.push_vlan_data.etherType_idx = tpid_idx;
                    flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.push_vlan = ENABLED;
                    return SDN_HAL_RETURN_OK;
                }
            }

            /*create entry if resource is enough.*/
            for(tpid_idx = 0; tpid_idx < SDN_HAL_TPID_NUM_MAX; tpid_idx++)
            {
                if (g_tpid[tpid_idx].validate == false)
                {
                    if(RT_ERR_OK == rtk_vlan_tpidEntry_set(unit_id, VLAN_TAG_TYPE_OUTER, tpid_idx, flow_entry_p->action_data.eth_type))
                    {
                        g_tpid[tpid_idx].validate = true;
                        g_tpid[tpid_idx].ether_type = flow_entry_p->action_data.eth_type;
                        flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.push_vlan = ENABLED;
                        flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.push_vlan_data.etherType_idx = tpid_idx;
                        return SDN_HAL_RETURN_OK;
                    }
                }
            }
            return SDN_HAL_RETURN_FAILED;
        }
        case EN_OFPAT_POP_VLAN:
        {
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.pop_vlan = ENABLED;
            break;
        }
        case EN_OFPAT_PUSH_MPLS:
        {
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.push_mpls = ENABLED;
            if (flow_entry_p->action_data.eth_type == 0x8847)
                flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.push_mpls_data.etherType = 0;
            else if (flow_entry_p->action_data.eth_type == 0x8848)
                flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.push_mpls_data.etherType = 1;
            else
                return SDN_HAL_RETURN_FAILED;
            break;
        }
        case EN_OFPAT_POP_MPLS:
        {
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.pop_mpls = ENABLED;
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.pop_mpls_type = OF_POP_MPLS_TYPE_OUTERMOST_LABEL;
            break;
        }
        case EN_OFPAT_SET_QUEUE:
        {
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.set_queue = ENABLED;
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.qid = flow_entry_p->action_data.qid;
            break;
        }
        case EN_OFPAT_GROUP:
        {
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.group = ENABLED;
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.gid = SDN_HAL_GET_ASIC_GROUPID(flow_entry_p->action_data.group_id);
            break;
        }
        case EN_OFPAT_SET_NW_TTL:
        {
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.set_field_0_data.field_type = OF_IGR_FT0_SF_TYPE0_IP_TTL;
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.set_field_0_data.field_data = flow_entry_p->action_data.ttl;
            break;
        }
        case EN_OFPAT_DEC_NW_TTL:
        {
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.dec_ip_ttl = ENABLED;
            break;
        }
        case EN_OFPAT_SET_FIELD:
        {
            if(flow_entry_p->action_data.set_svid)
            {
                flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.set_field_2_data.field_type = OF_IGR_FT0_SF_TYPE2_VLAN_ID;
                flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.set_field_2_data.field_data = flow_entry_p->action_data.svid;
            }
            if(flow_entry_p->action_data.set_remark_val)
            {
                flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.set_field_2_data.field_type = OF_IGR_FT0_SF_TYPE2_IP_DSCP;
                flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.set_field_2_data.field_data = flow_entry_p->action_data.remark_val;
            }
            if(flow_entry_p->action_data.set_pri)
            {
                flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.set_field_2_data.field_type = OF_IGR_FT0_SF_TYPE2_VLAN_PRI;
                flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.set_field_2_data.field_data = flow_entry_p->action_data.pri;
            }
            if(flow_entry_p->action_data.set_l4_src_port)
            {
                flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.set_field_4_data.field_type = OF_IGR_FT0_SF_TYPE4_L4_SPORT;
                flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.set_field_4_data.field_data = flow_entry_p->action_data.l4_src_port;
            }
            if(flow_entry_p->action_data.set_l4_dst_port)
            {
                flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.set_field_4_data.field_type = OF_IGR_FT0_SF_TYPE4_L4_DPORT;
                flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.set_field_4_data.field_data = flow_entry_p->action_data.l4_dst_port;
            }
            if(flow_entry_p->action_data.set_src_ip)
            {
                flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.set_field_3_data.field_type = OF_IGR_FT0_SF_TYPE3_SIP;
                flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.set_field_3_data.field_data = flow_entry_p->action_data.src_ip;
            }
            if(flow_entry_p->action_data.set_dst_ip)
            {
                flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.set_field_3_data.field_type = OF_IGR_FT0_SF_TYPE3_DIP;
                flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.set_field_3_data.field_data = flow_entry_p->action_data.dst_ip;
            }
            if(flow_entry_p->action_data.set_src_mac)
            {

                flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.set_field_0_data.field_type = OF_IGR_FT0_SF_TYPE0_SA;
                memcpy(&flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.set_field_0_data.mac,
                    ((char*)&flow_entry_p->action_data.src_mac), OFP_ETH_ALEN);
            }
            if(flow_entry_p->action_data.set_dst_mac)
            {
                flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.set_field_1_data.field_type = OF_IGR_FT0_SF_TYPE1_DA;
                memcpy(&flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wa_data.set_field_1_data.mac.octet,
                    &flow_entry_p->action_data.dst_mac, OFP_ETH_ALEN);
            }
            break;
        }
        default:
            break;
    }

    return SDN_HAL_RETURN_OK;
}

static int32 _sdn_hal_flow_Ft3IngressAction_assign(uint32 action_type,
    sdn_hal_rule_flow_mgmt_t  *flow_entry_p,
    sdn_hal_flow_t *flow_p)
{
    uint32 phy_port = 0xffff;
    uint32 unit_id  = 0;
    uint32 tpid_idx = 0;
    SDN_PARAM_CHK(flow_entry_p == NULL, SDN_HAL_RETURN_FAILED);
    SDN_PARAM_CHK(flow_p == NULL, SDN_HAL_RETURN_FAILED);

    DBG_SDN("[%s-%d] table 3 action(%d)\r\n", __FUNCTION__, __LINE__, action_type);

    unit_id = SDN_HAL_MY_UNIT_ID();
    switch(action_type)
    {
        case EN_OFPAT_OUTPUT:
        {
            rtk_portmask_t phy_port_list;
            if (flow_entry_p->action_data.port_list_is_used == TRUE)
            {
                memset(&phy_port_list, 0, sizeof(phy_port_list));
                if (SDN_HAL_RETURN_OK != sdn_hal_logicalPortList2PhyPortList(unit_id, &flow_entry_p->action_data.port_list, &phy_port_list))
                {
                    DBG_SDN("[%s-%d] convert logic port to phy port failed\r\n", __FUNCTION__, __LINE__);
                    return SDN_HAL_RETURN_BAD_PORT;
                }

                if(flow_entry_p->action_data.controller_port == TRUE)
                {
                    sdn_hal_logicalPort2PhyPort(SDN_HAL_CPU_PORT, &phy_port);
                    SDN_HAL_LIB_PORT_MASK_ADD(phy_port_list.bits, phy_port);
                }

                flow_p->ft0_flow.ft0_3_4_ins.igrFt_3.wa_data.output_data.type = OF_OUTPUT_TYPE_MULTI_EGR_PORT;
                flow_p->ft0_flow.ft0_3_4_ins.igrFt_3.wa_data.output_data.portmask = phy_port_list;
            }
            else
            {
                if(flow_entry_p->action_data.controller_port == TRUE)
                {
                    flow_entry_p->action_data.port_no = SDN_HAL_CPU_PORT;
                    if (SDN_HAL_RETURN_OK != sdn_hal_logicalPort2PhyPort(flow_entry_p->action_data.port_no, &phy_port))
                    {
                        DBG_SDN("[%s-%d] convert logic port to phy port failed\r\n", __FUNCTION__, __LINE__);
                        return SDN_HAL_RETURN_BAD_PORT;
                    }
                    flow_p->ft0_flow.ft0_3_4_ins.igrFt_3.wa_data.output_data.type = OF_OUTPUT_TYPE_PHY_PORT;
                    flow_p->ft0_flow.ft0_3_4_ins.igrFt_3.wa_data.output_data.port = phy_port;
                }
                else if (flow_entry_p->action_data.is_normal_output == TRUE)
                {
                    flow_p->ft0_flow.ft0_3_4_ins.igrFt_3.wa_data.output_data.type = OF_OUTPUT_TYPE_LB;
                }
                else if (flow_entry_p->action_data.phy_in_port == TRUE)
                {
                    flow_p->ft0_flow.ft0_3_4_ins.igrFt_3.wa_data.output_data.type = OF_OUTPUT_TYPE_IN_PORT;
                }
                else if (flow_entry_p->action_data.flood_port == TRUE)
                {
                    flow_p->ft0_flow.ft0_3_4_ins.igrFt_3.wa_data.output_data.type = OF_OUTPUT_TYPE_FLOOD;
                }
                else
                {
                    if (SDN_HAL_RETURN_OK != sdn_hal_logicalPort2PhyPort(flow_entry_p->action_data.port_no, &phy_port))
                    {
                        DBG_SDN("[%s-%d] convert logic port to phy port failed\r\n", __FUNCTION__, __LINE__);
                        return SDN_HAL_RETURN_BAD_PORT;
                    }
                    flow_p->ft0_flow.ft0_3_4_ins.igrFt_3.wa_data.output_data.type = OF_OUTPUT_TYPE_PHY_PORT;
                    flow_p->ft0_flow.ft0_3_4_ins.igrFt_3.wa_data.output_data.port = phy_port;
                }
            }
            break;
        }
        case EN_OFPAT_COPY_TTL_OUT:
        {
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.cp_ttl_outward = ENABLED;
            break;
        }
        case EN_OFPAT_COPY_TTL_IN:
        {
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.cp_ttl_inward = ENABLED;
            break;
        }
        case EN_OFPAT_SET_MPLS_TTL:
        {
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.set_field_0_data.field_type = OF_IGR_FT0_SF_TYPE0_MPLS_TTL;
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.set_field_0_data.field_data = flow_entry_p->action_data.ttl;
            break;
        }
        case EN_OFPAT_DEC_MPLS_TTL:
        {
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.dec_mpls_ttl = ENABLED;
            break;
        }
        case EN_OFPAT_PUSH_VLAN:
        {
            for(tpid_idx = 0; tpid_idx < SDN_HAL_TPID_NUM_MAX; tpid_idx++)
            {
                /*Search for the exist entry.*/
                if ((g_tpid[tpid_idx].validate == true)
                    && (g_tpid[tpid_idx].ether_type == flow_entry_p->action_data.eth_type))
                {
                    flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.push_vlan_data.etherType_idx = tpid_idx;
                    flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.push_vlan = ENABLED;
                    return SDN_HAL_RETURN_OK;
                }
            }

            /*create entry if resource is enough.*/
            for(tpid_idx = 0; tpid_idx < SDN_HAL_TPID_NUM_MAX; tpid_idx++)
            {
                if (g_tpid[tpid_idx].validate == false)
                {
                    if(RT_ERR_OK == rtk_vlan_tpidEntry_set(unit_id, VLAN_TAG_TYPE_OUTER, tpid_idx, flow_entry_p->action_data.eth_type))
                    {
                        g_tpid[tpid_idx].validate = true;
                        g_tpid[tpid_idx].ether_type = flow_entry_p->action_data.eth_type;
                        flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.push_vlan = ENABLED;
                        flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.push_vlan_data.etherType_idx = tpid_idx;
                        return SDN_HAL_RETURN_OK;
                    }
                }
            }
            return SDN_HAL_RETURN_FAILED;
        }
        case EN_OFPAT_POP_VLAN:
        {
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.pop_vlan = ENABLED;
            break;
        }
        case EN_OFPAT_PUSH_MPLS:
        {
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.push_mpls = ENABLED;
            if (flow_entry_p->action_data.eth_type == 0x8847)
                flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.push_mpls_data.etherType = 0;
            else if (flow_entry_p->action_data.eth_type == 0x8848)
                flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.push_mpls_data.etherType = 1;
            else
                return SDN_HAL_RETURN_FAILED;
            break;
        }
        case EN_OFPAT_POP_MPLS:
        {
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.pop_mpls = ENABLED;
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.pop_mpls_type = OF_POP_MPLS_TYPE_OUTERMOST_LABEL;
            break;
        }
        case EN_OFPAT_SET_QUEUE:
        {
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.set_queue = ENABLED;
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.qid = flow_entry_p->action_data.qid;
            break;
        }
        case EN_OFPAT_GROUP:
        {
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.group = ENABLED;
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.gid = SDN_HAL_GET_ASIC_GROUPID(flow_entry_p->action_data.group_id);
            break;
        }
        case EN_OFPAT_SET_NW_TTL:
        {
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.set_field_0_data.field_type = OF_IGR_FT0_SF_TYPE0_IP_TTL;
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.set_field_0_data.field_data = flow_entry_p->action_data.ttl;
            break;
        }
        case EN_OFPAT_DEC_NW_TTL:
        {
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.dec_ip_ttl = ENABLED;
            break;
        }
        case EN_OFPAT_SET_FIELD:
        {
            if(flow_entry_p->action_data.set_svid)
            {
                flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.set_field_2_data.field_type = OF_IGR_FT0_SF_TYPE2_VLAN_ID;
                flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.set_field_2_data.field_data = flow_entry_p->action_data.svid;
            }
            if(flow_entry_p->action_data.set_remark_val)
            {
                flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.set_field_2_data.field_type = OF_IGR_FT0_SF_TYPE2_IP_DSCP;
                flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.set_field_2_data.field_data = flow_entry_p->action_data.remark_val;
            }
            if(flow_entry_p->action_data.set_pri)
            {
                flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.set_field_2_data.field_type = OF_IGR_FT0_SF_TYPE2_VLAN_PRI;
                flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.set_field_2_data.field_data = flow_entry_p->action_data.pri;
            }
            if(flow_entry_p->action_data.set_l4_src_port)
            {
                flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.set_field_4_data.field_type = OF_IGR_FT0_SF_TYPE4_L4_SPORT;
                flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.set_field_4_data.field_data = flow_entry_p->action_data.l4_src_port;
            }
            if(flow_entry_p->action_data.set_l4_dst_port)
            {
                flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.set_field_4_data.field_type = OF_IGR_FT0_SF_TYPE4_L4_DPORT;
                flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.set_field_4_data.field_data = flow_entry_p->action_data.l4_dst_port;
            }
            if(flow_entry_p->action_data.set_src_ip)
            {
                flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.set_field_3_data.field_type = OF_IGR_FT0_SF_TYPE3_SIP;
                flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.set_field_3_data.field_data = flow_entry_p->action_data.src_ip;
            }
            if(flow_entry_p->action_data.set_dst_ip)
            {
                flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.set_field_3_data.field_type = OF_IGR_FT0_SF_TYPE3_DIP;
                flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.set_field_3_data.field_data = flow_entry_p->action_data.dst_ip;
            }
            if(flow_entry_p->action_data.set_src_mac)
            {

                flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.set_field_0_data.field_type = OF_IGR_FT0_SF_TYPE0_SA;
                memcpy(&flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.set_field_0_data.mac,
                    ((char*)&flow_entry_p->action_data.src_mac), OFP_ETH_ALEN);
            }
            if(flow_entry_p->action_data.set_dst_mac)
            {
                flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.set_field_1_data.field_type = OF_IGR_FT0_SF_TYPE1_DA;
                memcpy(&flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wa_data.set_field_1_data.mac.octet,
                    &flow_entry_p->action_data.dst_mac, OFP_ETH_ALEN);
            }
            break;
        }
        default:
            break;
    }

    return SDN_HAL_RETURN_OK;
}

static int32 _sdn_hal_flow_egressAction_assign(uint32 action_type,
                                                        sdn_hal_rule_flow_mgmt_t *flow_entry_p,
                                                        sdn_hal_flow_t *flow_p)
{
    uint32 unit_id  = 0;
    uint32 tpid_idx = 0;
    SDN_PARAM_CHK(flow_entry_p == NULL, SDN_HAL_RETURN_FAILED);
    SDN_PARAM_CHK(flow_p == NULL, SDN_HAL_RETURN_FAILED);

    DBG_SDN("[%s-%d] egress table 0 action(%d)\r\n", __FUNCTION__, __LINE__, action_type);

    unit_id = SDN_HAL_MY_UNIT_ID();
    switch(action_type)
    {
        case EN_OFPAT_PUSH_VLAN:
        {
            for(tpid_idx = 0; tpid_idx < SDN_HAL_TPID_NUM_MAX; tpid_idx++)
            {
                /*Search for the exist entry.*/
                if ((g_tpid[tpid_idx].validate == true)
                    && (g_tpid[tpid_idx].ether_type == flow_entry_p->action_data.eth_type))
                {
                    flow_p->ft4_flow.ft0_3_4_ins.egrFt_0.wa_data.push_vlan_data.etherType_idx = tpid_idx;
                    flow_p->ft4_flow.ft0_3_4_ins.egrFt_0.wa_data.push_vlan = ENABLED;
                    return SDN_HAL_RETURN_OK;
                }
            }

            /*create entry if resource is enough.*/
            for(tpid_idx = 0; tpid_idx < SDN_HAL_TPID_NUM_MAX; tpid_idx++)
            {
                if (g_tpid[tpid_idx].validate == false)
                {
                    if(RT_ERR_OK == rtk_vlan_tpidEntry_set(unit_id, VLAN_TAG_TYPE_OUTER, tpid_idx, flow_entry_p->action_data.eth_type))
                    {
                        g_tpid[tpid_idx].validate = true;
                        g_tpid[tpid_idx].ether_type = flow_entry_p->action_data.eth_type;
                        flow_p->ft4_flow.ft0_3_4_ins.egrFt_0.wa_data.push_vlan = ENABLED;
                        flow_p->ft4_flow.ft0_3_4_ins.egrFt_0.wa_data.push_vlan_data.etherType_idx = tpid_idx;
                        return SDN_HAL_RETURN_OK;
                    }
                }
            }
            return SDN_HAL_RETURN_FAILED;
        }
        case EN_OFPAT_POP_VLAN:
        {
            flow_p->ft4_flow.ft0_3_4_ins.egrFt_0.wa_data.pop_vlan = ENABLED;
            break;
        }
        case EN_OFPAT_SET_FIELD:
        {
            if(flow_entry_p->action_data.set_svid)
            {
                flow_p->ft4_flow.ft0_3_4_ins.egrFt_0.wa_data.set_field_1_data.field_type = OF_EGR_FT_SF_TYPE1_VLAN_ID;
                flow_p->ft4_flow.ft0_3_4_ins.egrFt_0.wa_data.set_field_1_data.field_data = flow_entry_p->action_data.svid;
            }
            if(flow_entry_p->action_data.set_remark_val)
            {
                flow_p->ft4_flow.ft0_3_4_ins.egrFt_0.wa_data.set_field_0_data.field_type = OF_EGR_FT_SF_TYPE0_IP_DSCP ;
                flow_p->ft4_flow.ft0_3_4_ins.egrFt_0.wa_data.set_field_0_data.field_data = flow_entry_p->action_data.remark_val;
            }
            if(flow_entry_p->action_data.set_pri)
            {
                flow_p->ft4_flow.ft0_3_4_ins.egrFt_0.wa_data.set_field_0_data.field_type = OF_EGR_FT_SF_TYPE0_VLAN_PRI;
                flow_p->ft4_flow.ft0_3_4_ins.egrFt_0.wa_data.set_field_0_data.field_data = flow_entry_p->action_data.pri;
            }
            break;
        }
        default:
            break;
    }

    return SDN_HAL_RETURN_OK;
}
static int32 _sdn_hal_flow_l2Action_assign(uint32 action_type,
                                                        sdn_hal_rule_flow_mgmt_t *flow_entry_p,
                                                        sdn_hal_flow_t *flow_p)
{
    uint32 phy_port = 0xffff;
    uint32 unit_id  = 0;
    uint32 tpid_idx = 0;
    SDN_PARAM_CHK(flow_entry_p == NULL, SDN_HAL_RETURN_FAILED);
    SDN_PARAM_CHK(flow_p == NULL, SDN_HAL_RETURN_FAILED);

    DBG_SDN("[%s-%d] table 1 action(%d)\r\n", __FUNCTION__, __LINE__, action_type);

    unit_id = SDN_HAL_MY_UNIT_ID();
    switch(action_type)
    {
        case EN_OFPAT_OUTPUT:
        {
            rtk_portmask_t phy_port_list;
            if (flow_entry_p->action_data.port_list_is_used == TRUE)
            {
                memset(&phy_port_list, 0, sizeof(phy_port_list));
                if (SDN_HAL_RETURN_OK != sdn_hal_logicalPortList2PhyPortList(unit_id, &flow_entry_p->action_data.port_list, &phy_port_list))
                {
                    DBG_SDN("[%s-%d] convert logic port to phy port failed\r\n", __FUNCTION__, __LINE__);
                    return SDN_HAL_RETURN_BAD_PORT;
                }

                if(flow_entry_p->action_data.controller_port == TRUE)
                {
                    sdn_hal_logicalPort2PhyPort(SDN_HAL_CPU_PORT, &phy_port);
                    SDN_HAL_LIB_PORT_MASK_ADD(phy_port_list.bits, phy_port);
                }

                flow_p->ft1_flow.ins.wa_data.output_data.type = OF_OUTPUT_TYPE_MULTI_EGR_PORT;
                flow_p->ft1_flow.ins.wa_data.output_data.portmask = phy_port_list;
            }
            else
            {
                if(flow_entry_p->action_data.controller_port == TRUE)
                {
                    flow_entry_p->action_data.port_no = SDN_HAL_CPU_PORT;
                    if (SDN_HAL_RETURN_OK != sdn_hal_logicalPort2PhyPort(flow_entry_p->action_data.port_no, &phy_port))
                    {
                        DBG_SDN("[%s-%d] convert logic port to phy port failed\r\n", __FUNCTION__, __LINE__);
                        return SDN_HAL_RETURN_BAD_PORT;
                    }
                    flow_p->ft1_flow.ins.wa_data.output_data.type = OF_OUTPUT_TYPE_PHY_PORT;
                    flow_p->ft1_flow.ins.wa_data.output_data.port = phy_port;
                }
                else if (flow_entry_p->action_data.is_normal_output == TRUE)
                {
                    flow_p->ft1_flow.ins.wa_data.output_data.type = OF_OUTPUT_TYPE_LB;
                }
                else if (flow_entry_p->action_data.phy_in_port == TRUE)
                {
                    flow_p->ft1_flow.ins.wa_data.output_data.type = OF_OUTPUT_TYPE_IN_PORT;
                }
                else if (flow_entry_p->action_data.flood_port == TRUE)
                {
                    flow_p->ft1_flow.ins.wa_data.output_data.type = OF_OUTPUT_TYPE_FLOOD;
                }
                else
                {
                    if (SDN_HAL_RETURN_OK != sdn_hal_logicalPort2PhyPort(flow_entry_p->action_data.port_no, &phy_port))
                    {
                        DBG_SDN("[%s-%d] convert logic port to phy port failed\r\n", __FUNCTION__, __LINE__);
                        return SDN_HAL_RETURN_BAD_PORT;
                    }
                    flow_p->ft1_flow.ins.wa_data.output_data.type = OF_OUTPUT_TYPE_PHY_PORT;
                    flow_p->ft1_flow.ins.wa_data.output_data.port = phy_port;
                }
            }
            break;
        }
        case EN_OFPAT_PUSH_VLAN:
        {
            for(tpid_idx = 0; tpid_idx < SDN_HAL_TPID_NUM_MAX; tpid_idx++)
            {
                /*Search for the exist entry.*/
                if ((g_tpid[tpid_idx].validate == true)
                    && (g_tpid[tpid_idx].ether_type == flow_entry_p->action_data.eth_type))
                {
                    flow_p->ft1_flow.ins.wa_data.push_vlan_data.etherType_idx = tpid_idx;
                    flow_p->ft1_flow.ins.wa_data.push_vlan = ENABLED;
                    return SDN_HAL_RETURN_OK;
                }
            }

            /*create entry if resource is enough.*/
            for(tpid_idx = 0; tpid_idx < SDN_HAL_TPID_NUM_MAX; tpid_idx++)
            {
                if (g_tpid[tpid_idx].validate == false)
                {
                    if(RT_ERR_OK == rtk_vlan_tpidEntry_set(unit_id, VLAN_TAG_TYPE_OUTER, tpid_idx, flow_entry_p->action_data.eth_type))
                    {
                        g_tpid[tpid_idx].validate = true;
                        g_tpid[tpid_idx].ether_type = flow_entry_p->action_data.eth_type;
                        flow_p->ft1_flow.ins.wa_data.push_vlan = ENABLED;
                        flow_p->ft1_flow.ins.wa_data.push_vlan_data.etherType_idx = tpid_idx;
                        return SDN_HAL_RETURN_OK;
                    }
                }
            }
            return SDN_HAL_RETURN_FAILED;
        }
        case EN_OFPAT_SET_QUEUE:
        {
            flow_p->ft1_flow.ins.wa_data.set_queue = ENABLED;
            flow_p->ft1_flow.ins.wa_data.qid = flow_entry_p->action_data.qid;
            break;
        }
        case EN_OFPAT_SET_FIELD:
        {
            if(flow_entry_p->action_data.set_remark_val)
            {
                flow_p->ft1_flow.ins.wa_data.set_field_0_data.field_type = OF_IGR_FT_SF_TYPE0_IP_DSCP;
                flow_p->ft1_flow.ins.wa_data.set_field_0_data.field_data = flow_entry_p->action_data.remark_val;
            }
            if(flow_entry_p->action_data.set_svid)
            {
                flow_p->ft1_flow.ins.wa_data.set_field_0_data.field_type = OF_IGR_FT_SF_TYPE0_VLAN_ID;
                flow_p->ft1_flow.ins.wa_data.set_field_0_data.field_data = flow_entry_p->action_data.svid;
            }
            if(flow_entry_p->action_data.set_pri)
            {
                flow_p->ft1_flow.ins.wa_data.set_field_1_data.field_type = OF_IGR_FT_SF_TYPE1_VLAN_PRI;
                flow_p->ft1_flow.ins.wa_data.set_field_1_data.field_data = flow_entry_p->action_data.pri;
            }
            break;
        }
        default:
            break;
    }

    return SDN_HAL_RETURN_OK;

}
static int32 _sdn_hal_flow_l3Action_assign(uint32 action_type,
                                                        sdn_hal_rule_flow_mgmt_t *flow_entry_p,
                                                        sdn_hal_flow_t *flow_p)
{
    rtk_of_igrFT2Ins_t inst;
    uint32 phy_port = 0xffff;
    uint32 unit_id  = 0;
    uint32 tpid_idx = 0;
    SDN_PARAM_CHK(flow_entry_p == NULL, SDN_HAL_RETURN_FAILED);
    SDN_PARAM_CHK(flow_p == NULL, SDN_HAL_RETURN_FAILED);
    memset(&inst, 0, sizeof(rtk_of_igrFT2Ins_t));
    if(_sdn_hal_l3Cam_is_used(flow_entry_p))
    {
        memcpy(&inst.wa_data, &flow_p->ft2_flow.ft2_cam_flow.ins.wa_data, sizeof(rtk_of_igrFT2InsWriteAct_t));
    }
    else
    {
        memcpy(&inst.wa_data, &flow_p->ft2_flow.ft2_hash_flow.ins.wa_data, sizeof(rtk_of_igrFT2InsWriteAct_t));
    }

    DBG_SDN("[%s-%d] table 2 action(%d)\r\n", __FUNCTION__, __LINE__, action_type);

    unit_id = SDN_HAL_MY_UNIT_ID();
    switch(action_type)
    {
        case EN_OFPAT_OUTPUT:
        {
            rtk_portmask_t phy_port_list;
            if (flow_entry_p->action_data.port_list_is_used == TRUE)
            {
                memset(&phy_port_list, 0, sizeof(phy_port_list));
                if (SDN_HAL_RETURN_OK != sdn_hal_logicalPortList2PhyPortList(unit_id, &flow_entry_p->action_data.port_list, &phy_port_list))
                {
                    DBG_SDN("[%s-%d] convert logic port to phy port failed\r\n", __FUNCTION__, __LINE__);
                    return SDN_HAL_RETURN_BAD_PORT;
                }

                if(flow_entry_p->action_data.controller_port == TRUE)
                {
                    sdn_hal_logicalPort2PhyPort(SDN_HAL_CPU_PORT, &phy_port);
                    SDN_HAL_LIB_PORT_MASK_ADD(phy_port_list.bits, phy_port);
                }

                inst.wa_data.output_data.type = OF_OUTPUT_TYPE_MULTI_EGR_PORT;
                inst.wa_data.output_data.portmask = phy_port_list;
            }
            else
            {
                if(flow_entry_p->action_data.controller_port == TRUE)
                {
                    flow_entry_p->action_data.port_no = SDN_HAL_CPU_PORT;
                    if (SDN_HAL_RETURN_OK != sdn_hal_logicalPort2PhyPort(flow_entry_p->action_data.port_no, &phy_port))
                    {
                        DBG_SDN("[%s-%d] convert logic port to phy port failed\r\n", __FUNCTION__, __LINE__);
                        return SDN_HAL_RETURN_BAD_PORT;
                    }
                    inst.wa_data.output_data.type = OF_OUTPUT_TYPE_PHY_PORT;
                    inst.wa_data.output_data.port = phy_port;
                }
                else if (flow_entry_p->action_data.is_normal_output == TRUE)
                {
                    inst.wa_data.output_data.type = OF_OUTPUT_TYPE_LB;
                }
                else if (flow_entry_p->action_data.phy_in_port == TRUE)
                {
                    inst.wa_data.output_data.type = OF_OUTPUT_TYPE_IN_PORT;
                }
                else if (flow_entry_p->action_data.flood_port == TRUE)
                {
                    inst.wa_data.output_data.type = OF_OUTPUT_TYPE_FLOOD;
                }
                else
                {
                    if (SDN_HAL_RETURN_OK != sdn_hal_logicalPort2PhyPort(flow_entry_p->action_data.port_no, &phy_port))
                    {
                        DBG_SDN("[%s-%d] convert logic port to phy port failed\r\n", __FUNCTION__, __LINE__);
                        return SDN_HAL_RETURN_BAD_PORT;
                    }
                    inst.wa_data.output_data.type = OF_OUTPUT_TYPE_PHY_PORT;
                    inst.wa_data.output_data.port = phy_port;
                }
            }
            break;
        }
        case EN_OFPAT_PUSH_VLAN:
        {
            for(tpid_idx = 0; tpid_idx < SDN_HAL_TPID_NUM_MAX; tpid_idx++)
            {
                /*Search for the exist entry.*/
                if ((g_tpid[tpid_idx].validate == true)
                    && (g_tpid[tpid_idx].ether_type == flow_entry_p->action_data.eth_type))
                {
                    inst.wa_data.push_vlan_data.etherType_idx = tpid_idx;
                    inst.wa_data.push_vlan = ENABLED;
                    goto L3_INS_FILL_SUCCESS;
                }
            }

            /*create entry if resource is enough.*/
            for(tpid_idx = 0; tpid_idx < SDN_HAL_TPID_NUM_MAX; tpid_idx++)
            {
                if (g_tpid[tpid_idx].validate == false)
                {
                    if(RT_ERR_OK == rtk_vlan_tpidEntry_set(unit_id, VLAN_TAG_TYPE_OUTER, tpid_idx, flow_entry_p->action_data.eth_type))
                    {
                        g_tpid[tpid_idx].validate = true;
                        g_tpid[tpid_idx].ether_type = flow_entry_p->action_data.eth_type;
                        inst.wa_data.push_vlan = ENABLED;
                        inst.wa_data.push_vlan_data.etherType_idx = tpid_idx;
                        goto L3_INS_FILL_SUCCESS;
                    }
                }
            }
            return SDN_HAL_RETURN_FAILED;
        }
        case EN_OFPAT_SET_QUEUE:
        {
            inst.wa_data.set_queue = ENABLED;
            inst.wa_data.qid = flow_entry_p->action_data.qid;
            break;
        }
        case EN_OFPAT_GROUP:
        {
            inst.wa_data.group = ENABLED;
            inst.wa_data.gid = SDN_HAL_GET_ASIC_GROUPID(flow_entry_p->action_data.group_id); // flow_entry_p->action_data.group_id;
            break;
        }
        case EN_OFPAT_DEC_NW_TTL:
        {
            inst.wa_data.dec_ip_ttl = ENABLED;
            break;
        }
        case EN_OFPAT_SET_FIELD:
        {
            if(flow_entry_p->action_data.set_svid)
            {
                inst.wa_data.set_field_2_data.field_type = OF_IGR_FT2_ACT_SF_TYPE2_VLAN_ID;
                inst.wa_data.set_field_2_data.field_data = flow_entry_p->action_data.svid;
            }
            if(flow_entry_p->action_data.set_remark_val)
            {
                inst.wa_data.set_field_2_data.field_type = OF_IGR_FT2_ACT_SF_TYPE2_IP_DSCP;
                inst.wa_data.set_field_2_data.field_data = flow_entry_p->action_data.remark_val;
            }
            if(flow_entry_p->action_data.set_pri)
            {
                inst.wa_data.set_field_2_data.field_type = OF_IGR_FT2_ACT_SF_TYPE2_VLAN_PRI;
                inst.wa_data.set_field_2_data.field_data = flow_entry_p->action_data.pri;
            }
            if(flow_entry_p->action_data.set_src_mac)
            {
                    DBG_SDN("[%s-%d] table 2 action(%d)\r\n", __FUNCTION__, __LINE__, action_type);
                inst.wa_data.set_field_0_data.field_type = OF_IGR_FT2_ACT_SF_TYPE0_SA;
                memcpy(&inst.wa_data.set_field_0_data.mac,
                    ((char*)&flow_entry_p->action_data.src_mac), OFP_ETH_ALEN);
            }
            if(flow_entry_p->action_data.set_dst_mac)
            {
                    DBG_SDN("[%s-%d] table 2 action(%d)\r\n", __FUNCTION__, __LINE__, action_type);
                inst.wa_data.set_field_1_data.field_type = OF_IGR_FT2_ACT_SF_TYPE1_DA;
                memcpy(&inst.wa_data.set_field_1_data.mac,
                    ((char*)&flow_entry_p->action_data.dst_mac), OFP_ETH_ALEN);
            }
            break;
        }
        default:
            break;
    }

L3_INS_FILL_SUCCESS:
    if(_sdn_hal_l3Cam_is_used(flow_entry_p))
    {
        memcpy(&flow_p->ft2_flow.ft2_cam_flow.ins.wa_data , &inst.wa_data, sizeof(rtk_of_igrFT2InsWriteAct_t));
    }
    else
    {
        memcpy(&flow_p->ft2_flow.ft2_hash_flow.ins.wa_data , &inst.wa_data, sizeof(rtk_of_igrFT2InsWriteAct_t));
    }

    return SDN_HAL_RETURN_OK;

}
int32 sdn_hal_flow_action_fill(rtk_of_flowtable_phase_t phase, sdn_hal_rule_flow_mgmt_t *flow_entry_p, sdn_hal_flow_t *flow_p)
{
    uint32 action_index = 0;
    uint32 ret = SDN_HAL_RETURN_FAILED;
    SDN_PARAM_CHK(flow_p == NULL, SDN_HAL_RETURN_NULL_DATA);

    for (action_index = 0; action_index < EN_OFPAT_STD_MAX; action_index++)
    {
        if (!HAL_LIB_CHK_BIT_ON(flow_entry_p->action_data.action_mask, action_index))
        {
            continue;
        }

        switch(phase)
        {
            case FT_PHASE_IGR_FT_0:
//                DBG_SDN("[%s-%d] table0 action set  action_type %d \r\n", __FUNCTION__, __LINE__, action_index);
                DBG_SDN("[%s-%d] table0 action set\r\n", __FUNCTION__, __LINE__);
                if ((ret = _sdn_hal_flow_Ft0IngressAction_assign(action_index, flow_entry_p, flow_p)) != SDN_HAL_RETURN_OK)
                    return ret;
                break;

            case FT_PHASE_IGR_FT_1:
                DBG_SDN("[%s-%d] table1 action set\r\n", __FUNCTION__, __LINE__);
                if ((ret = _sdn_hal_flow_l2Action_assign(action_index, flow_entry_p, flow_p)) != SDN_HAL_RETURN_OK)
                    return ret;
                break;

            case FT_PHASE_IGR_FT_2:
                DBG_SDN("[%s-%d] table2 action set\r\n", __FUNCTION__, __LINE__);
                if ((ret = _sdn_hal_flow_l3Action_assign(action_index, flow_entry_p, flow_p)) != SDN_HAL_RETURN_OK)
                    return ret;
                break;

            case FT_PHASE_IGR_FT_3:
                DBG_SDN("[%s-%d] table3 action set\r\n", __FUNCTION__, __LINE__);
                if ((ret = _sdn_hal_flow_Ft3IngressAction_assign(action_index, flow_entry_p, flow_p)) != SDN_HAL_RETURN_OK)
                    return ret;
                break;

            case FT_PHASE_EGR_FT_0:
                DBG_SDN("[%s-%d] table4 action set\r\n", __FUNCTION__, __LINE__);
                if ((ret = _sdn_hal_flow_egressAction_assign(action_index, flow_entry_p, flow_p)) != SDN_HAL_RETURN_OK)
                    return ret;
                break;
            default:
                break;
        }
    }

    return SDN_HAL_RETURN_OK;
}

int32 _sdn_hal_flow_ft0_instruction_fill(uint32 ins_type, sdn_hal_rule_flow_mgmt_t *flow_entry_p, sdn_hal_flow_t *flow_p)
{
    SDN_PARAM_CHK(flow_entry_p == NULL, SDN_HAL_RETURN_OK);
    SDN_PARAM_CHK(flow_p == NULL, SDN_HAL_RETURN_OK);

    DBG_SDN("[%s-%d] table 0 instruction(%d)\r\n", __FUNCTION__, __LINE__, ins_type);

    /* insert ingress action*/

    switch(ins_type)
    {
        case EN_OFPIT_GOTO_TABLE:
        {
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.gotoTbl_en = ENABLED;
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.gt_data.tbl_id = flow_entry_p->goto_table_id;
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.gt_data.type = OF_GOTOTBL_TYPE_NORMAL;
            break;
        }
        case EN_OFPIT_WRITE_METADATA:
        {
            if(SDN_HAL_IS_VALID_METADATA(flow_entry_p->metadata) && SDN_HAL_IS_VALID_METADATA(flow_entry_p->metadata_mask))
            {
                flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.writeMetadata_en = ENABLED;
                flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wm_data.data = flow_entry_p->metadata;
                flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.wm_data.mask = flow_entry_p->metadata_mask;
            }
            else
                return SDN_HAL_RETURN_INVALID_METADATA;
            break;
        }
        case EN_OFPIT_WRITE_ACTIONS:
        case EN_OFPIT_APPLY_ACTIONS:
        {
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.writeAct_en = ENABLED;
            break;
        }
        case EN_OFPIT_CLEAR_ACTIONS:
        {
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.clearAct_en = ENABLED;
            break;
        }
        case EN_OFPIT_METER:
        {
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.meter_en = ENABLED;
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.meter_data.meter_id = SDN_HAL_METER_INDEX(flow_entry_p->meter_id);
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.meter_data.red = OF_METER_TYPE_DROP;
            flow_p->ft0_flow.ft0_3_4_ins.igrFt_0.meter_data.yellow = OF_METER_TYPE_DROP;
            break;
        }
        default:
            return SDN_HAL_RETURN_UNSUPPORTED_INS;

    }

    return SDN_HAL_RETURN_OK;
}

int32 _sdn_hal_flow_ft1_instruction_fill(uint32 ins_type, sdn_hal_rule_flow_mgmt_t *flow_entry_p, sdn_hal_flow_t *flow_p)
{
    SDN_PARAM_CHK(flow_entry_p == NULL, SDN_HAL_RETURN_OK);
    SDN_PARAM_CHK(flow_p == NULL, SDN_HAL_RETURN_OK);

    DBG_SDN("[%s-%d] table 1 instruction(%d)\r\n", __FUNCTION__, __LINE__, ins_type);

    /* insert ingress action*/

    switch(ins_type)
    {
        case EN_OFPIT_GOTO_TABLE:
        {
            flow_p->ft1_flow.ins.gotoTbl_en = ENABLED;
            flow_p->ft1_flow.ins.gt_data.tbl_id = flow_entry_p->goto_table_id;
            flow_p->ft1_flow.ins.gt_data.type = OF_GOTOTBL_TYPE_NORMAL;
            break;
        }
        case EN_OFPIT_WRITE_METADATA:
        {
            if(SDN_HAL_IS_VALID_METADATA(flow_entry_p->metadata) && SDN_HAL_IS_VALID_METADATA(flow_entry_p->metadata_mask))
            {
                flow_p->ft1_flow.ins.writeMetadata_en = ENABLED;
                flow_p->ft1_flow.ins.wm_data.data = flow_entry_p->metadata;
                flow_p->ft1_flow.ins.wm_data.mask = flow_entry_p->metadata_mask;
            }
            else
                return SDN_HAL_RETURN_INVALID_METADATA;
            break;
        }
        case EN_OFPIT_WRITE_ACTIONS:
        {
            flow_p->ft1_flow.ins.writeAct_en = ENABLED;
            break;
        }
        case EN_OFPIT_CLEAR_ACTIONS:
        {
            flow_p->ft1_flow.ins.clearAct_en = ENABLED;
            break;
        }
        case EN_OFPIT_METER:
        case EN_OFPIT_APPLY_ACTIONS:
        default:
            return SDN_HAL_RETURN_UNSUPPORTED_INS;
    }

    return SDN_HAL_RETURN_OK;

}

int32 _sdn_hal_flow_ft2_hash_instruction_fill(uint32 ins_type, sdn_hal_rule_flow_mgmt_t *flow_entry_p, sdn_hal_flow_t *flow_p)
{
    DBG_SDN("[%s-%d] table 2 hash instruction(%d)\r\n", __FUNCTION__, __LINE__, ins_type);

    /* insert ingress action*/
    switch(ins_type)
    {
        case EN_OFPIT_GOTO_TABLE:
        {
            flow_p->ft2_flow.ft2_hash_flow.ins.gotoTbl_en = ENABLED;
            flow_p->ft2_flow.ft2_hash_flow.ins.gt_data.tbl_id = flow_entry_p->goto_table_id;
            flow_p->ft2_flow.ft2_hash_flow.ins.gt_data.type = OF_GOTOTBL_TYPE_NORMAL;
            break;
        }
        case EN_OFPIT_WRITE_METADATA:
        {
            if(SDN_HAL_IS_VALID_METADATA(flow_entry_p->metadata) && SDN_HAL_IS_VALID_METADATA(flow_entry_p->metadata_mask))
            {
                flow_p->ft2_flow.ft2_hash_flow.ins.writeMetadata_en = ENABLED;
                flow_p->ft2_flow.ft2_hash_flow.ins.wm_data.data = flow_entry_p->metadata;
                flow_p->ft2_flow.ft2_hash_flow.ins.wm_data.mask = flow_entry_p->metadata_mask;
            }
            else
                return SDN_HAL_RETURN_INVALID_METADATA;
            break;
        }
        case EN_OFPIT_WRITE_ACTIONS:
        {
            flow_p->ft2_flow.ft2_hash_flow.ins.writeAct_en = ENABLED;
            break;
        }
        case EN_OFPIT_CLEAR_ACTIONS:
        {
            flow_p->ft2_flow.ft2_hash_flow.ins.clearAct_en = ENABLED;
            break;
        }
        case EN_OFPIT_APPLY_ACTIONS:
        case EN_OFPIT_METER:
        default:
            return SDN_HAL_RETURN_UNSUPPORTED_INS;

    }

    return SDN_HAL_RETURN_OK;
}

int32 _sdn_hal_flow_ft2_cam_instruction_fill(uint32 ins_type, sdn_hal_rule_flow_mgmt_t *flow_entry_p, sdn_hal_flow_t *flow_p)
{
    DBG_SDN("[%s-%d] table 2 cam instruction(%d)\r\n", __FUNCTION__, __LINE__, ins_type);

    /* insert ingress action*/
    switch(ins_type)
    {
        case EN_OFPIT_GOTO_TABLE:
        {
            flow_p->ft2_flow.ft2_cam_flow.ins.gotoTbl_en = ENABLED;
            flow_p->ft2_flow.ft2_cam_flow.ins.gt_data.tbl_id = flow_entry_p->goto_table_id;
            flow_p->ft2_flow.ft2_cam_flow.ins.gt_data.type = OF_GOTOTBL_TYPE_NORMAL;
            break;
        }
        case EN_OFPIT_WRITE_METADATA:
        {
            if(SDN_HAL_IS_VALID_METADATA(flow_entry_p->metadata) && SDN_HAL_IS_VALID_METADATA(flow_entry_p->metadata_mask))
            {
                flow_p->ft2_flow.ft2_cam_flow.ins.writeMetadata_en = ENABLED;
                flow_p->ft2_flow.ft2_cam_flow.ins.wm_data.data = flow_entry_p->metadata;
                flow_p->ft2_flow.ft2_cam_flow.ins.wm_data.mask = flow_entry_p->metadata_mask;
            }
            else
                return SDN_HAL_RETURN_INVALID_METADATA;
            break;
        }
        case EN_OFPIT_WRITE_ACTIONS:
        {
            flow_p->ft2_flow.ft2_cam_flow.ins.writeAct_en = ENABLED;
            break;
        }
        case EN_OFPIT_CLEAR_ACTIONS:
        {
            flow_p->ft2_flow.ft2_cam_flow.ins.clearAct_en = ENABLED;
            break;
        }
        case EN_OFPIT_APPLY_ACTIONS:
        case EN_OFPIT_METER:
        default:
            return SDN_HAL_RETURN_UNSUPPORTED_INS;

    }

    return SDN_HAL_RETURN_OK;
}

int32 _sdn_hal_flow_ft2_instruction_fill(uint32 ins_type, sdn_hal_rule_flow_mgmt_t *flow_entry_p, sdn_hal_flow_t *flow_p)
{
    SDN_PARAM_CHK(flow_entry_p == NULL, SDN_HAL_RETURN_OK);
    SDN_PARAM_CHK(flow_p == NULL, SDN_HAL_RETURN_OK);
    DBG_SDN("[%s-%d] table 2 instruction(%d)\r\n", __FUNCTION__, __LINE__, ins_type);

    if(_sdn_hal_l3Cam_is_used(flow_entry_p))
    {
        return _sdn_hal_flow_ft2_cam_instruction_fill(ins_type, flow_entry_p, flow_p);
    }
    else
    {
        return _sdn_hal_flow_ft2_hash_instruction_fill(ins_type, flow_entry_p, flow_p);
    }
}

int32 _sdn_hal_flow_ft3_instruction_fill(uint32 ins_type, sdn_hal_rule_flow_mgmt_t *flow_entry_p, sdn_hal_flow_t *flow_p)
{
    SDN_PARAM_CHK(flow_entry_p == NULL, SDN_HAL_RETURN_OK);
    SDN_PARAM_CHK(flow_p == NULL, SDN_HAL_RETURN_OK);

    DBG_SDN("[%s-%d] table 3 instruction(%d)\r\n", __FUNCTION__, __LINE__, ins_type);

    /* insert ingress action*/
    switch(ins_type)
    {
        case EN_OFPIT_GOTO_TABLE:
        {
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.gotoTbl_en = ENABLED;
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.gt_data.tbl_id = flow_entry_p->goto_table_id;
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.gt_data.type = OF_GOTOTBL_TYPE_NORMAL;
            break;
        }
        case EN_OFPIT_WRITE_METADATA:
        {
            if(SDN_HAL_IS_VALID_METADATA(flow_entry_p->metadata) && SDN_HAL_IS_VALID_METADATA(flow_entry_p->metadata_mask))
            {
                flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.writeMetadata_en = ENABLED;
                flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wm_data.data = flow_entry_p->metadata;
                flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.wm_data.mask = flow_entry_p->metadata_mask;
            }
            else
                return SDN_HAL_RETURN_INVALID_METADATA;
            break;
        }
        case EN_OFPIT_WRITE_ACTIONS:
        {
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.writeAct_en = ENABLED;
            break;
        }
        case EN_OFPIT_CLEAR_ACTIONS:
        {
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.clearAct_en = ENABLED;
            break;
        }
        case EN_OFPIT_METER:
        {
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.meter_en = ENABLED;
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.meter_data.meter_id = SDN_HAL_METER_INDEX(flow_entry_p->meter_id);
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.meter_data.red = OF_METER_TYPE_DROP;
            flow_p->ft3_flow.ft0_3_4_ins.igrFt_3.meter_data.yellow = OF_METER_TYPE_DROP;
            break;
        }
        case EN_OFPIT_APPLY_ACTIONS:
        default:
            return SDN_HAL_RETURN_UNSUPPORTED_INS;

    }

    return SDN_HAL_RETURN_OK;
}

int32 _sdn_hal_flow_ft4_instruction_fill(uint32 ins_type, sdn_hal_rule_flow_mgmt_t *flow_entry_p, sdn_hal_flow_t *flow_p)
{
    SDN_PARAM_CHK(flow_entry_p == NULL, SDN_HAL_RETURN_OK);
    SDN_PARAM_CHK(flow_p == NULL, SDN_HAL_RETURN_OK);

    DBG_SDN("[%s-%d] table 4 instruction(%d)\r\n", __FUNCTION__, __LINE__, ins_type);

    /* insert ingress action*/
    switch(ins_type)
    {
        case EN_OFPIT_WRITE_ACTIONS:
        {
            flow_p->ft4_flow.ft0_3_4_ins.egrFt_0.writeAct_en = ENABLED;
            break;
        }
        case EN_OFPIT_METER:
        {
            flow_p->ft4_flow.ft0_3_4_ins.egrFt_0.meter_en = ENABLED;
            flow_p->ft4_flow.ft0_3_4_ins.egrFt_0.meter_data.meter_id = SDN_HAL_METER_INDEX(flow_entry_p->meter_id);
            flow_p->ft4_flow.ft0_3_4_ins.egrFt_0.meter_data.red = OF_METER_TYPE_DROP;
            flow_p->ft4_flow.ft0_3_4_ins.egrFt_0.meter_data.yellow = OF_METER_TYPE_DROP;
            break;
        }
        case EN_OFPIT_APPLY_ACTIONS:
        default:
            return SDN_HAL_RETURN_UNSUPPORTED_INS;
    }

    return SDN_HAL_RETURN_OK;
}


int32 sdn_hal_flow_instruction_fill(rtk_of_flowtable_phase_t phase, sdn_hal_rule_flow_mgmt_t *flow_entry_p, sdn_hal_flow_t *flow_p)
{
    uint32 ins_type;
    uint32 ret = SDN_HAL_RETURN_FAILED;
    SDN_PARAM_CHK(flow_entry_p == NULL, SDN_HAL_RETURN_NULL_DATA);
    SDN_PARAM_CHK(flow_p == NULL, SDN_HAL_RETURN_NULL_DATA);

    DBG_SDN("[%s-%d] ins_type bitmask(%x)\r\n", __FUNCTION__, __LINE__, flow_entry_p->instruction_mask);

    for (ins_type = 0; ins_type < EN_OFPIT_MAX; ins_type++)
    {
        if (!HAL_LIB_CHK_BIT_ON(flow_entry_p->instruction_mask, ins_type))
        {
            continue;
        }

        switch(phase)
        {
            case FT_PHASE_IGR_FT_0:
                DBG_SDN("[%s-%d] table0 instruction fill\r\n", __FUNCTION__, __LINE__);
                if((ret = _sdn_hal_flow_ft0_instruction_fill(ins_type, flow_entry_p, flow_p)) != SDN_HAL_RETURN_OK)
                    return ret;
                break;
            case FT_PHASE_IGR_FT_1:
                DBG_SDN("[%s-%d] table1 instruction fill\r\n", __FUNCTION__, __LINE__);
                if((ret =_sdn_hal_flow_ft1_instruction_fill(ins_type, flow_entry_p, flow_p)) != SDN_HAL_RETURN_OK)
                    return ret;
                break;
            case FT_PHASE_IGR_FT_2:
                DBG_SDN("[%s-%d] table2 instruction fill\r\n", __FUNCTION__, __LINE__);
                if((ret =_sdn_hal_flow_ft2_instruction_fill(ins_type, flow_entry_p, flow_p)) != SDN_HAL_RETURN_OK)
                    return ret;
                break;
            case FT_PHASE_IGR_FT_3:
                DBG_SDN("[%s-%d] table3 instruction fill\r\n", __FUNCTION__, __LINE__);
                if((ret =_sdn_hal_flow_ft3_instruction_fill(ins_type, flow_entry_p, flow_p)) != SDN_HAL_RETURN_OK)
                    return ret;
                break;
            case FT_PHASE_EGR_FT_0:
                DBG_SDN("[%s-%d] table4 instruction fill\r\n", __FUNCTION__, __LINE__);
                if((ret =_sdn_hal_flow_ft4_instruction_fill(ins_type, flow_entry_p, flow_p)) != SDN_HAL_RETURN_OK)
                    return ret;
                break;
            default:
                break;
        }
    }
    return SDN_HAL_RETURN_OK;
}
int32 sdn_hal_flow_matchField_fill(rtk_of_flowtable_phase_t phase, sdn_hal_rule_flow_mgmt_t *flow_entry_p, sdn_hal_flow_t *flow)
{
    uint32 ret = SDN_HAL_RETURN_FAILED;
    SDN_PARAM_CHK(flow == NULL, SDN_HAL_RETURN_NULL_DATA);
#ifdef CONFIG_FLOW_AG
    SDN_PARAM_CHK((FT_PHASE_IGR_FT_0 == phase || FT_PHASE_IGR_FT_3 == phase) && flow_entry_p->flow_ag == FALSE, SDN_HAL_RETURN_FAILED);
#endif

    DBG_SDN("[%s-%d] action bitmask(%x)\r\n", __FUNCTION__, __LINE__, flow_entry_p->action_data.action_mask);

    switch(phase)
    {
        case FT_PHASE_IGR_FT_0:
            DBG_SDN("[%s-%d] table0 match fill\r\n", __FUNCTION__, __LINE__);
            if((ret = _sdn_hal_flow_ft0_matchField_fill(flow_entry_p, flow)) != SDN_HAL_RETURN_OK)
                return ret;
            break;
        case FT_PHASE_IGR_FT_1:
            DBG_SDN("[%s-%d] table1 match fill\r\n", __FUNCTION__, __LINE__);
            if((ret = _sdn_hal_flow_ft1_matchField_fill(flow_entry_p, flow)) != SDN_HAL_RETURN_OK)
                return ret;
            break;
        case FT_PHASE_IGR_FT_2:
            DBG_SDN("[%s-%d] table2 match fill\r\n", __FUNCTION__, __LINE__);
            if((ret =_sdn_hal_flow_ft2_matchField_fill(flow_entry_p, flow)) != SDN_HAL_RETURN_OK)
                return ret;
            break;
        case FT_PHASE_IGR_FT_3:
            DBG_SDN("[%s-%d] table3 match fill\r\n", __FUNCTION__, __LINE__);
            DBG_SDN("\n[%s][%d]\n",__FUNCTION__,__LINE__);
            if((ret =_sdn_hal_flow_ft3_matchField_fill(flow_entry_p, flow)) != SDN_HAL_RETURN_OK)
                return ret;
            break;
        case FT_PHASE_EGR_FT_0:
            DBG_SDN("[%s-%d] table4 match fill\r\n", __FUNCTION__, __LINE__);
            if((ret =_sdn_hal_flow_ft4_matchField_fill(flow_entry_p, flow)) != SDN_HAL_RETURN_OK)
                return ret;
            break;
        default:
            break;
    }
    return SDN_HAL_RETURN_OK;
}
int32 sdn_hal_flow_set(rtk_of_flowtable_phase_t phase, sdn_hal_rule_flow_mgmt_t *flow_entry_p, sdn_hal_flow_t *flow)
{
    int32 ret = RT_ERR_FAILED;
    uint32 unit = UNIT_ID_0;
    uint32 idx = 0;
    uint32 j = 0;
    uint32 flow_id;
    uint32 l3_physical_id;
    uint32 l3_logical_id;
    rtk_of_flowMove_t l3_cam_move;
    switch(phase)
    {
        case FT_PHASE_IGR_FT_0:
        {
            DBG_SDN("[%s-%d] table0 flow set\r\n", __FUNCTION__, __LINE__);
#ifndef CONFIG_FLOW_AG
            flow_id = flow_entry_p->flow_id;
#else
    rtk_of_flowOperation_t operation;
            for(j = 0; j < SDN_HAL_MAX_NUM_BLOCK_TEMPLATE; j++)
            {
                flow_id = flow_entry_p->flow_id + j;
                memset(&operation, 0, sizeof(rtk_of_flowOperation_t));
                operation.aggr_1 = ENABLED;
                rtk_of_flowEntryOperation_set(unit, phase, flow_id, &operation);
#endif
                if(RT_ERR_OK != rtk_of_flowEntryInstruction_set(unit, phase, flow_id, &flow->ft0_flow.ft0_3_4_ins))
                    return SDN_HAL_RETURN_FAILED;
                ret = rtk_of_flowEntryValidate_set(unit, phase, flow_id, ENABLED);
#ifndef CONFIG_FLOW_AG

#else
            }
#endif
            break;
        }
        case FT_PHASE_IGR_FT_1:
        {
            DBG_SDN("[%s-%d] table1 flow set\r\n", __FUNCTION__, __LINE__);
            ret = rtk_of_l2FlowEntry_add(unit, &flow->ft1_flow);
            break;
        }
        case FT_PHASE_IGR_FT_2:
        {
            DBG_SDN("[%s-%d] table2 flow set\r\n", __FUNCTION__, __LINE__);

            ret = _sdn_hal_physicalId_to_logicalId(phase, flow_entry_p->flow_id, &l3_logical_id);
            if(ret != SDN_HAL_RETURN_OK)
                return RT_ERR_FAILED;

            if(_sdn_hal_l3Cam_is_used(flow_entry_p))
            {
                SDN_PARAM_CHK(l3_cam_flow_count >= SDN_HAL_INGRESS_L3_TABLE2_CAM_RULE_NUM, SDN_HAL_RETURN_FAILED);
                memset(&l3_cam_move, 0, sizeof(rtk_of_flowMove_t));
                /*Add flow at the end of exist flows*/
                if (l3_cam_flow_count == 0
                    || ((l3_cam_flow_count > 0) && (l3_logical_id > l3_cam_logic_id[l3_cam_flow_count-1])))
                {
                    l3_cam_logic_id[l3_cam_flow_count] = l3_logical_id;
                }
                else
                {
                    /*Compare logical flow index to find an appropriate position.*/
                    for(idx = 0; idx < l3_cam_flow_count; idx++)
                    {
                        if(l3_logical_id < l3_cam_logic_id[idx])
                        {
                            /*physical flow entry move.*/
                            l3_cam_move.length = (l3_cam_flow_count - idx) * SDN_HAL_INGRESS_L3_DIV_FACTOR;
                            l3_cam_move.move_from = idx * SDN_HAL_INGRESS_L3_DIV_FACTOR;
                            l3_cam_move.move_to = (idx + 1) * SDN_HAL_INGRESS_L3_DIV_FACTOR;
                            if (RT_ERR_OK != rtk_of_l3CamFlowEntry_move(unit, &l3_cam_move))
                            {
                                return SDN_HAL_RETURN_FAILED;
                            }

                            /*move physical id position*/
                            for(j = l3_cam_flow_count; j > idx ; j--)
                            {
                                l3_cam_logic_id[j] = l3_cam_logic_id[j - 1];
                            }

                            l3_cam_logic_id[idx] = l3_logical_id;
                            break;
                        }
                    }
                }
                _sdn_hal_logicalId_to_physicalId(FT_PHASE_IGR_FT_2, idx, &l3_physical_id);
                SDN_HAL_MSG("\n[%s][%d] rtk_of_l3CamFlowEntry_add()\n",__FUNCTION__,__LINE__);
                ret = rtk_of_l3CamFlowEntry_add(unit, flow_entry_p->flow_id, &flow->ft2_flow.ft2_cam_flow);
                if (ret == RT_ERR_OK)
                {
                    l3_cam_flow_count++;
                }
                else
                {
                    SDN_HAL_MSG("\n[%s][%d] rtk_of_l3CamFlowEntry_add() ret = 0x%08x\n",__FUNCTION__,__LINE__,ret);
                    /*Add entry failed, database move back.*/
                    for(j = idx; j < l3_cam_flow_count; j++)
                    {
                        l3_cam_logic_id[j] = l3_cam_logic_id[j + 1];
                    }
                }
            }
            else
            {
                SDN_HAL_MSG("\n[%s][%d] rtk_of_l3HashFlowEntry_add\n",__FUNCTION__,__LINE__);
                ret = rtk_of_l3HashFlowEntry_add(unit, &flow->ft2_flow.ft2_hash_flow);
            }

            break;
        }
        case FT_PHASE_IGR_FT_3:
        {
#ifndef CONFIG_FLOW_AG
            flow_id = flow_entry_p->flow_id;
#else
            rtk_of_flowOperation_t operation;
            for(j = 0; j < SDN_HAL_MAX_NUM_BLOCK_TEMPLATE; j++)
            {
                flow_id = flow_entry_p->flow_id + j;
                memset(&operation, 0, sizeof(rtk_of_flowOperation_t));
                operation.aggr_1 = ENABLED;
                rtk_of_flowEntryOperation_set(unit, phase, flow_id, &operation);
#endif
                DBG_SDN("[%s-%d] table3 flow set\r\n", __FUNCTION__, __LINE__);

                if(RT_ERR_OK != rtk_of_flowEntryInstruction_set(unit, phase, flow_id, &flow->ft3_flow.ft0_3_4_ins))
                    return SDN_HAL_RETURN_FAILED;

                ret = rtk_of_flowEntryValidate_set(unit, phase, flow_id, ENABLED);

#ifndef CONFIG_FLOW_AG

#else
            }
#endif
            break;
        }
        case FT_PHASE_EGR_FT_0:
        {
            if(RT_ERR_OK != rtk_of_flowEntryInstruction_set(unit, phase, flow_entry_p->flow_id, &flow->ft4_flow.ft0_3_4_ins))
                return SDN_HAL_RETURN_FAILED;
            DBG_SDN("[%s-%d] table4 flow set\r\n", __FUNCTION__, __LINE__);
            ret = rtk_of_flowEntryValidate_set(unit, FT_PHASE_EGR_FT_0, flow_entry_p->flow_id, ENABLED);
            break;
        }
        default:
            break;
    }

    if (ret != RT_ERR_OK)
    {
        DBG_SDN("[%s-%d] flow set failed.\r\n", __FUNCTION__, __LINE__);
        return SDN_HAL_RETURN_FAILED;
    }
    return SDN_HAL_RETURN_OK;
}
int32 _meter_transform(sdn_hal_rule_meter_mgmt_t *meter_data, const sdn_db_meter_mod_t *meter)
{
    meter_data->meter_id = SDN_HAL_METER_INDEX(meter->meter_id);
    if(meter->n_bands == 0)
    {
        return SDN_HAL_RETURN_INVALID_PARAM;
    }

    if ((meter->flags & EN_OFPMF_BURST) && meter->n_bands)
        meter_data->meter_burst_size = meter->bands[0].burst_size;
    else
        meter_data->meter_burst_size = 0;


    if (meter->flags & EN_OFPMF_KBPS)
    {
        meter_data->meter_flags = SDN_HAL_RULE_METER_FLAG_KBPS;
    }
    else if(meter->flags & EN_OFPMF_PKTPS)
    {
        meter_data->meter_flags = SDN_HAL_RULE_METER_FLAG_PKTPS;
    }
    else
    {
        /* Others are unsupported */
        return SDN_HAL_RETURN_NOT_SUPPORTED;
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

    return SDN_HAL_RETURN_OK;
}

int32 sdn_hal_meter_entry_add(const sdn_db_meter_mod_t *meter_entry)
{
    uint32 ret = SDN_HAL_RETURN_FAILED;
    uint32 unit = SDN_HAL_MY_UNIT_ID();
    sdn_hal_rule_meter_mgmt_t meter_data;
    rtk_pie_meterEntry_t meter;

    if (meter_entry->meter_id >= SDN_HAL_RTK_METER_SIZE)
        return SDN_HAL_RETURN_FAILED;

    if(SDN_HAL_RETURN_OK != _meter_transform(&meter_data, meter_entry))
        return SDN_HAL_RETURN_FAILED;

    if (meter_data.meter_band_type != SDN_HAL_RULE_METER_BAND_DROP)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    memset(&meter, 0x0, sizeof(rtk_pie_meterEntry_t));

    meter.type = METER_TYPE_DLB;
    meter.color_aware    = 0;
    meter.mode = meter_data.meter_flags;
    if (meter_data.meter_flags == SDN_HAL_RULE_METER_FLAG_KBPS)
    {
        meter.mode = METER_MODE_BYTE;
        meter.lb0_rate = HAL_METER_ROUND_TO_DEVIDE_BY_16(meter_data.meter_rate);         /* rate granularity is 16Kbps, or 1pps */
        meter.lb1_rate = HAL_METER_ROUND_TO_DEVIDE_BY_16(meter_data.meter_rate);         /* rate granularity is 16Kbps, or 1pps */
        meter.lb0_bs = HAL_METER_ROUND_TO_DEVIDE_BY_16(meter_data.meter_burst_size);     /* rate granularity is 16Kbps, or 1pps */
        meter.lb1_bs = HAL_METER_ROUND_TO_DEVIDE_BY_16(meter_data.meter_burst_size);     /* rate granularity is 16Kbps, or 1pps */
    }
    else
    {
        meter.mode = METER_MODE_PACKET;
        meter.lb0_rate = meter_data.meter_rate;         /* rate granularity is 16Kbps, or 1pps */
        meter.lb1_rate = meter_data.meter_rate;         /* rate granularity is 16Kbps, or 1pps */
        meter.lb0_bs = meter_data.meter_burst_size;     /* rate granularity is 16Kbps, or 1pps */
        meter.lb1_bs = meter_data.meter_burst_size;     /* rate granularity is 16Kbps, or 1pps */
    }
    ret = rtk_pie_meterEntry_set(unit, meter_data.meter_id, &meter);

    if (ret != RT_ERR_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    return SDN_HAL_RETURN_OK;

}

int32 sdn_hal_meter_entry_del(uint32 meter_id)
{
    uint32 ret = SDN_HAL_RETURN_FAILED;
    uint32 unit = 0;
    rtk_pie_meterEntry_t meter;

    unit = SDN_HAL_MY_UNIT_ID();

    if (meter_id < SDN_HAL_RTK_METER_SIZE)
    {
        memset(&meter, 0x0, sizeof(rtk_pie_meterEntry_t));
        ret = rtk_pie_meterEntry_set(unit, SDN_HAL_METER_INDEX(meter_id), &meter);

        if (ret != RT_ERR_OK)
        {
            return SDN_HAL_RETURN_FAILED;
        }
    }

    return SDN_HAL_RETURN_OK;
}

int32 sdn_hal_meter_entry_modify(const sdn_db_meter_mod_t *meter_entry)
{
    uint32 ret = SDN_HAL_RETURN_FAILED;
    uint32 unit = SDN_HAL_MY_UNIT_ID();
    sdn_hal_rule_meter_mgmt_t meter_data;
    rtk_pie_meterEntry_t meter;

    if (meter_entry->meter_id >= SDN_HAL_RTK_METER_SIZE)
        return SDN_HAL_RETURN_FAILED;

    if(SDN_HAL_RETURN_OK != _meter_transform(&meter_data, meter_entry))
        return SDN_HAL_RETURN_FAILED;

    if (meter_data.meter_band_type != SDN_HAL_RULE_METER_BAND_DROP)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    memset(&meter, 0x0, sizeof(rtk_pie_meterEntry_t));

    if ((ret = rtk_pie_meterEntry_get(unit, meter_data.meter_id, &meter)) == RT_ERR_OK)
    {
        meter.type = METER_TYPE_DLB;
        meter.color_aware = 0;
        if (meter_data.meter_flags == SDN_HAL_RULE_METER_FLAG_KBPS)
        {
            meter.mode = METER_MODE_BYTE;
            meter.lb0_rate = HAL_METER_ROUND_TO_DEVIDE_BY_16(meter_data.meter_rate);    /* rate granularity is 16Kbps, or 1pps */
            meter.lb1_rate = HAL_METER_ROUND_TO_DEVIDE_BY_16(meter_data.meter_rate);    /* rate granularity is 16Kbps, or 1pps */
            meter.lb0_bs = HAL_METER_ROUND_TO_DEVIDE_BY_16(meter_data.meter_burst_size);     /* rate granularity is 16Kbps, or 1pps */
            meter.lb1_bs = HAL_METER_ROUND_TO_DEVIDE_BY_16(meter_data.meter_burst_size);     /* rate granularity is 16Kbps, or 1pps */
        }
        else
        {
            meter.mode = METER_MODE_PACKET;
            meter.lb0_rate = meter_data.meter_rate;         /* rate granularity is 16Kbps, or 1pps */
            meter.lb1_rate = meter_data.meter_rate;         /* rate granularity is 16Kbps, or 1pps */
            meter.lb0_bs = meter_data.meter_burst_size;     /* rate granularity is 16Kbps, or 1pps */
            meter.lb1_bs = meter_data.meter_burst_size;     /* rate granularity is 16Kbps, or 1pps */
        }
        ret = rtk_pie_meterEntry_set(unit, meter_data.meter_id, &meter);
        if (ret != RT_ERR_OK)
        {
            return SDN_HAL_RETURN_FAILED;
        }
    }

    return SDN_HAL_RETURN_OK;
}

int32 sdn_hal_meter_table_features_get(sdn_db_meter_table_features_t *feature)
{
    feature->max_meters = SDN_HAL_RTK_METER_SIZE;
    feature->max_bands  = 1;
    SDN_HAL_LIB_BIT_ON(feature->band_types, EN_OFPMBT_DROP);
    SDN_HAL_LIB_BIT_ON(feature->band_types, EN_OFPMBT_DSCP_REMARK);
    feature->max_colors = 0;
    SDN_HAL_LIB_BIT_ON(feature->capabilities, SDN_HAL_RULE_METER_FLAG_KBPS);
    SDN_HAL_LIB_BIT_ON(feature->capabilities, SDN_HAL_RULE_METER_FLAG_PKTPS);
    SDN_HAL_LIB_BIT_ON(feature->capabilities, SDN_HAL_RULE_METER_BURST);

    return SDN_HAL_RETURN_OK;
}

int32 sdn_hal_table_miss_action_set(uint8_t table_id, rtk_of_tblMissAct_t action)
{
    uint32 unit = SDN_HAL_MY_UNIT_ID();
    rtk_of_tblMissAct_t old_action = OF_TBLMISS_ACT_END;
    rtk_of_flowtable_phase_t phase;

    phase = sdn_hal_rule_get_flow_table(table_id);

    if (rtk_of_tblMissAction_get(unit, phase, &old_action) != RT_ERR_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    if (action == old_action)
    {
        return SDN_HAL_RETURN_OK;
    }

    if (rtk_of_tblMissAction_set(unit, phase, action) != RT_ERR_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    return SDN_HAL_RETURN_OK;
}

int32 sdn_hal_flow_hitstatus_get(uint8_t table_id, uint32_t flow_id, uint32_t *hit_status_p)
{
    uint32 unit = SDN_HAL_MY_UNIT_ID();
    rtk_of_flowtable_phase_t phase;

    phase = sdn_hal_rule_get_flow_table(table_id);
    SDN_PARAM_CHK(hit_status_p == NULL, SDN_HAL_RETURN_FAILED);

    if (RT_ERR_OK != rtk_of_flowEntryHitSts_get(unit, phase, flow_id, TRUE, hit_status_p))
    {
        return SDN_HAL_RETURN_FAILED;
    }

    return SDN_HAL_RETURN_OK;
}

int32 sdn_hal_l2andl3flowentry_hitstatus_get(sdn_db_flow_entry_t *flow_entry_p, uint32_t *hit_status_p)
{
    uint32 unit = SDN_HAL_MY_UNIT_ID();
    uint32 l3cam_idx = 0;
    uint32 l3_physical_id = 0;
    rtk_of_flowtable_phase_t phase;
    sdn_hal_flow_t flow;
    sdn_hal_rule_flow_mgmt_t flow_data;

    if (flow_entry_p == NULL || hit_status_p == NULL)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    memset(&flow_data, 0, sizeof(sdn_hal_rule_flow_mgmt_t));
    memset(&flow, 0, sizeof(sdn_hal_flow_t));

    match_transform(&flow_data, flow_entry_p);
    inst_transform(&flow_data, flow_entry_p);

    flow_data.flow_id = flow_entry_p->flow_index;
    flow_data.table_id = flow_entry_p->table_id;
    unit = SDN_HAL_MY_UNIT_ID();
    phase = sdn_hal_rule_get_flow_table(flow_entry_p->table_id);

    if (SDN_HAL_RETURN_OK != sdn_hal_flow_matchField_fill(phase, &flow_data, &flow))
    {
        return SDN_HAL_RETURN_FAILED;
    }

    switch (phase)
    {
        case FT_PHASE_IGR_FT_1:
            if (rtk_of_l2FlowEntryHitSts_get(unit, &flow.ft1_flow, TRUE, hit_status_p) != RT_ERR_OK)
            {
                return SDN_HAL_RETURN_FAILED;
            }
        break;

        case FT_PHASE_IGR_FT_2:
           if(_sdn_hal_l3Cam_is_used(&flow_data))
           {
                for( l3cam_idx = 0; l3cam_idx < l3_cam_flow_count; l3cam_idx++)
                {
                   if (l3_cam_logic_id[l3cam_idx] == flow_data.flow_id)
                   {
                       break;
                   }
                }
                if (l3cam_idx == l3_cam_flow_count)
                {
                    return SDN_HAL_RETURN_FAILED;
                }

                _sdn_hal_logicalId_to_physicalId(FT_PHASE_IGR_FT_2, l3cam_idx, &l3_physical_id);

               //if (rtk_of_l3CamflowEntryHitSts_get((unit, &flow.ft1_flow, , hit_status_p) != RT_ERR_OK))
               if (rtk_of_l3CamflowEntryHitSts_get(unit, l3_physical_id, TRUE, hit_status_p) != RT_ERR_OK)
               {
                   return SDN_HAL_RETURN_FAILED;
               }
           }
           else
           {
               if (rtk_of_l3HashflowEntryHitSts_get(unit, &flow.ft2_flow.ft2_hash_flow, TRUE, hit_status_p) != RT_ERR_OK)
               {
                   return SDN_HAL_RETURN_FAILED;
               }
           }
        break;

        default:
            return SDN_HAL_RETURN_FAILED;
    }
    return SDN_HAL_RETURN_OK;
}
//new group code
/*
    rtk_enable_t    igr_port;
    rtk_enable_t    smac;
    rtk_enable_t    dmac;
    rtk_enable_t    sip;
    rtk_enable_t    dip;
    rtk_enable_t    l4_sport;
    rtk_enable_t    l4_dport;
  rtk_of_groupTblHashPara_t;
*/
static int32 sdn_hal_convert_bucket_to_hal(sdn_db_group_action_bucket_t *bucket_data_p, rtk_of_actionBucket_t *bucket_entry_p, rtk_of_groupTblHashPara_t *group_tb_hash_profile_p)
{
    uint32_t action_id      = 0;
    uint32_t total_port_num = 0;
    uint32_t port_id        = 0;
    uint32_t phy_port_id    = 0;
    rtk_portmask_t phy_port_list;
    //sdn_db_bucket_data_t *action_p = NULL;

    if (bucket_data_p == NULL || bucket_entry_p == NULL || group_tb_hash_profile_p == NULL)
    {
        DBG_SDN("\r\n[%s:%d] null pointer\r\n", __FUNCTION__, __LINE__);
        return SDN_HAL_RETURN_FAILED;
    }
    memset(&phy_port_list, 0, sizeof(phy_port_list));

    DBG_SDN("\r\n[%s:%d] total action_num(%d)\r\n", __FUNCTION__, __LINE__, bucket_data_p->action_len);
    if (bucket_data_p->actions_p)
    {
        for (action_id = 0; action_id < bucket_data_p->action_len; action_id++)
        {
            uint32_t act_type = bucket_data_p->actions_p[action_id].act_type;

            DBG_SDN("\r\n[%s:%d] total action_num(%d) id(%d) action_type(%d)\r\n", __FUNCTION__, __LINE__,  bucket_data_p->action_len, action_id, bucket_data_p->actions_p[action_id].act_type);
            switch (act_type)
            {
                case EN_OFPAT_OUTPUT:
                {
                    group_tb_hash_profile_p->igr_port = ENABLED;
                    if (total_port_num == 0)
                    {
                        if (EN_OFPP_CONTROLLER == bucket_data_p->actions_p[action_id].value.low)
                        {
                            bucket_entry_p->output_data.type = OF_OUTPUT_TYPE_PHY_PORT;
                            bucket_entry_p->output_data.port = sdn_hal_cpu_port_get();
                            total_port_num++;
                        }
                        else if (EN_OFPP_ALL == bucket_data_p->actions_p[action_id].value.low)
                        {
                            bucket_entry_p->output_data.type = OF_OUTPUT_TYPE_MULTI_EGR_PORT;
                            for (port_id = 0; port_id < sdn_hal_port_max_get() ; port_id++)
                            {
                                sdn_hal_logicalPort2PhyPort(port_id, &phy_port_id);
                                PORTMASK_PORT_SET(bucket_entry_p->output_data.portmask, phy_port_id);
                                total_port_num++;
                            }
                        }
                        else
                        {
                             bucket_entry_p->output_data.type = OF_OUTPUT_TYPE_PHY_PORT;
                             port_id = (uint32_t)bucket_data_p->actions_p[action_id].value.low;
                             DBG_SDN("[%s:%d] add port(%d) to action bucket \n", __FUNCTION__, __LINE__, port_id);
                             sdn_hal_logicalPort2PhyPort(port_id, &phy_port_id);
                             bucket_entry_p->output_data.port = phy_port_id;
                             total_port_num++;
                        }
                    }
                    else
                    {
                        bucket_entry_p->output_data.type = OF_OUTPUT_TYPE_MULTI_EGR_PORT;
                        if (total_port_num == 1)
                        {
                            phy_port_id = (uint32_t)bucket_entry_p->output_data.port;
                            //sdn_hal_logicalPort2PhyPort(port_id, &phy_port_id);
                            PORTMASK_PORT_SET(phy_port_list, phy_port_id);
                            //PORTMASK_PORT_SET(bucket_entry_p->output_data.portmask, phy_port_id);
                            DBG_SDN("[%s:%d] add port(%d) to action bucket port mask \n", __FUNCTION__, __LINE__, phy_port_id);
                        }

                        if (EN_OFPP_CONTROLLER == bucket_data_p->actions_p[action_id].value.low)
                        {
                            PORTMASK_PORT_SET(bucket_entry_p->output_data.portmask, sdn_hal_cpu_port_get());
                            total_port_num++;
                        }
                        else if (EN_OFPP_ALL == bucket_data_p->actions_p[action_id].value.low)
                        {
                            for (port_id = 0; port_id < sdn_hal_port_max_get(); port_id++)
                            {
                                sdn_hal_logicalPort2PhyPort(port_id, &phy_port_id);
                                PORTMASK_PORT_SET(phy_port_list, phy_port_id);
                                //PORTMASK_PORT_SET(bucket_entry_p->output_data.portmask, phy_port_id);
                                total_port_num++;
                            }
                        }
                        else
                        {
                             port_id = (uint32_t)bucket_data_p->actions_p[action_id].value.low;
                             sdn_hal_logicalPort2PhyPort(port_id, &phy_port_id);
                             //PORTMASK_PORT_SET(bucket_entry_p->output_data.portmask, phy_port_id);
                             PORTMASK_PORT_SET(phy_port_list, phy_port_id);
                             DBG_SDN("[%s:%d] add port(%d) to action bucket port mask", __FUNCTION__, __LINE__, port_id);
                             total_port_num++;
                        }
                        bucket_entry_p->output_data.portmask = phy_port_list;
                    }

                }
                break;
                case EN_OFPAT_PUSH_VLAN:
                {
                    uint32_t tpid_idx = 0;
                    for(tpid_idx = 0; tpid_idx < SDN_HAL_TPID_NUM_MAX; tpid_idx++)
                    {
                        /*Search for the exist entry.*/
                        if ((g_tpid[tpid_idx].validate == true)
                            && (g_tpid[tpid_idx].ether_type == (uint16)bucket_data_p->actions_p[action_id].value.low))
                        {
                            goto fill_pushvlan;
                       }
                    }

                    /*create entry if resource is enough.*/
                    for(tpid_idx = 0; tpid_idx < SDN_HAL_TPID_NUM_MAX; tpid_idx++)
                    {
                        if (g_tpid[tpid_idx].validate == false)
                        {
                            if(RT_ERR_OK == rtk_vlan_tpidEntry_set(SDN_HAL_MY_UNIT_ID(), VLAN_TAG_TYPE_OUTER, tpid_idx, (uint32)bucket_data_p->actions_p[action_id].value.low))
                            {
                                g_tpid[tpid_idx].validate = true;
                                g_tpid[tpid_idx].ether_type = (uint16)bucket_data_p->actions_p[action_id].value.low;
                                goto fill_pushvlan;
                            }
                        }
                    }
fill_pushvlan:
                    bucket_entry_p->push_vlan                    = ENABLED;
                    bucket_entry_p->push_vlan_data.etherType_idx = tpid_idx;
                }
                break;
                case EN_OFPAT_COPY_TTL_IN:
                    group_tb_hash_profile_p->sip  = ENABLED;
                    bucket_entry_p->cp_ttl_inward = ENABLED;
                break;
                case EN_OFPAT_COPY_TTL_OUT:
                    group_tb_hash_profile_p->dip   = ENABLED;
                    bucket_entry_p->cp_ttl_outward = ENABLED;
                break;
                case EN_OFPAT_SET_QUEUE:
                    bucket_entry_p->set_queue = ENABLED;
                    bucket_entry_p->qid       = (uint32)bucket_data_p->actions_p[action_id].value.low;
                break;
                case EN_OFPAT_DEC_MPLS_TTL:
                    bucket_entry_p->dec_mpls_ttl = ENABLED;
                break;
                case EN_OFPAT_PUSH_MPLS:
                    bucket_entry_p->push_mpls    = ENABLED;
                    bucket_entry_p->push_mpls_data.etherType = (uint32)bucket_data_p->actions_p[action_id].value.low;
                break;
                case EN_OFPAT_POP_MPLS:
                    bucket_entry_p->pop_mpls     = ENABLED;
                    bucket_entry_p->pop_mpls_type= (uint32)bucket_data_p->actions_p[action_id].value.low;
                break;
                case EN_OFPAT_SET_FIELD:
                {
                    /*todo: Each set field action can set all oxm type at most one times.*/
                    switch(bucket_data_p->actions_p[action_id].oxm_type)
                    {
                        case EN_OFPXMT_OFB_VLAN_VID:
                        {
                            bucket_entry_p->set_field_2_data.field_type = OF_IGR_FT0_SF_TYPE2_VLAN_ID;
                            bucket_entry_p->set_field_2_data.field_data  = (uint16)bucket_data_p->actions_p[action_id].value.low;
                            DBG_SDN("\r\n[%s:%d] filed_data(%d)", __FUNCTION__, __LINE__, (uint32)bucket_entry_p->set_field_2_data.field_data);
                            break;
                        }
                        case EN_OFPXMT_OFB_IP_DSCP:
                        {
                            bucket_entry_p->set_field_0_data.field_type = OF_IGR_FT0_SF_TYPE0_IP_DSCP;
                            bucket_entry_p->set_field_0_data.field_data = (uint8)bucket_data_p->actions_p[action_id].value.low;
                            bucket_entry_p->set_field_1_data.field_type = OF_IGR_FT0_SF_TYPE1_IP_DSCP;
                            bucket_entry_p->set_field_1_data.field_data = (uint8)bucket_data_p->actions_p[action_id].value.low;
                            bucket_entry_p->set_field_2_data.field_type = OF_IGR_FT0_SF_TYPE2_IP_DSCP;
                            bucket_entry_p->set_field_2_data.field_data = (uint8)bucket_data_p->actions_p[action_id].value.low;
                            DBG_SDN("\r\n[%s:%d] filed_data(%d)", __FUNCTION__, __LINE__, (uint32)bucket_entry_p->set_field_2_data.field_data);
                            break;
                        }
                        case EN_OFPAT_SET_NW_TTL:
                        {
                            bucket_entry_p->set_field_0_data.field_type = OF_IGR_FT0_SF_TYPE0_IP_TTL;
                            bucket_entry_p->set_field_0_data.field_data = (uint32)bucket_data_p->actions_p[action_id].value.low;
                            bucket_entry_p->set_field_1_data.field_type = OF_IGR_FT0_SF_TYPE1_IP_TTL;
                            bucket_entry_p->set_field_1_data.field_data = (uint32)bucket_data_p->actions_p[action_id].value.low;
                            bucket_entry_p->set_field_2_data.field_type = OF_IGR_FT0_SF_TYPE2_IP_TTL;
                            bucket_entry_p->set_field_2_data.field_data = (uint32)bucket_data_p->actions_p[action_id].value.low;
                            DBG_SDN("\r\n[%s:%d] filed_data(%d)", __FUNCTION__, __LINE__, (uint32)bucket_entry_p->set_field_2_data.field_data);
                            break;
                        }
                        case EN_OFPXMT_OFB_VLAN_PCP:
                        {
                            bucket_entry_p->set_field_0_data.field_type = OF_IGR_FT0_SF_TYPE0_VLAN_PRI;
                            bucket_entry_p->set_field_0_data.field_data  = (uint8)bucket_data_p->actions_p[action_id].value.low;
                            bucket_entry_p->set_field_1_data.field_type = OF_IGR_FT0_SF_TYPE1_VLAN_PRI;
                            bucket_entry_p->set_field_1_data.field_data  = (uint8)bucket_data_p->actions_p[action_id].value.low;
                            bucket_entry_p->set_field_2_data.field_type = OF_IGR_FT0_SF_TYPE2_VLAN_PRI;
                            bucket_entry_p->set_field_2_data.field_data  = (uint8)bucket_data_p->actions_p[action_id].value.low;
                            DBG_SDN("\r\n[%s:%d] filed_data(%d)", __FUNCTION__, __LINE__, (uint32)bucket_entry_p->set_field_2_data.field_data);
                            break;
                        }
                        case EN_OFPXMT_OFB_ETH_SRC:
                        {
                            bucket_entry_p->set_field_0_data.field_type = OF_IGR_FT0_SF_TYPE0_SA;
                            memcpy(bucket_entry_p->set_field_0_data.mac.octet, ((char*)&bucket_data_p->actions_p[action_id].value.low) + 2, OFP_ETH_ALEN);
                            group_tb_hash_profile_p->smac = ENABLED;
                            break;
                        }
                        case EN_OFPXMT_OFB_ETH_DST:
                        {
                            bucket_entry_p->set_field_1_data.field_type = OF_IGR_FT0_SF_TYPE1_DA;
                            memcpy(bucket_entry_p->set_field_1_data.mac.octet, ((char*)&bucket_data_p->actions_p[action_id].value.low) + 2, OFP_ETH_ALEN);
                            group_tb_hash_profile_p->dmac = ENABLED;
                            break;
                        }
                        case EN_OFPXMT_OFB_MPLS_LABEL:
                        {
                            bucket_entry_p->set_field_1_data.field_type = OF_IGR_FT0_SF_TYPE1_MPLS_LABEL;
                            bucket_entry_p->set_field_1_data.field_data = (uint32)bucket_data_p->actions_p[action_id].value.low;
                            DBG_SDN("\r\n[%s:%d] filed_data(%d)", __FUNCTION__, __LINE__, (uint32)bucket_entry_p->set_field_2_data.field_data);
                            break;
                        }
                        case EN_OFPXMT_OFB_MPLS_TC:
                        {
                            bucket_entry_p->set_field_0_data.field_type = OF_IGR_FT0_SF_TYPE0_MPLS_TC;
                            bucket_entry_p->set_field_0_data.field_data = (uint8)bucket_data_p->actions_p[action_id].value.low;
                            bucket_entry_p->set_field_1_data.field_type = OF_IGR_FT0_SF_TYPE1_MPLS_TC;
                            bucket_entry_p->set_field_1_data.field_data = (uint8)bucket_data_p->actions_p[action_id].value.low;
                            bucket_entry_p->set_field_2_data.field_type = OF_IGR_FT0_SF_TYPE1_MPLS_TC;
                            bucket_entry_p->set_field_2_data.field_data = (uint8)bucket_data_p->actions_p[action_id].value.low;
                            DBG_SDN("\r\n[%s:%d] filed_data(%d)", __FUNCTION__, __LINE__, (uint32)bucket_entry_p->set_field_2_data.field_data);
                            break;
                        }
#if 0
                        case EN_OFPXMT_OFB_MPLS_TTL:
                        {
                            bucket_entry_p->set_field_0_data.field_type = OF_IGR_FT0_SF_TYPE0_MPLS_TTL;
                            bucket_entry_p->set_field_0_data.field_data = (uint32)bucket_data_p->actions_p[action_id].value.low;
                            bucket_entry_p->set_field_1_data.field_type = OF_IGR_FT0_SF_TYPE1_MPLS_TTL;
                            bucket_entry_p->set_field_1_data.field_data = (uint32)bucket_data_p->actions_p[action_id].value.low;
                            bucket_entry_p->set_field_2_data.field_type = OF_IGR_FT0_SF_TYPE2_MPLS_TTL;
                            bucket_entry_p->set_field_2_data.field_data = (uint32)bucket_data_p->actions_p[action_id].value.low;
                        }
#endif
                        default:
                        {
                            break;
                        }
                    }
                }
                break;
                default:
                    return SDN_HAL_RETURN_FAILED;
            }
        }
    }

    return SDN_HAL_RETURN_OK;
}

static int32 sdn_hal_apply_actionbucket(sdn_db_group_action_bucket_t *action_bucket_p, uint32_t bucket_len, rtk_of_groupTblHashPara_t *group_tb_hash_profile_p)
{
    uint32_t bucket_id = 0;
    uint32_t rc        = RT_ERR_OK;
    rtk_of_actionBucket_t hal_group_action_bucket;

    if (action_bucket_p == NULL || group_tb_hash_profile_p == NULL)
    {
        DBG_SDN("\r\n[%s:%d] action bucket is null \r\n", __FUNCTION__, __LINE__);
        return SDN_HAL_RETURN_FAILED;
    }

    for (bucket_id = 0; bucket_id < bucket_len; bucket_id++)
    {
        memset(&hal_group_action_bucket, 0, sizeof(rtk_of_actionBucket_t));

        rtk_of_actionBucket_get(0, g_total_actions_bucket_num + bucket_id,&hal_group_action_bucket);

        if (sdn_hal_convert_bucket_to_hal(&action_bucket_p[bucket_id], &hal_group_action_bucket, group_tb_hash_profile_p) != SDN_HAL_RETURN_OK)
        {
            DBG_SDN("\r\n[%s:%d] action converted is failed \r\n", __FUNCTION__, __LINE__);
            return SDN_HAL_RETURN_FAILED;
        }

        if((rc = rtk_of_actionBucket_set(0, g_total_actions_bucket_num + bucket_id, &hal_group_action_bucket)) != RT_ERR_OK)
        {
            DBG_SDN("\r\n[%s:%d] action is set to chip failed error(%d) bucket_id(%d)\r\n", __FUNCTION__, __LINE__, rc, g_total_actions_bucket_num + bucket_id);
            return SDN_HAL_RETURN_FAILED;
        }
    }

    g_total_actions_bucket_num += bucket_len;
    DBG_SDN("\r\n[%s:%d] ACTION BUCKET join into hal done!!!\n", __FUNCTION__, __LINE__);
    return SDN_HAL_RETURN_OK;
}
/*
    rtk_enable_t    igr_port;
    rtk_enable_t    smac;
    rtk_enable_t    dmac;
    rtk_enable_t    sip;
    rtk_enable_t    dip;
    rtk_enable_t    l4_sport;
    rtk_enable_t    l4_dport;
  rtk_of_groupTblHashPara_t;
*/
int32 sdn_hal_add_groupentry(sdn_db_group_entry_t *group_entry_p)
{
    uint32_t hal_group_id = 0;
    rtk_of_groupEntry_t hal_group_entry;
    rtk_of_groupTblHashPara_t hal_group_select_hash_profile;

    if (group_entry_p == NULL)
    {
        DBG_SDN("\r\n[%s:%d] group entry is null \r\n", __FUNCTION__, __LINE__);
        return SDN_HAL_RETURN_FAILED;
    }

    memset(&hal_group_select_hash_profile, 0, sizeof(rtk_of_groupTblHashPara_t));

    rtk_of_groupTblHashPara_get(SDN_HAL_MY_UNIT_ID(), &hal_group_select_hash_profile);

    if(sdn_hal_apply_actionbucket(group_entry_p->action_bucket_p, group_entry_p->bucket_len, &hal_group_select_hash_profile) != SDN_HAL_RETURN_OK)
    {
        DBG_SDN("\r\n[%s:%d] convert to chip action bucket is failed \r\n", __FUNCTION__, __LINE__);
        return SDN_HAL_RETURN_FAILED;
    }

    hal_group_entry.type       = group_entry_p->group_type;
    hal_group_entry.bucket_num = group_entry_p->bucket_len;
    hal_group_entry.bucket_id  = g_total_actions_bucket_num - group_entry_p->bucket_len;
    hal_group_id = SDN_HAL_GET_ASIC_GROUPID(group_entry_p->entry_id); // (group_entry_p->entry_id >= 1 ? (group_entry_p->entry_id - 1) : 0);

    if (rtk_of_groupEntry_set(SDN_HAL_MY_UNIT_ID() , hal_group_id, &hal_group_entry) != RT_ERR_OK)
    {
        DBG_SDN("\r\n[%s:%d] apply group entry into chip is failed. \r\n", __FUNCTION__, __LINE__);
        return SDN_HAL_RETURN_FAILED;
    }

    if (hal_group_entry.type == OF_GROUP_TYPE_ALL)
    {
        memset(&hal_group_select_hash_profile, ENABLED, sizeof(rtk_of_groupTblHashPara_t));
        DBG_SDN("\r\n[%s:%d] apply group mode is ALL . \r\n", __FUNCTION__, __LINE__);
    }
    else
    {
        DBG_SDN("\r\n[%s:%d] apply group mode is SELECT . \r\n", __FUNCTION__, __LINE__);
    }

    rtk_of_groupTblHashPara_set(SDN_HAL_MY_UNIT_ID(), &hal_group_select_hash_profile);

    DBG_SDN("\r\n[%s:%d] group entry join into hal id(%d) done!!!\n", __FUNCTION__, __LINE__, group_entry_p->entry_id);
    return SDN_HAL_RETURN_OK;
}

int32 sdn_hal_del_groupentry(sdn_db_group_entry_t *group_entry_p)
{
    uint32_t hal_group_id = 0;
    uint32_t bucket_id = 0;
    uint32_t last_bucket_id = 0;
    rtk_of_groupEntry_t    hal_group_entry;
    rtk_of_actionBucket_t  hal_group_action_bucket;

    if (group_entry_p == NULL)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    DBG_SDN("\r\n[%s:%d] try to del group id(%d) ok!!!\n", __FUNCTION__, __LINE__, group_entry_p->entry_id);

    memset(&hal_group_entry, 0, sizeof(rtk_of_groupEntry_t));
    memset(&hal_group_action_bucket, 0, sizeof(rtk_of_actionBucket_t));

    hal_group_id = SDN_HAL_GET_ASIC_GROUPID(group_entry_p->entry_id); // (group_entry_p->entry_id >= 1 ? (group_entry_p->entry_id - 1) : 0);

    DBG_SDN("\r\n[%s:%d] try to get chip group id(%d) ok!!!\n", __FUNCTION__, __LINE__, hal_group_id);

    if (rtk_of_groupEntry_get(0, hal_group_id /*group_entry_p->entry_id*/, &hal_group_entry) != RT_ERR_OK)
    {
        return SDN_HAL_RETURN_FAILED;
    }

    last_bucket_id = hal_group_entry.bucket_id + hal_group_entry.bucket_num;

    for (bucket_id = hal_group_entry.bucket_id; bucket_id < last_bucket_id; bucket_id++)
    {
        memset(&hal_group_action_bucket, 0, sizeof(rtk_of_actionBucket_t));
        hal_group_entry.bucket_id = bucket_id;
        hal_group_action_bucket.pop_mpls_type = OF_POP_MPLS_TYPE_OUTERMOST_LABEL;
        hal_group_action_bucket.push_mpls_data.vpn_type = OF_PUSH_MPLS_VPN_TYPE_L3;
        hal_group_action_bucket.push_mpls_data.etherType = 0x8847;

        if (rtk_of_actionBucket_set(0, hal_group_entry.bucket_id, &hal_group_action_bucket) != RT_ERR_OK)
        {
            DBG_SDN("\r\n[%s:%d] group entry del hal action bucket id(%d) failed!!!\n", __FUNCTION__, __LINE__, hal_group_entry.bucket_id);
            return SDN_HAL_RETURN_FAILED;
        }
        DBG_SDN("\r\n[%s:%d] group entry del hal action bucket id(%d) ok!!!\n", __FUNCTION__, __LINE__, hal_group_entry.bucket_id);
    }

    memset(&hal_group_entry, 0, sizeof(rtk_of_groupEntry_t));

    hal_group_entry.type       = OF_GROUP_TYPE_ALL;
    hal_group_entry.bucket_num = 1;
    if (rtk_of_groupEntry_set( 0, hal_group_id /*group_entry_p->entry_id*/, &hal_group_entry) != RT_ERR_OK)
    {
        DBG_SDN("\r\n[%s:%d] group entry del from hal id(%d) soft id(%d) failed!!!\n", __FUNCTION__, __LINE__, hal_group_id, group_entry_p->entry_id);
        return SDN_HAL_RETURN_FAILED;
    }

    if (g_total_actions_bucket_num > 0)
    {
        g_total_actions_bucket_num -= group_entry_p->bucket_len;
    }

    DBG_SDN("\r\n[%s:%d] group entry del from hal id(%d) done!!!\n", __FUNCTION__, __LINE__, group_entry_p->entry_id);

    return SDN_HAL_RETURN_OK;
}

int32 sdn_hal_get_groupfeature(EN_OFP_GROUP_TYPE_T type)
{

    if ((sizeof(g_sdn_hal_group_feature)/sizeof(g_sdn_hal_group_feature[0])) < type )
    {
        return 0;
    }

    return g_sdn_hal_group_feature[type].actions_feature;
}
//new group code


//qos code
static int32 sdn_hal_local_destroy_qos(uint32_t phy_port)
{
    uint32_t queue_id = 0;

    for ( queue_id  = 0; queue_id < 8; queue_id++)
    {
        if (RT_ERR_OK != rtk_rate_portEgrQueueBwCtrlEnable_set(SDN_HAL_MY_UNIT_ID(), phy_port, queue_id, DISABLED))
        {
             return SDN_HAL_RETURN_FAILED;
        }

        if (RT_ERR_OK != rtk_rate_portEgrQueueBwCtrlRate_set(SDN_HAL_MY_UNIT_ID(), phy_port, queue_id, 0xfffff))
        {
             return SDN_HAL_RETURN_FAILED;
        }
    }

    return SDN_HAL_RETURN_OK;
}

int sdn_hal_set_queueconfig(sdn_db_qos_entry_t qos_entry)
{
    uint32_t phy_port = 0;
    rtk_of_classifierData_t of_port;

    for(phy_port = 0; phy_port < sdn_hal_port_max_get(); phy_port++)
    {
        of_port.port = phy_port;
        rtk_of_classifier_get(SDN_HAL_MY_UNIT_ID() , OF_CLASSIFIER_TYPE_PORT, &of_port);

        if ((of_port.enable != ENABLED) || (phy_port == PHY_PORT_56))
        {
            continue;
        }

        if (RT_ERR_OK != rtk_rate_portEgrQueueBwCtrlRate_set(SDN_HAL_MY_UNIT_ID(), phy_port, qos_entry.qid, qos_entry.max_rate))
        {
            DBG_SDN("[%s:%d] set port(%d) queue(%d) rate(%d) failed \n", __FUNCTION__, __LINE__, phy_port, qos_entry.qid, qos_entry.max_rate);
            return SDN_HAL_RETURN_FAILED;
        }
    }

    return SDN_HAL_RETURN_OK;
}

int sdn_hal_set_port_queueconfig(uint32_t phy_port, sdn_db_qos_entry_t qos_entry)
{
    if (RT_ERR_OK != rtk_rate_portEgrQueueBwCtrlRate_set(SDN_HAL_MY_UNIT_ID(), phy_port, qos_entry.qid, qos_entry.max_rate))
    {
        DBG_SDN("[%s:%d] set port(%d) queue(%d) rate(%d) failed \n", __FUNCTION__, __LINE__, phy_port, qos_entry.qid, qos_entry.max_rate);
        return SDN_HAL_RETURN_FAILED;
    }

    if (RT_ERR_OK != rtk_rate_portEgrQueueBwCtrlEnable_set(SDN_HAL_MY_UNIT_ID(), phy_port, qos_entry.qid, ENABLED))
    {
        DBG_SDN("[%s:%d] enable port(%d) queue(%d) rate(%d) failed \n", __FUNCTION__, __LINE__, phy_port, qos_entry.qid, qos_entry.max_rate);
        return SDN_HAL_RETURN_FAILED;
    }

    return SDN_HAL_RETURN_OK;
}

int sdn_hal_del_port_queueconfig(uint32_t phy_port,  sdn_db_qos_entry_t qos_entry)
{
    if (RT_ERR_OK != rtk_rate_portEgrQueueBwCtrlRate_set(SDN_HAL_MY_UNIT_ID(), phy_port, qos_entry.qid, qos_entry.max_rate))
    {
        return SDN_HAL_RETURN_FAILED;
    }

    if (RT_ERR_OK != rtk_rate_portEgrQueueBwCtrlEnable_set(SDN_HAL_MY_UNIT_ID(), phy_port, qos_entry.qid, DISABLED))
    {
        return SDN_HAL_RETURN_FAILED;
    }

    return SDN_HAL_RETURN_OK;
}

int sdn_hal_get_port_queueconfig(uint32_t port, uint32_t queue_id, uint32_t *max_rate_p, uint32_t *min_rate_p)
{
    //uint32_t port     = 0xff;
    //uint32_t queue_id = 0xff;
    uint32_t rate = 0xff;

    if ( /*(port_p == NULL)
         || (queue_id_p == NULL)
         || */(max_rate_p == NULL)
         || (min_rate_p == NULL)
       )
    {
        DBG_SDN("[%s:%d] failed \n ", __FUNCTION__, __LINE__);
        return SDN_HAL_RETURN_FAILED;
    }

    //port     = *port_p;
    //queue_id = *queue_id_p;

    if (RT_ERR_OK != rtk_rate_portEgrQueueBwCtrlRate_get(SDN_HAL_MY_UNIT_ID(), port, queue_id, &rate))
    {
        DBG_SDN("[%s:%d] port(%d), queue(%d) failed \n ", __FUNCTION__, __LINE__, port, queue_id);
        return SDN_HAL_RETURN_FAILED;
    }

    *max_rate_p = rate;
    *min_rate_p = rate;

    DBG_SDN("[%s:%d] port(%d), queue(%d) rate(%08x) \n ", __FUNCTION__, __LINE__, port, queue_id, rate);
    return SDN_HAL_RETURN_OK;
}

int sdn_hal_get_port_queuestats(uint32_t port, uint32_t queue_id, uint32_t *tx_packets_p, uint32_t *tx_error_p)
{

    //uint32_t port     = 0xff;
    //uint32_t queue_id = 0xff;
    uint32_t tx_packets = 0xf;
    uint32_t tx_errors  = 0xff;

    if ( /*(port_p == NULL)
         || (queue_id_p == NULL)
         ||*/ (tx_packets_p == NULL)
         || (tx_error_p == NULL)
       )
    {
        return SDN_HAL_RETURN_FAILED;
    }

    //port     = *port_p;
    //queue_id = *queue_id_p;

    DBG_SDN("[%s:%d] try to get port(%d) queue(%d) stat\n", __FUNCTION__, __LINE__, port, queue_id);
    if ( RT_ERR_OK != rtk_stat_port_get(SDN_HAL_MY_UNIT_ID(), port, queue_stat_id[queue_id].out_pkt_queue_id, (uint64_t *)&tx_packets)
        || RT_ERR_OK != rtk_stat_port_get(SDN_HAL_MY_UNIT_ID(), port, queue_stat_id[queue_id].out_pkt_drop_id, (uint64_t *)&tx_errors))
    {
        return SDN_HAL_RETURN_FAILED;
    }

    *tx_packets_p = tx_packets;
    *tx_error_p   = tx_errors;
    DBG_SDN("[%s:%d] try to get port(%d) queue(%d) tx packets(%d) tx errors(%d)\n", __FUNCTION__, __LINE__, port, queue_id, *tx_packets_p, *tx_error_p);

    return SDN_HAL_RETURN_OK;
}


