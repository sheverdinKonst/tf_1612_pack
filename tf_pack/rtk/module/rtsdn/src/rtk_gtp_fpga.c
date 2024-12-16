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
 * Purpose : Definition for GTP FPGA
 *
 * Feature : The file includes the following modules and sub-modules
 *
 */

#include "rtk_dpa.h"
#include "rtk_gtp_fpga.h"
#include <common/util/rt_util.h>
#include "dal/rtrpc/rtrpc_diag.h"
#include <private/drv/nic/nic_diag.h>


rtk_dpa_vp_tunnel_db_t g_dpa_tunnelVP_db[DPA_CAPAC_VP_TUNNEL_MAX_ENTRY];

uint8 g_fpga_def_mac[] = {0x00, 0xe0, 0x4c, 0x00, 0x00, 0x01};
uint8 g_fpga_def_swMac[] = {0x00, 0xe0, 0x4c, 0x00, 0x00, 0x00};
uint8 g_fpga_etherType[] = {0x88, 0x99};
uint8 g_fpga_encapRuleCMD[] = {0x01};
uint8 g_fpga_encapRuleLen[] = {0x00, 0x20};
uint8 g_tx_initial = 0; /* For Test Only */

int
_vp_tunnel_db_find(rtk_dpa_encap_t *pEncap, rtk_gtp_fpga_vpdb_entry_sts_t *sts, int *hit_idx)
{
    uint32  index;

    *sts = RTK_GTP_FPGA_VPBD_ENTRY_STS_MISS;
    *hit_idx = 0;

    for(index = 0; index < DPA_CAPAC_VP_TUNNEL_MAX_ENTRY; index++)
    {
        if((g_dpa_tunnelVP_db[index].vp_db.diffserv == pEncap->diffserv ) &&
           (g_dpa_tunnelVP_db[index].vp_db.teid == pEncap->teid ) &&
           (g_dpa_tunnelVP_db[index].vp_db.outer_dip == pEncap->outer_dip ) &&
           (g_dpa_tunnelVP_db[index].vp_db.outer_sip == pEncap->outer_sip ) &&
           (g_dpa_tunnelVP_db[index].vp_db.identification == pEncap->identification ) &&
           (g_dpa_tunnelVP_db[index].vp_db.ttl == pEncap->ttl ) &&
           (g_dpa_tunnelVP_db[index].vp_db.outer_udp_srcPort == pEncap->outer_udp_srcPort ) &&
           (g_dpa_tunnelVP_db[index].vp_db.outer_udp_srcPort == pEncap->outer_udp_srcPort ))
        {
            *sts = RTK_GTP_FPGA_VPBD_ENTRY_STS_FOUND;
            index = VPDB_TO_FPGA_INDEX(index);
            *hit_idx = index;
            DPA_MODULE_MSG("\n[%s] FOUND FPGA index = %d",__FUNCTION__,index);
            return RT_ERR_OK;
        }
    }
    return RT_ERR_OK;
}

