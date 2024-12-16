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

#ifndef RTK_SDN_H
#define RTK_SDN_H
//#include "rtk/rtk_type.h"
#include <stdio.h>
#include <stdlib.h>
//#include "sdn_type.h"
#include <sdn_type.h>
//#include "sdn_db.h"
#include <sdn_db.h>
#include "rtk/openflow.h"
#include "rtk_dpa_debug.h"


#define RTK_SDN_MODULE_UNINITIALIZED            0
#define RTK_SDN_MODULE_INITIALIZED              1

#ifdef RTK_SDN_DBG_MSG
#define RTK_SDN_MSG(fmt, args...)     do { printf(fmt, ##args); } while (0)
#else
#define RTK_SDN_MSG(fmt, args...)     do { } while (0)
#endif


#define RTK_SDN_PARAM_CHK(expr, errCode)\
do {\
    if ((int32)(expr)) {\
        return errCode; \
    }\
} while (0)

#define RTK_SDN_CHECK_IS_NULL(_data_p) \
do{ \
    if(_data_p == NULL) \
    { \
        return RT_ERR_NULL_POINTER; \
    } \
}while(0)

#define RTK_SDN_OPENFLOW_RANGE_CHECK(ofp_port) \
if ((ofp_port > sdn_db_port_max_get()) && (ofp_port != OVS_OFPP_LOCAL))\
{\
    return -1;\
}

#define RTK_SDN_CHECK_IS_OPENFLOW_PORT(ofp_port)\
do{ \
    sdn_db_port_t port_desc;\
    RTK_SDN_OPENFLOW_RANGE_CHECK(ofp_port)\
    sdn_db_port_description_get(ofp_port, &port_desc);\
    if(port_desc.ofp_exist != true) {\
        return -1;\
    }\
}while(0)

#define RTK_SDN_CHECK_RANGE_IS_VALID(_chkval, _maxval) \
do { \
    if (_chkval > _maxval) \
    { \
        return -1; \
    } \
}while (0)

typedef enum rtk_sdn_port_config_s
{
    /* OpenFlow 1.0 and 1.1 share these values for these port config bits. */
    RTK_SDN_PC_PORT_DOWN    = 1 << 0, /* Port is administratively down. */
    RTK_SDN_PC_NO_RECV      = 1 << 2, /* Drop all packets received by port. */
    RTK_SDN_PC_NO_FWD       = 1 << 5, /* Drop packets forwarded to port. */
    RTK_SDN_PC_NO_PACKET_IN = 1 << 6, /* No send packet-in msgs for port. */
    /* OpenFlow 1.0 only. */
    RTK_SDN_PC_NO_STP       = 1 << 1, /* No 802.1D spanning tree for port. */
    RTK_SDN_PC_NO_RECV_STP  = 1 << 3, /* Drop received 802.1D STP packets. */
    RTK_SDN_PC_NO_FLOOD     = 1 << 4, /* Do not include port when flooding. */
    /* There are no OpenFlow 1.1-only bits. */
}rtk_sdn_port_config_t;

typedef enum rtk_sdn_return_e
{
    RTK_SDN_RETURN_OK = 0,
    RTK_SDN_RETURN_FAILED,
    RTK_SDN_RETURN_NULL_DATA,
    RTK_SDN_RETURN_FULL_TABLE,
    RTK_SDN_RETURN_BAD_TABLE_ID,
    RTK_SDN_RETURN_INVALID_METADATA,
    RTK_SDN_RETURN_UNSUPPORTED_INS,
    RTK_SDN_RETURN_UNSUPPORTED_MASK,
    RTK_SDN_RETURN_BAD_PORT,
}rtk_sdn_return_t;

typedef enum rtk_sdn_msg_type_e
{
    RTK_SDN_MSG_TYPE_INIT = 0,
    RTK_SDN_MSG_TYPE_NOTIFY,
    RTK_SDN_MSG_TYPE_SEND,
    RTK_SDN_MSG_TYPE_SEND_FIN,
    RTK_SDN_MSG_TYPE_REPLY,
    RTK_SDN_MSG_TYPE_REPLY_FIN,
    RTK_SDN_MSG_TYPE_END,
}rtk_sdn_msg_type_t;

typedef enum rtk_sdn_pkt_rx_type_e
{
    RTK_SDN_PKT_RX_PACKETIN = 0,
    RTK_SDN_PKT_RX_END,
}rtk_sdn_pkt_rx_type_t;

typedef struct rtk_sdn_msg_hdr_s
{
    uint32_t cmd;
    rtk_sdn_msg_type_t msg_type;
    uint16 tousr_totlen;
    uint16 tokrn_totlen;
    uint16 curr_len;
    uint16 curr_offset;
}rtk_sdn_msg_hdr_t;

