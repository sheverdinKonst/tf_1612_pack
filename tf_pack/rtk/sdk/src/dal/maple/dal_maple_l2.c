/*
 * Copyright (C) 2009-2016 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * $Revision$
 * $Date$
 *
 * Purpose : Definition those public L2 APIs and its data type in the SDK.
 *
 * Feature : The file have include the following module and sub-modules
 *           1) l2 address table
 *
 */

/*
 * Include Files
 */
#include <common/rt_type.h>
#include <common/rt_error.h>
#include <common/util/rt_bitop.h>
#include <common/util/rt_util.h>
#include <common/debug/rt_log.h>
#include <osal/sem.h>
#include <osal/lib.h>
#include <osal/memory.h>
#include <osal/time.h>
#include <hal/chipdef/allmem.h>
#include <hal/chipdef/allreg.h>
#include <hal/chipdef/maple/rtk_maple_table_struct.h>
#include <hal/chipdef/maple/rtk_maple_reg_struct.h>
#include <hal/mac/reg.h>
#include <hal/mac/mem.h>
#include <hal/common/halctrl.h>
#include <dal/maple/dal_maple_trunk.h>
#include <dal/maple/dal_maple_l2.h>
#include <dal/maple/dal_maple_l3.h>
#include <dal/maple/dal_maple_vlan.h>
#include <dal/maple/dal_maple_trap.h>
#include <dal/dal_mgmt.h>
#include <rtk/default.h>
#include <rtk/l2.h>

/*
 * Symbol Definition
 */
#define END_OF_MCAST_IDX    (0xFFFF)
#define MCAST_IDX_ALLOCATED (0xFFFE)
#define PORT_DONT_CARE_8380 (0X1F)

/* Special definition for RTL8380 */
#define ACCADDR_L2TYPE_OFFSET (14)

#ifndef SRAM
#define SRAM 0
#endif

#ifndef CAM
#define CAM 1
#endif

typedef enum dal_maple_l2_getMethod_e
{
    L2_GET_EXIST_ONLY = 0,
    L2_GET_EXIST_OR_FREE,
    L2_GET_FREE_ONLY,
    DAL_MAPLE_GETMETHOD_END
} dal_maple_l2_getMethod_t;

typedef struct dal_maple_mcast_index_s
{
    uint16  next_index;
    uint16  ref_count;
} dal_maple_mcast_index_t;

typedef struct dal_maple_mcast_index_pool_s
{
    dal_maple_mcast_index_t *pMcast_index_pool;
    uint32                  size_of_mcast_fwd_index;
    uint16                  free_index_head;
    uint16                  free_entry_count;
} dal_maple_mcast_index_pool_t;

typedef struct dal_maple_ip6_cache_entry_s {
    uint16          rvid;
    rtk_ipv6_addr_t dip;
    rtk_ipv6_addr_t sip;
    uint8           valid;
} dal_maple_ip6_cache_entry_t;

typedef struct dal_maple_l2_info_s {
    uint8   limitLearningCnt_enable[RTK_MAX_NUM_OF_PORTS];
    uint32  limitLearningCnt[RTK_MAX_NUM_OF_PORTS];
} dal_maple_l2_info_t;
/*
 * Data Declaration
 */
static uint32               l2_init[RTK_MAX_NUM_OF_UNIT] = {INIT_NOT_COMPLETED};
static osal_mutex_t         l2_sem[RTK_MAX_NUM_OF_UNIT];
static uint32               ip4HashFmt[RTK_MAX_NUM_OF_UNIT] = {0};
static uint32               ip6HashFmt[RTK_MAX_NUM_OF_UNIT] = {0};

static dal_maple_mcast_index_pool_t    mcast_idx_pool[RTK_MAX_NUM_OF_UNIT];

static uint32               algoType[RTK_MAX_NUM_OF_UNIT] = {0};
static uint8                ipmcst_fvid_cmp;
static uint32               ip6SipCareByte;
static uint32               ip6DipCareByte;
static uint8                ucst_dlf_pmsk_inited;
static uint8                bcst_dlf_pmsk_inited;
static uint32               hashTable_size[RTK_MAX_NUM_OF_UNIT] = {0};
static uint32               cam_size[RTK_MAX_NUM_OF_UNIT] = {0};

static int16                ip6CacheIndexTbl[RTK_DEFAULT_L2_IP6_CACHE_IDX_TBL_SIZE];
static dal_maple_ip6_cache_entry_t ip6CacheTbl[RTK_DEFAULT_L2_IP6_CACHE_TBL_SIZE];
static int16                curCacheTblIdx;
static dal_maple_l2_info_t   *pL2_info[RTK_MAX_NUM_OF_UNIT];

/*
 * Macro Definition
 */
/* l2 semaphore handling */
#define L2_SEM_LOCK(unit)    \
do {\
    if (osal_sem_mutex_take(l2_sem[unit], OSAL_SEM_WAIT_FOREVER) != RT_ERR_OK)\
    {\
        RT_ERR(RT_ERR_SEM_LOCK_FAILED, (MOD_DAL|MOD_L2), "semaphore lock failed");\
        return RT_ERR_SEM_LOCK_FAILED;\
    }\
} while(0)
#define L2_SEM_UNLOCK(unit)   \
do {\
    if (osal_sem_mutex_give(l2_sem[unit]) != RT_ERR_OK)\
    {\
        RT_ERR(RT_ERR_SEM_UNLOCK_FAILED, (MOD_DAL|MOD_L2), "semaphore unlock failed");\
        return RT_ERR_SEM_UNLOCK_FAILED;\
    }\
} while(0)

/*
 * Function Declaration
 */
static int32 _dal_maple_l2_getL2EntryfromCAM(uint32 unit, uint32 index, dal_maple_l2_entry_t *pL2_entry, uint32 *pIsValid);
static int32 _dal_maple_l2_setL2CAMEntry(uint32 unit, dal_maple_l2_entry_t *pL2_entry, dal_maple_l2_index_t *pL2_index);
static int32 _dal_maple_l2_getL2EntryfromHash(uint32 unit, uint32 hashkey, uint32 location, dal_maple_l2_entry_t *pL2_entry, uint32 *pIsValid);
static int32 _dal_maple_l2_setL2HASHEntry(uint32 unit, dal_maple_l2_entry_t *pL2_entry, dal_maple_l2_index_t *pL2_index);
static int32 _dal_maple_l2_allocMcastIdx(uint32 unit, int32 *pMcastIdx);
static int32 _dal_maple_l2_isMcastIdxUsed(uint32 unit, int32 mcastIdx);
static int32 _dal_maple_l2_freeMcastIdx(uint32 unit, int32 mcastIdx);
static int32 _dal_maple_l2_compareEntry(dal_maple_l2_entry_t *pSrcEntry, dal_maple_l2_entry_t *pDstEntry);
static int32 _dal_maple_l2_entryToHashKey(uint32 unit, dal_maple_l2_entry_t *pL2_entry, uint32 *pKey);
static int32 _dal_maple_l2_getExistOrFreeL2Entry(uint32 unit, dal_maple_l2_entry_t *pL2_entry, dal_maple_l2_getMethod_t get_method, dal_maple_l2_index_t *pL2_index);
static int32 _dal_maple_l2_ip6CareByteConvertToIP(ipaddr_t *pIp4Dip, ipaddr_t *pIp4Sip, rtk_ipv6_addr_t *pIp6Dip, rtk_ipv6_addr_t *pIp6Sip);
static int32 _dal_maple_l2_ip6CareByteConvertToIP6Addr(rtk_ipv6_addr_t *pIp6Dip, rtk_ipv6_addr_t *pIp6Sip, ipaddr_t *pIp4Dip, ipaddr_t *pIp4Sip);
static int32 _dal_maple_l2_nextValidAddr_get(uint32 unit,int32 *pScan_idx, uint32 type, uint32 include_static, dal_maple_l2_entry_t *pL2_data);

#if !defined(__BOOTLOADER__)
static int32 _dal_maple_l2_init_config(uint32 unit);
#endif /* !defined(__BOOTLOADER__) */
/* Function Name:
 *      dal_maple_l2Mapper_init
 * Description:
 *      Hook l2 module of the specified device.
 * Input:
 *      pMapper - pointer of mapper
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 * Note:
 *      Must Hook l2 module before calling any l2 APIs.
 */
int32
dal_maple_l2Mapper_init(dal_mapper_t *pMapper)
{
#ifndef __BOOTLOADER__
    pMapper->l2_init = dal_maple_l2_init;
    pMapper->l2_flushLinkDownPortAddrEnable_get = dal_maple_l2_flushLinkDownPortAddrEnable_get;
    pMapper->l2_flushLinkDownPortAddrEnable_set = dal_maple_l2_flushLinkDownPortAddrEnable_set;
    pMapper->l2_ucastAddr_flush = dal_maple_l2_ucastAddr_flush;
    pMapper->l2_macLearningCnt_get = dal_maple_l2_macLearningCnt_get;
    pMapper->l2_limitLearningNum_get = dal_maple_l2_limitLearningNum_get;
    pMapper->l2_limitLearningNum_set = dal_maple_l2_limitLearningNum_set;
    pMapper->l2_limitLearningAction_get = dal_maple_l2_limitLearningAction_get;
    pMapper->l2_limitLearningAction_set = dal_maple_l2_limitLearningAction_set;
    pMapper->l2_fidLimitLearningEntry_get = dal_maple_l2_fidLimitLearningEntry_get;
    pMapper->l2_fidLimitLearningEntry_set = dal_maple_l2_fidLimitLearningEntry_set;
    pMapper->l2_fidLearningCnt_reset = dal_maple_l2_fidLearningCnt_reset;
    pMapper->l2_agingTime_get = dal_maple_l2_agingTime_get;
    pMapper->l2_agingTime_set = dal_maple_l2_agingTime_set;
    pMapper->l2_portAgingEnable_get = dal_maple_l2_portAgingEnable_get;
    pMapper->l2_portAgingEnable_set = dal_maple_l2_portAgingEnable_set;
    pMapper->l2_hashAlgo_get = dal_maple_l2_hashAlgo_get;
    pMapper->l2_hashAlgo_set = dal_maple_l2_hashAlgo_set;
    pMapper->l2_bucketHashAlgo_get = dal_maple_l2_bucketHashAlgo_get;
    pMapper->l2_bucketHashAlgo_set = dal_maple_l2_bucketHashAlgo_set;
    pMapper->l2_vlanMode_get = dal_maple_l2_vlanMode_get;
    pMapper->l2_vlanMode_set = dal_maple_l2_vlanMode_set;
    pMapper->l2_portNewMacOp_get = dal_maple_l2_portNewMacOp_get;
    pMapper->l2_portNewMacOp_set = dal_maple_l2_portNewMacOp_set;
    pMapper->l2_addr_init = dal_maple_l2_addr_init;
    pMapper->l2_addr_add = dal_maple_l2_addr_add;
    pMapper->l2_addr_del = dal_maple_l2_addr_del;
    pMapper->l2_addr_get = dal_maple_l2_addr_get;
    pMapper->l2_addr_set = dal_maple_l2_addr_set;
    pMapper->l2_addr_delAll = dal_maple_l2_addr_delAll;
    pMapper->l2_nextValidAddr_get = dal_maple_l2_nextValidAddr_get;
    pMapper->l2_mcastAddr_init = dal_maple_l2_mcastAddr_init;
    pMapper->l2_mcastAddr_add = dal_maple_l2_mcastAddr_add;
    pMapper->l2_mcastAddr_del = dal_maple_l2_mcastAddr_del;
    pMapper->l2_mcastAddr_get = dal_maple_l2_mcastAddr_get;
    pMapper->l2_mcastAddr_set = dal_maple_l2_mcastAddr_set;
    pMapper->l2_mcastAddr_addByIndex = dal_maple_l2_mcastAddr_addByIndex;
    pMapper->l2_mcastAddr_setByIndex = dal_maple_l2_mcastAddr_setByIndex;
    pMapper->l2_mcastAddr_delIgnoreIndex = dal_maple_l2_mcastAddr_delIgnoreIndex;
    pMapper->l2_nextValidMcastAddr_get = dal_maple_l2_nextValidMcastAddr_get;
    pMapper->l2_ipmcMode_get = dal_maple_l2_ipmcMode_get;
    pMapper->l2_ipmcMode_set = dal_maple_l2_ipmcMode_set;
    pMapper->l2_ipMcastAddrExt_init = dal_maple_l2_ipMcastAddrExt_init;
    pMapper->l2_ipMcastAddr_add = dal_maple_l2_ipMcastAddr_add;
    pMapper->l2_ipMcastAddr_del = dal_maple_l2_ipMcastAddr_del;
    pMapper->l2_ipMcastAddr_get = dal_maple_l2_ipMcastAddr_get;
    pMapper->l2_ipMcastAddr_set = dal_maple_l2_ipMcastAddr_set;
    pMapper->l2_ipMcastAddr_addByIndex = dal_maple_l2_ipMcastAddr_addByIndex;
    pMapper->l2_ipMcastAddr_setByIndex = dal_maple_l2_ipMcastAddr_setByIndex;
    pMapper->l2_ipMcastAddr_delIgnoreIndex = dal_maple_l2_ipMcastAddr_delIgnoreIndex;
    pMapper->l2_nextValidIpMcastAddr_get = dal_maple_l2_nextValidIpMcastAddr_get;
    pMapper->l2_ipMcastAddrChkEnable_get = dal_maple_l2_ipMcastAddrChkEnable_get;
    pMapper->l2_ipMcastAddrChkEnable_set = dal_maple_l2_ipMcastAddrChkEnable_set;
    pMapper->l2_ipMcstFidVidCompareEnable_get = dal_maple_l2_ipMcstFidVidCompareEnable_get;
    pMapper->l2_ipMcstFidVidCompareEnable_set = dal_maple_l2_ipMcstFidVidCompareEnable_set;
    pMapper->l2_ip6mcMode_get = dal_maple_l2_ip6mcMode_get;
    pMapper->l2_ip6mcMode_set = dal_maple_l2_ip6mcMode_set;
    pMapper->l2_ip6CareByte_get = dal_maple_l2_ip6CareByte_get;
    pMapper->l2_ip6CareByte_set = dal_maple_l2_ip6CareByte_set;
    pMapper->l2_ip6McastAddrExt_init = dal_maple_l2_ip6McastAddrExt_init;
    pMapper->l2_ip6McastAddr_add = dal_maple_l2_ip6McastAddr_add;
    pMapper->l2_ip6McastAddr_del = dal_maple_l2_ip6McastAddr_del;
    pMapper->l2_ip6McastAddr_get = dal_maple_l2_ip6McastAddr_get;
    pMapper->l2_ip6McastAddr_set = dal_maple_l2_ip6McastAddr_set;
    pMapper->l2_ip6McastAddr_addByIndex = dal_maple_l2_ip6McastAddr_addByIndex;
    pMapper->l2_ip6McastAddr_setByIndex = dal_maple_l2_ip6McastAddr_setByIndex;
    pMapper->l2_ip6McastAddr_delIgnoreIndex = dal_maple_l2_ip6McastAddr_delIgnoreIndex;
    pMapper->l2_nextValidIp6McastAddr_get = dal_maple_l2_nextValidIp6McastAddr_get;
    pMapper->l2_mcastFwdIndex_alloc = dal_maple_l2_mcastFwdIndex_alloc;
    pMapper->l2_mcastFwdIndex_free = dal_maple_l2_mcastFwdIndex_free;
    pMapper->l2_mcastFwdIndexFreeCount_get = dal_maple_l2_mcastFwdIndexFreeCount_get;
    pMapper->l2_mcastFwdPortmaskEntry_get = dal_maple_l2_mcastFwdPortmaskEntry_get;
    pMapper->l2_mcastFwdPortmaskEntry_set = dal_maple_l2_mcastFwdPortmaskEntry_set;
    pMapper->l2_cpuMacAddr_add = dal_maple_l2_cpuMacAddr_add;
    pMapper->l2_cpuMacAddr_del = dal_maple_l2_cpuMacAddr_del;
    pMapper->l2_portMoveAction_get = dal_maple_l2_portMoveAction_get;
    pMapper->l2_portMoveAction_set = dal_maple_l2_portMoveAction_set;
    pMapper->l2_legalPortMoveFlushAddrEnable_get = dal_maple_l2_legalPortMoveFlushAddrEnable_get;
    pMapper->l2_legalPortMoveFlushAddrEnable_set = dal_maple_l2_legalPortMoveFlushAddrEnable_set;
    pMapper->l2_staticPortMoveAction_get = dal_maple_l2_staticPortMoveAction_get;
    pMapper->l2_staticPortMoveAction_set = dal_maple_l2_staticPortMoveAction_set;
    pMapper->l2_lookupMissFloodPortMask_get = dal_maple_l2_lookupMissFloodPortMask_get;
    pMapper->l2_lookupMissFloodPortMask_add = dal_maple_l2_lookupMissFloodPortMask_add;
    pMapper->l2_lookupMissFloodPortMask_del = dal_maple_l2_lookupMissFloodPortMask_del;
    pMapper->l2_lookupMissFloodPortMask_setByIndex = dal_maple_l2_lookupMissFloodPortMask_setByIndex;
    pMapper->l2_lookupMissFloodPortMaskIdx_get = dal_maple_l2_lookupMissFloodPortMaskIdx_get;
    pMapper->l2_lookupMissFloodPortMaskIdx_set = dal_maple_l2_lookupMissFloodPortMaskIdx_set;
    pMapper->l2_portLookupMissAction_get = dal_maple_l2_portLookupMissAction_get;
    pMapper->l2_portLookupMissAction_set = dal_maple_l2_portLookupMissAction_set;
    pMapper->l2_portUcastLookupMissAction_get = dal_maple_l2_portUcastLookupMissAction_get;
    pMapper->l2_portUcastLookupMissAction_set = dal_maple_l2_portUcastLookupMissAction_set;
    pMapper->l2_srcPortEgrFilterMask_get = dal_maple_l2_srcPortEgrFilterMask_get;
    pMapper->l2_srcPortEgrFilterMask_set = dal_maple_l2_srcPortEgrFilterMask_set;
    pMapper->l2_srcPortEgrFilterMask_add = dal_maple_l2_srcPortEgrFilterMask_add;
    pMapper->l2_srcPortEgrFilterMask_del = dal_maple_l2_srcPortEgrFilterMask_del;
    pMapper->l2_exceptionAddrAction_get = dal_maple_l2_exceptionAddrAction_get;
    pMapper->l2_exceptionAddrAction_set = dal_maple_l2_exceptionAddrAction_set;
    pMapper->l2_addrEntry_get = dal_maple_l2_addrEntry_get;
    pMapper->l2_conflictAddr_get = dal_maple_l2_conflictAddr_get;
    pMapper->l2_zeroSALearningEnable_get = dal_maple_l2_zeroSALearningEnable_get;
    pMapper->l2_zeroSALearningEnable_set = dal_maple_l2_zeroSALearningEnable_set;
    pMapper->l2_secureMacMode_get = dal_maple_l2_secureMacMode_get;
    pMapper->l2_secureMacMode_set = dal_maple_l2_secureMacMode_set;
    pMapper->l2_hwNextValidAddr_get = dal_maple_l2_hwNextValidAddr_get;
    pMapper->l2_hashIdx_get = dal_maple_l2_hashIdx_get;
#else
    /* mapper for U-Boot */
    pMapper->l2_portNewMacOp_set = dal_maple_l2_portNewMacOp_set;

#endif
    return RT_ERR_OK;
}

/* Function Name:
 *      _dal_maple_l2_getL2EntryfromHash
 * Description:
 *      Get L2 Entry from Chip
 * Input:
 *      unit      - unit id
 *      hashKey   - Hash Key for this Entry
 *      location  - Entry location in Hash Bucket
 *      pL2_entry - L2 entry used to do search
 *      pIsValid  - Is valid entry
 *
 * Output:
 *      pL2_entry - L2 entry
 *      pIsValid  - Is valid or invalid entry
 *                    TRUE: valid entry
 *                    FALSE: invalid entry
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_VLAN_VID - invalid vid
 * Note:
 *      We can not distinguish an entry between IP multicast in L2 mode and L2 multicast
 */

static int32
_dal_maple_l2_getL2EntryfromHash(uint32 unit, uint32 hashkey, uint32 location, dal_maple_l2_entry_t *pL2_entry, uint32 *pIsValid)
{
    int32   ret;
    l2_entry_t  l2_entry;
    uint32  l2_index;
    uint32  ip_multi, ip6_multi;
    uint32  mac_uint32[2];
    uint32  is_l2mcast;
    uint32  aging;
    uint32  is_static;
    uint32  partialkey1;
    uint32  dablock, sablock;
    uint32  next_hop;
    uint32  suspending;
    uint32  agg_vid;
    uint32  vid_sel = 0;
    uint32  mac_idx = 0;

    RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "unit=%d, hashkey=%d, location=%d, \
           entry_type=%d, Mac=%x-%x-%x-%x-%x-%x", unit, hashkey, location, pL2_entry->entry_type,
           pL2_entry->unicast.mac.octet[0], pL2_entry->unicast.mac.octet[1], pL2_entry->unicast.mac.octet[2],
           pL2_entry->unicast.mac.octet[3], pL2_entry->unicast.mac.octet[4], pL2_entry->unicast.mac.octet[5]);

    osal_memset(&l2_entry, 0, sizeof(l2_entry));

    /* calculate l2 index in hash table */
    l2_index = (SRAM << ACCADDR_L2TYPE_OFFSET) | (hashkey << 2) | location;

    /* read entry from chip */
    if ((ret = table_read(unit, MAPLE_L2_UCt, l2_index, (uint32 *)&l2_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    /* get IP_MULTI and IP6_MULTI bits from l2_entry */
    if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_IP_MCtf, &ip_multi, (uint32 *)&l2_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_IP6_MCtf, &ip6_multi, (uint32 *)&l2_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    /* Get mac addr */
    is_l2mcast = 0;
    next_hop = 0;
    if (0 == ip_multi)
    {
        if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_MACtf, &mac_uint32[0], (uint32 *)&l2_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        is_l2mcast = (uint8) ((mac_uint32[1] >> 8) & BITMASK_1B);
        if(0 == is_l2mcast)
        {
            /* get next hop bit */
            if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_NEXT_HOPtf, &next_hop, (uint32 *)&l2_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }
        }
    }

    aging = 0;
    sablock = 0;
    dablock = 0;
    is_static = 0;
    suspending = 0;
    agg_vid = 0;

    /*
     * check whether this entry is l2 multicast entry.
     * If not l2 multicast entry, get more information for valid check
     */
    if (0 == is_l2mcast && 0 == ip_multi)
    {
        if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_AGEtf, &aging, (uint32 *)&l2_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_SA_BLKtf, &sablock, (uint32 *)&l2_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_DA_BLKtf, &dablock, (uint32 *)&l2_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_STATICtf, &is_static, (uint32 *)&l2_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_SUSPENDtf, &suspending, (uint32 *)&l2_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_NEXT_HOPtf, &next_hop, (uint32 *)&l2_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        if (next_hop == TRUE)
        {
            if ((ret = table_field_get(unit, MAPLE_L2_NEXT_HOPt, MAPLE_L2_NEXT_HOP_VLAN_TARGETtf, &vid_sel, (uint32 *)&l2_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }
            if ((ret = table_field_get(unit, MAPLE_L2_NEXT_HOPt, MAPLE_L2_NEXT_HOP_ROUTE_IDXtf, &mac_idx, (uint32 *)&l2_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }
        }
        else
        {
            if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_VIDtf, &agg_vid, (uint32 *)&l2_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }
        }
    }

    /* check whether this entry is valid entry */
    if (!(ip_multi | is_l2mcast | aging | sablock | dablock | is_static | next_hop))
    {
        /* this is not a valid entry. No need to futher process*/
        *pIsValid = FALSE;

        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pIsValid=%d", *pIsValid);
        return RT_ERR_OK;
    }
    else
    {
        *pIsValid = TRUE;
    }

    /* Extract content of each kind of entry from l2_enry */
    switch ((ip6_multi << 2) | (ip_multi << 1) | is_l2mcast)
    {
        case 0x0: /* L2 unicast */

            pL2_entry->entry_type = L2_UNICAST;

            {
                if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_SLPtf, &pL2_entry->unicast.port, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_FID_RVIDtf, &pL2_entry->unicast.fid, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                pL2_entry->unicast.mac.octet[0] = (uint8)(mac_uint32[1] >> 8);
                pL2_entry->unicast.mac.octet[1] = (uint8)(mac_uint32[1]);
                pL2_entry->unicast.mac.octet[2] = (uint8)(mac_uint32[0] >> 24);
                pL2_entry->unicast.mac.octet[3] = (uint8)(mac_uint32[0] >> 16);
                pL2_entry->unicast.mac.octet[4] = (uint8)(mac_uint32[0] >> 8);
                pL2_entry->unicast.mac.octet[5] = (uint8)(mac_uint32[0]);
                pL2_entry->unicast.sablock      = sablock;
                pL2_entry->unicast.dablock      = dablock;
                pL2_entry->unicast.is_static    = is_static;
                pL2_entry->unicast.aging        = aging;
                pL2_entry->unicast.suspending   = suspending;
                pL2_entry->unicast.nh           = next_hop;
                if(pL2_entry->unicast.nh == TRUE)
                {
                    pL2_entry->unicast.vlan_target = vid_sel;
                    pL2_entry->unicast.route_idx = mac_idx;
                }
                else
                    pL2_entry->unicast.agg_vid      = agg_vid;
            }

            /* convert FID from hash algorithm */
            pL2_entry->unicast.fid = (hashkey & BITMASK_11B) | (pL2_entry->unicast.fid & (~BITMASK_11B));
            if ((ret = _dal_maple_l2_entryToHashKey(unit, pL2_entry, &partialkey1)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }
            pL2_entry->unicast.fid  = (partialkey1 & BITMASK_11B) | (pL2_entry->unicast.fid & (~BITMASK_11B));

            break;
        case 0x1: /* l2 Multicast */
            pL2_entry->entry_type = L2_MULTICAST;

            if (table_field_get(unit, MAPLE_L2_MCt, MAPLE_L2_MC_MC_PMSK_IDXtf, &pL2_entry->l2mcast.index, (uint32 *)&l2_entry) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return RT_ERR_FAILED;
            }

            if (table_field_get(unit, MAPLE_L2_MCt, MAPLE_L2_MC_VIDtf, &pL2_entry->l2mcast.agg_vid, (uint32 *)&l2_entry) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return RT_ERR_FAILED;
            }

            if ((ret = table_field_get(unit, MAPLE_L2_MCt, MAPLE_L2_MC_FID_RVIDtf, &pL2_entry->l2mcast.rvid, (uint32 *)&l2_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            pL2_entry->l2mcast.mac.octet[0] = (uint8)(mac_uint32[1] >> 8);
            pL2_entry->l2mcast.mac.octet[1] = (uint8)(mac_uint32[1]);
            pL2_entry->l2mcast.mac.octet[2] = (uint8)(mac_uint32[0] >> 24);
            pL2_entry->l2mcast.mac.octet[3] = (uint8)(mac_uint32[0] >> 16);
            pL2_entry->l2mcast.mac.octet[4] = (uint8)(mac_uint32[0] >> 8);
            pL2_entry->l2mcast.mac.octet[5] = (uint8)(mac_uint32[0]);

            /* convert RVID from hash algorithm */
            pL2_entry->l2mcast.rvid = (hashkey & BITMASK_11B) | (pL2_entry->l2mcast.rvid & (~BITMASK_11B));
            if ((ret = _dal_maple_l2_entryToHashKey(unit, pL2_entry, &partialkey1)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }
            pL2_entry->l2mcast.rvid  = (partialkey1 & BITMASK_11B) | (pL2_entry->l2mcast.rvid & (~BITMASK_11B));
            break;
        case 0x2: /* IPv4 multicast */
            pL2_entry->entry_type = IP4_MULTICAST;
            if (LOOKUP_ON_FVID_AND_MAC == ip4HashFmt[unit])
            {
                RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "Should not come here");
            }
            else if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
            {
                if ((ret = table_field_get(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_MC_PMSK_IDXtf, &pL2_entry->ipmcast_ip_mc_sip.index, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_get(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_FID_RVIDtf, &pL2_entry->ipmcast_ip_mc_sip.rvid, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_get(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_GIPtf, &pL2_entry->ipmcast_ip_mc_sip.dip, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_get(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_SIPtf, &pL2_entry->ipmcast_ip_mc_sip.sip, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                pL2_entry->ipmcast_ip_mc_sip.sip = (hashkey & BITMASK_11B) | (pL2_entry->ipmcast_ip_mc_sip.sip & (~BITMASK_11B));
                if (( ret = _dal_maple_l2_entryToHashKey(unit, pL2_entry, &partialkey1)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                pL2_entry->ipmcast_ip_mc_sip.sip = (partialkey1 & BITMASK_11B) | (pL2_entry->ipmcast_ip_mc_sip.sip & (~BITMASK_11B));
            }
            else if (LOOKUP_ON_DIP_AND_FVID == ip4HashFmt[unit])
            {
                if ((ret = table_field_get(unit, MAPLE_L2_IP_MCt, MAPLE_L2_IP_MC_MC_PMSK_IDXtf, &pL2_entry->ipmcast_ip_mc.index, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_get(unit, MAPLE_L2_IP_MCt, MAPLE_L2_IP_MC_FID_RVIDtf, &pL2_entry->ipmcast_ip_mc.rvid, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_get(unit, MAPLE_L2_IP_MCt, MAPLE_L2_IP_MC_GIPtf, &pL2_entry->ipmcast_ip_mc.dip, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                pL2_entry->ipmcast_ip_mc.rvid = (hashkey & BITMASK_11B) | (pL2_entry->ipmcast_ip_mc.rvid & (~BITMASK_11B));
                if ((ret = _dal_maple_l2_entryToHashKey(unit, pL2_entry, &partialkey1)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                pL2_entry->ipmcast_ip_mc.rvid  = (partialkey1 & BITMASK_11B) | (pL2_entry->ipmcast_ip_mc.rvid & (~BITMASK_11B));
            }
            break;
        case 0x6: /* IPv6 multicast */
            pL2_entry->entry_type = IP6_MULTICAST;
            if (LOOKUP_ON_FVID_AND_MAC == ip6HashFmt[unit])
            {
                RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "Should not come here");
            }
            else if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
            {
                if ((ret = table_field_get(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_MC_PMSK_IDXtf, &pL2_entry->ipmcast_ip_mc_sip.index, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_get(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_FID_RVIDtf, &pL2_entry->ipmcast_ip_mc_sip.rvid, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_get(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_GIPtf, &pL2_entry->ipmcast_ip_mc_sip.dip, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_get(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_SIPtf, &pL2_entry->ipmcast_ip_mc_sip.sip, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                pL2_entry->ipmcast_ip_mc_sip.sip = (hashkey & BITMASK_11B) | (pL2_entry->ipmcast_ip_mc_sip.sip & (~BITMASK_11B));
                if ((ret = _dal_maple_l2_entryToHashKey(unit, pL2_entry, &partialkey1)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                pL2_entry->ipmcast_ip_mc_sip.sip = (partialkey1 & BITMASK_11B) | (pL2_entry->ipmcast_ip_mc_sip.sip & (~BITMASK_11B));
            }
            else if (LOOKUP_ON_DIP_AND_FVID == ip6HashFmt[unit])
            {
                if ((ret = table_field_get(unit, MAPLE_L2_IP_MCt, MAPLE_L2_IP_MC_MC_PMSK_IDXtf, &pL2_entry->ipmcast_ip_mc.index, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_get(unit, MAPLE_L2_IP_MCt, MAPLE_L2_IP_MC_FID_RVIDtf, &pL2_entry->ipmcast_ip_mc.rvid, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_get(unit, MAPLE_L2_IP_MCt, MAPLE_L2_IP_MC_GIPtf, &pL2_entry->ipmcast_ip_mc.dip, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                pL2_entry->ipmcast_ip_mc.rvid = (hashkey & BITMASK_11B) | (pL2_entry->ipmcast_ip_mc.rvid & (~BITMASK_11B));
                if ((ret = _dal_maple_l2_entryToHashKey(unit, pL2_entry, &partialkey1)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                pL2_entry->ipmcast_ip_mc.rvid  = (partialkey1 & BITMASK_11B) | (pL2_entry->ipmcast_ip_mc.rvid & (~BITMASK_11B));
            }
            break;

        default:
            return RT_ERR_FAILED;
     }

     RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pIsValid=%d", *pIsValid);

     return RT_ERR_OK;
} /* end of _dal_maple_l2_getL2EntryfromHash */

/* Function Name:
 *      _dal_maple_l2_getL2EntryfromCAM
 * Description:
 *      Get L2 Entry from CAM
 * Input:
 *      unit      - unit id
 *      index     - CAM index for this Entry
 *      pL2_entry - L2 entry used to do search
 *      pIsValid  - Is valid entry
 *
 * Output:
 *      pL2_entry - L2 entry
 *      pIsValid  - Is valid or invalid entry
 *                    TRUE: valid entry
 *                    FALSE: invalid entry
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_VLAN_VID - invalid vid
 * Note:
 *      We can not distinguish an entry between IP multicast in L2 mode and L2 multicast
 */
static int32
_dal_maple_l2_getL2EntryfromCAM(uint32 unit, uint32 index, dal_maple_l2_entry_t *pL2_entry, uint32 *pIsValid)
{
    int32   ret;
    l2cam_entry_t  l2cam_entry;
    uint32  l2_index;
    uint32  ip_multi, ip6_multi;
    uint32  mac_uint32[2];
    uint32  is_l2mcast;
    uint32  aging;
    uint32  is_static;
    uint32  dablock, sablock;
    uint32  next_hop = FALSE;
    uint32  suspending;
    uint32  agg_vid;

    RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "unit=%d, index=%d, entry_type=%d, \
           Mac=%x-%x-%x-%x-%x-%x", unit, index, pL2_entry->entry_type, pL2_entry->unicast.mac.octet[0],
           pL2_entry->unicast.mac.octet[1], pL2_entry->unicast.mac.octet[2], pL2_entry->unicast.mac.octet[3],
           pL2_entry->unicast.mac.octet[4], pL2_entry->unicast.mac.octet[5]);

    osal_memset(&l2cam_entry, 0, sizeof(l2cam_entry));

    /* calculate l2 index in hash table */
    l2_index = index;

    /* read entry from chip */
    if ((ret = table_read(unit, MAPLE_L2_CAM_UCt, l2_index, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    /* get IP_MULTI and IP6_MULTI bits from l2_entry */
    if ((ret = table_field_get(unit, MAPLE_L2_CAM_UCt, MAPLE_L2_CAM_UC_IP_MCtf, &ip_multi, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }
    if ((ret = table_field_get(unit, MAPLE_L2_CAM_UCt, MAPLE_L2_CAM_UC_IP6_MCtf, &ip6_multi, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    is_l2mcast = 0;
    /* Get mac addr */
    if (0 == ip_multi)
    {
        if ((ret = table_field_get(unit, MAPLE_L2_CAM_UCt, MAPLE_L2_CAM_UC_MACtf, &mac_uint32[0], (uint32 *)&l2cam_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        if ((ret = table_field_get(unit, MAPLE_L2_CAM_UCt, MAPLE_L2_CAM_UC_NEXT_HOPtf, &next_hop, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        if (0 == next_hop)
            is_l2mcast = (uint8) ((mac_uint32[1] >> 8) & BITMASK_1B);
    }

    aging = 0;
    sablock = 0;
    dablock = 0;
    is_static = 0;
    suspending = 0;
    agg_vid = 0;

    /*
     * check whether this entry is l2 multicast entry.
     * If not l2 multicast entry, get more information for valid check
     */
    if (0 == is_l2mcast && 0 == ip_multi)
    {
        if ((ret = table_field_get(unit, MAPLE_L2_CAM_UCt, MAPLE_L2_CAM_UC_AGEtf, &aging, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        if ((ret = table_field_get(unit, MAPLE_L2_CAM_UCt, MAPLE_L2_CAM_UC_SA_BLKtf, &sablock, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        if ((ret = table_field_get(unit, MAPLE_L2_CAM_UCt, MAPLE_L2_CAM_UC_DA_BLKtf, &dablock, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        if ((ret = table_field_get(unit, MAPLE_L2_CAM_UCt, MAPLE_L2_CAM_UC_STATICtf, &is_static, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        if ((ret = table_field_get(unit, MAPLE_L2_CAM_UCt, MAPLE_L2_CAM_UC_SUSPENDtf, &suspending, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        if ((ret = table_field_get(unit, MAPLE_L2_CAM_UCt, MAPLE_L2_CAM_UC_VIDtf, &agg_vid, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }
    }

    /* check whether this entry is valid entry */
    if (!(ip_multi | is_l2mcast | aging | sablock | dablock | is_static | next_hop))
    {
        /* this is not a valid entry. No need to futher process*/
        *pIsValid = FALSE;
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pIsValid=%d", *pIsValid);
        return RT_ERR_OK;
    }
    else
    {
        *pIsValid = TRUE;
    }

    /* Extract content of each kind of entry from l2_enry */
    switch ((ip6_multi << 2) | (ip_multi << 1) | is_l2mcast)
    {
        case 0x00: /* L2 unicast */
            pL2_entry->entry_type = L2_UNICAST;
            if ((ret = table_field_get(unit, MAPLE_L2_CAM_UCt, MAPLE_L2_CAM_UC_SLPtf, &pL2_entry->unicast.port, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            if ((ret = table_field_get(unit, MAPLE_L2_CAM_UCt, MAPLE_L2_CAM_UC_FID_RVIDtf, &pL2_entry->unicast.fid, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            pL2_entry->unicast.mac.octet[0] = (uint8)(mac_uint32[1] >> 8);
            pL2_entry->unicast.mac.octet[1] = (uint8)(mac_uint32[1]);
            pL2_entry->unicast.mac.octet[2] = (uint8)(mac_uint32[0] >> 24);
            pL2_entry->unicast.mac.octet[3] = (uint8)(mac_uint32[0] >> 16);
            pL2_entry->unicast.mac.octet[4] = (uint8)(mac_uint32[0] >> 8);
            pL2_entry->unicast.mac.octet[5] = (uint8)(mac_uint32[0]);

            pL2_entry->unicast.sablock      = sablock;
            pL2_entry->unicast.dablock      = dablock;
            pL2_entry->unicast.is_static    = is_static;
            pL2_entry->unicast.aging        = aging;
            pL2_entry->unicast.suspending   = suspending;
            pL2_entry->unicast.nh           = next_hop;
            pL2_entry->unicast.agg_vid      = agg_vid;

            break;
        case 0x01: /* l2 Multicast */
            pL2_entry->entry_type = L2_MULTICAST;
            if ((ret = table_field_get(unit, MAPLE_L2_CAM_MCt, MAPLE_L2_CAM_MC_MC_PMSK_IDXtf, &pL2_entry->l2mcast.index, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            if ((ret = table_field_get(unit, MAPLE_L2_CAM_MCt, MAPLE_L2_CAM_MC_VIDtf, &pL2_entry->l2mcast.agg_vid, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            if ((ret = table_field_get(unit, MAPLE_L2_CAM_MCt, MAPLE_L2_CAM_MC_FID_RVIDtf, &pL2_entry->l2mcast.rvid, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            pL2_entry->l2mcast.mac.octet[0] = (uint8)(mac_uint32[1] >> 8);
            pL2_entry->l2mcast.mac.octet[1] = (uint8)(mac_uint32[1]);
            pL2_entry->l2mcast.mac.octet[2] = (uint8)(mac_uint32[0] >> 24);
            pL2_entry->l2mcast.mac.octet[3] = (uint8)(mac_uint32[0] >> 16);
            pL2_entry->l2mcast.mac.octet[4] = (uint8)(mac_uint32[0] >> 8);
            pL2_entry->l2mcast.mac.octet[5] = (uint8)(mac_uint32[0]);

            break;
        case 0x2: /* IPv4 multicast */
            pL2_entry->entry_type = IP4_MULTICAST;
            if (LOOKUP_ON_FVID_AND_MAC == ip4HashFmt[unit])
            {
                RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "Should not come here");
            }
            else if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
            {
                if ((ret = table_field_get(unit, MAPLE_L2_CAM_IP_MC_SIPt, MAPLE_L2_CAM_IP_MC_SIP_MC_PMSK_IDXtf, &pL2_entry->ipmcast_ip_mc_sip.index, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                if ((ret = table_field_get(unit, MAPLE_L2_CAM_IP_MC_SIPt, MAPLE_L2_CAM_IP_MC_SIP_FID_RVIDtf, &pL2_entry->ipmcast_ip_mc_sip.rvid, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                if ((ret = table_field_get(unit, MAPLE_L2_CAM_IP_MC_SIPt, MAPLE_L2_CAM_IP_MC_SIP_SIPtf, &pL2_entry->ipmcast_ip_mc_sip.sip, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                if ((ret = table_field_get(unit, MAPLE_L2_CAM_IP_MC_SIPt, MAPLE_L2_CAM_IP_MC_SIP_GIPtf, &pL2_entry->ipmcast_ip_mc_sip.dip, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                };
            }
            else if (LOOKUP_ON_DIP_AND_FVID == ip4HashFmt[unit])
            {
                if ((ret = table_field_get(unit, MAPLE_L2_CAM_IP_MCt, MAPLE_L2_CAM_IP_MC_MC_PMSK_IDXtf, &pL2_entry->ipmcast_ip_mc.index, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                if ((ret = table_field_get(unit, MAPLE_L2_CAM_IP_MCt, MAPLE_L2_CAM_IP_MC_FID_RVIDtf, &pL2_entry->ipmcast_ip_mc.rvid, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                if ((ret = table_field_get(unit, MAPLE_L2_CAM_IP_MCt, MAPLE_L2_CAM_IP_MC_GIPtf, &pL2_entry->ipmcast_ip_mc.dip, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
            }

            break;
        case 0x6: /* IPv6 multicast */
            pL2_entry->entry_type = IP6_MULTICAST;
            if (LOOKUP_ON_FVID_AND_MAC == ip6HashFmt[unit])
            {
                RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "Should not come here");
            }
            else if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
            {
                if ((ret = table_field_get(unit, MAPLE_L2_CAM_IP_MC_SIPt, MAPLE_L2_CAM_IP_MC_SIP_MC_PMSK_IDXtf, &pL2_entry->ipmcast_ip_mc_sip.index, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                if ((ret = table_field_get(unit, MAPLE_L2_CAM_IP_MC_SIPt, MAPLE_L2_CAM_IP_MC_SIP_FID_RVIDtf, &pL2_entry->ipmcast_ip_mc_sip.rvid, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                if ((ret = table_field_get(unit, MAPLE_L2_CAM_IP_MC_SIPt, MAPLE_L2_CAM_IP_MC_SIP_SIPtf, &pL2_entry->ipmcast_ip_mc_sip.sip, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                if ((ret = table_field_get(unit, MAPLE_L2_CAM_IP_MC_SIPt, MAPLE_L2_CAM_IP_MC_SIP_GIPtf, &pL2_entry->ipmcast_ip_mc_sip.dip, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
            }
            else if (LOOKUP_ON_DIP_AND_FVID == ip6HashFmt[unit])
            {
                if ((ret = table_field_get(unit, MAPLE_L2_CAM_IP_MCt, MAPLE_L2_CAM_IP_MC_MC_PMSK_IDXtf, &pL2_entry->ipmcast_ip_mc.index, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                if ((ret = table_field_get(unit, MAPLE_L2_CAM_IP_MCt, MAPLE_L2_CAM_IP_MC_FID_RVIDtf, &pL2_entry->ipmcast_ip_mc.rvid, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                if ((ret = table_field_get(unit, MAPLE_L2_CAM_IP_MCt, MAPLE_L2_CAM_IP_MC_GIPtf, &pL2_entry->ipmcast_ip_mc.dip, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
            }
            break;
        default:
            return RT_ERR_FAILED;
     }

     RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pIsValid=%d", *pIsValid);

     return RT_ERR_OK;
} /* end of _dal_maple_l2_getL2EntryfromCAM */

/* Function Name:
 *      _dal_maple_l2_getExistOrFreeL2Entry
 * Description:
 *      Get exist entry or free entry
 * Input:
 *      unit       - unit id
 *      pL2_entry  - L2 entry used to do search
 *      pL2_index  - index of found entry(free or exist)
 *                      - index_type = IN_CAM mean this index is in CAM
 *                      - index_type = IN_HASH mean this index is in HASH
 *      get_method - use which method to get
 *                      - L2_GET_EXIST_ONLY
 *                      - L2_GET_EXIST_OR_FREE
 *                      - L2_GET_FREE_ONLY
 * Output:
 *      pL2_entry  - L2 entry
 *      pL2_index  - the index of L2 entry
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_VLAN_VID - invalid vid
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      None
 */
static int32
_dal_maple_l2_getExistOrFreeL2Entry(uint32 unit, dal_maple_l2_entry_t *pL2_entry, dal_maple_l2_getMethod_t get_method
        , dal_maple_l2_index_t *pL2_index)
{
    int32   ret;
    dal_maple_l2_entry_t  l2_entry;
    uint32  hashkey;
    uint32  hash_depth;
    uint32  cam_index;
    uint32  isValid;
    uint32  found_exist;
    uint32  found_free;
    uint32  value;
    rtk_enable_t    l2CamEbl;

    RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "unit=%d, entry_type=%d, Mac=%x-%x-%x-%x-%x-%x\
           get_method=%d, ",
           unit, pL2_entry->entry_type, pL2_entry->unicast.mac.octet[0], pL2_entry->unicast.mac.octet[1],
           pL2_entry->unicast.mac.octet[2], pL2_entry->unicast.mac.octet[3], pL2_entry->unicast.mac.octet[4],
           pL2_entry->unicast.mac.octet[5], get_method);

    /* parameter check */
    RT_PARAM_CHK((NULL == pL2_entry) || (NULL == pL2_index), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((get_method >= DAL_MAPLE_GETMETHOD_END), RT_ERR_INPUT);

    osal_memset(&l2_entry, 0, sizeof(l2_entry));

    pL2_entry->is_entry_exist = FALSE;

    /* calculate hash key */
    if ((ret = _dal_maple_l2_entryToHashKey(unit, pL2_entry, &hashkey)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    found_exist = FALSE;
    found_free = FALSE;

    /* Search L2 unicast entry in hash table */
    for (hash_depth = 0; hash_depth < HAL_L2_HASHDEPTH(unit); hash_depth++)
    {
        if (_dal_maple_l2_getL2EntryfromHash(unit, hashkey, hash_depth, &l2_entry, &isValid) != RT_ERR_OK)
        {
            /* not found in hash table, search CAM */
            break;
        }

        if ((TRUE == isValid)
            && (get_method == L2_GET_EXIST_ONLY || get_method == L2_GET_EXIST_OR_FREE)
            && (l2_entry.entry_type == pL2_entry->entry_type))
        {
            if (_dal_maple_l2_compareEntry(&l2_entry, pL2_entry) == RT_ERR_OK)
            {
                found_exist = TRUE;
                pL2_index->index_type = L2_IN_HASH;
                pL2_index->index = hashkey;
                pL2_index->hashdepth = hash_depth;
                osal_memcpy(pL2_entry, &l2_entry, sizeof(l2_entry));
                pL2_entry->is_entry_exist = TRUE;
                return RT_ERR_OK;
            }
        }
        else if (FALSE == isValid)
        {
            if (L2_GET_FREE_ONLY == get_method)
            {
                found_free = TRUE;
                pL2_index->index_type = L2_IN_HASH;
                pL2_index->index = hashkey;
                pL2_index->hashdepth = hash_depth;
                return RT_ERR_OK;
            }
            else if ((L2_GET_EXIST_OR_FREE == get_method) && FALSE == found_free)
            {
                found_free = TRUE;
                pL2_index->index_type = L2_IN_HASH;
                pL2_index->index = hashkey;
                pL2_index->hashdepth = hash_depth;
            }
        }
    }

    if ((ret = reg_field_read(unit, MAPLE_L2_CTRL_0r, MAPLE_LUTCAM_ENf, &value)) != RT_ERR_OK)
        return ret;
    l2CamEbl = value ? ENABLED : DISABLED;
    for (cam_index = 0; l2CamEbl && (cam_index < cam_size[unit]); cam_index++)
    {
        if (_dal_maple_l2_getL2EntryfromCAM(unit, cam_index, &l2_entry, &isValid) != RT_ERR_OK)
        {
            /* not found */
            break;
        }

        if ((TRUE == isValid)
            && (get_method == L2_GET_EXIST_ONLY || get_method == L2_GET_EXIST_OR_FREE)
            && (l2_entry.entry_type == pL2_entry->entry_type))
        {
            if (l2_entry.entry_type == L2_UNICAST && (l2_entry.unicast.nh == TRUE))
                continue;

            if (_dal_maple_l2_compareEntry(&l2_entry, pL2_entry) == RT_ERR_OK)
            {
                found_exist = TRUE;
                pL2_index->index_type = L2_IN_CAM;
                pL2_index->index = cam_index;
                pL2_index->hashdepth = 0;
                osal_memcpy(pL2_entry, &l2_entry, sizeof(l2_entry));
                pL2_entry->is_entry_exist = TRUE;
                return RT_ERR_OK;
            }
        }
        else if (FALSE == isValid)
        {
            if (L2_GET_FREE_ONLY == get_method)
            {
                found_free = TRUE;
                pL2_index->index_type = L2_IN_CAM;
                pL2_index->index = cam_index;
                pL2_index->hashdepth = 0;
                return RT_ERR_OK;
            }
            else if ((L2_GET_EXIST_OR_FREE == get_method) && FALSE == found_free)
            {
                found_free = TRUE;
                pL2_index->index_type = L2_IN_CAM;
                pL2_index->index = cam_index;
                pL2_index->hashdepth = 0;
            }
        }
    }

    if ((L2_GET_EXIST_OR_FREE == get_method) && (TRUE == found_free))
    {
        return RT_ERR_OK;
    }
    else if ((L2_GET_EXIST_ONLY == get_method) && (FALSE == found_exist))
    {
        return RT_ERR_L2_ENTRY_NOTFOUND;
    }
    return RT_ERR_L2_NO_EMPTY_ENTRY;
} /* end of _dal_maple_l2_getExistOrFreeL2Entry */


/* Function Name:
 *      _dal_maple_l2_getFirstDynamicEntry
 * Description:
 *      Get first dynamic entry
 * Input:
 *      unit       - unit id
 *      pL2_entry  - L2 entry used to do search
 *      pL2_index  - index of found entry(free or exist)
 *                      - index_type = IN_CAM mean this index is in CAM
 *                      - index_type = IN_HASH mean this index is in HASH
 * Output:
 *      pL2_entry  - L2 entry
 *      pL2_index  - the index of L2 entry
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_UNIT_ID  - invalid unit id
 *      RT_ERR_VLAN_VID - invalid vid
 *      RT_ERR_INPUT    - invalid input parameter
 *      RT_ERR_L2_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      None
 */
static int32
_dal_maple_l2_getFirstDynamicEntry(uint32 unit, dal_maple_l2_entry_t *pL2_entry
        , dal_maple_l2_index_t *pL2_index)
{
    int32   ret;
    dal_maple_l2_entry_t  l2_entry;
    uint32  hashkey;
    uint32  hash_depth;
    uint32  cam_index;
    uint32  isValid;
    uint32  value;
    rtk_enable_t    l2CamEbl;

    /* parameter check */
    RT_PARAM_CHK((NULL == pL2_entry) || (NULL == pL2_index), RT_ERR_NULL_POINTER);

    RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "unit=%d, entry_type=%d, Mac=%x-%x-%x-%x-%x-%x",
           unit, pL2_entry->entry_type, pL2_entry->unicast.mac.octet[0], pL2_entry->unicast.mac.octet[1],
           pL2_entry->unicast.mac.octet[2], pL2_entry->unicast.mac.octet[3], pL2_entry->unicast.mac.octet[4],
           pL2_entry->unicast.mac.octet[5]);

    osal_memset(&l2_entry, 0, sizeof(l2_entry));

    pL2_entry->is_entry_exist = FALSE;

    /* calculate hash key */
    if ((ret = _dal_maple_l2_entryToHashKey(unit, pL2_entry, &hashkey)) != RT_ERR_OK)
    {
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    /* Search L2 unicast entry in hash table */
    for (hash_depth = 0; hash_depth < HAL_L2_HASHDEPTH(unit); hash_depth++)
    {
        if (_dal_maple_l2_getL2EntryfromHash(unit, hashkey, hash_depth, &l2_entry, &isValid) != RT_ERR_OK)
        {
            /* not found in hash table, search CAM */
            break;
        }

        if ((TRUE == isValid ) && (l2_entry.entry_type == L2_UNICAST))
        {
            if (l2_entry.unicast.sablock == 0 && l2_entry.unicast.dablock == 0 &&
                l2_entry.unicast.is_static == 0 && l2_entry.unicast.nh == 0 && l2_entry.unicast.suspending == 0)
            {
                //found_exist = TRUE;
                pL2_index->index_type = L2_IN_HASH;
                pL2_index->index = hashkey;
                pL2_index->hashdepth = hash_depth;
                osal_memcpy(pL2_entry, &l2_entry, sizeof(l2_entry));
                pL2_entry->is_entry_exist = TRUE;
                return RT_ERR_OK;
            }
        }
    }

    if ((ret = reg_field_read(unit, MAPLE_L2_CTRL_0r, MAPLE_LUTCAM_ENf, &value)) != RT_ERR_OK)
        return ret;
    l2CamEbl = value ? ENABLED : DISABLED;
    for (cam_index = 0; l2CamEbl && (cam_index < cam_size[unit]); cam_index++)
    {
        if (_dal_maple_l2_getL2EntryfromCAM(unit, cam_index, &l2_entry, &isValid) != RT_ERR_OK)
        {
            /* not found in hash table, search CAM */
            break;
        }

        if ((TRUE == isValid ) && (l2_entry.entry_type == L2_UNICAST))
        {
            if (l2_entry.unicast.sablock == 0 && l2_entry.unicast.dablock == 0 &&
                l2_entry.unicast.is_static == 0 && l2_entry.unicast.suspending == 0)
            {
                //found_exist = TRUE;
                pL2_index->index_type = L2_IN_CAM;
                pL2_index->index = cam_index;
                osal_memcpy(pL2_entry, &l2_entry, sizeof(l2_entry));
                pL2_entry->is_entry_exist = TRUE;
                return RT_ERR_OK;
            }
        }
    }

    return RT_ERR_L2_ENTRY_NOTFOUND;
}

/* Function Name:
 *      _dal_maple_l2_allocMcastIdx
 * Description:
 *      get a free mcast index
 * Input:
 *      unit      - unit id
 *      pMcastIdx - buffer to store free idx
 * Output:
 *
 * Return:
 *      RT_ERR_OK               - key of two entry is same
 *      RT_ERR_L2_INDEXTBL_FULL - L2 index table is full
 * Note:
 *      None
 */
static int32 _dal_maple_l2_allocMcastIdx(uint32 unit, int32 *pMcastIdx)
{
    int16   free_idx;
    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d", unit);

    if (*pMcastIdx >= 0)
    {
        mcast_idx_pool[unit].pMcast_index_pool[*pMcastIdx].ref_count++;
        return RT_ERR_OK;
    }

    while(mcast_idx_pool[unit].free_index_head != END_OF_MCAST_IDX)
    {
        free_idx = mcast_idx_pool[unit].free_index_head;
        mcast_idx_pool[unit].free_index_head = mcast_idx_pool[unit].pMcast_index_pool[free_idx].next_index;
        mcast_idx_pool[unit].pMcast_index_pool[free_idx].next_index = MCAST_IDX_ALLOCATED;

        if (0 == mcast_idx_pool[unit].pMcast_index_pool[free_idx].ref_count)
        {
            *pMcastIdx = free_idx;
            mcast_idx_pool[unit].pMcast_index_pool[free_idx].ref_count++;
            mcast_idx_pool[unit].free_entry_count--;
            return RT_ERR_OK;
        }
    }

    return RT_ERR_L2_INDEXTBL_FULL;
} /* end of _dal_maple_l2_allocMcastIdx */

static int32 _dal_maple_l2_ip6CareByteConvertToIP(ipaddr_t *pIp4Dip, ipaddr_t *pIp4Sip, rtk_ipv6_addr_t * pIp6Dip, rtk_ipv6_addr_t * pIp6Sip)
{
    uint8 byte[4];

    byte[0] = pIp6Dip->octet[15 - (ip6DipCareByte & 0xf)];
    byte[1] = pIp6Dip->octet[15 - ((ip6DipCareByte & 0xf0) >> 4)];
    byte[2] = pIp6Dip->octet[15 - ((ip6DipCareByte & 0xf00) >> 8)];
    byte[3] = pIp6Dip->octet[15 - ((ip6DipCareByte & 0xf000) >> 12)];
    *pIp4Dip = (byte[3] << 24) | (byte[2] << 16) | (byte[1] << 8) | byte[0];

    byte[0] = pIp6Sip->octet[15 - (ip6SipCareByte & 0xf)];
    byte[1] = pIp6Sip->octet[15 - ((ip6SipCareByte & 0xf0) >> 4)];
    byte[2] = pIp6Sip->octet[15 - ((ip6SipCareByte & 0xf00) >> 8)];
    byte[3] = pIp6Sip->octet[15 - ((ip6SipCareByte & 0xf000) >> 12)];
    *pIp4Sip = (byte[3] << 24) | (byte[2] << 16) | (byte[1] << 8) | byte[0];

    return RT_ERR_OK;
} /* end of _dal_maple_l2_ip6CareByteConvertToIP */

static int32 _dal_maple_l2_ip6CareByteConvertToIP6Addr(rtk_ipv6_addr_t *pIp6Dip, rtk_ipv6_addr_t *pIp6Sip, ipaddr_t *pIp4Dip, ipaddr_t *pIp4Sip)
{
    uint8 byte[4];

    byte[0] = *pIp4Dip & 0xff;
    byte[1] = (*pIp4Dip & 0xff00) >> 8;
    byte[2] = (*pIp4Dip & 0xff0000) >> 16;
    byte[3] = (*pIp4Dip & 0xff000000) >> 24;
    pIp6Dip->octet[15 - (ip6DipCareByte & 0xf)] = byte[0];
    pIp6Dip->octet[15 - ((ip6DipCareByte & 0xf0) >> 4)] = byte[1];
    pIp6Dip->octet[15 - ((ip6DipCareByte & 0xf00) >> 8)] = byte[2];
    pIp6Dip->octet[15 - ((ip6DipCareByte & 0xf000) >> 12)] = byte[3];

    byte[0] = *pIp4Sip & 0xff;
    byte[1] = (*pIp4Sip & 0xff00) >> 8;
    byte[2] = (*pIp4Sip & 0xff0000) >> 16;
    byte[3] = (*pIp4Sip & 0xff000000) >> 24;
    pIp6Sip->octet[15 - (ip6SipCareByte & 0xf)] = byte[0];
    pIp6Sip->octet[15 - ((ip6SipCareByte & 0xf0) >> 4)] = byte[1];
    pIp6Sip->octet[15 - ((ip6SipCareByte & 0xf00) >> 8)] = byte[2];
    pIp6Sip->octet[15 - ((ip6SipCareByte & 0xf000) >> 12)] = byte[3];

    return RT_ERR_OK;
} /* end of _dal_maple_l2_ip6CareByteConvertToIP6Addr */

/* Function Name:
 *      _dal_maple_l2_freeMcastIdx
 * Description:
 *      Free a mcast index
 * Input:
 *      unit     - unit id
 *      mcastIdx - multicast index to free
 * Output:
 *
 * Return:
 *      RT_ERR_OK     - key of two entry is same
 *      RT_ERR_FAILED - key of two entry is different
 * Note:
 *      None
 */
static int32 _dal_maple_l2_freeMcastIdx(uint32 unit, int32 mcastIdx)
{
    int32   ret;
    multicast_index_entry_t mcast_entry;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, mcastIdx=%d", unit, mcastIdx);
    RT_PARAM_CHK((mcastIdx >= (int32)mcast_idx_pool[unit].size_of_mcast_fwd_index) || mcastIdx < 0, RT_ERR_L2_MULTI_FWD_INDEX);

    if(mcast_idx_pool[unit].pMcast_index_pool[mcastIdx].ref_count >0)
    {
        mcast_idx_pool[unit].pMcast_index_pool[mcastIdx].ref_count--;

        if (0 == mcast_idx_pool[unit].pMcast_index_pool[mcastIdx].ref_count)
        {
            if ( MCAST_IDX_ALLOCATED == mcast_idx_pool[unit].pMcast_index_pool[mcastIdx].next_index)
            {
                mcast_idx_pool[unit].pMcast_index_pool[mcastIdx].next_index = mcast_idx_pool[unit].free_index_head;
                mcast_idx_pool[unit].free_index_head = mcastIdx;
                mcast_idx_pool[unit].free_entry_count++;
            }

            /* when multicast entry is freed, reset the portmask to zero */
            osal_memset(&mcast_entry, 0, sizeof(mcast_entry));
            if ((ret = table_write(unit, MAPLE_MC_PMSKt, mcastIdx, (uint32 *)&mcast_entry)) != RT_ERR_OK)
            {
                RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
                return ret;
            }
        }
    }

    return RT_ERR_OK;
} /* end of _dal_maple_l2_freeMcastIdx */

/* Function Name:
 *      _dal_maple_l2_isMcastIdxUsed
 * Description:
 *      Free a mcast index
 * Input:
 *      unit     - unit id
 *      mcastIdx - multicast index for checking
 * Output:
 *
 * Return:
 *      RT_ERR_OK     - index is used
 *      RT_ERR_FAILED - index is not used
 * Note:
 *      None
 */
static int32 _dal_maple_l2_isMcastIdxUsed(uint32 unit, int32 mcastIdx)
{
    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, mcastIdx=%d", unit, mcastIdx);

    if (0 != mcast_idx_pool[unit].pMcast_index_pool[mcastIdx].ref_count)
    {
        return RT_ERR_OK;
    }
    else
    {
        return RT_ERR_FAILED;
    }

} /* end of _dal_maple_l2_isMcastIdxUsed */

/* Function Name:
 *      _dal_maple_l2_entryToHashKey
 * Description:
 *      Translate L2 entry to seed of Hash
 * Input:
 *      unit      - unit id
 *      pL2_entry - L2 entry used to generate seed
 *      pKey      - key for Hash
 * Output:
 *      pKey      - key for hash
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *      None
 */
static int32
_dal_maple_l2_entryToHashKey(uint32 unit, dal_maple_l2_entry_t *pL2_entry, uint32 *pKey)
{
    uint64  hashSeed = 0;
    uint32  hash1 = 0, hash3 = 0, hash4 = 0;

    RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "unit=%d, entry_type=%d, \
           Mac=%x-%x-%x-%x-%x-%x", unit, pL2_entry->entry_type,
           pL2_entry->unicast.mac.octet[0], pL2_entry->unicast.mac.octet[1], pL2_entry->unicast.mac.octet[2],
           pL2_entry->unicast.mac.octet[3], pL2_entry->unicast.mac.octet[4], pL2_entry->unicast.mac.octet[5]);

    /* get hash seed from l2 entry */
    switch (pL2_entry->entry_type)
    {
        case L2_UNICAST:
            /* if it is unicast, key will be fid+mac */
            hashSeed = (((uint64)pL2_entry->unicast.mac.octet[0] << 52) |
                        ((uint64)pL2_entry->unicast.mac.octet[1] << 44) |
                        ((uint64)pL2_entry->unicast.mac.octet[2] << 36) |
                        ((uint64)pL2_entry->unicast.mac.octet[3] << 28) |
                        ((uint64)pL2_entry->unicast.mac.octet[4] << 20) |
                        ((uint64)pL2_entry->unicast.mac.octet[5] << 12) |
                        ((uint64)pL2_entry->unicast.fid));
            break;
        case L2_MULTICAST:
            /* if it is l2 multicast, key will be rvid+mac */
            hashSeed = (((uint64)pL2_entry->l2mcast.mac.octet[0] << 52) |
                        ((uint64)pL2_entry->l2mcast.mac.octet[1] << 44) |
                        ((uint64)pL2_entry->l2mcast.mac.octet[2] << 36) |
                        ((uint64)pL2_entry->l2mcast.mac.octet[3] << 28) |
                        ((uint64)pL2_entry->l2mcast.mac.octet[4] << 20) |
                        ((uint64)pL2_entry->l2mcast.mac.octet[5] << 12) |
                        ((uint64)pL2_entry->l2mcast.rvid));
            break;
        case IP4_MULTICAST:
            if (LOOKUP_ON_FVID_AND_MAC == ip4HashFmt[unit])
            {
                hashSeed = (((uint64)pL2_entry->ipmcast_mc_ip.mac.octet[0] << 52) |
                            ((uint64)pL2_entry->ipmcast_mc_ip.mac.octet[1] << 44) |
                            ((uint64)pL2_entry->ipmcast_mc_ip.mac.octet[2] << 36) |
                            ((uint64)pL2_entry->ipmcast_mc_ip.mac.octet[3] << 28) |
                            ((uint64)pL2_entry->ipmcast_mc_ip.mac.octet[4] << 20) |
                            ((uint64)pL2_entry->ipmcast_mc_ip.mac.octet[5] << 12) |
                            ((uint64)pL2_entry->ipmcast_mc_ip.rvid));
            }
            else if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
            {
                hashSeed = (((uint64)pL2_entry->ipmcast_ip_mc_sip.dip << 32) |
                            ((uint64)pL2_entry->ipmcast_ip_mc_sip.sip));
            }
            else if (LOOKUP_ON_DIP_AND_FVID == ip4HashFmt[unit])
            {
                hashSeed = (((uint64)pL2_entry->ipmcast_ip_mc.dip << 12) |
                            ((uint64)pL2_entry->ipmcast_ip_mc.rvid));
            }
            break;
        case IP6_MULTICAST:
            if (LOOKUP_ON_FVID_AND_MAC == ip6HashFmt[unit])
            {
                hashSeed = (((uint64)pL2_entry->ipmcast_mc_ip.mac.octet[0] << 52) |
                            ((uint64)pL2_entry->ipmcast_mc_ip.mac.octet[1] << 44) |
                            ((uint64)pL2_entry->ipmcast_mc_ip.mac.octet[2] << 36) |
                            ((uint64)pL2_entry->ipmcast_mc_ip.mac.octet[3] << 28) |
                            ((uint64)pL2_entry->ipmcast_mc_ip.mac.octet[4] << 20) |
                            ((uint64)pL2_entry->ipmcast_mc_ip.mac.octet[5] << 12) |
                            ((uint64)pL2_entry->ipmcast_mc_ip.rvid));
            }
            else if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
            {
                hashSeed = (((uint64)pL2_entry->ipmcast_ip_mc_sip.dip << 32) |
                            ((uint64)pL2_entry->ipmcast_ip_mc_sip.sip));
            }
            else if (LOOKUP_ON_DIP_AND_FVID == ip6HashFmt[unit])
            {
                hashSeed = (((uint64)pL2_entry->ipmcast_ip_mc.dip << 12) |
                            ((uint64)pL2_entry->ipmcast_ip_mc.rvid));
            }
            break;
        default:
            return RT_ERR_FAILED;
    }

    /* TBD */
    if (0 == algoType[unit])
    {
        *pKey = (uint32) ((hashSeed >> 55) & BITMASK_9B)
                        ^ ((hashSeed >> 44) & BITMASK_11B)
                        ^ ((hashSeed >> 33) & BITMASK_11B)
                        ^ ((hashSeed >> 22) & BITMASK_11B)
                        ^ ((hashSeed >> 11) & BITMASK_11B)
                        ^ (hashSeed  & BITMASK_11B);
    }
    else if (1 == algoType[unit])
    {
        hash1 = (hashSeed>>11) & BITMASK_11B;
        hash1 = ((hash1 & BITMASK_5B) <<6) | ((hash1>>5) & BITMASK_6B);

        hash3 = (hashSeed>>33) & BITMASK_11B;
        hash3 = ((hash3 & BITMASK_6B) <<5) | ((hash3>>6) & BITMASK_5B);

        hash4 = (hashSeed>>44) & BITMASK_11B;
        hash4 = ((hash4 & BITMASK_7B) <<4) | ((hash4>>7) & BITMASK_4B);

        *pKey = (uint32) ((hashSeed >> 55) & BITMASK_9B)
                        ^ hash4
                        ^ hash3
                        ^ ((hashSeed >> 22) & BITMASK_11B)
                        ^ hash1
                        ^ (hashSeed  & BITMASK_11B);
    }
    else
    {
        return RT_ERR_FAILED;
    }


    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pKey=%d", *pKey);
    return RT_ERR_OK;
} /* end of _dal_maple_l2_entryToHashKey */

/* Function Name:
 *      _dal_maple_l2_compareEntry
 * Description:
 *      Compare L2 entry
 * Input:
 *      pSrcEntry - source entry
 *      pDstEntry - Destination entry
 * Output:
 *
 * Return:
 *      RT_ERR_OK     - key of two entry is same
 *      RT_ERR_FAILED - key of two entry is different
 * Note:
 *      None
 */
static int32
_dal_maple_l2_compareEntry(dal_maple_l2_entry_t *pSrcEntry, dal_maple_l2_entry_t *pDstEntry)
{
    uint32  unit = 0;

    RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "entry_type=%d", pSrcEntry->entry_type);

    if (pSrcEntry->entry_type != pDstEntry->entry_type)
        return RT_ERR_FAILED;

    switch (pSrcEntry->entry_type)
    {
        case L2_UNICAST:
            RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "srcfid=%d, dstfid=%d, srcMac=%x-%x-%x-%x-%x-%x,\
                   dstMac=%x-%x-%x-%x-%x-%x", pSrcEntry->unicast.fid, pDstEntry->unicast.fid,
                   pSrcEntry->unicast.mac.octet[0], pSrcEntry->unicast.mac.octet[1], pSrcEntry->unicast.mac.octet[2],
                   pSrcEntry->unicast.mac.octet[3], pSrcEntry->unicast.mac.octet[4], pSrcEntry->unicast.mac.octet[5],
                   pDstEntry->unicast.mac.octet[0], pDstEntry->unicast.mac.octet[1], pDstEntry->unicast.mac.octet[2],
                   pDstEntry->unicast.mac.octet[3], pDstEntry->unicast.mac.octet[4], pDstEntry->unicast.mac.octet[5]);

            if ((osal_memcmp(&pSrcEntry->unicast.mac, &pDstEntry->unicast.mac, sizeof(rtk_mac_t)))
               || (pSrcEntry->unicast.fid != pDstEntry->unicast.fid))
            {
                return RT_ERR_FAILED;
            }

            return RT_ERR_OK;

        case L2_MULTICAST:
            RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "srcrvid=%d, dstrvid=%d, srcMac=%x-%x-%x-%x-%x-%x,\
                   dstMac=%x-%x-%x-%x-%x-%x", pSrcEntry->l2mcast.rvid, pDstEntry->l2mcast.rvid,
                   pSrcEntry->l2mcast.mac.octet[0], pSrcEntry->l2mcast.mac.octet[1], pSrcEntry->l2mcast.mac.octet[2],
                   pSrcEntry->l2mcast.mac.octet[3], pSrcEntry->l2mcast.mac.octet[4], pSrcEntry->l2mcast.mac.octet[5],
                   pDstEntry->l2mcast.mac.octet[0], pDstEntry->l2mcast.mac.octet[1], pDstEntry->l2mcast.mac.octet[2],
                   pDstEntry->l2mcast.mac.octet[3], pDstEntry->l2mcast.mac.octet[4], pDstEntry->l2mcast.mac.octet[5]);

            if ((osal_memcmp(&pSrcEntry->l2mcast.mac, &pDstEntry->l2mcast.mac, sizeof(rtk_mac_t)))
               || (pSrcEntry->l2mcast.rvid != pDstEntry->l2mcast.rvid))
            {
                return RT_ERR_FAILED;
            }

            return RT_ERR_OK;

        case IP4_MULTICAST:
            if (LOOKUP_ON_FVID_AND_MAC == ip4HashFmt[unit])
            {
                RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "srcrvid=%d, dstrvid=%d, srcMac=%x-%x-%x-%x-%x-%x,\
                       dstMac=%x-%x-%x-%x-%x-%x", pSrcEntry->ipmcast_mc_ip.rvid, pDstEntry->ipmcast_mc_ip.rvid,
                       pSrcEntry->ipmcast_mc_ip.mac.octet[0], pSrcEntry->ipmcast_mc_ip.mac.octet[1], pSrcEntry->ipmcast_mc_ip.mac.octet[2],
                       pSrcEntry->ipmcast_mc_ip.mac.octet[3], pSrcEntry->ipmcast_mc_ip.mac.octet[4], pSrcEntry->ipmcast_mc_ip.mac.octet[5],
                       pDstEntry->ipmcast_mc_ip.mac.octet[0], pDstEntry->ipmcast_mc_ip.mac.octet[1], pDstEntry->ipmcast_mc_ip.mac.octet[2],
                       pDstEntry->ipmcast_mc_ip.mac.octet[3], pDstEntry->ipmcast_mc_ip.mac.octet[4], pDstEntry->ipmcast_mc_ip.mac.octet[5]);

                if ((osal_memcmp(&pSrcEntry->l2mcast.mac, &pDstEntry->l2mcast.mac, sizeof(rtk_mac_t)))
                   || (pSrcEntry->l2mcast.rvid != pDstEntry->l2mcast.rvid))
                {
                    return RT_ERR_FAILED;
                }
            }
            else if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
            {
                RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "srcdip=%d, dstdip=%d, srcsip=%d, dstsip=%d",
                       pSrcEntry->ipmcast_ip_mc_sip.dip, pDstEntry->ipmcast_ip_mc_sip.dip, pSrcEntry->ipmcast_ip_mc_sip.sip, pDstEntry->ipmcast_ip_mc_sip.sip);
                if (ipmcst_fvid_cmp && pSrcEntry->ipmcast_ip_mc_sip.rvid != pDstEntry->ipmcast_ip_mc_sip.rvid)
                    return RT_ERR_FAILED;
                if ((pSrcEntry->ipmcast_ip_mc_sip.dip != pDstEntry->ipmcast_ip_mc_sip.dip)
                    ||(pSrcEntry->ipmcast_ip_mc_sip.sip != pDstEntry->ipmcast_ip_mc_sip.sip))
                {
                    return RT_ERR_FAILED;
                }
            }
            else if (LOOKUP_ON_DIP_AND_FVID == ip4HashFmt[unit])
            {
                RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "srcrvid=%d, dstrvid=%d, srcdip=%d, dstdip=%d",
                        pSrcEntry->ipmcast_ip_mc.rvid, pDstEntry->ipmcast_ip_mc.rvid,
                        pSrcEntry->ipmcast_ip_mc.dip, pDstEntry->ipmcast_ip_mc.dip);
                if ((pSrcEntry->ipmcast_ip_mc.dip != pDstEntry->ipmcast_ip_mc.dip)
                    ||(pSrcEntry->ipmcast_ip_mc.rvid != pDstEntry->ipmcast_ip_mc.rvid))
                {
                    return RT_ERR_FAILED;
                }
            }

            return RT_ERR_OK;

        case IP6_MULTICAST:
            if (LOOKUP_ON_FVID_AND_MAC == ip6HashFmt[unit])
            {
                RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "srcrvid=%d, dstrvid=%d, srcMac=%x-%x-%x-%x-%x-%x,\
                       dstMac=%x-%x-%x-%x-%x-%x", pSrcEntry->ipmcast_mc_ip.rvid, pDstEntry->ipmcast_mc_ip.rvid,
                       pSrcEntry->ipmcast_mc_ip.mac.octet[0], pSrcEntry->ipmcast_mc_ip.mac.octet[1], pSrcEntry->ipmcast_mc_ip.mac.octet[2],
                       pSrcEntry->ipmcast_mc_ip.mac.octet[3], pSrcEntry->ipmcast_mc_ip.mac.octet[4], pSrcEntry->ipmcast_mc_ip.mac.octet[5],
                       pDstEntry->ipmcast_mc_ip.mac.octet[0], pDstEntry->ipmcast_mc_ip.mac.octet[1], pDstEntry->ipmcast_mc_ip.mac.octet[2],
                       pDstEntry->ipmcast_mc_ip.mac.octet[3], pDstEntry->ipmcast_mc_ip.mac.octet[4], pDstEntry->ipmcast_mc_ip.mac.octet[5]);

                if ((osal_memcmp(&pSrcEntry->l2mcast.mac, &pDstEntry->l2mcast.mac, sizeof(rtk_mac_t)))
                   || (pSrcEntry->l2mcast.rvid != pDstEntry->l2mcast.rvid))
                {
                    return RT_ERR_FAILED;
                }
            }
            else if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
            {
                RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "srcdip=%d, dstdip=%d, srcsip=%d, dstsip=%d",
                       pSrcEntry->ipmcast_ip_mc_sip.dip, pDstEntry->ipmcast_ip_mc_sip.dip, pSrcEntry->ipmcast_ip_mc_sip.sip, pDstEntry->ipmcast_ip_mc_sip.sip);
                if (ipmcst_fvid_cmp && pSrcEntry->ipmcast_ip_mc_sip.rvid != pDstEntry->ipmcast_ip_mc_sip.rvid)
                    return RT_ERR_FAILED;
                if ((pSrcEntry->ipmcast_ip_mc_sip.dip != pDstEntry->ipmcast_ip_mc_sip.dip)
                    ||(pSrcEntry->ipmcast_ip_mc_sip.sip != pDstEntry->ipmcast_ip_mc_sip.sip))
                {
                    return RT_ERR_FAILED;
                }
            }
            else if (LOOKUP_ON_DIP_AND_FVID == ip6HashFmt[unit])
            {
                RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "srcrvid=%d, dstrvid=%d, srcdip=%d, dstdip=%d",
                        pSrcEntry->ipmcast_ip_mc.rvid, pDstEntry->ipmcast_ip_mc.rvid,
                        pSrcEntry->ipmcast_ip_mc.dip, pDstEntry->ipmcast_ip_mc.dip);
                if ((pSrcEntry->ipmcast_ip_mc.dip != pDstEntry->ipmcast_ip_mc.dip)
                    ||(pSrcEntry->ipmcast_ip_mc.rvid != pDstEntry->ipmcast_ip_mc.rvid))
                {
                    return RT_ERR_FAILED;
                }
            }

            return RT_ERR_OK;

        default:
            return RT_ERR_FAILED;
    }
} /* end of _dal_maple_l2_compareEntry*/

/* Function Name:
 *      _dal_maple_l2_nextValidAddr_get
 * Description:
 *      Get next valid L2 unicast, multicast or ip multicast address entry from the specified device.
 * Input:
 *      unit           - unit id
 *      pScan_idx      - currently scan index of l2 table to get next.
 *      type           - address type
 *      include_static - the get type, include static mac or not.
 * Output:
 *      pL2_data       - structure of l2 address data
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_VLAN_VID          - invalid vid
 *      RT_ERR_MAC               - invalid mac address
 *      RT_ERR_NULL_POINTER      - input parameter may be null pointer
 *      RT_ERR_L2_ENTRY_NOTFOUND - specified entry not found
 *      RT_ERR_OUT_OF_RANGE      - input parameter out of range
 * Note:
 *      1) The function will skip valid l2 multicast and ip multicast entry and
 *         reply next valid L2 unicast address is based on index order of l2 table.
 *      2) Please input -1 for get the first entry of l2 table.
 *      3) The *pScan_idx is the input and also is the output argument.
 *      4) Valid type value is L2_UNICAST, L2_MULTICAST and IP_MULTICAST
 */
static int32
_dal_maple_l2_nextValidAddr_get(
    uint32              unit,
    int32               *pScan_idx,
    uint32              type,
    uint32              include_static,
    dal_maple_l2_entry_t  *pL2_data)
{
    int32   ret;
    dal_maple_l2_entry_t  l2_entry;
    uint32  l2_idx;
    uint32  l2cam_idx;
    uint32  hashkey, location;
    uint32  isValid;
    uint32  found;
    uint32  value;
    rtk_enable_t    l2CamEbl;

    RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "unit=%d, pScan_idx=%d, type=%d, include_static=%d",
           unit, *pScan_idx, type, include_static);


    /* parameter check */
    RT_PARAM_CHK(*pScan_idx >= (int32)(hashTable_size[unit] + cam_size[unit]), RT_ERR_OUT_OF_RANGE);

    l2cam_idx = 0;
    if (*pScan_idx < 0)
    {
        l2_idx = 0;
    }
    else
    {
        l2_idx = *pScan_idx + 1;
    }

    found = FALSE;
    for (; l2_idx < hashTable_size[unit]; l2_idx++)
    {
        hashkey = l2_idx >> 2;
        location = l2_idx & BITMASK_2B;

        l2_entry.entry_type = 0;

        if ((_dal_maple_l2_getL2EntryfromHash(unit, hashkey, location, &l2_entry, &isValid) == RT_ERR_OK)
            && (TRUE == isValid) && (type == l2_entry.entry_type))
        {
            if (L2_UNICAST == l2_entry.entry_type)
            {
                if ((FALSE == include_static) && (1 == l2_entry.unicast.is_static))
                {
                    /* if not need to include static and this entry is static, just skip this entry */
                    continue;
                }
            }

            //osal_printf("get entry index = %d, depth=%d\n", hashkey, location);

            found = TRUE;
            *pScan_idx = l2_idx;
            break;
        }
    }

    if (FALSE == found)
    {
        if ((ret = reg_field_read(unit, MAPLE_L2_CTRL_0r, MAPLE_LUTCAM_ENf, &value)) != RT_ERR_OK)
            return ret;
        l2CamEbl = value == 1 ? ENABLED : DISABLED;
        for (l2cam_idx = l2_idx - hashTable_size[unit]; l2CamEbl && (l2cam_idx < cam_size[unit]); l2cam_idx++)
        {
            if ((_dal_maple_l2_getL2EntryfromCAM(unit, l2cam_idx, &l2_entry, &isValid) == RT_ERR_OK)
                && (TRUE == isValid) && (type == l2_entry.entry_type))
            {
                if (L2_UNICAST == l2_entry.entry_type)
                {
                    if ((FALSE == include_static) && (1 == l2_entry.unicast.is_static))
                    {
                        /* if not need to include static and this entry is static, just skip this entry */
                        continue;
                    }
                    if (1 == l2_entry.unicast.nh)
                    {
                        /* nh entry is not included in this API */
                        continue;
                    }
                }

                found = TRUE;
                *pScan_idx = l2cam_idx + hashTable_size[unit];
                break;
            }
        }
    }

    if (FALSE == found)
    {
        return RT_ERR_ENTRY_NOTFOUND;
    }

    l2_entry.is_entry_exist = 1;

    osal_memcpy(pL2_data, &l2_entry, sizeof(dal_maple_l2_entry_t));
    return RT_ERR_OK;
} /* end of _dal_maple_l2_nextValidAddr_get */

/* Function Name:
 *      _dal_maple_l2_setL2HASHEntry
 * Description:
 *      Get exist entry or free entry
 * Input:
 *      unit       - unit id
 *      pL2_entry  - L2 entry used to do search
 *      pL2_index  - index of found entry(free or exist)
 *                      - index_type = IN_CAM mean this index is in CAM
 *                      - index_type = IN_HASH mean this index is in HASH
 *      get_method - use which method to get
 *                      - L2_GET_EXIST_ONLY
 *                      - L2_GET_EXIST_OR_FREE
 *                      - L2_GET_FREE_ONLY
 * Output:
 *      pL2_entry  - L2 entry
 *      pL2_index  - the index of L2 entry
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_VLAN_VID - invalid vid
 * Note:
 *      None
 */
static int32
_dal_maple_l2_setL2HASHEntry(uint32 unit, dal_maple_l2_entry_t *pL2_entry, dal_maple_l2_index_t *pL2_index)
{
    int32       ret;
    l2_entry_t  l2_entry;
    uint32      mac_uint32[2];
    uint32      l2_index;
    uint32      ip_multi, ip6_multi;

    RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "unit=%d, entry_type=%d, hashdepth=%d, index=%d",
           unit, pL2_entry->entry_type, pL2_index->hashdepth, pL2_index->index);
/*
    osal_printf( "unit=%d, entry_type=%d, hashdepth=%d, index=%d",
           unit, pL2_entry->entry_type, pL2_index->hashdepth, pL2_index->index);
*/
    osal_memset(&l2_entry, 0, sizeof(l2_entry));

    /* Extract content of each kind of entry from l2_enry */
    switch (pL2_entry->entry_type)
    {
        case L2_UNICAST: /* L2 unicast */
            RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "da_block=%d, sa_block=%d, port=%d, is_static=%d, aging=%d, suspend=%d, nh=%d, Mac=%x-%x-%x-%x-%x-%x",
                   pL2_entry->unicast.dablock, pL2_entry->unicast.sablock, pL2_entry->unicast.port,
                   pL2_entry->unicast.is_static, pL2_entry->unicast.aging, pL2_entry->unicast.suspending, pL2_entry->unicast.nh, pL2_entry->unicast.mac.octet[0],
                   pL2_entry->unicast.mac.octet[1], pL2_entry->unicast.mac.octet[2], pL2_entry->unicast.mac.octet[3],
                   pL2_entry->unicast.mac.octet[4], pL2_entry->unicast.mac.octet[5]);

            {
                ip_multi = 0;
                ip6_multi = 0;
                if ((ret = table_field_set(unit, MAPLE_L2_UCt, MAPLE_L2_UC_IP_MCtf, &ip_multi, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_set(unit, MAPLE_L2_UCt, MAPLE_L2_UC_IP6_MCtf, &ip6_multi, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_set(unit, MAPLE_L2_UCt, MAPLE_L2_UC_DA_BLKtf, &pL2_entry->unicast.dablock, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_set(unit, MAPLE_L2_UCt, MAPLE_L2_UC_SA_BLKtf, &pL2_entry->unicast.sablock, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_set(unit, MAPLE_L2_UCt, MAPLE_L2_UC_SLPtf, &pL2_entry->unicast.port, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_set(unit, MAPLE_L2_UCt, MAPLE_L2_UC_STATICtf, &pL2_entry->unicast.is_static, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_set(unit, MAPLE_L2_UCt, MAPLE_L2_UC_AGEtf, &pL2_entry->unicast.aging, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_set(unit, MAPLE_L2_UCt, MAPLE_L2_UC_SUSPENDtf, &pL2_entry->unicast.suspending, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_set(unit, MAPLE_L2_UCt, MAPLE_L2_UC_NEXT_HOPtf, &pL2_entry->unicast.nh, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_set(unit, MAPLE_L2_UCt, MAPLE_L2_UC_FID_RVIDtf, &pL2_entry->unicast.fid, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if (pL2_entry->unicast.nh == TRUE)
                {
                    if ((ret = table_field_set(unit, MAPLE_L2_NEXT_HOPt, MAPLE_L2_NEXT_HOP_VLAN_TARGETtf, &pL2_entry->unicast.vlan_target, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_NEXT_HOPt, MAPLE_L2_NEXT_HOP_ROUTE_IDXtf, &pL2_entry->unicast.route_idx, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                }
                else
                {
                    if ((ret = table_field_set(unit, MAPLE_L2_UCt, MAPLE_L2_UC_VIDtf, &pL2_entry->unicast.agg_vid, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                }

                mac_uint32[1] = ((uint32)pL2_entry->unicast.mac.octet[0]) << 8;
                mac_uint32[1] |= ((uint32)pL2_entry->unicast.mac.octet[1]);
                mac_uint32[0] = ((uint32)pL2_entry->unicast.mac.octet[2]) << 24;
                mac_uint32[0] |= ((uint32)pL2_entry->unicast.mac.octet[3]) << 16;
                mac_uint32[0] |= ((uint32)pL2_entry->unicast.mac.octet[4]) << 8;
                mac_uint32[0] |= ((uint32)pL2_entry->unicast.mac.octet[5]);

                if ((ret = table_field_set(unit, MAPLE_L2_UCt, MAPLE_L2_UC_MACtf, &mac_uint32[0], (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
            }
            break;
        case L2_MULTICAST: /* l2 Multicast */
            RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "index=%d, Mac=%x-%x-%x-%x-%x-%x",
                   pL2_entry->l2mcast.index, pL2_entry->l2mcast.mac.octet[0],
                   pL2_entry->l2mcast.mac.octet[1], pL2_entry->l2mcast.mac.octet[2], pL2_entry->l2mcast.mac.octet[3],
                   pL2_entry->l2mcast.mac.octet[4], pL2_entry->l2mcast.mac.octet[5]);

            ip_multi = 0;
            ip6_multi = 0;
            if ((ret = table_field_set(unit, MAPLE_L2_MCt, MAPLE_L2_MC_IP_MCtf, &ip_multi, (uint32 *)&l2_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            if ((ret = table_field_set(unit, MAPLE_L2_MCt, MAPLE_L2_MC_IP6_MCtf, &ip6_multi, (uint32 *)&l2_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            if ((ret = table_field_set(unit, MAPLE_L2_MCt, MAPLE_L2_MC_MC_PMSK_IDXtf, &pL2_entry->l2mcast.index, (uint32 *)&l2_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            if ((ret = table_field_set(unit, MAPLE_L2_MCt, MAPLE_L2_MC_VIDtf, &pL2_entry->l2mcast.agg_vid, (uint32 *)&l2_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            if ((ret = table_field_set(unit, MAPLE_L2_MCt, MAPLE_L2_MC_FID_RVIDtf , &pL2_entry->l2mcast.rvid, (uint32 *)&l2_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            mac_uint32[1] = ((uint32)pL2_entry->l2mcast.mac.octet[0]) << 8;
            mac_uint32[1] |= ((uint32)pL2_entry->l2mcast.mac.octet[1]);
            mac_uint32[0] = ((uint32)pL2_entry->l2mcast.mac.octet[2]) << 24;
            mac_uint32[0] |= ((uint32)pL2_entry->l2mcast.mac.octet[3]) << 16;
            mac_uint32[0] |= ((uint32)pL2_entry->l2mcast.mac.octet[4]) << 8;
            mac_uint32[0] |= ((uint32)pL2_entry->l2mcast.mac.octet[5]);

            if ((ret = table_field_set(unit, MAPLE_L2_MCt, MAPLE_L2_MC_MACtf, &mac_uint32[0], (uint32 *)&l2_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }
            break;
        case IP4_MULTICAST: /* IP multicast */
            if (LOOKUP_ON_FVID_AND_MAC == ip4HashFmt[unit])
            {
                RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "index=%d, Mac=%x-%x-%x-%x-%x-%x",
                   pL2_entry->ipmcast_mc_ip.index, pL2_entry->ipmcast_mc_ip.mac.octet[0],
                   pL2_entry->ipmcast_mc_ip.mac.octet[1], pL2_entry->ipmcast_mc_ip.mac.octet[2], pL2_entry->ipmcast_mc_ip.mac.octet[3],
                   pL2_entry->ipmcast_mc_ip.mac.octet[4], pL2_entry->ipmcast_mc_ip.mac.octet[5]);

                ip_multi = 0;
                ip6_multi = 0;
                if ((ret = table_field_set(unit, MAPLE_L2_MCt, MAPLE_L2_MC_IP_MCtf, &ip_multi, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_set(unit, MAPLE_L2_MCt, MAPLE_L2_MC_IP6_MCtf, &ip6_multi, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_set(unit, MAPLE_L2_MCt, MAPLE_L2_MC_MC_PMSK_IDXtf, &pL2_entry->ipmcast_mc_ip.index, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_set(unit, MAPLE_L2_MCt, MAPLE_L2_MC_VIDtf, &pL2_entry->ipmcast_mc_ip.agg_vid, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_set(unit, MAPLE_L2_MCt, MAPLE_L2_MC_FID_RVIDtf , &pL2_entry->ipmcast_mc_ip.rvid, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                mac_uint32[1] = ((uint32)pL2_entry->ipmcast_mc_ip.mac.octet[0]) << 8;
                mac_uint32[1] |= ((uint32)pL2_entry->ipmcast_mc_ip.mac.octet[1]);
                mac_uint32[0] = ((uint32)pL2_entry->ipmcast_mc_ip.mac.octet[2]) << 24;
                mac_uint32[0] |= ((uint32)pL2_entry->ipmcast_mc_ip.mac.octet[3]) << 16;
                mac_uint32[0] |= ((uint32)pL2_entry->ipmcast_mc_ip.mac.octet[4]) << 8;
                mac_uint32[0] |= ((uint32)pL2_entry->ipmcast_mc_ip.mac.octet[5]);

                if ((ret = table_field_set(unit, MAPLE_L2_MCt, MAPLE_L2_MC_MACtf, &mac_uint32[0], (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
            }
            else if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
            {
                RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "index=%d, rvid=%d, dip=%d, sip=%d",
                        pL2_entry->ipmcast_ip_mc_sip.index, pL2_entry->ipmcast_ip_mc_sip.rvid, pL2_entry->ipmcast_ip_mc_sip.dip, pL2_entry->ipmcast_ip_mc_sip.sip);

                if (pL2_entry->ipmcast_ip_mc_sip.dip == 0)
                {   /* if dip is zero, mean delete entry. So set ip_multi = 0 */
                    ip_multi = 0;
                    ip6_multi = 0;
                }
                else
                {
                    ip_multi = 1;
                    ip6_multi = 0;

                    if ((ret = table_field_set(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_IP_MCtf, &ip_multi, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_IP6_MCtf, &ip6_multi, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_GIPtf, &pL2_entry->ipmcast_ip_mc_sip.dip, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }

                    if ((ret = table_field_set(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_SIPtf, &pL2_entry->ipmcast_ip_mc_sip.sip, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_FID_RVIDtf, &pL2_entry->ipmcast_ip_mc_sip.rvid, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_MC_PMSK_IDXtf, &pL2_entry->ipmcast_ip_mc_sip.index, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                }
            }
            else if (LOOKUP_ON_DIP_AND_FVID == ip4HashFmt[unit])
            {
                RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "index=%d, dip=%d", pL2_entry->ipmcast_ip_mc.index, pL2_entry->ipmcast_ip_mc.dip);

                if (pL2_entry->ipmcast_ip_mc.dip == 0)
                {   /* if dip is zero, mean delete entry. So set ip_multi = 0 */
                    ip_multi = 0;
                    ip6_multi = 0;
                }
                else
                {
                    ip_multi = 1;
                    ip6_multi = 0;

                    if ((ret = table_field_set(unit, MAPLE_L2_IP_MCt, MAPLE_L2_IP_MC_IP_MCtf, &ip_multi, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_IP_MCt, MAPLE_L2_IP_MC_IP6_MCtf, &ip6_multi, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_IP_MCt, MAPLE_L2_IP_MC_GIPtf, &pL2_entry->ipmcast_ip_mc.dip, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_IP_MCt, MAPLE_L2_IP_MC_MC_PMSK_IDXtf, &pL2_entry->ipmcast_ip_mc.index, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }

                    if ((ret = table_field_set(unit, MAPLE_L2_IP_MCt, MAPLE_L2_IP_MC_FID_RVIDtf , &pL2_entry->ipmcast_ip_mc.rvid, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                }
            }
            break;
        case IP6_MULTICAST: /* IP multicast */
            if (LOOKUP_ON_FVID_AND_MAC == ip6HashFmt[unit])
            {
                RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "index=%d, Mac=%x-%x-%x-%x-%x-%x",
                   pL2_entry->ipmcast_mc_ip.index, pL2_entry->ipmcast_mc_ip.mac.octet[0],
                   pL2_entry->ipmcast_mc_ip.mac.octet[1], pL2_entry->ipmcast_mc_ip.mac.octet[2], pL2_entry->ipmcast_mc_ip.mac.octet[3],
                   pL2_entry->ipmcast_mc_ip.mac.octet[4], pL2_entry->ipmcast_mc_ip.mac.octet[5]);

                ip_multi = 0;
                ip6_multi = 0;
                if ((ret = table_field_set(unit, MAPLE_L2_MCt, MAPLE_L2_MC_IP_MCtf, &ip_multi, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                if ((ret = table_field_set(unit, MAPLE_L2_MCt, MAPLE_L2_MC_IP6_MCtf, &ip6_multi, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_set(unit, MAPLE_L2_MCt, MAPLE_L2_MC_MC_PMSK_IDXtf, &pL2_entry->ipmcast_mc_ip.index, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_set(unit, MAPLE_L2_MCt, MAPLE_L2_MC_VIDtf, &pL2_entry->ipmcast_mc_ip.agg_vid, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_set(unit, MAPLE_L2_MCt, MAPLE_L2_MC_FID_RVIDtf , &pL2_entry->ipmcast_mc_ip.rvid, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                mac_uint32[1] = ((uint32)pL2_entry->ipmcast_mc_ip.mac.octet[0]) << 8;
                mac_uint32[1] |= ((uint32)pL2_entry->ipmcast_mc_ip.mac.octet[1]);
                mac_uint32[0] = ((uint32)pL2_entry->ipmcast_mc_ip.mac.octet[2]) << 24;
                mac_uint32[0] |= ((uint32)pL2_entry->ipmcast_mc_ip.mac.octet[3]) << 16;
                mac_uint32[0] |= ((uint32)pL2_entry->ipmcast_mc_ip.mac.octet[4]) << 8;
                mac_uint32[0] |= ((uint32)pL2_entry->ipmcast_mc_ip.mac.octet[5]);

                if ((ret = table_field_set(unit, MAPLE_L2_MCt, MAPLE_L2_MC_MACtf, &mac_uint32[0], (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
            }
            else if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
            {
                RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "index=%d, rvid=%d, dip=%d, sip=%d",
                        pL2_entry->ipmcast_ip_mc_sip.index, pL2_entry->ipmcast_ip_mc_sip.rvid, pL2_entry->ipmcast_ip_mc_sip.dip, pL2_entry->ipmcast_ip_mc_sip.sip);

                if (pL2_entry->ipmcast_ip_mc_sip.dip == 0)
                {   /* if dip is zero, mean delete entry. So set ip_multi = 0 */
                    ip_multi = 0;
                    ip6_multi = 0;
                }
                else
                {
                    ip_multi = 1;
                    ip6_multi = 1;

                    if ((ret = table_field_set(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_IP_MCtf, &ip_multi, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_IP6_MCtf, &ip6_multi, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_GIPtf, &pL2_entry->ipmcast_ip_mc_sip.dip, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }

                    if ((ret = table_field_set(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_SIPtf, &pL2_entry->ipmcast_ip_mc_sip.sip, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_FID_RVIDtf, &pL2_entry->ipmcast_ip_mc_sip.rvid, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_MC_PMSK_IDXtf, &pL2_entry->ipmcast_ip_mc_sip.index, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                }
            }
            else if (LOOKUP_ON_DIP_AND_FVID == ip6HashFmt[unit])
            {
                RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "index=%d, dip=%d", pL2_entry->ipmcast_ip_mc.index, pL2_entry->ipmcast_ip_mc.dip);

                if (pL2_entry->ipmcast_ip_mc.dip == 0)
                {   /* if dip is zero, mean delete entry. So set ip_multi = 0 */
                    ip_multi = 0;
                    ip6_multi = 0;
                }
                else
                {
                    ip_multi = 1;
                    ip6_multi = 1;

                    if ((ret = table_field_set(unit, MAPLE_L2_IP_MCt, MAPLE_L2_IP_MC_IP_MCtf, &ip_multi, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_IP_MCt, MAPLE_L2_IP_MC_IP6_MCtf, &ip6_multi, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_IP_MCt, MAPLE_L2_IP_MC_GIPtf, &pL2_entry->ipmcast_ip_mc.dip, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_IP_MCt, MAPLE_L2_IP_MC_MC_PMSK_IDXtf, &pL2_entry->ipmcast_ip_mc.index, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }

                    if ((ret = table_field_set(unit, MAPLE_L2_IP_MCt, MAPLE_L2_IP_MC_FID_RVIDtf , &pL2_entry->ipmcast_ip_mc.rvid, (uint32 *)&l2_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                }
            }
            break;
        default:
            return RT_ERR_FAILED;
     }

    l2_index = (SRAM << ACCADDR_L2TYPE_OFFSET) | (pL2_index->index << 2) | pL2_index->hashdepth;

    /* read entry from chip */
    if ((ret = table_write(unit, MAPLE_L2_UCt, l2_index, (uint32 *)&l2_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of _dal_maple_l2_setL2HASHEntry */

/* Function Name:
 *      _dal_maple_l2_setL2CAMEntry
 * Description:
 *      Get exist entry or free entry
 * Input:
 *      unit       - unit id
 *      pL2_entry  - L2 entry used to do search
 *      pL2_index  - index of found entry(free or exist)
 *                      - index_type = IN_CAM mean this index is in CAM
 *                      - index_type = IN_HASH mean this index is in HASH
 *      get_method - use which method to get
 *                      - L2_GET_EXIST_ONLY
 *                      - L2_GET_EXIST_OR_FREE
 *                      - L2_GET_FREE_ONLY
 * Output:
 *      pL2_index  - the index of L2 entry
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_VLAN_VID - invalid vid
 * Note:
 *      None
 */
static int32
_dal_maple_l2_setL2CAMEntry(uint32 unit, dal_maple_l2_entry_t *pL2_entry, dal_maple_l2_index_t *pL2_index)
{
    int32   ret;
    l2cam_entry_t l2cam_entry;
    uint32  mac_uint32[2];
    uint32  l2_index;
    uint32  ip_multi = 0, ip6_multi = 0;

    RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "unit=%d, entry_type=%d, hashdepth=%d, index=%d",
           unit, pL2_entry->entry_type, pL2_index->hashdepth, pL2_index->index);

    l2_index = pL2_index->index ;

    osal_memset(&l2cam_entry, 0, sizeof(l2cam_entry));

    /* Extract content of each kind of entry from l2_enry */
    switch (pL2_entry->entry_type)
    {
        case L2_UNICAST: /* L2 unicast */
            RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "da_block=%d, sa_block=%d, port=%d, fid=%d, is_static=%d, aging=%d, suspend=%d, nh=%d, \
                   Mac=%x-%x-%x-%x-%x-%x",
                   pL2_entry->unicast.dablock, pL2_entry->unicast.sablock, pL2_entry->unicast.port, pL2_entry->unicast.fid,
                   pL2_entry->unicast.is_static, pL2_entry->unicast.aging, pL2_entry->unicast.suspending, pL2_entry->unicast.nh, pL2_entry->unicast.mac.octet[0],
                   pL2_entry->unicast.mac.octet[1], pL2_entry->unicast.mac.octet[2], pL2_entry->unicast.mac.octet[3],
                   pL2_entry->unicast.mac.octet[4], pL2_entry->unicast.mac.octet[5]);
            ip_multi = 0;
            ip6_multi = 0;
            if ((ret = table_field_set(unit, MAPLE_L2_CAM_UCt, MAPLE_L2_CAM_UC_IP_MCtf, &ip_multi, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }
            if ((ret = table_field_set(unit, MAPLE_L2_CAM_UCt, MAPLE_L2_CAM_UC_IP6_MCtf, &ip6_multi, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            if ((ret = table_field_set(unit, MAPLE_L2_CAM_UCt, MAPLE_L2_CAM_UC_DA_BLKtf, &pL2_entry->unicast.dablock, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            if ((ret = table_field_set(unit, MAPLE_L2_CAM_UCt, MAPLE_L2_CAM_UC_SA_BLKtf, &pL2_entry->unicast.sablock, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            if ((ret = table_field_set(unit, MAPLE_L2_CAM_UCt, MAPLE_L2_CAM_UC_SLPtf, &pL2_entry->unicast.port, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            if ((ret = table_field_set(unit, MAPLE_L2_CAM_UCt, MAPLE_L2_CAM_UC_FID_RVIDtf, &pL2_entry->unicast.fid, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            if ((ret = table_field_set(unit, MAPLE_L2_CAM_UCt, MAPLE_L2_CAM_UC_STATICtf, &pL2_entry->unicast.is_static, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            if ((ret = table_field_set(unit, MAPLE_L2_CAM_UCt, MAPLE_L2_CAM_UC_AGEtf, &pL2_entry->unicast.aging, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            if ((ret = table_field_set(unit, MAPLE_L2_CAM_UCt, MAPLE_L2_CAM_UC_SUSPENDtf, &pL2_entry->unicast.suspending, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            if ((ret = table_field_set(unit, MAPLE_L2_CAM_UCt, MAPLE_L2_CAM_UC_NEXT_HOPtf, &pL2_entry->unicast.nh, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            if ((ret = table_field_set(unit, MAPLE_L2_CAM_UCt, MAPLE_L2_CAM_UC_VIDtf, &pL2_entry->unicast.agg_vid, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            mac_uint32[1] = ((uint32)pL2_entry->unicast.mac.octet[0]) << 8;
            mac_uint32[1] |= ((uint32)pL2_entry->unicast.mac.octet[1]);
            mac_uint32[0] = ((uint32)pL2_entry->unicast.mac.octet[2]) << 24;
            mac_uint32[0] |= ((uint32)pL2_entry->unicast.mac.octet[3]) << 16;
            mac_uint32[0] |= ((uint32)pL2_entry->unicast.mac.octet[4]) << 8;
            mac_uint32[0] |= ((uint32)pL2_entry->unicast.mac.octet[5]);

            if ((ret = table_field_set(unit, MAPLE_L2_CAM_UCt, MAPLE_L2_CAM_UC_MACtf, &mac_uint32[0], (uint32 *)&l2cam_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }
            break;
        case L2_MULTICAST: /* l2 Multicast */
            RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "index=%d, rvid=%d, Mac=%x-%x-%x-%x-%x-%x",
                   pL2_entry->l2mcast.index, pL2_entry->l2mcast.rvid, pL2_entry->l2mcast.mac.octet[0],
                   pL2_entry->l2mcast.mac.octet[1], pL2_entry->l2mcast.mac.octet[2], pL2_entry->l2mcast.mac.octet[3],
                   pL2_entry->l2mcast.mac.octet[4], pL2_entry->l2mcast.mac.octet[5]);
            ip_multi = 0;
            ip6_multi = 0;
            if ((ret = table_field_set(unit, MAPLE_L2_CAM_MCt, MAPLE_L2_CAM_MC_IP_MCtf, &ip_multi, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }
            if ((ret = table_field_set(unit, MAPLE_L2_CAM_MCt, MAPLE_L2_CAM_MC_IP6_MCtf, &ip6_multi, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            if ((ret = table_field_set(unit, MAPLE_L2_CAM_MCt, MAPLE_L2_CAM_MC_MC_PMSK_IDXtf, &pL2_entry->l2mcast.index, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            if ((ret = table_field_set(unit, MAPLE_L2_CAM_MCt, MAPLE_L2_CAM_MC_FID_RVIDtf, &pL2_entry->l2mcast.rvid, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            if ((ret = table_field_set(unit, MAPLE_L2_CAM_MCt, MAPLE_L2_CAM_MC_VIDtf, &pL2_entry->l2mcast.agg_vid, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            mac_uint32[1] = ((uint32)pL2_entry->l2mcast.mac.octet[0]) << 8;
            mac_uint32[1] |= ((uint32)pL2_entry->l2mcast.mac.octet[1]);
            mac_uint32[0] = ((uint32)pL2_entry->l2mcast.mac.octet[2]) << 24;
            mac_uint32[0] |= ((uint32)pL2_entry->l2mcast.mac.octet[3]) << 16;
            mac_uint32[0] |= ((uint32)pL2_entry->l2mcast.mac.octet[4]) << 8;
            mac_uint32[0] |= ((uint32)pL2_entry->l2mcast.mac.octet[5]);

            if ((ret = table_field_set(unit, MAPLE_L2_CAM_MCt, MAPLE_L2_CAM_MC_MACtf, &mac_uint32[0], (uint32 *)&l2cam_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }
            break;
        case IP4_MULTICAST: /* IP multicast */
            if (LOOKUP_ON_FVID_AND_MAC == ip4HashFmt[unit])
            {
                return RT_ERR_CHIP_NOT_SUPPORTED;
            }
            else if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
            {
                RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "index=%d, rvid=%d, dip=%d, sip=%d",
                        pL2_entry->ipmcast_ip_mc_sip.index, pL2_entry->ipmcast_ip_mc_sip.rvid, pL2_entry->ipmcast_ip_mc_sip.dip, pL2_entry->ipmcast_ip_mc_sip.sip);

                if (pL2_entry->ipmcast_ip_mc_sip.dip == 0)
                {   /* if dip is zero, mean delete entry. So set ip_multi = 0 */
                    ip_multi = 0;
                    ip6_multi = 0;
                }
                else
                {
                    ip_multi = 1;
                    ip6_multi = 0;

                    if ((ret = table_field_set(unit, MAPLE_L2_CAM_IP_MC_SIPt, MAPLE_L2_CAM_IP_MC_SIP_IP_MCtf, &ip_multi, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_CAM_IP_MC_SIPt, MAPLE_L2_CAM_IP_MC_SIP_IP6_MCtf, &ip6_multi, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_CAM_IP_MC_SIPt, MAPLE_L2_CAM_IP_MC_SIP_GIPtf, &pL2_entry->ipmcast_ip_mc_sip.dip, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_CAM_IP_MC_SIPt, MAPLE_L2_CAM_IP_MC_SIP_SIPtf, &pL2_entry->ipmcast_ip_mc_sip.sip, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_CAM_IP_MC_SIPt, MAPLE_L2_CAM_IP_MC_SIP_FID_RVIDtf, &pL2_entry->ipmcast_ip_mc_sip.rvid, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_CAM_IP_MC_SIPt, MAPLE_L2_CAM_IP_MC_SIP_MC_PMSK_IDXtf, &pL2_entry->ipmcast_ip_mc_sip.index, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                }
            }
            else if (LOOKUP_ON_DIP_AND_FVID == ip4HashFmt[unit])
            {
                RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "index=%d, rvid=%d, dip=%d", pL2_entry->ipmcast_ip_mc.index, pL2_entry->ipmcast_ip_mc.rvid, pL2_entry->ipmcast_ip_mc.dip);

                if (pL2_entry->ipmcast_ip_mc_sip.dip == 0)
                {   /* if dip is zero, mean delete entry. So set ip_multi = 0 */
                    ip_multi = 0;
                    ip6_multi = 0;
                }
                else
                {
                    ip_multi = 1;
                    ip6_multi = 0;

                    if ((ret = table_field_set(unit, MAPLE_L2_CAM_IP_MCt, MAPLE_L2_CAM_IP_MC_IP_MCtf, &ip_multi, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_CAM_IP_MCt, MAPLE_L2_CAM_IP_MC_IP6_MCtf, &ip6_multi, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_CAM_IP_MCt, MAPLE_L2_CAM_IP_MC_GIPtf, &pL2_entry->ipmcast_ip_mc.dip, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_CAM_IP_MCt, MAPLE_L2_CAM_IP_MC_MC_PMSK_IDXtf, &pL2_entry->ipmcast_ip_mc.index, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_CAM_IP_MCt, MAPLE_L2_CAM_IP_MC_FID_RVIDtf, &pL2_entry->ipmcast_ip_mc.rvid, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                }
            }
            break;
        case IP6_MULTICAST: /* IPv6 multicast */
            if (LOOKUP_ON_FVID_AND_MAC == ip6HashFmt[unit])
            {
                return RT_ERR_CHIP_NOT_SUPPORTED;
            }
            else if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
            {
                RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "index=%d, rvid=%d, dip=%d, sip=%d",
                        pL2_entry->ipmcast_ip_mc_sip.index, pL2_entry->ipmcast_ip_mc_sip.rvid, pL2_entry->ipmcast_ip_mc_sip.dip, pL2_entry->ipmcast_ip_mc_sip.sip);

                if (pL2_entry->ipmcast_ip_mc_sip.dip == 0)
                {   /* if dip is zero, mean delete entry. So set ip_multi = 0 */
                    ip_multi = 0;
                    ip6_multi = 0;
                }
                else
                {
                    ip_multi = 1;
                    ip6_multi = 1;

                    if ((ret = table_field_set(unit, MAPLE_L2_CAM_IP_MC_SIPt, MAPLE_L2_CAM_IP_MC_SIP_IP_MCtf, &ip_multi, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_CAM_IP_MC_SIPt, MAPLE_L2_CAM_IP_MC_SIP_IP6_MCtf, &ip6_multi, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_CAM_IP_MC_SIPt, MAPLE_L2_CAM_IP_MC_SIP_GIPtf, &pL2_entry->ipmcast_ip_mc_sip.dip, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_CAM_IP_MC_SIPt, MAPLE_L2_CAM_IP_MC_SIP_SIPtf, &pL2_entry->ipmcast_ip_mc_sip.sip, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_CAM_IP_MC_SIPt, MAPLE_L2_CAM_IP_MC_SIP_FID_RVIDtf, &pL2_entry->ipmcast_ip_mc_sip.rvid, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_CAM_IP_MC_SIPt, MAPLE_L2_CAM_IP_MC_SIP_MC_PMSK_IDXtf, &pL2_entry->ipmcast_ip_mc_sip.index, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                }
            }
            else if (LOOKUP_ON_DIP_AND_FVID == ip6HashFmt[unit])
            {
                RT_LOG(LOG_TRACE, (MOD_DAL|MOD_L2), "index=%d, rvid=%d, dip=%d", pL2_entry->ipmcast_ip_mc.index, pL2_entry->ipmcast_ip_mc.rvid, pL2_entry->ipmcast_ip_mc.dip);

                if (pL2_entry->ipmcast_ip_mc_sip.dip == 0)
                {   /* if dip is zero, mean delete entry. So set ip_multi = 0 */
                    ip_multi = 0;
                    ip6_multi = 0;
                }
                else
                {
                    ip_multi = 1;
                    ip6_multi = 1;

                    if ((ret = table_field_set(unit, MAPLE_L2_CAM_IP_MCt, MAPLE_L2_CAM_IP_MC_IP_MCtf, &ip_multi, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_CAM_IP_MCt, MAPLE_L2_CAM_IP_MC_IP6_MCtf, &ip6_multi, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_CAM_IP_MCt, MAPLE_L2_CAM_IP_MC_GIPtf, &pL2_entry->ipmcast_ip_mc.dip, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_CAM_IP_MCt, MAPLE_L2_CAM_IP_MC_MC_PMSK_IDXtf, &pL2_entry->ipmcast_ip_mc.index, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_set(unit, MAPLE_L2_CAM_IP_MCt, MAPLE_L2_CAM_IP_MC_FID_RVIDtf, &pL2_entry->ipmcast_ip_mc.rvid, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
                    {
                        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                }
            }
            break;
        default:
            return RT_ERR_FAILED;
     }

    /* read entry from chip */
    if ((ret = table_write(unit, MAPLE_L2_CAM_UCt, l2_index, (uint32 *)&l2cam_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of _dal_maple_l2_setL2CAMEntry */

/* Module: static function */
/* Function Name:
 *      _dal_maple_l2_init_config
 * Description:
 *      Initialize config of l2 module for the specified device.
 * Input:
 *      unit - unit id
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *      Must initialize l2 module before calling this API.
 */
#if !defined(__BOOTLOADER__)
static int32
_dal_maple_l2_init_config(uint32 unit)
{
    int32   ret;
    uint32  port;

    if ((ret = dal_maple_l2_flushLinkDownPortAddrEnable_set(unit, RTK_DEFAULT_L2_FLUSH_LINKDOWN_MAC)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if ((ret = dal_maple_l2_exceptionAddrAction_set(unit, SA_IS_BCAST_OR_MCAST, ACTION_DROP)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    port= HWP_CPU_MACID(unit);
    if ((ret = dal_maple_l2_portNewMacOp_set(unit, port, NOT_LEARNING, ACTION_FORWARD)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if ((ret = dal_maple_l2_limitLearningCntAction_set(unit, LIMIT_LEARN_CNT_ACTION_DROP)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    HWP_PORT_TRAVS(unit, port)
    {
        if ((!HWP_PORT_EXIST(unit, port)) || HWP_IS_CPU_PORT(unit, port))
            continue;
        if ((ret = dal_maple_l2_portLimitLearningCntAction_set(unit, port, LIMIT_LEARN_CNT_ACTION_DROP)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }
    }

    if ((ret = dal_maple_l2_fidLearningCntAction_set(unit, LIMIT_LEARN_CNT_ACTION_DROP)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if ((ret = dal_maple_l2_ipMcastAddrChkEnable_set(unit, ENABLED)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if ((ret = dal_maple_l2_ipMcstFidVidCompareEnable_set(unit, ENABLED)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if ((ret = dal_maple_l2_ipmcMode_set(unit, LOOKUP_ON_DIP_AND_FVID)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if ((ret = dal_maple_l2_ip6mcMode_set(unit, LOOKUP_ON_DIP_AND_FVID)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if ((ret = dal_maple_l2_bucketHashAlgo_set(unit, 0, 0)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of _dal_maple_l2_init_config */
#endif /* !defined(__BOOTLOADER__) */

/* Function Name:
 *      dal_maple_l2_getL2EntryfromHash
 * Description:
 *      Get L2 Entry from Chip
 * Input:
 *      unit      - unit id
 *      hashKey   - Hash Key for this Entry
 *      location  - Entry location in Hash Bucket
 *      pL2_entry - L2 entry used to do search
 *      pIsValid  - Is valid entry
 *
 * Output:
 *      pL2_entry - L2 entry
 *      pIsValid  - Is valid or invalid entry
 *                    TRUE: valid entry
 *                    FALSE: invalid entry
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 * Note:
 *     This is for extern module to call
 */
int32
dal_maple_l2_getL2EntryfromHash(uint32 unit, uint32 hashkey, uint32 location, dal_maple_l2_entry_t *pL2_entry, uint32 *pIsValid)
{
    int32 ret = 0;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    L2_SEM_LOCK(unit);
    ret = _dal_maple_l2_getL2EntryfromHash(unit, hashkey, location, pL2_entry, pIsValid);
    L2_SEM_UNLOCK(unit);
    return ret;
} /* end of dal_maple_l2_getL2EntryfromHash */


/* Module Name    : L2     */
/* Sub-module Name: Global */

/* Function Name:
 *      dal_maple_l2_init
 * Description:
 *      Initialize l2 module of the specified device.
 * Input:
 *      unit - unit id
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *      Must initialize l2 module before calling any l2 APIs.
 */
int32
dal_maple_l2_init(uint32 unit)
{
    int32   ret;
    uint32  index;
    uint32  mcast_tableSize;
#if !defined(__BOOTLOADER__)
    rtk_portmask_t  portmask;
    uint32  val;
#endif

    RT_INIT_REENTRY_CHK(l2_init[unit]);
    l2_init[unit] = INIT_NOT_COMPLETED;

    /* create semaphore */
    l2_sem[unit] = osal_sem_mutex_create();
    if (0 == l2_sem[unit])
    {
        RT_ERR(RT_ERR_FAILED, (MOD_DAL|MOD_L2), "semaphore create failed");
        return RT_ERR_FAILED;
    }

    if ((ret = table_size_get(unit, MAPLE_MC_PMSKt, &mcast_tableSize)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_LOCK(unit);

    pL2_info[unit] = (dal_maple_l2_info_t *)osal_alloc(sizeof(dal_maple_l2_info_t));
    if (NULL == pL2_info[unit])
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(RT_ERR_FAILED, (MOD_DAL|MOD_L2), "memory allocate failed");
        return RT_ERR_FAILED;
    }
    osal_memset(pL2_info[unit], 0, sizeof(dal_maple_l2_info_t));

    /* allocate memory for free multicast index */
    mcast_idx_pool[unit].pMcast_index_pool = (dal_maple_mcast_index_t *)osal_alloc(mcast_tableSize * sizeof(dal_maple_mcast_index_t));
    if (NULL == mcast_idx_pool[unit].pMcast_index_pool)
    {
        osal_free(pL2_info[unit]);
        pL2_info[unit] = NULL;
        mcast_idx_pool[unit].pMcast_index_pool = 0;
        L2_SEM_UNLOCK(unit);
        RT_ERR(RT_ERR_FAILED, (MOD_DAL|MOD_L2), "memory allocate failed");
        return RT_ERR_FAILED;
    }

    mcast_idx_pool[unit].size_of_mcast_fwd_index = mcast_tableSize;
    mcast_idx_pool[unit].free_entry_count = mcast_tableSize;
    /* first free index is 0 */
    mcast_idx_pool[unit].free_index_head = 0;

    /* create free link-list for all entry, from 0 ~ max index - 2 */
    for (index = 0; index < (mcast_tableSize - 1); index++)
    {
        mcast_idx_pool[unit].pMcast_index_pool[index].next_index = index + 1;
        mcast_idx_pool[unit].pMcast_index_pool[index].ref_count = 0;
    }
    mcast_idx_pool[unit].pMcast_index_pool[mcast_tableSize - 1].next_index = END_OF_MCAST_IDX;
    mcast_idx_pool[unit].pMcast_index_pool[mcast_tableSize - 1].ref_count = 0;

    ucst_dlf_pmsk_inited = FALSE;
    bcst_dlf_pmsk_inited = FALSE;
    ipmcst_fvid_cmp = FALSE;

    osal_memset(ip6CacheIndexTbl, -1, RTK_DEFAULT_L2_IP6_CACHE_IDX_TBL_SIZE);
    curCacheTblIdx = 0;

    if ((ret = table_size_get(unit, MAPLE_L2_UCt, &hashTable_size[unit])) != RT_ERR_OK)
    {
        osal_free(pL2_info[unit]);
        pL2_info[unit] = NULL;
        osal_free(mcast_idx_pool[unit].pMcast_index_pool);
        mcast_idx_pool[unit].pMcast_index_pool = NULL;
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }
    if ((ret = table_size_get(unit, MAPLE_L2_CAM_UCt, &cam_size[unit])) != RT_ERR_OK)
    {
        osal_free(pL2_info[unit]);
        pL2_info[unit] = NULL;
        osal_free(mcast_idx_pool[unit].pMcast_index_pool);
        mcast_idx_pool[unit].pMcast_index_pool = NULL;
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    /* set init flag to complete init */
    l2_init[unit] = INIT_COMPLETED;

#if !defined(__BOOTLOADER__)
    if ((ret = _dal_maple_l2_init_config(unit)) != RT_ERR_OK)
    {
        l2_init[unit] = INIT_NOT_COMPLETED;
        osal_free(mcast_idx_pool[unit].pMcast_index_pool);
        mcast_idx_pool[unit].pMcast_index_pool = 0;
        mcast_idx_pool[unit].free_index_head = 0;
        mcast_idx_pool[unit].free_entry_count = 0;
        mcast_idx_pool[unit].size_of_mcast_fwd_index = 0;
        osal_free(pL2_info[unit]);
        pL2_info[unit] = NULL;
        RT_ERR(ret, (MOD_DAL|MOD_L2), "L2 default config initialize failed");
        return ret;
    }

    /* Init global variables */
    dal_maple_l2_bucketHashAlgo_get(unit, 0, &algoType[unit]);
    dal_maple_l2_ip6CareByte_get(unit, L2_SIP_HASH_CARE_BYTE, &ip6SipCareByte);
    dal_maple_l2_ip6CareByte_get(unit, L2_DIP_HASH_CARE_BYTE, &ip6DipCareByte);

    /* init fwd table entry 0, so that it won't be allocated to other application */
    index = 0;
    for(val=0; val < 3*HAL_MAX_NUM_OF_VLAN_PROFILE(unit); val++)
        _dal_maple_l2_allocMcastIdx(unit, (int32 *)&index);

    HWP_GET_ALL_PORTMASK(unit, portmask);
    dal_maple_l2_lookupMissFloodPortMask_setByIndex(unit, DLF_TYPE_UCAST, 0, &portmask);
    dal_maple_l2_lookupMissFloodPortMask_setByIndex(unit, DLF_TYPE_BCAST, 0, &portmask);

    val = 7;
    reg_write(unit, MAPLE_L2_LRN_CONSTRT_ENr, &val);
#endif /* !defined(__BOOTLOADER__) */
    return RT_ERR_OK;
} /* end of dal_maple_l2_init */

/* Function Name:
 *      dal_maple_l2_flushLinkDownPortAddrEnable_get
 * Description:
 *      Get HW flush linkdown port mac configuration of the specified device.
 * Input:
 *      unit    - unit id
 * Output:
 *      pEnable - pointer buffer of state of HW clear linkdown port mac
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      1) Make sure chip have supported the function before using the API.
 *      2) The API is apply to whole system.
 *      3)The status of flush linkdown port address is as following:
 *        - DISABLED
 *        - ENABLED
 */
int32
dal_maple_l2_flushLinkDownPortAddrEnable_get(uint32 unit, rtk_enable_t *pEnable)
{
    int32   ret;
    uint32  enable;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d", unit);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((NULL == pEnable), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if ((ret = reg_field_read(unit, MAPLE_L2_CTRL_0r, MAPLE_LINK_DOWN_P_INVLDf, &enable)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }
    L2_SEM_UNLOCK(unit);

    /* translate chip's value to definition */
    if (1 == enable)
    {
        *pEnable = ENABLED;
    }
    else
    {
        *pEnable = DISABLED;
    }

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pEnable=%d", *pEnable);

    return RT_ERR_OK;
} /* end of dal_maple_l2_flushLinkDownPortAddrEnable_get */

/* Function Name:
 *      dal_maple_l2_flushLinkDownPortAddrEnable_set
 * Description:
 *      Set HW flush linkdown port mac configuration of the specified device.
 * Input:
 *      unit   - unit id
 *      enable - configure value
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      1) Make sure chip have supported the function before using the API.
 *      2) The API is apply to whole system.
 *      3) The status of flush linkdown port address is as following:
 *        - DISABLED
 *        - ENABLED
 */
int32
dal_maple_l2_flushLinkDownPortAddrEnable_set(uint32 unit, rtk_enable_t enable)
{
    int32   ret;
    uint32  value;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, enable=%d",
           unit, enable);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((enable >= RTK_ENABLE_END), RT_ERR_INPUT);

    /* translate definition to chip's value  */
    if (ENABLED == enable)
    {
        value = 1;
    }
    else
    {
        value = 0;
    }

    L2_SEM_LOCK(unit);

    /* programming value to CHIP*/
    if ((ret = reg_field_write(unit, MAPLE_L2_CTRL_0r, MAPLE_LINK_DOWN_P_INVLDf, &value)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }
    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_flushLinkDownPortAddrEnable_set */

/* Function Name:
 *      dal_maple_l2_ucastAddr_flush
 * Description:
 *      Flush unicast address
 * Input:
 *      unit    - unit id
 *      pConfig - flush config
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_VLAN_VID     - invalid vlan id
 * Note:
 *      None
 */
int32
dal_maple_l2_ucastAddr_flush(uint32 unit, rtk_l2_flushCfg_t *pConfig)
{
    int32   ret;
    uint32  value, data;
    uint32 flush_port;
    uint32 replace_port;
    rtk_portmask_t trunk_portmask;
    int32              first_trunkMember = -1;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((NULL == pConfig), RT_ERR_NULL_POINTER);
    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, act=%d, byVid=%d, byPort=%d, vid=%d, port=%d, replacingPort=%d",
        unit, pConfig->act, pConfig->flushByVid, pConfig->flushByPort, pConfig->vid, pConfig->port, pConfig->replacingPort);
    RT_PARAM_CHK((pConfig->act >= FLUSH_ACT_END), RT_ERR_INPUT);
    RT_PARAM_CHK((pConfig->flushByVid >= RTK_ENABLE_END), RT_ERR_INPUT);
    RT_PARAM_CHK((pConfig->flushByPort >= RTK_ENABLE_END), RT_ERR_INPUT);
    RT_PARAM_CHK((TRUE == pConfig->flushByVid && (pConfig->vid > RTK_VLAN_ID_MAX)), RT_ERR_VLAN_VID);
    RT_PARAM_CHK((TRUE == pConfig->flushByPort && ((pConfig->portOrTrunk == ENABLED && !HWP_PORT_EXIST(unit, pConfig->port)) || (pConfig->portOrTrunk==DISABLED && pConfig->port>=HAL_MAX_NUM_OF_TRUNK(unit)))), RT_ERR_PORT_ID);
    RT_PARAM_CHK((TRUE == pConfig->flushByPort && FLUSH_ACT_REPLACE == pConfig->act && ((pConfig->portOrTrunk == ENABLED && !HWP_PORT_EXIST(unit, pConfig->replacingPort)) || (pConfig->portOrTrunk==DISABLED && pConfig->replacingPort>=HAL_MAX_NUM_OF_TRUNK(unit)))), RT_ERR_PORT_ID);
    RT_PARAM_CHK((pConfig->portOrTrunk >= RTK_ENABLE_END), RT_ERR_INPUT);

    if(TRUE == pConfig->flushByPort)
    {
        if(pConfig->portOrTrunk == ENABLED)
        {
            flush_port = pConfig->port;
        }
        else
        {
                if (dal_maple_trunk_port_get(unit, pConfig->port, &trunk_portmask) == RT_ERR_OK)
                {
                    if ((first_trunkMember = RTK_PORTMASK_GET_FIRST_PORT(trunk_portmask)) == -1)
                    {
                        /* no trunk member */
                        return RT_ERR_INPUT;
                    }
                }

                flush_port = (uint32)first_trunkMember;
        }

        if(pConfig->act == FLUSH_ACT_REPLACE)
        {

            if(pConfig->portOrTrunk == ENABLED)
            {
                replace_port = pConfig->replacingPort;
            }
            else
            {
                    if (dal_maple_trunk_port_get(unit, pConfig->replacingPort, &trunk_portmask) == RT_ERR_OK)
                    {
                        if ((first_trunkMember = RTK_PORTMASK_GET_FIRST_PORT(trunk_portmask)) == -1)
                        {
                            /* no trunk member */
                            return RT_ERR_INPUT;
                        }

                        replace_port = (uint32)first_trunkMember;
                    }
            }
        }
    }
    else
    {
        flush_port = 0;
        replace_port = 0;
    }

    data = 0;

    L2_SEM_LOCK(unit);
    value = (pConfig->act == FLUSH_ACT_FLUSH) ? 0 : 1;
    if ((ret = reg_field_set(unit, MAPLE_L2_TBL_FLUSH_CTRLr, MAPLE_ACTf, &value, &data)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if ((ret = reg_field_set(unit, MAPLE_L2_TBL_FLUSH_CTRLr, MAPLE_FVID_CMPf, &pConfig->flushByVid, &data)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if ((ret = reg_field_set(unit, MAPLE_L2_TBL_FLUSH_CTRLr, MAPLE_PORT_CMPf, &pConfig->flushByPort, &data)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    value = pConfig->flushStaticAddr ? 0 : 1;  /* 0:include static   1:dynamic only */
    if ((ret = reg_field_set(unit, MAPLE_L2_TBL_FLUSH_CTRLr, MAPLE_ENTRY_TYPEf, &value, &data)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if ((ret = reg_field_set(unit, MAPLE_L2_TBL_FLUSH_CTRLr, MAPLE_FVIDf, &pConfig->vid, &data)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if ((ret = reg_field_set(unit, MAPLE_L2_TBL_FLUSH_CTRLr, MAPLE_PORTf, &flush_port, &data)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if ((ret = reg_field_set(unit, MAPLE_L2_TBL_FLUSH_CTRLr, MAPLE_REPLACING_PORTf, &replace_port, &data)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    value = 1;  /* start */
    if ((ret = reg_field_set(unit, MAPLE_L2_TBL_FLUSH_CTRLr, MAPLE_STSf, &value, &data)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    /* program value to CHIP*/
    if ((ret = reg_write(unit, MAPLE_L2_TBL_FLUSH_CTRLr, &data)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    while(1)
    {
        reg_read(unit, MAPLE_L2_TBL_FLUSH_CTRLr, &data);
        if((data & 0x4000000) == 0)
            break;
    }

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_ucastAddr_flush */

/* Function Name:
 *      dal_maple_l2_learningCnt_get
 * Description:
 *      Get the mac learning counts of the specified device.
 * Input:
 *      unit     - unit id
 * Output:
 *      pMac_cnt - pointer buffer of mac learning counts of the port
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      1) The mac learning counts only calculate dynamic mac numbers.
 */
int32
dal_maple_l2_learningCnt_get(uint32 unit, uint32 *pMac_cnt)
{
    int32 ret;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d", unit);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((NULL == pMac_cnt), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if ((ret = reg_field_read(unit, MAPLE_L2_LRN_CONSTRTr, MAPLE_LRN_CNTf, pMac_cnt)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }
    L2_SEM_UNLOCK(unit);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pMac_cnt=%d", *pMac_cnt);

    return RT_ERR_OK;
} /* end of dal_maple_l2_learningCnt_get */

/* Function Name:
 *      dal_maple_l2_limitLearningCnt_get
 * Description:
 *      Get the maximum mac learning counts of the specified device.
 * Input:
 *      unit     - unit id
 * Output:
 *      pMac_cnt - pointer buffer of maximum mac learning counts
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      1) The maximum mac learning counts only limit for dynamic learning mac
 *         address, not apply to static mac address.
 *      2) Set the mac_cnt to 0 mean disable learning in the system.
 */
int32
dal_maple_l2_limitLearningCnt_get(uint32 unit, uint32 *pMac_cnt)
{
    int32 ret;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d", unit);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((NULL == pMac_cnt), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if ((ret = reg_field_read(unit, MAPLE_L2_LRN_CONSTRTr, MAPLE_CONSTRT_NUMf, pMac_cnt)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }
    L2_SEM_UNLOCK(unit);

    if (*pMac_cnt == HAL_L2_LEARN_LIMIT_CNT_DISABLE(unit))
        *pMac_cnt = L2_MAC_CST_DISABLE;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pMac_cnt=%d", *pMac_cnt);

    return RT_ERR_OK;
} /* end of dal_maple_l2_limitLearningCnt_get */

/* Function Name:
 *      dal_maple_l2_limitLearningCnt_set
 * Description:
 *      Set the maximum mac learning counts of the specified device.
 * Input:
 *      unit    - unit id
 *      mac_cnt - maximum mac learning counts
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT            - The module is not initial
 *      RT_ERR_LIMITED_L2ENTRY_NUM - invalid limited L2 entry number
 * Note:
 *      1) The maximum mac learning counts only limit for dynamic learning mac
 *         address, not apply to static mac address.
 *      2) Set the mac_cnt to 0 mean disable learning in the system.
 */
int32
dal_maple_l2_limitLearningCnt_set(uint32 unit, uint32 mac_cnt)
{
    int32 ret;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, mac_cnt=%d", unit, mac_cnt);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((mac_cnt > HAL_L2_LEARN_LIMIT_CNT_MAX(unit)) && (mac_cnt != HAL_L2_LEARN_LIMIT_CNT_DISABLE(unit)), RT_ERR_LIMITED_L2ENTRY_NUM);

    L2_SEM_LOCK(unit);

    /* programming value into CHIP*/
    if ((ret = reg_field_write(unit, MAPLE_L2_LRN_CONSTRTr, MAPLE_CONSTRT_NUMf, &mac_cnt)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }
    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_limitLearningCnt_set */

/* Function Name:
 *      dal_maple_l2_limitLearningCntAction_get
 * Description:
 *      Get the action when over learning maximum mac counts of the specified device.
 * Input:
 *      unit    - unit id
 * Output:
 *      pAction - pointer buffer of action when over learning maximum mac counts
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      1) The action symbol as following
 *         - LIMIT_LEARN_CNT_ACTION_DROP
 *         - LIMIT_LEARN_CNT_ACTION_FORWARD
 *         - LIMIT_LEARN_CNT_ACTION_TO_CPU
 */
int32
dal_maple_l2_limitLearningCntAction_get(uint32 unit, rtk_l2_limitLearnCntAction_t *pAction)
{
    int32   ret;
    uint32  act;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d", unit);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((NULL == pAction), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if ((ret = reg_field_read(unit, MAPLE_L2_LRN_CONSTRTr, MAPLE_ACTf, &act)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "ret=%d",ret);
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    /* translate register value to action */
    switch (act)
    {
        case LIMIT_LEARN_CNT_ACTION_DROP:
            *pAction = 1;
            break;
        case LIMIT_LEARN_CNT_ACTION_FORWARD:
            *pAction = 0;
            break;
        case LIMIT_LEARN_CNT_ACTION_TO_CPU:
            *pAction = 2;
            break;
        case LIMIT_LEARN_CNT_ACTION_COPY_TO_CPU:
            *pAction = 3;
            break;
        default:
            return RT_ERR_FAILED;
    }

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pAction=%d", *pAction);

    return RT_ERR_OK;
} /* end of dal_maple_l2_limitLearningCntAction_get */

/* Function Name:
 *      dal_maple_l2_limitLearningCntAction_set
 * Description:
 *      Set the action when over learning maximum mac counts of the specified device.
 * Input:
 *      unit   - unit id
 *      action - action when over learning maximum mac counts
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      1) The action symbol as following
 *        - LIMIT_LEARN_CNT_ACTION_DROP
 *        - LIMIT_LEARN_CNT_ACTION_FORWARD
 *        - LIMIT_LEARN_CNT_ACTION_TO_CPU
 */
int32
dal_maple_l2_limitLearningCntAction_set(uint32 unit, rtk_l2_limitLearnCntAction_t action)
{
    int32   ret;
    uint32  act;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, action=%d", unit, action);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(action >= LIMIT_LEARN_CNT_ACTION_END, RT_ERR_INPUT);

    switch (action)
    {
        case LIMIT_LEARN_CNT_ACTION_DROP:
            act = 1;
            break;
        case LIMIT_LEARN_CNT_ACTION_FORWARD:
            act = 0;
            break;
        case LIMIT_LEARN_CNT_ACTION_TO_CPU:
            act = 2;
            break;
        case LIMIT_LEARN_CNT_ACTION_COPY_TO_CPU:
            act = 3;
            break;
        default:
            return RT_ERR_INPUT;
    }

    L2_SEM_LOCK(unit);

    /* programming value to CHIP*/
    if ((ret = reg_field_write(unit, MAPLE_L2_LRN_CONSTRTr, MAPLE_ACTf, &act)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_limitLearningCntAction_set */

/* Function Name:
 *      dal_maple_l2_portLearningCnt_get
 * Description:
 *      Get the mac learning counts of the port from the specified device.
 * Input:
 *      unit     - unit id
 *      port     - port id
 * Output:
 *      pMac_cnt - pointer buffer of mac learning counts of the port
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      1) The mac learning counts only calculate dynamic mac numbers.
 */
int32
dal_maple_l2_portLearningCnt_get(uint32 unit, rtk_port_t port, uint32 *pMac_cnt)
{
    int32 ret;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, port=%d", unit, port);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(!HWP_ETHER_PORT(unit, port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((NULL == pMac_cnt), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if ((ret = reg_array_field_read(unit, MAPLE_L2_PORT_LRN_CONSTRTr, port, REG_ARRAY_INDEX_NONE, MAPLE_LRN_CNTf, pMac_cnt)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }
    L2_SEM_UNLOCK(unit);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pMac_cnt=%d", *pMac_cnt);

    return RT_ERR_OK;
} /* end of dal_maple_l2_portLearningCnt_get */

/* Function Name:
 *      dal_maple_l2_portLimitLearningCnt_get
 * Description:
 *      Get the maximum mac learning counts of the port from the specified device.
 * Input:
 *      unit     - unit id
 *      port     - port id
 * Output:
 *      pMac_cnt - pointer buffer of maximum mac learning counts
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      1) The maximum mac learning counts only limit for dynamic learning mac
 *         address, not apply to static mac address.
 *      2) Set the mac_cnt to 0 mean disable learning in the port.
 */
int32
dal_maple_l2_portLimitLearningCnt_get(uint32 unit, rtk_port_t port, uint32 *pMac_cnt)
{
    int32 ret;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, port=%d", unit, port);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(!HWP_ETHER_PORT(unit, port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((NULL == pMac_cnt), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if((ret = reg_array_field_read(unit, MAPLE_L2_PORT_LRN_CONSTRTr, port, REG_ARRAY_INDEX_NONE, MAPLE_CONSTRT_NUMf, pMac_cnt)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "ret=%d",ret);
        return ret;
    }
    L2_SEM_UNLOCK(unit);

    if (*pMac_cnt == HAL_L2_LEARN_LIMIT_CNT_DISABLE(unit))
        *pMac_cnt = L2_MAC_CST_DISABLE;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pMac_cnt=%d", *pMac_cnt);

    return RT_ERR_OK;
} /* end of dal_maple_l2_portLimitLearningCnt_get */

/* Function Name:
 *      dal_maple_l2_portLimitLearningCnt_set
 * Description:
 *      Set the maximum mac learning counts of the port to the specified device.
 * Input:
 *      unit    - unit id
 *      port    - port id
 *      mac_cnt - maximum mac learning counts
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT            - The module is not initial
 *      RT_ERR_PORT_ID             - invalid port id
 *      RT_ERR_LIMITED_L2ENTRY_NUM - invalid limited L2 entry number
 * Note:
 *      1) The maximum mac learning counts only limit for dynamic learning mac
 *         address, not apply to static mac address.
 *      2) Set the mac_cnt to 0 mean disable learning in the port.
 */
int32
dal_maple_l2_portLimitLearningCnt_set(uint32 unit, rtk_port_t port, uint32 mac_cnt)
{
    int32 ret;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, port=%d, mac_cnt=%d",
           unit, port, mac_cnt);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(!HWP_ETHER_PORT(unit, port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((mac_cnt > HAL_L2_LEARN_LIMIT_CNT_MAX(unit)) && (mac_cnt != HAL_L2_LEARN_LIMIT_CNT_DISABLE(unit)), RT_ERR_LIMITED_L2ENTRY_NUM);

    L2_SEM_LOCK(unit);

    /* programming value into CHIP*/
    if((ret = reg_array_field_write(unit, MAPLE_L2_PORT_LRN_CONSTRTr, port, REG_ARRAY_INDEX_NONE, MAPLE_CONSTRT_NUMf, &mac_cnt)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "ret=%d",ret);
        return ret;
    }
    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_portLimitLearningCnt_set */

/* Function Name:
 *      dal_maple_l2_portLimitLearningCntAction_get
 * Description:
 *      Get the action when over learning maximum mac counts of the port from the specified device.
 * Input:
 *      unit    - unit id
 *      port    - port id
 * Output:
 *      pAction - pointer buffer of action when over learning maximum mac counts
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      1) The action symbol as following
 *        - LIMIT_LEARN_CNT_ACTION_DROP
 *        - LIMIT_LEARN_CNT_ACTION_FORWARD
 *        - LIMIT_LEARN_CNT_ACTION_TO_CPU
 */
int32
dal_maple_l2_portLimitLearningCntAction_get(uint32 unit, rtk_port_t port, rtk_l2_limitLearnCntAction_t *pAction)
{
    int32   ret;
    uint32  act;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, port=%d", unit, port);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(!HWP_ETHER_PORT(unit, port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((NULL == pAction), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if((ret = reg_array_field_read(unit, MAPLE_L2_PORT_LRN_CONSTRTr, port, REG_ARRAY_INDEX_NONE, MAPLE_ACTf, &act)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "ret=%d",ret);
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    /* translate register value to action */
    switch (act)
    {
        case LIMIT_LEARN_CNT_ACTION_DROP:
            *pAction = 1;
            break;
        case LIMIT_LEARN_CNT_ACTION_FORWARD:
            *pAction = 0;
            break;
        case LIMIT_LEARN_CNT_ACTION_TO_CPU:
            *pAction = 2;
            break;
        case LIMIT_LEARN_CNT_ACTION_COPY_TO_CPU:
            *pAction = 3;
            break;
        default:
            return RT_ERR_FAILED;
    }

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pAction=%d", *pAction);

    return RT_ERR_OK;
} /* end of dal_maple_l2_portLimitLearningCntAction_get */

/* Function Name:
 *      dal_maple_l2_portLimitLearningCntAction_set
 * Description:
 *      Set the action when over learning maximum mac counts of the port to the specified device.
 * Input:
 *      unit   - unit id
 *      port   - port id
 *      action - action when over learning maximum mac counts
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_PORT_ID  - invalid port id
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      1) The action symbol as following
 *        - LIMIT_LEARN_CNT_ACTION_DROP
 *        - LIMIT_LEARN_CNT_ACTION_FORWARD
 *        - LIMIT_LEARN_CNT_ACTION_TO_CPU
 *        - LIMIT_LEARN_CNT_ACTION_COPY_TO_CPU
 */
int32
dal_maple_l2_portLimitLearningCntAction_set(uint32 unit, rtk_port_t port, rtk_l2_limitLearnCntAction_t action)
{
    int32   ret;
    uint32  act;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, port=%d, action=%d", unit, port, action);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(!HWP_ETHER_PORT(unit, port), RT_ERR_PORT_ID);
    RT_PARAM_CHK(action >= LIMIT_LEARN_CNT_ACTION_END, RT_ERR_INPUT);

    switch (action)
    {
        case LIMIT_LEARN_CNT_ACTION_DROP:
            act = 1;
            break;
        case LIMIT_LEARN_CNT_ACTION_FORWARD:
            act = 0;
            break;
        case LIMIT_LEARN_CNT_ACTION_TO_CPU:
            act = 2;
            break;
        case LIMIT_LEARN_CNT_ACTION_COPY_TO_CPU:
            act = 3;
            break;
        default:
            return RT_ERR_INPUT;
    }

    L2_SEM_LOCK(unit);

    /* programming value to CHIP*/
    if((ret = reg_array_field_write(unit, MAPLE_L2_PORT_LRN_CONSTRTr, port, REG_ARRAY_INDEX_NONE, MAPLE_ACTf, &act)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "ret=%d",ret);
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_portLimitLearningCntAction_set */

/* Function Name:
 *      dal_maple_l2_fidLimitLearningEntry_get
 * Description:
 *      Get FID MAC limit entry.
 * Input:
 *      unit              - unit id
 *      fid_macLimit_idx  - index of FID MAC limit entry
 * Output:
 *      pFidMacLimitEntry - pointer to MAC limit entry
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_OUT_OF_RANGE - input parameter out of range
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Forwarding action is as following
 *      - LIMIT_LEARN_CNT_ACTION_DROP
 *      - LIMIT_LEARN_CNT_ACTION_FORWARD
 *      - LIMIT_LEARN_CNT_ACTION_TO_CPU
 */
int32
dal_maple_l2_fidLimitLearningEntry_get(uint32 unit, uint32 fid_macLimit_idx, rtk_l2_fidMacLimitEntry_t *pFidMacLimitEntry)
{
    int32   ret;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, fid_macLimit_idx=%d", unit, fid_macLimit_idx);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((fid_macLimit_idx >= HAL_L2_FID_LEARN_LIMIT_ENTRY_MAX(unit)), RT_ERR_OUT_OF_RANGE);
    RT_PARAM_CHK((NULL == pFidMacLimitEntry), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);
    if ((ret = reg_array_field_read(unit, MAPLE_L2_VLAN_LRN_CONSTRTr, REG_ARRAY_INDEX_NONE, fid_macLimit_idx, MAPLE_FVIDf, &pFidMacLimitEntry->fid)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "ret=%d",ret);
        return ret;
    }
    if ((ret = reg_array_field_read(unit, MAPLE_L2_VLAN_LRN_CONSTRTr, REG_ARRAY_INDEX_NONE, fid_macLimit_idx, MAPLE_PORTf, &pFidMacLimitEntry->port)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }
    if ((ret = reg_array_field_read(unit, MAPLE_L2_VLAN_LRN_CONSTRTr, REG_ARRAY_INDEX_NONE, fid_macLimit_idx, MAPLE_CONSTRT_NUMf, &pFidMacLimitEntry->maxNum)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }
    L2_SEM_UNLOCK(unit);

    if (pFidMacLimitEntry->maxNum == HAL_L2_LEARN_LIMIT_CNT_DISABLE(unit))
        pFidMacLimitEntry->maxNum = L2_MAC_CST_DISABLE;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "fid=%d", pFidMacLimitEntry->fid);
    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "port=%d", pFidMacLimitEntry->port);
    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "maxNum=%d", pFidMacLimitEntry->maxNum);

    return RT_ERR_OK;
} /* end of dal_maple_l2_fidLimitLearningEntry_get */

/* Function Name:
 *      dal_maple_l2_fidLimitLearningEntry_set
 * Description:
 *      Set FID MAC limit entry.
 * Input:
 *      unit              - unit id
 *      fid_macLimit_idx  - index of FID MAC limit entry
 *      pFidMacLimitEntry - MAC limit entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_OUT_OF_RANGE - input parameter out of range
 *      RT_ERR_VLAN_VID     - invalid vlan id
 *      RT_ERR_PORT_ID      - invalid port id
 * Note:
 *      Forwarding action is as following
 *      - LIMIT_LEARN_CNT_ACTION_DROP
 *      - LIMIT_LEARN_CNT_ACTION_FORWARD
 *      - LIMIT_LEARN_CNT_ACTION_TO_CPU
 */
int32
dal_maple_l2_fidLimitLearningEntry_set(uint32 unit, uint32 fid_macLimit_idx, rtk_l2_fidMacLimitEntry_t *pFidMacLimitEntry)
{
    int32       ret;
    uint32              oriVid, zero = 0;
    rtk_l2_flushCfg_t   config;
    rtk_port_t          lPort;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);


    if (pFidMacLimitEntry->port == L2_PORT_DONT_CARE)
        lPort = PORT_DONT_CARE_8380;
    else
        lPort = pFidMacLimitEntry->port;

    /* parameter check */
    RT_PARAM_CHK(NULL == pFidMacLimitEntry, RT_ERR_NULL_POINTER);
    if (pFidMacLimitEntry->maxNum == L2_MAC_CST_DISABLE)
        pFidMacLimitEntry->maxNum = HAL_L2_LEARN_LIMIT_CNT_DISABLE(unit);
    RT_PARAM_CHK((pFidMacLimitEntry->maxNum > HAL_L2_LEARN_LIMIT_CNT_MAX(unit)) && (pFidMacLimitEntry->maxNum != HAL_L2_LEARN_LIMIT_CNT_DISABLE(unit)) , RT_ERR_LIMITED_L2ENTRY_NUM);

    /* Display debug message */
    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, fid_macLimit_idx, fid=%d, port=%d, maxNum=%d", unit, fid_macLimit_idx,
                                        pFidMacLimitEntry->fid, pFidMacLimitEntry->port, pFidMacLimitEntry->maxNum);
    RT_PARAM_CHK((fid_macLimit_idx >= HAL_L2_FID_LEARN_LIMIT_ENTRY_MAX(unit)), RT_ERR_OUT_OF_RANGE);
    RT_PARAM_CHK((pFidMacLimitEntry->fid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    RT_PARAM_CHK((!HWP_PORT_EXIST(unit, lPort) && lPort != PORT_DONT_CARE_8380), RT_ERR_PORT_ID);


    osal_memset(&config, 0, sizeof(rtk_l2_flushCfg_t));


    L2_SEM_LOCK(unit);

    if ((ret = reg_array_field_read(unit, MAPLE_L2_VLAN_LRN_CONSTRTr, REG_ARRAY_INDEX_NONE, fid_macLimit_idx, MAPLE_FVIDf, &oriVid)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    /* If this entry has been used, we should flush all MACs leart on this previous vlan */
    if (oriVid != 0)
    {
        if ((ret = reg_array_field_write(unit, MAPLE_L2_VLAN_LRN_CONSTRTr, REG_ARRAY_INDEX_NONE, fid_macLimit_idx, MAPLE_CONSTRT_NUMf, &zero)) != RT_ERR_OK)
        {
            L2_SEM_UNLOCK(unit);
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }
        config.act = 0;
        config.flushByVid = TRUE;
        config.vid = oriVid;
        L2_SEM_UNLOCK(unit);
        dal_maple_l2_ucastAddr_flush(unit, &config);
        L2_SEM_LOCK(unit);
    }

    /* Disable learning and flush MACs on specified vlan */
    if ((ret = reg_array_field_write(unit, MAPLE_L2_VLAN_LRN_CONSTRTr, REG_ARRAY_INDEX_NONE, fid_macLimit_idx, MAPLE_CONSTRT_NUMf, &zero)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }
    if ((ret = reg_array_field_write(unit, MAPLE_L2_VLAN_LRN_CONSTRTr, REG_ARRAY_INDEX_NONE, fid_macLimit_idx, MAPLE_FVIDf, &pFidMacLimitEntry->fid)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "ret=%d",ret);
        return ret;
    }
    if ((ret = reg_array_field_write(unit, MAPLE_L2_VLAN_LRN_CONSTRTr, REG_ARRAY_INDEX_NONE, fid_macLimit_idx, MAPLE_PORTf, &lPort)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }
    L2_SEM_UNLOCK(unit);

    config.flushByVid = TRUE;
    config.vid = pFidMacLimitEntry->fid;
    dal_maple_l2_ucastAddr_flush(unit, &config);


    /* Enable learning to user specified number */
    L2_SEM_LOCK(unit);
    if ((ret = reg_array_field_write(unit, MAPLE_L2_VLAN_LRN_CONSTRTr, REG_ARRAY_INDEX_NONE, fid_macLimit_idx, MAPLE_CONSTRT_NUMf, &pFidMacLimitEntry->maxNum)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);


    return ret;
} /* end of dal_maple_l2_srcPortEgrFilterMask_set */

/* Function Name:
 *      dal_maple_l2_fidLearningCnt_get
 * Description:
 *      Get number of learned MAC addresses on specified fid.
 * Input:
 *      unit             - unit id
 *      fid_macLimit_idx - index of FID MAC limit entry
 * Output:
 *      pNum             - number of learned MAC addresses
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_OUT_OF_RANGE - input parameter out of range
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
dal_maple_l2_fidLearningCnt_get(uint32 unit, uint32 fid_macLimit_idx, uint32 *pNum)
{
    int32   ret;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, fid_macLimit_idx=%d", unit, fid_macLimit_idx);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((fid_macLimit_idx >= HAL_L2_FID_LEARN_LIMIT_ENTRY_MAX(unit)), RT_ERR_OUT_OF_RANGE);
    RT_PARAM_CHK((NULL == pNum), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);
    if ((ret = reg_array_field_read(unit, MAPLE_L2_VLAN_LRN_CONSTRT_CNTr, REG_ARRAY_INDEX_NONE, fid_macLimit_idx, MAPLE_LRN_CNTf, pNum)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }
    L2_SEM_UNLOCK(unit);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pNum=%d", *pNum);

    return RT_ERR_OK;
} /* end of dal_maple_l2_fidLearningCnt_get */

/* Function Name:
 *      dal_maple_l2_fidLearningCnt_reset
 * Description:
 *      Reset number of learned MAC addresses on specified entry of fid MAC limit.
 * Input:
 *      unit             - unit id
 *      fid_macLimit_idx - index of FID MAC limit entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_OUT_OF_RANGE - input parameter out of range
 * Note:
 *      None
 */
int32
dal_maple_l2_fidLearningCnt_reset(uint32 unit, uint32 fid_macLimit_idx)
{
    int32   ret;
    uint32  value;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((fid_macLimit_idx >= HAL_L2_FID_LEARN_LIMIT_ENTRY_MAX(unit)), RT_ERR_OUT_OF_RANGE);

    /* Display debug message */
    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, fid_macLimit_idx=%d", unit, fid_macLimit_idx);

    value = 0;
    L2_SEM_LOCK(unit);

    if ((ret = reg_array_field_write(unit, MAPLE_L2_VLAN_LRN_CONSTRT_CNTr, REG_ARRAY_INDEX_NONE, fid_macLimit_idx, MAPLE_LRN_CNTf, &value)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    return ret;
} /* end of dal_maple_l2_fidLearningCnt_reset */

/* Function Name:
 *      dal_maple_l2_fidLearningCntAction_get
 * Description:
 *      Get the action when over learning maximum mac counts on specified entry of fid MAC limit.
 * Input:
 *      unit             - unit id
 * Output:
 *      pAction          - pointer buffer of action when over learning maximum mac counts
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
dal_maple_l2_fidLearningCntAction_get(uint32 unit, rtk_l2_limitLearnCntAction_t *pAction)
{
    int32   ret;
    uint32  act;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d", unit);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((NULL == pAction), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if((ret = reg_field_read(unit, MAPLE_L2_VLAN_LRN_CONSTRT_ACTr, MAPLE_ACTf, &act)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    /* translate register value to action */
    switch (act)
    {
        case LIMIT_LEARN_CNT_ACTION_DROP:
            *pAction = 1;
            break;
        case LIMIT_LEARN_CNT_ACTION_FORWARD:
            *pAction = 0;
            break;
        case LIMIT_LEARN_CNT_ACTION_TO_CPU:
            *pAction = 2;
            break;
        case LIMIT_LEARN_CNT_ACTION_COPY_TO_CPU:
            *pAction = 3;
            break;
        default:
            return RT_ERR_FAILED;
    }

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pAction=%d", *pAction);

    return RT_ERR_OK;
} /* end of dal_maple_l2_fidLearningCntAction_get */

/* Function Name:
 *      dal_maple_l2_fidLearningCntAction_set
 * Description:
 *      Set the action when over learning maximum mac counts on specified entry of fid MAC limit.
 * Input:
 *      unit   - unit id
 *      action - action when over learning maximum mac counts
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      None
 */
int32
dal_maple_l2_fidLearningCntAction_set(uint32 unit, rtk_l2_limitLearnCntAction_t action)
{
    int32   ret;
    uint32  act;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* Display debug message */
    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d", unit);

    /* parameter check */
    RT_PARAM_CHK(action >= LIMIT_LEARN_CNT_ACTION_END, RT_ERR_INPUT);

    switch (action)
    {
        case LIMIT_LEARN_CNT_ACTION_DROP:
            act = 1;
            break;
        case LIMIT_LEARN_CNT_ACTION_FORWARD:
            act = 0;
            break;
        case LIMIT_LEARN_CNT_ACTION_TO_CPU:
            act = 2;
            break;
        case LIMIT_LEARN_CNT_ACTION_COPY_TO_CPU:
            act = 3;
            break;
        default:
            return RT_ERR_INPUT;
    }

    L2_SEM_LOCK(unit);
    if ((ret = reg_field_write(unit, MAPLE_L2_VLAN_LRN_CONSTRT_ACTr, MAPLE_ACTf, &act)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    return ret;
} /* end of dal_maple_l2_fidLearningCntAction_set */

/* Function Name:
 *      dal_maple_l2_aging_get
 * Description:
 *      Get the dynamic address aging time from the specified device.
 * Input:
 *      unit        - unit id
 * Output:
 *      pAging_time - pointer buffer of aging time
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Get aging_time as 0 mean disable aging mechanism. (seconds)
 */
int32
dal_maple_l2_aging_get(uint32 unit, uint32 *pAging_time)
{
    int32   ret;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d", unit);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((NULL == pAging_time), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    if((ret = reg_field_read(unit, MAPLE_L2_CTRL_1r, MAPLE_AGE_UNITf, pAging_time)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    *pAging_time = (uint32)((*pAging_time)*128)/625;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pAging_time=%ld", *pAging_time);

    return RT_ERR_OK;
} /* end of dal_maple_l2_aging_get */

/* Function Name:
 *      dal_maple_l2_aging_set
 * Description:
 *      Set the dynamic address aging time to the specified device.
 * Input:
 *      unit       - unit id
 *      aging_time - aging time
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      apply aging_time as 0 mean disable aging mechanism.
 */
int32
dal_maple_l2_aging_set(uint32 unit, uint32 aging_time)
{
    int32   ret;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d", unit);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /*
     * translate to asic time: Val(user) = 0.1024 * 2 * Val(chip)
     * Val(chip) = Val(user)/(0.1024 *2) = Val(user)/(0.2048)
     *           = Val(user)*10000/2048 = Val(user)*625/128
     */
    aging_time = (uint32)((aging_time*625+127)/128);

    L2_SEM_LOCK(unit);

    if((ret = reg_field_write(unit, MAPLE_L2_CTRL_1r, MAPLE_AGE_UNITf, &aging_time)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_aging_set */

/* Function Name:
 *      dal_maple_l2_portNewMacOp_get
 * Description:
 *      Get learning mode and forwarding action of new learned address on specified port.
 * Input:
 *      unit       - unit id
 *      port       - port id
 * Output:
 *      pLrnMode   - pointer to learning mode
 *      pFwdAction - pointer to forwarding action
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Learning mode is as following
 *      - HARDWARE_LEARNING
 *      - SUSPEND_LEARNING
 *      - NOT_LEARNING
 *
 *      Forwarding action is as following
 *      - ACTION_FORWARD
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 *      - ACTION_COPY2CPU
 */
int32
dal_maple_l2_portNewMacOp_get(uint32 unit, rtk_port_t port, rtk_l2_newMacLrnMode_t *pLrnMode, rtk_action_t *pFwdAction)
{
    int32   ret;
    uint32  mode, act;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, port=%d", unit, port);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(!HWP_PORT_EXIST(unit, port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((NULL == pLrnMode), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == pFwdAction), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    if((ret = reg_array_field_read(unit, MAPLE_L2_PORT_SALRNr, port, REG_ARRAY_INDEX_NONE, MAPLE_SALRNf, &mode)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "ret=%d",ret);
        return ret;
    }
    if((ret = reg_array_field_read(unit, MAPLE_L2_PORT_NEW_SA_FWDr, port, REG_ARRAY_INDEX_NONE, MAPLE_NEW_SA_FWDf, &act)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "ret=%d",ret);
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    switch (mode)
    {
        case 0:
            *pLrnMode = HARDWARE_LEARNING;
            break;
        case 1:
            *pLrnMode = SOFTWARE_LEARNING;
            break;
        case 2:
            *pLrnMode = NOT_LEARNING;
            break;
        default:
            return RT_ERR_FAILED;
    }

    switch (act)
    {
        case 0:
            *pFwdAction = ACTION_FORWARD;
            break;
        case 1:
            *pFwdAction = ACTION_DROP;
            break;
        case 2:
            *pFwdAction = ACTION_TRAP2CPU;
            break;
        case 3:
            *pFwdAction = ACTION_COPY2CPU;
            break;
        default:
            return RT_ERR_FAILED;
    }

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pLrnMode=%d  pFwdAction:%d", *pLrnMode, *pFwdAction);

    return RT_ERR_OK;
} /* end of dal_maple_l2_newMacOp_get */

/* Function Name:
 *      dal_maple_l2_portNewMacOp_set
 * Description:
 *      Set learning mode and forwarding action of new learned address on specified port.
 * Input:
 *      unit      - unit id
 *      port      - port id
 *      lrnMode   - learning mode
 *      fwdAction - forwarding action
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT   - The module is not initial
 *      RT_ERR_PORT_ID    - invalid port id
 *      RT_ERR_INPUT      - invalid input parameter
 *      RT_ERR_FWD_ACTION - invalid forwarding action
 * Note:
 *      Learning mode is as following
 *      - HARDWARE_LEARNING
 *      - SUSPEND_LEARNING
 *      - NOT_LEARNING
 *
 *      Forwarding action is as following
 *      - ACTION_FORWARD
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 *      - ACTION_COPY2CPU
 */
int32
dal_maple_l2_portNewMacOp_set(uint32 unit, rtk_port_t port, rtk_l2_newMacLrnMode_t lrnMode, rtk_action_t fwdAction)
{
    int32   ret;
    uint32  mode, act;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, port=%d, lrnMode=%d, fwdAction=%d", unit, port, lrnMode, fwdAction);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(!HWP_PORT_EXIST(unit, port), RT_ERR_PORT_ID);
    RT_PARAM_CHK(lrnMode >= LEARNING_MODE_END, RT_ERR_INPUT);
    RT_PARAM_CHK(fwdAction >= ACTION_END, RT_ERR_INPUT);

    switch (lrnMode)
    {
        case HARDWARE_LEARNING:
            mode = 0;
            break;
        case SOFTWARE_LEARNING:
            mode = 1;
            break;
        case NOT_LEARNING:
            mode = 2;
            break;
        default:
            return RT_ERR_INPUT;
    }

    switch (fwdAction)
    {
        case ACTION_FORWARD:
            act = 0;
            break;
        case ACTION_DROP:
            act = 1;
            break;
        case ACTION_TRAP2CPU:
            act = 2;
            break;
        case ACTION_COPY2CPU:
            act = 3;
            break;
        default:
            return RT_ERR_FWD_ACTION;
    }

    L2_SEM_LOCK(unit);

    if((ret = reg_array_field_write(unit, MAPLE_L2_PORT_SALRNr, port, REG_ARRAY_INDEX_NONE, MAPLE_SALRNf, &mode)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "ret=%d",ret);
        return ret;
    }
    if((ret = reg_array_field_write(unit, MAPLE_L2_PORT_NEW_SA_FWDr, port, REG_ARRAY_INDEX_NONE, MAPLE_NEW_SA_FWDf, &act)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "ret=%d",ret);
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_newMacOp_set */

/* Function Name:
 *      dal_maple_l2_addr_init
 * Description:
 *      Initialize content of buffer of L2 entry.
 *      Will fill vid ,MAC address and reset other field of L2 entry.
 * Input:
 *      unit     - unit id
 *      vid      - vlan id
 *      pMac     - MAC address
 *      pL2_addr - L2 entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_UNIT_ID      - invalid unit id
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_VLAN_VID     - invalid vlan id
 *      RT_ERR_MAC          - invalid mac address
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_INPUT        - invalid input parameter
 * Note:
 *      Need to initialize L2 entry before add it.
 */
int32
dal_maple_l2_addr_init(
    uint32              unit,
    rtk_vlan_t          vid,
    rtk_mac_t           *pMac,
    rtk_l2_ucastAddr_t  *pL2_addr)
{
    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    RT_PARAM_CHK(NULL == pL2_addr, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK(NULL == pMac, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pMac->octet[0] & BITMASK_1B) != 0, RT_ERR_MAC);
    RT_PARAM_CHK((vid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);

    osal_memset(pL2_addr, 0, sizeof(rtk_l2_ucastAddr_t));
    pL2_addr->vid = vid;
    osal_memcpy(&pL2_addr->mac.octet[0], &pMac->octet[0], sizeof(rtk_mac_t));
    pL2_addr->age = 3;

    return RT_ERR_OK;
}

/* Module Name    : L2      */
/* Sub-module Name: Unicast */

/* Function Name:
 *      dal_maple_l2_addr_add
 * Description:
 *      Add L2 entry to ASIC.
 * Input:
 *      unit     - unit id
 *      pL2_addr - L2 entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_VLAN_VID     - invalid vlan id
 *      RT_ERR_MAC          - invalid mac address
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_INPUT        - invalid input parameter
 * Note:
 *      Need to initialize L2 entry before add it.
 */
int32
dal_maple_l2_addr_add(uint32 unit, rtk_l2_ucastAddr_t *pL2_addr)
{
    int32               ret;
    dal_maple_l2_entry_t  l2_entry;
    dal_maple_l2_index_t  index_entry;
    rtk_portmask_t      trunk_portmask;
    uint32              trk_gid;
    uint32              first_trunkMember;
    rtk_port_t          lPort;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, pL2_addr=%x", unit, pL2_addr);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    if (pL2_addr->port == L2_PORT_DONT_CARE)
        lPort = PORT_DONT_CARE_8380;
    else
        lPort = pL2_addr->port;

    /* parameter check */
    RT_PARAM_CHK(NULL == pL2_addr, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pL2_addr->mac.octet[0] & BITMASK_1B) != 0, RT_ERR_MAC);
    if (pL2_addr->flags & RTK_L2_UCAST_FLAG_TRUNK_PORT)
        RT_PARAM_CHK((pL2_addr->trk_gid >= HAL_MAX_NUM_OF_TRUNK(unit)), RT_ERR_TRUNK_ID);
    else
    {
        if (((pL2_addr->flags & RTK_L2_UCAST_FLAG_SA_BLOCK) || (pL2_addr->flags & RTK_L2_UCAST_FLAG_DA_BLOCK) || (pL2_addr->flags & RTK_L2_UCAST_FLAG_NEXTHOP)) && !(pL2_addr->flags & RTK_L2_UCAST_FLAG_STATIC))
            RT_PARAM_CHK((!HWP_PORT_EXIST(unit, lPort) && lPort != PORT_DONT_CARE_8380), RT_ERR_PORT_ID);
        else
            RT_PARAM_CHK((!HWP_PORT_EXIST(unit, lPort)), RT_ERR_PORT_ID);
    }
    RT_PARAM_CHK((pL2_addr->vid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    RT_PARAM_CHK(((pL2_addr->flags & RTK_L2_UCAST_FLAG_NEXTHOP) && pL2_addr->vlan_target >= 2 &&
                   pL2_addr->route_idx >= HAL_MAX_NUM_OF_ROUTE_HOST_ADDR(unit)), RT_ERR_INPUT);

    RT_PARAM_CHK(((pL2_addr->add_op_flags & RTK_L2_ADD_OP_FLAG_AGE_SET) && pL2_addr->age>3), RT_ERR_INPUT);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2)
            , "unit=%d, vid=%d, port=%d, flags=%x, trk_gid=%d, state=%d, vlan_target =%d, route_idx=%d"
            , unit, pL2_addr->vid, pL2_addr->port, pL2_addr->flags
            , pL2_addr->trk_gid, pL2_addr->state, pL2_addr->vlan_target, pL2_addr->route_idx);

    if((pL2_addr->add_op_flags & RTK_L2_ADD_OP_FLAG_AGE_SET) && pL2_addr->age==0)
    {
        if  (
                (pL2_addr->flags & RTK_L2_UCAST_FLAG_STATIC)==0 &&
                (pL2_addr->flags & RTK_L2_UCAST_FLAG_SA_BLOCK) == 0 &&
                (pL2_addr->flags & RTK_L2_UCAST_FLAG_DA_BLOCK) == 0 &&
                (pL2_addr->flags & RTK_L2_UCAST_FLAG_NEXTHOP) == 0
            )
            return RT_ERR_INPUT;
    }

    if (lPort != PORT_DONT_CARE_8380)
    {
        if (!(pL2_addr->flags & RTK_L2_UCAST_FLAG_TRUNK_PORT))
        {
            for (trk_gid = 0; trk_gid < HAL_MAX_NUM_OF_TRUNK(unit); trk_gid++)
            {
                if (dal_maple_trunk_port_get(unit, trk_gid, &trunk_portmask) == RT_ERR_OK)
                {
                    if ((first_trunkMember = RTK_PORTMASK_GET_FIRST_PORT(trunk_portmask)) == -1)
                    {
                        /* no trunk member */
                        continue;
                    }

                    if ((RTK_PORTMASK_IS_PORT_SET(trunk_portmask, pL2_addr->port)) &&
                        !(first_trunkMember == pL2_addr->port))
                    {
                        /* this port is trunk member and not first trunk member, not allow to add */
                        /* don't print error here because this is a easy-to-happen error that occurs when a large number of L2 entry is modifying and LACP changing members */
                        return RT_ERR_PORT_ID;
                    }
                }
            }
        }
        else
        {
            RT_PARAM_CHK(pL2_addr->trk_gid >= HAL_MAX_NUM_OF_TRUNK(unit), RT_ERR_LA_TRUNK_ID);
            /* Transfer pL2_addr->trk_gid to represent port and update pL2_addr->port */
            if ((ret = dal_maple_trunk_port_get(unit, pL2_addr->trk_gid, &trunk_portmask)) != RT_ERR_OK)
            {
                RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            if ((ret = RTK_PORTMASK_GET_FIRST_PORT(trunk_portmask)) == -1)
            {
                /* no trunk member */
                ret = RT_ERR_LA_TRUNK_ID;
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return RT_ERR_LA_TRUNK_ID;
            }
            lPort = (uint32)ret;
        }
    }

    /* search exist or free entry */
    l2_entry.entry_type = L2_UNICAST;
    l2_entry.unicast.fid = pL2_addr->vid;
    osal_memcpy(&l2_entry.unicast.mac, &(pL2_addr->mac), sizeof(rtk_mac_t));

    L2_SEM_LOCK(unit);

    ret = _dal_maple_l2_getExistOrFreeL2Entry(unit, &l2_entry, L2_GET_EXIST_OR_FREE, &index_entry);
    if (ret == RT_ERR_OK && l2_entry.is_entry_exist==1 && (pL2_addr->add_op_flags & RTK_L2_ADD_OP_FLAG_NOT_UPDATE_EXIST))
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
        if(index_entry.index_type == L2_IN_HASH)
            pL2_addr->l2_idx = (index_entry.index<<2) | index_entry.hashdepth;
        else
            pL2_addr->l2_idx = hashTable_size[unit] + index_entry.index;
        return RT_ERR_L2_ENTRY_EXIST;
    }
    else if (ret == RT_ERR_L2_NO_EMPTY_ENTRY && (pL2_addr->add_op_flags & RTK_L2_ADD_OP_FLAG_REPLACE_DYNAMIC) != 0)
    {
        ret = _dal_maple_l2_getFirstDynamicEntry(unit, &l2_entry, &index_entry);
        if (ret != RT_ERR_OK)
        {
            L2_SEM_UNLOCK(unit);
            RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
            return RT_ERR_L2_NO_EMPTY_ENTRY;
        }

        osal_memcpy(&pL2_addr->macInfo.mac, &l2_entry.unicast.mac, ETHER_ADDR_LEN);
        pL2_addr->macInfo.port      = l2_entry.unicast.port;
        pL2_addr->macInfo.vid       = l2_entry.unicast.fid;
        pL2_addr->macInfo.state     = l2_entry.unicast.suspending ? RTK_L2_UCAST_STATE_SUSPEND : 0;
        pL2_addr->macInfo.age       = l2_entry.unicast.aging;
    }
    else if (RT_ERR_OK != ret)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    /* fill content */
    osal_memcpy(&l2_entry.unicast.mac, &(pL2_addr->mac), sizeof(rtk_mac_t));
    l2_entry.unicast.fid        = pL2_addr->vid;
    l2_entry.unicast.port       = lPort;
    l2_entry.unicast.aging      = (pL2_addr->add_op_flags & RTK_L2_ADD_OP_FLAG_AGE_SET) ? pL2_addr->age : 3;
    l2_entry.unicast.sablock    = (pL2_addr->flags & RTK_L2_UCAST_FLAG_SA_BLOCK) ? TRUE: FALSE;
    l2_entry.unicast.dablock    = (pL2_addr->flags & RTK_L2_UCAST_FLAG_DA_BLOCK) ? TRUE: FALSE;
    l2_entry.unicast.is_static  = (pL2_addr->flags & RTK_L2_UCAST_FLAG_STATIC) ? TRUE: FALSE;
    l2_entry.unicast.nh         = (pL2_addr->flags & RTK_L2_UCAST_FLAG_NEXTHOP) ? TRUE : FALSE;
    l2_entry.unicast.suspending = (pL2_addr->state & RTK_L2_UCAST_STATE_SUSPEND) ? TRUE: FALSE;

    l2_entry.unicast.vlan_target = pL2_addr->vlan_target;
    l2_entry.unicast.route_idx   = pL2_addr->route_idx;
    l2_entry.unicast.agg_vid     = pL2_addr->agg_vid;


    if(l2_entry.unicast.aging==0 && l2_entry.unicast.is_static==0)
    {
        l2_entry.unicast.port       = PORT_DONT_CARE_8380;
        l2_entry.unicast.suspending = 0;
        l2_entry.unicast.agg_vid    = 0;
    }

    if (L2_IN_HASH == index_entry.index_type)
    {
        /* if found entry is in HASH, programming in HASH */
        ret = _dal_maple_l2_setL2HASHEntry(unit, &l2_entry, &index_entry);
        /* return the entry index */
        pL2_addr->l2_idx = (index_entry.index << 2) | index_entry.hashdepth;
    }
    else if (L2_IN_CAM == index_entry.index_type)
    {
        /* if found entry is in CAM, programming in CAM */
        ret = _dal_maple_l2_setL2CAMEntry(unit, &l2_entry, &index_entry);
        pL2_addr->l2_idx = hashTable_size[unit] + index_entry.index;
    }
    else
    {
        ret = RT_ERR_FAILED;
    }

    L2_SEM_UNLOCK(unit);

    return ret;
} /* end of dal_maple_l2_addr_add */

/* Function Name:
 *      dal_maple_l2_addr_del
 * Description:
 *      Delete a L2 unicast address entry from the specified device.
 * Input:
 *      unit - unit id
 *      vid  - vlan id
 *      pMac - mac address
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT          - The module is not initial
 *      RT_ERR_VLAN_VID          - invalid vid
 *      RT_ERR_MAC               - invalid mac address
 *      RT_ERR_L2_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      1) vid is same as fid in IVL mode.
 *      2) For IVL and SVL co-work mode, need to discuss API late.
 */
int32
dal_maple_l2_addr_del(uint32 unit, rtk_vlan_t vid, rtk_mac_t *pMac)
{
    int32 ret;
    dal_maple_l2_entry_t  l2_entry;
    dal_maple_l2_index_t  index_entry;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, vid=%d", unit, vid);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((vid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    RT_PARAM_CHK(NULL == pMac, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pMac->octet[0] & BITMASK_1B) != 0, RT_ERR_MAC);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pMac=%x-%x-%x-%x-%x-%x",
           pMac->octet[0], pMac->octet[1], pMac->octet[2],
           pMac->octet[3], pMac->octet[4], pMac->octet[5]);

    /* search exist or free entry */
    l2_entry.entry_type = L2_UNICAST;
    l2_entry.unicast.fid = vid;
    osal_memcpy(&l2_entry.unicast.mac, pMac, sizeof(rtk_mac_t));

    L2_SEM_LOCK(unit);

    if ((ret = _dal_maple_l2_getExistOrFreeL2Entry(unit, &l2_entry, L2_GET_EXIST_ONLY, &index_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "ret=%d", ret);
        return ret;
    }

    /* fill content */
    osal_memset(&l2_entry, 0, sizeof(dal_maple_l2_entry_t));
    l2_entry.entry_type = L2_UNICAST;

    if (L2_IN_HASH == index_entry.index_type)
    {
        /* if found entry is in HASH, programming in HASH */
        ret = _dal_maple_l2_setL2HASHEntry(unit, &l2_entry, &index_entry);
    }
    else if (L2_IN_CAM == index_entry.index_type)
    {
        /* if found entry is in CAM, programming in CAM */
        ret = _dal_maple_l2_setL2CAMEntry(unit, &l2_entry, &index_entry);
    }
    else
    {
        ret = RT_ERR_FAILED;
    }

    L2_SEM_UNLOCK(unit);

    return ret;
} /* end of dal_maple_l2_addr_del */

/* Function Name:
 *      dal_maple_l2_addr_get
 * Description:
 *      Get a L2 unicast address entry from the specified device.
 * Input:
 *      unit     - unit id
 *      pL2_data - structure of l2 address data
 * Output:
 *      pL2_data - structure of l2 address data
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT          - The module is not initial
 *      RT_ERR_VLAN_VID          - invalid vid
 *      RT_ERR_MAC               - invalid mac address
 *      RT_ERR_NULL_POINTER      - input parameter may be null pointer
 *      RT_ERR_L2_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      1) vid is same as fid in IVL mode.
 *      2) For IVL and SVL co-work mode, need to discuss API late.
 *      3) The *pL2_data.vid and *pL2_data.mac is input key
 *      4) The *pL2_data.port, *pL2_data.sa_block,
 *         *pL2_data.da_block and *pL2_data.is_static is output.
 */
int32
dal_maple_l2_addr_get(uint32 unit, rtk_l2_ucastAddr_t *pL2_data)
{
    int32 ret;
    uint32                  trk_gid;
    uint32                  first_trunkMember;
    rtk_portmask_t          trunk_portmask;
    dal_maple_l2_entry_t  l2_entry;
    dal_maple_l2_index_t  index_entry;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, vid=%d", unit, pL2_data->vid);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(NULL == pL2_data, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pL2_data->mac.octet[0] & BITMASK_1B) != 0, RT_ERR_MAC);
    RT_PARAM_CHK((pL2_data->vid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pMac=%x-%x-%x-%x-%x-%x",
           pL2_data->mac.octet[0], pL2_data->mac.octet[1], pL2_data->mac.octet[2],
           pL2_data->mac.octet[3], pL2_data->mac.octet[4], pL2_data->mac.octet[5]);

    /* search exist or free entry */
    osal_memset(&l2_entry, 0, sizeof(dal_maple_l2_entry_t));
    l2_entry.entry_type     = L2_UNICAST;
    l2_entry.unicast.fid    = pL2_data->vid;
    osal_memcpy(&l2_entry.unicast.mac, &pL2_data->mac, sizeof(rtk_mac_t));

    L2_SEM_LOCK(unit);

    if ((ret = _dal_maple_l2_getExistOrFreeL2Entry(unit, &l2_entry, L2_GET_EXIST_ONLY, &index_entry)) != RT_ERR_OK)
    {
        /* Return Fail if not found */
        L2_SEM_UNLOCK(unit);
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    /* fill content */
    osal_memcpy(&pL2_data->mac, &l2_entry.unicast.mac, sizeof(rtk_mac_t));
    pL2_data->vid       = l2_entry.unicast.fid;
    pL2_data->port      = l2_entry.unicast.port;
    pL2_data->flags     = 0;    /* Clear data */
    pL2_data->state     = 0;    /* Clear data */

    for (trk_gid = 0; trk_gid < HAL_MAX_NUM_OF_TRUNK(unit); trk_gid++)
    {
        if (dal_maple_trunk_port_get(unit, trk_gid, &trunk_portmask) == RT_ERR_OK)
        {
            if ((first_trunkMember = RTK_PORTMASK_GET_FIRST_PORT(trunk_portmask)) == -1)
            {
                /* no trunk member */
                continue;
            }

            if ((RTK_PORTMASK_IS_PORT_SET(trunk_portmask, pL2_data->port)) &&
                (first_trunkMember == pL2_data->port))
            {
                pL2_data->trk_gid = trk_gid;
                pL2_data->flags |= RTK_L2_UCAST_FLAG_TRUNK_PORT;
                break;
            }
        }
    }

    if(l2_entry.unicast.sablock)
        pL2_data->flags = RTK_L2_UCAST_FLAG_SA_BLOCK;
    if(l2_entry.unicast.dablock)
        pL2_data->flags |= RTK_L2_UCAST_FLAG_DA_BLOCK;
    if(l2_entry.unicast.is_static)
        pL2_data->flags |= RTK_L2_UCAST_FLAG_STATIC;
    if(l2_entry.unicast.nh)
        pL2_data->flags |= RTK_L2_UCAST_FLAG_NEXTHOP;
    if(l2_entry.unicast.suspending)
        pL2_data->state |= RTK_L2_UCAST_STATE_SUSPEND;
    if(l2_entry.unicast.aging == 0)
        pL2_data->isAged = TRUE;
    else
        pL2_data->isAged = FALSE;
    pL2_data->vlan_target   = l2_entry.unicast.vlan_target;
    pL2_data->route_idx     = l2_entry.unicast.route_idx;
    pL2_data->agg_vid       = l2_entry.unicast.agg_vid;

    if(index_entry.index_type == L2_IN_HASH)
        pL2_data->l2_idx = (index_entry.index << 2) | index_entry.hashdepth;
    else
        pL2_data->l2_idx = hashTable_size[unit] + index_entry.index;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "vid=%d, port=%d, sa_block=%d, da_block=%d, is_static=%d, nh=%d, suspend=%d, agg_vid=:%d",
        pL2_data->vid, pL2_data->port, l2_entry.unicast.sablock, l2_entry.unicast.dablock, l2_entry.unicast.is_static,
        l2_entry.unicast.nh, l2_entry.unicast.suspending, l2_entry.unicast.agg_vid);

    return RT_ERR_OK;
} /* end of dal_maple_l2_addr_get */

/* Function Name:
 *      dal_maple_l2_addr_set
 * Description:
 *      Update content of L2 entry.
 * Input:
 *      unit     - unit id
 *      pL2_addr - L2 entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_VLAN_VID     - invalid vlan id
 *      RT_ERR_MAC          - invalid mac address
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_INPUT        - invalid input parameter
 * Note:
 *      None
 */
int32
dal_maple_l2_addr_set(uint32 unit, rtk_l2_ucastAddr_t *pL2_addr)
{
    int32               ret;
    dal_maple_l2_entry_t  l2_entry;
    dal_maple_l2_index_t  index_entry;
    rtk_portmask_t      trunk_portmask;
    uint32              trk_gid;
    uint32              first_trunkMember;
    rtk_port_t          lPort;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, pL2_addr=%x", unit, pL2_addr);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    if (pL2_addr->port == L2_PORT_DONT_CARE)
        lPort = PORT_DONT_CARE_8380;
    else
        lPort = pL2_addr->port;

    /* parameter check */
    RT_PARAM_CHK(NULL == pL2_addr, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pL2_addr->mac.octet[0] & BITMASK_1B) != 0, RT_ERR_MAC);
    if (pL2_addr->flags & RTK_L2_UCAST_FLAG_TRUNK_PORT)
        RT_PARAM_CHK((pL2_addr->trk_gid >= HAL_MAX_NUM_OF_TRUNK(unit)), RT_ERR_TRUNK_ID);
    else
    {
        if (((pL2_addr->flags & RTK_L2_UCAST_FLAG_SA_BLOCK) || (pL2_addr->flags & RTK_L2_UCAST_FLAG_DA_BLOCK) || (pL2_addr->flags & RTK_L2_UCAST_FLAG_NEXTHOP)) && !(pL2_addr->flags & RTK_L2_UCAST_FLAG_STATIC))
            RT_PARAM_CHK((!HWP_PORT_EXIST(unit, lPort) && lPort != PORT_DONT_CARE_8380), RT_ERR_PORT_ID);
        else
            RT_PARAM_CHK((!HWP_PORT_EXIST(unit, lPort)), RT_ERR_PORT_ID);
    }
    RT_PARAM_CHK((pL2_addr->vid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    RT_PARAM_CHK(((pL2_addr->flags & RTK_L2_UCAST_FLAG_NEXTHOP) && pL2_addr->vlan_target >= 2 &&
                   pL2_addr->route_idx >= HAL_MAX_NUM_OF_ROUTE_HOST_ADDR(unit)), RT_ERR_INPUT);

    RT_PARAM_CHK(((pL2_addr->add_op_flags & RTK_L2_ADD_OP_FLAG_AGE_SET) && pL2_addr->age>3), RT_ERR_INPUT);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2)
            , "unit=%d, vid=%d, port=%d, flags=%x, trk_gid=%d, state=%d, vlan_target =%d, route_idx=%d"
            , unit, pL2_addr->vid, pL2_addr->port, pL2_addr->flags
            , pL2_addr->trk_gid, pL2_addr->state, pL2_addr->vlan_target, pL2_addr->route_idx);

    if((pL2_addr->add_op_flags & RTK_L2_ADD_OP_FLAG_AGE_SET) && pL2_addr->age==0)
    {
        if  (
                (pL2_addr->flags & RTK_L2_UCAST_FLAG_STATIC)==0 &&
                (pL2_addr->flags & RTK_L2_UCAST_FLAG_SA_BLOCK) == 0 &&
                (pL2_addr->flags & RTK_L2_UCAST_FLAG_DA_BLOCK) == 0 &&
                (pL2_addr->flags & RTK_L2_UCAST_FLAG_NEXTHOP) == 0
            )
            return RT_ERR_INPUT;
    }

    if (!(pL2_addr->flags & RTK_L2_UCAST_FLAG_TRUNK_PORT))
    {
        for (trk_gid = 0; trk_gid < HAL_MAX_NUM_OF_TRUNK(unit); trk_gid++)
        {
            if (dal_maple_trunk_port_get(unit, trk_gid, &trunk_portmask) == RT_ERR_OK)
            {
                if ((first_trunkMember = RTK_PORTMASK_GET_FIRST_PORT(trunk_portmask)) == -1)
                {
                    /* no trunk member */
                    continue;
                }

                if ((RTK_PORTMASK_IS_PORT_SET(trunk_portmask, pL2_addr->port)) &&
                    !(first_trunkMember == pL2_addr->port))
                {
                    /* this port is trunk member and not first trunk member, not allow to add */
                    /* don't print error here because this is a easy-to-happen error that occurs when a large number of L2 entry is modifying and LACP changing members */
                    return RT_ERR_PORT_ID;
                }
            }
        }
    }
    else
    {
        RT_PARAM_CHK(pL2_addr->trk_gid >= HAL_MAX_NUM_OF_TRUNK(unit), RT_ERR_LA_TRUNK_ID);
        /* Transfer pL2_addr->trk_gid to represent port and update pL2_addr->port */
        if ((ret = dal_maple_trunk_port_get(unit, pL2_addr->trk_gid, &trunk_portmask)) != RT_ERR_OK)
        {
            RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        if ((ret = RTK_PORTMASK_GET_FIRST_PORT(trunk_portmask)) == -1)
        {
            /* no trunk member */
            ret = RT_ERR_LA_TRUNK_ID;
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return RT_ERR_LA_TRUNK_ID;
        }
        lPort = (uint32)ret;
    }

    /* search exist or free entry */
    l2_entry.entry_type = L2_UNICAST;
    l2_entry.unicast.fid = pL2_addr->vid;
    osal_memcpy(&l2_entry.unicast.mac, &(pL2_addr->mac), sizeof(rtk_mac_t));

    L2_SEM_LOCK(unit);

    if ((ret = _dal_maple_l2_getExistOrFreeL2Entry(unit, &l2_entry, L2_GET_EXIST_ONLY, &index_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "ret=%d", ret);
        return ret;
    }

    /* fill content */
    osal_memcpy(&l2_entry.unicast.mac, &(pL2_addr->mac), sizeof(rtk_mac_t));
    l2_entry.unicast.fid        = pL2_addr->vid;
    l2_entry.unicast.port       = lPort;
    l2_entry.unicast.aging      = (pL2_addr->add_op_flags & RTK_L2_ADD_OP_FLAG_AGE_SET) ? pL2_addr->age : 3;
    l2_entry.unicast.sablock    = (pL2_addr->flags & RTK_L2_UCAST_FLAG_SA_BLOCK) ? TRUE: FALSE;
    l2_entry.unicast.dablock    = (pL2_addr->flags & RTK_L2_UCAST_FLAG_DA_BLOCK) ? TRUE: FALSE;
    l2_entry.unicast.is_static  = (pL2_addr->flags & RTK_L2_UCAST_FLAG_STATIC) ? TRUE: FALSE;
    l2_entry.unicast.nh         = (pL2_addr->flags & RTK_L2_UCAST_FLAG_NEXTHOP) ? TRUE : FALSE;
    l2_entry.unicast.suspending = (pL2_addr->state & RTK_L2_UCAST_STATE_SUSPEND) ? TRUE: FALSE;

    l2_entry.unicast.vlan_target = pL2_addr->vlan_target;
    l2_entry.unicast.route_idx   = pL2_addr->route_idx;
    l2_entry.unicast.agg_vid     = pL2_addr->agg_vid;


    if(l2_entry.unicast.aging==0 && l2_entry.unicast.is_static==0)
    {
        l2_entry.unicast.port       = PORT_DONT_CARE_8380;
        l2_entry.unicast.suspending = 0;
        l2_entry.unicast.agg_vid    = 0;
    }

    if (L2_IN_HASH == index_entry.index_type)
    {
        /* if found entry is in HASH, programming in HASH */
        ret = _dal_maple_l2_setL2HASHEntry(unit, &l2_entry, &index_entry);
        /* return the entry index */
        pL2_addr->l2_idx = (index_entry.index << 2) | index_entry.hashdepth;
    }
    else if (L2_IN_CAM == index_entry.index_type)
    {
        /* if found entry is in CAM, programming in CAM */
        ret = _dal_maple_l2_setL2CAMEntry(unit, &l2_entry, &index_entry);
        pL2_addr->l2_idx = hashTable_size[unit] + index_entry.index;
    }
    else
    {
        ret = RT_ERR_FAILED;
    }

    L2_SEM_UNLOCK(unit);

    return ret;
}/* end of dal_maple_l2_addr_set */

/* Function Name:
 *      dal_maple_l2_addr_delAll
 * Description:
 *      Delete all L2 unicast address entry from the specified device.
 * Input:
 *      unit           - unit id
 *      include_static - include static mac or not?
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 * Note:
 *      None
 */
int32
dal_maple_l2_addr_delAll(uint32 unit, uint32 include_static)
{
    int32   ret;
    uint32  zero = 0;
    uint32  value, data;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, include_static=%d", unit, include_static);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((include_static > 1), RT_ERR_INPUT);

    L2_SEM_LOCK(unit);

    /* get data from CHIP*/
    data = 0;
    if ((ret = reg_read(unit, MAPLE_L2_TBL_FLUSH_CTRLr, &data)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if ((ret = reg_field_set(unit, MAPLE_L2_TBL_FLUSH_CTRLr, MAPLE_FVID_CMPf, &zero, &data)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if ((ret = reg_field_set(unit, MAPLE_L2_TBL_FLUSH_CTRLr, MAPLE_PORT_CMPf, &zero, &data)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if ((ret = reg_field_set(unit, MAPLE_L2_TBL_FLUSH_CTRLr, MAPLE_FVIDf, &zero, &data)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if ((ret = reg_field_set(unit, MAPLE_L2_TBL_FLUSH_CTRLr, MAPLE_PORTf, &zero, &data)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    value = include_static ? 0 : 1;  /* 0:whole table, 1:dynamic only */
    if ((ret = reg_field_set(unit, MAPLE_L2_TBL_FLUSH_CTRLr, MAPLE_ENTRY_TYPEf, &value, &data)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    value = 0;  /* flush */
    if ((ret = reg_field_set(unit, MAPLE_L2_TBL_FLUSH_CTRLr, MAPLE_ACTf, &value, &data)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    value = 1;  /* start */
    if ((ret = reg_field_set(unit, MAPLE_L2_TBL_FLUSH_CTRLr, MAPLE_STSf, &value, &data)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    /* program value to CHIP*/
    if ((ret = reg_write(unit, MAPLE_L2_TBL_FLUSH_CTRLr, &data)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    while(1)
    {
        if ((ret = reg_field_read(unit, MAPLE_L2_TBL_FLUSH_CTRLr, MAPLE_STSf, &data)) != RT_ERR_OK)
        {
            L2_SEM_UNLOCK(unit);
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }
        else if(data == 0)
            break;
    }

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_addr_delAll */

/* Function Name:
 *      dal_maple_l2_nextValidAddr_get
 * Description:
 *      Get next valid L2 unicast address entry from the specified device.
 * Input:
 *      unit           - unit id
 *      pScan_idx      - currently scan index of l2 table to get next.
 *      include_static - the get type, include static mac or not.
 * Output:
 *      pL2_data       - structure of l2 address data
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT          - The module is not initial
 *      RT_ERR_VLAN_VID          - invalid vid
 *      RT_ERR_MAC               - invalid mac address
 *      RT_ERR_NULL_POINTER      - input parameter may be null pointer
 *      RT_ERR_L2_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      1) The function will skip valid l2 multicast and ip multicast entry and
 *         reply next valid L2 unicast address is based on index order of l2 table.
 *      2) Please input -1 for get the first entry of l2 table.
 *      3) The *pScan_idx is the input and also is the output argument.
 */
int32
dal_maple_l2_nextValidAddr_get(
    uint32              unit,
    int32               *pScan_idx,
    uint32              include_static,
    rtk_l2_ucastAddr_t  *pL2_data)
{
    int32  ret;
    dal_maple_l2_entry_t  l2_entry;
    uint32                  trk_gid;
    uint32                  first_trunkMember;
    rtk_portmask_t          trunk_portmask;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((NULL == pScan_idx), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((include_static > 1), RT_ERR_INPUT);
    RT_PARAM_CHK((NULL == pL2_data), RT_ERR_NULL_POINTER);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, pScan_idx=%d, include_static=%d", unit, *pScan_idx, include_static);

    osal_memset(pL2_data, 0, sizeof(rtk_l2_ucastAddr_t));
    osal_memset(&trunk_portmask, 0, sizeof(rtk_portmask_t));

    L2_SEM_LOCK(unit);

    if ((ret = _dal_maple_l2_nextValidAddr_get(unit, pScan_idx, L2_UNICAST, include_static, &l2_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    osal_memcpy(&pL2_data->mac, &l2_entry.unicast.mac, sizeof(rtk_mac_t));
    pL2_data->vid       = l2_entry.unicast.fid;
    pL2_data->port      = l2_entry.unicast.port;
    pL2_data->age       = l2_entry.unicast.aging;

    if(l2_entry.unicast.nh == TRUE)
    {
        pL2_data->vlan_target = l2_entry.unicast.vlan_target;
        pL2_data->route_idx = l2_entry.unicast.route_idx;
    }
    else
        pL2_data->agg_vid = l2_entry.unicast.agg_vid;

    for (trk_gid = 0; trk_gid < HAL_MAX_NUM_OF_TRUNK(unit); trk_gid++)
    {
        if (dal_maple_trunk_port_get(unit, trk_gid, &trunk_portmask) == RT_ERR_OK)
        {
            if ((first_trunkMember = RTK_PORTMASK_GET_FIRST_PORT(trunk_portmask)) == -1)
            {
                /* no trunk member */
                continue;
            }

            if ((RTK_PORTMASK_IS_PORT_SET(trunk_portmask, pL2_data->port)) &&
                (first_trunkMember == pL2_data->port))
            {
                pL2_data->trk_gid = trk_gid;
                pL2_data->flags |= RTK_L2_UCAST_FLAG_TRUNK_PORT;
                break;
            }
        }
    }

    if(l2_entry.unicast.sablock)
        pL2_data->flags = RTK_L2_UCAST_FLAG_SA_BLOCK;
    if(l2_entry.unicast.dablock)
        pL2_data->flags |= RTK_L2_UCAST_FLAG_DA_BLOCK;
    if(l2_entry.unicast.is_static)
        pL2_data->flags |= RTK_L2_UCAST_FLAG_STATIC;
    if(l2_entry.unicast.nh)
        pL2_data->flags |= RTK_L2_UCAST_FLAG_NEXTHOP;
    if(l2_entry.unicast.suspending)
        pL2_data->state |= RTK_L2_UCAST_STATE_SUSPEND;
    if(l2_entry.unicast.aging == 0)
        pL2_data->isAged = TRUE;
    else
        pL2_data->isAged = FALSE;
    pL2_data->l2_idx = *pScan_idx;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "vid=%d, port=%d, sa_block=%d, da_block=%d, is_static=%d, nh=%d, suspend=%d",
        pL2_data->vid, pL2_data->port, l2_entry.unicast.sablock, l2_entry.unicast.dablock, l2_entry.unicast.is_static, l2_entry.unicast.nh, l2_entry.unicast.suspending);

    return RT_ERR_OK;
} /* end of dal_maple_l2_nextValidAddr_get */


/* Module Name    : L2           */
/* Sub-module Name: l2 multicast */

/* Function Name:
 *      dal_maple_l2_mcastAddr_init
 * Description:
 *      Initialize content of buffer of L2 multicast entry.
 *      Will fill vid ,MAC address and reset other field of L2 multicast entry.
 * Input:
 *      unit        - unit id
 *      vid         - vlan id
 *      pMac        - MAC address
 *      pMcast_addr - L2 multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT         - The module is not initial
 *      RT_ERR_UNIT_ID          - invalid unit id
 *      RT_ERR_VLAN_VID         - invalid vlan id
 *      RT_ERR_MAC              - invalid mac address
 *      RT_ERR_NULL_POINTER     - input parameter may be null pointer
 *      RT_ERR_INPUT            - invalid input parameter
 * Note:
 *      Need to initialize L2 multicast entry before add it.
 */
int32
dal_maple_l2_mcastAddr_init(uint32 unit, rtk_vlan_t vid, rtk_mac_t *pMac, rtk_l2_mcastAddr_t *pMcast_addr)
{
    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    RT_PARAM_CHK(NULL == pMcast_addr, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK(NULL == pMac, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pMac->octet[0] & BITMASK_1B) != 1, RT_ERR_MAC);
    RT_PARAM_CHK((vid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);

    osal_memset(pMcast_addr, 0, sizeof(rtk_l2_mcastAddr_t));
    pMcast_addr->rvid = vid;
    osal_memcpy(&pMcast_addr->mac.octet[0], &pMac->octet[0], sizeof(rtk_mac_t));

    return RT_ERR_OK;
}

/* Function Name:
 *      dal_maple_l2_mcastAddr_add
 * Description:
 *      Add L2 multicast entry to ASIC.
 * Input:
 *      unit        - unit id
 *      pMcast_addr - L2 multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT    - The module is not initial
 *      RT_ERR_VLAN_VID    - invalid vlan id
 *      RT_ERR_MAC         - invalid mac address
 *      RT_ERR_NULL_POINTE - input parameter may be null pointer
 *      RT_ERR_INPUT       - invalid input parameter
 * Note:
 *      Need to initialize L2 multicast entry before add it.
 */
int32
dal_maple_l2_mcastAddr_add(uint32 unit, rtk_l2_mcastAddr_t *pMcast_addr)
{
    int32   ret;

    pMcast_addr->fwdIndex = -1; /* for automatically allocate */

    ret = dal_maple_l2_mcastAddr_addByIndex(unit, pMcast_addr);

    return ret;
} /* end of dal_maple_l2_mcastAddr_add */

/* Function Name:
 *      dal_maple_l2_mcastAddr_get
 * Description:
 *      Get L2 multicast entry based on specified vid and MAC address.
 * Input:
 *      unit        - unit id
 *      pMcast_addr - L2 multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_VLAN_VID     - invalid vlan id
 *      RT_ERR_MAC          - invalid mac address
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_INPUT        - invalid input parameter
 * Note:
 *      None
 */
int32
dal_maple_l2_mcastAddr_get(uint32 unit, rtk_l2_mcastAddr_t *pMcast_addr)
{
    int32 ret;
    dal_maple_l2_entry_t  l2_entry;
    dal_maple_l2_index_t  index_entry;
    multicast_index_entry_t mcast_entry;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dal_maple_l2_mcastAddr_get: unit=%d, vid=%d, agg_vid=%d, pMac=%x-%x-%x-%x-%x-%x",
           unit, pMcast_addr->rvid, pMcast_addr->agg_vid, pMcast_addr->mac.octet[0], pMcast_addr->mac.octet[1], pMcast_addr->mac.octet[2], pMcast_addr->mac.octet[3],
           pMcast_addr->mac.octet[4], pMcast_addr->mac.octet[5]);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(NULL == pMcast_addr, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pMcast_addr->mac.octet[0] & BITMASK_1B) != 1, RT_ERR_MAC);
    RT_PARAM_CHK((pMcast_addr->rvid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);

    /* search exist or free entry */
    l2_entry.entry_type     = L2_MULTICAST;
    l2_entry.l2mcast.rvid   = pMcast_addr->rvid;
    osal_memcpy(&l2_entry.l2mcast.mac, &(pMcast_addr->mac), sizeof(rtk_mac_t));

    ret = _dal_maple_l2_getExistOrFreeL2Entry(unit, &l2_entry, L2_GET_EXIST_ONLY, &index_entry);

    if (RT_ERR_OK != ret)
    {
        /* fill content */
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    if (index_entry.index_type == L2_IN_HASH)
        pMcast_addr->l2_idx = (index_entry.index << 2) | index_entry.hashdepth;
    else
        pMcast_addr->l2_idx = (1 << 13) + index_entry.index;

    pMcast_addr->fwdIndex = (int32)l2_entry.l2mcast.index;
    pMcast_addr->agg_vid = l2_entry.l2mcast.agg_vid;

    L2_SEM_LOCK(unit);
    if ((ret = table_read(unit, MAPLE_MC_PMSKt, l2_entry.l2mcast.index, (uint32 *)&mcast_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return RT_ERR_FAILED;
    }
    L2_SEM_UNLOCK(unit);

    if ((ret = table_field_get(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, &(pMcast_addr->portmask.bits[0]), (uint32 *) &mcast_entry)) != RT_ERR_OK)
    {
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return RT_ERR_FAILED;
    }

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dal_maple_l2_mcastAddr_get: pPortmask==0x%x", pMcast_addr->portmask.bits[0]);

    return ret;
} /* end of dal_maple_l2_mcastAddr_get */

/* Function Name:
 *      dal_maple_l2_mcastAddr_set
 * Description:
 *      Set a L2 multicast address entry to the specified device.
 * Input:
 *      unit        - unit id
 * Output:
 *      pMcast_addr - pointer to L2 multicast entry
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_VLAN_VID     - invalid vlan id
 *      RT_ERR_MAC          - invalid mac address
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
dal_maple_l2_mcastAddr_set(uint32 unit, rtk_l2_mcastAddr_t *pMcast_addr)
{
    int32 ret;
    dal_maple_l2_entry_t  l2_entry;
    dal_maple_l2_index_t  index_entry;
    multicast_index_entry_t mcast_entry;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(NULL == pMcast_addr, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pMcast_addr->mac.octet[0] & BITMASK_1B) != 1, RT_ERR_MAC);
    RT_PARAM_CHK((pMcast_addr->rvid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    RT_PARAM_CHK((pMcast_addr->agg_vid> RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    RT_PARAM_CHK(!HWP_PMSK_EXIST(unit, &pMcast_addr->portmask), RT_ERR_PORT_MASK);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dal_maple_l2_mcastAddr_set: unit=%d, vid=%d, agg_vid =%d, pMac=%x-%x-%x-%x-%x-%x \
           pPortmask==0x%x", unit, pMcast_addr->rvid, pMcast_addr->agg_vid , pMcast_addr->mac.octet[0], pMcast_addr->mac.octet[1], pMcast_addr->mac.octet[2],
           pMcast_addr->mac.octet[3], pMcast_addr->mac.octet[4], pMcast_addr->mac.octet[5], pMcast_addr->portmask.bits[0]);

    /* search exist or free entry */
    l2_entry.entry_type = L2_MULTICAST;
    l2_entry.l2mcast.rvid    = pMcast_addr->rvid;
    osal_memcpy(&l2_entry.l2mcast.mac.octet[0], &pMcast_addr->mac.octet[0], sizeof(rtk_mac_t));

    L2_SEM_LOCK(unit);

    ret = _dal_maple_l2_getExistOrFreeL2Entry(unit, &l2_entry, L2_GET_EXIST_ONLY, &index_entry);

    if (RT_ERR_OK != ret)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    osal_memset(&mcast_entry, 0, sizeof(mcast_entry));

    if ((ret = table_field_set(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, &pMcast_addr->portmask.bits[0], (uint32 *) &mcast_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return RT_ERR_FAILED;
    }

    if ((ret = table_write(unit, MAPLE_MC_PMSKt, l2_entry.l2mcast.index, (uint32 *)&mcast_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return RT_ERR_FAILED;
    }

    if (l2_entry.l2mcast.agg_vid  !=  pMcast_addr->agg_vid)
    {
        l2_entry.l2mcast.agg_vid = pMcast_addr->agg_vid;
        if (L2_IN_HASH == index_entry.index_type)
        {
            /* if found entry is in HASH, programming in CAM */
            ret = _dal_maple_l2_setL2HASHEntry(unit, &l2_entry, &index_entry);
        }
        else if(L2_IN_CAM == index_entry.index_type)
        {
            /* if found entry is in CAM, programming in CAM */
            ret = _dal_maple_l2_setL2CAMEntry(unit, &l2_entry, &index_entry);
        }
    }

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_mcastAddr_set */

/* Function Name:
 *      dal_maple_l2_mcastAddr_del
 * Description:
 *      Delete a L2 multicast address entry from the specified device.
 * Input:
 *      unit - unit id
 *      vid  - vlan id
 *      pMac - multicast mac address
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT       - The module is not initial
 *      RT_ERR_L2_HASH_KEY    - invalid L2 Hash key
 *      RT_ERR_L2_EMPTY_ENTRY - the entry is empty(invalid)
 * Note:
 *      None
 */
int32
dal_maple_l2_mcastAddr_del(uint32 unit, rtk_vlan_t vid, rtk_mac_t *pMac)
{
    int32 ret;
    dal_maple_l2_entry_t  l2_entry;
    dal_maple_l2_index_t  index_entry;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, vid=%d",
           unit, vid);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((vid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    RT_PARAM_CHK(NULL == pMac, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pMac->octet[0] & BITMASK_1B) != 1, RT_ERR_MAC);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pMac=%x-%x-%x-%x-%x-%x",
           pMac->octet[0], pMac->octet[1], pMac->octet[2],
           pMac->octet[3], pMac->octet[4], pMac->octet[5]);

    /* search exist or free entry */
    l2_entry.entry_type = L2_MULTICAST;
    l2_entry.l2mcast.rvid = vid;
    osal_memcpy(&l2_entry.l2mcast.mac, pMac, sizeof(rtk_mac_t));

    L2_SEM_LOCK(unit);

    ret = _dal_maple_l2_getExistOrFreeL2Entry(unit, &l2_entry, L2_GET_EXIST_ONLY, &index_entry);

    if (RT_ERR_OK != ret)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    _dal_maple_l2_freeMcastIdx(unit, (int32)l2_entry.l2mcast.index);

    osal_memset(&l2_entry.l2mcast.mac, 0, sizeof(rtk_mac_t));
    l2_entry.l2mcast.rvid    = 0;
    l2_entry.l2mcast.index   = 0;

    if (L2_IN_HASH == index_entry.index_type)
    {
        /* if found entry is in HASH, programming in HASH */
        ret = _dal_maple_l2_setL2HASHEntry(unit, &l2_entry, &index_entry);
    }
    else if (L2_IN_CAM == index_entry.index_type)
    {
        /* if found entry is in CAM, programming in CAM */
        ret = _dal_maple_l2_setL2CAMEntry(unit, &l2_entry, &index_entry);
    }
    else
    {
        ret = RT_ERR_FAILED;
    }

    L2_SEM_UNLOCK(unit);

    return ret;
} /* end of dal_maple_l2_mcastAddr_del */

/* Function Name:
 *      dal_maple_l2_mcastAddr_setByIndex
 * Description:
 *      Set a L2 multicast address entry to the specified device.
 * Input:
 *      unit        - unit id
 * Output:
 *      pMcast_addr - pointer to L2 multicast entry
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_VLAN_VID     - invalid vlan id
 *      RT_ERR_MAC          - invalid mac address
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_L2_MULTI_FWD_INDEX - invalid index of multicast forwarding portmask
 * Note:
 *      None
 */
int32
dal_maple_l2_mcastAddr_setByIndex(uint32 unit, rtk_l2_mcastAddr_t *pMcast_addr)
{
    int32 ret;
    dal_maple_l2_entry_t  l2_entry;
    dal_maple_l2_index_t  index_entry;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(NULL == pMcast_addr, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pMcast_addr->mac.octet[0] & BITMASK_1B) != 1, RT_ERR_MAC);
    RT_PARAM_CHK((pMcast_addr->rvid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    RT_PARAM_CHK((pMcast_addr->agg_vid> RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    RT_PARAM_CHK(pMcast_addr->fwdIndex >= (int32)mcast_idx_pool[unit].size_of_mcast_fwd_index, RT_ERR_L2_MULTI_FWD_INDEX);
    //RT_PARAM_CHK(!HWP_PMSK_EXIST(unit, &pMcast_addr->portmask), RT_ERR_PORT_MASK);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dal_maple_l2_mcastAddr_set: unit=%d, vid=%d, agg_vid =%d, pMac=%x-%x-%x-%x-%x-%x \
           pPortmask==0x%x", unit, pMcast_addr->rvid, pMcast_addr->agg_vid , pMcast_addr->mac.octet[0], pMcast_addr->mac.octet[1], pMcast_addr->mac.octet[2],
           pMcast_addr->mac.octet[3], pMcast_addr->mac.octet[4], pMcast_addr->mac.octet[5], pMcast_addr->portmask.bits[0]);

    /* search exist or free entry */
    l2_entry.entry_type = L2_MULTICAST;
    l2_entry.l2mcast.rvid    = pMcast_addr->rvid;
    osal_memcpy(&l2_entry.l2mcast.mac.octet[0], &pMcast_addr->mac.octet[0], sizeof(rtk_mac_t));

    L2_SEM_LOCK(unit);

    ret = _dal_maple_l2_getExistOrFreeL2Entry(unit, &l2_entry, L2_GET_EXIST_ONLY, &index_entry);

    if (RT_ERR_OK != ret)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    if ((l2_entry.l2mcast.agg_vid != pMcast_addr->agg_vid) || (l2_entry.l2mcast.index != pMcast_addr->fwdIndex))
    {
        l2_entry.l2mcast.agg_vid = pMcast_addr->agg_vid;
        l2_entry.l2mcast.index = pMcast_addr->fwdIndex;
        if (L2_IN_HASH == index_entry.index_type)
        {
            /* if found entry is in HASH, programming in CAM */
            ret = _dal_maple_l2_setL2HASHEntry(unit, &l2_entry, &index_entry);
        }
        else if(L2_IN_CAM == index_entry.index_type)
        {
            /* if found entry is in CAM, programming in CAM */
            ret = _dal_maple_l2_setL2CAMEntry(unit, &l2_entry, &index_entry);
        }
    }

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_mcastAddr_setByIndex */


/* Function Name:
 *      dal_maple_l2_mcastAddr_addByIndex
 * Description:
 *      Add a L2 multicast address entry and multicast index to the specified device.
 * Input:
 *      unit                  - unit id
 *      pMcast_addr           - content of L2 multicast address entry
 * Output:
 *      pMcast_addr->fwdIndex - index of multicast forwarding entry
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT           - The module is not initial
 *      RT_ERR_L2_HASH_KEY        - invalid L2 Hash key
 *      RT_ERR_NULL_POINTER       - input parameter may be null pointer
 *      RT_ERR_L2_MULTI_FWD_INDEX - invalid index of multicast forwarding portmask
 * Note:
 *      If fwdIndex is larger than or equal to 0, will use fwdIndex as multicast index
 *          and won't config portmask.
 *
 *      If fwdIndex is smaller than 0, will allocate a free index and return it.
 *          It will also config portmask.
 */
int32
dal_maple_l2_mcastAddr_addByIndex(uint32 unit, rtk_l2_mcastAddr_t *pMcast_addr)
{
    int32 ret;
    int32 fwdIndex;
    dal_maple_l2_entry_t  l2_entry, dynamic_l2_entry;
    dal_maple_l2_index_t  index_entry;
    multicast_index_entry_t mcast_entry;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(NULL == pMcast_addr, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pMcast_addr->mac.octet[0] & BITMASK_1B) != 1, RT_ERR_MAC);
    RT_PARAM_CHK(pMcast_addr->fwdIndex >= (int32)mcast_idx_pool[unit].size_of_mcast_fwd_index, RT_ERR_L2_MULTI_FWD_INDEX);
    RT_PARAM_CHK((pMcast_addr->rvid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    RT_PARAM_CHK((pMcast_addr->agg_vid> RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    RT_PARAM_CHK(!HWP_PMSK_EXIST(unit, &pMcast_addr->portmask), RT_ERR_PORT_MASK);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dal_maple_l2_mcastAddr_addByIndex: unit=%d, rvid=%d, agg_vid=%d, pMac=%x-%x-%x-%x-%x-%x \
           pPortmask==0x%x, pIndex=%d", unit, pMcast_addr->rvid, pMcast_addr->agg_vid, pMcast_addr->mac.octet[0], pMcast_addr->mac.octet[1], pMcast_addr->mac.octet[2],
           pMcast_addr->mac.octet[3], pMcast_addr->mac.octet[4], pMcast_addr->mac.octet[5], pMcast_addr->portmask.bits[0], pMcast_addr->fwdIndex);

    /* search exist or free entry */
    l2_entry.entry_type = L2_MULTICAST;
    l2_entry.l2mcast.rvid = pMcast_addr->rvid;
    osal_memcpy(&l2_entry.l2mcast.mac, &(pMcast_addr->mac), sizeof(rtk_mac_t));

    L2_SEM_LOCK(unit);

    ret = _dal_maple_l2_getExistOrFreeL2Entry(unit, &l2_entry, L2_GET_EXIST_OR_FREE, &index_entry);
    if (ret == RT_ERR_L2_NO_EMPTY_ENTRY && (pMcast_addr->add_op_flags & RTK_L2_ADD_OP_FLAG_REPLACE_DYNAMIC) != 0)
    {
        osal_memcpy(&dynamic_l2_entry, &l2_entry, sizeof(dal_maple_l2_entry_t));
        dynamic_l2_entry.entry_type  = L2_MULTICAST;

        ret = _dal_maple_l2_getFirstDynamicEntry(unit, &dynamic_l2_entry, &index_entry);
        if (ret)
        {
            L2_SEM_UNLOCK(unit);
            RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
            if (RT_ERR_L2_ENTRY_NOTFOUND == ret)
                return RT_ERR_L2_NO_EMPTY_ENTRY;
            else
                return ret;
        }
    }
    else if (RT_ERR_OK != ret)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    if (TRUE == l2_entry.is_entry_exist)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "entry exist");
        return RT_ERR_L2_ENTRY_EXIST;
    }

    if (index_entry.index_type == L2_IN_HASH)
        pMcast_addr->l2_idx = (index_entry.index << 2) | index_entry.hashdepth;
    else
        pMcast_addr->l2_idx = (1 << 13) + index_entry.index;

    fwdIndex = pMcast_addr->fwdIndex;

    /* get a free multicast index */
    if ((ret = _dal_maple_l2_allocMcastIdx(unit, &(pMcast_addr->fwdIndex))) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    osal_memset(&mcast_entry, 0, sizeof(mcast_entry));

    if (fwdIndex < 0)
    { /* Will configure portmask when fwdIndex is allocate automatically */
        if ((ret = table_field_set(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, &(pMcast_addr->portmask.bits[0]), (uint32 *) &mcast_entry)) != RT_ERR_OK)
        {
            L2_SEM_UNLOCK(unit);
            RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
            return RT_ERR_FAILED;
        }

        if ((ret = table_write(unit, MAPLE_MC_PMSKt, (uint32)pMcast_addr->fwdIndex, (uint32 *)&mcast_entry)) != RT_ERR_OK)
        {
            L2_SEM_UNLOCK(unit);
            RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
            return RT_ERR_FAILED;
        }
    }

    /* fill content */
    osal_memcpy(&l2_entry.l2mcast.mac, &(pMcast_addr->mac), sizeof(rtk_mac_t));
    l2_entry.l2mcast.rvid    = pMcast_addr->rvid;
    l2_entry.l2mcast.index   = (uint32)(pMcast_addr->fwdIndex);
    l2_entry.l2mcast.agg_vid = pMcast_addr->agg_vid;

    if (L2_IN_HASH == index_entry.index_type)
    {
        /* if found entry is in HASH, programming in CAM */
        ret = _dal_maple_l2_setL2HASHEntry(unit, &l2_entry, &index_entry);
    }
    else if(L2_IN_CAM == index_entry.index_type)
    {
        /* if found entry is in CAM, programming in CAM */
        ret = _dal_maple_l2_setL2CAMEntry(unit, &l2_entry, &index_entry);
    }

    if (RT_ERR_OK != ret)
    {
        _dal_maple_l2_freeMcastIdx(unit, (int32)l2_entry.l2mcast.index);
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_mcastAddr_addByIndex */

/* Function Name:
 *      dal_maple_l2_mcastAddr_delIgnoreIndex
 * Description:
 *      Delete a L2 multicast address entry from the specified device.
 * Input:
 *      unit - unit id
 *      vid  - vlan id
 *      pMac - multicast mac address
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT       - The module is not initial
 *      RT_ERR_L2_HASH_KEY    - invalid L2 Hash key
 *      RT_ERR_L2_EMPTY_ENTRY - the entry is empty(invalid)
 * Note:
 *      None
 */
int32
dal_maple_l2_mcastAddr_delIgnoreIndex(uint32 unit, rtk_vlan_t vid, rtk_mac_t *pMac)
{
    int32 ret;
    dal_maple_l2_entry_t  l2_entry;
    dal_maple_l2_index_t  index_entry;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, vid=%d",
           unit, vid);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((vid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    RT_PARAM_CHK(NULL == pMac, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pMac->octet[0] & BITMASK_1B) != 1, RT_ERR_MAC);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pMac=%x-%x-%x-%x-%x-%x",
           pMac->octet[0], pMac->octet[1], pMac->octet[2],
           pMac->octet[3], pMac->octet[4], pMac->octet[5]);

    /* search exist or free entry */
    l2_entry.entry_type = L2_MULTICAST;
    l2_entry.l2mcast.rvid = vid;
    osal_memcpy(&l2_entry.l2mcast.mac, pMac, sizeof(rtk_mac_t));

    L2_SEM_LOCK(unit);

    ret = _dal_maple_l2_getExistOrFreeL2Entry(unit, &l2_entry, L2_GET_EXIST_ONLY, &index_entry);

    if (RT_ERR_OK != ret)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    osal_memset(&l2_entry.l2mcast.mac, 0, sizeof(rtk_mac_t));
    l2_entry.l2mcast.rvid    = 0;
    l2_entry.l2mcast.index   = 0;

    if (L2_IN_HASH == index_entry.index_type)
    {
        /* if found entry is in HASH, programming in HASH */
        ret = _dal_maple_l2_setL2HASHEntry(unit, &l2_entry, &index_entry);
    }
    else if (L2_IN_CAM == index_entry.index_type)
    {
        /* if found entry is in CAM, programming in CAM */
        ret = _dal_maple_l2_setL2CAMEntry(unit, &l2_entry, &index_entry);
    }
    else
    {
        ret = RT_ERR_FAILED;
    }

    L2_SEM_UNLOCK(unit);

    return ret;
} /* end of dal_maple_l2_mcastAddr_delIgnoreIndex */


/* Function Name:
 *      dal_maple_l2_nextValidMcastAddr_get
 * Description:
 *      Get next valid L2 multicast address entry from the specified device.
 * Input:
 *      unit      - unit id
 *      pScan_idx - currently scan index of l2 table to get next.
 * Output:
 *      pL2_data  - structure of l2 address data
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT          - The module is not initial
 *      RT_ERR_VLAN_VID          - invalid vid
 *      RT_ERR_MAC               - invalid mac address
 *      RT_ERR_NULL_POINTER      - input parameter may be null pointer
 *      RT_ERR_L2_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      1. The function will skip valid l2 unicast and ip multicast entry and
 *         reply next valid L2 multicast address is based on index order of l2 table.
 *      2. Please input -1 for get the first entry of l2 table.
 *      3. The *pScan_idx is the input and also is the output argument.
 */
int32
dal_maple_l2_nextValidMcastAddr_get(
    uint32              unit,
    int32               *pScan_idx,
    rtk_l2_mcastAddr_t  *pL2_data)
{
    int32  ret;
    dal_maple_l2_entry_t  l2_entry;
    multicast_index_entry_t mcast_entry;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((NULL == pScan_idx), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == pL2_data), RT_ERR_NULL_POINTER);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, pScan_idx=%d", unit, *pScan_idx);

    L2_SEM_LOCK(unit);
    if ((ret = _dal_maple_l2_nextValidAddr_get(unit, pScan_idx, L2_MULTICAST, FALSE, &l2_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        return ret;
    }

    pL2_data->rvid = l2_entry.l2mcast.rvid;
    osal_memcpy(&pL2_data->mac, &l2_entry.l2mcast.mac, sizeof(rtk_mac_t));
    pL2_data->agg_vid = l2_entry.l2mcast.agg_vid;
    pL2_data->fwdIndex = l2_entry.l2mcast.index;

    osal_memset(&mcast_entry, 0, sizeof(mcast_entry));
    if ((ret = table_read(unit, MAPLE_MC_PMSKt, l2_entry.l2mcast.index, (uint32 *)&mcast_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if ((ret = table_field_get(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, pL2_data->portmask.bits, (uint32 *) &mcast_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    pL2_data->l2_idx = *pScan_idx;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, pScan_idx=%d, rvid=%d, agg_vid=%d, mac=%x-%x-%x-%x-%x-%x, portmask==0x%x",
           unit, *pScan_idx, pL2_data->rvid, pL2_data->agg_vid, pL2_data->mac.octet[0], pL2_data->mac.octet[1], pL2_data->mac.octet[2], pL2_data->mac.octet[3],
           pL2_data->mac.octet[4], pL2_data->mac.octet[5], pL2_data->portmask.bits[0]);

    return RT_ERR_OK;
} /* end of dal_maple_l2_nextValidMcastAddr_get */


/* Module Name    : L2           */
/* Sub-module Name: ip multicast */

/* Function Name:
 *      dal_maple_l2_ipmcMode_get
 * Description:
 *      Get lookup mode of layer2 ip multicast switching.
 * Input:
 *      unit  - unit id
 * Output:
 *      pMode - pointer to lookup mode of layer2 ip multicast switching
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Lookup mode of layer2 ip multicast switching for RTL8380 is as following
 *      - LOOKUP_ON_FVID_AND_MAC
 *      - LOOKUP_ON_DIP_AND_SIP
 *      - LOOKUP_ON_DIP_AND_FVID
 */
int32
dal_maple_l2_ipmcMode_get(uint32 unit, rtk_l2_ipmcMode_t *pMode)
{
    int32   ret;
    uint32  format = 0;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d", unit);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((NULL == pMode), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if ((ret = reg_field_read(unit, MAPLE_L2_CTRL_0r, MAPLE_IPV4_MC_HASH_KEY_FMTf, &format)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    switch (format)
    {
        case 0:
            *pMode = LOOKUP_ON_FVID_AND_MAC;
            break;
        case 1:
            *pMode = LOOKUP_ON_DIP_AND_SIP;
            break;
        case 2:
            *pMode = LOOKUP_ON_DIP_AND_FVID;
            break;
        default:
            return RT_ERR_FAILED;
    }

    ip4HashFmt[unit] = *pMode;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pMode=%ld", *pMode);

    return RT_ERR_OK;
} /* end of dal_maple_l2_ipmcMode_get */

/* Function Name:
 *      dal_maple_l2_ipmcMode_set
 * Description:
 *      Set lookup mode of layer2 ip multicast switching.
 * Input:
 *      unit - unit id
 *      mode - lookup mode of layer2 ip multicast switching
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      Lookup mode of layer2 ip multicast switching for RTL8380 is as following
 *      - LOOKUP_ON_FVID_AND_MAC
 *      - LOOKUP_ON_DIP_AND_SIP
 *      - LOOKUP_ON_DIP_AND_FVID
 */
int32
dal_maple_l2_ipmcMode_set(uint32 unit, rtk_l2_ipmcMode_t mode)
{
    int32   ret;
    uint32  value;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, mode=%d", unit, mode);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(mode >= IPMC_MODE_END, RT_ERR_INPUT);

    switch (mode)
    {
        case LOOKUP_ON_FVID_AND_MAC:
            value = 0;
            break;
        case LOOKUP_ON_DIP_AND_SIP:
            value = 1;
            break;
        case LOOKUP_ON_DIP_AND_FVID:
            value = 2;
            break;
        default:
            return RT_ERR_INPUT;
    }

    L2_SEM_LOCK(unit);

    if((ret = reg_field_write(unit, MAPLE_L2_CTRL_0r, MAPLE_IPV4_MC_HASH_KEY_FMTf, &value)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    ip4HashFmt[unit] = value;

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
}/*end of dal_maple_l2_ipmcMode_set*/

/* Function Name:
 *      dal_maple_l2_ipMcastAddrExt_init
 * Description:
 *      Initialize content of buffer of IP multicast entry.
 *      Will destination IP ,source IP and reset other field of IP multicast entry.
 * Input:
 *      unit                - unit id
 *      pIpMcast_hashKey    - the hash key to initialize content of buffer
 *      pIpMcast_addr       - IP multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT         - The module is not initial
 *      RT_ERR_UNIT_ID          - invalid unit id
 *      RT_ERR_IPV4_ADDRESS     - Invalid IPv4 address
 *      RT_ERR_NULL_POINTER     - input parameter may be null pointer
 * Note:
 *      Need to initialize IP multicast entry before add it.
 */
int32
dal_maple_l2_ipMcastAddrExt_init(uint32 unit, rtk_l2_ipMcastHashKey_t *pIpMcast_hashKey, rtk_l2_ipMcastAddr_t *pIpMcast_addr)
{
    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    RT_PARAM_CHK(NULL == pIpMcast_addr, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK(((pIpMcast_hashKey->dip >> 28) & BITMASK_4B) != 0xE, RT_ERR_IPV4_ADDRESS);
    if (LOOKUP_ON_DIP_AND_FVID == ip4HashFmt[unit])
        RT_PARAM_CHK((pIpMcast_hashKey->rvid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);

    osal_memset(pIpMcast_addr, 0, sizeof(rtk_l2_ipMcastAddr_t));

    pIpMcast_addr->dip = pIpMcast_hashKey->dip;
    if (LOOKUP_ON_DIP_AND_FVID == ip4HashFmt[unit])
        pIpMcast_addr->rvid = pIpMcast_hashKey->rvid;
    else if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
        pIpMcast_addr->sip = pIpMcast_hashKey->sip;

    return RT_ERR_OK;
}

/* Function Name:
 *      dal_maple_l2_ipMcastAddr_add
 * Description:
 *      Add IP multicast entry to ASIC.
 * Input:
 *      unit          - unit id
 *      pIpmcast_addr - IP multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_IPV4_ADDRESS - Invalid IPv4 address
 *      RT_ERR_VLAN_VID     - invalid vlan id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_INPUT        - invalid input parameter
 * Note:
 *      Need to initialize IP multicast entry before add it.
 */
int32
dal_maple_l2_ipMcastAddr_add(uint32 unit, rtk_l2_ipMcastAddr_t *pIpmcast_addr)
{
    int32 ret;

    pIpmcast_addr->fwdIndex = -1;

    ret = dal_maple_l2_ipMcastAddr_addByIndex(unit, pIpmcast_addr);

    return ret;
} /* end of dal_maple_l2_ipMcastAddr_add */

/* Function Name:
 *      dal_maple_l2_ipMcastAddr_get
 * Description:
 *      Get IP multicast entry on specified dip and sip.
 * Input:
 *      unit          - unit id
 *      pIpmcast_addr - IP multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_IPV4_ADDRESS - Invalid IPv4 address
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Need to initialize IP multicast entry before add it.
 */
int32
dal_maple_l2_ipMcastAddr_get(uint32 unit, rtk_l2_ipMcastAddr_t *pIpMcast_addr)
{
    int32   ret;
    dal_maple_l2_entry_t  l2_entry;
    dal_maple_l2_index_t  index_entry;
    multicast_index_entry_t mcast_entry;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dal_maple_l2_ipMcastAddr_get: unit=%d, sip=%x, dip=%x, vid=%d",
           unit, pIpMcast_addr->sip, pIpMcast_addr->dip, pIpMcast_addr->rvid);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(NULL == pIpMcast_addr, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pIpMcast_addr->rvid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);

    /* search exist or free entry */
    if (LOOKUP_ON_FVID_AND_MAC == ip4HashFmt[unit])
    {
        return RT_ERR_CHIP_NOT_SUPPORTED;
    }
    else if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc_sip.rvid = pIpMcast_addr->rvid;
        l2_entry.ipmcast_ip_mc_sip.dip  = pIpMcast_addr->dip;
        l2_entry.ipmcast_ip_mc_sip.sip  = pIpMcast_addr->sip;
    }
    else if (LOOKUP_ON_DIP_AND_FVID == ip4HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc.rvid = pIpMcast_addr->rvid;
        l2_entry.ipmcast_ip_mc.dip  = pIpMcast_addr->dip;
    }
    l2_entry.entry_type = IP4_MULTICAST;

    ret = _dal_maple_l2_getExistOrFreeL2Entry(unit, &l2_entry, L2_GET_EXIST_ONLY, &index_entry);
    if (RT_ERR_OK != ret)
    {
        return ret;
    }

    if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
    {
        pIpMcast_addr->rvid = l2_entry.ipmcast_ip_mc_sip.rvid;
    }

    if (index_entry.index_type == L2_IN_HASH)
        pIpMcast_addr->l2_idx = (index_entry.index << 2) | index_entry.hashdepth;
    else
        pIpMcast_addr->l2_idx = (1 << 13) + index_entry.index;

    if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
        pIpMcast_addr->fwdIndex = l2_entry.ipmcast_ip_mc_sip.index;
    else
        pIpMcast_addr->fwdIndex = l2_entry.ipmcast_ip_mc.index;

    L2_SEM_LOCK(unit);
    if ((ret = table_read(unit, MAPLE_MC_PMSKt, pIpMcast_addr->fwdIndex, (uint32 *)&mcast_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }
    L2_SEM_UNLOCK(unit);

    if ((ret = table_field_get(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, &(pIpMcast_addr->portmask.bits[0]), (uint32 *) &mcast_entry)) != RT_ERR_OK)
    {
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dal_maple_l2_ipMcastAddr_get: pPortmask==0x%x", pIpMcast_addr->portmask.bits[0]);

    return ret;
} /* end of dal_maple_l2_ipMcastAddr_get */

/* Function Name:
 *      dal_maple_l2_ipMcastAddr_set
 * Description:
 *      Update content of IP multicast entry.
 * Input:
 *      unit          - unit id
 *      pIpmcast_addr - IP multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_IPV4_ADDRESS - Invalid IPv4 address
 *      RT_ERR_VLAN_VID     - invalid vlan id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_INPUT        - invalid input parameter
 * Note:
 *      None
 */
int32
dal_maple_l2_ipMcastAddr_set(uint32 unit, rtk_l2_ipMcastAddr_t *pIpmcast_addr)
{
    int32   ret;
    uint32  index;
    dal_maple_l2_entry_t  l2_entry;
    dal_maple_l2_index_t  index_entry;
    multicast_index_entry_t mcast_entry;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dal_maple_l2_ipMcastAddr_set: unit=%d, sip=%x, dip=%x, vid=%d, pPortmask==0x%x",
           unit, pIpmcast_addr->sip, pIpmcast_addr->dip, (uint32)pIpmcast_addr->rvid, pIpmcast_addr->portmask.bits[0]);
    RT_PARAM_CHK(((pIpmcast_addr->dip >>28) & BITMASK_4B) != 0xE, RT_ERR_IPV4_ADDRESS);
    RT_PARAM_CHK((pIpmcast_addr->rvid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    RT_PARAM_CHK(!HWP_PMSK_EXIST(unit, &pIpmcast_addr->portmask), RT_ERR_PORT_MASK);

    /* parameter check */
    RT_PARAM_CHK(NULL == pIpmcast_addr, RT_ERR_NULL_POINTER);
    if (LOOKUP_ON_FVID_AND_MAC == ip4HashFmt[unit])
    {
        return RT_ERR_CHIP_NOT_SUPPORTED;
    }
    else if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc_sip.rvid = pIpmcast_addr->rvid;
        l2_entry.ipmcast_ip_mc_sip.dip  = pIpmcast_addr->dip;
        l2_entry.ipmcast_ip_mc_sip.sip  = pIpmcast_addr->sip;
    }
    else if (LOOKUP_ON_DIP_AND_FVID == ip4HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc.rvid = pIpmcast_addr->rvid;
        l2_entry.ipmcast_ip_mc.dip  = pIpmcast_addr->dip;
    }
    l2_entry.entry_type = IP4_MULTICAST;

    L2_SEM_LOCK(unit);

    /* search exist or free entry */
    ret = _dal_maple_l2_getExistOrFreeL2Entry(unit, &l2_entry, L2_GET_EXIST_ONLY, &index_entry);

    if (RT_ERR_OK != ret)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    osal_memset(&mcast_entry, 0, sizeof(mcast_entry));

    if ((ret = table_field_set(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, pIpmcast_addr->portmask.bits, (uint32 *) &mcast_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
        index = l2_entry.ipmcast_ip_mc_sip.index;
    else
        index = l2_entry.ipmcast_ip_mc.index;
    if ((ret = table_write(unit, MAPLE_MC_PMSKt, index, (uint32 *)&mcast_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    /* Update fvid if needs */
    if (ipmcst_fvid_cmp == FALSE && LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit] && l2_entry.ipmcast_ip_mc_sip.rvid != pIpmcast_addr->rvid)
    {
        l2_entry.ipmcast_ip_mc_sip.rvid = pIpmcast_addr->rvid;
        if (L2_IN_HASH == index_entry.index_type)
        {
            /* if found entry is in HASH, programming in SRAM */
            ret = _dal_maple_l2_setL2HASHEntry(unit, &l2_entry, &index_entry);
        }
        else if(L2_IN_CAM == index_entry.index_type)
        {
            /* if found entry is in CAM, programming in CAM */
            ret = _dal_maple_l2_setL2CAMEntry(unit, &l2_entry, &index_entry);
        }
    }

    L2_SEM_UNLOCK(unit);

    return ret;
} /* end of dal_maple_l2_ipMcastAddr_set */

/* Function Name:
 *      dal_maple_l2_ipMcastAddr_del
 * Description:
 *      Delete a L2 ip multicast address entry from the specified device.
 * Input:
 *      unit - unit id
 *      sip  - source ip address
 *      dip  - destination ip address
 *      vid  - vlan id
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT       - The module is not initial
 *      RT_ERR_L2_HASH_KEY    - invalid L2 Hash key
 *      RT_ERR_L2_EMPTY_ENTRY - the entry is empty(invalid)
 * Note:
 *      1) In vlan unaware mode (SVL), the vid will be ignore, suggest to
 *         input vid=0 in vlan unaware mode.
 *      2) In vlan aware mode (IVL), the vid will be care.
 */
int32
dal_maple_l2_ipMcastAddr_del(uint32 unit, ipaddr_t sip, ipaddr_t dip, rtk_vlan_t vid)
{
    int32 ret;
    int32 mcFwdIndex;
    dal_maple_l2_entry_t  l2_entry;
    dal_maple_l2_index_t  index_entry;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, sip=%x, dip=%x, vid=%d",
           unit, sip, dip, vid);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((vid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);

    /* search exist or free entry */
    if (LOOKUP_ON_FVID_AND_MAC == ip4HashFmt[unit])
    {
        return RT_ERR_CHIP_NOT_SUPPORTED;
    }
    else if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc_sip.rvid = vid;
        l2_entry.ipmcast_ip_mc_sip.dip  = dip;
        l2_entry.ipmcast_ip_mc_sip.sip  = sip;
    }
    else if (LOOKUP_ON_DIP_AND_FVID == ip4HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc.rvid = vid;
        l2_entry.ipmcast_ip_mc.dip  = dip;
    }
    l2_entry.entry_type   = IP4_MULTICAST;

    L2_SEM_LOCK(unit);

    ret = _dal_maple_l2_getExistOrFreeL2Entry(unit, &l2_entry, L2_GET_EXIST_ONLY, &index_entry);

    if (RT_ERR_OK != ret)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
        mcFwdIndex = l2_entry.ipmcast_ip_mc_sip.index;
    else
        mcFwdIndex = l2_entry.ipmcast_ip_mc.index;

    ret = _dal_maple_l2_freeMcastIdx(unit, (int32)mcFwdIndex);
    if(ret != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        return RT_ERR_L2_IPMULTI_PARAM;
    }

    if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc_sip.rvid = 0;
        l2_entry.ipmcast_ip_mc_sip.dip  = 0;
        l2_entry.ipmcast_ip_mc_sip.sip  = 0;
        l2_entry.ipmcast_ip_mc_sip.index = 0;
    }
    else if (LOOKUP_ON_DIP_AND_FVID == ip4HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc.rvid = 0;
        l2_entry.ipmcast_ip_mc.dip  = 0;
        l2_entry.ipmcast_ip_mc.index = 0;
    }

    if (L2_IN_HASH == index_entry.index_type)
    {
        /* if found entry is in HASH, programming in SRAM */
        ret = _dal_maple_l2_setL2HASHEntry(unit, &l2_entry, &index_entry);
    }
    else if (L2_IN_CAM == index_entry.index_type)
    {
        /* if found entry is in CAM, programming in CAM */
        ret = _dal_maple_l2_setL2CAMEntry(unit, &l2_entry, &index_entry);
    }
    else
    {
        ret = RT_ERR_FAILED;
    }

    L2_SEM_UNLOCK(unit);

    return ret;
} /* end of dal_maple_l2_ipMcastAddr_del */

/* Function Name:
 *      dal_maple_l2_ipMcastAddr_addByIndex
 * Description:
 *      Add a IP multicast address entry and multicast index to the specified device.
 * Input:
 *      unit                    - unit id
 *      vid                     - vlan id
 *      pIpMcast_addr           - content of IP multicast address entry
 * Output:
 *      pIpMcast_addr->fwdIndex - index of multicast forwarding entry
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT           - The module is not initial
 *      RT_ERR_L2_HASH_KEY        - invalid L2 Hash key
 *      RT_ERR_NULL_POINTER       - input parameter may be null pointer
 *      RT_ERR_L2_MULTI_FWD_INDEX - invalid index of multicast forwarding portmask
 * Note:
 *      If fwdIndex is larger than or equal to 0, will use fwdIndex as multicast index
 *          and won't config portmask.
 *
 *      If fwdIndex is smaller than 0, will allocate a free index and return it.
 *          It will also config portmask.
 */
int32
dal_maple_l2_ipMcastAddr_addByIndex(uint32 unit, rtk_l2_ipMcastAddr_t *pIpMcast_addr)
{
    int32 ret;
    int32 fwdIndex;
    dal_maple_l2_entry_t  l2_entry, dynamic_l2_entry;
    dal_maple_l2_index_t  index_entry;
    multicast_index_entry_t mcast_entry;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(NULL == pIpMcast_addr, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK(pIpMcast_addr->fwdIndex >= (int32)mcast_idx_pool[unit].size_of_mcast_fwd_index, RT_ERR_L2_MULTI_FWD_INDEX);
    RT_PARAM_CHK((pIpMcast_addr->rvid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    RT_PARAM_CHK(((pIpMcast_addr->dip >>28) & BITMASK_4B) != 0xE, RT_ERR_L2_IPMULTI_PARAM);
    RT_PARAM_CHK(!HWP_PMSK_EXIST(unit, &pIpMcast_addr->portmask), RT_ERR_PORT_MASK);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dal_maple_l2_ipMcastAddr_addByIndex: unit=%d, sip=%x, dip=%x, vid=%d, pPortmask==0x%x",
           unit, pIpMcast_addr->sip, pIpMcast_addr->dip, (uint32)pIpMcast_addr->rvid, pIpMcast_addr->portmask.bits[0]);

    if (LOOKUP_ON_FVID_AND_MAC == ip4HashFmt[unit])
    {
        return RT_ERR_CHIP_NOT_SUPPORTED;
    }
    else if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc_sip.rvid = pIpMcast_addr->rvid;
        l2_entry.ipmcast_ip_mc_sip.dip  = pIpMcast_addr->dip;
        l2_entry.ipmcast_ip_mc_sip.sip  = pIpMcast_addr->sip;
    }
    else if (LOOKUP_ON_DIP_AND_FVID == ip4HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc.rvid = pIpMcast_addr->rvid;
        l2_entry.ipmcast_ip_mc.dip  = pIpMcast_addr->dip;
    }
    l2_entry.entry_type = IP4_MULTICAST;

    L2_SEM_LOCK(unit);

    /* search exist or free entry */
    ret = _dal_maple_l2_getExistOrFreeL2Entry(unit, &l2_entry, L2_GET_EXIST_OR_FREE, &index_entry);

    if (ret == RT_ERR_L2_NO_EMPTY_ENTRY && (pIpMcast_addr->add_op_flags & RTK_L2_ADD_OP_FLAG_REPLACE_DYNAMIC) != 0)
    {
        osal_memcpy(&dynamic_l2_entry, &l2_entry, sizeof(dal_maple_l2_entry_t));
        dynamic_l2_entry.entry_type  = IP4_MULTICAST;

        ret = _dal_maple_l2_getFirstDynamicEntry(unit, &dynamic_l2_entry, &index_entry);
        if (ret)
        {
            L2_SEM_UNLOCK(unit);
            RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
            if (RT_ERR_L2_ENTRY_NOTFOUND == ret)
                return RT_ERR_L2_NO_EMPTY_ENTRY;
            else
                return ret;
        }
    }
    else if (RT_ERR_OK != ret)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    if (TRUE == l2_entry.is_entry_exist)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "entry exist");
        return RT_ERR_L2_ENTRY_EXIST;
    }

    if (index_entry.index_type == L2_IN_HASH)
        pIpMcast_addr->l2_idx = (index_entry.index << 2) | index_entry.hashdepth;
    else
        pIpMcast_addr->l2_idx = (1 << 13) + index_entry.index;

    fwdIndex = pIpMcast_addr->fwdIndex;

    /* get a free multicast index or increase reference count of mcast index */
    if ((ret = _dal_maple_l2_allocMcastIdx(unit, &(pIpMcast_addr->fwdIndex))) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    osal_memset(&mcast_entry, 0, sizeof(mcast_entry));

    if (fwdIndex < 0)
    { /* Will configure portmask when fwdIndex is allocate automatically */
        if ((ret = table_field_set(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, &(pIpMcast_addr->portmask.bits[0]), (uint32 *) &mcast_entry)) != RT_ERR_OK)
        {
            L2_SEM_UNLOCK(unit);
            RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
            return ret;
        }

        if ((ret = table_write(unit, MAPLE_MC_PMSKt, (uint32)(pIpMcast_addr->fwdIndex), (uint32 *)&mcast_entry)) != RT_ERR_OK)
        {
            L2_SEM_UNLOCK(unit);
            RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
            return ret;
        }
    }

    if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
        l2_entry.ipmcast_ip_mc_sip.index = (uint32)(pIpMcast_addr->fwdIndex);
    else if (LOOKUP_ON_DIP_AND_FVID == ip4HashFmt[unit])
        l2_entry.ipmcast_ip_mc.index = (uint32)(pIpMcast_addr->fwdIndex);

    if (L2_IN_HASH == index_entry.index_type)
    {
        /* if found entry is in HASH, programming in CAM */
        ret = _dal_maple_l2_setL2HASHEntry(unit, &l2_entry, &index_entry);
    }
    else if(L2_IN_CAM == index_entry.index_type)
    {
        /* if found entry is in CAM, programming in CAM */
        ret = _dal_maple_l2_setL2CAMEntry(unit, &l2_entry, &index_entry);
    }

    if (RT_ERR_OK != ret)
    {
        _dal_maple_l2_freeMcastIdx(unit, (int32)l2_entry.l2mcast.index);
        L2_SEM_UNLOCK(unit);
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_ipMcastAddr_addByIndex */

/* Function Name:
 *      dal_maple_l2_ipMcastAddr_setByIndex
 * Description:
 *      Update content of IP multicast entry.
 * Input:
 *      unit          - unit id
 *      pIpmcast_addr - IP multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_IPV4_ADDRESS - Invalid IPv4 address
 *      RT_ERR_VLAN_VID     - invalid vlan id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_INPUT        - invalid input parameter
 * Note:
 *      None
 */
int32
dal_maple_l2_ipMcastAddr_setByIndex(uint32 unit, rtk_l2_ipMcastAddr_t *pIpmcast_addr)
{
    int32   ret;
    uint32  *pIndex=NULL;
    dal_maple_l2_entry_t  l2_entry;
    dal_maple_l2_index_t  index_entry;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dal_maple_l2_ipMcastAddr_set: unit=%d, sip=%x, dip=%x, vid=%d, pPortmask==0x%x",
           unit, pIpmcast_addr->sip, pIpmcast_addr->dip, (uint32)pIpmcast_addr->rvid, pIpmcast_addr->portmask.bits[0]);
    RT_PARAM_CHK(((pIpmcast_addr->dip >>28) & BITMASK_4B) != 0xE, RT_ERR_IPV4_ADDRESS);
    RT_PARAM_CHK((pIpmcast_addr->rvid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    RT_PARAM_CHK(!HWP_PMSK_EXIST(unit, &pIpmcast_addr->portmask), RT_ERR_PORT_MASK);

    /* parameter check */
    RT_PARAM_CHK(NULL == pIpmcast_addr, RT_ERR_NULL_POINTER);
    if (LOOKUP_ON_FVID_AND_MAC == ip4HashFmt[unit])
    {
        return RT_ERR_CHIP_NOT_SUPPORTED;
    }
    else if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc_sip.rvid = pIpmcast_addr->rvid;
        l2_entry.ipmcast_ip_mc_sip.dip  = pIpmcast_addr->dip;
        l2_entry.ipmcast_ip_mc_sip.sip  = pIpmcast_addr->sip;
    }
    else if (LOOKUP_ON_DIP_AND_FVID == ip4HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc.rvid = pIpmcast_addr->rvid;
        l2_entry.ipmcast_ip_mc.dip  = pIpmcast_addr->dip;
    }
    l2_entry.entry_type = IP4_MULTICAST;

    L2_SEM_LOCK(unit);

    /* search exist or free entry */
    ret = _dal_maple_l2_getExistOrFreeL2Entry(unit, &l2_entry, L2_GET_EXIST_ONLY, &index_entry);

    if (RT_ERR_OK != ret)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
        pIndex = &l2_entry.ipmcast_ip_mc_sip.index;
    else
        pIndex = &l2_entry.ipmcast_ip_mc.index;

    /* Update fvid if needs */
    if ((ipmcst_fvid_cmp == FALSE && LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit] && l2_entry.ipmcast_ip_mc_sip.rvid != pIpmcast_addr->rvid) ||
        (*pIndex != pIpmcast_addr->fwdIndex))
    {
        l2_entry.ipmcast_ip_mc_sip.rvid = pIpmcast_addr->rvid;
        *pIndex = (uint32)pIpmcast_addr->fwdIndex;
        if (L2_IN_HASH == index_entry.index_type)
        {
            /* if found entry is in HASH, programming in SRAM */
            ret = _dal_maple_l2_setL2HASHEntry(unit, &l2_entry, &index_entry);
        }
        else if(L2_IN_CAM == index_entry.index_type)
        {
            /* if found entry is in CAM, programming in CAM */
            ret = _dal_maple_l2_setL2CAMEntry(unit, &l2_entry, &index_entry);
        }
    }

    L2_SEM_UNLOCK(unit);

    return ret;
} /* end of dal_maple_l2_ipMcastAddr_setByIndex */

/* Function Name:
 *      dal_maple_l2_ipMcastAddr_delIgnoreIndex
 * Description:
 *      Delete a L2 ip multicast address entry from the specified device.
 * Input:
 *      unit - unit id
 *      sip  - source ip address
 *      dip  - destination ip address
 *      vid  - vlan id
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT       - The module is not initial
 *      RT_ERR_L2_HASH_KEY    - invalid L2 Hash key
 *      RT_ERR_L2_EMPTY_ENTRY - the entry is empty(invalid)
 * Note:
 *      1) In vlan unaware mode (SVL), the vid will be ignore, suggest to
 *         input vid=0 in vlan unaware mode.
 *      2) In vlan aware mode (IVL), the vid will be care.
 */
int32
dal_maple_l2_ipMcastAddr_delIgnoreIndex(uint32 unit, ipaddr_t sip, ipaddr_t dip, rtk_vlan_t vid)
{
    int32 ret;
    dal_maple_l2_entry_t  l2_entry;
    dal_maple_l2_index_t  index_entry;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, sip=%x, dip=%x, vid=%d",
           unit, sip, dip, vid);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((vid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);

    /* search exist or free entry */
    if (LOOKUP_ON_FVID_AND_MAC == ip4HashFmt[unit])
    {
        return RT_ERR_CHIP_NOT_SUPPORTED;
    }
    else if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc_sip.rvid = vid;
        l2_entry.ipmcast_ip_mc_sip.dip  = dip;
        l2_entry.ipmcast_ip_mc_sip.sip  = sip;
    }
    else if (LOOKUP_ON_DIP_AND_FVID == ip4HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc.rvid = vid;
        l2_entry.ipmcast_ip_mc.dip  = dip;
    }
    l2_entry.entry_type   = IP4_MULTICAST;

    L2_SEM_LOCK(unit);

    ret = _dal_maple_l2_getExistOrFreeL2Entry(unit, &l2_entry, L2_GET_EXIST_ONLY, &index_entry);

    if (RT_ERR_OK != ret)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc_sip.rvid = 0;
        l2_entry.ipmcast_ip_mc_sip.dip  = 0;
        l2_entry.ipmcast_ip_mc_sip.sip  = 0;
        l2_entry.ipmcast_ip_mc_sip.index = 0;
    }
    else if (LOOKUP_ON_DIP_AND_FVID == ip4HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc.rvid = 0;
        l2_entry.ipmcast_ip_mc.dip  = 0;
        l2_entry.ipmcast_ip_mc.index = 0;
    }

    if (L2_IN_HASH == index_entry.index_type)
    {
        /* if found entry is in HASH, programming in SRAM */
        ret = _dal_maple_l2_setL2HASHEntry(unit, &l2_entry, &index_entry);
    }
    else if (L2_IN_CAM == index_entry.index_type)
    {
        /* if found entry is in CAM, programming in CAM */
        ret = _dal_maple_l2_setL2CAMEntry(unit, &l2_entry, &index_entry);
    }
    else
    {
        ret = RT_ERR_FAILED;
    }

    L2_SEM_UNLOCK(unit);

    return ret;
} /* end of dal_maple_l2_ipMcastAddr_delIgnoreIndex */


/* Function Name:
 *      dal_maple_l2_ipMcastAddrChkEnable_get
 * Description:
 *      Get DIP check status for IPv4 multicast packet.
 * Input:
 *      unit    - unit id
 * Output:
 *      pEnable - pointer to DIP check status for IPv4 multicast packet
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None.
 */
int32
dal_maple_l2_ipMcastAddrChkEnable_get(uint32 unit, rtk_enable_t *pEnable)
{
    int32   ret;
    uint32  enable;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d", unit);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((NULL == pEnable), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if ((ret = reg_field_read(unit, MAPLE_L2_CTRL_0r, MAPLE_IP_MC_DIP_CHKf, &enable)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }
    L2_SEM_UNLOCK(unit);

    /* translate chip's value to definition */
    if (1 == enable)
    {
        *pEnable = ENABLED;
    }
    else
    {
        *pEnable = DISABLED;
    }

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pEnable=%d", *pEnable);

    return RT_ERR_OK;
}  /* end of dal_maple_l2_ipMcastAddrChkEnable_get */

/* Function Name:
 *      dal_maple_l2_ipMcastAddrChkEnable_set
 * Description:
 *      Set DIP check status for IPv4 multicast packet.
 * Input:
 *      unit   - unit id
 *      enable - DIP check status for IPv4 multicast packet
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      None
 */
int32
dal_maple_l2_ipMcastAddrChkEnable_set(uint32 unit, rtk_enable_t enable)
{
    int32   ret;
    uint32  value;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, enable=%d",
           unit, enable);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((ENABLED != enable && DISABLED != enable), RT_ERR_INPUT);

    /* translate definition to chip's value  */
    if (ENABLED == enable)
    {
        value = 1;
    }
    else
    {
        value = 0;
    }

    L2_SEM_LOCK(unit);

    /* programming value to CHIP*/
    if ((ret = reg_field_write(unit, MAPLE_L2_CTRL_0r, MAPLE_IP_MC_DIP_CHKf, &value)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }
    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_ipMcastAddrChkEnable_set */

/* Function Name:
 *      dal_maple_l2_nextValidIpMcastAddr_get
 * Description:
 *      Get next valid L2 ip multicast address entry from the specified device.
 * Input:
 *      unit      - unit id
 *      pScan_idx - currently scan index of l2 table to get next.
 * Output:
 *      pL2_data  - structure of l2 address data
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT          - The module is not initial
 *      RT_ERR_VLAN_VID          - invalid vid
 *      RT_ERR_NULL_POINTER      - input parameter may be null pointer
 *      RT_ERR_L2_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      1) The function will skip valid l2 unicast and multicast entry and
 *         reply next valid L2 ip multicast address is based on index order of l2 table.
 *      2) Please input -1 for get the first entry of l2 table.
 *      3) The *pScan_idx is the input and also is the output argument.
 */
int32
dal_maple_l2_nextValidIpMcastAddr_get(
    uint32                  unit,
    int32                   *pScan_idx,
    rtk_l2_ipMcastAddr_t    *pL2_data)
{
    int32   ret;
    uint32  index = 0;
    dal_maple_l2_entry_t    l2_entry;
    multicast_index_entry_t mcast_entry;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((NULL == pScan_idx), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == pL2_data), RT_ERR_NULL_POINTER);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, pScan_idx=%d",
           unit, *pScan_idx);

    L2_SEM_LOCK(unit);

    if ((ret = _dal_maple_l2_nextValidAddr_get(unit, pScan_idx, IP4_MULTICAST, FALSE, &l2_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        return ret;
    }

    if (LOOKUP_ON_FVID_AND_MAC == ip4HashFmt[unit])
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "Should not come here");
        return RT_ERR_CHIP_NOT_SUPPORTED;
    }
    else if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
    {
        pL2_data->rvid = l2_entry.ipmcast_ip_mc_sip.rvid;
        pL2_data->dip  = l2_entry.ipmcast_ip_mc_sip.dip;
        pL2_data->sip  = l2_entry.ipmcast_ip_mc_sip.sip;
        index           = l2_entry.ipmcast_ip_mc_sip.index;
    }
    else if (LOOKUP_ON_DIP_AND_FVID == ip4HashFmt[unit])
    {
        pL2_data->rvid = l2_entry.ipmcast_ip_mc.rvid;
        pL2_data->dip  = l2_entry.ipmcast_ip_mc.dip;
        index           = l2_entry.ipmcast_ip_mc.index;
    }

    osal_memset(&mcast_entry, 0, sizeof(mcast_entry));
    if ((ret = table_read(unit, MAPLE_MC_PMSKt, index, (uint32 *)&mcast_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if ((ret = table_field_get(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, pL2_data->portmask.bits, (uint32 *) &mcast_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    pL2_data->l2_idx = *pScan_idx;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, pScan_idx=%d, rvid=%d, dip=%x, sip=%x, portmask==0x%x",
           unit, *pScan_idx, pL2_data->rvid, pL2_data->dip, pL2_data->sip, pL2_data->portmask.bits[0]);

    return RT_ERR_OK;
} /* end of dal_maple_l2_nextValidIpMcastAddr_get */

/* Function Name:
 *      dal_maple_l2_ip6mcMode_get
 * Description:
 *      Get lookup mode of layer2 IPv6 multicast switching.
 * Input:
 *      unit  - unit id
 * Output:
 *      pMode - pointer to lookup mode of layer2 IPv6 multicast switching
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Lookup mode of layer2 IPv6 multicast switching for RTL8380 is as following
 *      - LOOKUP_ON_FVID_AND_MAC
 *      - LOOKUP_ON_DIP_AND_SIP
 *      - LOOKUP_ON_DIP_AND_FVID
 */
int32
dal_maple_l2_ip6mcMode_get(uint32 unit, rtk_l2_ipmcMode_t *pMode)
{
    int32   ret;
    uint32  format = 0;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d", unit);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((NULL == pMode), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if ((ret = reg_field_read(unit, MAPLE_L2_CTRL_0r, MAPLE_IPV6_MC_HASH_KEY_FMTf, &format)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    switch (format)
    {
        case 0:
            *pMode = LOOKUP_ON_FVID_AND_MAC;
            break;
        case 1:
            *pMode = LOOKUP_ON_DIP_AND_SIP;
            break;
        case 2:
            *pMode = LOOKUP_ON_DIP_AND_FVID;
            break;
        default:
            return RT_ERR_FAILED;
    }

    ip6HashFmt[unit] = *pMode;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pMode=%ld", *pMode);

    return RT_ERR_OK;
} /* end of dal_maple_l2_ip6mcMode_get */

/* Function Name:
 *      dal_maple_l2_ip6mcMode_set
 * Description:
 *      Set lookup mode of layer2 IPv6 multicast switching.
 * Input:
 *      unit - unit id
 *      mode - lookup mode of layer2 IPv6 multicast switching
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      Lookup mode of layer2 IPv6 multicast switching for RTL8380 is as following
 *      - LOOKUP_ON_FVID_AND_MAC
 *      - LOOKUP_ON_DIP_AND_SIP
 *      - LOOKUP_ON_DIP_AND_FVID
 */
int32
dal_maple_l2_ip6mcMode_set(uint32 unit, rtk_l2_ipmcMode_t mode)
{
    int32   ret;
    uint32  value;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, mode=%d", unit, mode);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((mode >= IPMC_MODE_END), RT_ERR_INPUT);

    switch (mode)
    {
        case LOOKUP_ON_FVID_AND_MAC:
            value = 0;
            break;
        case LOOKUP_ON_DIP_AND_SIP:
            value = 1;
            break;
        case LOOKUP_ON_DIP_AND_FVID:
            value = 2;
            break;
        default:
            return RT_ERR_INPUT;
    }

    L2_SEM_LOCK(unit);

    if((ret = reg_field_write(unit, MAPLE_L2_CTRL_0r, MAPLE_IPV6_MC_HASH_KEY_FMTf, &value)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    ip6HashFmt[unit] = value;

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_ip6mcMode_set */

/* Function Name:
 *      dal_maple_l2_ip6McastAddrExt_init
 * Description:
 *      Initialize content of buffer of IPv6 multicast entry.
 *      Will destination IP ,source IP (or vid) and reset other field of IPv6 multicast entry.
 * Input:
 *      unit                - unit id
 *      pIp6Mcast_hashKey   - the hash key to initialize content of buffer
 *      pIp6Mcast_addr      - IPv6 multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT         - The module is not initial
 *      RT_ERR_UNIT_ID          - invalid unit id
 *      RT_ERR_IPV6_ADDRESS     - Invalid IPv6 address
 *      RT_ERR_NULL_POINTER     - input parameter may be null pointer
 * Note:
 *      Need to initialize IPv6 multicast entry before add it.
 */
int32
dal_maple_l2_ip6McastAddrExt_init(uint32 unit, rtk_l2_ip6McastHashKey_t *pIp6Mcast_hashKey, rtk_l2_ip6McastAddr_t *pIp6Mcast_addr)
{
    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    RT_PARAM_CHK(NULL == pIp6Mcast_hashKey, RT_ERR_NULL_POINTER);
    if (LOOKUP_ON_DIP_AND_FVID == ip6HashFmt[unit])
        RT_PARAM_CHK((pIp6Mcast_hashKey->rvid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);

    osal_memset(pIp6Mcast_addr, 0, sizeof(rtk_l2_ip6McastAddr_t));

    pIp6Mcast_addr->dip = pIp6Mcast_hashKey->dip;
    if (LOOKUP_ON_DIP_AND_FVID == ip6HashFmt[unit])
        pIp6Mcast_addr->rvid = pIp6Mcast_hashKey->rvid;
    else if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
        pIp6Mcast_addr->sip = pIp6Mcast_hashKey->sip;

    return RT_ERR_OK;
}

/* Function Name:
 *      dal_maple_l2_ip6McastAddr_add
 * Description:
 *      Add IPv6 multicast entry to ASIC.
 * Input:
 *      unit          - unit id
 *      pIpmcast_addr - IP multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_IPV6_ADDRESS - Invalid IPv6 address
 *      RT_ERR_VLAN_VID     - invalid vlan id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_INPUT        - invalid input parameter
 * Note:
 *      Need to initialize IP multicast entry before add it.
 */
int32
dal_maple_l2_ip6McastAddr_add(uint32 unit, rtk_l2_ip6McastAddr_t *pIpmcast_addr)
{
    int32 ret;

    pIpmcast_addr->fwdIndex = -1;

    ret = dal_maple_l2_ip6McastAddr_addByIndex(unit, pIpmcast_addr);

    return ret;
} /* end of dal_maple_l2_ip6McastAddr_add */

/* Function Name:
 *      dal_maple_l2_ip6McastAddr_get
 * Description:
 *      Get IP multicast entry on specified dip and sip.
 * Input:
 *      unit          - unit id
 *      pIpmcast_addr - IP multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_IPV4_ADDRESS - Invalid IPv4 address
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Need to initialize IP multicast entry before add it.
 */
int32
dal_maple_l2_ip6McastAddr_get(uint32 unit, rtk_l2_ip6McastAddr_t *pIpMcast_addr)
{
    int32   ret;
    dal_maple_l2_entry_t  l2_entry;
    dal_maple_l2_index_t  index_entry;
    multicast_index_entry_t mcast_entry;
    uint32 index;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dal_maple_l2_ip6McastAddr_get: unit=%d, vid=%d, pPortmask==0x%x\n",
           unit, pIpMcast_addr->rvid, pIpMcast_addr->portmask.bits[0]);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "sip=%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
        pIpMcast_addr->sip.octet[0],
        pIpMcast_addr->sip.octet[1],
        pIpMcast_addr->sip.octet[2],
        pIpMcast_addr->sip.octet[3],
        pIpMcast_addr->sip.octet[4],
        pIpMcast_addr->sip.octet[5],
        pIpMcast_addr->sip.octet[6],
        pIpMcast_addr->sip.octet[7],
        pIpMcast_addr->sip.octet[8],
        pIpMcast_addr->sip.octet[9],
        pIpMcast_addr->sip.octet[10],
        pIpMcast_addr->sip.octet[11],
        pIpMcast_addr->sip.octet[12],
        pIpMcast_addr->sip.octet[13],
        pIpMcast_addr->sip.octet[14],
        pIpMcast_addr->sip.octet[15]
        );
    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dip=%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
        pIpMcast_addr->dip.octet[0],
        pIpMcast_addr->dip.octet[1],
        pIpMcast_addr->dip.octet[2],
        pIpMcast_addr->dip.octet[3],
        pIpMcast_addr->dip.octet[4],
        pIpMcast_addr->dip.octet[5],
        pIpMcast_addr->dip.octet[6],
        pIpMcast_addr->dip.octet[7],
        pIpMcast_addr->dip.octet[8],
        pIpMcast_addr->dip.octet[9],
        pIpMcast_addr->dip.octet[10],
        pIpMcast_addr->dip.octet[11],
        pIpMcast_addr->dip.octet[12],
        pIpMcast_addr->dip.octet[13],
        pIpMcast_addr->dip.octet[14],
        pIpMcast_addr->dip.octet[15]
        );
    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(NULL == pIpMcast_addr, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pIpMcast_addr->rvid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);

    /* search exist or free entry */
    _dal_maple_l2_ip6CareByteConvertToIP(&l2_entry.ipmcast_ip_mc_sip.dip, &l2_entry.ipmcast_ip_mc_sip.sip,
        &pIpMcast_addr->dip, &pIpMcast_addr->sip);
    if (LOOKUP_ON_FVID_AND_MAC == ip6HashFmt[unit])
    {
        return RT_ERR_CHIP_NOT_SUPPORTED;
    }
    else if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc_sip.rvid = pIpMcast_addr->rvid;
    }
    else if (LOOKUP_ON_DIP_AND_FVID == ip6HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc.rvid = pIpMcast_addr->rvid;
    }
    l2_entry.entry_type = IP6_MULTICAST;

    ret = _dal_maple_l2_getExistOrFreeL2Entry(unit, &l2_entry, L2_GET_EXIST_ONLY, &index_entry);
    if (RT_ERR_OK != ret)
    {
        return ret;
    }

    if (index_entry.index_type == L2_IN_HASH)
        pIpMcast_addr->l2_idx = (index_entry.index << 2) | index_entry.hashdepth;
    else
        pIpMcast_addr->l2_idx = (1 << 13) + index_entry.index;

    if (L2_IN_HASH == index_entry.index_type)
        index = ip6CacheIndexTbl[index_entry.index * HAL_L2_HASHDEPTH(unit) + index_entry.hashdepth];
    else
        index = ip6CacheIndexTbl[8192 + index_entry.index];
    if (osal_memcmp(&ip6CacheTbl[index].dip, &pIpMcast_addr->dip, sizeof(rtk_ipv6_addr_t)) != 0 ||
            osal_memcmp(&ip6CacheTbl[index].sip, &pIpMcast_addr->sip, sizeof(rtk_ipv6_addr_t)) != 0)
    {
        return RT_ERR_L2_ENTRY_NOTFOUND;
    }

    if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
    {
        pIpMcast_addr->rvid = l2_entry.ipmcast_ip_mc_sip.rvid;
        pIpMcast_addr->fwdIndex = l2_entry.ipmcast_ip_mc_sip.index;
    }
    else
    {
        pIpMcast_addr->rvid = l2_entry.ipmcast_ip_mc.rvid;
        pIpMcast_addr->fwdIndex = l2_entry.ipmcast_ip_mc.index;
    }

    L2_SEM_LOCK(unit);
    if ((ret = table_read(unit, MAPLE_MC_PMSKt, pIpMcast_addr->fwdIndex, (uint32 *)&mcast_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }
    L2_SEM_UNLOCK(unit);

    if ((ret = table_field_get(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, &(pIpMcast_addr->portmask.bits[0]), (uint32 *) &mcast_entry)) != RT_ERR_OK)
    {
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dal_maple_l2_ip6McastAddr_get: pPortmask==0x%x", pIpMcast_addr->portmask.bits[0]);

    return ret;
} /* end of dal_maple_l2_ip6McastAddr_get */

/* Function Name:
 *      dal_maple_l2_ip6McastAddr_set
 * Description:
 *      Update content of IP multicast entry.
 * Input:
 *      unit          - unit id
 *      pIpmcast_addr - IP multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_IPV6_ADDRESS - Invalid IPv4 address
 *      RT_ERR_VLAN_VID     - invalid vlan id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_INPUT        - invalid input parameter
 * Note:
 *      None
 */
int32
dal_maple_l2_ip6McastAddr_set(uint32 unit, rtk_l2_ip6McastAddr_t *pIpmcast_addr)
{
    int32   ret;
    uint32  index;
    dal_maple_l2_entry_t  l2_entry;
    dal_maple_l2_index_t  index_entry;
    multicast_index_entry_t mcast_entry;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dal_maple_l2_ip6McastAddr_set: unit=%d, vid=%d, pPortmask==0x%x\n",
           unit, pIpmcast_addr->rvid, pIpmcast_addr->portmask.bits[0]);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "sip=%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
        pIpmcast_addr->sip.octet[0],
        pIpmcast_addr->sip.octet[1],
        pIpmcast_addr->sip.octet[2],
        pIpmcast_addr->sip.octet[3],
        pIpmcast_addr->sip.octet[4],
        pIpmcast_addr->sip.octet[5],
        pIpmcast_addr->sip.octet[6],
        pIpmcast_addr->sip.octet[7],
        pIpmcast_addr->sip.octet[8],
        pIpmcast_addr->sip.octet[9],
        pIpmcast_addr->sip.octet[10],
        pIpmcast_addr->sip.octet[11],
        pIpmcast_addr->sip.octet[12],
        pIpmcast_addr->sip.octet[13],
        pIpmcast_addr->sip.octet[14],
        pIpmcast_addr->sip.octet[15]
        );
    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dip=%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
        pIpmcast_addr->dip.octet[0],
        pIpmcast_addr->dip.octet[1],
        pIpmcast_addr->dip.octet[2],
        pIpmcast_addr->dip.octet[3],
        pIpmcast_addr->dip.octet[4],
        pIpmcast_addr->dip.octet[5],
        pIpmcast_addr->dip.octet[6],
        pIpmcast_addr->dip.octet[7],
        pIpmcast_addr->dip.octet[8],
        pIpmcast_addr->dip.octet[9],
        pIpmcast_addr->dip.octet[10],
        pIpmcast_addr->dip.octet[11],
        pIpmcast_addr->dip.octet[12],
        pIpmcast_addr->dip.octet[13],
        pIpmcast_addr->dip.octet[14],
        pIpmcast_addr->dip.octet[15]
        );

    _dal_maple_l2_ip6CareByteConvertToIP(&l2_entry.ipmcast_ip_mc_sip.dip, &l2_entry.ipmcast_ip_mc_sip.sip,
        &pIpmcast_addr->dip, &pIpmcast_addr->sip);

    /* parameter check */
    RT_PARAM_CHK(NULL == pIpmcast_addr, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK(!HWP_PMSK_EXIST(unit, &pIpmcast_addr->portmask), RT_ERR_PORT_MASK);

    if (LOOKUP_ON_FVID_AND_MAC == ip6HashFmt[unit])
    {
        return RT_ERR_CHIP_NOT_SUPPORTED;
    }
    else if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc_sip.rvid = pIpmcast_addr->rvid;
    }
    else if (LOOKUP_ON_DIP_AND_FVID == ip6HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc.rvid = pIpmcast_addr->rvid;
    }

    l2_entry.entry_type             = IP6_MULTICAST;

    L2_SEM_LOCK(unit);

    /* search exist or free entry */
    ret = _dal_maple_l2_getExistOrFreeL2Entry(unit, &l2_entry, L2_GET_EXIST_ONLY, &index_entry);

    if (RT_ERR_OK != ret)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    if (L2_IN_HASH == index_entry.index_type)
        index = (uint32)ip6CacheIndexTbl[index_entry.index * HAL_L2_HASHDEPTH(unit) + index_entry.hashdepth];
    else
        index = ip6CacheIndexTbl[8192 + index_entry.index];

    if (osal_memcmp(&ip6CacheTbl[index].dip, &pIpmcast_addr->dip, sizeof(rtk_ipv6_addr_t)) != 0 ||
            osal_memcmp(&ip6CacheTbl[index].sip, &pIpmcast_addr->sip, sizeof(rtk_ipv6_addr_t)) != 0)
    {
        L2_SEM_UNLOCK(unit);
        return RT_ERR_L2_ENTRY_NOTFOUND;
    }

    osal_memset(&mcast_entry, 0, sizeof(mcast_entry));

    if ((ret = table_field_set(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, pIpmcast_addr->portmask.bits, (uint32 *) &mcast_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
        index = l2_entry.ipmcast_ip_mc_sip.index;
    else
        index = l2_entry.ipmcast_ip_mc.index;
    if ((ret = table_write(unit, MAPLE_MC_PMSKt, index, (uint32 *)&mcast_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    /* Update fvid if needs */
    if (ipmcst_fvid_cmp == FALSE && LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit] && l2_entry.ipmcast_ip_mc_sip.rvid != pIpmcast_addr->rvid)
    {
        l2_entry.ipmcast_ip_mc_sip.rvid = pIpmcast_addr->rvid;
        if (L2_IN_HASH == index_entry.index_type)
        {
            /* if found entry is in HASH, programming in SRAM */
            ret = _dal_maple_l2_setL2HASHEntry(unit, &l2_entry, &index_entry);
        }
        else if(L2_IN_CAM == index_entry.index_type)
        {
            /* if found entry is in CAM, programming in CAM */
            ret = _dal_maple_l2_setL2CAMEntry(unit, &l2_entry, &index_entry);
        }
    }


    L2_SEM_UNLOCK(unit);

    return ret;
} /* end of dal_maple_l2_ip6McastAddr_set */

/* Function Name:
 *      dal_maple_l2_ip6McastAddr_del
 * Description:
 *      Delete a L2 ipv6 multicast address entry from the specified device.
 * Input:
 *      unit - unit id
 *      sip  - source ip address
 *      dip  - destination ip address
 *      vid  - vlan id
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT       - The module is not initial
 *      RT_ERR_VLAN_VID       - invalid vlan id
 *      RT_ERR_L2_HASH_KEY    - invalid L2 Hash key
 *      RT_ERR_L2_EMPTY_ENTRY - the entry is empty(invalid)
 * Note:
 *      1) In vlan unaware mode (SVL), the vid will be ignore, suggest to
 *         input vid=0 in vlan unaware mode.
 *      2) In vlan aware mode (IVL), the vid will be care.
 */
int32
dal_maple_l2_ip6McastAddr_del(uint32 unit, rtk_ipv6_addr_t sip, rtk_ipv6_addr_t dip, rtk_vlan_t vid)
{
    int32 ret;
    dal_maple_l2_entry_t  l2_entry;
    dal_maple_l2_index_t  index_entry;
    uint32 index, mcFwdIndex;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dal_maple_l2_ip6McastAddr_del: unit=%d, vid=%d, pPortmask=%x\n",
           unit, vid);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "sip=%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
        sip.octet[0], sip.octet[1], sip.octet[2], sip.octet[3],
        sip.octet[4], sip.octet[5], sip.octet[6], sip.octet[7],
        sip.octet[8], sip.octet[9], sip.octet[10], sip.octet[11],
        sip.octet[12], sip.octet[13], sip.octet[14], sip.octet[15]);
    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dip=%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
        dip.octet[0], dip.octet[1], dip.octet[2], dip.octet[3],
        dip.octet[4], dip.octet[5], dip.octet[6], dip.octet[7],
        dip.octet[8], dip.octet[9], dip.octet[10], dip.octet[11],
        dip.octet[12], dip.octet[13], dip.octet[14], dip.octet[15]);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((vid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);

    /* search exist or free entry */
    _dal_maple_l2_ip6CareByteConvertToIP(&l2_entry.ipmcast_ip_mc_sip.dip, &l2_entry.ipmcast_ip_mc_sip.sip, (rtk_ipv6_addr_t *)&dip, (rtk_ipv6_addr_t *)&sip);
    if (LOOKUP_ON_FVID_AND_MAC == ip6HashFmt[unit])
    {
        return RT_ERR_CHIP_NOT_SUPPORTED;
    }
    else if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc_sip.rvid = vid;
    }
    else if (LOOKUP_ON_DIP_AND_FVID == ip6HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc.rvid = vid;
    }
    l2_entry.entry_type   = IP6_MULTICAST;

    L2_SEM_LOCK(unit);

    ret = _dal_maple_l2_getExistOrFreeL2Entry(unit, &l2_entry, L2_GET_EXIST_ONLY, &index_entry);

    if (RT_ERR_OK != ret)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
        mcFwdIndex = l2_entry.ipmcast_ip_mc_sip.index;
    else
        mcFwdIndex = l2_entry.ipmcast_ip_mc.index;

    if (L2_IN_HASH == index_entry.index_type)
        index = (uint32)ip6CacheIndexTbl[index_entry.index * HAL_L2_HASHDEPTH(unit) + index_entry.hashdepth];
    else
        index = ip6CacheIndexTbl[8192 + index_entry.index];

    if (osal_memcmp(&ip6CacheTbl[index].dip, &dip, sizeof(rtk_ipv6_addr_t)) == 0 &&
            osal_memcmp(&ip6CacheTbl[index].sip, &sip, sizeof(rtk_ipv6_addr_t)) == 0)
    {
        ip6CacheTbl[index].valid = FALSE;
        osal_memset(&ip6CacheTbl[index].dip, 0, sizeof(rtk_ipv6_addr_t));
        osal_memset(&ip6CacheTbl[index].sip, 0, sizeof(rtk_ipv6_addr_t));
        if (index < curCacheTblIdx || curCacheTblIdx == -1)
            curCacheTblIdx = index;
    }
    else
    {
        L2_SEM_UNLOCK(unit);
        return RT_ERR_L2_ENTRY_NOTFOUND;
    }

    ret = _dal_maple_l2_freeMcastIdx(unit, (int32)mcFwdIndex);
    if(ret != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        return RT_ERR_L2_IPMULTI_PARAM;
    }

    if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc_sip.rvid = 0;
        l2_entry.ipmcast_ip_mc_sip.dip  = 0;
        l2_entry.ipmcast_ip_mc_sip.sip  = 0;
        l2_entry.ipmcast_ip_mc_sip.index = 0;
    }
    else if (LOOKUP_ON_DIP_AND_FVID == ip6HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc.rvid = 0;
        l2_entry.ipmcast_ip_mc.dip  = 0;
        l2_entry.ipmcast_ip_mc.index = 0;
    }

    if (L2_IN_HASH == index_entry.index_type)
    {
        /* if found entry is in HASH, programming in SRAM */
        ret = _dal_maple_l2_setL2HASHEntry(unit, &l2_entry, &index_entry);
    }
    else if (L2_IN_CAM == index_entry.index_type)
    {
        /* if found entry is in CAM, programming in CAM */
        ret = _dal_maple_l2_setL2CAMEntry(unit, &l2_entry, &index_entry);
    }
    else
    {
        ret = RT_ERR_FAILED;
    }

    L2_SEM_UNLOCK(unit);

    return ret;
} /* end of dal_maple_l2_ip6McastAddr_del */

/* Function Name:
 *      dal_maple_l2_ip6McastAddr_addByIndex
 * Description:
 *      Add a IPv6 multicast address entry and multicast index to the specified device.
 * Input:
 *      unit                    - unit id
 *      vid                     - vlan id
 *      pIpMcast_addr           - content of IP multicast address entry
 * Output:
 *      pIpMcast_addr->fwdIndex - index of multicast forwarding entry
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT           - The module is not initial
 *      RT_ERR_L2_HASH_KEY        - invalid L2 Hash key
 *      RT_ERR_NULL_POINTER       - input parameter may be null pointer
 *      RT_ERR_L2_MULTI_FWD_INDEX - invalid index of multicast forwarding portmask
 * Note:
 *      If fwdIndex is larger than or equal to 0, will use fwdIndex as multicast index
 *          and won't config portmask.
 *
 *      If fwdIndex is smaller than 0, will allocate a free index and return it.
 *          It will also config portmask.
 */
int32
dal_maple_l2_ip6McastAddr_addByIndex(uint32 unit, rtk_l2_ip6McastAddr_t *pIpMcast_addr)
{
    int32 ret;
    int32 fwdIndex;
    dal_maple_l2_entry_t  l2_entry, dynamic_l2_entry;
    dal_maple_l2_index_t  index_entry;
    multicast_index_entry_t mcast_entry;
    uint32 index;
    uint32 found = FALSE;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(NULL == pIpMcast_addr, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK(pIpMcast_addr->fwdIndex >= (int32)mcast_idx_pool[unit].size_of_mcast_fwd_index, RT_ERR_L2_MULTI_FWD_INDEX);
    RT_PARAM_CHK((pIpMcast_addr->rvid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    RT_PARAM_CHK(curCacheTblIdx == -1, RT_ERR_L2_IP6_CACHETBL_FULL);
    RT_PARAM_CHK(!HWP_PMSK_EXIST(unit, &pIpMcast_addr->portmask), RT_ERR_PORT_MASK);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dal_maple_l2_ip6McastAddr_addByIndex: unit=%d, vid=%d, pPortmask==0x%x\n",
           unit, pIpMcast_addr->rvid, pIpMcast_addr->portmask.bits[0]);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "sip=%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
        pIpMcast_addr->sip.octet[0],
        pIpMcast_addr->sip.octet[1],
        pIpMcast_addr->sip.octet[2],
        pIpMcast_addr->sip.octet[3],
        pIpMcast_addr->sip.octet[4],
        pIpMcast_addr->sip.octet[5],
        pIpMcast_addr->sip.octet[6],
        pIpMcast_addr->sip.octet[7],
        pIpMcast_addr->sip.octet[8],
        pIpMcast_addr->sip.octet[9],
        pIpMcast_addr->sip.octet[10],
        pIpMcast_addr->sip.octet[11],
        pIpMcast_addr->sip.octet[12],
        pIpMcast_addr->sip.octet[13],
        pIpMcast_addr->sip.octet[14],
        pIpMcast_addr->sip.octet[15]
        );
    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dip=%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
        pIpMcast_addr->dip.octet[0],
        pIpMcast_addr->dip.octet[1],
        pIpMcast_addr->dip.octet[2],
        pIpMcast_addr->dip.octet[3],
        pIpMcast_addr->dip.octet[4],
        pIpMcast_addr->dip.octet[5],
        pIpMcast_addr->dip.octet[6],
        pIpMcast_addr->dip.octet[7],
        pIpMcast_addr->dip.octet[8],
        pIpMcast_addr->dip.octet[9],
        pIpMcast_addr->dip.octet[10],
        pIpMcast_addr->dip.octet[11],
        pIpMcast_addr->dip.octet[12],
        pIpMcast_addr->dip.octet[13],
        pIpMcast_addr->dip.octet[14],
        pIpMcast_addr->dip.octet[15]
        );

    _dal_maple_l2_ip6CareByteConvertToIP(&l2_entry.ipmcast_ip_mc_sip.dip, &l2_entry.ipmcast_ip_mc_sip.sip,
        &pIpMcast_addr->dip, &pIpMcast_addr->sip);
    if (LOOKUP_ON_FVID_AND_MAC == ip6HashFmt[unit])
    {
        return RT_ERR_CHIP_NOT_SUPPORTED;
    }
    else if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
    {
        l2_entry.entry_type             = IP6_MULTICAST;
        l2_entry.ipmcast_ip_mc_sip.rvid = pIpMcast_addr->rvid;
    }
    else if (LOOKUP_ON_DIP_AND_FVID == ip6HashFmt[unit])
    {
        l2_entry.entry_type         = IP6_MULTICAST;
        l2_entry.ipmcast_ip_mc.rvid = pIpMcast_addr->rvid;
    }

    L2_SEM_LOCK(unit);

     /* search exist or free entry */
    ret = _dal_maple_l2_getExistOrFreeL2Entry(unit, &l2_entry, L2_GET_EXIST_OR_FREE, &index_entry);
    if (ret == RT_ERR_L2_NO_EMPTY_ENTRY && (pIpMcast_addr->add_op_flags & RTK_L2_ADD_OP_FLAG_REPLACE_DYNAMIC) != 0)
    {
        osal_memcpy(&dynamic_l2_entry, &l2_entry, sizeof(dal_maple_l2_entry_t));
        dynamic_l2_entry.entry_type  = IP6_MULTICAST;

        ret = _dal_maple_l2_getFirstDynamicEntry(unit, &dynamic_l2_entry, &index_entry);
        if (ret)
        {
            RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
            L2_SEM_UNLOCK(unit);
            if (RT_ERR_L2_ENTRY_NOTFOUND == ret)
                return RT_ERR_L2_NO_EMPTY_ENTRY;
            else
                return ret;
        }
    }
    else if (RT_ERR_OK != ret)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    if (TRUE == l2_entry.is_entry_exist)
    {
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "entry exist");

        if (L2_IN_HASH == index_entry.index_type)
            index = (uint32)ip6CacheIndexTbl[index_entry.index * HAL_L2_HASHDEPTH(unit) + index_entry.hashdepth];
        else
            index = ip6CacheIndexTbl[8192 + index_entry.index];

        if (osal_memcmp(&ip6CacheTbl[index].dip, &pIpMcast_addr->dip, sizeof(rtk_ipv6_addr_t)) == 0 &&
            osal_memcmp(&ip6CacheTbl[index].sip, &pIpMcast_addr->sip, sizeof(rtk_ipv6_addr_t)) == 0)
        {
            L2_SEM_UNLOCK(unit);
            return RT_ERR_L2_ENTRY_EXIST;
        }
        else
        {
            L2_SEM_UNLOCK(unit);
            return RT_ERR_L2_IP6_HASHKEY_EXIST;
        }
    }

    if (index_entry.index_type == L2_IN_HASH)
        pIpMcast_addr->l2_idx = (index_entry.index << 2) | index_entry.hashdepth;
    else
        pIpMcast_addr->l2_idx = (1 << 13) + index_entry.index;

    ip6CacheTbl[curCacheTblIdx].valid = TRUE;
    osal_memcpy(&ip6CacheTbl[curCacheTblIdx].dip, &pIpMcast_addr->dip, sizeof(rtk_ipv6_addr_t));
    osal_memcpy(&ip6CacheTbl[curCacheTblIdx].sip, &pIpMcast_addr->sip, sizeof(rtk_ipv6_addr_t));
    if (L2_IN_HASH == index_entry.index_type)
        ip6CacheIndexTbl[index_entry.index * HAL_L2_HASHDEPTH(unit) + index_entry.hashdepth] = curCacheTblIdx;
    else
        ip6CacheIndexTbl[8192 + index_entry.index] = curCacheTblIdx;

    for (index = curCacheTblIdx + 1; index < RTK_DEFAULT_L2_IP6_CACHE_TBL_SIZE; index++)
    {
        if (ip6CacheTbl[index].valid == 0)
        {
            curCacheTblIdx = index;
            found = TRUE;
            break;
        }
    }
    if (!found)
        curCacheTblIdx = -1;

    fwdIndex = pIpMcast_addr->fwdIndex;

    /* get a free multicast index or increase reference count of mcast index */
    if ((ret = _dal_maple_l2_allocMcastIdx(unit, &(pIpMcast_addr->fwdIndex))) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    osal_memset(&mcast_entry, 0, sizeof(mcast_entry));

    if (fwdIndex < 0)
    { /* Will configure portmask when fwdIndex is allocate automatically */
        if ((ret = table_field_set(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, &(pIpMcast_addr->portmask.bits[0]), (uint32 *) &mcast_entry)) != RT_ERR_OK)
        {
            L2_SEM_UNLOCK(unit);
            RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
            return ret;
        }

        if ((ret = table_write(unit, MAPLE_MC_PMSKt, (uint32)(pIpMcast_addr->fwdIndex), (uint32 *)&mcast_entry)) != RT_ERR_OK)
        {
            L2_SEM_UNLOCK(unit);
            RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
            return ret;
        }
    }

    if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
        l2_entry.ipmcast_ip_mc_sip.index = (uint32)(pIpMcast_addr->fwdIndex);
    else if (LOOKUP_ON_DIP_AND_FVID == ip6HashFmt[unit])
        l2_entry.ipmcast_ip_mc.index = (uint32)(pIpMcast_addr->fwdIndex);

    if (L2_IN_HASH == index_entry.index_type)
    {
        /* if found entry is in HASH, programming in SRAM */
        ret = _dal_maple_l2_setL2HASHEntry(unit, &l2_entry, &index_entry);
    }
    else if(L2_IN_CAM == index_entry.index_type)
    {
        /* if found entry is in CAM, programming in CAM */
        ret = _dal_maple_l2_setL2CAMEntry(unit, &l2_entry, &index_entry);
    }

    if (RT_ERR_OK != ret)
    {
        _dal_maple_l2_freeMcastIdx(unit, (int32)l2_entry.l2mcast.index);
        L2_SEM_UNLOCK(unit);
        return ret;
    }

    L2_SEM_UNLOCK(unit);
    return RT_ERR_OK;
} /* end of dal_maple_l2_ip6McastAddr_addByIndex */

/* Function Name:
 *      dal_maple_l2_ip6McastAddr_setByIndex
 * Description:
 *      Update content of IP multicast entry.
 * Input:
 *      unit          - unit id
 *      pIpmcast_addr - IP multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_IPV6_ADDRESS - Invalid IPv4 address
 *      RT_ERR_VLAN_VID     - invalid vlan id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_INPUT        - invalid input parameter
 * Note:
 *      None
 */
int32
dal_maple_l2_ip6McastAddr_setByIndex(uint32 unit, rtk_l2_ip6McastAddr_t *pIpmcast_addr)
{
    int32   ret;
    uint32  *pIndex=NULL;
    uint32  index;
    dal_maple_l2_entry_t  l2_entry;
    dal_maple_l2_index_t  index_entry;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dal_maple_l2_ip6McastAddr_set: unit=%d, vid=%d, pPortmask==0x%x\n",
           unit, pIpmcast_addr->rvid, pIpmcast_addr->portmask.bits[0]);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "sip=%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
        pIpmcast_addr->sip.octet[0],
        pIpmcast_addr->sip.octet[1],
        pIpmcast_addr->sip.octet[2],
        pIpmcast_addr->sip.octet[3],
        pIpmcast_addr->sip.octet[4],
        pIpmcast_addr->sip.octet[5],
        pIpmcast_addr->sip.octet[6],
        pIpmcast_addr->sip.octet[7],
        pIpmcast_addr->sip.octet[8],
        pIpmcast_addr->sip.octet[9],
        pIpmcast_addr->sip.octet[10],
        pIpmcast_addr->sip.octet[11],
        pIpmcast_addr->sip.octet[12],
        pIpmcast_addr->sip.octet[13],
        pIpmcast_addr->sip.octet[14],
        pIpmcast_addr->sip.octet[15]
        );
    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dip=%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
        pIpmcast_addr->dip.octet[0],
        pIpmcast_addr->dip.octet[1],
        pIpmcast_addr->dip.octet[2],
        pIpmcast_addr->dip.octet[3],
        pIpmcast_addr->dip.octet[4],
        pIpmcast_addr->dip.octet[5],
        pIpmcast_addr->dip.octet[6],
        pIpmcast_addr->dip.octet[7],
        pIpmcast_addr->dip.octet[8],
        pIpmcast_addr->dip.octet[9],
        pIpmcast_addr->dip.octet[10],
        pIpmcast_addr->dip.octet[11],
        pIpmcast_addr->dip.octet[12],
        pIpmcast_addr->dip.octet[13],
        pIpmcast_addr->dip.octet[14],
        pIpmcast_addr->dip.octet[15]
        );

    _dal_maple_l2_ip6CareByteConvertToIP(&l2_entry.ipmcast_ip_mc_sip.dip, &l2_entry.ipmcast_ip_mc_sip.sip,
        &pIpmcast_addr->dip, &pIpmcast_addr->sip);

    /* parameter check */
    RT_PARAM_CHK(NULL == pIpmcast_addr, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK(!HWP_PMSK_EXIST(unit, &pIpmcast_addr->portmask), RT_ERR_PORT_MASK);

    if (LOOKUP_ON_FVID_AND_MAC == ip6HashFmt[unit])
    {
        return RT_ERR_CHIP_NOT_SUPPORTED;
    }
    else if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc_sip.rvid = pIpmcast_addr->rvid;
    }
    else if (LOOKUP_ON_DIP_AND_FVID == ip6HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc.rvid = pIpmcast_addr->rvid;
    }

    l2_entry.entry_type             = IP6_MULTICAST;

    L2_SEM_LOCK(unit);

    /* search exist or free entry */
    ret = _dal_maple_l2_getExistOrFreeL2Entry(unit, &l2_entry, L2_GET_EXIST_ONLY, &index_entry);

    if (RT_ERR_OK != ret)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    if (L2_IN_HASH == index_entry.index_type)
        index = (uint32)ip6CacheIndexTbl[index_entry.index * HAL_L2_HASHDEPTH(unit) + index_entry.hashdepth];
    else
        index = ip6CacheIndexTbl[8192 + index_entry.index];

    if (osal_memcmp(&ip6CacheTbl[index].dip, &pIpmcast_addr->dip, sizeof(rtk_ipv6_addr_t)) != 0 ||
            osal_memcmp(&ip6CacheTbl[index].sip, &pIpmcast_addr->sip, sizeof(rtk_ipv6_addr_t)) != 0)
    {
        L2_SEM_UNLOCK(unit);
        return RT_ERR_L2_ENTRY_NOTFOUND;
    }


    if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
        pIndex = &l2_entry.ipmcast_ip_mc_sip.index;
    else
        pIndex = &l2_entry.ipmcast_ip_mc.index;

    /* Update fvid if needs */
    if ((ipmcst_fvid_cmp == FALSE && LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit] && l2_entry.ipmcast_ip_mc_sip.rvid != pIpmcast_addr->rvid) ||
        (*pIndex != pIpmcast_addr->fwdIndex))
    {
        l2_entry.ipmcast_ip_mc_sip.rvid = pIpmcast_addr->rvid;
        *pIndex = (uint32)pIpmcast_addr->fwdIndex;
        if (L2_IN_HASH == index_entry.index_type)
        {
            /* if found entry is in HASH, programming in SRAM */
            ret = _dal_maple_l2_setL2HASHEntry(unit, &l2_entry, &index_entry);
        }
        else if(L2_IN_CAM == index_entry.index_type)
        {
            /* if found entry is in CAM, programming in CAM */
            ret = _dal_maple_l2_setL2CAMEntry(unit, &l2_entry, &index_entry);
        }
    }


    L2_SEM_UNLOCK(unit);

    return ret;
} /* end of dal_maple_l2_ip6McastAddr_setByIndex */

/* Function Name:
 *      dal_maple_l2_ip6McastAddr_delIgnoreIndex
 * Description:
 *      Delete a L2 ipv6 multicast address entry from the specified device.
 * Input:
 *      unit - unit id
 *      sip  - source ip address
 *      dip  - destination ip address
 *      vid  - vlan id
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT       - The module is not initial
 *      RT_ERR_VLAN_VID       - invalid vlan id
 *      RT_ERR_L2_HASH_KEY    - invalid L2 Hash key
 *      RT_ERR_L2_EMPTY_ENTRY - the entry is empty(invalid)
 * Note:
 *      1) In vlan unaware mode (SVL), the vid will be ignore, suggest to
 *         input vid=0 in vlan unaware mode.
 *      2) In vlan aware mode (IVL), the vid will be care.
 */
int32
dal_maple_l2_ip6McastAddr_delIgnoreIndex(uint32 unit, rtk_ipv6_addr_t sip, rtk_ipv6_addr_t dip, rtk_vlan_t vid)
{
    int32 ret;
    dal_maple_l2_entry_t  l2_entry;
    dal_maple_l2_index_t  index_entry;
    uint32 index;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dal_maple_l2_ip6McastAddr_del: unit=%d, vid=%d, pPortmask=%x\n",
           unit, vid);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "sip=%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
        sip.octet[0], sip.octet[1], sip.octet[2], sip.octet[3],
        sip.octet[4], sip.octet[5], sip.octet[6], sip.octet[7],
        sip.octet[8], sip.octet[9], sip.octet[10], sip.octet[11],
        sip.octet[12], sip.octet[13], sip.octet[14], sip.octet[15]);
    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dip=%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
        dip.octet[0], dip.octet[1], dip.octet[2], dip.octet[3],
        dip.octet[4], dip.octet[5], dip.octet[6], dip.octet[7],
        dip.octet[8], dip.octet[9], dip.octet[10], dip.octet[11],
        dip.octet[12], dip.octet[13], dip.octet[14], dip.octet[15]);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((vid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);

    /* search exist or free entry */
    _dal_maple_l2_ip6CareByteConvertToIP(&l2_entry.ipmcast_ip_mc_sip.dip, &l2_entry.ipmcast_ip_mc_sip.sip, (rtk_ipv6_addr_t *)&dip, (rtk_ipv6_addr_t *)&sip);
    if (LOOKUP_ON_FVID_AND_MAC == ip6HashFmt[unit])
    {
        return RT_ERR_CHIP_NOT_SUPPORTED;
    }
    else if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc_sip.rvid = vid;
    }
    else if (LOOKUP_ON_DIP_AND_FVID == ip6HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc.rvid = vid;
    }
    l2_entry.entry_type   = IP6_MULTICAST;

    L2_SEM_LOCK(unit);

    ret = _dal_maple_l2_getExistOrFreeL2Entry(unit, &l2_entry, L2_GET_EXIST_ONLY, &index_entry);

    if (RT_ERR_OK != ret)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if (L2_IN_HASH == index_entry.index_type)
        index = (uint32)ip6CacheIndexTbl[index_entry.index * HAL_L2_HASHDEPTH(unit) + index_entry.hashdepth];
    else
        index = ip6CacheIndexTbl[8192 + index_entry.index];

    if (osal_memcmp(&ip6CacheTbl[index].dip, &dip, sizeof(rtk_ipv6_addr_t)) == 0 &&
            osal_memcmp(&ip6CacheTbl[index].sip, &sip, sizeof(rtk_ipv6_addr_t)) == 0)
    {
        ip6CacheTbl[index].valid = FALSE;
        osal_memset(&ip6CacheTbl[index].dip, 0, sizeof(rtk_ipv6_addr_t));
        osal_memset(&ip6CacheTbl[index].sip, 0, sizeof(rtk_ipv6_addr_t));
        if (index < curCacheTblIdx || curCacheTblIdx == -1)
            curCacheTblIdx = index;
    }
    else
    {
        L2_SEM_UNLOCK(unit);
        return RT_ERR_L2_ENTRY_NOTFOUND;
    }

    if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc_sip.rvid = 0;
        l2_entry.ipmcast_ip_mc_sip.dip  = 0;
        l2_entry.ipmcast_ip_mc_sip.sip  = 0;
        l2_entry.ipmcast_ip_mc_sip.index = 0;
    }
    else if (LOOKUP_ON_DIP_AND_FVID == ip6HashFmt[unit])
    {
        l2_entry.ipmcast_ip_mc.rvid = 0;
        l2_entry.ipmcast_ip_mc.dip  = 0;
        l2_entry.ipmcast_ip_mc.index = 0;
    }

    if (L2_IN_HASH == index_entry.index_type)
    {
        /* if found entry is in HASH, programming in SRAM */
        ret = _dal_maple_l2_setL2HASHEntry(unit, &l2_entry, &index_entry);
    }
    else if (L2_IN_CAM == index_entry.index_type)
    {
        /* if found entry is in CAM, programming in CAM */
        ret = _dal_maple_l2_setL2CAMEntry(unit, &l2_entry, &index_entry);
    }
    else
    {
        ret = RT_ERR_FAILED;
    }

    L2_SEM_UNLOCK(unit);

    return ret;
} /* end of dal_maple_l2_ip6McastAddr_delIgnoreIndex */


/* Function Name:
 *      dal_maple_l2_nextValidIp6McastAddr_get
 * Description:
 *      Get next valid L2 IPv6 multicast address entry from the specified device.
 * Input:
 *      unit      - unit id
 *      pScan_idx - currently scan index of l2 table to get next.
 * Output:
 *      pL2_data  - structure of l2 address data
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT          - The module is not initial
 *      RT_ERR_VLAN_VID          - invalid vid
 *      RT_ERR_NULL_POINTER      - input parameter may be null pointer
 *      RT_ERR_L2_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      1) The function will skip valid l2 unicast and multicast entry and
 *         reply next valid L2 ip multicast address is based on index order of l2 table.
 *      2) Please input -1 for get the first entry of l2 table.
 *      3) The *pScan_idx is the input and also is the output argument.
 */
int32
dal_maple_l2_nextValidIp6McastAddr_get(uint32 unit, int32 *pScan_idx, rtk_l2_ip6McastAddr_t *pL2_data)
{
    int32   ret;
    uint32  index = 0;
    dal_maple_l2_entry_t    l2_entry;
    multicast_index_entry_t mcast_entry;
    //ipaddr_t invalid_ipv4_addr = 0;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((NULL == pScan_idx), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == pL2_data), RT_ERR_NULL_POINTER);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, pScan_idx=%d", unit, *pScan_idx);

    L2_SEM_LOCK(unit);

    if ((ret = _dal_maple_l2_nextValidAddr_get(unit, pScan_idx, IP6_MULTICAST, FALSE, &l2_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        return ret;
    }

    if (LOOKUP_ON_FVID_AND_MAC == ip6HashFmt[unit])
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "Should not come here");
        return RT_ERR_CHIP_NOT_SUPPORTED;
    }

    index = ip6CacheIndexTbl[*pScan_idx];
    osal_memcpy(&pL2_data->dip, &ip6CacheTbl[index].dip, sizeof(rtk_ipv6_addr_t));
    osal_memcpy(&pL2_data->sip, &ip6CacheTbl[index].sip, sizeof(rtk_ipv6_addr_t));

    if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
    {
        pL2_data->rvid = l2_entry.ipmcast_ip_mc_sip.rvid;
        index          = l2_entry.ipmcast_ip_mc_sip.index;
        //_dal_maple_l2_ip6CareByteConvertToIP6Addr(&pL2_data->dip, &pL2_data->sip, &l2_entry.ipmcast_ip_mc_sip.dip, &l2_entry.ipmcast_ip_mc_sip.sip);
    }
    else if (LOOKUP_ON_DIP_AND_FVID == ip6HashFmt[unit])
    {
        pL2_data->rvid = l2_entry.ipmcast_ip_mc.rvid;
        index          = l2_entry.ipmcast_ip_mc.index;
        //_dal_maple_l2_ip6CareByteConvertToIP6Addr(&pL2_data->dip, &pL2_data->sip, &l2_entry.ipmcast_ip_mc.dip, &invalid_ipv4_addr);
        //osal_memset(&pL2_data->sip, 0, sizeof(rtk_ipv6_addr_t));
    }

    osal_memset(&mcast_entry, 0, sizeof(mcast_entry));
    if ((ret = table_read(unit, MAPLE_MC_PMSKt, index, (uint32 *)&mcast_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if ((ret = table_field_get(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, pL2_data->portmask.bits, (uint32 *) &mcast_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    pL2_data->l2_idx = *pScan_idx;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dal_maple_l2_nextValidIp6McastAddr_get: unit=%d, vid=%d, pPortmask==0x%x\n",
           unit, pL2_data->rvid, pL2_data->portmask.bits[0]);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "sip=%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
        pL2_data->sip.octet[0],
        pL2_data->sip.octet[1],
        pL2_data->sip.octet[2],
        pL2_data->sip.octet[3],
        pL2_data->sip.octet[4],
        pL2_data->sip.octet[5],
        pL2_data->sip.octet[6],
        pL2_data->sip.octet[7],
        pL2_data->sip.octet[8],
        pL2_data->sip.octet[9],
        pL2_data->sip.octet[10],
        pL2_data->sip.octet[11],
        pL2_data->sip.octet[12],
        pL2_data->sip.octet[13],
        pL2_data->sip.octet[14],
        pL2_data->sip.octet[15]
        );
    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dip=%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
        pL2_data->dip.octet[0],
        pL2_data->dip.octet[1],
        pL2_data->dip.octet[2],
        pL2_data->dip.octet[3],
        pL2_data->dip.octet[4],
        pL2_data->dip.octet[5],
        pL2_data->dip.octet[6],
        pL2_data->dip.octet[7],
        pL2_data->dip.octet[8],
        pL2_data->dip.octet[9],
        pL2_data->dip.octet[10],
        pL2_data->dip.octet[11],
        pL2_data->dip.octet[12],
        pL2_data->dip.octet[13],
        pL2_data->dip.octet[14],
        pL2_data->dip.octet[15]
        );
    return RT_ERR_OK;
} /* end of dal_maple_l2_nextValidIpMcastAddr_get */

/* Function Name:
 *      dal_maple_l2_cpuMacAddr_add
 * Description:
 *      Add a CPU mac adress of the vlan id to the specified device.
 * Input:
 *      unit - unit id
 *      vid  - vlan id
 *      pMac - cpu mac address
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_VLAN_VID     - invalid vid
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
dal_maple_l2_cpuMacAddr_add(uint32 unit, rtk_vlan_t vid, rtk_mac_t *pMac)
{
    int32 ret;
    dal_maple_l2_entry_t  l2_entry;
    dal_maple_l2_index_t  index_entry;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, vid=%d", unit, vid);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((vid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    RT_PARAM_CHK(NULL == pMac, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pMac->octet[0] & BITMASK_1B) != 0, RT_ERR_MAC);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pMac=%x-%x-%x-%x-%x-%x",
           pMac->octet[0], pMac->octet[1], pMac->octet[2],
           pMac->octet[3], pMac->octet[4], pMac->octet[5]);

    /* search exist or free entry */
    l2_entry.entry_type = L2_UNICAST;
    l2_entry.unicast.fid = vid;
    osal_memcpy(&l2_entry.unicast.mac, pMac, sizeof(rtk_mac_t));

    L2_SEM_LOCK(unit);

    if ((ret = _dal_maple_l2_getExistOrFreeL2Entry(unit, &l2_entry, L2_GET_EXIST_OR_FREE, &index_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    /* fill content */
    osal_memcpy(&l2_entry.unicast.mac, pMac, sizeof(rtk_mac_t));
    l2_entry.unicast.fid = vid;
    if (-1 == HWP_CPU_MACID(unit))
    {
        L2_SEM_UNLOCK(unit);
        return RT_ERR_L2_NO_CPU_PORT;
    }
    l2_entry.unicast.port       = HWP_CPU_MACID(unit);
    l2_entry.unicast.aging      = 3;
    l2_entry.unicast.sablock    = 0;
    l2_entry.unicast.dablock    = 0;
    l2_entry.unicast.is_static  = 1;
    l2_entry.unicast.suspending = 0;
    l2_entry.unicast.nh         = 0;

    if (L2_IN_HASH == index_entry.index_type)
    {
        /* if found entry is in HASH, programming in CAM */
        ret = _dal_maple_l2_setL2HASHEntry(unit, &l2_entry, &index_entry);
    }
    else if (L2_IN_CAM == index_entry.index_type)
    {
        /* if found entry is in CAM, programming in CAM */
        ret = _dal_maple_l2_setL2CAMEntry(unit, &l2_entry, &index_entry);
    }
    else
    {
        ret = RT_ERR_FAILED;
    }

    L2_SEM_UNLOCK(unit);

    return ret;
} /* end of dal_maple_l2_cpuMacAddr_add */

/* Function Name:
 *      dal_maple_l2_cpuMacAddr_del
 * Description:
 *      Delete a CPU mac adress of the vlan id from the specified device.
 * Input:
 *      unit - unit id
 *      vid  - vlan id
 *      pMac - cpu mac address
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_VLAN_VID     - invalid vid
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
dal_maple_l2_cpuMacAddr_del(uint32 unit, rtk_vlan_t vid, rtk_mac_t *pMac)
{
    int32 ret;
    dal_maple_l2_entry_t  l2_entry;
    dal_maple_l2_index_t  index_entry;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, vid=%d", unit, vid);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((vid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    RT_PARAM_CHK(NULL == pMac, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pMac->octet[0] & BITMASK_1B) != 0, RT_ERR_MAC);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pMac=%x-%x-%x-%x-%x-%x",
           pMac->octet[0], pMac->octet[1], pMac->octet[2],
           pMac->octet[3], pMac->octet[4], pMac->octet[5]);

    /* search exist or free entry */
    l2_entry.entry_type = L2_UNICAST;
    l2_entry.unicast.fid = vid;
    osal_memcpy(&l2_entry.unicast.mac, pMac, sizeof(rtk_mac_t));

    L2_SEM_LOCK(unit);

    if ((ret = _dal_maple_l2_getExistOrFreeL2Entry(unit, &l2_entry, L2_GET_EXIST_ONLY, &index_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    /* fill content */
    osal_memset(&l2_entry, 0, sizeof(dal_maple_l2_entry_t));
    l2_entry.entry_type = L2_UNICAST;

    if (L2_IN_HASH == index_entry.index_type)
    {
        /* if found entry is in HASH, programming in CAM */
        ret = _dal_maple_l2_setL2HASHEntry(unit, &l2_entry, &index_entry);
    }
    else if (L2_IN_CAM == index_entry.index_type)
    {
        /* if found entry is in CAM, programming in CAM */
        ret = _dal_maple_l2_setL2CAMEntry(unit, &l2_entry, &index_entry);
    }
    else
    {
        ret = RT_ERR_FAILED;
    }

    L2_SEM_UNLOCK(unit);

    return ret;
} /* end of dal_maple_l2_cpuMacAddr_del */

/* Function Name:
 *      dal_maple_l2_staticPortMoveAction_get
 * Description:
 *      Get forwarding action when port moving happen on specified port which is learnt as static entry.
 * Input:
 *      unit       - unit id
 *      port       - port id
 * Output:
 *      pFwdAction - pointer to forwarding action
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
dal_maple_l2_staticPortMoveAction_get(uint32 unit, rtk_port_t port, rtk_action_t *pFwdAction)
{
    int32   ret;
    uint32  act;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, port=%d", unit, port);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(!HWP_PORT_EXIST(unit, port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((NULL == pFwdAction), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if((ret = reg_array_field_read(unit, MAPLE_L2_PORT_STATIC_MV_ACTr, port, REG_ARRAY_INDEX_NONE, MAPLE_ACTf, &act)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    /* translate register value to action */
    switch (act)
    {
        case ACTION_FORWARD:
            *pFwdAction = 0;
            break;
        case ACTION_DROP:
            *pFwdAction = 1;
            break;
        case ACTION_TRAP2CPU:
            *pFwdAction = 2;
            break;
        case ACTION_COPY2CPU:
            *pFwdAction = 3;
            break;
        default:
            return RT_ERR_FAILED;
    }

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pFwdAction=%d", *pFwdAction);

    return RT_ERR_OK;
} /* end of dal_maple_l2_staticPortMoveAction_get */

/* Function Name:
 *      dal_maple_l2_staticPortMoveAction_set
 * Description:
 *      Set forwarding action when legal port moving happen on specified port which is learnt as static entry.
 * Input:
 *      unit      - unit id
 *      port      - port id
 *      fwdAction - forwarding action
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_PORT_ID  - invalid port id
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      None
 */
int32
dal_maple_l2_staticPortMoveAction_set(uint32 unit, rtk_l2_lookupMissType_t port, rtk_action_t fwdAction)
{
    int32   ret;
    uint32  act;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, port=%d, fwdAction=%d", unit, port, fwdAction);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(!HWP_PORT_EXIST(unit, port), RT_ERR_PORT_ID);
    RT_PARAM_CHK(fwdAction > ACTION_COPY2CPU, RT_ERR_INPUT);

    switch (fwdAction)
    {
        case ACTION_FORWARD:
            act = 0;
            break;
        case ACTION_DROP:
            act = 1;
            break;
        case ACTION_TRAP2CPU:
            act = 2;
            break;
        case ACTION_COPY2CPU:
            act = 3;
            break;
        default:
            return RT_ERR_INPUT;
    }

    L2_SEM_LOCK(unit);

    /* programming value to CHIP*/
    if((ret = reg_array_field_write(unit, MAPLE_L2_PORT_STATIC_MV_ACTr, port, REG_ARRAY_INDEX_NONE, MAPLE_ACTf, &act)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_staticPortMoveAction_set */

/* Function Name:
 *      dal_maple_l2_lookupMissFloodPortMask_get
 * Description:
 *      Get flooding port mask when unicast or multicast address lookup missed in L2 table.
 * Input:
 *      unit            - unit id
 *      type            - type of lookup miss
 * Output:
 *      pFlood_portmask - flooding port mask configuration when unicast/multicast lookup missed.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT         - The module is not initial
 *      RT_ERR_INPUT            - invalid input parameter
 *      RT_ERR_NULL_POINTER     - input parameter may be null pointer
 *      RT_ERR_L2_PMSK_NOT_INIT - flooding portmask is not initialized
 * Note:
 *      In 8380, flood-portmask get/set only supports DLF_TYPE_UCAST and DLF_TYPE_BCAST. For DLF_TYPE_MCAST,
 *      DLF_TYPE_IPMC and DLF_TYPE_IP6MC, please refer to rtk_vlan_profile_set()/rtk_vlan_profile_get().
 */
int32
dal_maple_l2_lookupMissFloodPortMask_get(uint32 unit, rtk_l2_lookupMissType_t type, rtk_portmask_t *pFlood_portmask)
{
    int32   ret;
    uint32  reg_idx, field;
    uint32  index;
    multicast_index_entry_t mcast_entry;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d", unit);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((type >= DLF_TYPE_END), RT_ERR_INPUT);
    RT_PARAM_CHK(NULL == pFlood_portmask, RT_ERR_NULL_POINTER);

    switch (type)
    {
        case DLF_TYPE_UCAST:
            if (!ucst_dlf_pmsk_inited)
                return RT_ERR_L2_PMSK_NOT_INIT;
            reg_idx = MAPLE_L2_FLD_PMSKr;
            field = MAPLE_L2_UNKN_UC_FLD_PMSKf;
            break;
        case DLF_TYPE_BCAST:
            if (!bcst_dlf_pmsk_inited)
                return RT_ERR_L2_PMSK_NOT_INIT;
            reg_idx = MAPLE_L2_FLD_PMSKr;
            field = MAPLE_L2_BC_FLD_PMSKf;
            break;
        default:
            return RT_ERR_INPUT;
    }

    L2_SEM_LOCK(unit);

    if ((ret = reg_field_read(unit, reg_idx, field, &index)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if ((ret = table_read(unit, MAPLE_MC_PMSKt, index, (uint32 *)&mcast_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    if ((ret = table_field_get(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, &(pFlood_portmask->bits[0]), (uint32 *) &mcast_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "flood_portmask==0x%x", pFlood_portmask->bits[0]);

    return ret;
} /* end of dal_maple_l2_lookupMissFloodPortMask_get */

/* Function Name:
 *      dal_maple_l2_lookupMissFloodPortMask_set
 * Description:
 *      Set flooding port mask when unicast or multicast address lookup missed in L2 table.
 * Input:
 *      unit            - unit id
 *      type            - type of lookup miss
 * Output:
 *      pFlood_portmask - flooding port mask configuration when unicast/multicast lookup missed.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT         - The module is not initial
 *      RT_ERR_NULL_POINTER     - input parameter may be null pointer
 *      RT_ERR_L2_PMSK_NOT_INIT - flooding portmask is not initialized
 * Note:
 *      In 8380, flood-portmask get/set only supports DLF_TYPE_UCAST and DLF_TYPE_BCAST. For DLF_TYPE_MCAST,
 *      DLF_TYPE_IPMC and DLF_TYPE_IP6MC, please refer to rtk_vlan_profile_set()/rtk_vlan_profile_get().
 */
int32
dal_maple_l2_lookupMissFloodPortMask_set(uint32 unit, rtk_l2_lookupMissType_t type, rtk_portmask_t *pFlood_portmask)
{
    int32   ret;
    uint32  reg_idx, field, idx;
    multicast_index_entry_t mcast_entry;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((type >= DLF_TYPE_END), RT_ERR_INPUT);
    RT_PARAM_CHK(NULL == pFlood_portmask, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK(!HWP_PMSK_EXIST(unit, pFlood_portmask), RT_ERR_PORT_MASK);

    /* Display debug message */
    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, pFlood_portmask==0x%x",
           unit, pFlood_portmask->bits[0]);

    osal_memset(&mcast_entry, 0, sizeof(multicast_index_entry_t));

    switch (type)
    {
        case DLF_TYPE_UCAST:
            if (!ucst_dlf_pmsk_inited)
                return RT_ERR_L2_PMSK_NOT_INIT;
            reg_idx = MAPLE_L2_FLD_PMSKr;
            field = MAPLE_L2_UNKN_UC_FLD_PMSKf;
            break;
        case DLF_TYPE_BCAST:
            if (!bcst_dlf_pmsk_inited)
                return RT_ERR_L2_PMSK_NOT_INIT;
            reg_idx = MAPLE_L2_FLD_PMSKr;
            field = MAPLE_L2_BC_FLD_PMSKf;
            break;
        default:
            return RT_ERR_INPUT;
    }

    L2_SEM_LOCK(unit);
    if ((ret = reg_field_read(unit, reg_idx, field, &idx)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if ((ret = table_field_set(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, &(pFlood_portmask->bits[0]), (uint32 *) &mcast_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    if ((ret = table_write(unit, MAPLE_MC_PMSKt, (uint32)idx, (uint32 *)&mcast_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    L2_SEM_UNLOCK(unit);
    return ret;
} /* end of dal_maple_l2_lookupMissFloodPortMask_set */

/* Function Name:
 *      dal_maple_l2_lookupMissFloodPortMask_setByIndex
 * Description:
 *      Set flooding port mask when unicast or multicast address lookup missed in L2 table.
 * Input:
 *      unit            - unit id
 *      type            - type of lookup miss
 *      idx             - index to multicast portmask table
 *      pFlood_portmask - flooding port mask configuration when unicast/multicast lookup missed.
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_INPUT        - invalid input parameter
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      In 8380, flood-portmask get/set only supports DLF_TYPE_UCAST and DLF_TYPE_BCAST. For DLF_TYPE_MCAST,
 *      DLF_TYPE_IPMC and DLF_TYPE_IP6MC, please refer to rtk_vlan_profile_set()/rtk_vlan_profile_get().
 */
int32
dal_maple_l2_lookupMissFloodPortMask_setByIndex(uint32 unit, rtk_l2_lookupMissType_t type, uint32 idx, rtk_portmask_t *pFlood_portmask)
{
    int32   ret;
    uint32  reg_idx, field, inited, oldIdx;
    multicast_index_entry_t mcast_entry;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((type >= DLF_TYPE_END), RT_ERR_INPUT);
    RT_PARAM_CHK(NULL == pFlood_portmask, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK(!HWP_PMSK_EXIST(unit, pFlood_portmask), RT_ERR_PORT_MASK);

    RT_PARAM_CHK((int32)idx >= (int32)mcast_idx_pool[unit].size_of_mcast_fwd_index, RT_ERR_ENTRY_INDEX);


    /* Display debug message */
    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, pFlood_portmask==0x%x",
           unit, pFlood_portmask->bits[0]);

    osal_memset(&mcast_entry, 0, sizeof(multicast_index_entry_t));

    switch (type)
    {
        case DLF_TYPE_UCAST:
            reg_idx = MAPLE_L2_FLD_PMSKr;
            field = MAPLE_L2_UNKN_UC_FLD_PMSKf;
            inited = ucst_dlf_pmsk_inited;
            break;
        case DLF_TYPE_BCAST:
            reg_idx = MAPLE_L2_FLD_PMSKr;
            field = MAPLE_L2_BC_FLD_PMSKf;
            inited = bcst_dlf_pmsk_inited;
            break;
        default:
            return RT_ERR_INPUT;
    }

    L2_SEM_LOCK(unit);

    if (inited)
    {
        reg_field_read(unit, reg_idx, field, &oldIdx);
        if (oldIdx != idx)
        {
            dal_maple_l2_mcastFwdIndex_free(unit, oldIdx);

            ret = dal_maple_l2_mcastFwdIndex_alloc(unit, (int32 *)&idx);
            if (ret == RT_ERR_L2_MCAST_FWD_ENTRY_EXIST)
            {
                _dal_maple_l2_allocMcastIdx(unit, (int32 *)&idx);
            }
            else if (ret != RT_ERR_OK)
            {
                L2_SEM_UNLOCK(unit);
                RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
                return ret;
            }
        }
        else
        {
            /* Need not to alloc */
        }
    }
    else
    {
        ret = dal_maple_l2_mcastFwdIndex_alloc(unit, (int32 *)&idx);
        if (ret == RT_ERR_L2_MCAST_FWD_ENTRY_EXIST)
        {
            _dal_maple_l2_allocMcastIdx(unit, (int32 *)&idx);
        }
        else if (ret != RT_ERR_OK)
        {
            L2_SEM_UNLOCK(unit);
            RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
            return ret;
        }
    }

    if ((ret = reg_field_write(unit, reg_idx, field, &idx)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if ((ret = table_field_set(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, &(pFlood_portmask->bits[0]), (uint32 *) &mcast_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    if ((ret = table_write(unit, MAPLE_MC_PMSKt, (uint32)idx, (uint32 *)&mcast_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    if (type == DLF_TYPE_UCAST)
        ucst_dlf_pmsk_inited = TRUE;
    else if (type == DLF_TYPE_BCAST)
        bcst_dlf_pmsk_inited = TRUE;

    return ret;
} /* end of dal_maple_l2_lookupMissFloodPortMask_setByIndex */

/* Function Name:
 *      dal_maple_l2_lookupMissFloodPortMask_add
 * Description:
 *      Add one port member to flooding port mask when unicast or multicast address lookup missed in L2 table.
 * Input:
 *      unit       - unit id
 *      type       - type of lookup miss
 *      flood_port - port id that is going to be added in flooding port mask.
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_INPUT    - invalid input parameter
 *      RT_ERR_PORT_ID  - invalid port id
 * Note:
 *      In 8380, flood-portmask get/set only supports DLF_TYPE_UCAST and DLF_TYPE_BCAST.
 */
int32
dal_maple_l2_lookupMissFloodPortMask_add(uint32 unit, rtk_l2_lookupMissType_t type, rtk_port_t flood_port)
{
    int32 ret;
    rtk_portmask_t  portmask;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, flood_port=%d",
           unit, flood_port);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((type >= DLF_TYPE_END), RT_ERR_INPUT);
    RT_PARAM_CHK(!HWP_PORT_EXIST(unit, flood_port), RT_ERR_PORT_ID);

    osal_memset(&portmask, 0, sizeof(rtk_portmask_t));

    ret = dal_maple_l2_lookupMissFloodPortMask_get(unit, type, &portmask);
    if (RT_ERR_OK != ret)
    {
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "ret=%d",ret);
        return ret;
    }

    RTK_PORTMASK_PORT_SET(portmask, flood_port);

    ret = dal_maple_l2_lookupMissFloodPortMask_set(unit, type, &portmask);
    if (RT_ERR_OK != ret)
    {
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "ret=%d",ret);
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_maple_l2_lookupMissFloodPortMask_add */

/* Function Name:
 *      dal_maple_l2_lookupMissFloodPortMask_del
 * Description:
 *      Del one port member in flooding port mask when unicast or multicast address lookup missed in L2 table.
 * Input:
 *      unit       - unit id
 *      type       - type of lookup miss
 *      flood_port - port id that is going to be added in flooding port mask.
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_INPUT    - invalid input parameter
 *      RT_ERR_PORT_ID  - invalid port id
 * Note:
 *      In 8380, flood-portmask get/set only supports DLF_TYPE_UCAST and DLF_TYPE_BCAST.
 */
int32
dal_maple_l2_lookupMissFloodPortMask_del(uint32 unit, rtk_l2_lookupMissType_t type, rtk_port_t flood_port)
{
    int32 ret;
    rtk_portmask_t  portmask;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, flood_port=%d",
           unit, flood_port);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((type >= DLF_TYPE_END), RT_ERR_INPUT);
    RT_PARAM_CHK(!HWP_PORT_EXIST(unit, flood_port), RT_ERR_PORT_ID);

    osal_memset(&portmask, 0, sizeof(rtk_portmask_t));

    ret = dal_maple_l2_lookupMissFloodPortMask_get(unit, type, &portmask);
    if (RT_ERR_OK != ret)
    {
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "ret=%d",ret);
        return ret;
    }

    RTK_PORTMASK_PORT_CLEAR(portmask, flood_port);

    ret = dal_maple_l2_lookupMissFloodPortMask_set(unit, type, &portmask);
    if (RT_ERR_OK != ret)
    {
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "ret=%d",ret);
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_maple_l2_lookupMissFloodPortMask_del */


/* Function Name:
 *      dal_maple_l2_lookupMissFloodPortMaskIdx_get
 * Description:
 *      Get the entry index of forwarding table that is used as unicast/broadcast flooding port mask.
 * Input:
 *      unit    - unit id
 *      type    - type of lookup miss
 * Output:
 *      pIdx    - flooding port mask configuration when unicast/multicast lookup missed.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT         - The module is not initial
 *      RT_ERR_NULL_POINTER     - input parameter may be null pointer
 *      RT_ERR_L2_PMSK_NOT_INIT - flooding portmask is not initialized
 *      RT_ERR_INPUT            - invalid input parameter
 * Note:
 *      In 8380, flood-portmask get/set only supports DLF_TYPE_UCAST and DLF_TYPE_BCAST. For DLF_TYPE_MCAST,
 *      DLF_TYPE_IPMC and DLF_TYPE_IP6MC, please refer to rtk_vlan_profile_set()/rtk_vlan_profile_get().
 */
int32
dal_maple_l2_lookupMissFloodPortMaskIdx_get(uint32 unit, rtk_l2_lookupMissType_t type, uint32 *pIdx)
{
    int32   ret;
    uint32  reg_idx, field;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d", unit);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(type >= DLF_TYPE_END, RT_ERR_INPUT);
    RT_PARAM_CHK(NULL == pIdx, RT_ERR_NULL_POINTER);

    switch (type)
    {
        case DLF_TYPE_UCAST:
            if (!ucst_dlf_pmsk_inited)
                return RT_ERR_L2_PMSK_NOT_INIT;
            reg_idx = MAPLE_L2_FLD_PMSKr;
            field = MAPLE_L2_UNKN_UC_FLD_PMSKf;
            break;
        case DLF_TYPE_BCAST:
            if (!bcst_dlf_pmsk_inited)
                return RT_ERR_L2_PMSK_NOT_INIT;
            reg_idx = MAPLE_L2_FLD_PMSKr;
            field = MAPLE_L2_BC_FLD_PMSKf;
            break;
        default:
            return RT_ERR_INPUT;
    }

    L2_SEM_LOCK(unit);

    if ((ret = reg_field_read(unit, reg_idx, field, pIdx)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pIdx=%d", *pIdx);

    return ret;
}

/* Function Name:
 *      dal_maple_l2_lookupMissFloodPortMaskIdx_set
 * Description:
 *      Set the entry index of forwarding table that is used as unicast/broadcast flooding port mask.
 * Input:
 *      unit    - unit id
 *      type    - type of lookup miss
 *      idx     - flooding port mask configuration when unicast/multicast lookup missed.
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT         - The module is not initial
 *      RT_ERR_L2_PMSK_NOT_INIT - flooding portmask is not initialized
 *      RT_ERR_INPUT            - invalid input parameter
 * Note:
 *      In 8380, flood-portmask get/set only supports DLF_TYPE_UCAST and DLF_TYPE_BCAST. For DLF_TYPE_MCAST,
 *      DLF_TYPE_IPMC and DLF_TYPE_IP6MC, please refer to rtk_vlan_profile_set()/rtk_vlan_profile_get().
 */
int32
dal_maple_l2_lookupMissFloodPortMaskIdx_set(uint32 unit, rtk_l2_lookupMissType_t type, uint32 idx)
{
    int32   ret;
    uint32  reg_idx, field, inited, oldIdx;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((type >= DLF_TYPE_END), RT_ERR_INPUT);
    RT_PARAM_CHK((int32)idx >= (int32)mcast_idx_pool[unit].size_of_mcast_fwd_index, RT_ERR_ENTRY_INDEX);

    /* Display debug message */
    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, rtk_l2_lookupMissType_t=%d, index=%d",
           unit, type, idx);


    switch (type)
    {
        case DLF_TYPE_UCAST:
            reg_idx = MAPLE_L2_FLD_PMSKr;
            field = MAPLE_L2_UNKN_UC_FLD_PMSKf;
            inited = ucst_dlf_pmsk_inited;
            break;
        case DLF_TYPE_BCAST:
            reg_idx = MAPLE_L2_FLD_PMSKr;
            field = MAPLE_L2_BC_FLD_PMSKf;
            inited = bcst_dlf_pmsk_inited;
            break;
        default:
            return RT_ERR_FAILED;
            break;
    }

    L2_SEM_LOCK(unit);

    if (inited)
    {
        reg_field_read(unit, reg_idx, field, &oldIdx);
        if (oldIdx != idx)
        {
            dal_maple_l2_mcastFwdIndex_free(unit, oldIdx);

            ret = dal_maple_l2_mcastFwdIndex_alloc(unit, (int32 *)&idx);
            if (ret == RT_ERR_L2_MCAST_FWD_ENTRY_EXIST)
            {
                _dal_maple_l2_allocMcastIdx(unit, (int32 *)&idx);
            }
            else if (ret != RT_ERR_OK)
            {
                L2_SEM_UNLOCK(unit);
                RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
                return ret;
            }
        }
        else
        {
            /* Need not to alloc */
        }
    }
    else
    {
        ret = dal_maple_l2_mcastFwdIndex_alloc(unit, (int32 *)&idx);
        if (ret == RT_ERR_L2_MCAST_FWD_ENTRY_EXIST)
        {
            _dal_maple_l2_allocMcastIdx(unit, (int32 *)&idx);
        }
        else if (ret != RT_ERR_OK)
        {
            L2_SEM_UNLOCK(unit);
            RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
            return ret;
        }
    }

    if ((ret = reg_field_write(unit, reg_idx, field, &idx)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    if (type == DLF_TYPE_UCAST)
        ucst_dlf_pmsk_inited = TRUE;
    else if (type == DLF_TYPE_BCAST)
        bcst_dlf_pmsk_inited = TRUE;

    return ret;
}


/* Function Name:
 *      dal_maple_l2_portLookupMissAction_get
 * Description:
 *      Get forwarding action of specified port when destination address lookup miss.
 * Input:
 *      unit    - unit id
 *      port    - port id
 *      type    - type of lookup miss
 * Output:
 *      pAction - pointer to forwarding action
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_INPUT        - invalid type of lookup miss
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Type of lookup missis as following:
 *      - DLF_TYPE_IPMC
 *      - DLF_TYPE_IP6MC
 *      - DLF_TYPE_UCAST
 *      - DLF_TYPE_MCAST
 *
 *      Forwarding action is as following:
 *      - ACTION_FORWARD
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 *      - ACTION_COPY2CPU
 */
int32
dal_maple_l2_portLookupMissAction_get(uint32 unit, rtk_port_t port, rtk_l2_lookupMissType_t type, rtk_action_t *pAction)
{
    int32   ret;
    uint32  field;
    uint32  act = 0;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d  port=%d  type=%d", unit, port, type);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(!HWP_PORT_EXIST(unit, port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((type >= DLF_TYPE_END), RT_ERR_INPUT);
    RT_PARAM_CHK((NULL == pAction), RT_ERR_NULL_POINTER);

    switch (type)
    {
        case DLF_TYPE_UCAST:
            field = MAPLE_L2_UC_LM_ACTf;
            break;
        case DLF_TYPE_MCAST:
            field = MAPLE_L2_MC_LM_ACTf;
            break;
        case DLF_TYPE_IPMC:
            field = MAPLE_IP_MC_LM_ACTf;
            break;
        case DLF_TYPE_IP6MC:
            field = MAPLE_IP6_MC_LM_ACTf;
            break;
        case DLF_TYPE_BCAST:
        default:
            return RT_ERR_INPUT;
    }

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if ((ret = reg_array_field_read(unit, MAPLE_L2_PORT_LM_ACTr, port, REG_ARRAY_INDEX_NONE, field, &act)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    switch (act)
    {
        case 0:
            *pAction = ACTION_FORWARD;
            break;
        case 1:
            *pAction = ACTION_DROP;
            break;
        case 2:
            *pAction = ACTION_TRAP2CPU;
            break;
        case 3:
            *pAction = ACTION_COPY2CPU;
            break;
        default:
            return RT_ERR_FAILED;
    }

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pAction=%ld", *pAction);

    return RT_ERR_OK;
} /* end of dal_maple_l2_portLookupMissAction_get */

/* Function Name:
 *      dal_maple_l2_portLookupMissAction_set
 * Description:
 *      Set forwarding action of specified port when destination address lookup miss.
 * Input:
 *      unit   - unit id
 *      port   - port id
 *      type   - type of lookup miss
 *      action - forwarding action
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT   - The module is not initial
 *      RT_ERR_PORT_ID    - invalid port id
 *      RT_ERR_INPUT      - invalid type of lookup miss
 *      RT_ERR_FWD_ACTION - invalid forwarding action
 * Note:
 *      Type of lookup missis as following:
 *      - DLF_TYPE_IPMC
 *      - DLF_TYPE_IP6MC
 *      - DLF_TYPE_UCAST
 *      - DLF_TYPE_MCAST
 *
 *      Forwarding action is as following:
 *      - ACTION_FORWARD
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 *      - ACTION_COPY2CPU
 */
int32
dal_maple_l2_portLookupMissAction_set(uint32 unit, rtk_port_t port, rtk_l2_lookupMissType_t type, rtk_action_t action)
{
    int32   ret;
    uint32  field;
    uint32  act = 0;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, port=%d, type=%d action=%d", unit, port, type, action);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(!HWP_PORT_EXIST(unit, port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((type >= DLF_TYPE_END), RT_ERR_INPUT);
    RT_PARAM_CHK((action >= ACTION_END), RT_ERR_FWD_ACTION);

    switch (type)
    {
        case DLF_TYPE_UCAST:
            field = MAPLE_L2_UC_LM_ACTf;
            break;
        case DLF_TYPE_MCAST:
            field = MAPLE_L2_MC_LM_ACTf;
            break;
        case DLF_TYPE_IPMC:
            field = MAPLE_IP_MC_LM_ACTf;
            break;
        case DLF_TYPE_IP6MC:
            field = MAPLE_IP6_MC_LM_ACTf;
            break;
        default:
            return RT_ERR_INPUT;
    }

    switch (action)
    {
        case ACTION_FORWARD:
            act = 0;
            break;
        case ACTION_DROP:
            act = 1;
            break;
        case ACTION_TRAP2CPU:
            act = 2;
            break;
        case ACTION_COPY2CPU:
            act = 3;
            break;
        default:
            return RT_ERR_INPUT;
    }

    L2_SEM_LOCK(unit);

    if((ret = reg_array_field_write(unit, MAPLE_L2_PORT_LM_ACTr, port, REG_ARRAY_INDEX_NONE, field, &act)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "ret=%d",ret);
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
}/* end of dal_maple_l2_portLookupMissAction_set */

/* Function Name:
 *      dal_maple_l2_srcPortEgrFilterMask_get
 * Description:
 *      Get source port egress filter mask to determine if mac need to do source filtering for an specific port
 *      when packet egress.
 * Input:
 *      unit             - unit id
 * Output:
 *      pFilter_portmask - source port egress filtering configuration when packet egress.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      May be used when wirless device connected.
 */
int32
dal_maple_l2_srcPortEgrFilterMask_get(uint32 unit, rtk_portmask_t *pFilter_portmask)
{
    int32       ret = RT_ERR_FAILED;
    uint32      value;
    rtk_port_t  port;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d", unit);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(NULL == pFilter_portmask, RT_ERR_NULL_POINTER);

    osal_memset(pFilter_portmask, 0, sizeof(rtk_portmask_t));

    L2_SEM_LOCK(unit);

    HWP_PORT_TRAVS(unit, port)
    {
        if ((ret = reg_array_field_read(unit, MAPLE_L2_IGR_P_FLTRr, port, REG_ARRAY_INDEX_NONE, MAPLE_EGR_FLTR_ENf, &value)) != RT_ERR_OK)
        {
            L2_SEM_UNLOCK(unit);
            RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "ret=%d",ret);
            return ret;
        }

        if (value)
        {
            RTK_PORTMASK_PORT_SET(*pFilter_portmask, port);
        }
    }

    L2_SEM_UNLOCK(unit);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "filter_portmask==0x%x", pFilter_portmask->bits[0]);

    return ret;
} /* end of dal_maple_l2_srcPortEgrFilterMask_get */

/* Function Name:
 *      dal_maple_l2_srcPortEgrFilterMask_set
 * Description:
 *      Set source port egress filter mask to determine if mac need to do source filtering for an specific port
 *      when packet egress.
 * Input:
 *      unit             - unit id
 *      pFilter_portmask - source port egress filtering configuration when packet egress.
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      May be used when wirless device connected.
 */
int32
dal_maple_l2_srcPortEgrFilterMask_set(uint32 unit, rtk_portmask_t *pFilter_portmask)
{
    int32       ret = RT_ERR_FAILED;
    uint32      value;
    rtk_port_t  port;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(NULL == pFilter_portmask, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK(!HWP_PMSK_EXIST(unit, pFilter_portmask), RT_ERR_PORT_MASK);

    /* Display debug message */
    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, filter_portmask==0x%x", unit, pFilter_portmask->bits[0]);

    L2_SEM_LOCK(unit);

    HWP_PORT_TRAVS(unit, port)
    {
        value = (RTK_PORTMASK_IS_PORT_SET(*pFilter_portmask, port) ? 1 : 0);
        if ((ret = reg_array_field_write(unit, MAPLE_L2_IGR_P_FLTRr, port, REG_ARRAY_INDEX_NONE, MAPLE_EGR_FLTR_ENf, &value)) != RT_ERR_OK)
        {
            L2_SEM_UNLOCK(unit);
            RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "ret=%d",ret);
            return ret;
        }
    }

    L2_SEM_UNLOCK(unit);

    return ret;
} /* end of dal_maple_l2_srcPortEgrFilterMask_set */

/* Function Name:
 *      dal_maple_l2_srcPortEgrFilterMask_add
 * Description:
 *      Add one port member to source port egress filter mask to determine if mac need to do source filtering for an specific port
 *      when packet egress.
 * Input:
 *      unit        - unit id
 *      filter_port - port id that is going to be added in source port egress filtering mask.
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_PORT_ID  - invalid port id
 * Note:
 *      May be used when wirless device connected.
 */
int32
dal_maple_l2_srcPortEgrFilterMask_add(uint32 unit, rtk_port_t filter_port)
{
    int32 ret;
    rtk_portmask_t  portmask;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, filter_port=%d",
           unit, filter_port);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(!HWP_PORT_EXIST(unit, filter_port), RT_ERR_PORT_ID);

    osal_memset(&portmask, 0, sizeof(rtk_portmask_t));

    ret = dal_maple_l2_srcPortEgrFilterMask_get(unit, &portmask);
    if (RT_ERR_OK != ret)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    RTK_PORTMASK_PORT_SET(portmask, filter_port);

    ret = dal_maple_l2_srcPortEgrFilterMask_set(unit, &portmask);
    if (RT_ERR_OK != ret)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_maple_l2_srcPortEgrFilterMask_add */

/* Function Name:
 *      dal_maple_l2_srcPortEgrFilterMask_del
 * Description:
 *      Del one port member in source port egress filter mask to determine if mac need to do source filtering for an specific port
 *      when packet egress.
 * Input:
 *      unit        - unit id
 *      filter_port - port id that is going to be deleted in source port egress filtering mask.
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_PORT_ID  - invalid port id
 * Note:
 *      May be used when wirless device connected.
 */
int32
dal_maple_l2_srcPortEgrFilterMask_del(uint32 unit, rtk_port_t filter_port)
{
    int32 ret = RT_ERR_FAILED;
    rtk_portmask_t  portmask;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, filter_port=%d",
           unit, filter_port);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(!HWP_PORT_EXIST(unit, filter_port), RT_ERR_PORT_ID);

    osal_memset(&portmask, 0, sizeof(rtk_portmask_t));

    ret = dal_maple_l2_srcPortEgrFilterMask_get(unit, &portmask);
    if (RT_ERR_OK != ret)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    RTK_PORTMASK_PORT_CLEAR(portmask, filter_port);

    ret = dal_maple_l2_srcPortEgrFilterMask_set(unit, &portmask);
    if (RT_ERR_OK != ret)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_maple_l2_srcPortEgrFilterMask_del */

/* Function Name:
 *      dal_maple_l2_exceptionAddrAction_get
 * Description:
 *      Get forwarding action of packet with exception address.
 * Input:
 *      unit       - unit id
 *      exceptType - type of exception address
 * Output:
 *      pAction    - pointer to forward action
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT            - The module is not initial
 *      RT_ERR_L2_EXCEPT_ADDR_TYPE - invalid exception address type
 *      RT_ERR_NULL_POINTER        - input parameter may be null pointer
 * Note:
 *      SA_IS_ZERO is not supported.
 *      Exception address type is as following
 *      - RTL8380 doesn't provide separated actions for SA_IS_MCAST and SA_IS_BCAST. They use the same action.
 *
 *      Forwarding action is as following
 *      - ACTION_FORWARD
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 */
int32
dal_maple_l2_exceptionAddrAction_get(uint32 unit, rtk_l2_exceptionAddrType_t exceptType, rtk_action_t *pAction)
{
    int32   ret;
    uint32  action;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, exceptType=%d", unit, exceptType);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(((exceptType != SA_IS_BCAST_OR_MCAST) && (exceptType != SA_IS_ZERO)), RT_ERR_L2_EXCEPT_ADDR_TYPE);
    RT_PARAM_CHK(NULL == pAction, RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    if(SA_IS_BCAST_OR_MCAST == exceptType)
    {
        if ((ret = reg_field_read(unit, MAPLE_L2_CTRL_0r, MAPLE_MC_BC_SA_TRAPf, &action)) != RT_ERR_OK)
        {
            L2_SEM_UNLOCK(unit);
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        if(action == 1)
            *pAction = ACTION_TRAP2CPU;
        else
        {
            if ((ret = reg_field_read(unit, MAPLE_L2_CTRL_0r, MAPLE_MC_BC_SA_DROPf, &action)) != RT_ERR_OK)
            {
                L2_SEM_UNLOCK(unit);
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            if(action == 1)
                *pAction = ACTION_DROP;
            else
                *pAction = ACTION_FORWARD;
        }
    }
    else
    {
        if ((ret = reg_field_read(unit, MAPLE_L2_CTRL_0r, MAPLE_ZERO_SA_ACTf, &action)) != RT_ERR_OK)
        {
            L2_SEM_UNLOCK(unit);
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        if (action == 0)
            *pAction = ACTION_FORWARD;
        else if (action == 1)
            *pAction = ACTION_DROP;
        else if(action == 2)
            *pAction = ACTION_TRAP2CPU;
        else
        {
            L2_SEM_UNLOCK(unit);
            return RT_ERR_FAILED;
        }
    }

    L2_SEM_UNLOCK(unit);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pAction=%d", *pAction);

    return RT_ERR_OK;
} /* end of dal_maple_l2_exceptionAddrAction_get */

/* Function Name:
 *      dal_maple_l2_exceptionAddrAction_set
 * Description:
 *      Set forwarding action of packet with exception address.
 * Input:
 *      unit       - unit id
 *      exceptType - type of exception address
 *      action     - forward action
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT            - The module is not initial
 *      RT_ERR_L2_EXCEPT_ADDR_TYPE - invalid exception address type
 *      RT_ERR_FWD_ACTION          - invalid forwarding action
 * Note:
 *      SA_IS_ZERO is not supported.
 *      Exception address type is as following
 *      - RTL8380 doesn't provide separated actions for SA_IS_MCAST and SA_IS_BCAST. They use the same action.
 *
 *      Forwarding action is as following
 *      - ACTION_FORWARD
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 */
int32
dal_maple_l2_exceptionAddrAction_set(uint32 unit, rtk_l2_exceptionAddrType_t exceptType, rtk_action_t action)
{
    int32   ret;
    uint32  value;
    uint32 val_h, val_l;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, exceptType=%d, action=%d", unit, exceptType, action);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(((exceptType != SA_IS_BCAST_OR_MCAST) && (exceptType != SA_IS_ZERO)), RT_ERR_L2_EXCEPT_ADDR_TYPE);
    RT_PARAM_CHK(((action != ACTION_FORWARD) && (action != ACTION_DROP) && (action != ACTION_TRAP2CPU)), RT_ERR_INPUT);

    if(SA_IS_BCAST_OR_MCAST == exceptType)
    {
        if(action == ACTION_TRAP2CPU)
        {
            val_h =1; val_l = 0;
        }
        else if(action == ACTION_DROP)
        {
            val_h =0; val_l = 1;
        }
        else
        {
            val_h =0; val_l = 0;
        }
    }
    else
    {
        if(action == ACTION_TRAP2CPU)
            value = 2;
        else if(action == ACTION_DROP)
            value = 1;
        else
            value = 0;
    }

    L2_SEM_LOCK(unit);

    if(SA_IS_BCAST_OR_MCAST == exceptType)
    {
        if ((ret = reg_field_write(unit, MAPLE_L2_CTRL_0r, MAPLE_MC_BC_SA_TRAPf, &val_h)) != RT_ERR_OK)
        {
            L2_SEM_UNLOCK(unit);
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        if ((ret = reg_field_write(unit, MAPLE_L2_CTRL_0r, MAPLE_MC_BC_SA_DROPf, &val_l)) != RT_ERR_OK)
        {
            L2_SEM_UNLOCK(unit);
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }
    }
    else
    {
        /* programming value to CHIP*/
        if ((ret = reg_field_write(unit, MAPLE_L2_CTRL_0r, MAPLE_ZERO_SA_ACTf, &value)) != RT_ERR_OK)
        {
            L2_SEM_UNLOCK(unit);
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }
    }

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_exceptionAddrAction_set */

/* Function Name:
 *      dal_maple_l2_ipMcstFidVidCompareEnable_get
 * Description:
 *      Get VID/FID comparison configuration of IP multicast entry.
 * Input:
 *      unit    - unit id
 * Output:
 *      pEnable - pointer to FID/VID comparison status for IPv4 multicast packet
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
dal_maple_l2_ipMcstFidVidCompareEnable_get(uint32 unit, rtk_enable_t *pEnable)
{
    int32   ret;
    uint32  enable;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d", unit);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((NULL == pEnable), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if ((ret = reg_field_read(unit, MAPLE_L2_CTRL_0r, MAPLE_IP_MC_FVID_CMPf, &enable)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }
    L2_SEM_UNLOCK(unit);

    /* translate chip's value to definition */
    if (1 == enable)
    {
        *pEnable = ENABLED;
    }
    else
    {
        *pEnable = DISABLED;
    }

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pEnable=%d", *pEnable);

    return RT_ERR_OK;
} /* end of dal_maple_l2_ipMcstFidVidCompareEnable_get */

/* Function Name:
 *      dal_maple_l2_ipMcstFidVidCompareEnable_set
 * Description:
 *      Get VID/FID comparison configuration of IP multicast entry.
 * Input:
 *      unit   - unit id
 *      enable - configure value
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_INPUT    - invalid type of lookup miss
 * Note:
 *      None
 */
int32
dal_maple_l2_ipMcstFidVidCompareEnable_set(uint32 unit, rtk_enable_t enable)
{
    int32   ret;
    uint32  value;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, enable=%d",
           unit, enable);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((ENABLED != enable && DISABLED != enable), RT_ERR_INPUT);

    /* translate definition to chip's value  */
    if (ENABLED == enable)
    {
        value = 1;
    }
    else
    {
        value = 0;
    }

    L2_SEM_LOCK(unit);

    /* programming value to CHIP*/
    if ((ret = reg_field_write(unit, MAPLE_L2_CTRL_0r, MAPLE_IP_MC_FVID_CMPf, &value)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }
    L2_SEM_UNLOCK(unit);

    ipmcst_fvid_cmp = enable;

    return RT_ERR_OK;
} /* end of dal_maple_l2_ipMcstFidVidCompareEnable_set */

/* Function Name:
 *      dal_maple_l2_zeroSALearningEnable_get
 * Description:
 *      Get enable status of all-zero-SA learning.
 * Input:
 *      unit    - unit id
 * Output:
 *      pEnable - pointer to enable status of CPU tag adding
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
dal_maple_l2_zeroSALearningEnable_get(uint32 unit, rtk_enable_t *pEnable)
{
    int32   ret;
    uint32  enable;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d", unit);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((NULL == pEnable), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if ((ret = reg_field_read(unit, MAPLE_L2_CTRL_0r, MAPLE_SA_ALL_ZERO_LRNf, &enable)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }
    L2_SEM_UNLOCK(unit);

    /* translate chip's value to definition */
    if (1 == enable)
    {
        *pEnable = ENABLED;
    }
    else
    {
        *pEnable = DISABLED;
    }

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pEnable=%d", *pEnable);

    return RT_ERR_OK;
} /* end of dal_maple_l2_zeroSALearningEnable_get */

/* Function Name:
 *      dal_maple_l2_zeroSALearningEnable_set
 * Description:
 *      Set enable status of all-zero-SA learning.
 * Input:
 *      unit   - unit id
 *      enable - enable status of CPU tag adding
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      None
 */
int32
dal_maple_l2_zeroSALearningEnable_set(uint32 unit, rtk_enable_t enable)
{
    int32   ret;
    uint32  value;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, enable=%d",
           unit, enable);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((ENABLED != enable && DISABLED != enable), RT_ERR_INPUT);

    /* translate definition to chip's value  */
    if (ENABLED == enable)
    {
        value = 1;
    }
    else
    {
        value = 0;
    }

    L2_SEM_LOCK(unit);

    /* programming value to CHIP*/
    if ((ret = reg_field_write(unit, MAPLE_L2_CTRL_0r, MAPLE_SA_ALL_ZERO_LRNf, &value)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }
    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_zeroSALearningEnable_set */

/* Function Name:
 *      dal_maple_l2_vlanMode_get
 * Description:
 *      Get vlan(inner/outer vlan) for L2 lookup on specified port.
 * Input:
 *      unit      - unit id
 *      port      - port id
 * Output:
 *      pVlanMode - pointer to inner/outer vlan
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
dal_maple_l2_vlanMode_get(uint32 unit, rtk_port_t port, rtk_l2_vlanMode_t *pVlanMode)
{
    int32   ret;
    uint32  value;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d", unit);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(!HWP_PORT_EXIST(unit, port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((NULL == pVlanMode), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    if((ret = reg_array_field_read(unit, MAPLE_VLAN_PORT_FWDr,
                    port, REG_ARRAY_INDEX_NONE, MAPLE_FWD_BASEf, &value)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    /* translate chip's value to definition */
    switch (value)
    {
        case 0:
            *pVlanMode = BASED_ON_INNER_VLAN;
            break;
        case 1:
            *pVlanMode = BASED_ON_OUTER_VLAN;
            break;
        default:
            return RT_ERR_FAILED;
    }

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pVlanMode=%d", *pVlanMode);

    return RT_ERR_OK;
}/* end of dal_maple_l2_vlanMode_get */

/* Function Name:
 *      dal_maple_l2_vlanMode_set
 * Description:
 *      Set vlan(inner/outer vlan) for L2 lookup on specified port.
 * Input:
 *      unit     - unit id
 *      port     - port id
 *      vlanMode - inner/outer vlan
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_PORT_ID  - invalid port id
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      None
 */
int32
dal_maple_l2_vlanMode_set(uint32 unit, rtk_port_t port, rtk_l2_vlanMode_t vlanMode)
{
    int32   ret;
    uint32  value;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, vlanMode=%d", unit, vlanMode);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(!HWP_PORT_EXIST(unit, port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((vlanMode >= FWD_VLAN_MODE_END), RT_ERR_INPUT);

    /* translate value to chip's definition */
    switch (vlanMode)
    {
        case BASED_ON_INNER_VLAN:
            value = 0;
            break;
        case BASED_ON_OUTER_VLAN:
            value = 1;
            break;
        default:
            return RT_ERR_INPUT;
    }

    L2_SEM_LOCK(unit);

    if((ret = reg_array_field_write(unit, MAPLE_VLAN_PORT_FWDr,
                    port, REG_ARRAY_INDEX_NONE, MAPLE_FWD_BASEf, &value)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
}/*end of dal_maple_l2_vlanMode_set*/

/* Function Name:
 *      dal_maple_l2_mcastFwdIndex_alloc
 * Description:
 *      Allocate index for multicast forwarding entry
 * Input:
 *      unit      - unit id
 *      pFwdIndex - pointer to index of multicast forwarding entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT                 - The module is not initial
 *      RT_ERR_NULL_POINTER             - input parameter may be null pointer
 *      RT_ERR_L2_MULTI_FWD_INDEX       - invalid index of multicast forwarding entry
 *      RT_ERR_L2_MCAST_FWD_ENTRY_EXIST - index of forwarding entry is used.
 * Note:
 *      If *pFwdIndex is larger than or equal to 0, will use *pFwdIndex as multicast index.
 *      If *pFwdIndex is smaller than 0, will allocate a free index and return it.
 */
int32
dal_maple_l2_mcastFwdIndex_alloc(
    uint32  unit,
    int32   *pFwdIndex)
{
    RT_PARAM_CHK(NULL == pFwdIndex, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK(*pFwdIndex >= (int32)mcast_idx_pool[unit].size_of_mcast_fwd_index, RT_ERR_L2_MULTI_FWD_INDEX);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, index=%d", unit, *pFwdIndex);

    if (*pFwdIndex >= 0)
    {
        if (RT_ERR_OK == _dal_maple_l2_isMcastIdxUsed(unit, *pFwdIndex))
        {
            return RT_ERR_L2_MCAST_FWD_ENTRY_EXIST;
        }
    }

    return _dal_maple_l2_allocMcastIdx(unit, pFwdIndex);
} /* end of dal_maple_l2_mcastFwdIndex_alloc */

/* Function Name:
 *      dal_maple_l2_mcastFwdIndex_free
 * Description:
 *      Free index for multicast forwarding entry
 * Input:
 *      unit  - unit id
 *      index - index of multicast forwarding entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT                     - The module is not initial
 *      RT_ERR_L2_MULTI_FWD_INDEX           - invalid index of multicast forwarding portmask
 *      RT_ERR_L2_MCAST_FWD_ENTRY_NOT_EXIST - index of forwarding entry is not exist
 * Note:
 */
int32
dal_maple_l2_mcastFwdIndex_free(
    uint32  unit,
    int32   index)
{
    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, index=%d", unit, index);

    RT_PARAM_CHK((index >= (int32)mcast_idx_pool[unit].size_of_mcast_fwd_index) || index < 0, RT_ERR_L2_MULTI_FWD_INDEX);

    if (RT_ERR_OK != _dal_maple_l2_isMcastIdxUsed(unit, index))
    {
        return RT_ERR_L2_MCAST_FWD_ENTRY_NOT_EXIST;
    }

    return _dal_maple_l2_freeMcastIdx(unit, index);
} /* end of dal_maple_l2_mcastFwdIndex_free */

/* Function Name:
 *      dal_maple_l2_mcastFwdIndexFreeCount_get
 * Description:
 *      Get free count of multicast forwarding entry
 * Input:
 *      unit       - unit id
 *      pFreeCount - pointer to free count of multicast forwarding entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
dal_maple_l2_mcastFwdIndexFreeCount_get(uint32 unit, uint32 *pFreeCount)
{
    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(NULL == pFreeCount, RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);
    *pFreeCount = mcast_idx_pool[unit].free_entry_count;
    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_mcastFwdIndexFreeCount_get */

/* Function Name:
 *      dal_maple_l2_mcastFwdPortmask_set
 * Description:
 *      Get portmask of multicast forwarding entry
 * Input:
 *      unit      - unit id
 *      index     - index of multicast forwarding portmask
 *      pPortmask - pointer buffer of ip multicast ports
 *      crossVlan - cross vlan flag of ip multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT           - The module is not initial
 *      RT_ERR_NULL_POINTER       - input parameter may be null pointer
 *      RT_ERR_L2_MULTI_FWD_INDEX - invalid index of multicast forwarding portmask
 * Note:
 *      None.
 */
int32
dal_maple_l2_mcastFwdPortmask_set(
    uint32          unit,
    int32           index,
    rtk_portmask_t  *pPortmask,
    uint32          crossVlan)
{
    int32 ret;
    multicast_index_entry_t mcast_entry;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(index >= mcast_idx_pool[unit].size_of_mcast_fwd_index, RT_ERR_L2_MULTI_FWD_INDEX);
    RT_PARAM_CHK(NULL == pPortmask, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK(!HWP_PMSK_EXIST(unit, pPortmask), RT_ERR_PORT_MASK);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dal_maple_l2_mcastFwdPortmask_set: unit=%d, index=%d, pPortmask==0x%x",
           unit, index, pPortmask->bits[0]);

    osal_memset(&mcast_entry, 0, sizeof(mcast_entry));

    if ((ret = table_field_set(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, &(pPortmask->bits[0]), (uint32 *) &mcast_entry)) != RT_ERR_OK)
    {
         RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
         return ret;
    }

    L2_SEM_LOCK(unit);
    if ((ret = table_write(unit, MAPLE_MC_PMSKt, (uint32)index, (uint32 *)&mcast_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }
    L2_SEM_UNLOCK(unit);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dal_maple_l2_mcastFwdPortmask_set: pPortmask==0x%x", pPortmask->bits[0]);

    return RT_ERR_OK;
} /* end of dal_maple_l2_mcastFwdPortmask_set */

/* Function Name:
 *      dal_maple_l2_mcastFwdPortmask_get
 * Description:
 *      Get portmask of multicast forwarding entry
 * Input:
 *      unit       - unit id
 *      index      - index of multicast forwarding portmask
 * Output:
 *      pPortmask  - pointer buffer of ip multicast ports
 *      pCrossVlan - pointer of cross vlan flag
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT           - The module is not initial
 *      RT_ERR_NULL_POINTER       - input parameter may be null pointer
 *      RT_ERR_L2_MULTI_FWD_INDEX - invalid index of multicast forwarding portmask
 * Note:
 *      None.
 */
int32
dal_maple_l2_mcastFwdPortmask_get(
    uint32          unit,
    int32           index,
    rtk_portmask_t  *pPortmask,
    uint32          *pCrossVlan)
{
    int32 ret;
    multicast_index_entry_t mcast_entry;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(index >= mcast_idx_pool[unit].size_of_mcast_fwd_index, RT_ERR_L2_MULTI_FWD_INDEX);
    RT_PARAM_CHK(NULL == pPortmask, RT_ERR_NULL_POINTER);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dal_maple_l2_mcastFwdPortmask_get: unit=%d, pPortmask=%x", unit, index);

    osal_memset(&mcast_entry, 0, sizeof(mcast_entry));

    L2_SEM_LOCK(unit);
    if ((ret = table_read(unit, MAPLE_MC_PMSKt, index, (uint32 *)&mcast_entry)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }
    L2_SEM_UNLOCK(unit);

    osal_memset(pPortmask, 0, sizeof(rtk_portmask_t));
    if ((ret = table_field_get(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, &(pPortmask->bits[0]), (uint32 *) &mcast_entry)) != RT_ERR_OK)
    {
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return ret;
    }

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "dal_maple_l2_mcastFwdPortmask_get: pPortmask=0x%x", pPortmask->bits[0]);

    return RT_ERR_OK;
} /* end of dal_maple_l2_mcastFwdPortmask_get */

/* Function Name:
 *      dal_maple_l2_portAgingEnable_get
 * Description:
 *      Set the dynamic address aging out configuration of the specified port to the specified device.
 * Input:
 *      unit    - unit id
 *      port    - port id
 * Output:
 *      pEnable - enable status of aging out
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
dal_maple_l2_portAgingEnable_get(uint32 unit, rtk_port_t port, rtk_enable_t *pEnable)
{
    int32   ret;
    uint32  enable;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, port=%d", unit, port);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(!HWP_PORT_EXIST(unit, port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((NULL == pEnable), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if ((ret = reg_array_field_read(unit, MAPLE_L2_PORT_AGING_OUTr, port, REG_ARRAY_INDEX_NONE, MAPLE_AGING_OUT_ENf, &enable)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    /* translate chip's value to definition */
    if (1 == enable)
    {
        *pEnable = ENABLED;
    }
    else
    {
        *pEnable = DISABLED;
    }

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pEnable=%d", *pEnable);

    return RT_ERR_OK;
} /* end of dal_maple_l2_portAgingEnable_get */

/* Function Name:
 *      dal_maple_l2_portAgingEnable_set
 * Description:
 *      Set the dynamic address aging out configuration of the specified port to the specified device.
 * Input:
 *      unit   - unit id
 *      port   - port id
 *      enable - enable status of aging out
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_PORT_ID  - invalid port id
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      None
 */
int32
dal_maple_l2_portAgingEnable_set(uint32 unit, rtk_port_t port, rtk_enable_t enable)
{
    int32   ret;
    uint32  value;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, port=%d, enable=%d", unit, port, enable);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(!HWP_PORT_EXIST(unit, port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((ENABLED != enable && DISABLED != enable), RT_ERR_INPUT);

    /* translate definition to chip's value  */
    if (ENABLED == enable)
    {
        value = 1;
    }
    else
    {
        value = 0;
    }

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if ((ret = reg_array_field_write(unit, MAPLE_L2_PORT_AGING_OUTr, port, REG_ARRAY_INDEX_NONE, MAPLE_AGING_OUT_ENf, &value)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_portAgingEnable_set */

/* Function Name:
 *      dal_maple_l2_portLegalPortMoveAction_get
 * Description:
 *      Get forwarding action when port moving happen on specified port which is learnt as dynamic entry.
 * Input:
 *      unit       - unit id
 *      port       - port id
 * Output:
 *      pFwdAction - pointer to forwarding action
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
dal_maple_l2_portLegalPortMoveAction_get(uint32 unit, rtk_port_t port, rtk_action_t *pFwdAction)
{
    int32   ret;
    uint32  act;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, port=%d", unit, port);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(!HWP_PORT_EXIST(unit, port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((NULL == pFwdAction), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if ((ret = reg_array_field_read(unit, MAPLE_L2_PORT_MV_ACTr, port, REG_ARRAY_INDEX_NONE, MAPLE_ACTf, &act)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    /* translate register value to action */
    switch (act)
    {
        case ACTION_FORWARD:
            *pFwdAction = 0;
            break;
        case ACTION_DROP:
            *pFwdAction = 1;
            break;
        case ACTION_TRAP2CPU:
            *pFwdAction = 2;
            break;
        case ACTION_COPY2CPU:
            *pFwdAction = 3;
            break;
        default:
            return RT_ERR_FAILED;
    }

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pFwdAction=%d", *pFwdAction);

    return RT_ERR_OK;
} /* end of dal_maple_l2_portLegalPortMoveAction_get */

/* Function Name:
 *      dal_maple_l2_portLegalPortMoveAction_set
 * Description:
 *      Set forwarding action when legal port moving happen on specified port which is learnt as dynamic entry.
 * Input:
 *      unit      - unit id
 *      port      - port id
 *      fwdAction - forwarding action
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT   - The module is not initial
 *      RT_ERR_PORT_ID    - invalid port id
 *      RT_ERR_FWD_ACTION - invalid forwarding action
 * Note:
 *      None
 */
int32
dal_maple_l2_portLegalPortMoveAction_set(uint32 unit, rtk_port_t port, rtk_action_t fwdAction)
{
    int32   ret;
    uint32  act;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, port=%d, fwdAction=%d", unit, port, fwdAction);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(!HWP_PORT_EXIST(unit, port), RT_ERR_PORT_ID);
    RT_PARAM_CHK(fwdAction > ACTION_COPY2CPU, RT_ERR_FWD_ACTION);

    switch (fwdAction)
    {
        case ACTION_FORWARD:
            act = 0;
            break;
        case ACTION_DROP:
            act = 1;
            break;
        case ACTION_TRAP2CPU:
            act = 2;
            break;
        case ACTION_COPY2CPU:
            act = 3;
            break;
        default:
            return RT_ERR_FWD_ACTION;
    }

    L2_SEM_LOCK(unit);

    /* programming value to CHIP*/
    if ((ret = reg_array_field_write(unit, MAPLE_L2_PORT_MV_ACTr, port, REG_ARRAY_INDEX_NONE, MAPLE_ACTf, &act)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_portLegalPortMoveAction_set */

/* Function Name:
 *      dal_maple_l2_legalPortMoveFlushAddrEnable_get
 * Description:
 *      Get the configuration of HW flush moved-port's mac of the specified device.
 * Input:
 *      unit    - unit id
 *      port    - port id
 * Output:
 *      pEnable - enable status
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
dal_maple_l2_legalPortMoveFlushAddrEnable_get(uint32 unit, rtk_port_t port, rtk_enable_t *pEnable)
{
    int32   ret;
    uint32  value;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, port=%d", unit, port);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(!HWP_PORT_EXIST(unit, port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((NULL == pEnable), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if ((ret = reg_array_field_read(unit, MAPLE_L2_PORT_MV_INVALIDATEr, port, REG_ARRAY_INDEX_NONE, MAPLE_P_MV_INVLDf, &value)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    /* translate register value */
    if (1 == value)
    {
        *pEnable = ENABLED;
    }
    else
    {
        *pEnable = DISABLED;
    }

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pEnable=%d", *pEnable);

    return RT_ERR_OK;
} /* end of dal_maple_l2_legalPortMoveFlushAddrEnable_get */

/* Function Name:
 *      dal_maple_l2_legalPortMoveFlushAddrEnable_set
 * Description:
 *      Set the configuration of HW flush moved-port's mac of the specified device.
 * Input:
 *      unit   - unit id
 *      port   - port id
 *      enable - enable status
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT   - The module is not initial
 *      RT_ERR_PORT_ID    - invalid port id
 *      RT_ERR_FWD_ACTION - invalid forwarding action
 * Note:
 *      None
 */
int32
dal_maple_l2_legalPortMoveFlushAddrEnable_set(uint32 unit, rtk_port_t port, rtk_enable_t enable)
{
    int32   ret;
    uint32  value;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, port=%d, enable=%d", unit, port, enable);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(!HWP_PORT_EXIST(unit, port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((ENABLED != enable && DISABLED != enable), RT_ERR_INPUT);

    if (ENABLED == enable)
    {
        value = 1;
    }
    else
    {
        value = 0;
    }

    L2_SEM_LOCK(unit);

    /* programming value to CHIP*/
    if ((ret = reg_array_field_write(unit, MAPLE_L2_PORT_MV_INVALIDATEr, port, REG_ARRAY_INDEX_NONE, MAPLE_P_MV_INVLDf, &value)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_legalPortMoveFlushAddrEnable_set */

/* Function Name:
 *      dal_maple_l2_ip6CareByte_get
 * Description:
 *      Get the hash care-byte of IPv6 DIP/SIP address. These bytes are used to compose the LUT hash key.
 * Input:
 *      unit      - unit id
 *      type      - the type of care-byte. May be either SIP or DIP.
 * Output:
 *      pCareByte - pointer to the care-byte of IPv6 DIP/SIP address
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_INPUT        - invalid input parameter
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
dal_maple_l2_ip6CareByte_get(uint32 unit, rtk_l2_ip6_careByte_type_t type, uint32 *pCareByte)
{
    int32   ret;
    uint32  field;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, type=%d", unit, type);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(type >= L2_CARE_BYTE_END, RT_ERR_INPUT);
    RT_PARAM_CHK((NULL == pCareByte), RT_ERR_NULL_POINTER);

    switch (type)
    {
        case L2_DIP_HASH_CARE_BYTE:
            field = MAPLE_DIP_CARE_BYTEf;
            break;
        case L2_SIP_HASH_CARE_BYTE:
            field = MAPLE_SIP_CARE_BYTEf;
            break;
        default:
            return RT_ERR_INPUT;
    }

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if ((ret = reg_field_read(unit, MAPLE_L2_IPV6_MC_IP_CARE_BYTEr, field, pCareByte)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pCareByte=%#x", *pCareByte);

    return RT_ERR_OK;
} /* end of dal_maple_l2_ip6CareByte_get */

/* Function Name:
 *      dal_maple_l2_ip6CareByte_set
 * Description:
 *      Set the hash care-byte of IPv6 DIP/SIP address. These bytes are used to compose the LUT hash key.
 * Input:
 *      unit     - unit id
 *      type     - the type of care-byte. May be either SIP or DIP
 *      careByte - the care-byte of IPv6 DIP/SIP address
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      None
 */
int32
dal_maple_l2_ip6CareByte_set(uint32 unit, rtk_l2_ip6_careByte_type_t type, uint32 careByte)
{
    int32   ret;
    uint32  field;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, careByte=%d", unit, careByte);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(type >= L2_CARE_BYTE_END, RT_ERR_INPUT);

    switch (type)
    {
        case L2_DIP_HASH_CARE_BYTE:
            field = MAPLE_DIP_CARE_BYTEf;
            break;
        case L2_SIP_HASH_CARE_BYTE:
            field = MAPLE_SIP_CARE_BYTEf;
            break;
        default:
            return RT_ERR_INPUT;
    }

    L2_SEM_LOCK(unit);

    if((ret = reg_field_write(unit, MAPLE_L2_IPV6_MC_IP_CARE_BYTEr, field, &careByte)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if (type == L2_DIP_HASH_CARE_BYTE)
        ip6DipCareByte = careByte;
    else
        ip6SipCareByte = careByte;

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
}/* end of dal_maple_l2_ip6CareByte_set */

/* Function Name:
 *      dal_maple_l2_hashAlgo_get
 * Description:
 *      Get hash algorithm of layer2 table.
 * Input:
 *      unit        - unit id
 * Output:
 *      pHash_algo  - pointer to hash algorithm of layer2 switching
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_UNIT_ID      - invalid unit id
 *      RT_ERR_BUCKET_ID    - invalid bucket id
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      none.
 */
int32
dal_maple_l2_hashAlgo_get(uint32 unit, uint32 *pHash_algo)
{
    return dal_maple_l2_bucketHashAlgo_get(unit, 0, pHash_algo);
}

/* Function Name:
 *      dal_maple_l2_hashAlgo_set
 * Description:
 *      Set hash algorithm of layer2 table.
 * Input:
 *      unit        - unit id
 * Output:
 *      pHash_algo  - pointer to hash algorithm of layer2 switching
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_UNIT_ID      - invalid unit id
 *      RT_ERR_BUCKET_ID    - invalid bucket id
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      none.
 */
int32
dal_maple_l2_hashAlgo_set(uint32 unit, uint32 hash_algo)
{
    return dal_maple_l2_bucketHashAlgo_set(unit, 0, hash_algo);
}

/* Function Name:
 *      dal_maple_l2_bucketHashAlgo_get
 * Description:
 *      Get bucket hash algorithm of layer2 table.
 * Input:
 *      unit        - unit id
 *      bucket      - bucket id
 * Output:
 *      pHash_algo  - pointer to hash algorithm of layer2 switching
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_UNIT_ID      - invalid unit id
 *      RT_ERR_BUCKET_ID    - invalid bucket id
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      bucket can only be 0.
 */
int32
dal_maple_l2_bucketHashAlgo_get(uint32 unit, uint32 bucket, uint32 *pHash_algo)
{
    int32   ret;
    uint32  value = 0;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d", unit);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((NULL == pHash_algo), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((bucket > 0), RT_ERR_INPUT);

    L2_SEM_LOCK(unit);

    if((ret = reg_field_read(unit, MAPLE_L2_CTRL_0r, MAPLE_L2_HASH_ALGOf, &value)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    *pHash_algo = value;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pHash_algo=%d", *pHash_algo);

    return RT_ERR_OK;
}

/* Function Name:
 *      dal_maple_l2_bucketHashAlgo_set
 * Description:
 *      Set bucket hash algorithm of layer2 table.
 * Input:
 *      unit        - unit id
 *      bucket      - bucket id
 *      hash_algo   - hash algorithm of layer2 switching
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_UNIT_ID      - invalid unit id
 *      RT_ERR_BUCKET_ID    - invalid bucket id
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_INPUT        - invalid input parameter
 * Note:
 *      The valid bucket can only be 0, and hash_algo is 0 and 1
 */
int32
dal_maple_l2_bucketHashAlgo_set(uint32 unit, uint32 bucket, uint32 hash_algo)
{
    int32   ret;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d  hash_algo=%d", unit, hash_algo);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);
    RT_PARAM_CHK((bucket > 0), RT_ERR_INPUT);
    RT_PARAM_CHK((hash_algo >= HAL_MAX_NUM_OF_L2_HASH_ALGO(unit)), RT_ERR_INPUT);

    L2_SEM_LOCK(unit);

    if((ret = reg_field_write(unit, MAPLE_L2_CTRL_0r, MAPLE_L2_HASH_ALGOf, &hash_algo)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    algoType[unit] = hash_algo;

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
}

/* Function Name:
 *      dal_maple_l2_secureMacMode_get
 * Description:
 *      Get enable status of SA secure Mac mode.
 * Input:
 *      unit    - unit id
 * Output:
 *      pEnable - pointer to enable status of CAM entry
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
dal_maple_l2_secureMacMode_get(uint32 unit, rtk_enable_t *pEnable)
{
    int32   ret;
    uint32  value;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d", unit);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((NULL == pEnable), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if((ret = reg_field_read(unit, MAPLE_L2_CTRL_0r, MAPLE_SECURE_SAf, &value)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    /* translate chip's value to definition */
    if (1 == value)
    {
        *pEnable = ENABLED;
    }
    else
    {
        *pEnable = DISABLED;
    }

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pEnable=%d", *pEnable);

    return RT_ERR_OK;
} /* end of dal_maple_l2_secureMacMode_get */

/* Function Name:
 *      dal_maple_l2_secureMacMode_set
 * Description:
 *      Set enable status of SA secure Mac mode.
 * Input:
 *      unit   - unit id
 *      enable - enable status of SA secure Mac mode
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      None
 */
int32
dal_maple_l2_secureMacMode_set(uint32 unit, rtk_enable_t enable)
{
    int32   ret;
    uint32  value;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, enable=%d", unit, enable);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((ENABLED != enable && DISABLED != enable), RT_ERR_INPUT);

    /* translate definition to chip's value  */
    if (ENABLED == enable)
    {
        value = 1;
    }
    else
    {
        value = 0;
    }

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if((ret = reg_field_write(unit, MAPLE_L2_CTRL_0r, MAPLE_SECURE_SAf, &value)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_secureMacMode_set */

/* Function Name:
 *      dal_maple_l2_macLimitEnable_get
 * Description:
 *      Get enable status of mac limit mode.
 * Input:
 *      unit    - unit id
 * Output:
 *      pEnable - pointer to enable status of mac limit
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
dal_maple_l2_macLimitEnable_get(uint32 unit, rtk_enable_t *pEnable)
{
    int32   ret;
    uint32  value;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d", unit);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((NULL == pEnable), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if ((ret = reg_field_read(unit, MAPLE_L2_LRN_CONSTRT_ENr, MAPLE_SYS_CONSTRT_ENf, &value)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    /* translate chip's value to definition */
    if (1 == value)
    {
        *pEnable = ENABLED;
    }
    else
    {
        *pEnable = DISABLED;
    }

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pEnable=%d", *pEnable);

    return RT_ERR_OK;
} /* end of dal_maple_l2_macLimitEnable_get */

/* Function Name:
 *      dal_maple_l2_macLimitEnable_set
 * Description:
 *      Set enable status of mac limit mode.
 * Input:
 *      unit   - unit id
 *      enable - enable status of mac limit mode
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      None
 */
int32
dal_maple_l2_macLimitEnable_set(uint32 unit, rtk_enable_t enable)
{
    int32   ret;
    uint32  value;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, enable=%d", unit, enable);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((ENABLED != enable && DISABLED != enable), RT_ERR_INPUT);

    /* translate definition to chip's value  */
    if (ENABLED == enable)
    {
        value = 1;
    }
    else
    {
        value = 0;
    }

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if ((ret = reg_field_write(unit, MAPLE_L2_LRN_CONSTRT_ENr, MAPLE_SYS_CONSTRT_ENf, &value)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_macLimitEnable_set */

/* Function Name:
 *      dal_maple_l2_macLimitEnable_get
 * Description:
 *      Get enable status of port mac limit mode.
 * Input:
 *      unit    - unit id
 * Output:
 *      pEnable - pointer to enable status of port mac limit
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
dal_maple_l2_portMacLimitEnable_get(uint32 unit, rtk_enable_t *pEnable)
{
    int32   ret;
    uint32  value;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d", unit);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((NULL == pEnable), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if ((ret = reg_field_read(unit, MAPLE_L2_LRN_CONSTRT_ENr, MAPLE_PORT_CONSTRT_ENf, &value)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    /* translate chip's value to definition */
    if (1 == value)
    {
        *pEnable = ENABLED;
    }
    else
    {
        *pEnable = DISABLED;
    }

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pEnable=%d", *pEnable);

    return RT_ERR_OK;
} /* end of dal_maple_l2_portMacLimitEnable_get */

/* Function Name:
 *      dal_maple_l2_portMacLimitEnable_set
 * Description:
 *      Set enable status of port mac limit mode.
 * Input:
 *      unit   - unit id
 *      enable - enable status of port mac limit mode
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      None
 */
int32
dal_maple_l2_portMacLimitEnable_set(uint32 unit, rtk_enable_t enable)
{
    int32   ret;
    uint32  value;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, enable=%d", unit, enable);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((ENABLED != enable && DISABLED != enable), RT_ERR_INPUT);

    /* translate definition to chip's value  */
    if (ENABLED == enable)
    {
        value = 1;
    }
    else
    {
        value = 0;
    }

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if ((ret = reg_field_write(unit, MAPLE_L2_LRN_CONSTRT_ENr, MAPLE_PORT_CONSTRT_ENf, &value)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_portMacLimitEnable_set */

/* Function Name:
 *      dal_maple_l2_macLimitEnable_get
 * Description:
 *      Get enable status of fid mac limit mode.
 * Input:
 *      unit    - unit id
 * Output:
 *      pEnable - pointer to enable status of fid mac limit
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
dal_maple_l2_fidMacLimitEnable_get(uint32 unit, rtk_enable_t *pEnable)
{
    int32   ret;
    uint32  value;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d", unit);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((NULL == pEnable), RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if((ret = reg_field_read(unit, MAPLE_L2_LRN_CONSTRT_ENr, MAPLE_VLAN_CONSTRT_ENf, &value)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    /* translate chip's value to definition */
    if (1 == value)
    {
        *pEnable = ENABLED;
    }
    else
    {
        *pEnable = DISABLED;
    }

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pEnable=%d", *pEnable);

    return RT_ERR_OK;
} /* end of dal_maple_l2_fidMacLimitEnable_get */

/* Function Name:
 *      dal_maple_l2_portMacLimitEnable_set
 * Description:
 *      Set enable status of fid mac limit mode.
 * Input:
 *      unit   - unit id
 *      enable - enable status of fid mac limit mode
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      None
 */
int32
dal_maple_l2_fidMacLimitEnable_set(uint32 unit, rtk_enable_t enable)
{
    int32   ret;
    uint32  value;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, enable=%d", unit, enable);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK((ENABLED != enable && DISABLED != enable), RT_ERR_INPUT);

    /* translate definition to chip's value  */
    if (ENABLED == enable)
    {
        value = 1;
    }
    else
    {
        value = 0;
    }

    L2_SEM_LOCK(unit);

    /* get value from CHIP*/
    if((ret = reg_field_write(unit, MAPLE_L2_LRN_CONSTRT_ENr, MAPLE_VLAN_CONSTRT_ENf, &value)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    return RT_ERR_OK;
} /* end of dal_maple_l2_fidMacLimitEnable_set */

/* Function Name:
 *      dal_maple_l2_addrEntry_get
 * Description:
 *      Get the L2 table entry by index of the specified unit.
 * Input:
 *      unit      - unit id
 *      index     - l2 table index
 * Output:
 *      pL2_entry - pointer buffer of l2 table entry
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      1) The index valid range is from 0 to (L2 hash table size - 1)
 *         - 0 ~ (L2 hash table size - 1) entry in L2 hash table
 *      2) The output entry have 2 variables (valid and entry_type) and its detail data structure
 *         - valid: 1 mean the entry is valid; 0: invalid
 *         - entry_type: FLOW_TYPE_UNICAST, FLOW_TYPE_L2_MULTI, FLOW_TYPE_IP4_MULTI and FLOW_TYPE_IP6_MULTI
 *                       the field is ignored if valid field is 0.
 *         - detail data structure is ignored if valid is 0, and its field meanings are depended
 *           on the entry_type value.
 *      3) If pL2_entry->flags have enabled the RTK_L2_UCAST_FLAG_TRUNK_PORT flag, mean the
 *         pL2_entry->unicast.trk_gid value is valid trunk id value.
 */
int32
dal_maple_l2_addrEntry_get(uint32 unit, uint32 index, rtk_l2_entry_t *pL2_entry)
{
    uint32  hashkey = 0, location = 0, isValid = 0;
    int32   ret;
    dal_maple_l2_entry_t  l2_entry;
    multicast_index_entry_t mcast_entry;
    rtk_portmask_t      trunk_portmask;
    uint32              trk_gid;
    uint32              first_trunkMember;
    uint32              value;
    rtk_enable_t        l2CamEbl;

    ipaddr_t invalid_ipv4_addr = 0;

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d, index=%d", unit, index);

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* check index valid range */
    if ((ret = reg_field_read(unit, MAPLE_L2_CTRL_0r, MAPLE_LUTCAM_ENf, &value)) != RT_ERR_OK)
        return ret;
    l2CamEbl = value ? ENABLED : DISABLED;
    if ((index >= (hashTable_size[unit] + cam_size[unit])) || ((index >= hashTable_size[unit]) && (l2CamEbl == DISABLED)))
        return RT_ERR_INPUT;

    /* parameter check */
    RT_PARAM_CHK(NULL == pL2_entry, RT_ERR_NULL_POINTER);

    L2_SEM_LOCK(unit);

    /* L2 Hash Table Entry */
    if (index < hashTable_size[unit])
    {
        hashkey = (index >> 2) & 0xFFF;
        location = index & BITMASK_2B;
    }
    else
    {
        location = index - hashTable_size[unit];
    }

    l2_entry.entry_type = 0;

    if (index < hashTable_size[unit])
    {
        if ((ret = _dal_maple_l2_getL2EntryfromHash(unit, hashkey, location, &l2_entry, &isValid)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            L2_SEM_UNLOCK(unit);
            return ret;
        }
    }
    else
    {
        if ((ret = _dal_maple_l2_getL2EntryfromCAM(unit, location, &l2_entry, &isValid)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            L2_SEM_UNLOCK(unit);
            return ret;
        }
    }

    /* Transfer from dal_maple_l2_entry_t to rtk_l2_entry_t */
    pL2_entry->valid = isValid;
    if (pL2_entry->valid)
    {
        pL2_entry->entry_type = l2_entry.entry_type;
        switch (pL2_entry->entry_type)
        {
            case FLOW_TYPE_UNICAST:
                pL2_entry->unicast.vid = l2_entry.unicast.fid;
                osal_memcpy(&pL2_entry->unicast.mac, &l2_entry.unicast.mac, sizeof(rtk_mac_t));
                pL2_entry->unicast.port = l2_entry.unicast.port;
                pL2_entry->unicast.flags = 0;
                pL2_entry->unicast.state = 0;
                if(l2_entry.unicast.nh)
                {
                    pL2_entry->unicast.vlan_target = l2_entry.unicast.vlan_target;
                    pL2_entry->unicast.route_idx = l2_entry.unicast.route_idx;
                }
                else
                    pL2_entry->unicast.agg_vid = l2_entry.unicast.agg_vid;

                for (trk_gid = 0; trk_gid < HAL_MAX_NUM_OF_TRUNK(unit); trk_gid++)
                {
                    if (dal_maple_trunk_port_get(unit, trk_gid, &trunk_portmask) == RT_ERR_OK)
                    {
                        if ((first_trunkMember = RTK_PORTMASK_GET_FIRST_PORT(trunk_portmask)) == -1)
                        {
                            /* no trunk member */
                            continue;
                        }

                        if ((RTK_PORTMASK_IS_PORT_SET(trunk_portmask, pL2_entry->unicast.port)) &&
                            (first_trunkMember == pL2_entry->unicast.port))
                        {
                            pL2_entry->unicast.trk_gid = trk_gid;
                            pL2_entry->unicast.flags |= RTK_L2_UCAST_FLAG_TRUNK_PORT;
                            break;
                        }
                    }
                }
                if(l2_entry.unicast.sablock)
                    pL2_entry->unicast.flags |= RTK_L2_UCAST_FLAG_SA_BLOCK;
                if(l2_entry.unicast.dablock)
                    pL2_entry->unicast.flags |= RTK_L2_UCAST_FLAG_DA_BLOCK;
                if(l2_entry.unicast.is_static)
                    pL2_entry->unicast.flags |= RTK_L2_UCAST_FLAG_STATIC;
                if(l2_entry.unicast.nh)
                    pL2_entry->unicast.flags |= RTK_L2_UCAST_FLAG_NEXTHOP;
                if(l2_entry.unicast.suspending)
                    pL2_entry->unicast.state |= RTK_L2_UCAST_STATE_SUSPEND;
                if(l2_entry.unicast.aging == 0)
                    pL2_entry->unicast.isAged = TRUE;
                else
                    pL2_entry->unicast.isAged = FALSE;
                pL2_entry->unicast.age = l2_entry.unicast.aging;
                break;

            case FLOW_TYPE_L2_MULTI:
                pL2_entry->l2mcast.rvid = l2_entry.l2mcast.rvid;
                osal_memcpy(&pL2_entry->l2mcast.mac, &l2_entry.l2mcast.mac, sizeof(rtk_mac_t));
                pL2_entry->l2mcast.fwdIndex = l2_entry.l2mcast.index;
                osal_memset(&mcast_entry, 0, sizeof(mcast_entry));
                if ((ret = table_read(unit, MAPLE_MC_PMSKt, l2_entry.l2mcast.index, (uint32 *)&mcast_entry)) != RT_ERR_OK)
                {
                    L2_SEM_UNLOCK(unit);
                    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                if ((ret = table_field_get(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, pL2_entry->l2mcast.portmask.bits, (uint32 *) &mcast_entry)) != RT_ERR_OK)
                {
                    L2_SEM_UNLOCK(unit);
                    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                break;

            case FLOW_TYPE_IP4_MULTI:
                if (LOOKUP_ON_FVID_AND_MAC == ip4HashFmt[unit])
                {
                    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "It should not be existing an IP-multicast entry in FVID_AND_MAC mode");
                }
                else if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
                {
                    pL2_entry->ipmcast.rvid = l2_entry.ipmcast_ip_mc_sip.rvid;
                    pL2_entry->ipmcast.dip = l2_entry.ipmcast_ip_mc_sip.dip;
                    pL2_entry->ipmcast.sip = l2_entry.ipmcast_ip_mc_sip.sip;
                    pL2_entry->ipmcast.fwdIndex = l2_entry.ipmcast_ip_mc_sip.index;
                }
                else if (LOOKUP_ON_DIP_AND_FVID == ip4HashFmt[unit])
                {
                    pL2_entry->ipmcast.rvid = l2_entry.ipmcast_ip_mc.rvid;
                    pL2_entry->ipmcast.dip = l2_entry.ipmcast_ip_mc.dip;
                    pL2_entry->ipmcast.fwdIndex = l2_entry.ipmcast_ip_mc.index;
                }
                else
                {
                    L2_SEM_UNLOCK(unit);
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return RT_ERR_FAILED;
                }

                osal_memset(&mcast_entry, 0, sizeof(mcast_entry));
                if ((ret = table_read(unit, MAPLE_MC_PMSKt, pL2_entry->ipmcast.fwdIndex, (uint32 *)&mcast_entry)) != RT_ERR_OK)
                {
                    L2_SEM_UNLOCK(unit);
                    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                if ((ret = table_field_get(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, pL2_entry->ipmcast.portmask.bits, (uint32 *) &mcast_entry)) != RT_ERR_OK)
                {
                    L2_SEM_UNLOCK(unit);
                    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                break;

            case FLOW_TYPE_IP6_MULTI:
                if (LOOKUP_ON_FVID_AND_MAC == ip6HashFmt[unit])
                {
                    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "It should not be existing an IP-multicast entry in FVID_AND_MAC mode");
                }
                else if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
                {
                    pL2_entry->ip6mcast.rvid = l2_entry.ipmcast_ip_mc_sip.rvid;
                    pL2_entry->ip6mcast.fwdIndex = l2_entry.ipmcast_ip_mc_sip.index;
                    _dal_maple_l2_ip6CareByteConvertToIP6Addr(&pL2_entry->ip6mcast.dip, &pL2_entry->ip6mcast.sip, &l2_entry.ipmcast_ip_mc_sip.dip, &l2_entry.ipmcast_ip_mc_sip.sip);
                }
                else if (LOOKUP_ON_DIP_AND_FVID == ip6HashFmt[unit])
                {
                    pL2_entry->ip6mcast.rvid = l2_entry.ipmcast_ip_mc.rvid;
                    pL2_entry->ip6mcast.fwdIndex = l2_entry.ipmcast_ip_mc.index;
                    _dal_maple_l2_ip6CareByteConvertToIP6Addr(&pL2_entry->ip6mcast.dip, &pL2_entry->ip6mcast.sip, &l2_entry.ipmcast_ip_mc.dip, &invalid_ipv4_addr);
                    osal_memset(&pL2_entry->ip6mcast.sip, 0, sizeof(rtk_ipv6_addr_t));
                }
                else
                {
                    L2_SEM_UNLOCK(unit);
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return RT_ERR_FAILED;
                }
                osal_memset(&mcast_entry, 0, sizeof(mcast_entry));
                if ((ret = table_read(unit, MAPLE_MC_PMSKt, pL2_entry->ip6mcast.fwdIndex, (uint32 *)&mcast_entry)) != RT_ERR_OK)
                {
                    L2_SEM_UNLOCK(unit);
                    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                if ((ret = table_field_get(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, pL2_entry->ip6mcast.portmask.bits, (uint32 *) &mcast_entry)) != RT_ERR_OK)
                {
                    L2_SEM_UNLOCK(unit);
                    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                break;

            default:
                L2_SEM_UNLOCK(unit);
                return RT_ERR_FAILED;
        }
    }

    L2_SEM_UNLOCK(unit);
    return RT_ERR_OK;
} /* end of dal_maple_l2_addrEntry_get */

/* Function Name:
 *      dal_maple_l2_conflictAddr_get
 * Description:
 *      Get the conflict L2 table entry from one given L2 address in the specified unit.
 * Input:
 *      unit            - unit id
 *      pL2Addr         - l2 address to find its conflict entries
 *      cfAddrList_size - buffer size of the pCfAddrList
 * Output:
 *      pCfAddrList     - pointer buffer of the conflict l2 table entry list
 *      pCf_retCnt      - return number of find conflict l2 table entry list
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_OUT_OF_RANGE - input parameter out of range
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      1) The function can be used if add l2 entry return RT_ERR_L2_NO_EMPTY_ENTRY.
 *         Input the pL2Addr->entry_type and its hash key to get conflict entry information.
 *      2) User want to prepare the return buffer pCfAddrList and via. cfAddrList_size argument
 *         tell driver its size.
 *      3) The function will return valid L2 hash entry from the same bucket and the return number
 *         is filled in pCf_retCnt, entry data is filled in pCfAddrList.
 */
int32
dal_maple_l2_conflictAddr_get(
    uint32          unit,
    rtk_l2_entry_t  *pL2Addr,
    rtk_l2_entry_t  *pCfAddrList,
    uint32          cfAddrList_size,
    uint32          *pCf_retCnt)
{
    uint32  hash_key = 0, hash_depth;
    uint32  isValid;
    uint32  cf_num;
    uint32  trk_gid, first_trunkMember;
    rtk_portmask_t  trunk_portmask;
    int32   ret = RT_ERR_FAILED;
    dal_maple_l2_entry_t  l2_entry;
    multicast_index_entry_t mcast_entry;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* check input parameters */
    RT_PARAM_CHK(NULL == pL2Addr, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK(NULL == pCfAddrList, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK(0 == cfAddrList_size, RT_ERR_INPUT);
    RT_PARAM_CHK(NULL == pCf_retCnt, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK(pL2Addr->entry_type >= FLOW_TYPE_END, RT_ERR_OUT_OF_RANGE);

    osal_memset(&l2_entry, 0, sizeof(dal_maple_l2_entry_t));
    /* calculate the hash index from pL2Addr input */
    if (pL2Addr->entry_type == FLOW_TYPE_UNICAST)
    {   /* FLOW_TYPE_UNICAST */
        l2_entry.entry_type = FLOW_TYPE_UNICAST;
        l2_entry.unicast.fid = pL2Addr->unicast.vid;
        osal_memcpy(&l2_entry.unicast.mac, &pL2Addr->unicast.mac, sizeof(rtk_mac_t));
    }
    else if (pL2Addr->entry_type == FLOW_TYPE_L2_MULTI)
    {   /* FLOW_TYPE_L2_MULTI */
        l2_entry.entry_type = FLOW_TYPE_L2_MULTI;
        l2_entry.l2mcast.rvid = pL2Addr->l2mcast.rvid;
        osal_memcpy(&l2_entry.l2mcast.mac, &pL2Addr->l2mcast.mac, sizeof(rtk_mac_t));
    }
    else if (pL2Addr->entry_type == FLOW_TYPE_IP4_MULTI)
    {   /* FLOW_TYPE_IP4_MULTI */
        l2_entry.entry_type = FLOW_TYPE_IP4_MULTI;
        if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
        {
            l2_entry.ipmcast_ip_mc_sip.dip = pL2Addr->ipmcast.dip;
            l2_entry.ipmcast_ip_mc_sip.sip = pL2Addr->ipmcast.sip;
        }
        else if (LOOKUP_ON_DIP_AND_FVID == ip4HashFmt[unit])
        {
            l2_entry.ipmcast_ip_mc.rvid = pL2Addr->ipmcast.rvid;
            l2_entry.ipmcast_ip_mc.dip = pL2Addr->ipmcast.dip;
        }
        else
        {
            RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
            return RT_ERR_CHIP_NOT_SUPPORTED;
        }
    }
    else if (pL2Addr->entry_type == FLOW_TYPE_IP6_MULTI)
    {   /* FLOW_TYPE_IP6_MULTI */
        l2_entry.entry_type = FLOW_TYPE_IP6_MULTI;
        if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
        {
            l2_entry.ipmcast_ip_mc_sip.dip = pL2Addr->ipmcast.dip;
            l2_entry.ipmcast_ip_mc_sip.sip = pL2Addr->ipmcast.sip;
        }
        else if (LOOKUP_ON_DIP_AND_FVID == ip6HashFmt[unit])
        {
            l2_entry.ipmcast_ip_mc.rvid = pL2Addr->ipmcast.rvid;
            l2_entry.ipmcast_ip_mc.dip = pL2Addr->ipmcast.dip;
        }
        else
        {
            RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
            return RT_ERR_CHIP_NOT_SUPPORTED;
        }
    }
    else
    {
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
        return RT_ERR_CHIP_NOT_SUPPORTED;
    }

    if (_dal_maple_l2_entryToHashKey(unit, &l2_entry, &hash_key) != RT_ERR_OK)
    {
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
        return RT_ERR_FAILED;
    }

    cf_num = 0;
    for (hash_depth = 0; hash_depth < HAL_L2_HASHDEPTH(unit) && cf_num < cfAddrList_size; hash_depth++)
    {
        osal_memset(&l2_entry, 0, sizeof(dal_maple_l2_entry_t));
        isValid = 0;
        if (_dal_maple_l2_getL2EntryfromHash(unit, hash_key, hash_depth, &l2_entry, &isValid) != RT_ERR_OK)
        {
            /* not found in hash table, search CAM */
            break;
        }
        if (isValid)
        {   /* filled conflict address to return buffer pCfAddrList */
            (pCfAddrList + cf_num)->entry_type = l2_entry.entry_type;
            (pCfAddrList + cf_num)->valid = isValid;
            switch (l2_entry.entry_type)
            {
                case FLOW_TYPE_UNICAST:
                    (pCfAddrList + cf_num)->unicast.vid = l2_entry.unicast.fid;
                    osal_memcpy(&(pCfAddrList + cf_num)->unicast.mac, &l2_entry.unicast.mac, sizeof(rtk_mac_t));
                    (pCfAddrList + cf_num)->unicast.port = l2_entry.unicast.port;
                    (pCfAddrList + cf_num)->unicast.flags = 0;
                    (pCfAddrList + cf_num)->unicast.state = 0;

                    for (trk_gid = 0; trk_gid < HAL_MAX_NUM_OF_TRUNK(unit); trk_gid++)
                    {
                        if (dal_maple_trunk_port_get(unit, trk_gid, &trunk_portmask) == RT_ERR_OK)
                        {
                            if ((first_trunkMember = RTK_PORTMASK_GET_FIRST_PORT(trunk_portmask)) == -1)
                            {
                                /* no trunk member */
                                continue;
                            }

                            if ((RTK_PORTMASK_IS_PORT_SET(trunk_portmask, (pCfAddrList + cf_num)->unicast.port)) &&
                                (first_trunkMember == (pCfAddrList + cf_num)->unicast.port))
                            {
                                (pCfAddrList + cf_num)->unicast.trk_gid = trk_gid;
                                (pCfAddrList + cf_num)->unicast.flags |= RTK_L2_UCAST_FLAG_TRUNK_PORT;
                                break;
                            }
                        }
                    }
                    if(l2_entry.unicast.sablock)
                        (pCfAddrList + cf_num)->unicast.flags |= RTK_L2_UCAST_FLAG_SA_BLOCK;
                    if(l2_entry.unicast.dablock)
                        (pCfAddrList + cf_num)->unicast.flags |= RTK_L2_UCAST_FLAG_DA_BLOCK;
                    if(l2_entry.unicast.is_static)
                        (pCfAddrList + cf_num)->unicast.flags |= RTK_L2_UCAST_FLAG_STATIC;
                    if(l2_entry.unicast.nh)
                        (pCfAddrList + cf_num)->unicast.flags |= RTK_L2_UCAST_FLAG_NEXTHOP;
                    if(l2_entry.unicast.suspending)
                        (pCfAddrList + cf_num)->unicast.state |= RTK_L2_UCAST_STATE_SUSPEND;
                    if(l2_entry.unicast.aging == 0)
                        (pCfAddrList + cf_num)->unicast.isAged = TRUE;
                    else
                        (pCfAddrList + cf_num)->unicast.isAged = FALSE;
                    (pCfAddrList + cf_num)->unicast.age     = l2_entry.unicast.aging;
                    (pCfAddrList + cf_num)->unicast.l2_idx = (hash_key << 2) | hash_depth;
                    break;

                case FLOW_TYPE_L2_MULTI:
                    (pCfAddrList + cf_num)->l2mcast.rvid = l2_entry.l2mcast.rvid;
                    osal_memcpy(&(pCfAddrList + cf_num)->l2mcast.mac, &l2_entry.l2mcast.mac, sizeof(rtk_mac_t));
                    (pCfAddrList + cf_num)->l2mcast.fwdIndex    = l2_entry.l2mcast.index;
                    (pCfAddrList + cf_num)->l2mcast.l2_idx      = (hash_key << 2) | hash_depth;
                    osal_memset(&mcast_entry, 0, sizeof(mcast_entry));

                    if ((ret = table_read(unit, MAPLE_MC_PMSKt, l2_entry.l2mcast.index, (uint32 *)&mcast_entry)) != RT_ERR_OK)
                    {
                        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_get(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, (pCfAddrList + cf_num)->l2mcast.portmask.bits, (uint32 *) &mcast_entry)) != RT_ERR_OK)
                    {
                        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    break;

                case FLOW_TYPE_IP4_MULTI:
                    if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
                    {
                        (pCfAddrList + cf_num)->ipmcast.rvid = l2_entry.ipmcast_ip_mc_sip.rvid;
                        (pCfAddrList + cf_num)->ipmcast.dip = l2_entry.ipmcast_ip_mc_sip.dip;
                        (pCfAddrList + cf_num)->ipmcast.sip = l2_entry.ipmcast_ip_mc_sip.sip;
                        (pCfAddrList + cf_num)->ipmcast.fwdIndex = l2_entry.ipmcast_ip_mc_sip.index;
                    }
                    else if (LOOKUP_ON_DIP_AND_FVID == ip4HashFmt[unit])
                    {
                        (pCfAddrList + cf_num)->ipmcast.rvid = l2_entry.ipmcast_ip_mc.rvid;
                        (pCfAddrList + cf_num)->ipmcast.dip = l2_entry.ipmcast_ip_mc.dip;
                        (pCfAddrList + cf_num)->ipmcast.fwdIndex = l2_entry.ipmcast_ip_mc.index;
                    }
                    (pCfAddrList + cf_num)->ipmcast.l2_idx = (hash_key << 2) | hash_depth;
                    osal_memset(&mcast_entry, 0, sizeof(mcast_entry));
                    if ((ret = table_read(unit, MAPLE_MC_PMSKt, l2_entry.ipmcast_ip_mc.index, (uint32 *)&mcast_entry)) != RT_ERR_OK)
                    {
                        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_get(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, (pCfAddrList + cf_num)->ipmcast.portmask.bits, (uint32 *) &mcast_entry)) != RT_ERR_OK)
                    {
                        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    break;

                case FLOW_TYPE_IP6_MULTI:
                    if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
                    {
                        (pCfAddrList + cf_num)->ipmcast.rvid = l2_entry.ipmcast_ip_mc_sip.rvid;
                        (pCfAddrList + cf_num)->ipmcast.dip = l2_entry.ipmcast_ip_mc_sip.dip;
                        (pCfAddrList + cf_num)->ipmcast.sip = l2_entry.ipmcast_ip_mc_sip.sip;
                        (pCfAddrList + cf_num)->ipmcast.fwdIndex = l2_entry.ipmcast_ip_mc_sip.index;
                    }
                    else if (LOOKUP_ON_DIP_AND_FVID == ip6HashFmt[unit])
                    {
                        (pCfAddrList + cf_num)->ipmcast.rvid = l2_entry.ipmcast_ip_mc.rvid;
                        (pCfAddrList + cf_num)->ipmcast.dip = l2_entry.ipmcast_ip_mc.dip;
                        (pCfAddrList + cf_num)->ipmcast.fwdIndex = l2_entry.ipmcast_ip_mc.index;
                    }
                    (pCfAddrList + cf_num)->ip6mcast.l2_idx = (hash_key << 2) | hash_depth;
                    osal_memset(&mcast_entry, 0, sizeof(mcast_entry));
                    if ((ret = table_read(unit, MAPLE_MC_PMSKt, l2_entry.ipmcast_ip_mc.index, (uint32 *)&mcast_entry)) != RT_ERR_OK)
                    {
                        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    if ((ret = table_field_get(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, (pCfAddrList + cf_num)->ipmcast.portmask.bits, (uint32 *) &mcast_entry)) != RT_ERR_OK)
                    {
                        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
                        return ret;
                    }
                    break;

                default:
                    return RT_ERR_FAILED;
            }

            /* increase the count */
            cf_num++;
        }
    }

    (*pCf_retCnt) = cf_num;
    return RT_ERR_OK;
} /* end of dal_maple_l2_conflictAddr_get */


/* Function Name:
 *      dal_maple_l2_hwNextValidAddr_get
 * Description:
 *      get next valid entry with specific method.
 * Input:
 *      unit        - unit id
 *      pScan_idx   - the index which starting search from
 *      rtk_l2_nextValidType_t  - search Method
 * Output:
 *      pScan_idx           - the next valid entry index
 *      rtk_l2_entry_t      - the next valid entry
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_INPUT    - invalid entry index
 *      RT_ERR_UNIT_ID      - invalid unit id
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Applicable:
 *      8380
 * Note:
 *      rtk_l2_nextValidType_t
 *       {
 *          L2_NEXT_VALID_TYPE_CURRENT,  getting the entry which index=*pScan_idx
 *          .....
 *       }
 */
extern int32
dal_maple_l2_hwNextValidAddr_get(
    uint32              unit,
    int32               *pScan_idx,
    rtk_l2_nextValidType_t type,
    rtk_l2_entry_t  *pEntry)
{
    int32   ret;
    l2_entry_t  l2_entry;
    uint32 original_method;
    uint32 scanType;

    dal_maple_l2_entry_t  dal_entry;

    uint32  search_index;
    uint32  find_index;
    uint32  hashkey;
    uint32  partialkey1;

    uint32  ip_multi, ip6_multi;
    uint32  mac_uint32[2];
    uint32  is_l2mcast;
    uint32  aging;
    uint32  is_static;
    uint32  dablock, sablock;
    uint32  next_hop;
    uint32  suspending;
    uint32  agg_vid;
    uint32  vid_sel = 0;
    uint32  mac_idx = 0;

    rtk_portmask_t      trunk_portmask;
    uint32              trk_gid;
    uint32              first_trunkMember;

    multicast_index_entry_t mcast_entry;

    ipaddr_t invalid_ipv4_addr = 0;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* check index valid range */

    /* parameter check */
    RT_PARAM_CHK((pScan_idx ==NULL || pEntry==NULL), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((*pScan_idx >= 8192), RT_ERR_INPUT);

    if (L2_NEXT_VALID_TYPE_CURRENT == type)
        scanType = 0;
    else if (L2_NEXT_VALID_TYPE_DYNAMIC_UC == type)
        scanType = 1;
    else if (L2_NEXT_VALID_TYPE_DYNAMIC_UC_AND_MC == type)
        scanType = 2;
    else if (L2_NEXT_VALID_TYPE_UC_AND_MC == type)
        scanType = 3;
    else
    {
        return RT_ERR_INPUT;
    }

    osal_memset(&l2_entry, 0, sizeof(l2_entry));
    osal_memset(&dal_entry, 0, sizeof(dal_maple_l2_entry_t));
    osal_memset(pEntry, 0, sizeof(rtk_l2_entry_t));

    L2_SEM_LOCK(unit);

    if((ret = reg_field_read(unit, MAPLE_TBL_ACCESS_L2_METHOD_CTRLr, MAPLE_METHODf, &original_method)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if((ret = reg_field_write(unit, MAPLE_TBL_ACCESS_L2_METHOD_CTRLr, MAPLE_METHODf, &scanType)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    search_index = *pScan_idx;

    if ((ret = table_read(unit, MAPLE_L2_UCt, search_index, (uint32 *)&l2_entry)) != RT_ERR_OK)
    {
        if((ret = reg_field_write(unit, MAPLE_TBL_ACCESS_L2_METHOD_CTRLr, MAPLE_METHODf, &original_method)) != RT_ERR_OK)
        {
            L2_SEM_UNLOCK(unit);
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if((ret = reg_field_write(unit, MAPLE_TBL_ACCESS_L2_METHOD_CTRLr, MAPLE_METHODf, &original_method)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if((ret = reg_field_read(unit, MAPLE_TBL_ACCESS_L2_METHOD_CTRLr, MAPLE_HITf, &pEntry->valid)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if(pEntry->valid == 0)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return RT_ERR_ENTRY_NOTFOUND;
    }

    if((ret = reg_field_read(unit, MAPLE_TBL_ACCESS_L2_METHOD_CTRLr, MAPLE_HIT_ADDRf, &find_index)) != RT_ERR_OK)
    {
        L2_SEM_UNLOCK(unit);
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    L2_SEM_UNLOCK(unit);

    //osal_printf("find_index = %d\n", find_index);

    *pScan_idx = find_index;

    if(search_index > find_index)
    {
        pEntry->valid = 0;
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return RT_ERR_ENTRY_NOTFOUND;
    }

    //osal_printf("pScan_idx = %d\n", *pScan_idx);
    hashkey = find_index>>2;


    /* get IP_MULTI and IP6_MULTI bits from l2_entry */
    if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_IP_MCtf, &ip_multi, (uint32 *)&l2_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_IP6_MCtf, &ip6_multi, (uint32 *)&l2_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    /* Get mac addr */
    is_l2mcast = 0;
    next_hop = 0;
    if (0 == ip_multi)
    {
        if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_MACtf, &mac_uint32[0], (uint32 *)&l2_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        is_l2mcast = (uint8) ((mac_uint32[1] >> 8) & BITMASK_1B);
        if(0 == is_l2mcast)
        {
            /* get next hop bit */
            if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_NEXT_HOPtf, &next_hop, (uint32 *)&l2_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }
        }
    }

    aging = 0;
    sablock = 0;
    dablock = 0;
    is_static = 0;
    suspending =0;
    agg_vid = 0;

    /*
     * check whether this entry is l2 multicast entry.
     * If not l2 multicast entry, get more information for valid check
     */
    if (0 == is_l2mcast && 0 == ip_multi)
    {
        if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_AGEtf, &aging, (uint32 *)&l2_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_SA_BLKtf, &sablock, (uint32 *)&l2_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_DA_BLKtf, &dablock, (uint32 *)&l2_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_STATICtf, &is_static, (uint32 *)&l2_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_SUSPENDtf, &suspending, (uint32 *)&l2_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_NEXT_HOPtf, &next_hop, (uint32 *)&l2_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_L2), "");
            return ret;
        }

        if (next_hop == TRUE)
        {
            if ((ret = table_field_get(unit, MAPLE_L2_NEXT_HOPt, MAPLE_L2_NEXT_HOP_VLAN_TARGETtf, &vid_sel, (uint32 *)&l2_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }
            if ((ret = table_field_get(unit, MAPLE_L2_NEXT_HOPt, MAPLE_L2_NEXT_HOP_ROUTE_IDXtf, &mac_idx, (uint32 *)&l2_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }
        }
        else
        {
            if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_VIDtf, &agg_vid, (uint32 *)&l2_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }
        }
    }

    /* check whether this entry is valid entry */
    if (!(ip_multi | is_l2mcast | aging | sablock | dablock | is_static | next_hop))
    {
        /* this is not a valid entry. No need to futher process*/
        pEntry->valid = 0;
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pEntry->valid=%d, pScan_idx =%d", pEntry->valid, pScan_idx);
        return RT_ERR_ENTRY_NOTFOUND;
    }
    else
    {
        pEntry->valid = 1;
    }

    /* Extract content of each kind of entry from l2_enry */
    switch ((ip6_multi << 2) | (ip_multi << 1) | is_l2mcast)
    {
        case 0x0: /* L2 unicast */

            dal_entry.entry_type = FLOW_TYPE_UNICAST;

            {
                if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_SLPtf, &dal_entry.unicast.port, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_get(unit, MAPLE_L2_UCt, MAPLE_L2_UC_FID_RVIDtf, &dal_entry.unicast.fid, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                dal_entry.unicast.mac.octet[0] = (uint8)(mac_uint32[1] >> 8);
                dal_entry.unicast.mac.octet[1] = (uint8)(mac_uint32[1]);
                dal_entry.unicast.mac.octet[2] = (uint8)(mac_uint32[0] >> 24);
                dal_entry.unicast.mac.octet[3] = (uint8)(mac_uint32[0] >> 16);
                dal_entry.unicast.mac.octet[4] = (uint8)(mac_uint32[0] >> 8);
                dal_entry.unicast.mac.octet[5] = (uint8)(mac_uint32[0]);
                dal_entry.unicast.sablock      = sablock;
                dal_entry.unicast.dablock      = dablock;
                dal_entry.unicast.is_static    = is_static;
                dal_entry.unicast.aging        = aging;
                dal_entry.unicast.suspending   = suspending;
                dal_entry.unicast.nh           = next_hop;
                dal_entry.unicast.agg_vid      = agg_vid;
                dal_entry.unicast.vlan_target  = vid_sel;
                dal_entry.unicast.route_idx    = mac_idx;
            }

            /* convert FID from hash algorithm */
            dal_entry.unicast.fid = (hashkey & BITMASK_11B) | (dal_entry.unicast.fid & (~BITMASK_11B));
            if ((ret = _dal_maple_l2_entryToHashKey(unit, &dal_entry, &partialkey1)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }
            dal_entry.unicast.fid  = (partialkey1 & BITMASK_11B) | (dal_entry.unicast.fid & (~BITMASK_11B));

            break;
        case 0x1: /* l2 Multicast */
            dal_entry.entry_type = L2_MULTICAST;

            if (table_field_get(unit, MAPLE_L2_MCt, MAPLE_L2_MC_MC_PMSK_IDXtf, &dal_entry.l2mcast.index, (uint32 *)&l2_entry) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return RT_ERR_FAILED;
            }

            if (table_field_get(unit, MAPLE_L2_MCt, MAPLE_L2_MC_VIDtf, &dal_entry.l2mcast.agg_vid, (uint32 *)&l2_entry) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return RT_ERR_FAILED;
            }

            if ((ret = table_field_get(unit, MAPLE_L2_MCt, MAPLE_L2_MC_FID_RVIDtf, &dal_entry.l2mcast.rvid, (uint32 *)&l2_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }

            dal_entry.l2mcast.mac.octet[0] = (uint8)(mac_uint32[1] >> 8);
            dal_entry.l2mcast.mac.octet[1] = (uint8)(mac_uint32[1]);
            dal_entry.l2mcast.mac.octet[2] = (uint8)(mac_uint32[0] >> 24);
            dal_entry.l2mcast.mac.octet[3] = (uint8)(mac_uint32[0] >> 16);
            dal_entry.l2mcast.mac.octet[4] = (uint8)(mac_uint32[0] >> 8);
            dal_entry.l2mcast.mac.octet[5] = (uint8)(mac_uint32[0]);

            /* convert RVID from hash algorithm */
            dal_entry.l2mcast.rvid = (hashkey & BITMASK_11B) | (dal_entry.l2mcast.rvid & (~BITMASK_11B));
            if ((ret = _dal_maple_l2_entryToHashKey(unit, &dal_entry, &partialkey1)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                return ret;
            }
            dal_entry.l2mcast.rvid  = (partialkey1 & BITMASK_11B) | (dal_entry.l2mcast.rvid & (~BITMASK_11B));
            break;
        case 0x2: /* IPv4 multicast */
            dal_entry.entry_type = IP4_MULTICAST;
            if (LOOKUP_ON_FVID_AND_MAC == ip4HashFmt[unit])
            {
                RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "Should not come here");
            }
            else if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
            {
                if ((ret = table_field_get(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_MC_PMSK_IDXtf, &dal_entry.ipmcast_ip_mc_sip.index, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_get(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_FID_RVIDtf, &dal_entry.ipmcast_ip_mc_sip.rvid, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_get(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_GIPtf, &dal_entry.ipmcast_ip_mc_sip.dip, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_get(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_SIPtf, &dal_entry.ipmcast_ip_mc_sip.sip, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                dal_entry.ipmcast_ip_mc_sip.sip = (hashkey & BITMASK_11B) | (dal_entry.ipmcast_ip_mc_sip.sip & (~BITMASK_11B));
                if (( ret = _dal_maple_l2_entryToHashKey(unit, &dal_entry, &partialkey1)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                dal_entry.ipmcast_ip_mc_sip.sip = (partialkey1 & BITMASK_11B) | (dal_entry.ipmcast_ip_mc_sip.sip & (~BITMASK_11B));
            }
            else if (LOOKUP_ON_DIP_AND_FVID == ip4HashFmt[unit])
            {
                if ((ret = table_field_get(unit, MAPLE_L2_IP_MCt, MAPLE_L2_IP_MC_MC_PMSK_IDXtf, &dal_entry.ipmcast_ip_mc.index, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_get(unit, MAPLE_L2_IP_MCt, MAPLE_L2_IP_MC_FID_RVIDtf, &dal_entry.ipmcast_ip_mc.rvid, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_get(unit, MAPLE_L2_IP_MCt, MAPLE_L2_IP_MC_GIPtf, &dal_entry.ipmcast_ip_mc.dip, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                dal_entry.ipmcast_ip_mc.rvid = (hashkey & BITMASK_11B) | (dal_entry.ipmcast_ip_mc.rvid & (~BITMASK_11B));
                if ((ret = _dal_maple_l2_entryToHashKey(unit, &dal_entry, &partialkey1)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                dal_entry.ipmcast_ip_mc.rvid  = (partialkey1 & BITMASK_11B) | (dal_entry.ipmcast_ip_mc.rvid & (~BITMASK_11B));
            }
            break;
        case 0x6: /* IPv6 multicast */
            dal_entry.entry_type = IP6_MULTICAST;
            if (LOOKUP_ON_FVID_AND_MAC == ip6HashFmt[unit])
            {
                RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "Should not come here");
            }
            else if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
            {
                if ((ret = table_field_get(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_MC_PMSK_IDXtf, &dal_entry.ipmcast_ip_mc_sip.index, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_get(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_FID_RVIDtf, &dal_entry.ipmcast_ip_mc_sip.rvid, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_get(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_GIPtf, &dal_entry.ipmcast_ip_mc_sip.dip, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_get(unit, MAPLE_L2_IP_MC_SIPt, MAPLE_L2_IP_MC_SIP_SIPtf, &dal_entry.ipmcast_ip_mc_sip.sip, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                dal_entry.ipmcast_ip_mc_sip.sip = (hashkey & BITMASK_11B) | (dal_entry.ipmcast_ip_mc_sip.sip & (~BITMASK_11B));
                if ((ret = _dal_maple_l2_entryToHashKey(unit, &dal_entry, &partialkey1)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                dal_entry.ipmcast_ip_mc_sip.sip = (partialkey1 & BITMASK_11B) | (dal_entry.ipmcast_ip_mc_sip.sip & (~BITMASK_11B));
            }
            else if (LOOKUP_ON_DIP_AND_FVID == ip6HashFmt[unit])
            {
                if ((ret = table_field_get(unit, MAPLE_L2_IP_MCt, MAPLE_L2_IP_MC_MC_PMSK_IDXtf, &dal_entry.ipmcast_ip_mc.index, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_get(unit, MAPLE_L2_IP_MCt, MAPLE_L2_IP_MC_FID_RVIDtf, &dal_entry.ipmcast_ip_mc.rvid, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                if ((ret = table_field_get(unit, MAPLE_L2_IP_MCt, MAPLE_L2_IP_MC_GIPtf, &dal_entry.ipmcast_ip_mc.dip, (uint32 *)&l2_entry)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }

                dal_entry.ipmcast_ip_mc.rvid = (hashkey & BITMASK_11B) | (dal_entry.ipmcast_ip_mc.rvid & (~BITMASK_11B));
                if ((ret = _dal_maple_l2_entryToHashKey(unit, &dal_entry, &partialkey1)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                dal_entry.ipmcast_ip_mc.rvid  = (partialkey1 & BITMASK_11B) | (dal_entry.ipmcast_ip_mc.rvid & (~BITMASK_11B));
            }
            break;

        default:
            return RT_ERR_FAILED;
     }

     RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "pIsValid=%d", pEntry->valid);

    /* Transfer from dal_maple_l2_entry_t to rtk_l2_entry_t */

    L2_SEM_LOCK(unit);

    if (pEntry->valid == 1)
    {
        pEntry->entry_type = dal_entry.entry_type;
        switch (pEntry->entry_type)
        {
            case FLOW_TYPE_UNICAST:
                pEntry->unicast.vid = dal_entry.unicast.fid;
                osal_memcpy(&pEntry->unicast.mac, &dal_entry.unicast.mac, sizeof(rtk_mac_t));
                pEntry->unicast.port = dal_entry.unicast.port;
                pEntry->unicast.flags = 0;
                pEntry->unicast.state = 0;

                for (trk_gid = 0; trk_gid < HAL_MAX_NUM_OF_TRUNK(unit); trk_gid++)
                {
                    if (dal_maple_trunk_port_get(unit, trk_gid, &trunk_portmask) == RT_ERR_OK)
                    {
                        if ((first_trunkMember = RTK_PORTMASK_GET_FIRST_PORT(trunk_portmask)) == -1)
                        {
                            /* no trunk member */
                            continue;
                        }

                        if ((RTK_PORTMASK_IS_PORT_SET(trunk_portmask, pEntry->unicast.port)) &&
                            (first_trunkMember == pEntry->unicast.port))
                        {
                            pEntry->unicast.trk_gid = trk_gid;
                            pEntry->unicast.flags |= RTK_L2_UCAST_FLAG_TRUNK_PORT;
                            break;
                        }
                    }
                }

                pEntry->unicast.l2_idx = find_index;

                if(dal_entry.unicast.sablock)
                    pEntry->unicast.flags |= RTK_L2_UCAST_FLAG_SA_BLOCK;
                if(dal_entry.unicast.dablock)
                    pEntry->unicast.flags |= RTK_L2_UCAST_FLAG_DA_BLOCK;
                if(dal_entry.unicast.is_static)
                    pEntry->unicast.flags |= RTK_L2_UCAST_FLAG_STATIC;
                if(dal_entry.unicast.nh)
                {
                    pEntry->unicast.vlan_target = dal_entry.unicast.vlan_target;
                    pEntry->unicast.route_idx = dal_entry.unicast.route_idx;
                    pEntry->unicast.flags |= RTK_L2_UCAST_FLAG_NEXTHOP;
                }
                else
                    pEntry->unicast.agg_vid = dal_entry.unicast.agg_vid;
                if(dal_entry.unicast.suspending)
                    pEntry->unicast.state |= RTK_L2_UCAST_STATE_SUSPEND;
                if(dal_entry.unicast.aging == 0)
                    pEntry->unicast.isAged = TRUE;
                else
                    pEntry->unicast.isAged = FALSE;
                break;

            case FLOW_TYPE_L2_MULTI:
                pEntry->l2mcast.rvid = dal_entry.l2mcast.rvid;
                osal_memcpy(&pEntry->l2mcast.mac, &dal_entry.l2mcast.mac, sizeof(rtk_mac_t));
                pEntry->l2mcast.fwdIndex = dal_entry.l2mcast.index;
                osal_memset(&mcast_entry, 0, sizeof(mcast_entry));
                if ((ret = table_read(unit, MAPLE_MC_PMSKt, dal_entry.l2mcast.index, (uint32 *)&mcast_entry)) != RT_ERR_OK)
                {
                    L2_SEM_UNLOCK(unit);
                    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                if ((ret = table_field_get(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, pEntry->l2mcast.portmask.bits, (uint32 *) &mcast_entry)) != RT_ERR_OK)
                {
                    L2_SEM_UNLOCK(unit);
                    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                break;

            case FLOW_TYPE_IP4_MULTI:
                if (LOOKUP_ON_FVID_AND_MAC == ip4HashFmt[unit])
                {
                    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "It should not be existing an IP-multicast entry in FVID_AND_MAC mode");
                }
                else if (LOOKUP_ON_DIP_AND_SIP == ip4HashFmt[unit])
                {
                    pEntry->ipmcast.rvid = dal_entry.ipmcast_ip_mc_sip.rvid;
                    pEntry->ipmcast.dip = dal_entry.ipmcast_ip_mc_sip.dip;
                    pEntry->ipmcast.sip = dal_entry.ipmcast_ip_mc_sip.sip;
                    pEntry->ipmcast.fwdIndex = dal_entry.ipmcast_ip_mc_sip.index;
                }
                else if (LOOKUP_ON_DIP_AND_FVID == ip4HashFmt[unit])
                {
                    pEntry->ipmcast.rvid = dal_entry.ipmcast_ip_mc.rvid;
                    pEntry->ipmcast.dip = dal_entry.ipmcast_ip_mc.dip;
                    pEntry->ipmcast.fwdIndex = dal_entry.ipmcast_ip_mc.index;
                }
                else
                {
                    L2_SEM_UNLOCK(unit);
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return RT_ERR_FAILED;
                }

                osal_memset(&mcast_entry, 0, sizeof(mcast_entry));
                if ((ret = table_read(unit, MAPLE_MC_PMSKt, pEntry->ipmcast.fwdIndex, (uint32 *)&mcast_entry)) != RT_ERR_OK)
                {
                    L2_SEM_UNLOCK(unit);
                    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                if ((ret = table_field_get(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, pEntry->ipmcast.portmask.bits, (uint32 *) &mcast_entry)) != RT_ERR_OK)
                {
                    L2_SEM_UNLOCK(unit);
                    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                break;

            case FLOW_TYPE_IP6_MULTI:
                if (LOOKUP_ON_FVID_AND_MAC == ip6HashFmt[unit])
                {
                    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "It should not be existing an IP-multicast entry in FVID_AND_MAC mode");
                }
                else if (LOOKUP_ON_DIP_AND_SIP == ip6HashFmt[unit])
                {
                    pEntry->ip6mcast.rvid = dal_entry.ipmcast_ip_mc_sip.rvid;
                    pEntry->ip6mcast.fwdIndex = dal_entry.ipmcast_ip_mc_sip.index;
                    _dal_maple_l2_ip6CareByteConvertToIP6Addr(&pEntry->ip6mcast.dip, &pEntry->ip6mcast.sip, &dal_entry.ipmcast_ip_mc_sip.dip, &dal_entry.ipmcast_ip_mc_sip.sip);
                }
                else if (LOOKUP_ON_DIP_AND_FVID == ip6HashFmt[unit])
                {
                    pEntry->ip6mcast.rvid = dal_entry.ipmcast_ip_mc.rvid;
                    pEntry->ip6mcast.fwdIndex = dal_entry.ipmcast_ip_mc.index;
                    _dal_maple_l2_ip6CareByteConvertToIP6Addr(&pEntry->ip6mcast.dip, &pEntry->ip6mcast.sip, &dal_entry.ipmcast_ip_mc.dip, &invalid_ipv4_addr);
                    osal_memset(&pEntry->ip6mcast.sip, 0, sizeof(rtk_ipv6_addr_t));
                }
                else
                {
                    L2_SEM_UNLOCK(unit);
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return RT_ERR_FAILED;
                }
                osal_memset(&mcast_entry, 0, sizeof(mcast_entry));
                if ((ret = table_read(unit, MAPLE_MC_PMSKt, pEntry->ip6mcast.fwdIndex, (uint32 *)&mcast_entry)) != RT_ERR_OK)
                {
                    L2_SEM_UNLOCK(unit);
                    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                if ((ret = table_field_get(unit, MAPLE_MC_PMSKt, MAPLE_MC_PMSK_PMSKtf, pEntry->ip6mcast.portmask.bits, (uint32 *) &mcast_entry)) != RT_ERR_OK)
                {
                    L2_SEM_UNLOCK(unit);
                    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
                break;

            default:
                L2_SEM_UNLOCK(unit);
                return RT_ERR_FAILED;
        }
    }

     L2_SEM_UNLOCK(unit);

     return RT_ERR_OK;
}

/* Function Name:
 *     dal_maple_l2_getL2EntryfromHash_dump
 * Description:
 *      Get L2 Entry from Chip
 * Input:
 *      unit      - unit id
 *      hashKey   - Hash Key for this Entry
 *      location  - Entry location in Hash Bucket
 *      pL2_entry - L2 entry used to do search
 *      pIsValid  - Is valid entry
 *
 * Output:
 *      pL2_entry - L2 entry
 *      pIsValid  - Is valid or invalid entry
 *                    TRUE: valid entry
 *                    FALSE: invalid entry
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 * Note:
 *      We can not distinguish an entry between IP multicast in L2 mode and L2 multicast
 */
int32
dal_maple_l2_getL2EntryfromHash_dump(uint32 unit, uint32 entry_idx, dal_maple_l2_entry_t *pL2_entry, uint32 *pIsValid)
{
    uint32  hashkey, location;
    uint32  isValid;
    int32   ret;

    hashkey = entry_idx >> 2;
    location = entry_idx & BITMASK_2B;

    ret = _dal_maple_l2_getL2EntryfromHash(unit, hashkey, location, pL2_entry, &isValid);
    *pIsValid = isValid;

    return ret;
} /* dal_maple_l2_getL2EntryfromHash_dump */

/* Function Name:
 *      dal_maple_l2_macLearningCnt_get
 * Description:
 *      Get number of learned MAC addresses of specified type.
 * Input:
 *      unit             - unit id
 *      fid_macLimit_idx - index of VLAN MAC limit entry
 * Output:
 *      pNum             - number of learned MAC addresses
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_UNIT_ID      - invalid unit id
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_OUT_OF_RANGE - input parameter out of range
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      (1) The valid range of fid_macLimit_idx is 0~31 in 8390 and 9310, and 0~7 in 8380 and 9300
 */
int32
dal_maple_l2_macLearningCnt_get(uint32 unit, rtk_l2_macLimitType_t type, rtk_l2_macCnt_t *pLimitCnt)
{
    RT_PARAM_CHK((NULL == pLimitCnt), RT_ERR_NULL_POINTER);

    switch(type)
    {
        case L2_MAC_LIMIT_GLOBAL:
            return dal_maple_l2_learningCnt_get(unit, &pLimitCnt->glbCnt);
        case L2_MAC_LIMIT_PORT:
            return dal_maple_l2_portLearningCnt_get(unit, pLimitCnt->portTrkCnt.id, &pLimitCnt->portTrkCnt.cnt);
        case L2_MAC_LIMIT_FID:
            return dal_maple_l2_fidLearningCnt_get(unit, pLimitCnt->fidCnt.entryId, &pLimitCnt->fidCnt.cnt);
        default:
            return RT_ERR_INPUT;
    }
}

/* Function Name:
 *      dal_maple_l2_limitLearningNum_get
 * Description:
 *      Get the mac limit learning counts of specified device.
 * Input:
 *      unit     - unit id
 *      type - mac limit type
 *      pLimitCnt - pointer ro mac limit parameters
 * Output:
 *      pLimitCnt - pointer of mac limit learning counts
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_UNIT_ID      - invalid unit id
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_TRUNK_ID     - invalid trunk id
 * Note:
 *      (1) The mac learning counts only calculate dynamic mac numbers.
 */
int32
dal_maple_l2_limitLearningNum_get(uint32 unit, rtk_l2_macLimitType_t type, rtk_l2_macCnt_t *pLimitCnt)
{
    int32 ret;
    rtk_l2_fidMacLimitEntry_t fidMacLimitEntry;

    RT_PARAM_CHK((NULL == pLimitCnt), RT_ERR_NULL_POINTER);

    switch(type)
    {
        case L2_MAC_LIMIT_GLOBAL:
            return dal_maple_l2_limitLearningCnt_get(unit, &pLimitCnt->glbCnt);
        case L2_MAC_LIMIT_PORT:
            return dal_maple_l2_portLimitLearningCnt_get(unit, pLimitCnt->portTrkCnt.id, &pLimitCnt->portTrkCnt.cnt);
        case L2_MAC_LIMIT_FID:
            if((ret= dal_maple_l2_fidLimitLearningEntry_get(unit, pLimitCnt->fidCnt.entryId, &fidMacLimitEntry)) !=RT_ERR_OK)
                return ret;
            pLimitCnt->fidCnt.cnt = fidMacLimitEntry.maxNum;
            return RT_ERR_OK;
        default:
            return RT_ERR_INPUT;
    }
}


/* Function Name:
 *      dal_maple_l2_limitLearningNum_set
 * Description:
 *      Set the mac limit learning counts of specified device.
 * Input:
 * Input:
 *      unit     - unit id
 *      type - mac limit type
 *      pLimitCnt - pointer ro mac limit parameters and mac limit learning counts
 * Output:
 *      none
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_UNIT_ID      - invalid unit id
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_TRUNK_ID     - invalid trunk id
 * Note:
 *      (1) The mac learning counts only calculate dynamic mac numbers.
 */
int32
dal_maple_l2_limitLearningNum_set(uint32 unit, rtk_l2_macLimitType_t type, rtk_l2_macCnt_t *pLimitCnt)
{
    int32 ret;
    rtk_l2_fidMacLimitEntry_t fidMacLimitEntry;

    RT_PARAM_CHK((NULL == pLimitCnt), RT_ERR_NULL_POINTER);

    switch(type)
    {
        case L2_MAC_LIMIT_GLOBAL:
            if (pLimitCnt->glbCnt == L2_MAC_CST_DISABLE)
                pLimitCnt->glbCnt = HAL_L2_LEARN_LIMIT_CNT_DISABLE(unit);
            return dal_maple_l2_limitLearningCnt_set(unit, pLimitCnt->glbCnt);
        case L2_MAC_LIMIT_PORT:
            if (pLimitCnt->portTrkCnt.cnt == L2_MAC_CST_DISABLE)
                pLimitCnt->portTrkCnt.cnt = HAL_L2_LEARN_LIMIT_CNT_DISABLE(unit);
            return dal_maple_l2_portLimitLearningCnt_set(unit, pLimitCnt->portTrkCnt.id, pLimitCnt->portTrkCnt.cnt);
        case L2_MAC_LIMIT_FID:
            if (pLimitCnt->fidCnt.cnt == L2_MAC_CST_DISABLE)
                pLimitCnt->fidCnt.cnt = HAL_L2_LEARN_LIMIT_CNT_DISABLE(unit);
            if((ret= dal_maple_l2_fidLimitLearningEntry_get(unit, pLimitCnt->fidCnt.entryId, &fidMacLimitEntry)) != RT_ERR_OK)
                return ret;
            fidMacLimitEntry.maxNum = pLimitCnt->fidCnt.cnt;
            return dal_maple_l2_fidLimitLearningEntry_set(unit, pLimitCnt->fidCnt.entryId, &fidMacLimitEntry);
        default:
            return RT_ERR_INPUT;
    }
}

/* Function Name:
 *      dal_maple_l2_limitLearningAction_get
 * Description:
 *      Get the mac limit action of specified device.
 * Input:
 *      unit     - unit id
 *      type - mac limit type
 *      pAction - pointer ro mac limit parameters
 * Output:
 *      pAction - pointer of mac limit action
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_UNIT_ID      - invalid unit id
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_TRUNK_ID     - invalid trunk id
 * Note:
 *      (1) The mac learning counts only calculate dynamic mac numbers.
 */
int32
dal_maple_l2_limitLearningAction_get(uint32 unit, rtk_l2_macLimitType_t type, rtk_l2_macLimitAction_t *pAction)
{
    RT_PARAM_CHK((NULL == pAction), RT_ERR_NULL_POINTER);

    switch(type)
    {
        case L2_MAC_LIMIT_GLOBAL:
            return dal_maple_l2_limitLearningCntAction_get(unit, (rtk_l2_limitLearnCntAction_t*)&pAction->glbAct);
        case L2_MAC_LIMIT_PORT:
            return dal_maple_l2_portLimitLearningCntAction_get(unit, pAction->portTrkAct.id, (rtk_l2_limitLearnCntAction_t*)&pAction->portTrkAct.act);
        case L2_MAC_LIMIT_FID:
            return dal_maple_l2_fidLearningCntAction_get(unit, (rtk_l2_limitLearnCntAction_t*)&pAction->fidAct.act);
        default:
            return RT_ERR_INPUT;
    }
}

/* Function Name:
 *      dal_maple_l2_limitLearningAction_set
 * Description:
 *      Set the mac limit action of specified device.
 * Input:
 * Input:
 *      unit     - unit id
 *      type - mac limit type
 *      pAction - pointer ro mac limit parameters and mac limit action
 * Output:
 *      none
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_UNIT_ID      - invalid unit id
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_TRUNK_ID     - invalid trunk id
 * Note:
 *      (1) The mac learning counts only calculate dynamic mac numbers.
 */
int32
dal_maple_l2_limitLearningAction_set(uint32 unit, rtk_l2_macLimitType_t type, rtk_l2_macLimitAction_t *pAction)
{
    RT_PARAM_CHK((NULL == pAction), RT_ERR_NULL_POINTER);

    switch(type)
    {
        case L2_MAC_LIMIT_GLOBAL:
            return dal_maple_l2_limitLearningCntAction_set(unit, pAction->glbAct);
        case L2_MAC_LIMIT_PORT:
            return dal_maple_l2_portLimitLearningCntAction_set(unit, pAction->portTrkAct.id, pAction->portTrkAct.act);
        case L2_MAC_LIMIT_FID:
            return dal_maple_l2_fidLearningCntAction_set(unit, pAction->fidAct.act);
        default:
            return RT_ERR_INPUT;
    }
}

/* Function Name:
 *      dal_maple_l2_agingTime_get
 * Description:
 *      Get the aging time of from the specified device.
 * Input:
 *      unit        - unit id
 *      type       - age time type
 * Output:
 *      pAging_time - pointer buffer of aging time
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_UNIT_ID      - invalid unit id
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      The unit is second.
 */
int32
dal_maple_l2_agingTime_get(uint32 unit, rtk_l2_ageTimeType_t type, uint32 *pAging_time)
{
    RT_PARAM_CHK(type>=L2_AGE_TIME_END, RT_ERR_INPUT);
    return dal_maple_l2_aging_get(unit, pAging_time);
}

/* Function Name:
 *      dal_maple_l2_agingTime_set
 * Description:
 *      Set the aging time of suspending entry from the specified device.
 * Input:
 *      unit       - unit id
 *      type       - age time type
 *      aging_time - aging time
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_UNIT_ID  - invalid unit id
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      The unit is second.
 */
int32
dal_maple_l2_agingTime_set(uint32 unit, rtk_l2_ageTimeType_t type, uint32 aging_time)
{
    RT_PARAM_CHK(type>=L2_AGE_TIME_END, RT_ERR_INPUT);
    return dal_maple_l2_aging_set(unit, aging_time);
}

/* Function Name:
 *      dal_maple_l2_portMoveAction_get
 * Description:
 *      Get forwarding action when port moving is detected.
 * Input:
 *      unit       - unit id
 *      type      - port move type
 *      pAction  - pointer to portmove parameter
 * Output:
 *      pAction - pointer to forwarding action
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_UNIT_ID      - invalid unit id
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
dal_maple_l2_portMoveAction_get(
    uint32              unit,
    rtk_l2_portMoveType_t type,
    rtk_l2_portMoveAct_t        *pAction)
{
    rtk_port_t      port;
    RT_PARAM_CHK((NULL == pAction), RT_ERR_NULL_POINTER);

    switch(type)
    {
        case L2_PORT_MOVE_DYNAMIC:
            return dal_maple_l2_portLegalPortMoveAction_get(unit, pAction->dynAct.port, &pAction->dynAct.act);
        case L2_PORT_MOVE_STATIC:
            HWP_PORT_TRAVS(unit, port)
            {
                return dal_maple_l2_staticPortMoveAction_get(unit, port, &pAction->sttAct.act);
            }
        default:
            return RT_ERR_INPUT;
    }
}

/* Function Name:
 *      dal_maple_l2_portMoveAction_set
 * Description:
 *      Set forwarding action when port moving is detected.
 * Input:
 *      unit       - unit id
 *      type      - port move type
 *      pAction  - pointer to portmove parameter and action
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_UNIT_ID    - invalid unit id
 *      RT_ERR_NOT_INIT   - The module is not initial
 *      RT_ERR_PORT_ID    - invalid port id
 *      RT_ERR_FWD_ACTION - invalid forwarding action
 * Note:
 *      None
 */
int32
dal_maple_l2_portMoveAction_set(
    uint32              unit,
    rtk_l2_portMoveType_t type,
    rtk_l2_portMoveAct_t        *pAction)
{
    int32 ret;
    rtk_port_t port;

    RT_PARAM_CHK((NULL == pAction), RT_ERR_NULL_POINTER);

    switch(type)
    {
        case L2_PORT_MOVE_DYNAMIC:
            return dal_maple_l2_portLegalPortMoveAction_set(unit, pAction->dynAct.port, pAction->dynAct.act);
        case L2_PORT_MOVE_STATIC:
            HWP_PORT_TRAVS_EXCEPT_CPU(unit, port)
            {
                if ((ret = dal_maple_l2_staticPortMoveAction_set(unit, port, pAction->sttAct.act)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_DAL|MOD_L2), "");
                    return ret;
                }
            }
            return RT_ERR_OK;
        default:
            return RT_ERR_INPUT;
    }
}

/* Function Name:
 *      dal_maple_l2_portUcastLookupMissAction_get
 * Description:
 *      Get forwarding action of specified port when unicast destination address lookup miss.
 * Input:
 *      unit    - unit id
 *      port    - port id
 * Output:
 *      pAction - pointer to forwarding action
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_UNIT_ID      - invalid unit id
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Forwarding action is as following:
 *      - ACTION_FORWARD
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 *      - ACTION_COPY2CPU
 *      - ACTION_TRAP2MASTERCPU
 *      - ACTION_COPY2MASTERCPU
 */
int32
dal_maple_l2_portUcastLookupMissAction_get(uint32 unit, rtk_port_t port, rtk_action_t *pAction)
{
    return dal_maple_l2_portLookupMissAction_get(unit, port, DLF_TYPE_UCAST, pAction);
}

/* Function Name:
 *      dal_maple_l2_portUcastLookupMissAction_set
 * Description:
 *      Set forwarding action of specified port when unicast destination address lookup miss.
 * Input:
 *      unit   - unit id
 *      port   - port id
 *      action - forwarding action
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_UNIT_ID    - invalid unit id
 *      RT_ERR_NOT_INIT   - The module is not initial
 *      RT_ERR_PORT_ID    - invalid port id
 *      RT_ERR_FWD_ACTION - invalid forwarding action
 * Note:
 *      Forwarding action is as following:
 *      - ACTION_FORWARD
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 *      - ACTION_COPY2CPU
 *      - ACTION_TRAP2MASTERCPU
 *      - ACTION_COPY2MASTERCPU
 */
int32
dal_maple_l2_portUcastLookupMissAction_set(uint32 unit, rtk_port_t port, rtk_action_t action)
{
    return dal_maple_l2_portLookupMissAction_set(unit, port, DLF_TYPE_UCAST, action);
}

/* Function Name:
 *      dal_maple_l2_mcastFwdPortmaskEntry_get
 * Description:
 *      Get portmask of multicast forwarding entry
 * Input:
 *      unit       - unit id
 *      index      - index of multicast forwarding portmask
 * Output:
 *      pPortmask  - pointer buffer of multicast ports
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_UNIT_ID            - invalid unit id
 *      RT_ERR_NOT_INIT           - The module is not initial
 *      RT_ERR_NULL_POINTER       - input parameter may be null pointer
 *      RT_ERR_L2_MULTI_FWD_INDEX - invalid index of multicast forwarding portmask
 * Note:
 *      The valid range of indx is 0 ~ (multicast forwarding table size - 1)
 */
extern int32
dal_maple_l2_mcastFwdPortmaskEntry_get(
    uint32          unit,
    int32           index,
    rtk_portmask_t  *pPortmask)
{
    uint32 crossVlan;
    return dal_maple_l2_mcastFwdPortmask_get(unit, index, pPortmask, &crossVlan);
}

/* Function Name:
 *      dal_maple_l2_mcastFwdPortmaskEntry_set
 * Description:
 *      Set portmask of multicast forwarding entry
 * Input:
 *      unit      - unit id
 *      index     - index of multicast forwarding portmask
 *      pPortmask - pointer buffer of multicast ports
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_UNIT_ID            - invalid unit id
 *      RT_ERR_NOT_INIT           - The module is not initial
 *      RT_ERR_NULL_POINTER       - input parameter may be null pointer
 *      RT_ERR_L2_MULTI_FWD_INDEX - invalid index of multicast forwarding portmask
 * Note:
 *      The valid range of indx is 0 ~ (multicast forwarding table size - 1)
 */
extern int32
dal_maple_l2_mcastFwdPortmaskEntry_set(
    uint32          unit,
    int32           index,
    rtk_portmask_t  *pPortmask)
{
    uint32 crossVlan =0;
    return dal_maple_l2_mcastFwdPortmask_set(unit, index, pPortmask, crossVlan);
}

/* Function Name:
 *      dal_maple_l2_hashIdx_get
 * Description:
 *      Get L2 hash index based on specified vid and MAC address
 * Input:
 *      unit        - unit id
 *      pMacHashIdx - pointer to vid and mac
 * Output:
 *      pMacHashIdx - pointer to hash indexes
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_UNIT_ID           - invalid unit id
 *      RT_ERR_NOT_INIT          - The module is not initial
 *      RT_ERR_VLAN_VID          - invalid vlan id
 *      RT_ERR_MAC               - invalid mac address
 *      RT_ERR_NULL_POINTER      - input parameter may be null pointer
 * Note:
 *      (1) VID is same as FID in IVL mode.
 *      (2) The pMacHashIdx.vid and pMacHashIdx.mac is input key
 *      (3) validBlk and validAlgo fields in pMacHashIdx specify how many blocks and hash-algo number
 *          the target unit supports.
 */
int32
dal_maple_l2_hashIdx_get(uint32 unit, rtk_l2_macHashIdx_t *pMacHashIdx)
{
    uint64  hashSeed = 0;
    uint32  hash1 = 0, hash3 = 0, hash4 = 0;
    uint32  hash0_idx;
    uint32  hash1_idx;

    /* check Init status */
    RT_INIT_CHK(l2_init[unit]);

    /* parameter check */
    RT_PARAM_CHK(NULL == pMacHashIdx, RT_ERR_NULL_POINTER);

    RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "unit=%d,vid=%p,pMac=%x-%x-%x-%x-%x-%x", unit, pMacHashIdx->vid,
           pMacHashIdx->mac.octet[0], pMacHashIdx->mac.octet[1], pMacHashIdx->mac.octet[2],
           pMacHashIdx->mac.octet[3], pMacHashIdx->mac.octet[4], pMacHashIdx->mac.octet[5]);

    hashSeed = (((uint64)pMacHashIdx->mac.octet[0] << 52) |
                ((uint64)pMacHashIdx->mac.octet[1] << 44) |
                ((uint64)pMacHashIdx->mac.octet[2] << 36) |
                ((uint64)pMacHashIdx->mac.octet[3] << 28) |
                ((uint64)pMacHashIdx->mac.octet[4] << 20) |
                ((uint64)pMacHashIdx->mac.octet[5] << 12) |
                ((uint64)pMacHashIdx->vid));


    /* Algo 0 */
    hash0_idx = (uint32) ((hashSeed >> 55) & BITMASK_9B)
                    ^ ((hashSeed >> 44) & BITMASK_11B)
                    ^ ((hashSeed >> 33) & BITMASK_11B)
                    ^ ((hashSeed >> 22) & BITMASK_11B)
                    ^ ((hashSeed >> 11) & BITMASK_11B)
                    ^ (hashSeed  & BITMASK_11B);

    /* Algo 1 */
    hash1 = (hashSeed>>11) & BITMASK_11B;
    hash1 = ((hash1 & BITMASK_5B) <<6) | ((hash1>>5) & BITMASK_6B);

    hash3 = (hashSeed>>33) & BITMASK_11B;
    hash3 = ((hash3 & BITMASK_6B) <<5) | ((hash3>>6) & BITMASK_5B);

    hash4 = (hashSeed>>44) & BITMASK_11B;
    hash4 = ((hash4 & BITMASK_7B) <<4) | ((hash4>>7) & BITMASK_4B);

    hash1_idx = (uint32) ((hashSeed >> 55) & BITMASK_9B)
                    ^ hash4
                    ^ hash3
                    ^ ((hashSeed >> 22) & BITMASK_11B)
                    ^ hash1
                    ^ (hashSeed  & BITMASK_11B);

    pMacHashIdx->idx[0][0] = hash0_idx;
    pMacHashIdx->idx[0][1] = hash1_idx;
    pMacHashIdx->validBlk = 1;
    pMacHashIdx->validAlgo = 2;

    return RT_ERR_OK;
}

