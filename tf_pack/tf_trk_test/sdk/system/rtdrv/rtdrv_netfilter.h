//
// Created by sheverdin on 7/22/24.
//

#ifndef RTK_DIAG_RTDRV_NETFILTER_H
#define RTK_DIAG_RTDRV_NETFILTER_H

#include "rt_type.h"
#include "rtk/phy.h"

#define RTDRV_BASE_CTL                          (64+1024+64+64+9000) /* 0x27E8 */
#define RTDRV_MODULE_MAX_NUM                    100
#define RTDRV_MODULE_MASK                       0xFFFF0000
#define RTDRV_MODULE_SHIFT                      16

#define RTDRV_INIT_OFFSET               (  1 << RTDRV_MODULE_SHIFT )
#define RTDRV_L2_OFFSET                 (  2 << RTDRV_MODULE_SHIFT )
#define RTDRV_L3_OFFSET                 (  3 << RTDRV_MODULE_SHIFT )
#define RTDRV_MCAST_OFFSET              (  4 << RTDRV_MODULE_SHIFT )
#define RTDRV_IPMC_OFFSET               (  5 << RTDRV_MODULE_SHIFT )
#define RTDRV_TUNNEL_OFFSET             (  6 << RTDRV_MODULE_SHIFT )
#define RTDRV_PORT_OFFSET               (  7 << RTDRV_MODULE_SHIFT )
#define RTDRV_VLAN_OFFSET               (  8 << RTDRV_MODULE_SHIFT )
#define RTDRV_STP_OFFSET                (  9 << RTDRV_MODULE_SHIFT )
#define RTDRV_REG_OFFSET                ( 10 << RTDRV_MODULE_SHIFT )
#define RTDRV_COUNTER_OFFSET            ( 11 << RTDRV_MODULE_SHIFT )
#define RTDRV_TIME_OFFSET               ( 12 << RTDRV_MODULE_SHIFT )
#define RTDRV_TRAP_OFFSET               ( 13 << RTDRV_MODULE_SHIFT )
#define RTDRV_QOS_OFFSET                ( 14 << RTDRV_MODULE_SHIFT )
#define RTDRV_TRUNK_OFFSET              ( 15 << RTDRV_MODULE_SHIFT )
#define RTDRV_MIRROR_OFFSET             ( 16 << RTDRV_MODULE_SHIFT )
#define RTDRV_STACK_OFFSET              ( 17 << RTDRV_MODULE_SHIFT )
#define RTDRV_FLOWCTRL_OFFSET           ( 18 << RTDRV_MODULE_SHIFT )
#define RTDRV_RATE_OFFSET               ( 19 << RTDRV_MODULE_SHIFT )
#define RTDRV_SWITCH_OFFSET             ( 20 << RTDRV_MODULE_SHIFT )
#define RTDRV_NIC_OFFSET                ( 21 << RTDRV_MODULE_SHIFT )
#define RTDRV_OAM_OFFSET                ( 22 << RTDRV_MODULE_SHIFT )
#define RTDRV_EEE_OFFSET                ( 23 << RTDRV_MODULE_SHIFT )
#define RTDRV_SEC_OFFSET                ( 24 << RTDRV_MODULE_SHIFT )
#define RTDRV_ACL_OFFSET                ( 25 << RTDRV_MODULE_SHIFT )
#define RTDRV_PIE_OFFSET                ( 26 << RTDRV_MODULE_SHIFT )
#define RTDRV_MPLS_OFFSET               ( 27 << RTDRV_MODULE_SHIFT )
#define RTDRV_DIAG_OFFSET               ( 28 << RTDRV_MODULE_SHIFT )
#define RTDRV_DEBUG_OFFSET              ( 29 << RTDRV_MODULE_SHIFT )
#define RTDRV_UART_OFFSET               ( 30 << RTDRV_MODULE_SHIFT )
#define RTDRV_LED_OFFSET                ( 31 << RTDRV_MODULE_SHIFT )
#define RTDRV_RTL8231_OFFSET            ( 32 << RTDRV_MODULE_SHIFT )
#define RTDRV_EXTGPIO_OFFSET            ( 33 << RTDRV_MODULE_SHIFT )
#define RTDRV_GPIO_OFFSET               ( 34 << RTDRV_MODULE_SHIFT )
#define RTDRV_L2NTFY_OFFSET             ( 35 << RTDRV_MODULE_SHIFT )
#define RTDRV_SPI_OFFSET                ( 36 << RTDRV_MODULE_SHIFT )
#define RTDRV_VXLAN_OFFSET              ( 37 << RTDRV_MODULE_SHIFT )
#define RTDRV_OPENFLOW_OFFSET           ( 38 << RTDRV_MODULE_SHIFT )
#define RTDRV_CAPWAP_OFFSET             ( 39 << RTDRV_MODULE_SHIFT )
#define RTDRV_I2C_OFFSET                ( 40 << RTDRV_MODULE_SHIFT )
#define RTDRV_HWP_OFFSET                ( 41 << RTDRV_MODULE_SHIFT )
#define RTDRV_SDK_OFFSET                ( 42 << RTDRV_MODULE_SHIFT )
#define RTDRV_BPE_OFFSET                ( 43 << RTDRV_MODULE_SHIFT )
#define RTDRV_SERDES_OFFSET             ( 44 << RTDRV_MODULE_SHIFT )
#define RTDRV_SC_OFFSET                 ( 45 << RTDRV_MODULE_SHIFT )
#define RTDRV_SYS_OFFSET                ( 46 << RTDRV_MODULE_SHIFT )
#define RTDRV_UTIL_OFFSET               ( 47 << RTDRV_MODULE_SHIFT )
#define RTDRV_END_OFFSET                ( 48 << RTDRV_MODULE_SHIFT )