int
_vp_tunnel_db_add(rtk_dpa_encap_t *pEncap, int *add_index)
{
    int     index;
    int     result = 0;

    for(index = 0; index < DPA_CAPAC_VP_TUNNEL_MAX_ENTRY; index++)
    {
        if(g_dpa_tunnelVP_db[index].valid == RTK_DPA_ENTRY_STATUS_INVALID)
        {
            g_dpa_tunnelVP_db[index].vp_db.diffserv = pEncap->diffserv;
            g_dpa_tunnelVP_db[index].vp_db.teid = pEncap->teid;
            g_dpa_tunnelVP_db[index].vp_db.outer_dip = pEncap->outer_dip;
            g_dpa_tunnelVP_db[index].vp_db.outer_sip = pEncap->outer_sip;
            g_dpa_tunnelVP_db[index].vp_db.identification = pEncap->identification;
            g_dpa_tunnelVP_db[index].vp_db.ttl = pEncap->ttl;
            g_dpa_tunnelVP_db[index].vp_db.outer_udp_srcPort = pEncap->outer_udp_srcPort;
            g_dpa_tunnelVP_db[index].vp_db.outer_udp_srcPort = pEncap->outer_udp_srcPort;
            g_dpa_tunnelVP_db[index].valid = RTK_DPA_ENTRY_STATUS_VALID;
            result = 1;
            index = VPDB_TO_FPGA_INDEX(index);
            *add_index = index;
            break;
        }
    }

    if(result == 0)
        return RT_ERR_FAILED;

    if(result == 1)
    {
        DPA_MODULE_MSG("\n[%s] ADD FPGA index = %d",__FUNCTION__,index);
        DPA_MODULE_MSG("\n[%s] vp_db.diffserv = 0x%08x",__FUNCTION__,g_dpa_tunnelVP_db[FPGA_TO_VPDB_INDEX(index)].vp_db.diffserv);
        DPA_MODULE_MSG("\n[%s] vp_db.teid = 0x%08x",__FUNCTION__,g_dpa_tunnelVP_db[FPGA_TO_VPDB_INDEX(index)].vp_db.teid);
        DPA_MODULE_MSG("\n[%s] vp_db.outer_dip = 0x%08x",__FUNCTION__,g_dpa_tunnelVP_db[FPGA_TO_VPDB_INDEX(index)].vp_db.outer_dip);
        DPA_MODULE_MSG("\n[%s] vp_db.outer_sip = 0x%08x",__FUNCTION__,g_dpa_tunnelVP_db[FPGA_TO_VPDB_INDEX(index)].vp_db.outer_sip);
        DPA_MODULE_MSG("\n[%s] vp_db.identification = 0x%08x",__FUNCTION__,g_dpa_tunnelVP_db[FPGA_TO_VPDB_INDEX(index)].vp_db.identification);
        DPA_MODULE_MSG("\n[%s] vp_db.ttl = 0x%08x",__FUNCTION__,g_dpa_tunnelVP_db[FPGA_TO_VPDB_INDEX(index)].vp_db.ttl);
        DPA_MODULE_MSG("\n[%s] vp_db.outer_udp_srcPort = 0x%08x",__FUNCTION__,g_dpa_tunnelVP_db[FPGA_TO_VPDB_INDEX(index)].vp_db.outer_udp_srcPort);
        DPA_MODULE_MSG("\n[%s] vp_db.outer_udp_destPort = 0x%08x\n",__FUNCTION__,g_dpa_tunnelVP_db[FPGA_TO_VPDB_INDEX(index)].vp_db.outer_udp_destPort);
    }
    return RT_ERR_OK;
}

int
_vp_tunnel_db_del(int *pDel_index)
{
    int     index = *pDel_index;

    index = FPGA_TO_VPDB_INDEX(index);

    if(index >= DPA_CAPAC_VP_TUNNEL_MAX_ENTRY)
        return RT_ERR_FAILED;

    memset(&(g_dpa_tunnelVP_db[index].vp_db), 0, sizeof(rtk_dpa_encap_t));
    g_dpa_tunnelVP_db[index].valid = RTK_DPA_ENTRY_STATUS_INVALID;

    return RT_ERR_OK;
}

