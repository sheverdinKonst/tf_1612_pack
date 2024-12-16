/*
 * ## Please DO NOT edit this file!! ##
 * This file is auto-generated from the register source files.
 * Any modifications to this file will be LOST when it is re-generated.
 *
 * ----------------------------------------------------------------
 * Copyright(c) Realtek Semiconductor Corporation, 2009-2016
 * All rights reserved.
 *
 * $Revision$
 * $Date$
 *
 * Purpose : chip table declaration in the SDK.
 *
 * Feature : chip table declaration
 *
 */

/*
 * Include Files
 */
#include <common/rt_autoconf.h>
#include <hal/chipdef/allmem.h>
#include <hal/chipdef/longan/rtk_longan_table_struct.h>
#include <hal/chipdef/longan/rtk_longan_tableField_list.h>

rtk_table_t rtk_longan_table_list[] =
{
#if defined(CONFIG_SDK_CHIP_FEATURE_802_1Q_VLAN___QINQ)
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "UNTAG",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     0,
        /* table size */            4096,
        /* total data registers */  1,
        /* total field numbers */   LONGAN_UNTAGFIELD_LIST_END,
        /* table fields */          RTL9300_UNTAG_FIELDS
    },
#endif
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "VLAN",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     1,
        /* table size */            4096,
        /* total data registers */  2,
        /* total field numbers */   LONGAN_VLANFIELD_LIST_END,
        /* table fields */          RTL9300_VLAN_FIELDS
    },
#endif
#endif   /* CONFIG_SDK_CHIP_FEATURE_802_1Q_VLAN___QINQ */
#if defined(CONFIG_SDK_CHIP_FEATURE_VLAN_TRANSLATION)
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "VLAN_EGR_CNVT",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     1,
        /* table size */            512,
        /* total data registers */  6,
        /* total field numbers */   LONGAN_VLAN_EGR_CNVTFIELD_LIST_END,
        /* table fields */          RTL9300_VLAN_EGR_CNVT_FIELDS
    },
#endif
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "VLAN_IGR_CNVT",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     0,
        /* table size */            1024,
        /* total data registers */  6,
        /* total field numbers */   LONGAN_VLAN_IGR_CNVTFIELD_LIST_END,
        /* table fields */          RTL9300_VLAN_IGR_CNVT_FIELDS
    },
#endif
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "VLAN_IP_BASED",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     0,
        /* table size */            1024,
        /* total data registers */  6,
        /* total field numbers */   LONGAN_VLAN_IP_BASEDFIELD_LIST_END,
        /* table fields */          RTL9300_VLAN_IP_BASED_FIELDS
    },
#endif
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "VLAN_MAC_BASED",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     0,
        /* table size */            1024,
        /* total data registers */  6,
        /* total field numbers */   LONGAN_VLAN_MAC_BASEDFIELD_LIST_END,
        /* table fields */          RTL9300_VLAN_MAC_BASED_FIELDS
    },
#endif
#endif   /* CONFIG_SDK_CHIP_FEATURE_VLAN_TRANSLATION */
#if defined(CONFIG_SDK_CHIP_FEATURE_LINK_AGGREGATION)
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "LAG",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      2,
        /* access table type */     7,
        /* table size */            64,
        /* total data registers */  3,
        /* total field numbers */   LONGAN_LAGFIELD_LIST_END,
        /* table fields */          RTL9300_LAG_FIELDS
    },
#endif
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "SRC_TRK_MAP",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      0,
        /* access table type */     8,
        /* table size */            1024,
        /* total data registers */  1,
        /* total field numbers */   LONGAN_SRC_TRK_MAPFIELD_LIST_END,
        /* table fields */          RTL9300_SRC_TRK_MAP_FIELDS
    },
#endif
#endif   /* CONFIG_SDK_CHIP_FEATURE_LINK_AGGREGATION */
#if defined(CONFIG_SDK_CHIP_FEATURE_SPANNING_TREE)
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "MSTI",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     4,
        /* table size */            64,
        /* total data registers */  2,
        /* total field numbers */   LONGAN_MSTIFIELD_LIST_END,
        /* table fields */          RTL9300_MSTI_FIELDS
    },
#endif
#endif   /* CONFIG_SDK_CHIP_FEATURE_SPANNING_TREE */
#if defined(CONFIG_SDK_CHIP_FEATURE_PORT_ISOLATION)
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "PORT_ISO_CTRL",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     6,
        /* table size */            1024,
        /* total data registers */  1,
        /* total field numbers */   LONGAN_PORT_ISO_CTRLFIELD_LIST_END,
        /* table fields */          RTL9300_PORT_ISO_CTRL_FIELDS
    },
