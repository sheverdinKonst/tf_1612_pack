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

#ifndef RTK_DPA_H
#define RTK_DPA_H

/*
 * Include Files
 */
#include <common/type.h>
#include <common/rt_type.h>
#include <common/error.h>
#include <osal/print.h>
#include "rtk_sdn.h"
#include <pthread.h>
#include "rtk_dpa_debug.h"


/*
 * Symbol Definition
 */

#ifdef DPA_MODULE_DBG
#define DPA_MODULE_MSG(fmt, args...)     do { osal_printf(fmt, ##args); } while (0)
#else
#define DPA_MODULE_MSG(fmt, args...)     do { } while (0)
#endif

#define RTK_DPA_TABLE_ID_TEID_LREAD     0       /* RTK FT phase 0 */
#define RTK_DPA_TABLE_ID_IP_FWD         2       /* RTK FT phase 2 */
#define RTK_DPA_TABLE_ID_APP_LIST       3       /* RTK FT phase 3 */
#define RTK_DPA_TABLE_ID_L2_FWD         0

#define DPA_FPGA_CONNECT_PORT           49      /* Encap/Decap FPGA connection port *//* OpenFlow view, 1 start*/

#define TEID_LEARN_NO_SRC_PORT          0xff
#define L2_FWD_NO_SRC_PORT              0xff

#define RTK_DPA_UNIT                    0x0

/* L2_FWD and TEID_LEARN share phase 0 table,
   L2_FWD priority is higher than TEID_LEARN;
   Moreover, Large number priority is higher than small */

#define L2_FWD_ADD_PRIORITY             2000    /* RTK FT phase 0 */
#define L2_FWD_ADD_DEF_PRIORITY         1500    /* RTK FT phase 0 */
#define TEID_LEARN_ADD_PRIORITY         1000    /* RTK FT phase 0 */
#define TEID_LEARN_ADD_DEF_PRIORITY     500     /* RTK FT phase 0 */
#define IP_FWD_ADD_PRIORITY             1000    /* RTK FT phase 2 */
#define IP_FWD_ADD_DEF_PRIORITY         500     /* RTK FT phase 2 */
#define APP_LIST_ADD_PRIORITY           1000    /* RTK FT phase 3 */
#define APP_LIST_ADD_DEF_PRIORITY       500     /* RTK FT phase 3 */

/* Define DPA module capacity  */
#define TEID_LEARN_TABLE_MAX_ENTRY      3644
#define IP_FWD_TABLE_MAX_ENTRY          1024
#define APP_LIST_TABLE_MAX_ENTRY        128
#define L2_FWD_TABLE_MAX_ENTRY          324 /* 27 * 12 eNB */
#define DPA_CAPAC_UE_MAX_NUM            1000
#define DPA_CAPAC_METER_MAX_NUM         512
#define DPA_CAPAC_PORT_MAX_NUM          12
#define DPA_CAPAC_PORT_MASK_LOW         0xfff
#define DPA_CAPAC_PORT_MASK_HIGH        0x0
#define DPA_CAPAC_VP_TUNNEL_MAX_ENTRY   1024


#define RTK_DPA_MODULE_UNINITIALIZED    0
#define RTK_DPA_MODULE_INITIALIZED      1

#define RTK_DPA_MATCH_INNER_SIP_HI_FSID     11
#define RTK_DPA_MATCH_INNER_SIP_LO_FSID     12
#define RTK_DPA_MATCH_INNER_DIP_HI_FSID     2
#define RTK_DPA_MATCH_INNER_DIP_LO_FSID     3
#define RTK_DPA_MATCH_INNER_L4DP_FSID       10
#define RTK_DPA_MATCH_INNER_IP_VER_FSID     5
#define RTK_DPA_MATCH_IP_FLG_FSID           4
#define RTK_DPA_MATCH_GTP_MT_FSID           8
#define RTK_DPA_MATCH_L2_INNER_IP_VER_FSID  9
#define RTK_DPA_MATCH_INNER_IP_PROTO_FSID   6
#define RTK_DPA_MATCH_INNER_TCP_FLAG_FSID   1

#define RTK_DPA_PKT_SIZE_RANGE_MAX_SIZE     0xffff
#define RTK_DPA_PKT_SIZE_RANGE_CHK_IDX      0

#define RTK_DPA_CHT_DISABLE_TABLE_MOVE      1


/*
 * Macro Declaration
 */
