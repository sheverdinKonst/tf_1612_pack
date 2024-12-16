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

/*
 * Include Files
 */
#ifndef SDN_HAL_H
#define SDN_HAL_H
#include <stdio.h>
#include <stdlib.h>
//#include "sdn_db.h"
#include <sdn_type.h>
//#include "ofp_common_enum.h"
#include <ofp_common_enum.h>
//#include "sdn_hal_common.h"
#include <sdn_hal_common.h>
#include "rtk/openflow.h"
#include "rtk/stat.h"
#include "rtk/acl.h"
#include "dal/rtrpc/rtrpc_openflow.h"
#include "dal/rtrpc/rtrpc_counter.h"
#include "dal/rtrpc/rtrpc_acl.h"

/*
 * Symbol Definition
 */

#ifdef SDN_HAL_DBG_MSG
#define SDN_HAL_MSG(fmt, args...)     do { printf(fmt, ##args); } while (0)
#else
#define SDN_HAL_MSG(fmt, args...)     do { } while (0)
#endif


#ifdef  CONFIG_SDK_RTL9310
#define    HAL_INGRESS_TABLE_NUM      (4)       /*ingress has 4 tables ,  table 0~ table3 */
#define    HAL_EGRESS_TABLE_NUM       (1)        /*ingress has only 1 table ,  table 0*/
#endif

#define    HAL_TABLE_NUM                                        (HAL_INGRESS_TABLE_NUM+HAL_EGRESS_TABLE_NUM)
#define    HAL_ALL_FULL_MATCH_RULE_NUM                          (4*1024)-HAL_INGRESS_RESERVE_ACL_RULE_NUM-HAL_EGRESS_RESERVE_ACL_RULE_NUM   // 32-2=30 blocks
#define    HAL_RULE_INVAILD_COUNTER_ID      0XFFFF

#define  HAL_MEMTER_NUM         (512)
#define  INVALID_METER_INDEX	HAL_MEMTER_NUM
#define  INVALID_FLOW_INDEX				(~0)
#define  INVALID_COUNTER_INDEX    HAL_RULE_COUNTER_SIZE

typedef rtk_acl_action_t   sdn_hal_lib_action_t;
typedef rtk_acl_vAct_t     sdn_hal_lib_vAction_t;
typedef rtk_acl_iAct_t     sdn_hal_lib_iAction_t;
typedef rtk_acl_eAct_t     sdn_hal_lib_eAction_t;


#define SDN_MAX_LOGICAL_AND_CPU_PORT_NUM                        (53)
#define PORTCONF_END_VAL                                        (0xFFFF)
#define SDN_HAL_ACL_ENTRY_SIZE_PER_BLOCK                        (128)
#define SDN_HAL_ACL_INGRESS_EGRESS_CUT_LINE                     (16)
#define SDN_HAL_ACL_START_BLOCK_ID_OF_TABLE_0                   (SDN_HAL_ACL_BLOCK_SIZE - SDN_HAL_ACL_INGRESS_EGRESS_CUT_LINE)
#define SDN_HAL_ACL_SHARED_TEMPLATE_ID                          (7)
#define SDN_HAL_ACL_TABLE0_TEMPLATE_ID                          (6)
#define SDN_HAL_ACL_BLOCK_SIZE                                  (18)
#define SDN_HAL_ACL_BLOCK_ID_START                              (0)
#define SDN_HAL_ACL_BLOCK_ID_MAX                                (SDN_HAL_ACL_BLOACK_SIZE - 1)
#define SDN_HAL_ACL_ENTRY_START                                 (0)
#define SDN_HAL_ACL_ENTRY_END                                   (SDN_HAL_ACL_BLOCK_SIZE * SDN_HAL_ACL_ENTRY_SIZE_PER_BLOCK - 1)
#define SDN_HAL_RULE_TABLE0_START_ID                            (SDN_HAL_ACL_START_BLOCK_ID_OF_TABLE_0 * SDN_HAL_ACL_ENTRY_SIZE_PER_BLOCK)
#define SDN_HAL_RULE_TABLE0_END_ID                              (SDN_HAL_ACL_ENTRY_END)
#define SDN_HAL_ENTRY_SIZE_PER_BLOCK                            (128)
#define SDN_HAL_AG_ENTRY_LENGTH                                 (2)
#define SDN_HAL_BLOCK_SIZE                                      (32)
#define SDN_HAL_LOGIC_BLOCK_ID(flow_idx)                        ((flow_idx)/SDN_HAL_BLOCK_SIZE)
#define SDN_HAL_BLOCK_ID_START                                  (0)
#define SDN_HAL_BLOCK_ID_MAX                                    (SDN_HAL_BLOCK_SIZE - 1)
#define SDN_HAL_TABLE0_BLOCK_SIZE                               (31)
#define SDN_HAL_TABLE3_BLOCK_SIZE                               (1)
#define SDN_HAL_TABLE4_BLOCK_SIZE                               (0)