#endif
#endif   /* CONFIG_SDK_CHIP_FEATURE_PORT_ISOLATION */
#if defined(CONFIG_SDK_CHIP_FEATURE_MAC_FORWARDING_CONTROL)
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "L2_CAM_MC",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     1,
        /* table size */            64,
        /* total data registers */  3,
        /* total field numbers */   LONGAN_L2_CAM_MCFIELD_LIST_END,
        /* table fields */          RTL9300_L2_CAM_MC_FIELDS
    },
#endif
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "L2_CAM_UC",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     1,
        /* table size */            64,
        /* total data registers */  3,
        /* total field numbers */   LONGAN_L2_CAM_UCFIELD_LIST_END,
        /* table fields */          RTL9300_L2_CAM_UC_FIELDS
    },
#endif
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "L2_MC",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     0,
        /* table size */            16384,
        /* total data registers */  3,
        /* total field numbers */   LONGAN_L2_MCFIELD_LIST_END,
        /* table fields */          RTL9300_L2_MC_FIELDS
    },
#endif
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "L2_UC",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     0,
        /* table size */            16384,
        /* total data registers */  3,
        /* total field numbers */   LONGAN_L2_UCFIELD_LIST_END,
        /* table fields */          RTL9300_L2_UC_FIELDS
    },
#endif
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "MC_PORTMASK",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     2,
        /* table size */            1024,
        /* total data registers */  1,
        /* total field numbers */   LONGAN_MC_PORTMASKFIELD_LIST_END,
        /* table fields */          RTL9300_MC_PORTMASK_FIELDS
    },
#endif
#endif   /* CONFIG_SDK_CHIP_FEATURE_MAC_FORWARDING_CONTROL */
#if defined(CONFIG_SDK_CHIP_FEATURE_METER_MARKER)
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "METER",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     5,
        /* table size */            256,
        /* total data registers */  3,
        /* total field numbers */   LONGAN_METERFIELD_LIST_END,
        /* table fields */          RTL9300_METER_FIELDS
    },
#endif
#endif   /* CONFIG_SDK_CHIP_FEATURE_METER_MARKER */
#if defined(CONFIG_SDK_CHIP_FEATURE_REMARKING)
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "REMARK",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     3,
        /* table size */            1024,
        /* total data registers */  1,
        /* total field numbers */   LONGAN_REMARKFIELD_LIST_END,
        /* table fields */          RTL9300_REMARK_FIELDS
    },
#endif
#endif   /* CONFIG_SDK_CHIP_FEATURE_REMARKING */
#if defined(CONFIG_SDK_CHIP_FEATURE_LAYER_3_ROUTING)
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "L3_EGR_INTF",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     4,
        /* table size */            128,
        /* total data registers */  2,
        /* total field numbers */   LONGAN_L3_EGR_INTFFIELD_LIST_END,
        /* table fields */          RTL9300_L3_EGR_INTF_FIELDS
    },
#endif
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "L3_EGR_INTF_LIST",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     5,
        /* table size */            512,
        /* total data registers */  1,
        /* total field numbers */   LONGAN_L3_EGR_INTF_LISTFIELD_LIST_END,
        /* table fields */          RTL9300_L3_EGR_INTF_LIST_FIELDS
    },
#endif
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "L3_EGR_INTF_MAC",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     2,
        /* table size */            2112,
        /* total data registers */  2,
        /* total field numbers */   LONGAN_L3_EGR_INTF_MACFIELD_LIST_END,
        /* table fields */          RTL9300_L3_EGR_INTF_MAC_FIELDS
    },
#endif
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "L3_HOST_ROUTE_IP6MC",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     1,
        /* table size */            6144,
        /* total data registers */  11,
        /* total field numbers */   LONGAN_L3_HOST_ROUTE_IP6MCFIELD_LIST_END,
        /* table fields */          RTL9300_L3_HOST_ROUTE_IP6MC_FIELDS
    },
#endif
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "L3_HOST_ROUTE_IP6UC",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     1,
        /* table size */            6144,
        /* total data registers */  5,
        /* total field numbers */   LONGAN_L3_HOST_ROUTE_IP6UCFIELD_LIST_END,
        /* table fields */          RTL9300_L3_HOST_ROUTE_IP6UC_FIELDS
    },
#endif
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "L3_HOST_ROUTE_IPMC",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     1,
        /* table size */            6144,
        /* total data registers */  11,
        /* total field numbers */   LONGAN_L3_HOST_ROUTE_IPMCFIELD_LIST_END,
        /* table fields */          RTL9300_L3_HOST_ROUTE_IPMC_FIELDS
    },
