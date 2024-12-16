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

#ifndef _SDN_HAL_COMMON_H_
#define _SDN_HAL_COMMON_H_
#include <stdio.h>
#include <stdlib.h>
#include <common/rt_type.h>
#include <rtk/port.h>
#include <osal/lib.h>
#include "ofp_common_enum.h"
#include <hwp/hw_profile.h>
#include <common/debug/rt_log.h>
#include "rtk_dpa_debug.h"


#define sdn_hal_portmask_t rtk_portmask_t
#define UNIT_ID_0                                       (0)
#define UNIT_ID_1                                       (1)
#define UNIT_ID_2                                       (2)
#define SDN_HAL_MY_UNIT_ID()                            UNIT_ID_0
#ifdef DBG_SDM_MESSGAE
#define DBG_SDN(string, args...)	                    RT_LOG(LOG_DEBUG, (MOD_OPENFLOW), string);
#else
#define DBG_SDN(string, args...)	                    do { } while(0);
#endif
#define SDN_HAL_RULE_DEV_NUM_MAX                        (RTK_MAX_NUM_OF_UNIT)  /* Maximum number of stacked devices */
#define SDN_HAL_RULE_PORT_NUM_MAX                       (RTK_MAX_NUM_OF_PORTS)
#define SDN_HAL_RULE_TRK_PORT_NUM                       (8)           /* Max number of trunk ports */
#define SDN_HAL_RULE_LOGICPORT_NUM_MAX                  ((SDN_HAL_RULE_DEV_NUM_MAX * SDN_HAL_RULE_PORT_NUM_MAX) + SDN_HAL_RULE_TRK_PORT_NUM)
#define SDN_HAL_RULE_NORMALPORT_NUM_MAX                 ((SDN_HAL_RULE_DEV_NUM_MAX * SDN_HAL_RULE_PORT_NUM_MAX))

#define SDN_PARAM_CHK(expr, errCode)\
do {\
    if ((int32)(expr)) {\
        return errCode; \
    }\
} while (0)

#define CHECK_TABLE_ID_IS_VALID(_id) \
{ \
    switch(_id) \
    { \
        case FT_PHASE_IGR_FT_1: \
        case FT_PHASE_IGR_FT_2: \
            return SDN_HAL_RETURN_NOT_SUPPORTED; \
        case FT_PHASE_IGR_FT_0: \
        case FT_PHASE_IGR_FT_3: \
        case FT_PHASE_EGR_FT_0: \
        default :\
            ; \
    }\
}while(0)

#define IF_TABLE_IS_L2ORL3(_id) \
    if ((_id == FT_PHASE_IGR_FT_1) || (_id == FT_PHASE_IGR_FT_2))

#define IF_TABLE_COUNTER_IS_NOT_AVAILABLE(_id) \
    if ((_id == FT_PHASE_IGR_FT_1) || (_id == FT_PHASE_IGR_FT_2))

/*define openflow constant
 */
#ifndef OFP_ETH_ALEN
#define SDN_HAL_RULE_ETH_ALEN 6
#else
#define SDN_HAL_RULE_ETH_ALEN OFP_ETH_ALEN
#endif

#ifndef OFP_MAX_PORT_NAME_LEN
#define SDN_HAL_RULE_MAX_PORT_NAME_LEN 16
#else
#define SDN_HAL_RULE_MAX_PORT_NAME_LEN OFP_MAX_PORT_NAME_LEN
#endif

#ifndef DESC_STR_LEN
#define SDN_HAL_RULE_DESC_STR_LEN 256
#else
#define SDN_HAL_RULE_DESC_STR_LEN DESC_STR_LEN
#endif

#ifndef SERIAL_NUM_LEN
#define SDN_HAL_RULE_SERIAL_NUM_LEN 32
#else
#define SDN_HAL_RULE_SERIAL_NUM_LEN SERIAL_NUM_LEN
#endif

#define SDN_HAL_RULE_MAX_DATAPATH_LEN 64

#define SDN_HAL_RULE_MAX_MATCH_FIELDS 40

#define SDN_HAL_RULE_MAX_IPV6_ADDR_LEN 16

typedef enum sdn_hal_rule_switch_capability_e
{
    SDN_HAL_RULE_SWITCH_FLOW_STATS  = 1 << 0,
    SDN_HAL_RULE_SWITCH_TABLE_STATS = 1 << 1,
    SDN_HAL_RULE_SWITCH_PORT_STATS  = 1 << 2,
    SDN_HAL_RULE_SWITCH_GROUP_STATS = 1 << 3,
    SDN_HAL_RULE_SWITCH_IP_REASM    = 1 << 5,
    SDN_HAL_RULE_SWITCH_QUEUE_STATS = 1 << 6,
    SDN_HAL_RULE_SWITCH_PORT_BLOCKED= 1 << 7,
}sdn_hal_rule_switch_capability_t;

#endif /*_SDN_HAL_COMMON_H_*/
