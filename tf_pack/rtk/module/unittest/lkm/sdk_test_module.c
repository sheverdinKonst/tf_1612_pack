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
 * $Revision: 71708 $
 * $Date: 2016-09-19 11:31:17 +0800 (Mon, 19 Sep 2016) $
 *
 * Purpose : Definition of SDK test APIs in the SDK
 *
 * Feature : SDK test APIs
 *
 */

#define EXPORT_SYMTAB 1

/*
 * Include Files
 */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <common/rt_autoconf.h>
#include <common/rt_error.h>
#include <osal/print.h>
#include <sdk_test.h>
#include <unittest_util.h>
#include <rtdrv/rtdrv_netfilter.h>

static void sdktest_drv_hook(void)
{
    sdkTest_drv.mode_get = sdktest_mode_get;
    sdkTest_drv.mode_set = sdktest_mode_set;
    sdkTest_drv.run = sdktest_run;
    sdkTest_drv.run_id = sdktest_run_id;
}
/* we install our own startup routine which sets up threads */
static int __init sdktest_init(void)
{
    osal_printf("Init SDKTEST Module....OK\n");
    sdktest_drv_hook();
    return RT_ERR_OK;
}

static void __exit sdktest_exit(void)
{
    osal_printf("Exit SDKTEST Module....OK\n");
}

module_init(sdktest_init);
module_exit(sdktest_exit);

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,19))
MODULE_LICENSE("GPL");
#endif


EXPORT_SYMBOL(sdktest_run);
EXPORT_SYMBOL(sdktest_run_id);
EXPORT_SYMBOL(sdktest_mode_get);
EXPORT_SYMBOL(sdktest_mode_set);
