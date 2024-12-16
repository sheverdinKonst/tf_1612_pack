/*
 * Copyright (C) 2018 Realtek Semiconductor Corp.
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
 * Purpose :
 *
 */
#ifndef __RLAPP_H__
#define __RLAPP_H__


/*
 * Include Files
 */


/*
 * Symbol Definition
 */


/*
 * Data Declaration
 */


/*
 * Macro Declaration
 */
#define rlapp_dbg(fmt, args...)     osal_printf("RealAPP:D(%d):%s:%d:"fmt, osal_thread_self(), __FUNCTION__, __LINE__, ##args)
#define rlapp_info(fmt, args...)    osal_printf("RealAPP:I(%d):%s:%d:"fmt, osal_thread_self(), __FUNCTION__, __LINE__, ##args)
#define rlapp_err(fmt, args...)     osal_printf("RealAPP:ErR(%d):%s:%d:"fmt, osal_thread_self(), __FUNCTION__, __LINE__, ##args)
#define rlapp_print(fmt, args...)   osal_printf(fmt, ##args)


/*
 * Function Declaration
 */

extern int32 rlapp_init(void);






#endif /* __RLAPP_H__ */


