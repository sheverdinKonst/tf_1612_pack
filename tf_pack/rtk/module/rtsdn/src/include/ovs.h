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
 * $Revision: 72662 $
 * $Date: 2016-10-21 17:33:32 +0800 (Fri, 21 Oct 2016) $
 *
 * Purpose : All header files of customer API will be located in this file.
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) Extern customer's API to other RTK processes.
 *
 */

#ifndef __OVS_H__
#define __OVS_H__
#include <common/rt_type.h>
#include <rtk/port.h>

/*
 * Include Files
 *      Any customer added header file which will be called by other RTK
 *      processes that not include by customer API, please add the header
 *      files in this customer_api.h.
 */

/*
 * Symbol Definition
 */

/*
 * Data Declaration
 */
typedef struct rtk_port_linkChange_s
{
    rtk_portmask_t portmask[RTK_MAX_NUM_OF_UNIT];
} rtk_port_linkChange_t;

/*
 * Function Declaration
 */

/* Module Name : customer */

/* Function Name:
 *      rtk_port_linkChange_init
 * Description:
 *      Intial port link change.
 * Input:
 *      unit            - unit id
 * Output:
 *      None
 * Return:
 *      None
 * Applicable:
 *
 * Note:
 *      This API is exported to other kernel module, then other modules can
 *      initial the customer API part, too.
 */

extern int32
rtk_ovs_portLinkChange_init(void);

extern int32
rtk_ovs_portLinkChange_hdlr(int32 unit, rtk_port_t port, rtk_port_linkStatus_t new_linkStatus);

extern int32
rtk_ovs_portLinkChange_get(uint32 unit, rtk_port_linkChange_t *change);

extern int32
rtk_ovs_portLinkChange_stop(uint32 unit);

#endif /* __OVS_H__ */
