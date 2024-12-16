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
#ifndef SDN_DB_H
#define SDN_DB_H
#include <stdio.h>
#include <stdlib.h>
//#include "ofp_common_enum.h"
#include <sdn_type.h>
//#include "sdn_hal.h"
#include <sdn_hal.h>
#include "common/error.h"
#include "rtk/openflow.h"

/*
 * Symbol Definition
 */
typedef enum sdn_db_return_e
{
    SDN_DB_RETURN_FAILED        = -1,
    SDN_DB_RETURN_OK            = 0,
    SDN_DB_RETURN_NOT_FOUND,
    SDN_DB_RETURN_INVALID_PARAM,
    SDN_DB_RETURN_NOT_SUPPORTED,
    SDN_DB_RETURN_TABLE_EMPTY,
    SDN_DB_RETURN_TABLE_FULL,
    SDN_DB_RETURN_FLOW_EXIST,
    SDN_DB_RETURN_OUT_OF_FLOW_RESOURCES,
    SDN_DB_RETURN_OUT_OF_METER_RESOURCES,
    SDN_DB_RETURN_INVALID_CALLBACK,
    SDN_DB_RETURN_MALLOC_FAIL,
    SDN_DB_RETURN_OHAL_FAIL,
    SDN_DB_RETURN_MAX,
}sdn_db_return_t;

typedef struct sdn_db_flow_table_s
{
    struct ofp_table_features table_feature;
    sdn_db_flow_entry_t       *flow_entry;
    uint32                    flow_count;
    uint32                    miss_action;
} sdn_db_flow_table_t;

typedef struct sdn_db_inst_meter_s
{
    uint32_t        meter_id;
} sdn_db_inst_meter_t;

#if 0
//move to sdn_type.h
typedef struct sdn_db_action_s
{
    uint8_t         oxm_type;
    uint16_t        act_type;
    uint128_t       value;
} sdn_db_action_t;
#endif

typedef struct sdn_db_inst_write_actions_s
{
    uint32_t            len;
    sdn_db_action_t    *action_set;
} sdn_db_inst_write_actions_t;

typedef struct sdn_db_inst_apply_actions_s
{
    uint32_t            len;
    sdn_db_action_t    *action_set;
} sdn_db_inst_apply_actions_t;

typedef struct sdn_db_inst_clear_actions_s
{
    uint32_t clear_enable;
} sdn_db_inst_clear_actions_t;


typedef struct sdn_db_inst_goto_table_s
{
    uint8_t     table_id;
} sdn_db_inst_goto_table_t;

typedef struct sdn_db_inst_write_metadata_s
{
    uint64_t    metadata;
    uint64_t    metadata_mask;
} sdn_db_inst_write_metadata_t;

typedef struct sdn_db_output_type_s
{
    EN_OFP_PORT_NO_T    of_output_type;
    rtk_of_outputType_t rtk_output_type;
}sdn_db_output_type_t;

#if 0
//move to sdn_type.h
typedef struct sdn_db_action_bucket_data_s
{
    uint32_t        action_len;
    sdn_db_action_t *actions_p;
}sdn_db_bucket_data_t;

typedef struct sdn_db_group_action_bucket_s
{
    uint32_t group_id;
    sdn_db_bucket_data_t action_data;
}sdn_db_group_action_bucket_t;

typedef struct sdn_db_group_entry_s
{
    bool is_vaild;
    EN_OFP_GROUP_TYPE_T group_type;
    uint32_t counters;
    uint32_t bucket_len;
    sdn_db_group_action_bucket_t *action_bucket_p;
}sdn_db_group_entry_t;

typedef struct sdn_db_group_tbl_s
{
    uint32_t total_counter;
    sdn_db_group_entry_t *group_entry_p;
}sdn_db_group_tbl_t;
#endif

//#ifdef  CONFIG_SDK_RTL9310
#if 0
typedef struct sdn_db_action_s
{
    uint16 act_type;
    uint8  oxm_type;
    uint128_t data;
}sdn_db_action_t;

typedef struct sdn_db_apply_actions_s
{
    uint32 bucket_id;
    uint16 action_len;
    sdn_db_action_t *action_p;
    sdn_db_apply_actions_s *next_act_p;
}sdn_db_apply_actions_t;

