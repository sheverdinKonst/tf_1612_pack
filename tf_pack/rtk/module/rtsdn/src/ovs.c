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
 * $Revision: 73601 $
 * $Date: 2016-11-23 21:04:02 +0800 (Wed, 23 Nov 2016) $
 *
 * Purpose : Definition of Customer API
 *
 * Feature : The file includes the following modules and sub-modules
 *
 */

/*
 * Include Files
 */
#include <common/rt_autoconf.h>
#if defined(CONFIG_SDK_KERNEL_LINUX_KERNEL_MODE)
#include <linux/module.h>
#endif
#include <common/rt_error.h>
#include <common/util/rt_util.h>
#include <common/util/rt_util_system.h>
#include <common/debug/rt_log.h>
#include <common/type.h>
#include <dal/dal_mgmt.h>
#include <rtk/default.h>
#include <osal/print.h>
#include <osal/time.h>
#include <osal/sem.h>
#include <osal/lib.h>
#include <osal/memory.h>
#include <osal/atomic.h>
#include <osal/wait.h>
#include <osal/thread.h>
#include <ioal/mem32.h>
#include <ovs.h>
#include <rtk/switch.h>
#include <rtk/port.h>
#include <hwp/hw_profile.h>


/*
 * Symbol Definition
 */

/*
 * Data Declaration
 */
osal_mutex_t    ovsPort_sem;
uint32  portLinkChange_eventId;

rtk_portmask_t linkChange[RTK_MAX_NUM_OF_UNIT];

#define OVS_PORT_SEM_LOCK()    \
do {\
    if (osal_sem_mutex_take(ovsPort_sem, OSAL_SEM_WAIT_FOREVER) != RT_ERR_OK)\
    {\
        RT_ERR(RT_ERR_SEM_LOCK_FAILED, (MOD_DAL|MOD_PORT), "semaphore lock failed");\
        return RT_ERR_SEM_LOCK_FAILED;\
    }\
} while(0)
#define OVS_PORT_SEM_UNLOCK()   \
do {\
    if (osal_sem_mutex_give(ovsPort_sem) != RT_ERR_OK)\
    {\
        RT_ERR(RT_ERR_SEM_UNLOCK_FAILED, (MOD_DAL|MOD_PORT), "semaphore unlock failed");\
        return RT_ERR_SEM_UNLOCK_FAILED;\
    }\
} while(0)

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
int32
rtk_ovs_portLinkChange_init()
{
    /* create semaphore */
    ovsPort_sem = osal_sem_mutex_create();
    osal_wait_module_create(&portLinkChange_eventId);

    osal_memset(linkChange, 0, RTK_MAX_NUM_OF_UNIT * sizeof(rtk_portmask_t));

    rtk_port_linkMon_register(rtk_ovs_portLinkChange_hdlr);

    if (0 == ovsPort_sem)
    {
        RT_ERR(RT_ERR_FAILED, (MOD_NIC), "semaphore create failed");
        return RT_ERR_FAILED;
    }

    return RT_ERR_OK;

}
int32 rtk_ovs_portLinkChange_hdlr(int32 unit, rtk_port_t port, rtk_port_linkStatus_t new_linkStatus)
{
    OVS_PORT_SEM_LOCK();

    /* set link change indicate bit */
    RTK_PORTMASK_PORT_SET(linkChange[unit], port);

    OVS_PORT_SEM_UNLOCK();
    osal_wake_up(portLinkChange_eventId);

    return RT_ERR_OK;
}

int32 rtk_ovs_portLinkChange_get(uint32 unit, rtk_port_linkChange_t *change)
{
    osal_wait_event(portLinkChange_eventId);

    OVS_PORT_SEM_LOCK();

    HWP_UNIT_TRAVS_LOCAL(unit)
    {
        osal_memcpy(&change->portmask[unit], &linkChange[unit], sizeof(rtk_portmask_t));
        osal_memset(&linkChange[unit], 0, sizeof(rtk_portmask_t));
    }

    OVS_PORT_SEM_UNLOCK();

    return RT_ERR_OK;
}

int32 rtk_ovs_portLinkChange_stop(uint32 unit)
{
    osal_wake_up(portLinkChange_eventId);

    return RT_ERR_OK;
}

#if defined(CONFIG_SDK_KERNEL_LINUX_KERNEL_MODE)
EXPORT_SYMBOL(rtk_ovs_portLinkChange_init);
EXPORT_SYMBOL(rtk_ovs_portLinkChange_get);
EXPORT_SYMBOL(rtk_ovs_portLinkChange_stop);
#endif