#define SDN_HAL_TABLE0_BLOCK_ID_START                           (0)
#define SDN_HAL_TABLE0_BLOCK_ID_END                             (SDN_HAL_TABLE0_BLOCK_ID_START + SDN_HAL_TABLE0_BLOCK_SIZE - 1)
#define SDN_HAL_TABLE3_BLOCK_ID_START                           (SDN_HAL_TABLE0_BLOCK_ID_END + 1)
#define SDN_HAL_TABLE3_BLOCK_ID_END                             (SDN_HAL_TABLE3_BLOCK_ID_START + SDN_HAL_TABLE3_BLOCK_SIZE - 1)
#define SDN_HAL_TABLE4_BLOCK_ID_START                           (SDN_HAL_TABLE3_BLOCK_ID_END + 1)
#define SDN_HAL_TABLE4_BLOCK_ID_END                             (SDN_HAL_TABLE4_BLOCK_ID_START + SDN_HAL_TABLE4_BLOCK_SIZE - 1)

#define SDN_HAL_INGRESS_L3_TYPE_SIP_V4_AND_DIP_V4_DIV_FACTOR    (2)
#define SDN_HAL_INGRESS_L3_TYPE_SIP_V6_AND_DIP_V6_DIV_FACTOR    (3)
#define SDN_HAL_INGRESS_L3_TYPE_SIP_V4_OR_DIP_V4_DIV_FACTOR     (2)
#define SDN_HAL_INGRESS_L3_TYPE_SIP_V6_OR_DIP_V6_DIV_FACTOR     (6)
#define SDN_HAL_INGRESS_L3_DIV_FACTOR                           (SDN_HAL_INGRESS_L3_TYPE_SIP_V4_AND_DIP_V4_DIV_FACTOR)

#if defined(CONFIG_FLOW_AG)
#define SDN_HAL_INGRESS_FULL_MATCH_TABLE0_RULE_NUM              ((SDN_HAL_TABLE0_BLOCK_SIZE * SDN_HAL_ENTRY_SIZE_PER_BLOCK)/SDN_HAL_AG_ENTRY_LENGTH)
#define SDN_HAL_INGRESS_FULL_MATCH_TABLE3_RULE_NUM              ((SDN_HAL_TABLE3_BLOCK_SIZE * SDN_HAL_ENTRY_SIZE_PER_BLOCK)/SDN_HAL_AG_ENTRY_LENGTH)     //10 blocks
#else
#define SDN_HAL_INGRESS_FULL_MATCH_TABLE0_RULE_NUM              (SDN_HAL_TABLE0_BLOCK_SIZE * SDN_HAL_ENTRY_SIZE_PER_BLOCK)     //10 blocks
#define SDN_HAL_INGRESS_FULL_MATCH_TABLE3_RULE_NUM              (SDN_HAL_TABLE3_BLOCK_SIZE * SDN_HAL_ENTRY_SIZE_PER_BLOCK)     //10 blocks
#endif
#define SDN_HAL_INGRESS_L2_TABLE1_RULE_NUM                      (16*1024)
#define SDN_HAL_INGRESS_L3_TABLE2_CAM_RULE_NUM                  (12*1024/SDN_HAL_INGRESS_L3_TYPE_SIP_V4_AND_DIP_V4_DIV_FACTOR)
#define SDN_HAL_INGRESS_L3_TABLE2_HASH_RULE_NUM                 (12*1024/SDN_HAL_INGRESS_L3_TYPE_SIP_V4_AND_DIP_V4_DIV_FACTOR)
#define SDN_HAL_INGRESS_L3_TABLE2_RULE_NUM                      (SDN_HAL_INGRESS_L3_TABLE2_CAM_RULE_NUM+SDN_HAL_INGRESS_L3_TABLE2_HASH_RULE_NUM)
#define SDN_HAL_EGRESS_TABLE0_RULE_NUM                          (SDN_HAL_TABLE4_BLOCK_SIZE * SDN_HAL_ENTRY_SIZE_PER_BLOCK)     //10 blocks
#define SDN_HAL_MAX_NUM_TEMPLATE_FIELD                          (RTK_MAX_NUM_OF_PIE_TEMPLATE_FIELD - 2)
#define SDN_HAL_MAX_NUM_BLOCK_TEMPLATE                          (RTK_OF_BLOCK_TEMPLATE_MAX)


#define SDN_HAL_RULE_USER_TEMPLATE_ID_0                         (5)
#define SDN_HAL_RULE_USER_TEMPLATE_ID_1                         (6)
#define SDN_HAL_RULE_USER_TEMPLATE_ID_2                         (7)
#define SDN_HAL_RULE_USER_TEMPLATE_ID_3                         (8)
#define SDN_HAL_RULE_USER_TEMPLATE_ID_4                         (9)
#define SDN_HAL_RULE_USER_TEMPLATE_NUM                          (3)


#define SDN_HAL_PORT_MAX                                        (RTK_MAX_PORT_PER_UNIT)
#define SDN_HAL_RTK_METER_SIZE                                  (512)
#define SDN_HAL_INVALID_UNIT                                    (255)
#define SDN_HAL_MAX_TABLE_NUM                                   (FT_PHASE_END)
#define SDN_HAL_CPU_PORT                                        (0xfffe)
#define SDN_HAL_MGMT_PORT_1                                     (46)
#define SDN_HAL_MGMT_PORT_2                                     (47)
#if 1  /* Brandon add */