typedef struct rtk_sdn_msg_s
{
    uint32 msg_id;
    rtk_sdn_msg_hdr_t msg_hdr;
    uint8 *tousr_data_p;
    uint8 *tokrn_data_p;
}rtk_sdn_msg_t;

typedef struct rtk_sdn_msg_param_s
{
    uint32 cmd;
    rtk_sdn_msg_t *msg_p;
    uint8  needDelMsg;
}rtk_sdn_msg_param_t;

struct rtk_sdn_pktin_data_s
{
    char pkt_buf[2048];
    size_t pktin_len;
    uint8 reason;
    uint64 tun_id;
    uint64 metadata;
    uint16 in_port;
};
typedef struct sdn_db_port_counter
{
    uint32    port_no;
    uint64    rx_packets;
    uint64    tx_packets;
    uint64    rx_bytes;
    uint64    tx_bytes;
    uint64    rx_dropped;
    uint64    tx_dropped;
    uint64    rx_errors;
    uint64    tx_errors;
    uint64    rx_frame_err;
    uint64    rx_over_err;
    uint64    rx_crc_err;
    uint64    collisions;
} sdn_db_port_counter_t;
#if 0 /*Mask by SD6*/
struct rtk_sdn_switch_features_s
{
    bool        has_virtual_table;
    bool        is_outband;
    uint8_t     n_tables;
    uint32_t    capabilities;
    uint64_t    datapath_id;
}rtk_sdn_switch_features_t;
#endif
#ifdef of_table_features
//int rtk_sdn_init(struct ofp_table_features tf[HAL_TABLE_NUM]);
int rtk_sdn_init(void);

int rtk_sdn_set_table_features(uint8_t n_table,
                                const struct ofp_table_features *tf);
int rtk_sdn_get_features(uint8_t n_table,
                                struct ofp_table_features *tf);
#else
int rtk_sdn_init(void);
int rtk_sdn_set_table_features(void);
int rtk_sdn_get_features(void);
#endif

//typedef int (*RTK_SDN_RX_CB_T)(drv_nic_pkt_t *pkt_p, void *cookie_p);

int rtk_sdn_pkt_rx(void *buf_p,
                   size_t pkt_buf_size,
                   size_t *pkt_total_len_p,
                   int    *reason_p,
                   uint64 *tun_id_p,
                   uint64 *metada_p,
                   uint16_t *in_port_p);

//int rtk_sdn_pkt_rx_cb_registar(int32 type_id, RTK_SDN_RX_CB_T func_p);
/* Tx packet */
int rtk_sdn_pkt_tx(char *dev_name,
                   void *pkt_p,
                   size_t pkt_size,
                   uint32_t ofp_in_port,
                   uint32_t port_no);

#ifdef  CONFIG_SDK_RTL9310
//new group
int32 rtk_sdn_get_groupfeature(EN_OFP_GROUP_TYPE_T type);
int rtk_sdn_add_groupentry(sdn_db_group_entry_t *add_group_entry_p);
int rtk_sdn_modify_groupentry(sdn_db_group_entry_t *group_entry_p);
int rtk_sdn_del_groupentry(uint32_t group_id);
//old
int rtk_sdn_add_group(uint32 group_id, rtk_of_groupType_t type, sdn_hal_apply_actions_t *applied_action_list_p);
int rtk_sdn_modify_group(uint32 group_id, rtk_of_groupType_t *type, sdn_hal_apply_actions_t *applied_action_list_p);
int rtk_sdn_delete_group(uint32 group_id, sdn_hal_apply_actions_t *applied_action_list_p);

/* Classifier */
int rtk_sdn_set_classifier(rtk_of_classifierType_t type, rtk_of_classifierData_t data);
#endif



/* Counter */
int rtk_sdn_get_flow_stats(uint8_t table_id,
                            uint16_t priority,
                            uint8_t match_num,
                            sdn_db_match_field_t *match,
                            /*ofp_match *match*/
                            uint64_t *packet_count,
                            uint64_t *byte_count);
int rtk_sdn_apply_flow_entry_counter(uint8_t table_id,
                                    uint32_t flow_id,
                                    uint32_t counter_flag);
int rtk_sdn_del_flow_entry_counter(uint8_t table_id,
                                    uint32_t flow_id,
                                    uint32_t count_id);
int rtk_sdn_clear_flow_entry_counter(uint8_t table_id,
                                    uint32_t flow_id,
                                    uint32_t count_id);