#define RTK_DPA_MODULE_INIT_CHECK(state)\
do {\
    if (RTK_DPA_MODULE_UNINITIALIZED == (state)) {\
        osal_printf(" %s DPA module is not initial yet!\n", __FUNCTION__);\
        return RT_ERR_NOT_INIT;\
    }\
} while (0)

#define RTK_DPA_CAPACITY_CHECK(state, capacity)\
do {\
    if (capacity < (state)) {\
        osal_printf(" %s DPA module is over capacity!\n", __FUNCTION__);\
        return RT_ERR_EXCEEDS_CAPACITY;\
    }\
} while (0)

#define RTK_DPA_PHY_TO_SDN_PORT(port)   (port+1)
#define RTK_DPA_SND_TO_PHY_PORT(port)   (port-1)

#define RTK_DPA_CHT_TL_OUTER_VLAN_TPID  0x8100


/*
 * Data Declaration
 */

typedef enum rtk_dpa_inst_type_e
{
    RTK_DPA_INST_TYPE_WRITE = 0,
    RTK_DPA_INST_TYPE_GOTO,
    RTK_DPA_INST_TYPE_METADATA,
    RTK_DPA_INST_TYPE_METER,
    RTK_DPA_INST_TYPE_END,

}rtk_dpa_inst_type_t;

typedef enum rtk_dpa_action_type_s
{
    RTK_DPA_ACTION_TYPE_OUTPUT=0,
    RTK_DPA_ACTION_TYPE_GOTO,
    RTK_DPA_ACTION_TYPE_ENCAP,
    RTK_DPA_ACTION_TYPE_DECAP,
    RTK_DPA_ACTION_TYPE_RATELIMIT,
    RTK_DPA_ACTION_TYPE_END,

}rtk_dpa_action_type_t;


typedef struct rtk_dpa_inst_s
{
    uint8   instNum[RTK_DPA_INST_TYPE_END];
    uint8   actionNum[RTK_DPA_INST_TYPE_END];
    uint8   actionPos[RTK_DPA_INST_TYPE_END][RTK_DPA_ACTION_TYPE_END];

}rtk_dpa_inst_t;

typedef enum rtk_dpa_operation_e
{
    RTK_DPA_OPERATION_ADD=0,
    RTK_DPA_OPERATION_ADD_DEF,
    RTK_DPA_OPERATION_DEL,
    RTK_DPA_OPERATION_DEL_DEF,
    RTK_DPA_OPERATION_FIND,
    RTK_DPA_OPERATION_END

}rtk_dpa_operation_t;

typedef enum rtk_dpa_tableName_e
{
    RTK_DPA_TABLE_NANE_TEID_LEARN=0,
    RTK_DPA_TABLE_NANE_IP_FWD,
    RTK_DPA_TABLE_NANE_APP_LIST,
    RTK_DPA_TABLE_NANE_L2_FWD,
    RTK_DPA_TABLE_NANE_END

}rtk_dpa_tableName_t;

typedef enum rtk_dpa_entry_status_e
{
    RTK_DPA_ENTRY_STATUS_INVALID=0,
    RTK_DPA_ENTRY_STATUS_VALID,
    RTK_DPA_ENTRY_STATUS_END
}rtk_dpa_entry_status_t;

typedef enum rtk_dpa_entry_hit_status_e
{
    RTK_DPA_ENTRY_HIT_STATUS_MISS=0,
    RTK_DPA_ENTRY_HIT_STATUS_HIT,
    RTK_DPA_ENTRY_HIT_STATUS_END
}rtk_dpa_entry_hit_status_t;


typedef struct rtk_dpa_port_s
{
    rtk_portmask_t portmask;

}rtk_dpa_portmask_t;

typedef struct rtk_dpa_encap_s
{
    uint32      teid;
    uint32      outer_dip;
    uint32      outer_sip;
    uint32      identification;
    uint32      diffserv;
    uint32      ttl;
    uint32      outer_udp_srcPort;
    uint32      outer_udp_destPort;
    uint32      gtp_flag;
    uint32      gtp_msgType;
}rtk_dpa_encap_t;

typedef struct rtk_dpa_ipFwd_output_s
{
    rtk_mac_t      dmac;
    rtk_mac_t      smac;
    rtk_portmask_t portmask;

}rtk_dpa_ipFwd_output_t;