/* Use this API to compose the FPGA inband configuration packet,
   and use NIC TX this packet to FPGA

    DATA0 = Tunnel_VP
    DATA1 = reserved + match action
    DATA2 = tunnel_DestIP
    DATA3 = tunnel_SrcIP
    DATA4 = optional inherit[3:0] + identification +diffserve + TTL [7:4]
    DATA5 = TTL[3:0] + udp_DesPort + udp_SrcPort[15:4]
    DATA6 = udp_SrcPort[3:0] + gtp_flags + gtp_msgType + 12`b0
*/
int
_vp_tunnel_config_pktDataConvert(rtk_dpa_encap_t *pEncap, uint8 *pPayload, int vp_tunnel_id)
{
    uint8   covert_array[4];
    uint32  covert_value;

    memcpy((pPayload+RTK_JN_ENCAP_CONFIG_DMAC_OFF),g_fpga_def_mac, 6);
    memcpy((pPayload+RTK_JN_ENCAP_CONFIG_SMAC_OFF),g_fpga_def_swMac, 6);
    memcpy((pPayload+RTK_JN_ENCAP_CONFIG_ETHTYPE_OFF),g_fpga_etherType, 2);
    memcpy((pPayload+RTK_JN_ENCAP_CONFIG_CMD_OFF),g_fpga_encapRuleCMD, 1);
    memcpy((pPayload+RTK_JN_ENCAP_CONFIG_LEN_OFF),g_fpga_encapRuleLen, 2);

    /* Address = tunnel_vp */
    memcpy((uint8*)covert_array,(uint8*)&vp_tunnel_id,sizeof(int));
    memcpy((pPayload+RTK_JN_ENCAP_CONFIG_ADDR_OFF),covert_array, 4);

    /* DATA0 = tunnel_vp */
    memcpy((uint8*)covert_array,(uint8*)&vp_tunnel_id,sizeof(int));
    memcpy((pPayload+RTK_JN_ENCAP_CONFIG_DATA0_OFF),covert_array, 4);

    /* DATA1 = reserved + match action */
    covert_value = RTK_JN_ENCAP_CONFIG_MATCH_ACT;
    memcpy((uint8*)covert_array,(uint8*)&covert_value,sizeof(uint32));
    memcpy((pPayload+RTK_JN_ENCAP_CONFIG_DATA1_OFF),covert_array, 4);


    /* DATA2 = rule_number[11:0] + tunnel_DestIP [31:12] */
    covert_value = 0;
    covert_value = (((vp_tunnel_id)<<20) | ((pEncap->outer_dip)>>12));
    memcpy((uint8*)covert_array,(uint8*)&covert_value,sizeof(uint32));
    memcpy((pPayload+RTK_JN_ENCAP_CONFIG_DATA2_OFF),covert_array, 4);

    /* DATA3 = tunnel_DestIP [11:0] + tunnel_SrcIP [31:12]*/
    covert_value = 0;
    covert_value = (((pEncap->outer_dip)<<20) | ((pEncap->outer_sip)>>12));
    memcpy((uint8*)covert_array,(uint8*)&covert_value,sizeof(uint32));
    memcpy((pPayload+RTK_JN_ENCAP_CONFIG_DATA3_OFF),covert_array, 4);

    /* DATA4 = tunnel_SrcIP [11:0] + optional inherit[3:0] + identification */
    covert_value = 0;
    covert_value = (((pEncap->outer_sip)<<20) | (pEncap->identification));
    memcpy((uint8*)covert_array, (uint8*)&covert_value ,sizeof(uint32));
    memcpy((pPayload+RTK_JN_ENCAP_CONFIG_DATA4_OFF),covert_array, 4);

    /* DATA5 = diffserv[7:0] + TTL[7:0] + udp_DesPort[15:0] */
    covert_value = 0;
    covert_value = (((pEncap->diffserv)<<24) | ((pEncap->ttl)<<16) | (pEncap->outer_udp_destPort));
    memcpy((uint8*)covert_array, (uint8*)&covert_value,sizeof(uint32));
    memcpy((pPayload+RTK_JN_ENCAP_CONFIG_DATA5_OFF),covert_array, 4);

    /* DATA6 = udp_SrcPort[15:0] + gtp_flags[7:0] + gtp_msgType[7:0] */
    pEncap->gtp_flag = GTP_HEADER_DEFAULT_FLAG;
    pEncap->gtp_msgType = GTP_HEADER_DEFAULT_MESSAGE_TYPE;
    covert_value = 0;
    covert_value = (((pEncap->outer_udp_srcPort) << 16) | ((pEncap->gtp_flag & 0xff)<<8) | (pEncap->gtp_msgType & 0xff));
    memcpy((uint8*)covert_array,(uint8*)&covert_value ,sizeof(uint32));
    memcpy((pPayload+RTK_JN_ENCAP_CONFIG_DATA6_OFF),covert_array, 4);

    return RT_ERR_OK;
}

int
_vp_tunnel_config_pktData_dump(uint8 *pPayload)
{
    uint32  index;

    DPA_MODULE_MSG("\nInBand configuration PACKET :\n");
    for(index = 0; index < RTK_JN_ENCAP_CONFIG_PKT_SIZE; index++)
    {
        if((index%4) == 0)
            DPA_MODULE_MSG("\n");
        DPA_MODULE_MSG("%02x ",(uint8)pPayload[index]);
    }

    return RT_ERR_OK;
}

/* Use this API to compose the FPGA inband configuration packet,
   and use NIC TX this packet to FPGA

    DATA0 = Tunnel_VP
    DATA1 = reserved + match action
    DATA2 = rule_number[11:0] + tunnel_DestIP [31:12]
    DATA3 = tunnel_DestIP [11:0] + tunnel_SrcIP [31:12]
    DATA4 = tunnel_SrcIP [11:0] + optional inherit[3:0] + identification
    DATA5 = diffserv[7:0] + TTL[7:0] + udp_DesPort[15:0]
    DATA6 = udp_SrcPort[15:0] + gtp_flags[7:0] + gtp_msgType[7:0]
*/

