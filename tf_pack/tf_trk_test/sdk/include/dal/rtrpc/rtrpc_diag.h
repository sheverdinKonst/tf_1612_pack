//
// Created by sheverdin on 7/22/24.
//
#include "include/common/type.h"
#include "rt_type.h"
#include "rtk/phy.h"
//#include "linux/rtdrv_netfilter.h"


#define RT_MGMT(unit)       pMgmt_node[unit]
#define RT_MAPPER(unit)     RT_MGMT(unit)->pMapper

#ifndef RTK_DIAG_RTRPC_DIAG_H
#define RTK_DIAG_RTRPC_DIAG_H
int32 rtrpc_diag_portRtctResult_get(uint32 unit, rtk_port_t port, rtk_rtctResult_t *pRtctResult);
int32 rtrpc_diag_rtctEnable_set(uint32 unit, rtk_portmask_t *pPortmask);
int32 rtk_diag_init(uint32 unit);
#endif //RTK_DIAG_RTRPC_DIAG_H