int rtk_sdn_get_flow_table_stats(uint8_t table_id, rtk_of_flowTblCntType_t type, uint32 *pCnt);
int rtk_sdn_get_flow_counter(uint8_t table_id,
                           uint16_t priority,
                           uint8_t match_num,
                           sdn_db_match_field_t *match,
                           /*ofp_match *match,*/
                           uint64_t *packet_count,
                           uint64_t *byte_count);
#if 1
/*Brandon:*/
int rtk_sdn_get_port_stats(uint32_t port_no, hal_port_stats_t *port_stats_data);
int rtk_sdn_destroy(void);
#endif
int rtk_sdn_port_config_set(uint32_t ofp_port, sdn_db_port_config_t port_config);

#if 0
int rtk_sdn_switch_capabilities_get(switch_capabilities *capabilities);
int rtk_sdn_switch_config_get(ofp_config_flags *sw_flags);
int rtk_sdn_switch_config_set(ofp_config_flags  sw_flags);
#endif
int rtk_sdn_flow_entry_search(uint32 table_id, uint32 priority, uint32 len_match, sdn_db_match_field_t* flow_match, uint32 *flow_index, uint32 *is_found);

int rtk_sdn_port_feature_get(uint32_t ofp_port, uint32_t *current, uint32_t *advertised, uint32_t *supported, uint32_t *peer);
void rtk_sdn_free_ofp_inst(sdn_db_flow_entry_t *pFlow);
//enum ofperr rtk_sdn_ofp_inst_convert(struct ofpact *ofpacts, size_t ofpacts_len, unsigned int *pInst_num, sdn_db_instruction_t **pp_inst);
int rtk_sdn_inst_validate(const uint8_t table_id, unsigned int inst_num, sdn_db_instruction_t *pInst, uint8_t  chosen_template_id);
void rtk_sdn_flow_free(sdn_db_flow_entry_t *flow);
int rtk_sdn_modify_flow(sdn_db_flow_entry_t *flow, bool reset_counter);
int rtk_sdn_delete_flow(uint8 table_id, uint16 priority, uint32 len_match, sdn_db_match_field_t* flow_match);
int rtk_sdn_add_flow( sdn_db_flow_entry_t *flow);
int rtk_sdn_port_max_get(void);
int rtk_sdn_ofpport_getByName(const char *name, uint32_t *ofp_portp);
int rtk_sdn_port_add(uint32_t port_no);
int rtk_sdn_port_del(uint32_t ofp_port);
int rtk_sdn_port_carrier_get(uint32_t ofp_port, bool *carrier);
int rtk_sdn_eth_addr_get(uint32_t ofp_port, uint8 mac[ETH_ADDR_LEN]);
int rtk_sdn_add_meter(const sdn_db_meter_mod_t *mm);
int rtk_sdn_modify_meter(const sdn_db_meter_mod_t *mm);
int rtk_sdn_delete_meter(uint32_t meter_id);
int rtk_sdn_meter_feature_get(sdn_db_meter_table_features_t *feature);
int rtk_sdn_match_validate(const uint8_t table_id, unsigned int match_num, sdn_db_match_field_t *p_m, uint8_t  *p_chosen_template_id);
int rtk_sdn_set_table_miss_action(uint8_t table_id, rtk_of_tblMissAct_t action);
int rtk_sdn_set_flow_entry_usedtime(uint8_t table_id,
                            uint16_t priority,
                            uint8_t match_num,
                            sdn_db_match_field_t *match_p,
                            uint32_t used_time);

int rtk_db_get_flow_entry_usedtime(uint8_t table_id,
                            uint16_t priority,
                            uint8_t match_num,
                            sdn_db_match_field_t *match_p,
                            uint32_t *used_time_p);

int rtk_sdn_get_switch_feature(uint32_t *switch_capabilities_p);
uint8_t rtk_sdn_get_table_max(void);
int rtk_sdn_get_port_queueconfig(uint32_t port, uint32_t queue_id, uint32_t *max_rate_p, uint32_t *min_rate_p);
int rtk_sdn_get_port_queuestats(uint32_t port, uint32_t queue_id, uint32_t *tx_packets_p, uint32_t *tx_error_p);
//qos code ++
int rtk_sdn_set_qos_entry(sdn_db_qos_entry_t qos_entry);
int rtk_sdn_del_qos_entry(sdn_db_qos_entry_t qos_entry);
int rtk_sdn_get_qos_entry(sdn_db_qos_entry_t *qos_entry_p);
int rtk_sdn_set_port_qos(uint32_t lport, uint32_t q_id);
int rtk_sdn_del_port_qos(uint32_t lport, uint32_t q_id);
//qos code --

#endif /*RTK_SDN_H*/