#define HAL_TABLE_NUM                                           (HAL_INGRESS_TABLE_NUM+HAL_EGRESS_TABLE_NUM)
#define HAL_INGRESS_RESERVE_ACL_RULE_NUM                        (2*128)    //reserve ACL rule number
#define HAL_EGRESS_RESERVE_ACL_RULE_NUM                         (0)
#define HAL_ALL_FULL_MATCH_RULE_NUM                             (4*1024)-HAL_INGRESS_RESERVE_ACL_RULE_NUM-HAL_EGRESS_RESERVE_ACL_RULE_NUM
#define HAL_RULE_COUNTER_SIZE                                   (4096)    /*temporarily define a value. */
#define HAL_RULE_PORT_NUM                                       (USR_PORT_54)      /*temporarily define a value.  */
#define HAL_RULE_INVAILD_COUNTER_ID                             0XFFFF
#define HAL_MEMTER_NUM                                          (512)
#define INVALID_METER_INDEX                                     HAL_MEMTER_NUM
#define INVALID_FLOW_INDEX                                      (~0)
#define INVALID_COUNTER_INDEX                                   HAL_RULE_COUNTER_SIZE
#define INVALID_TABLE_INDEX                                     0xff
#endif
#define SDN_HAL_TABLE(table_idx)                                (1 << (table_idx))
#define SDN_HAL_TABLE_ID_BITMAP_NONE                            (0)

#define SDN_HAL_METER_INDEX(meter_id)			                ((meter_id) - 1)
#define SDN_HAL_MAX_GROUP_BUCKET_LEN                            (128)
#define SDN_HAL_MAX_GROUP_ENTRY_NUM                             (2048)
#define SDN_HAL_MAX_GROUP_TABLE_NUM                             (1)
#define SDN_HAL_TPID_NUM_MAX                                    (4)

/* RTK ACL Field Select data field (two bytes)*/
#define RTK_DPA_OUTER_IP_VER_OFFSET                             0xc
#define RTK_DPA_INNER_IP_VER_OFFSET                             0xc
#define RTK_DPA_OUTER_IP_FLAG_OFFSET                            0xd
#define RTK_DPA_OUTER_TCP_FLAG_MASK                             0x3f
#define RTK_DPA_GTP_MT_VALUE                                    0xfe
#define RTK_DPA_GTP_MT_VALUE_MASK                               0xff
#define RTK_DPA_INNER_TCP_FLAG_MASK                             0x1ff


#define FOR_EACH_UNIT(idx)\
    for(idx = 0; idx < RTK_MAX_UNIT_ID; idx++)\
        if (unit_list[idx] != SDN_HAL_INVALID_UNIT) \

#define SDN_HAL_APPLY_FIELD_VALUE(_unit, _phase, _id, _field, _data, _mask)\
{ \
   uint8 ret = 0;\
   DBG_SDN("\r\n[OHAL_LIB_APPLY_FIELD_VALUE][%d]set unit(%d), phase(%d), entry(%d), field(%d), mask(0x%x)\r\n",  __LINE__, (UINT32_T)_unit, (UINT32_T)_phase, (UINT32_T)_id, (UINT32_T)_field,_mask);\
   ret = rtk_of_flowEntryField_write(_unit, _phase, _id, _field, _data, _mask); \
   DBG_SDN("\r\n[%s][%d] Brandon test , ret=[%d]\r\n",__FUNCTION__, __LINE__,ret);\
}
#define SDN_HAL_LIB_BIT_ON(_bitmap, bits) (_bitmap |= (1ULL << bits))

#define SDN_HAL_LIB_BIT_OFF(_bitmap, bits) (_bitmap &= ~( 1ULL << bits))

#define SDN_HAL_LIB_CHK_BIT_ON(_bitmap, bits) (_bitmap & ( 1ULL << bits))

#define SDN_HAL_LIB_PORT_MASK_IS_SET(array, index)\
    (array[index/(sizeof(array[0]) << 3)] & (0x1 << (index%(sizeof(array[0]) << 3))))
#define SDN_HAL_LIB_PORT_MASK_IS_CLEAR(array, index)\
    !(array[index/(sizeof(array[0]) << 3)] & (0x1 << (index%(sizeof(array[0]) << 3))))
#define SDN_HAL_LIB_PORT_MASK_ADD(array, index) do {\
    array[index/(sizeof(array[0]) << 3)] = \
    (array[index/(sizeof(array[0]) << 3)] | (0x1 << (index%(sizeof(array[0]) << 3)))); } while(0)
#define SDN_HAL_LIB_PORT_MASK_DEL(array, index) do {\
    array[index/(sizeof(array[0]) << 3)] = \
    (array[index/(sizeof(array[0]) << 3)] & ~(0x1 << (index%(sizeof(array[0]) << 3)))); } while(0)
