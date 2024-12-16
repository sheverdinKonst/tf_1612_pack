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
#include "private/drv/nic/nic_common.h"
#include "sdn_hal.h"
#include "common/type.h"
#include <common/util/rt_util.h>

#define MAC_LEN (6)
#define PORTMASK_GET_PORT_COUNT(portmask)\
    (bitop_numberOfSetBitsInArray((portmask).bits, RTK_TOTAL_NUM_OF_WORD_FOR_1BIT_PORT_LIST))
#define PORTMASK_IS_PORT_SET(portmask, port) \
    (((port) <= SDN_HAL_RULE_PORT_NUM_MAX)?BITMAP_IS_SET((portmask).bits, (port)): 0)
#define PORTMASK_PORT_SET(portmask, port) \
    do {\
        if ((port) <= SDN_HAL_RULE_PORT_NUM_MAX) {BITMAP_SET((portmask).bits, (port));}\
    } while (0)
#define PORTMASK_PORT_CLEAR(portmask, port) \
    do {\
        if ((port) <= SDN_HAL_RULE_PORT_NUM_MAX) {BITMAP_CLEAR((portmask).bits, (port));}\
    } while (0)
#define PORTMASK_SCAN(portmask, port)       \
                for (port = 0; port <= SDN_HAL_RULE_PORT_NUM_MAX; port++) \
                    if(PORTMASK_IS_PORT_SET(portmask, port))

/*bitval shall be shift*/
#define HAL_LIB_BIT_ON(_bitmap, bits)           (_bitmap |= (1ULL << bits) )
#define HAL_LIB_BIT_OFF(_bitmap, bits)          (_bitmap &= ~( 1ULL << bits))
#define HAL_LIB_CHK_BIT_ON(_bitmap, bits)       (_bitmap & ( 1ULL << bits))
#define HAL_METER_ROUND_TO_DEVIDE_BY_16(value)  ((value+8)/16)
int32 bitop_findLastBitInAaray(rtk_bitmap_t *pArray, uint32 arraySize);
uint32 bitop_numberOfSetBitsInArray(rtk_bitmap_t *pArray, uint32 arraySize);
int32 ipv6IsAllOnes_ret(uint8 *pIp6);
int32 ipv4IsAllOnes_ret(uint32 ip4_mask);
int32 macIsAllOnes_ret(uint8 *mac_mask);