int
_vp_tunnel_inBand_config(rtk_dpa_encap_t *pEncap, int vp_tunnel_id)
{
    uint8   *inBand_confPacket;
    int32   ret = RT_ERR_FAILED;


    inBand_confPacket = (uint8 *)malloc((sizeof(uint8)*RTK_JN_ENCAP_CONFIG_PKT_SIZE));
    memset(inBand_confPacket, 0, (sizeof(uint8)*RTK_JN_ENCAP_CONFIG_PKT_SIZE));

    _vp_tunnel_config_pktDataConvert(pEncap, inBand_confPacket, vp_tunnel_id);

    _vp_tunnel_config_pktData_dump(inBand_confPacket);

    ret = drv_nic_txData_set(RTK_DPA_UNIT, RTK_NIC_TXDATA_NO_AUTO_MODE, inBand_confPacket, RTK_JN_ENCAP_CONFIG_PKT_SIZE);
    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] drv_nic_txData_set() FAILED!!!",__FUNCTION__,__LINE__);
        free(inBand_confPacket);
        return ret;
    }
    ret = drv_nic_diagPkt_send(RTK_DPA_UNIT, 1);
    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] drv_nic_txData_set() FAILED!!!",__FUNCTION__,__LINE__);
        free(inBand_confPacket);
        return ret;
    }

    free(inBand_confPacket);

    return RT_ERR_OK;
}
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
int
rtk_gtp_fpga_module_init(void)
{
    int32   index;
    rtk_portmask_t  inBand_outputPort;
    uint8   dummy_array[8];

    memset(&inBand_outputPort, 0, sizeof(rtk_portmask_t));

    for(index = 0; index < DPA_CAPAC_VP_TUNNEL_MAX_ENTRY; index ++)
    {
        g_dpa_tunnelVP_db[index].valid = RTK_DPA_ENTRY_STATUS_INVALID;
        memset(&(g_dpa_tunnelVP_db[index].vp_db),0,sizeof(rtk_dpa_encap_t));
    }

    RTK_PORTMASK_PORT_SET(inBand_outputPort, RTK_DPA_SND_TO_PHY_PORT(DPA_FPGA_CONNECT_PORT));

    drv_nic_tag_set(RTK_DPA_UNIT, RTK_NIC_TXTAG_AUTO_MODE, dummy_array, &inBand_outputPort);

    return RT_ERR_OK;
}

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
int
rtk_gtp_fpga_tunnelVP_db_action(rtk_dpa_encap_t *pEncap, rtk_gtp_fpga_vpdb_action_t action, rtk_gtp_fpga_vpdb_entry_sts_t *pEntry_sts, int *pEntry_idx)
{
    int     ret = RT_ERR_OK;
    int     vp_idx;

    ret = _vp_tunnel_db_find(pEncap, pEntry_sts, pEntry_idx);
    if(ret != RT_ERR_OK)
        return ret;

    switch(action){
        case RTK_GTP_FPGA_VPBD_ACTION_FIND:
            DPA_MODULE_MSG("\n[%s][%d] (RTK_GTP_FPGA_VPBD_ACTION_FIND)pEntry_sts = %d\n",__FUNCTION__,__LINE__,*pEntry_sts);
        break;
        case RTK_GTP_FPGA_VPBD_ACTION_ADD:
            if(*pEntry_sts != RTK_GTP_FPGA_VPBD_ENTRY_STS_FOUND)
            {
                ret = _vp_tunnel_db_add(pEncap, pEntry_idx);
                if(ret != RT_ERR_OK)
                    return RT_ERR_FAILED;
                vp_idx = *pEntry_idx;
                _vp_tunnel_inBand_config(pEncap, vp_idx);
                DPA_MODULE_MSG("\n[%s][%d] pEntry_idx = %d\n",__FUNCTION__,__LINE__,*pEntry_idx);
            }
        break;
        case RTK_GTP_FPGA_VPBD_ACTION_DEL:
            if(*pEntry_sts == RTK_GTP_FPGA_VPBD_ENTRY_STS_FOUND)
            {
                ret = _vp_tunnel_db_del(pEntry_idx);
                if(ret != RT_ERR_OK)
                    return RT_ERR_FAILED;
            }
        break;
        default:
        return RT_ERR_FAILED;
    }
    return RT_ERR_OK;
}

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
int
rtk_gtp_fpga_compose_encap_dmac_format(int vp_tunnel_idx, uint64_t *pEncap_mac)
{
    uint64_t encap_mac;
    uint64_t vp_idx = (uint64_t)vp_tunnel_idx;
    uint64_t encap_cmd = (uint64_t)RTK_JETNETWORK_ENCAP_CMD;

    DPA_MODULE_MSG("\n[%s]vp_idx = %llu\n",__FUNCTION__,vp_idx);

    encap_mac = 0;
    encap_mac = (vp_idx & (uint64_t)0x0000000000000f00); /* Get Tunnel_VP[11:8] */
    encap_mac = (encap_mac << 36);
    encap_mac |= (encap_cmd << 40);
    vp_idx &= (uint64_t)0x00000000000000ff; /* Get Tunnel_VP[7:0] */
    encap_mac |= (vp_idx << 32);
    encap_mac |= (uint64_t)(g_dpa_tunnelVP_db[FPGA_TO_VPDB_INDEX(vp_tunnel_idx)].vp_db.teid);
    *pEncap_mac = encap_mac;
    return RT_ERR_OK;
}

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

