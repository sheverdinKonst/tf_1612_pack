//
// Created by sheverdin on 7/22/24.
//

#include "rtdrv/rtdrv_netfilter.h"
#include "dal/dal_mgmt.h"
#include "rtrpc_diag.h"
#include "rt_type.h"
#include "rtrpc_msg.h"
#include <string.h>
#include <stdio.h>

dal_mgmt_info_t         *pMgmt_node[RTK_MAX_NUM_OF_UNIT];
int32 rtrpc_diag_portRtctResult_get(uint32 unit, rtk_port_t port, rtk_rtctResult_t *pRtctResult)
{
    printf("rtrpc_diag_portRtctResult_get\n");
    rtdrv_diagCfg_t diag_cfg;
    uint32 master_view_unit = unit;
    RTRPC_UNIT_ID_XLATE(master_view_unit, &unit);

    memset(&diag_cfg, 0, sizeof(rtdrv_diagCfg_t));
    diag_cfg.unit = unit;
    diag_cfg.port = port;
    GETSOCKOPT(RTDRV_DIAG_RTCTRESULT_GET, &diag_cfg, rtdrv_diagCfg_t, 1);
    memcpy(pRtctResult, &diag_cfg.rtctResult, sizeof(rtk_rtctResult_t));

    return RT_ERR_OK;
}

int32 rtrpc_diag_rtctEnable_set(uint32 unit, rtk_portmask_t *pPortmask)
{
    rtdrv_diagCfg_t diag_cfg;
    uint32 master_view_unit = unit;
    RTRPC_UNIT_ID_XLATE(master_view_unit, &unit);

    diag_cfg.unit = unit;
    memcpy(&diag_cfg.portmask, pPortmask, sizeof(rtk_portmask_t));
    SETSOCKOPT(RTDRV_DIAG_RTCTENABLE_SET, &diag_cfg, rtdrv_diagCfg_t, 1);

    return RT_ERR_OK;
}

dal_mgmt_info_t dalMgmtInfo_0;
dal_mgmt_info_t dalMgmtInfo_1;
dal_mgmt_info_t dalMgmtInfo_2;
dal_mgmt_info_t dalMgmtInfo_3;
dal_mgmt_info_t dalMgmtInfo_4;
dal_mgmt_info_t dalMgmtInfo_5;
dal_mgmt_info_t dalMgmtInfo_6;
dal_mgmt_info_t dalMgmtInfo_7;
dal_mgmt_info_t dalMgmtInfo_8;



int32 rtk_diag_init(uint32 unit)
{
    printf("unit = %d\n", unit);

    pMgmt_node[0] = &dalMgmtInfo_0;
    pMgmt_node[1] = &dalMgmtInfo_1;
    pMgmt_node[2] = &dalMgmtInfo_2;
    pMgmt_node[3] = &dalMgmtInfo_3;
    pMgmt_node[4] = &dalMgmtInfo_4;
    pMgmt_node[5] = &dalMgmtInfo_5;
    pMgmt_node[6] = &dalMgmtInfo_6;
    pMgmt_node[7] = &dalMgmtInfo_7;
    pMgmt_node[8] = &dalMgmtInfo_8;

    pMgmt_node[1]->pMapper =

    if (unit > RTK_MAX_UNIT_ID)
    {
        printf("unit > RTK_MAX_UNIT_ID\n");
    }
    if (NULL == pMgmt_node)
    {
        printf("NULL == pMgmt_node\n");
    }
    if (NULL == pMgmt_node[unit])
    {
        printf("NULL == RT_MGMT(unit)\n");
    }
    if (NULL == pMgmt_node[unit]->pMapper)
    {
        printf("NULL == RT_MAPPER(unit)\n");
    }
    RT_PARAM_CHK((unit > RTK_MAX_UNIT_ID) || (NULL == RT_MGMT(unit)) || (NULL == RT_MAPPER(unit)), RT_ERR_UNIT_ID);

    return RT_MAPPER(unit)->diag_init(unit);
} /* end of rtk_diag_init */