typedef struct sdn_db_group_table_s
{
    uint32 group_id;
    uint32 action_buket_num;
    uint64 group_counter;
    sdn_db_apply_actions_t *action_buket_p;
    rtk_of_groupType_t group_type;
}sdn_db_group_table_t;
#endif
/*
 * Data Declaration
 */
typedef struct sdn_db_flow_bind_info_s
{
    uint32_t                counter_index;
    rtk_of_flowCntMode_t    counter_type;
    uint32_t                meter_index;
} sdn_db_flow_bind_info_t;

#if 0
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

/* Queue */
typedef struct sdn_db_queue_s
{
    uint16_t min_rate;
    uint16_t max_rate;
    uint32_t queue_id;
    uint32_t port_no;
}sdn_db_queue_t;

/* Port */
typedef struct sdn_db_port_s
{
    uint8_t     hw_addr[OFP_ETH_ALEN];
    uint8_t     name[OFP_MAX_PORT_NAME_LEN];
    uint32_t    port_no;
    uint32_t    config;
    uint32_t    config_mask;
    uint32_t    state;
    uint32_t    curr;
    uint32_t    advertised;
    uint32_t    supported;
    uint32_t    peer;
    uint32_t    curr_speed;
    uint32_t    max_speed;
    uint32_t    len_queue;
    uint32_t    exist;
    uint32_t    ofp_exist;
    sdn_db_queue_t    *queue;
    sdn_db_port_stats_t statistic;
}sdn_db_port_t;
#endif

#ifdef CONFIG_SDK_RTL9310
typedef struct sdn_db_table_flow_info_s
{
    sdn_db_flow_bind_info_t    ingress_table0_info[SDN_HAL_INGRESS_FULL_MATCH_TABLE0_RULE_NUM];
    sdn_db_flow_bind_info_t    ingress_table1_info[SDN_HAL_INGRESS_L2_TABLE1_RULE_NUM];
    sdn_db_flow_bind_info_t    ingress_table2_info[SDN_HAL_INGRESS_L3_TABLE2_RULE_NUM];
    sdn_db_flow_bind_info_t    ingress_table3_info[SDN_HAL_INGRESS_FULL_MATCH_TABLE3_RULE_NUM];
    sdn_db_flow_bind_info_t     egress_table0_info[SDN_HAL_EGRESS_TABLE0_RULE_NUM];
} SDN_DB_TABLE_FLOW_INFO_T;
#endif

typedef struct sdn_db_table_stats_s
{
    uint8_t     table_id;     /*Brandon: if loopback is applied, 4*64=256 (0~255) tables, so invalid_table_id= .......*/
    uint32_t   active_count;
    uint64_t   lookup_count;
    uint64_t   matched_count;
}SDN_DB_TABLE_STATS_T;

//Brandon: Only full match tables can use counter;  L2 and L3 tables have no counter. So maybe no need to get L2/L3 counter.
typedef struct sdn_db_flow_stats_s
{
    uint8_t     table_id;
    uint16_t   priority;
    uint32_t   flow_id;
    //uint32_t   counter_type;
    uint32_t   counter_flag;
    //rtk_of_flowCntMode_t cnt_mode;  /*Brandon: this may be not neccessary if per full match flow entry comes with a packet_counter and byte counter.*/
    uint64_t   packet_count;
    uint64_t   byte_count;
}SDN_DB_FLOW_STATS_T;

#if 0
/* Queue */
typedef struct sdn_db_queue_s
{
    uint16_t min_rate;
    uint16_t max_rate;
    uint32_t queue_id;
    uint32_t port_no;
}sdn_db_queue_t;

/* Port */

typedef struct sdn_db_port_s
{
    uint8_t     hw_addr[OFP_ETH_ALEN];
    uint8_t     name[OFP_MAX_PORT_NAME_LEN];
    uint32_t    port_no;
    uint32_t    config;
    uint32_t    config_mask;
    uint32_t    state;
    uint32_t    curr;
    uint32_t    advertised;
    uint32_t    supported;
    uint32_t    peer;
    uint32_t    curr_speed;
    uint32_t    max_speed;
    uint32_t    len_queue;
    uint32_t    exist;
    uint32_t    ofp_exist;
    sdn_db_queue_t    *queue;
    sdn_db_port_stats_t statistic;
}sdn_db_port_t;

