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
 * $Revision: 1 $
 * $Date: 2016-11-03 10:57:40 +0800 (Thur, 03 Nov 2016) $
 *
 * Purpose : Definition a task to get some statistic periodicly.
 *
 * Feature : The file have include the following module and sub-modules
 *            1) Task Initialization
 *            2) Task Destroy
 *
 */

 #ifndef RTK_SDN_TASK_H
#define RTK_SDN_TASK_H

int rtk_sdn_task_init(void);

void rtk_sdn_task_destroy(void);

#endif