typedef struct rtk_dpa_ipFwd_output_direct_s
{
    rtk_portmask_t portmask;

}rtk_dpa_ipFwd_output_direct_t;

typedef struct rtk_dpa_ipFwd_addVLAN_s
{
    uint16          vlan_ID;
    uint16          tpid;
}rtk_dpa_ipFwd_addVLAN_t;


typedef struct rtk_dpa_vp_tunnel_db_s
{
    rtk_dpa_entry_status_t  valid;
    rtk_dpa_encap_t         vp_db;
}rtk_dpa_vp_tunnel_db_t;

typedef struct rtk_dpa_ue_id_s
{
    uint32      ip;
    uint32      teid;
}rtk_dpa_ue_id_t;

typedef enum rtk_dpa_counter_mode_e
{
    RTK_DPA_COUNTER_MODE_BYTECOUNT=0,
    RTK_DPA_COUNTER_MODE_PKTCOUNT,
    RTK_DPA_COUNTER_MODE_END
}rtk_dpa_counter_mode_t;

typedef enum rtk_dpa_status_e
{
    RTK_DPA_STATUS_READY=0,
    RTK_DPA_STATUS_INIT_FAILED,
    RTK_DPA_STATUS_END
}rtk_dpa_status_t;

typedef enum rtk_dpa_tblMissAct_e
{
    RTK_DPA_TBLMISS_ACT_DROP=0,
    RTK_DPA_TBLMISS_ACT_TRAP,
    RTK_DPA_TBLMISS_ACT_FORWARD_NEXT_TBL,   /* forward to next table */
    RTK_DPA_TBLMISS_ACT_EXEC_ACTION_SET,    /* execute current action set */
    RTK_DPA_TBLMISS_ACT_END
}rtk_dpa_tblMissAct_t;

typedef struct rtk_dpa_capacity_s
{
    uint32      max_port_num;
    uint32      portMask_low;
    uint32      portMask_high;
    uint32      teidLearn_table_max;
    uint32      ipFwd_table_max;
    uint32      appList_table_max;
    uint32      l2Fwd_table_max;
    uint32      ue_num_max;
    uint32      meter_max;
}rtk_dpa_capacity_t;


typedef struct rtk_dpa_meter_s
{
   rtk_dpa_counter_mode_t   mode;
   uint32                   rate;
   uint32                   ref_count;
}rtk_dpa_meter_t;

/*
 * TEID LEARN
 */
typedef enum rtk_dpa_teid_learn_match_type_e
{
    RTK_DPA_TEID_LEARN_TYPE_TEID=0,
    RTK_DPA_TEID_LEARN_TYPE_DIP,
    RTK_DPA_TEID_LEARN_TYPE_SIP,
    RTK_DPA_TEID_LEARN_TYPE_L4_DP,
    RTK_DPA_TEID_LEARN_TYPE_SRC_PORT,
    RTK_DPA_TEID_LEARN_TYPE_IP_PROT,
    RTK_DPA_TEID_LEARN_TYPE_ETH_TYPE,
    RTK_DPA_TEID_LEARN_TYPE_END

}rtk_dpa_teid_learn_match_type_t;

/*  For IPv4 address format convert
    Example "172.21.15.187 = 0xac150fbb
    The same, for IP MASK
    Example "255.255.0.0" = 0xffff0000
*/
typedef struct rtk_dpa_teid_learn_match_s
{
    rtk_dpa_teid_learn_match_type_t type;

    union{
        uint32      teid;
        uint32      dip;
        uint32      sip;
        uint32      L4port;
        uint8       src_port;
        uint32      ip_prot;
        uint16      eth_type;
    }data;

    union{
        uint32      teid;
        uint32      dip;
        uint32      sip;
        uint32      L4port;
        uint8       src_port;
        uint32      ip_prot;
        uint16      eth_type;
    }mask;

}rtk_dpa_teid_learn_match_t;

typedef enum rtk_dpa_teid_learn_action_type_s
{
    RTK_DPA_TEID_LEARN_ACTION_TYPE_OUTPUT=0,
    RTK_DPA_TEID_LEARN_ACTION_TYPE_GOTO,
    RTK_DPA_TEID_LEARN_ACTION_TYPE_RATELIMIT,
    RTK_DPA_TEID_LEARN_ACTION_TYPE_DECAP,
    RTK_DPA_TEID_LEARN_ACTION_TYPE_END

}rtk_dpa_teid_learn_action_type_t;