typedef struct sdn_db_switch_features_s {
    bool     virtual_table_exist;
    bool     is_outband;
    uint8_t  n_tables;
    uint32_t capabilities;
    uint64_t datapath_id;
}sdn_db_switch_features_t;

#endif

extern uint32_t sdn_db_max_port;
extern sdn_db_port_t *port_info;
extern sdn_db_switch_features_t *switch_info;
extern sdn_db_meter_table_t *meter_table_info;

#if 0
extern sdn_db_flow_table_t *table_info;
#endif
/*
 * Symbol Definition
 */
/*SDN data base marco*/
#ifndef SDN_DB_MAX_TABLE_ID // for test only
#define SDN_DB_MAX_TABLE_ID 10
#endif
#ifndef of_table_features
#define of_table_features
#endif
#define SDN_DB_PORT_MAX                 sdn_db_max_port
#define SDN_DB_PORT(ofp_port)          (port_info[ofp_port])
#define SDN_DB_PORT_EXIST(ofp_port)    (port_info[ofp_port].exist)
#define SDN_DB_OF_PORT_EXIST(ofp_port) (port_info[ofp_port].ofp_exist)
#define SDN_DB_PORT_NAME(ofp_port)     (port_info[ofp_port].name)
#define SDN_DB_PORT_NUMBER(ofp_port)   (port_info[ofp_port].port_no)
#define SDN_DB_PORT_ETH_ADDR(ofp_port) (port_info[ofp_port].hw_addr)
#define SDN_DB_SWITCH                  (switch_info)
#define SDN_DB_SWITCH_TABLE_NUM_MAX    (switch_info->n_tables)
#define SDN_DB_SWITCH_TABLE_IDX_MAX    (switch_info->n_tables - 1)
#define SDN_DB_SWITCH_VTBL_EXITST      (switch_info->virtual_table_exist == true)
#define SDN_DB_SWITCH_CAPAILITY        (switch_info->capabilities)
#define SDN_DB_METER_MAX               (meter_table_info->meter_features.max_meters)
#define METER_ID_IS_INVALID(meter_id)  (meter_id > SDN_DB_METER_MAX)
#define SDN_BIT_ON(_val, _bit) (_val | 1 << _bit)
#define SDN_BIT_OFF(_val, _bit) (_val & (~(1 << _bit)))
#define SDN_BIT_CLEAR(_val) (_val & 0)

#define OVS_OFPP_LOCAL (56)/*todo:CPU port need to get from hal*/
/*
 * Function Declaration
 */
#ifdef  CONFIG_SDK_RTL9310
int32 sdn_db_add_group(uint32 group_id, rtk_of_groupType_t type, sdn_hal_apply_actions_t *applied_action_list_p);
int32 sdn_db_get_group(uint32 group_id, rtk_of_groupType_t *type, uint32 *bucket_num_p, sdn_hal_apply_actions_t **applied_action_list_p);
int32 sdn_db_set_group(uint32 group_id, rtk_of_groupType_t type, sdn_hal_apply_actions_t *applied_action_list_p);
int32 sdn_db_delete_group(uint32 group_id, sdn_hal_apply_actions_t *applied_action_list_p);
//new group code
int32 sdn_db_group_table_init(void);
int32 sdn_db_group_add_group_entry(sdn_db_group_entry_t *group_entry_p);
int32 sdn_db_group_del_group_entry(uint32_t group_id);
sdn_db_group_entry_t *sdn_db_group_get_entrybygroupid(uint32_t group_id);
/* Classifier */
int sdn_db_set_classifier(rtk_of_classifierType_t type, rtk_of_classifierData_t data);
#endif

//qos code ++
int sdn_db_init_qos_table(void);
int sdn_db_destroy_qos_table(void);
int sdn_db_set_qos_entry(sdn_db_qos_entry_t qos_entry);
int sdn_db_del_qos_entry(sdn_db_qos_entry_t qos_entry);
int sdn_db_get_qos_entry(sdn_db_qos_entry_t *qos_entry_p);
int sdn_db_set_port_qos(uint32_t lport, uint32_t q_id);
sdn_db_qos_entry_t *sdn_db_get_port_qos(uint32_t lport);
int sdn_db_del_port_qos(uint32_t lport, uint32_t q_id);
//qos code --

