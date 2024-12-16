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



#ifndef __RLAPP_STACK_H__
#define __RLAPP_STACK_H__

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


/*
 * Function Declaration
 */


extern int32 rlapp_stack_init(void);
extern void rlapp_stack_wait_rise_ready(void);
extern int32 rlapp_stack_msg_send_test(void);
extern int32 rlapp_stack_msg_testCase_forceSend(uint8 dstBoxID, uint8 cpu_unit, uint32 is_sync);
extern void rlapp_stack_continue_send(void);
extern void rlapp_stack_continue_send2(void);

#endif /* __RLAPP_STACK_H__ */