typedef enum rtk_dpa_rate_limit_type_e
{
    RTK_DPA_RATELIMIT_TYPE_PKT=0,
    RTK_DPA_RATELIMIT_TYPE_BYTE,
    RTK_DPA_RATELIMIT_TYPE_END

}rtk_dpa_rate_limit_type_t;

typedef struct rtk_dpa_rateLimit_s
{
    rtk_dpa_rate_limit_type_t   rate_limit_type;
    uint32                      rate;

}rtk_dpa_rateLimit_t;

typedef struct rtk_dpa_teid_learn_action_s
{

    rtk_dpa_teid_learn_action_type_t action;

    union{
        rtk_dpa_portmask_t portmask;                /* for RTK_DPA_TEID_LEARN_ACTION_TYPE_OUTPUT */
        rtk_dpa_tableName_t tableName;              /* for RTK_DPA_TEID_LEARN_ACTION_TYPE_GOTO */
        rtk_dpa_rateLimit_t ratelimit;
    }data;


}rtk_dpa_teid_learn_action_t;

/*
 * IP FWD
 */
typedef enum rtk_dpa_ip_fwd_match_type_e
{
    RTK_DPA_IP_FWD_TYPE_DIP=0,
    RTK_DPA_IP_FWD_TYPE_SRC_PORT,
    RTK_DPA_IP_FWD_TYPE_END
}rtk_dpa_ip_fwd_match_type_t;

/*  For IPv4 address format convert
    Example "172.21.15.187 = 0xac150fbb
    The same, for IP MASK
    Example "255.255.0.0" = 0xffff0000
*/
typedef struct rtk_dpa_ip_fwd_match_s
{
    rtk_dpa_ip_fwd_match_type_t type;

    union{
        uint32      dip;
        uint8       src_port;
    }data;

    union{
        uint32      dip;
        uint8       src_port;
    }mask;

}rtk_dpa_ip_fwd_match_t;


typedef enum rtk_dpa_ip_fwd_action_type_s
{
    RTK_DPA_IP_FWD_ACTION_TYPE_ENCAP = 0,
    RTK_DPA_IP_FWD_ACTION_TYPE_OUTPUT,
    RTK_DPA_IP_FWD_ACTION_TYPE_OUTPUT_DIRECT,
    RTK_DPA_IP_FWD_ACTION_TYPE_GOTO,
    RTK_DPA_IP_FWD_ACTION_TYPE_VLAN,
    RTK_DPA_IP_FWD_ACTION_TYPE_END

}rtk_dpa_ip_fwd_action_type_t;


typedef struct rtk_dpa_ip_fwd_action_s
{

    rtk_dpa_ip_fwd_action_type_t action;

    union{
        rtk_dpa_encap_t                 encap;         /* for RTK_DPA_IP_FWD_ACTION_TYPE_ENCAP */
        rtk_dpa_ipFwd_output_t          output;        /* for RTK_DPA_IP_FWD_ACTION_TYPE_OUTPUT */
        rtk_dpa_ipFwd_output_direct_t   output_direct; /* for RTK_DPA_IP_FWD_ACTION_TYPE_OUTPUT_DIRECT */
        rtk_dpa_tableName_t             tableName;     /* for RTK_DPA_IP_FWD_ACTION_TYPE_GOTO */
        rtk_dpa_ipFwd_addVLAN_t         vlanTag;       /* for RTK_DPA_IP_FWD_ACTION_TYPE_VLAN */
    }data;

}rtk_dpa_ip_fwd_action_t;


/*
 * APP LIST
 */
typedef enum rtk_dpa_app_list_match_type_e
{
    RTK_DPA_APP_LIST_TYPE_IN_SIP=0,
    RTK_DPA_APP_LIST_TYPE_IN_DIP,
    RTK_DPA_APP_LIST_TYPE_IN_L4DP,
    RTK_DPA_APP_LIST_TYPE_IN_IPVER,
    RTK_DPA_APP_LIST_TYPE_OUT_IPPROT,
    RTK_DPA_APP_LIST_TYPE_OUT_L4DP,
    RTK_DPA_APP_LIST_TYPE_ETH_TYPE,
    RTK_DPA_APP_LIST_TYPE_IP_FLAG,
    RTK_DPA_APP_LIST_TYPE_IN_IPPROT,
    RTK_DPA_APP_LIST_TYPE_IN_TCP_FLAG,
    RTK_DPA_APP_LIST_TYPE_END
}rtk_dpa_app_list_match_type_t;