/* Init */
#ifdef of_table_features
int sdn_db_init(struct ofp_table_features tf[HAL_TABLE_NUM]);
#else
int sdn_db_init(void);
#endif

/* Table Feature */
int sdn_db_set_table_features(uint8_t n_table, struct ofp_table_features *tf);
int sdn_db_get_table_features(uint8_t n_table, struct ofp_table_features *tf);
int sdn_db_get_switch_feature(uint32_t *switch_capabilities_p);

#if 1 /*Brandon add */
int sdn_db_apply_flow_entry_counter(uint8_t table_id,
                                    uint32_t flow_id,
                                    uint32_t count_id,
                                    uint32_t cnt_flag);
int sdn_db_del_flow_entry_counter(uint8_t table_id,
                                  uint32_t flow_id,
                                  uint32_t count_id);
int sdn_db_clear_flow_entry_counter(uint8_t table_id,
                                    uint32_t flow_id,
                                    uint32_t count_id);

int sdn_db_get_availible_counter_id(uint8_t table_id, uint32_t flow_id, uint32_t *pCounter_id);

int sdn_db_get_counter_id_by_table_flow(uint8_t table_id, uint32_t flow_id, uint32_t *pCount_id);

int sdn_db_get_flow_by_counter_id(uint8_t table_id, uint32_t counter_id , uint16_t *pflow_id);

int sdn_db_get_port_count(void);

int sdn_db_get_table_count(void);
sdn_db_flow_table_t * sdn_db_get_flow_table(uint8_t table_id);
int sdn_db_get_flow_table_stats(uint8_t table_id, rtk_of_flowTblCntType_t type, uint32 *pCnt);
int sdn_db_get_flow_entry_usedtime(uint8_t table_id, uint32_t flow_id, uint32_t *used_time_p);
int sdn_db_update_flow_entry_idletimeout(uint8_t table_id, uint32_t flow_id, uint32_t idle_time);
int sdn_db_flow_copy(sdn_db_flow_entry_t *dst, const sdn_db_flow_entry_t *src);
int sdn_db_get_flow_count(uint8_t table_id);

int sdn_db_set_port_stats(uint32_t port_no, hal_port_stats_t *pPort_stats_data);

int sdn_db_get_port_stats(uint32_t port_no, hal_port_stats_t *pPort_stats_data);

int sdn_db_set_flow_stats(uint8_t table_id,
                                              uint32_t flow_id,
                                              uint32_t counter_id,
                                              uint64_t packet_count,
                                              uint64_t byte_count);
int sdn_db_get_flow_stats(uint8_t table_id,
                          uint32_t flow_id,
                          uint64_t *packet_count,
                          uint64_t *byte_count);

int sdn_db_set_flow_table_stats(uint8_t table_id, rtk_of_flowTblCntType_t type, uint64_t table_counter);

#endif
int sdn_db_init_switch(void);
int sdn_db_init_port(void);
int sdn_db_init_flow_table(void);
int sdn_db_destroy(void);
int sdn_db_port_state_update(uint32_t ofp_port);
int sdn_db_port_description_get(uint32_t ofp_port, sdn_db_port_t *port_desc);
int sdn_db_port_max_get(void);
int sdn_db_ofpport_getByName(const char *name, uint32_t *ofp_port);
int sdn_db_port_add(uint32_t ofp_port);
int sdn_db_port_del(uint32_t ofp_port);
int sdn_db_choose_match_template(const uint8_t table_id, uint64_t match_bitmap,uint8_t *p_chosen_template_id, BOOL_T has_dropped_prerequisite);
int sdn_db_meter_table_features_get(sdn_db_meter_table_features_t *feature);
int sdn_db_meter_entry_modify(const sdn_db_meter_mod_t *meter);
int sdn_db_meter_entry_delete(uint32_t meter_id);
int sdn_db_meter_entry_add(const sdn_db_meter_mod_t *meter);
void sdn_db_destroy_meter_table(void);
int sdn_db_init_meter_table(void);
int sdn_db_set_table_miss_action(uint8_t table_id, rtk_of_tblMissAct_t action);
int sdn_db_init_table_stat(void);
/* Tx packet */
int sdn_db_pkt_tx(char *dev_name,
                  void *pkt_p,
                  uint32_t pkt_size,
                  uint32_t ofp_in_port,
                  uint32_t port_no);
#endif /*SDN_DB_H*/