#define SDN_HAL_IS_VALID_METADATA(value) ((value >> 12) == 0)
typedef struct sdn_hal_table_supported_field_s
{
    EN_OFP_OXM_OFB_MATCH_FIELDS_T  fields;
    uint32 table_id_bitmap;
}sdn_hal_table_supported_field_t;

typedef struct sdn_hal_table_supported_action_s
{
    EN_OFP_ACTION_TYPE_T  actions;
    uint32 table_id_bitmap;
}sdn_hal_table_supported_action_t;

typedef struct sdn_hal_table_supported_set_field_s
{
    EN_OFP_OXM_OFB_MATCH_FIELDS_T set_table_field;
    uint32 table_id_bitmap;
}sdn_hal_table_supported_set_field_t;

typedef struct sdn_hal_table_supported_inst_s
{
    EN_OFP_INSTRUCTION_TYPE_T  inst_action;
    uint32 table_id_bitmap;
}sdn_hal_table_supported_inst_t;

typedef struct sdn_ft0_3_4_match_s
{
    rtk_of_matchfieldType_t type;
    uint8 *pData;
    uint8 *pMask;
}sdn_ft0_3_4_match_t;
typedef struct ft0_3_4_flow_s
{
    rtk_of_flowIns_t ft0_3_4_ins;
    sdn_ft0_3_4_match_t ft0_3_4_match;
    uint32 match_len;
}ft0_3_4_flow_t;

typedef struct ft2_flow_s
{
    rtk_of_l3HashFlowEntry_t ft2_hash_flow;
    rtk_of_l3CamFlowEntry_t  ft2_cam_flow;
} ft2_flow_t;

typedef union sdn_hal_flow_u
{
    ft0_3_4_flow_t          ft0_flow;
    rtk_of_l2FlowEntry_t    ft1_flow;
    ft2_flow_t              ft2_flow;
    ft0_3_4_flow_t          ft3_flow;
    ft0_3_4_flow_t          ft4_flow;
}sdn_hal_flow_t;

/*
 * Data Type Declaration
 */
typedef enum sdn_hal_board_conf_phyPort_e
{
    PHY_PORT_0 = 0,
    PHY_PORT_1,
    PHY_PORT_2,
    PHY_PORT_3,
    PHY_PORT_4,
    PHY_PORT_5,
    PHY_PORT_6,
    PHY_PORT_7,
    PHY_PORT_8,
    PHY_PORT_9,
    PHY_PORT_10,
    PHY_PORT_11,
    PHY_PORT_12,
    PHY_PORT_13,
    PHY_PORT_14,
    PHY_PORT_15,
    PHY_PORT_16,
    PHY_PORT_17,
    PHY_PORT_18,
    PHY_PORT_19,
    PHY_PORT_20,
    PHY_PORT_21,
    PHY_PORT_22,
    PHY_PORT_23,
    PHY_PORT_24,
    PHY_PORT_25,
    PHY_PORT_26,
    PHY_PORT_27,
    PHY_PORT_28,
    PHY_PORT_29,
    PHY_PORT_30,
    PHY_PORT_31,
    PHY_PORT_32,
    PHY_PORT_33,
    PHY_PORT_34,
    PHY_PORT_35,
    PHY_PORT_36,
    PHY_PORT_37,
    PHY_PORT_38,
    PHY_PORT_39,
    PHY_PORT_40,
    PHY_PORT_41,
    PHY_PORT_42,
    PHY_PORT_43,
    PHY_PORT_44,
    PHY_PORT_45,
    PHY_PORT_46,
    PHY_PORT_47,
    PHY_PORT_48,
    PHY_PORT_49,
    PHY_PORT_50,
    PHY_PORT_51,
    PHY_PORT_52,
    PHY_PORT_53,
    PHY_PORT_54,
    PHY_PORT_55,
    PHY_PORT_56,
    PHY_PORT_57,
    PHY_PORT_END,
} sdn_hal_board_conf_phyPort_t;

typedef enum sdn_hal_board_conf_usrPort_e
{
    USR_PORT_1 = 1,
    USR_PORT_2,
    USR_PORT_3,
    USR_PORT_4,
    USR_PORT_5,
    USR_PORT_6,
    USR_PORT_7,
    USR_PORT_8,
    USR_PORT_9,
    USR_PORT_10,
    USR_PORT_11,
    USR_PORT_12,
    USR_PORT_13,
    USR_PORT_14,
    USR_PORT_15,
    USR_PORT_16,
    USR_PORT_17,
    USR_PORT_18,
    USR_PORT_19,
    USR_PORT_20,
    USR_PORT_21,
    USR_PORT_22,
    USR_PORT_23,
    USR_PORT_24,
    USR_PORT_25,
    USR_PORT_26,
    USR_PORT_27,
    USR_PORT_28,
    USR_PORT_29,
    USR_PORT_30,
    USR_PORT_31,
    USR_PORT_32,
    USR_PORT_33,
    USR_PORT_34,
    USR_PORT_35,
    USR_PORT_36,
    USR_PORT_37,
    USR_PORT_38,
    USR_PORT_39,
    USR_PORT_40,
    USR_PORT_41,
    USR_PORT_42,
    USR_PORT_43,
    USR_PORT_44,
    USR_PORT_45,
    USR_PORT_46,
    USR_PORT_47,
    USR_PORT_48,
    USR_PORT_49,
    USR_PORT_50,
    USR_PORT_51,
    USR_PORT_52,
    USR_PORT_53,
    USR_PORT_54,
    USR_PORT_55,
    USR_PORT_56,
    USR_PORT_57,
    USR_PORT_58 = SDN_HAL_CPU_PORT,
    USR_PORT_END,
} sdn_hal_board_conf_usrPort_t;