/*  For IPv4 address format convert
    Example "172.21.15.187 = 0xac150fbb
    The same, for IP MASK
    Example "255.255.0.0" = 0xffff0000
*/
typedef struct rtk_dpa_app_list_match_s
{
    rtk_dpa_app_list_match_type_t type;

    union{
        uint32      inner_sip;
        uint32      inner_dip;
        uint32      inner_L4port;
        uint8       inner_ip_ver;
        uint8       outer_ip_proto;
        uint32      outer_L4port;
        uint16      eth_type;
        uint8       ip_flag;
        uint8       inner_ip_proto;
        uint16      inner_tcp_flag;
    }data;

    union{
        uint32      inner_sip;
        uint32      inner_dip;
        uint32      inner_L4port;
        uint8       inner_ip_ver;
        uint8       outer_ip_proto;
        uint32      outer_L4port;
        uint16      eth_type;
        uint8       ip_flag;
        uint8       inner_ip_proto;
        uint16      inner_tcp_flag;
    }mask;

}rtk_dpa_app_list_match_t;


typedef enum rtk_dpa_app_list_action_type_s
{
    RTK_DPA_APP_LIST_ACTION_TYPE_DECAP = 0,
    RTK_DPA_APP_LIST_ACTION_TYPE_OUTPUT,
    RTK_DPA_APP_LIST_ACTION_TYPE_POP_VLAN,
    RTK_DPA_APP_LIST_ACTION_TYPE_END

}rtk_dpa_app_list_action_type_t;

typedef struct rtk_dpa_appList_removeVLAN_s
{
    uint16          vlan_ID;
    uint16          tpid;
}rtk_dpa_appList_removeVLAN_t;


typedef struct rtk_dpa_app_list_action_s
{
    rtk_dpa_app_list_action_type_t  action;
    union{
        rtk_dpa_portmask_t              portmask;
    }data;
}rtk_dpa_app_list_action_t;

/*
 * L2 FWD
 */
typedef enum rtk_dpa_l2_fwd_match_type_e
{
    RTK_DPA_L2_FWD_TYPE_SRC_PORT=0,
    RTK_DPA_L2_FWD_TYPE_ETH_TYPE,
    RTK_DPA_L2_FWD_TYPE_DMAC,
    RTK_DPA_L2_FWD_TYPE_IPPROT,
    RTK_DPA_L2_FWD_TYPE_GTP_MT,
    RTK_DPA_L2_FWD_TYPE_L4DP,
    RTK_DPA_L2_FWD_TYPE_INNER_IPVER,
    RTK_DPA_L2_FWD_TYPE_VLAN_ID,
    RTK_DPA_L2_FWD_TYPE_TCP_FLAG,
    RTK_DPA_L2_FWD_TYPE_FRAGMENT_PKT,
    RTK_DPA_L2_FWD_TYPE_OVER_PKT_SIZE,
    RTK_DPA_L2_FWD_TYPE_END
}rtk_dpa_l2_fwd_match_type_t;

/*  For IPv4 address format convert
    Example "172.21.15.187 = 0xac150fbb
    The same, for IP MASK
    Example "255.255.0.0" = 0xffff0000
    For mac address,
    Example : Fill "00:13:88:aa:bf:56" to dmac.octet[5]~[0]
              dmac.octet[5] = 0x00
              ........
              dmac.octet[0] = 0x56
*/
typedef struct rtk_dpa_l2_fwd_match_s
{
    rtk_dpa_l2_fwd_match_type_t type;

    union{
        uint8       src_port;
        uint16      eth_type;
        rtk_mac_t   dmac;
        uint8       ip_proto;
        uint8       gtp_mt;     /*GTP End Marker, GTP packet message type = 0xFE*/
        uint32      L4port;
        uint8       ip_ver;
        uint16      vlan_id;
        uint8       tcp_flag;
        uint8       fragmented_pkt;
        uint32      overPktSize; /* For incoming Packet Size, over the size is match  */
    }data;

    union{
        uint8       src_port;
        uint16      eth_type;
        rtk_mac_t   dmac;
        uint8       ip_proto;
        uint8       gtp_mt;     /*GTP End Marker, GTP packet message type = 0xFE*/
        uint32      L4port;
        uint8       ip_ver;
        uint16      vlan_id;
        uint8       tcp_flag;
        uint8       fragmented_pkt;
        uint32      overPktSize; /* For incoming Packet Size, over the size is match  */
    }mask;

}rtk_dpa_l2_fwd_match_t;