typedef struct rtdrv_diagCfg_s
{
    uint32              unit;
    rtk_port_t          port;
    uint32              serdesId;
    rtk_portmask_t      portmask;
    rtk_enable_t        enable;
    rtk_rtctResult_t    rtctResult;
    uint32              type;
    uint32              target_index;
    uint32              value[20];
} rtdrv_diagCfg_t;

enum rtdrv_diag_get_e
{
    RTDRV_DIAG_5GSERDES_REMOTE_LOOPBACK_GET = (RTDRV_BASE_CTL + RTDRV_DIAG_OFFSET),
    RTDRV_DIAG_5GSERDES_LOCAL_LOOPBACK_GET,
    RTDRV_DIAG_10GSERDES_REMOTE_LOOPBACK_GET,
    RTDRV_DIAG_10GSERDES_LOCAL_LOOPBACK_GET,
    RTDRV_DIAG_RTCTRESULT_GET,
    RTDRV_DIAG_TABLE_WHOLE_READ,
    RTDRV_DIAG_REG_WHOLE_READ,
    RTDRV_DIAG_PERIPHERAL_REG_READ,
    RTDRV_DIAG_PHY_REG_READ,
    RTDRV_DIAG_TABLE_ENTRY_READ,
    RTDRV_DIAG_TABLE_INDEX_NAME,
#if (defined(CONFIG_SDK_APP_DIAG_EXT) && defined (CONFIG_SDK_RTL9300))
    RTDRV_DIAG_DEBUG_FIELD_GET,
#endif
};

enum rtdrv_diag_set_e
{
    RTDRV_DIAG_RTCTENABLE_SET = (RTDRV_BASE_CTL + RTDRV_DIAG_OFFSET),
    RTDRV_DIAG_TABLE_ENTRY_DATAREG_WRITE,
    RTDRV_DIAG_TABLE_ENTRY_WRITE,
#if (defined(CONFIG_SDK_APP_DIAG_EXT) && defined (CONFIG_SDK_RTL9300))
    RTDRV_DIAG_DEBUG_FIELD_SET,
#endif
};

#endif //RTK_DIAG_RTDRV_NETFILTER_H
