/*
 * Copyright (C) 2009-2016 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 *
 * $Revision: 74600 $
 * $Date: 2016-12-22 21:09:04 +0800 (Thu, 22 Dec 2016) $
 *
 * Purpose : Realtek Switch SDK Rtusr API Module
 *
 * Feature : The file have include the following module and sub-modules
 *           1) ovs
 *
 */

#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ovs.h>
//#include <rtusr_util.h> /* Mask by SD6 */
#include <rtrpc_msg.h> /* Add by SD6 */
#include <rtdrv/rtdrv_netfilter.h>

int32 rtrpc_ovs_portLinkChange_get(uint32 unit, rtk_port_linkChange_t *change)
{
    rtdrv_ovsCfg_t ovs_cfg;
    uint32 master_view_unit = unit;
    RTRPC_UNIT_ID_XLATE(master_view_unit, &unit);

    osal_memset(&ovs_cfg, 0, sizeof(ovs_cfg));
    GETSOCKOPT(RTDRV_OVS_PORTLINKCHANGE_GET, &ovs_cfg, rtdrv_ovsCfg_t, 1);

    osal_memcpy(change, &ovs_cfg, sizeof(rtk_port_linkChange_t));

    return RT_ERR_OK;
}

int32 rtrpc_ovs_portLinkChange_stop(uint32 unit)
{
    rtdrv_ovsCfg_t ovs_cfg;
    uint32 master_view_unit = unit;
    RTRPC_UNIT_ID_XLATE(master_view_unit, &unit);

    osal_memset(&ovs_cfg, 0, sizeof(ovs_cfg));
    GETSOCKOPT(RTDRV_OVS_PORTLINKCHANGE_STOP, &ovs_cfg, rtdrv_ovsCfg_t, 1);

    return RT_ERR_OK;
}

