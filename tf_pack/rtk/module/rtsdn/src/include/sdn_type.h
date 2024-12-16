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

#ifndef _SDN_TYPE_H_
#define _SDN_TYPE_H_
#include <stdio.h>
#include <stdlib.h>
#include <common/rt_type.h>
#include <osal/lib.h>
#include "ofp_common_enum.h"
#include <rtk/pie.h>

#if 1
#ifndef bool
#define bool unsigned char
#endif

#ifndef uint32_t
#define uint32_t unsigned int
#endif

#ifndef uint8_t
#define uint8_t unsigned char
#endif

#ifndef uint16_t
#define uint16_t unsigned short
#endif

#ifndef uint64_t
#define uint64_t unsigned long long
#endif
#endif

#ifndef uint128_t
typedef struct uint128_s
{
    uint64_t high;
    uint64_t low;
}uint128_t;
#endif

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#ifndef BOOL_T
#define BOOL_T unsigned char
#define TRUE 1
#endif

#ifndef _countof
#define _countof(ar_) (sizeof(ar_) / sizeof(*ar_))
#endif

#ifndef MAX_TABLE_NUM
#define MAX_TABLE_NUM 1
#endif

#ifndef MIN_TABLE_NUM
#define MIN_TABLE_NUM 1
#endif

#ifdef  CONFIG_SDK_RTL9310
#define DFLT_SDN_CLASSIFIER_TYPE OF_CLASSIFIER_TYPE_PORT
#endif

#define UINT64MAX_VAL 0xffffffffffffffff
#define UINT32MAX_VAL 0xffffffff
#define UINT16MAX_VAL 0xffff

#define SDN_FLAG_IS_ON(_flag, _val) (_flag & (1 << _val))
#define SDN_FLAG_ON(_flag, _val) (_flag |= (1 << _val))
#define SDN_FLAG_OFF(_flag, _val) (_flag &= (~(1 << _val)))
#define SDN_MAX_QUEUE_ID 8
#define SDN_MIN_QUEUE_ID 0

typedef enum sdn_hal_rule_meter_band_type_e
{
    SDN_HAL_RULE_METER_BAND_DROP   = 0,
    SDN_HAL_RULE_METER_BAND_REMARK = 1,
    SDN_HAL_RULE_MAX_METER_BANDS,
}sdn_hal_rule_meter_band_type_t;

/*METER FLAGS MAP TO METER MODE
 */
typedef enum sdn_hal_rule_meter_flag_type_e
{
    SDN_HAL_RULE_METER_FLAG_KBPS   = METER_MODE_BYTE,
    SDN_HAL_RULE_METER_FLAG_PKTPS  = METER_MODE_PACKET,
    SDN_HAL_RULE_METER_BURST,
    SDN_HAL_RULE_MAX_METER_FLAGS,
}sdn_hal_rule_meter_flag_type_t;


typedef enum sdn_db_group_type_e
{
    SDN_DB_GROUP_TYPE_ALL,
    SDN_DB_GROUP_TYPE_SELECT,
    SDN_DB_GROUP_TYPE_END,
}sdn_db_group_type_t;

typedef enum sdn_db_qos_type_e
{
    SDN_DB_QOS_TYPE_HTB,
    SDN_DB_QOS_TYPE_HFSC,
    SDN_DB_QOS_TYPE_DEFAULT,
    SDN_DB_QOS_TYPE_OTHER,
    SDN_DB_QOS_TYPE_MAX,
}sdn_db_qos_type_t;
/*Brandon: Fields to match against flows Acopy from openflow-1.1.h*/
typedef struct ofp_match_header_s {
    uint16_t type;             /* One of OFPMT_* */
    uint16_t length;           /* Length of match */
}ofp_match_header;

typedef struct ofp_match_s{
    ofp_match_header omh;
    uint32_t in_port;          /* Input switch port. */
    uint32_t wildcards;        /* Wildcard fields. */
    uint8_t dl_src[OFP_ETH_ALEN]; /* Ethernet source address. */
    uint8_t dl_src_mask[OFP_ETH_ALEN]; /* Ethernet source address mask.  */
    uint8_t dl_dst[OFP_ETH_ALEN]; /* Ethernet destination address. */
    uint8_t dl_dst_mask[OFP_ETH_ALEN]; /* Ethernet destination address mask. */
    uint16_t dl_vlan;          /* Input VLAN id. */
    uint8_t dl_vlan_pcp;       /* Input VLAN priority. */
    uint8_t pad1[1];           /* Align to 32-bits */
    uint16_t dl_type;          /* Ethernet frame type. */
    uint8_t nw_tos;            /* IP ToS (actually DSCP field, 6 bits). */
    uint8_t nw_proto;          /* IP protocol or lower 8 bits of ARP opcode. */
    uint32_t nw_src;           /* IP source address. */
    uint32_t nw_src_mask;      /* IP source address mask. */
    uint32_t nw_dst;           /* IP destination address. */
    uint32_t nw_dst_mask;      /* IP destination address mask. */
    uint16_t tp_src;           /* TCP/UDP/SCTP source port. */
    uint16_t tp_dst;           /* TCP/UDP/SCTP destination port. */
    uint32_t mpls_label;       /* MPLS label. */
    uint8_t mpls_tc;           /* MPLS TC. */
    uint8_t pad2[3];           /* Align to 64-bits */
    uint64_t metadata;         /* Metadata passed between tables. */
    uint64_t metadata_mask;    /* Mask for metadata. */
}ofp_match;