typedef enum sdn_hal_return_e
{
    SDN_HAL_RETURN_FAILED = 0,
    SDN_HAL_RETURN_OK,
    SDN_HAL_RETURN_NOT_SUPPORTED,
    SDN_HAL_RETURN_NULL_DATA,
    SDN_HAL_RETURN_ERR_INPUT,
    SDN_HAL_RETURN_INVALID_PARAM,
    SDN_HAL_RETURN_INVALID_METADATA,
    SDN_HAL_RETURN_UNSUPPORTED_INS,
    SDN_HAL_RETURN_UNSUPPORTED_MASK,
    SDN_HAL_RETURN_BAD_PORT,
}sdn_hal_return_t;

typedef struct sdn_hal_port_linkChange_s
{
    sdn_hal_portmask_t portmask[RTK_MAX_NUM_OF_UNIT];
} sdn_hal_port_linkChange_t;

typedef struct sdn_hal_port_config_s
{
    uint32_t config;
    uint32_t config_mask;
} sdn_hal_port_config_t;

typedef struct sdn_hal_port_cfg_s
{
    uint32 usrPort;
    uint32 devId;
    uint32 phyPort;
} sdn_hal_port_cfg_t;

#ifdef CONFIG_SDK_RTL9310
typedef struct sdn_hal_action_s
{
    uint16 act_type;
    uint8  oxm_type;
    uint128_t data;
}sdn_hal_action_t;

typedef struct sdn_hal_apply_actions_s
{
    uint32 bucket_id;
    uint16 action_len;
    sdn_hal_action_t *action_p;
    struct sdn_hal_apply_actions_s *next_act_p;
}sdn_hal_apply_actions_t;

typedef struct sdn_hal_group_table_s
{
    uint32 group_id;
    uint32 action_buket_num;
    uint64 group_counter;
    sdn_hal_apply_actions_t *action_buket_p;
    rtk_of_groupType_t group_type;
}sdn_hal_group_table_t;
#endif
typedef enum  sdn_hal_rule_stats_type_e
{
    HAL_RULE_STATS_TYPE_PACKET_COUNTER = 0,
    HAL_RULE_STATS_TYPE_BYTE_COUNTER,
    HAL_RULE_STATS_TYPE_END,
} sdn_hal_rule_stats_type_t;

typedef struct hal_port_stats_s
{
    uint64_t  rx_packets;
    uint64_t  tx_packets;
    uint64_t  rx_bytes;
    uint64_t  tx_bytes;
    uint64_t  rx_drops;
    uint64_t  tx_drops;
    uint64_t  rx_errors;
    uint64_t  tx_errors;
    uint64_t  rx_frame_align_err;
    uint64_t  rx_overrun_errors;
    uint64_t  rx_crc_errors;
    uint64_t  collisions;
}hal_port_stats_t;

/************************************
 * Table
 */
typedef enum sdn_hal_rule_table_type_e
{
    SDN_HAL_RULE_TABLE_IGR = 0,
    SDN_HAL_RULE_TABLE_EGR,
    SDN_HAL_RULE_MAX_TABLE_NUM
}sdn_hal_rule_table_type_t;

typedef struct sdn_hal_rule_counter_mgmt_s
{
    BOOL_T is_apply_counter;
    uint32 counter_id;
    sdn_hal_rule_stats_type_t counter_type;
}sdn_hal_rule_counter_mgmt_t;

typedef struct sdn_hal_rule_meter_mgmt_s
{
    uint32 meter_id;
    uint32 meter_flags;
    uint32 meter_band_type;
    uint32 meter_rate;
    uint32 meter_burst_size;
}sdn_hal_rule_meter_mgmt_t;

typedef struct sdn_hal_rule_flow_ratelimit_s
{
    sdn_hal_rule_meter_flag_type_t mode;
    sdn_hal_rule_meter_band_type_t band_type;
    uint32 meter_id;
    uint32 rate;
    uint32 burstSize;
}sdn_hal_rule_flow_ratelimit_t;

typedef struct sdn_hal_tpid_s
{
    uint8  validate;
    uint32 ether_type;
}sdn_hal_tpid_t;

