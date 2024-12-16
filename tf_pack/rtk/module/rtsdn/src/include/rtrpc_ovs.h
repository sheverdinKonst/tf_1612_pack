/*
* Copyright(c) Realtek Semiconductor Corporation, 2008
* All rights reserved.
*/
#ifndef __RTRPC_OVS_H__
#define __RTRPC_OVS_H__

/*
* Include Files
*/
#include <dal/dal_mapper.h>
#include <ovs.h>
#define rtk_ovs_portLinkChange_get                                     rtrpc_ovs_portLinkChange_get
#define rtk_ovs_portLinkChange_stop                                    rtrpc_ovs_portLinkChange_stop
/*
 * Function Declaration
 */

extern int32 rtk_ovs_portLinkChange_get(uint32 unit, rtk_port_linkChange_t *change);
extern int32 rtk_ovs_portLinkChange_stop(uint32 unit);

#endif /* __RTRPC_OVS_H__ */