struct ofp_table_feature_prop_header
{
    uint16_t type;              /* One of OFPTFPT_*. */
    uint16_t length;            /* Length in bytes of this property. */
};

struct ofp_table_features
{
    char     name[OFP_MAX_TABLE_NAME_LEN];
    uint8_t  table_id;          /* Identifier of table. Lower numbered tables are consulted first. */
    uint8_t  command;           /* One of OFPTFC_*. */
    uint16_t length;            /* Length is padded to 64 bits. */
    uint32_t max_entries;       /* Max number of entries supported. */
    uint32_t features;          /* Bitmap of OFPTFF_* values. */
    uint32_t config;            /* Bitmap of OFPTC_* values */
    uint64_t metadata_match;    /* Bits of metadata table can match. */
    uint64_t metadata_write;    /* Bits of metadata table can write. */
    /* Table Feature Property list */
    uint16_t inst_bitmap;
    uint16_t inst_miss_bitmap;
    uint16_t next_tables_bitmap;
    uint16_t next_tables_miss_bitmap;
    uint64_t write_actions_bitmap;
    uint64_t write_actions_miss_bitmap;
    uint64_t apply_actions_bitmap;
    uint64_t apply_actions_miss_bitmap;
    uint64_t match_bitmap;
    uint64_t wildcards_bitmap;
    uint64_t write_setfield_bitmap;
    uint64_t write_setfield_miss_bitmap;
    uint64_t apply_setfield_bitmap;
    uint64_t apply_setfield_miss_bitmap;
    //struct ofp_table_feature_prop_header properties[0]; /* List of properties */
};

/* Meter band configuration for all supported band types. */
typedef struct sdn_db_meter_band_s {
    uint8_t  prec_level;         /* Non-zero if type == OFPMBT_DSCP_REMARK. */
    uint16_t type;
    uint32_t rate;
    uint32_t burst_size;
}sdn_db_meter_band_t;

typedef struct sdn_db_meter_mod_s
{
    uint32_t meter_id;
    uint16_t flags;
    uint16_t n_bands;
    sdn_db_meter_band_t *bands;
}sdn_db_meter_mod_t;

typedef struct sdn_db_match_field_s
{
    uint16_t oxm_class;
    uint16_t oxm_type;
    uint128_t value;
    uint128_t mask;
}sdn_db_match_field_t;

typedef struct sdn_db_instruction_s
{
    uint16_t type;
    void     *data_p;
}sdn_db_instruction_t;

typedef struct sdn_db_flow_entry_idlestatus_s
{
    uint32_t idle_time;
    uint32_t used_time;
    uint32_t hit_status;
}sdn_db_flow_entry_idlestatus_t;

/*flow entry*/
typedef struct sdn_db_flow_entry_s
{
    uint8_t  table_id;
    uint8_t  chosen_template_id;
    uint16_t priority;
    uint16_t flags;
    uint32_t len_match;
    uint32_t len_inst;
    sdn_db_match_field_t *match_field_p;
    sdn_db_instruction_t *instruction_p;
    uint32_t flow_index;
    uint32_t counter_index;
    uint32_t hash_val;
    uint32_t hard_timeout;
    sdn_db_flow_entry_idlestatus_t idle_timeout;
}sdn_db_flow_entry_t;

typedef struct sdn_db_port_stats_s
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
} sdn_db_port_stats_t;

typedef struct sdn_db_meter_table_features
{
    uint32    band_types;
    uint32    capabilities;
    uint32    max_meters;
    uint8     max_bands;
    uint8     max_colors;
} sdn_db_meter_table_features_t;

typedef struct sdn_db_meter_table_s
{
    sdn_db_meter_table_features_t   meter_features;
    sdn_db_meter_mod_t              *meter_entry;
} sdn_db_meter_table_t;