int
rtk_gtp_fpga_compose_decap_dmac_format(int vp_tunnel_idx, uint64_t *pDecap_mac)
{
    uint64_t decap_mac;
    uint64_t vp_idx = (uint64_t)vp_tunnel_idx;
    uint64_t decap_cmd = (uint64_t)RTK_JETNETWORK_DECAP_CMD;

    decap_mac = 0;
    decap_mac = (vp_idx & (uint64_t)0x0000000000000f00); /* Get Tunnel_VP[11:8] */
    decap_mac = (decap_mac << 36);
    decap_mac |= (decap_cmd << 40);
    vp_idx &= (uint64_t)0x00000000000000ff;            /* Get Tunnel_VP[7:0] */
    decap_mac |= (vp_idx << 32);
    *pDecap_mac = decap_mac;
    return RT_ERR_OK;
}

/* Use this API to compose the FPGA inband configuration packet,
   and use NIC TX this packet to FPGA

    DATA0 = Tunnel_VP
    DATA1 = reserved + match action
    DATA2 = rule_number[11:0] + tunnel_DestIP [31:12]
    DATA3 = tunnel_DestIP [11:0] + tunnel_SrcIP [31:12]
    DATA4 = tunnel_SrcIP [11:0] + optional inherit[3:0] + identification
    DATA5 = diffserv[7:0] + TTL[7:0] + udp_DesPort[15:0]
    DATA6 = udp_SrcPort[15:0] + gtp_flags[7:0] + gtp_msgType[7:0]

*/
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
 *      For Diag shell command used.
 * Changes:
 *      None
 */
int
rtk_gtp_fpga_inBand_config(rtk_dpa_encap_t *pEncap, int vp_tunnel_id, int tx_port)
{
    uint8   *inBand_confPacket;
    int32   ret = RT_ERR_FAILED;
    rtk_portmask_t  inBand_outputPort;
    uint8   dummy_array[8];

    inBand_confPacket = (uint8 *)malloc((sizeof(uint8)*RTK_JN_ENCAP_CONFIG_PKT_SIZE));
    memset(inBand_confPacket, 0, (sizeof(uint8)*RTK_JN_ENCAP_CONFIG_PKT_SIZE));

    RTK_PORTMASK_PORT_SET(inBand_outputPort, tx_port);
    drv_nic_tag_set(RTK_DPA_UNIT, RTK_NIC_TXTAG_AUTO_MODE, dummy_array, &inBand_outputPort);

    _vp_tunnel_config_pktDataConvert(pEncap, inBand_confPacket, vp_tunnel_id);

    _vp_tunnel_config_pktData_dump(inBand_confPacket);

    ret = drv_nic_txData_set(RTK_DPA_UNIT, RTK_NIC_TXDATA_NO_AUTO_MODE, inBand_confPacket, RTK_JN_ENCAP_CONFIG_PKT_SIZE);
    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] drv_nic_txData_set() FAILED!!!",__FUNCTION__,__LINE__);
        free(inBand_confPacket);
        return ret;
    }
    ret = drv_nic_diagPkt_send(RTK_DPA_UNIT, 1);
    if(ret != RT_ERR_OK)
    {
        DPA_MODULE_MSG("\n[%s][%d] drv_nic_txData_set() FAILED!!!",__FUNCTION__,__LINE__);
        free(inBand_confPacket);
        return ret;
    }

    free(inBand_confPacket);

    return RT_ERR_OK;
}