typedef struct sdn_hal_rule_flow_action_s
{
    BOOL_T   port_list_is_used;
    BOOL_T   is_normal_output;
    BOOL_T   phy_in_port;
    BOOL_T   flood_port;
    BOOL_T set_remark_val;
    BOOL_T set_pri;
    BOOL_T set_src_mac;
    BOOL_T set_dst_mac;
    BOOL_T set_l4_src_port;
    BOOL_T set_l4_dst_port;
    BOOL_T set_svid;
    BOOL_T set_src_ip;
    BOOL_T set_dst_ip;
    uint8  remark_val;
    uint8  mirror_id;
    uint8  pri;
    uint8  ttl;
    uint8  src_mac[SDN_HAL_RULE_ETH_ALEN];
    uint8  dst_mac[SDN_HAL_RULE_ETH_ALEN];
    uint16 l4_src_port;
    uint16 l4_dst_port;
    uint32 action_mask; /*ACTIONS Mask*/
    uint32 counter_id;
    uint32 cvid;
    uint32 svid;
    uint32 port_no;
    uint32 controller_port;
    uint32 eth_type;
    uint32 qid;
    uint32 src_ip;
    uint32 dst_ip;
    uint32 group_id;
    rtk_portmask_t port_list;
    EN_OFP_OXM_OFB_MATCH_FIELDS_T ofp_oxm_type;
    sdn_hal_rule_flow_ratelimit_t rate_limit_data;
}sdn_hal_rule_flow_action_t;

typedef struct sdn_hal_rule_match_fields_data_s
{
    uint8  protocol_val;
    uint8  src_mac[SDN_HAL_RULE_ETH_ALEN];
    uint8  dst_mac[SDN_HAL_RULE_ETH_ALEN];
    uint8  src_mac_mask[SDN_HAL_RULE_ETH_ALEN];
    uint8  dst_mac_mask[SDN_HAL_RULE_ETH_ALEN];
    uint8  ipv6_src_addr[SDN_HAL_RULE_MAX_IPV6_ADDR_LEN];
    uint8  ipv6_dst_addr[SDN_HAL_RULE_MAX_IPV6_ADDR_LEN];
    uint8  ipv6_src_mask[SDN_HAL_RULE_MAX_IPV6_ADDR_LEN];
    uint8  ipv6_dst_mask[SDN_HAL_RULE_MAX_IPV6_ADDR_LEN];
    uint8  src_port_list[SDN_HAL_RULE_LOGICPORT_NUM_MAX]; /*TODO: FILL LOGIC PORT MAX NUM*/
    uint8  dst_port_list[SDN_HAL_RULE_LOGICPORT_NUM_MAX]; /*TODO: FILL LOGIC PORT MAX NUM*/
    uint8  dscp;
    uint8  ip_ecn;
    uint8  icmp_type;
    uint8  icmp_code;
    uint8  pcp_value;
    uint8  pcp_mask;
    uint16 svid;
    uint16 cvid;
    uint16 eth_type_val;
    uint16 tcp_src_port_no;
    uint16 tcp_dst_port_no;
    uint16 udp_src_port_no;
    uint16 udp_dst_port_no;
    uint16 sctp_src_port_no;
    uint16 sctp_dst_port_no;
    uint16 arp_op;
    uint32 tunnel_id;
    uint32 tunnel_id_mask;
    uint32 src_ipv4_addr;
    uint32 src_ipv4_mask;
    uint32 dst_ipv4_addr;
    uint32 dst_ipv4_mask;
    uint32 vlan_rang_id;
    uint16 metadata;
    uint16 metadata_mask;
    uint32 l_port;
    /* Below elements are added for MEC DPA */
    uint32 gtp_teid;
    uint32 gtp_teid_mask;
    uint32 outer_ip_ver;
    uint32 outer_ip_ver_mask;
    uint8  outer_tcp_flag;
    uint8  outer_tcp_flag_mask;
    uint8  gtp_mt;
    uint8  gtp_mt_mask;
    uint8  outer_ip_flag;
    uint8  outer_ip_flag_mask;
    uint32 inner_ip_ver;
    uint32 inner_ip_ver_mask;
    uint32 inner_l4dp;
    uint32 inner_l4dp_mask;
    uint32 inner_sip;
    uint32 inner_sip_mask;
    uint32 inner_dip;
    uint32 inner_dip_mask;
    uint8  outer_frag_pkt;
    uint8  outer_frag_pkt_mask;
    uint8  inner_ip_proto;
    uint8  inner_ip_proto_mask;
    uint16 inner_tcp_flag;
    uint16 inner_tcp_flag_mask;
    uint16 range_chk_index;
    uint16 range_chk_index_mask;
}sdn_hal_rule_match_fields_data_t;

typedef struct sdn_hal_rule_flow_match_fields_s
{
    sdn_hal_rule_match_fields_data_t field_data;
    uint64 match_fields_bitmap;
    uint64 match_fields_bitmap_agg[SDN_HAL_MAX_NUM_BLOCK_TEMPLATE];
}sdn_hal_rule_flow_match_fields_t;

typedef struct sdn_hal_rule_flow_mgmt_s
{
    uint8 table_id;                 //Brandon: transfer table_id to HAL.
    uint8 pri;
    uint8 flow_ag;
    uint32 chosen_template_id;
    uint32 instruction_mask;
    uint32 flow_id;
    uint32 meter_id;
    uint32 goto_table_id;
    uint32 clear_action;
    uint32 metadata;
    uint32 metadata_mask;
    sdn_hal_rule_counter_mgmt_t      counter_data;
    sdn_hal_rule_flow_action_t       action_data;
    sdn_hal_rule_flow_match_fields_t match_fields;
}sdn_hal_rule_flow_mgmt_t;