#endif
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "L3_HOST_ROUTE_IPUC",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     1,
        /* table size */            6144,
        /* total data registers */  5,
        /* total field numbers */   LONGAN_L3_HOST_ROUTE_IPUCFIELD_LIST_END,
        /* table fields */          RTL9300_L3_HOST_ROUTE_IPUC_FIELDS
    },
#endif
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "L3_NEXTHOP",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     3,
        /* table size */            2048,
        /* total data registers */  1,
        /* total field numbers */   LONGAN_L3_NEXTHOPFIELD_LIST_END,
        /* table fields */          RTL9300_L3_NEXTHOP_FIELDS
    },
#endif
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "L3_PREFIX_ROUTE_IP6MC",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     2,
        /* table size */            512,
        /* total data registers */  20,
        /* total field numbers */   LONGAN_L3_PREFIX_ROUTE_IP6MCFIELD_LIST_END,
        /* table fields */          RTL9300_L3_PREFIX_ROUTE_IP6MC_FIELDS
    },
#endif
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "L3_PREFIX_ROUTE_IP6UC",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     2,
        /* table size */            512,
        /* total data registers */  11,
        /* total field numbers */   LONGAN_L3_PREFIX_ROUTE_IP6UCFIELD_LIST_END,
        /* table fields */          RTL9300_L3_PREFIX_ROUTE_IP6UC_FIELDS
    },
#endif
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "L3_PREFIX_ROUTE_IPMC",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     2,
        /* table size */            512,
        /* total data registers */  20,
        /* total field numbers */   LONGAN_L3_PREFIX_ROUTE_IPMCFIELD_LIST_END,
        /* table fields */          RTL9300_L3_PREFIX_ROUTE_IPMC_FIELDS
    },
#endif
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "L3_PREFIX_ROUTE_IPUC",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     2,
        /* table size */            512,
        /* total data registers */  11,
        /* total field numbers */   LONGAN_L3_PREFIX_ROUTE_IPUCFIELD_LIST_END,
        /* table fields */          RTL9300_L3_PREFIX_ROUTE_IPUC_FIELDS
    },
#endif
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "L3_ROUTER_MAC",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     0,
        /* table size */            64,
        /* total data registers */  7,
        /* total field numbers */   LONGAN_L3_ROUTER_MACFIELD_LIST_END,
        /* table fields */          RTL9300_L3_ROUTER_MAC_FIELDS
    },
#endif
#endif   /* CONFIG_SDK_CHIP_FEATURE_LAYER_3_ROUTING */
#if defined(CONFIG_SDK_CHIP_FEATURE_ACL)
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "IACL",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     2,
        /* table size */            2048,
        /* total data registers */  19,
        /* total field numbers */   LONGAN_IACLFIELD_LIST_END,
        /* table fields */          RTL9300_IACL_FIELDS
    },
#endif
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "LOG",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     3,
        /* table size */            2048,
        /* total data registers */  2,
        /* total field numbers */   LONGAN_LOGFIELD_LIST_END,
        /* table fields */          RTL9300_LOG_FIELDS
    },
#endif
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "VACL",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     2,
        /* table size */            2048,
        /* total data registers */  19,
        /* total field numbers */   LONGAN_VACLFIELD_LIST_END,
        /* table fields */          RTL9300_VACL_FIELDS
    },
#endif
#endif   /* CONFIG_SDK_CHIP_FEATURE_ACL */
#if defined(CONFIG_SDK_CHIP_FEATURE_PARSER_HSB)
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "HSB",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     0,
        /* table size */            64,
        /* total data registers */  30,
        /* total field numbers */   LONGAN_HSBFIELD_LIST_END,
        /* table fields */          RTL9300_HSB_FIELDS
    },
#endif
#endif   /* CONFIG_SDK_CHIP_FEATURE_PARSER_HSB */
#if defined(CONFIG_SDK_CHIP_FEATURE_MODIFIER_HSA)
#if defined(CONFIG_SDK_RTL9300)
    {
        #if defined(CONFIG_SDK_DUMP_TABLE_WITH_NAME)
        /* table name       */      "HSA",
        #endif  /* CONFIG_SDK_DUMP_TABLE_WITH_NAME */
        /* access table set */      99,
        /* access table type */     0,
        /* table size */            256,
        /* total data registers */  22,
        /* total field numbers */   LONGAN_HSAFIELD_LIST_END,
        /* table fields */          RTL9300_HSA_FIELDS
    },
#endif
#endif   /* CONFIG_SDK_CHIP_FEATURE_MODIFIER_HSA */
};

