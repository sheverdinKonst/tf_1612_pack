/*
 * Copyright (C) 2009-2019 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * $Revision: 94857 $
 * $Date: 2019-01-16 13:46:55 +0800 (Wed, 16 Jan 2019) $
 *
 * Purpose : Definition for DPA
 *
 * Feature : The file includes the following modules and sub-modules
 *
 */

#ifndef RTK_GTP_FPGA_H
#define RTK_GTP_FPGA_H

/*
 * Include Files
 */
#include <common/type.h>
#include <common/rt_type.h>
#include <common/error.h>
#include <osal/print.h>


#define RTK_JETNETWORK_ENCAP_CMD            0x4
#define RTK_JETNETWORK_DECAP_CMD            0xc

#define RTK_GTP_DECAP_VP_ID                 0x0  /* For Decap, VP_tunnel index is not necessary!!! */
#define RTK_JN_ENCAP_CONFIG_PKT_SIZE        0x40

#define RTK_JN_DEF_FPGA_MAC                 0x00e04c000001
#define RTK_JN_DEF_SW_MAC                   0x00e04c000000

/* In band config packet offset (Byte count) */
#define RTK_JN_ENCAP_CONFIG_DMAC_OFF        0
#define RTK_JN_ENCAP_CONFIG_SMAC_OFF        6
#define RTK_JN_ENCAP_CONFIG_ETHTYPE_OFF     12
#define RTK_JN_ENCAP_CONFIG_CMD_OFF         17
#define RTK_JN_ENCAP_CONFIG_LEN_OFF         18
#define RTK_JN_ENCAP_CONFIG_ADDR_OFF        20
#define RTK_JN_ENCAP_CONFIG_DATA0_OFF       24
#define RTK_JN_ENCAP_CONFIG_DATA1_OFF       28
#define RTK_JN_ENCAP_CONFIG_DATA2_OFF       32
#define RTK_JN_ENCAP_CONFIG_DATA3_OFF       36
#define RTK_JN_ENCAP_CONFIG_DATA4_OFF       40
#define RTK_JN_ENCAP_CONFIG_DATA5_OFF       44
#define RTK_JN_ENCAP_CONFIG_DATA6_OFF       48

#define RTK_JN_ENCAP_CONFIG_MATCH_ACT       1

#define RTK_NIC_TXTAG_AUTO_MODE             1
#define RTK_NIC_TXDATA_NO_AUTO_MODE         0

#define GTP_HEADER_DEFAULT_FLAG             0x30
#define GTP_HEADER_DEFAULT_MESSAGE_TYPE     0xFF

/* Jet Network FPGA Encap rule address range is 1 ~ 2047 */
#define VPDB_TO_FPGA_INDEX(idx)            ( idx + 1 )
#define FPGA_TO_VPDB_INDEX(idx)            ( idx - 1 )


typedef enum rtk_gtp_fpga_vpdb_action_e
{
    RTK_GTP_FPGA_VPBD_ACTION_FIND=0,
    RTK_GTP_FPGA_VPBD_ACTION_ADD,
    RTK_GTP_FPGA_VPBD_ACTION_DEL,
    RTK_GTP_FPGA_VPBD_ACTION_END
}rtk_gtp_fpga_vpdb_action_t;

typedef enum rtk_gtp_fpga_vpdb_entry_sts_e
{
    RTK_GTP_FPGA_VPBD_ENTRY_STS_MISS=0,
    RTK_GTP_FPGA_VPBD_ENTRY_STS_FOUND,
    RTK_GTP_FPGA_VPBD_ENTRY_STS_END
}rtk_gtp_fpga_vpdb_entry_sts_t;


extern rtk_dpa_vp_tunnel_db_t g_dpa_tunnelVP_db[];

/* Function Name:
 *      rtk_gtp_fpga_module_init
 * Description:
 *      GTP FPGA Inital Flow.
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Applicable:
 *      9310
 * Note:
 *
 * Changes:
 *      None
 */
extern int
rtk_gtp_fpga_module_init(void);

/* Function Name:
 *      rtk_gtp_fpga_tunnelVP_db_action
 * Description:
 *      GTP FPGA VP tunnel entry action.
 * Input:
 *      rtk_dpa_encap_t         - Encap data
 *      action                  - data base action
 * Output:
 *      pEntry_sts              - Feedback the entry status.
 *      pEntry_idx              - the paramter is valid, when action is
 *                                RTK_GTP_FPGA_VPBD_ACTION_FIND and
 *                                RTK_GTP_FPGA_VPBD_ACTION_ADD
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Applicable:
 *      9310
 * Note:
 *
 * Changes:
 *      None
 */
extern int
rtk_gtp_fpga_tunnelVP_db_action(rtk_dpa_encap_t *pEncap, rtk_gtp_fpga_vpdb_action_t action, rtk_gtp_fpga_vpdb_entry_sts_t *pEntry_sts, int *pEntry_idx);

/* Function Name:
 *      rtk_gtp_fpga_compose_encap_dmac_format
 * Description:
 *      Compose GTP FPGA Encap command in DMAC.
 * Input:
 *      vp_tunnel_idx           - Specific Encap rule
 * Output:
 *      pEncap_mac              - Encap command DMAC.
 * Return:
 *      RT_ERR_OK
 * Applicable:
 *      9310
 * Note:
 *
 * Changes:
 *      None
 */
extern int
rtk_gtp_fpga_compose_encap_dmac_format(int vp_tunnel_idx, uint64_t *pEncap_mac);


/* Function Name:
 *      rtk_gtp_fpga_compose_encap_dmac_format
 * Description:
 *      Compose GTP FPGA Decap command in DMAC.
 * Input:
 *      vp_tunnel_idx           - Specific Decap rule
 * Output:
 *      pDecap_mac              - Encap command DMAC.
 * Return:
 *      RT_ERR_OK
 * Applicable:
 *      9310
 * Note:
 *
 * Changes:
 *      None
 */
extern int
rtk_gtp_fpga_compose_decap_dmac_format(int vp_tunnel_idx, uint64_t *pDecap_mac);

/* Function Name:
 *      rtk_gtp_fpga_inBand_config
 * Description:
 *      Send GTP FPGA in band configuration packet.
 * Input:
 *      rtk_dpa_encap_t         - Encap data
 *      vp_tunnel_id            - data base action
 *      tx_port                 - in band configuration TX port
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Applicable:
 *      9310
 * Note:
 *
 * Changes:
 *      None
 */
extern int
rtk_gtp_fpga_inBand_config(rtk_dpa_encap_t *pEncap, int vp_tunnel_id, int tx_port);


#endif /*RTK_GTP_FPGA_H*/