typedef struct sdn_hal_tempate_info_s
{
    rtk_of_ftTemplateIdx_t template_id;
    rtk_pie_template_t template[SDN_HAL_MAX_NUM_BLOCK_TEMPLATE];
    uint64 template_bitmap[SDN_HAL_MAX_NUM_BLOCK_TEMPLATE];
}sdn_hal_tempate_info_t;

typedef struct sdn_hal_flow_table_feature_s
{
    uint32 instruction_bit_map; /* map to EN_OFP_INSTRUCTION_TYPE_T*/
    uint32 flow_table_id;
    uint32 max_actions;
    uint32 max_match_fields;
    uint32 action_bit_map; /* no large than 32 bit*/
    uint32 max_entry_nbr;
    uint32 next_table_id;
    uint32 block_id_start;
    uint64 set_table_fields_bit_map;
    uint64 match_fields_bit_map;
    uint64 wildcard_fields_bit_map;
    sdn_hal_tempate_info_t template_info;
}sdn_hal_flow_table_feature_t;

typedef struct sdn_hal_queue_stat_s
{
    uint32_t out_pkt_queue_id;
    uint32_t out_pkt_drop_id;
}sdn_hal_queue_stat_t;

//new group code
typedef struct sdn_hal_group_feature_s
{
    EN_OFP_GROUP_TYPE_T group_type;
    uint32 actions_feature;
}sdn_hal_group_feature_t;
//new group code

/*
 * Function Declaration
 */
/* Tx packet */
int sdn_hal_pkt_tx(char *dev_name,
                  void *pkt_p,
                  uint32_t pkt_size,
                  rtk_of_outputType_t *out_put_type,
                  uint32_t port_no);

int sdn_hal_init(struct ofp_table_features tf[MAX_TABLE_NUM]);
int32 sdn_hal_destroy(void);
/* Table Feature */
int32 sdn_hal_table_features_set(uint8_t n_table, const struct ofp_table_features *tf);
int32 sdn_hal_table_features_get(uint32 table_id, sdn_hal_flow_table_feature_t *table_features_p);

#ifdef  CONFIG_SDK_RTL9310
//new group code
int32 sdn_hal_add_groupentry(sdn_db_group_entry_t *group_entry_p);
int32 sdn_hal_del_groupentry(sdn_db_group_entry_t *group_entry_p);
//new group code
int32 sdn_hal_get_groupfeature(EN_OFP_GROUP_TYPE_T type);
int32 sdn_hal_add_group(uint32 group_id, rtk_of_groupType_t type, sdn_hal_group_table_t *entry_p);
int32 sdn_hal_get_group(uint32 group_id, rtk_of_groupType_t *type, uint32 *bucket_num_p, sdn_hal_apply_actions_t **applied_action_list_p);
int32 sdn_hal_delete_group(uint32 group_id);

/* Classifier */
int sdn_hal_classifier_set(rtk_of_classifierType_t type, rtk_of_classifierData_t data);