/* Queue */
typedef struct sdn_db_queue_s
{
    uint16_t min_rate;
    uint16_t max_rate;
    uint32_t queue_id;
}sdn_db_queue_t;

/* Port */
typedef enum sdn_ofp_port_config_e
{
    EN_OFPPC_PORT_DOWN          = 1 << 0,
    EN_OFPPC_NO_STP             = 1 << 1,
    EN_OFPPC_NO_RECV            = 1 << 2,
    EN_OFPPC_NO_RECV_STP        = 1 << 3,
    EN_OFPPC_NO_FLOOD           = 1 << 4,
    EN_OFPPC_NO_FWD             = 1 << 5,
    EN_OFPPC_NO_PACKET_IN       = 1 << 6,
    /*
     * EstiNet defined
     */
    EN_OFPPC_PORT_BIDIR         = 1 << 9,
    EN_OFPPC_PORT_INGRESS       = 1 << 10,
    EN_OFPPC_PORT_EGRESS        = 1 << 11,
}sdn_ofp_port_config_t;

typedef enum sdn_ofp_port_features
{
    EN_OFPPF_10MB_HD        = 1 << 0,
    EN_OFPPF_10MB_FD        = 1 << 1,
    EN_OFPPF_100MB_HD       = 1 << 2,
    EN_OFPPF_100MB_FD       = 1 << 3,
    EN_OFPPF_1GB_HD         = 1 << 4,
    EN_OFPPF_1GB_FD         = 1 << 5,
    EN_OFPPF_10GB_FD        = 1 << 6,
    EN_OFPPF_40GB_FD        = 1 << 7,
    EN_OFPPF_100GB_FD       = 1 << 8,
    EN_OFPPF_1TB_FD         = 1 << 9,
    EN_OFPPF_OTHER          = 1 << 10,
    EN_OFPPF_COPPER         = 1 << 11,
    EN_OFPPF_FIBER          = 1 << 12,
    EN_OFPPF_AUTONEG        = 1 << 13,
    EN_OFPPF_PAUSE          = 1 << 14,
    EN_OFPPF_PAUSE_ASYM     = 1 << 15,
}sdn_ofp_port_feature_t;

typedef struct sdn_db_port_feature_s
{
    uint32_t curr;
    uint32_t advertised;
    uint32_t supported;
    uint32_t peer;
}sdn_db_port_feature_t;

typedef struct sdn_db_port_config_s
{
    sdn_ofp_port_config_t config;
    uint32_t config_mask;
} sdn_db_port_config_t;

typedef struct sdn_db_port_s
{
    uint8_t     hw_addr[OFP_ETH_ALEN];
    uint8_t     name[OFP_MAX_PORT_NAME_LEN];
    uint32_t    port_no;
    uint32_t    exist;
    uint32_t    ofp_exist;
    uint32_t    state;
    sdn_db_port_stats_t statistic;
    sdn_db_port_config_t config;
    sdn_db_port_feature_t feature;
}sdn_db_port_t;


typedef struct sdn_db_switch_features_s {
    bool     virtual_table_exist;
    bool     is_outband;
    uint8_t  n_tables;
    uint32_t capabilities;
    uint64_t datapath_id;
}sdn_db_switch_features_t;

typedef struct sdn_db_action_s
{
    uint8_t         oxm_type;
    uint16_t        act_type;
    uint128_t       value;
} sdn_db_action_t;

typedef struct sdn_db_action_bucket_data_s
{
    uint32_t        action_len;
    sdn_db_action_t *actions_p;
}sdn_db_bucket_data_t;

typedef struct sdn_db_group_action_bucket_s
{
    uint32_t group_id;
    uint32_t action_len;
    sdn_db_action_t *actions_p;
    //sdn_db_bucket_data_t action_data;
}sdn_db_group_action_bucket_t;

typedef struct sdn_db_group_entry_s
{
    uint32_t entry_id;
    EN_OFP_GROUP_TYPE_T group_type;
    uint32_t counters;
    uint32_t bucket_len;
    sdn_db_group_action_bucket_t *action_bucket_p;
}sdn_db_group_entry_t;

typedef struct sdn_db_group_tbl_s
{
    uint32_t total_counter;
    uint32_t total_bucket_num;
    sdn_db_group_entry_t *group_entry_p;
}sdn_db_group_tbl_t;

typedef struct sdn_db_qos_entry_s
{
    bool     is_applied;
    uint32_t qid;
    sdn_db_qos_type_t qos_type;
    uint32_t burst_size;
    uint32_t priority;
    uint32_t max_rate;
    uint32_t min_rate;
    uint32_t low_dst_port_bitmap;
    uint32_t hi_dst_port_bitmap;
}sdn_db_qos_entry_t;

#endif