typedef enum rtk_dpa_l2_fwd_action_type_s
{
    RTK_DPA_L2_FWD_ACTION_TYPE_OUTPUT=0,
    RTK_DPA_L2_FWD_ACTION_TYPE_GOTO,
    RTK_DPA_L2_FWD_ACTION_TYPE_END

}rtk_dpa_l2_fwd_action_type_t;

typedef struct rtk_dpa_l2_fwd_action_s
{

    rtk_dpa_l2_fwd_action_type_t action;

    union{
        rtk_dpa_portmask_t portmask;
        rtk_dpa_tableName_t tableName;
    }data;

}rtk_dpa_l2_fwd_action_t;

extern uint8 g_rtk_sdn_module_statue;

/*
 * Function Declaration
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
extern int32
rtk_dpa_module_init(void);


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
extern int32
rtk_dpa_teid_learn_config(rtk_dpa_operation_t op, rtk_dpa_teid_learn_match_t *pMatch, int num_of_match, rtk_dpa_teid_learn_action_t *pAct, int num_of_act);

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
extern int32
rtk_dpa_teid_learn_find(rtk_dpa_teid_learn_match_t *pMatch, int num_of_match, int rule_priority, int *pEntryIndex, int *pResult);

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
extern int32
rtk_dpa_ip_fwd_config(rtk_dpa_operation_t op, rtk_dpa_ip_fwd_match_t *pMatch, int num_of_match, rtk_dpa_ip_fwd_action_t *pAct, int num_of_act);

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
extern int32
rtk_dpa_ip_fwd_find(rtk_dpa_ip_fwd_match_t *pMatch, int num_of_match, int rule_priority, int *pEntryIndex, int *pResult);


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
extern int32
rtk_dpa_app_list_config(rtk_dpa_operation_t op, rtk_dpa_app_list_match_t *pMatch, int num_of_match, rtk_dpa_app_list_action_t *pAct, int num_of_act);

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
extern int32
rtk_dpa_app_list_find(rtk_dpa_app_list_match_t *pMatch, int num_of_match, int rule_priority, int *pEntryIndex, int *pResult);

/* Function Name:
 *      rtk_dpa_l2_fwd_config
 * Description:
 *      Add/Add as default/Delete entry into table
 * Input:
 *      op              - table entry operation
 *      match           - entry's match field array
 *      num_of_match    - total match field
 *      act             - entry's action field array
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
rtk_dpa_l2_fwd_config(rtk_dpa_operation_t op, rtk_dpa_l2_fwd_match_t *pMatch, int num_of_match, rtk_dpa_l2_fwd_action_t *pAct, int num_of_act);

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
extern int32
rtk_dpa_l2_fwd_find(rtk_dpa_l2_fwd_match_t *pMatch, int num_of_match, int rule_priority, int *pEntryIndex, int *pResult);

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
extern int32
rtk_dpa_table_activeEntryNum_get(rtk_dpa_tableName_t tableName, int *pActiveNum);

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
extern int32
rtk_dpa_capacity_get(rtk_dpa_capacity_t *pDpaCapacity);

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
extern int32
rtk_dpa_ueDownlinkCount_get(rtk_dpa_teid_learn_match_t *pMatch, int num_of_match, uint64_t *pPacketCount, uint64_t *pByteCount);



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
extern int32
rtk_dpa_ueUplinkCount_get(rtk_dpa_app_list_match_t *pMatch, int num_of_match, uint64_t *pPacketCount, uint64_t *pByteCount);


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
extern int32
rtk_dpa_tableEntry_hitStatus_clear(rtk_dpa_tableName_t tableName, int entryIndex);

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
extern int32
rtk_dpa_tableEntry_hitStatus_get(rtk_dpa_tableName_t tableName, int entryIndex, rtk_dpa_entry_hit_status_t *pHitSts);

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
extern int32
rtk_dpa_status_get(rtk_dpa_status_t *pDpaStatus);

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
extern int32
rtk_dpa_factoryDefault_config(void);

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
extern int32
rtk_dpa_tableMissAction_set(rtk_dpa_tableName_t tableName, rtk_dpa_tblMissAct_t tbl_missAct);

#endif /*RTK_DPA_H*/