int sdn_hal_set_classifier(rtk_of_classifierType_t type, rtk_of_classifierData_t data);
#endif
int32 sdn_hal_logicalPort2PhyPort(uint32 logical_port, uint32 *phy_port_p);
int32 sdn_hal_usrPort_unit_get(uint32 port_no, uint32_t *unit);
int32 sdn_hal_phyPort2LogicalPort(uint32 unit, uint32 phy_port, uint32 *logical_port_p);
int32 sdn_hal_phyPort2UserPort(uint32 unit, uint32 phy_port, uint32 *user_port_p);
int32 sdn_hal_userPort2PhyPort(uint32 user_port, uint32 *phy_port_p);
int32 sdn_hal_logicalPort2UserPort(uint32 unit, uint32 logical_port, uint32 *user_port_p);
int32 sdn_hal_userPort2LogicalPort(uint32 unit, uint32 user_port, uint32 *logical_port_p);
int32 sdn_hal_get_flow_stats(uint8_t table_id, uint32_t flow_id, uint32_t *pCount_id);
int32 sdn_hal_port_init(void);
int32 sdn_hal_portLinkChange_get(sdn_hal_port_linkChange_t *change);
int32 sdn_hal_port_max_get(void);
int32 sdn_hal_cpu_port_get(void);
int32 sdn_hal_port_state_get(uint32_t port_no, uint32_t *port_state);
int sdn_hal_port_stats_get(uint32 port_no, hal_port_stats_t *pPort_stats_data);
int32 sdn_hal_port_exist(uint32 port_no);
int32 sdn_hal_port_config_set(uint32 port_no, sdn_hal_port_config_t *port_config);
int32 sdn_hal_port_add(uint32 port_no);
int32 sdn_hal_port_delete(uint32 port_no);
int32 sdn_hal_port_nbr_get(void);
int32 sdn_hal_port_feature_get(uint32 port_no, sdn_db_port_feature_t *pFeature);
int32 sdn_hal_eth_addr_get(uint32 port_no, uint8 mac_addr[ETH_ADDR_LEN]);
int32 sdn_hal_flow_entry_add(sdn_db_flow_entry_t *flow_entry_p);
int32 sdn_hal_flow_entry_modify(sdn_db_flow_entry_t *flow_entry_p);
int32 sdn_hal_flow_entry_delete(sdn_db_flow_entry_t *flow_entry_p);
int32 sdn_hal_l2andl3flowentry_hitstatus_get(sdn_db_flow_entry_t *flow_entry_p, uint32_t *hit_status_p);
//int32 sdn_hal_flow_entry_add(struct ofp_flow_mod *fm, uint8_t match_num, uint8_t inst_num, struct ofp_instruction_header *inst);
#if 0
int32 sdn_hal_flow_entry_modify(struct ofp_flow_mod *fm, uint8_t match_num, uint8_t inst_num, struct ofp_instruction_header *inst);
int32 sdn_hal_flow_entry_delete(struct ofp_flow_delete *fm, uint8_t match_num);
#endif
int32 sdn_hal_switch_features_get(sdn_db_switch_features_t *features);
int32 sdn_hal_flow_entry_counter_apply(uint8 table_id, uint32 flow_id, uint32 count_id, uint32_t counter_flag);
int32 sdn_hal_rule_get_flow_table(uint32 tableId);
int32 sdn_hal_flowId_asicId_transform(uint32 table_id, uint32 flow_id, uint32 *asic_id);
int32 sdn_hal_flow_aclField_fill(sdn_hal_rule_flow_mgmt_t *flow_entry_p);
int32 sdn_hal_flow_matchField_fill(rtk_of_flowtable_phase_t phase, sdn_hal_rule_flow_mgmt_t *flow_entry_p, sdn_hal_flow_t *flow);
int32 sdn_hal_flow_action_fill(rtk_of_flowtable_phase_t phase, sdn_hal_rule_flow_mgmt_t *flow_entry_p, sdn_hal_flow_t *flow_p);
int32 sdn_hal_flow_entry_move(uint8 table_id, uint32 flow_index_dst, uint32 flow_index_src, uint32 total_flows);
int32 sdn_hal_table_features_get(uint32 table_id, sdn_hal_flow_table_feature_t *table_features_p);
int32 sdn_hal_meter_entry_add(const sdn_db_meter_mod_t *meter_entry);
int32 sdn_hal_meter_entry_del(uint32 meter);
int32 sdn_hal_meter_entry_modify(const sdn_db_meter_mod_t *meter_entry);
int32 sdn_hal_meter_table_features_get(sdn_db_meter_table_features_t *feature);
int32 sdn_hal_table_miss_action_set(uint8_t table_id, rtk_of_tblMissAct_t action);
int32 sdn_hal_flow_hitstatus_get(uint8_t table_id, uint32_t flow_id, uint32_t *hit_status_p);
//int32 sdn_hal_flow_aclAction_fill(sdn_hal_rule_flow_mgmt_t *flow_entry_p, rtk_acl_action_t *acl_action_p);
int sdn_hal_flow_stats_periodic_get(uint8 table_id,
    uint32 flow_id,
    uint64_t *packet_count,
    uint64_t *byte_count);
int sdn_hal_flow_entry_counter_delete(uint8 table_id,
                                                  uint32 flow_id,
                                                  uint32 count_id);
int sdn_hal_flow_entry_counter_clear(uint8 table_id,
                                     uint32 flow_id,
                                     uint32 count_id);
/* Enhance by MEC DPA */
int sdn_hal_flow_entry_counter_get(uint8 table_id,
                                     uint32 flow_id,
                                     uint64 *pPacketCnt,
                                     uint64 *pByteCnt);


int sdn_hal_flow_table_stats_get(uint8 table_id, rtk_of_flowTblCntType_t type, uint32 *pCnt);
//qos code
int sdn_hal_set_queueconfig(sdn_db_qos_entry_t qos_entry);
int sdn_hal_set_port_queueconfig(uint32_t phy_port, sdn_db_qos_entry_t qos_entry);
int sdn_hal_del_port_queueconfig(uint32_t phy_port,  sdn_db_qos_entry_t qos_entry);
int sdn_hal_get_port_queueconfig(uint32_t port, uint32_t queue_id, uint32_t *max_rate_p, uint32_t *min_rate_p);
int sdn_hal_get_port_queuestats(uint32_t port, uint32_t queue_id, uint32_t *tx_packets_p, uint32_t *tx_error_p);

/* Add by RTKSD6*/
int32 sdn_hal_table_table_template_bind(uint32 unit);
int32 sdn_hal_flow_instruction_fill(rtk_of_flowtable_phase_t phase, sdn_hal_rule_flow_mgmt_t *flow_entry_p, sdn_hal_flow_t *flow_p);
int32 sdn_hal_flow_set(rtk_of_flowtable_phase_t phase, sdn_hal_rule_flow_mgmt_t *flow_entry_p, sdn_hal_flow_t *flow);

#endif /*SDN_HAL_H*/
