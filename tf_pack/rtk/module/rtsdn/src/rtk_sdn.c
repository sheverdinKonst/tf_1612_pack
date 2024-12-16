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
 */

/*
 * Include Files
 */
#include <sys/wait.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <private/drv/nic/nic_common.h>
#ifdef CONFIG_SYS_LIB_PKT_IN_OUT
#include <private/drv/nic/nic_pkt2usr.h>
#endif
#include <string.h>
#include "rtk_sdn.h"
#include <osal/memory.h>
#include <osal/sem.h>
#include "sdn_hal.h"
#include "rtk_sdn_task.h"
#include "sdn_db.h"
#include "sdn_util.h"
#include "sdn_type.h"
#include <unistd.h>
#include <rtcore/user/rtcore_drv_usr.h>
#include <private/drv/nic/nic_rx.h>
#ifdef of_table_features
#include "rtrpc_ovs.h"
#include "ovs.h"
#endif
#include <pthread.h>
#include "cht_sdn_resource.h"

/*
 * Symbol Definition
 */
#define RTK_SDN_PKT_QUEUE_SIZE 300
#ifdef __linux__
#define RTK_SDN_SEM_CREATE(sem) pthread_mutex_init(&sem, NULL)
#define RTK_SDN_LOCK(sem) \
{ \
     pthread_mutex_lock(&sem); \
     g_lock_count++; \
    if (g_lock_count > 1) \
    { \
        RTK_SDN_MSG("[%s:%d] dead lock try to lock\n", __FUNCTION__, __LINE__); \
    } \
}while(0)

#define RTK_SDN_UNLOCK(sem) \
{ \
    if (g_lock_count > 1) \
    { \
        RTK_SDN_MSG("[%s:%d] dead lock try to unlock\n", __FUNCTION__, __LINE__); \
    } \
    g_lock_count--; \
    pthread_mutex_unlock(&sem); \
}while(0)
#else
#define RTK_SDN_SEM_CREATE(sem) NULL
#define RTK_SDN_LOCK(sem)       NULL
#define RTK_SDN_UNLOCK(sem)     NULL
#endif
/*
 * Data Declaration
 */

static BOOL_T is_terminated  = FALSE;
static BOOL_T is_linkreader_terminated = FALSE;
//static int    g_rtk_sdn_sock_fd = 0xff;
//static int32  g_rtk_sdn_sem_id   = 0;
//static RTK_SDN_RX_CB_T g_rx_cb_p[RTK_SDN_PKT_RX_END];
static pthread_mutex_t pkt_sem;
//static pthread_mutex_t port_sem;
struct rtk_sdn_pktin_data_s g_rtk_sdn_pkt_queue[RTK_SDN_PKT_QUEUE_SIZE];
int16 g_rtk_sdn_dt_front = -1;
int16 g_rtk_sdn_dt_rear  = -1;
uint8 g_rtk_sdn_module_statue = RTK_SDN_MODULE_UNINITIALIZED;
/*
 * Function Declaration
 */
static int32 rtk_sdn_msg_init(void);
#ifdef CONFIG_SYS_LIB_PKT_IN_OUT
static void rtk_sdn_pkt_reader(void);
#endif
#ifdef CONFIG_SYS_LIB_OVS_LINKCHG
static void rtk_sdn_linkChange_reader(void);
#endif
static int32 rtk_sdn_is_assign_counter(sdn_db_flow_entry_t *flow_p);
#define DO1(buf) crc = crc_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);

/* ========================================================================
 * Table of CRC-32's of all single-byte values (made by make_crc_table)
 */
const uint32 crc_table[256] = {
  0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
  0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
  0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
  0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
  0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
  0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
  0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
  0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
  0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
  0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
  0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
  0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
  0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
  0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
  0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
  0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
  0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
  0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
  0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
  0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
  0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
  0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
  0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
  0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
  0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
  0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
  0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
  0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
  0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
  0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
  0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
  0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
  0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
  0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
  0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
  0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
  0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
  0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
  0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
  0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
  0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
  0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
  0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
  0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
  0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
  0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
  0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
  0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
  0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
  0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
  0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
  0x2d02ef8dL
};

uint32 crc32(uint32 crc, const char *buf, uint32 len)
{
    if (NULL == buf)
        return 0L;

    crc = crc ^ 0xffffffffL;

    while (len >= 8)
    {
        DO8(buf);
        len -= 8;
    }

    if (len)
    {
        do {
            DO1(buf);
        } while (--len);
    }

    return crc ^ 0xffffffffL;
}

#ifdef RTK_CODE
rtk_sdn_switch_features_t switch_features = {true, true, 0, 0, 0};

/* Timer thread for polling flow counter from chip to db */
/* should move to .c file */
#endif
#ifdef CONFIG_ENTRY_HIT_BIT_CHECK /* Add by DPA */
static int rtk_sdn_timer(void);
#endif
static int32 rtk_sdn_msg_init(void)
{
#ifdef __linux__

    RTK_SDN_SEM_CREATE(pkt_sem);

#endif

    return RT_ERR_OK;
}

void rtk_sdn_packet_queue_init(void)
{
    int i = 0;

    for(i = 0; i < RTK_SDN_PKT_QUEUE_SIZE; i++)
    {
        memset(&g_rtk_sdn_pkt_queue[i], 0, sizeof(struct rtk_sdn_pktin_data_s));
    }
}

int32 sdn_hal_2_rtk_sdn_err(sdn_hal_return_t error)
{
    switch (error)
    {
        case SDN_HAL_RETURN_INVALID_METADATA:
            return RTK_SDN_RETURN_INVALID_METADATA;
        case SDN_HAL_RETURN_UNSUPPORTED_INS:
            return RTK_SDN_RETURN_UNSUPPORTED_INS;
        case SDN_HAL_RETURN_UNSUPPORTED_MASK:
            return RTK_SDN_RETURN_UNSUPPORTED_MASK;
        case SDN_HAL_RETURN_BAD_PORT:
            return RTK_SDN_RETURN_BAD_PORT;
        default:
            return RTK_SDN_RETURN_FAILED;
    }
}

pthread_t pid;

#ifdef of_table_features
#if 0
int rtk_sdn_init(struct ofp_table_features tf[HAL_TABLE_NUM])
#else
int rtk_sdn_init(void)
#endif
{
    int ret = RT_ERR_OK;
    struct ofp_table_features tf[MAX_TABLE_NUM];

    g_rtk_sdn_module_statue = RTK_SDN_MODULE_UNINITIALIZED;

    RTK_SDN_MSG("\n[%s][%d]\n",__FUNCTION__,__LINE__);
    if( (ret = sdn_db_init(tf)) != RT_ERR_OK)
        return ret;

    pid = 0;

    /* rtk_sdn's packet-in */
    if (pthread_mutex_init(&pkt_sem, NULL)) {
        DBG_SDN("[ERR] rtk_sdn init : failed to initalize packet-in's mutex\n");
        return RT_ERR_FAILED;
    }

    ret = rtk_sdn_msg_init();
    if(ret != RT_ERR_OK)
    {
        RTK_SDN_MSG("\n[%s][%d] Failed!!!\n",__FUNCTION__,__LINE__);
        return RT_ERR_FAILED;
    }
    rtk_sdn_packet_queue_init();
    g_rtk_sdn_dt_rear = 0;
    g_rtk_sdn_dt_front = 0;
    ret = sdn_hal_init(tf);
    if(ret != SDN_HAL_RETURN_OK)
    {
        RTK_SDN_MSG("\n[%s][%d] Failed!!!\n",__FUNCTION__,__LINE__);
        return RT_ERR_FAILED;
    }

    ret = sdn_db_init_port();
    if(ret != SDN_DB_RETURN_OK)
    {
        RTK_SDN_MSG("\n[%s][%d] Failed!!!\n",__FUNCTION__,__LINE__);
        return RT_ERR_FAILED;
    }

    ret = sdn_db_init_switch();
    if(ret != SDN_DB_RETURN_OK)
    {
        RTK_SDN_MSG("\n[%s][%d] Failed!!!\n",__FUNCTION__,__LINE__);
        return RT_ERR_FAILED;
    }

    ret = sdn_db_init_flow_table();
    if(ret != SDN_HAL_RETURN_OK)
    {
        RTK_SDN_MSG("\n[%s][%d] Failed!!!\n",__FUNCTION__,__LINE__);
        return RT_ERR_FAILED;
    }

    ret = sdn_db_init_meter_table();
    if(ret != SDN_DB_RETURN_OK)
    {
        RTK_SDN_MSG("\n[%s][%d] Failed!!!\n",__FUNCTION__,__LINE__);
        return RT_ERR_FAILED;
    }

    ret = sdn_db_init_table_stat();
    if(ret != SDN_DB_RETURN_OK)
    {
        RTK_SDN_MSG("\n[%s][%d] Failed!!!\n",__FUNCTION__,__LINE__);
        return RT_ERR_FAILED;
    }

    ret = sdn_db_group_table_init();
    if(ret != SDN_DB_RETURN_OK)
    {
        RTK_SDN_MSG("\n[%s][%d] Failed!!!\n",__FUNCTION__,__LINE__);
        return RT_ERR_FAILED;
    }


    ret = sdn_db_init_qos_table();
    if(ret != SDN_DB_RETURN_OK)
    {
        RTK_SDN_MSG("\n[%s][%d] Failed!!!\n",__FUNCTION__,__LINE__);
        return RT_ERR_FAILED;
    }

    is_terminated = TRUE;
    is_linkreader_terminated = TRUE;

#ifdef CONFIG_SYS_LIB_PKT_IN_OUT
    pthread_create((pthread_t *)&pid, NULL, (void *)rtk_sdn_pkt_reader, NULL);
#endif
#ifdef CONFIG_SYS_LIB_OVS_LINKCHG
    pthread_create((pthread_t *)&pid, NULL, (void *)rtk_sdn_linkChange_reader, NULL);
#endif

#ifdef CONFIG_ENTRY_HIT_BIT_CHECK /* Add by DPA */
    if( (ret = rtk_sdn_timer()) != RT_ERR_OK)
        return ret;
#endif
    g_rtk_sdn_module_statue = RTK_SDN_MODULE_INITIALIZED;

    return 0;
}

#else
int rtk_sdn_init()
{
    int ret = RT_ERR_OK;

    if( (ret = sdn_db_init()) != RT_ERR_OK)
        return ret;

    pid = 0;

    rtk_sdn_msg_init();
    rtk_sdn_packet_queue_init();
    g_rtk_sdn_dt_rear = 0;
    g_rtk_sdn_dt_front = 0;
    pthread_create((pthread_t *)&pid, NULL, (void *)rtk_sdn_pkt_reader, NULL);
    pthread_create((pthread_t *)&pid, NULL, (void *)rtk_sdn_linkChange_reader, NULL);

    is_terminated = TRUE;
    if( (ret = rtk_sdn_timer()) != RT_ERR_OK)
        return ret;

    return ret;
}
#endif

#ifdef of_table_features
int rtk_sdn_set_table_features(uint8_t n_table,
                                const struct ofp_table_features *tf)
#else
int rtk_sdn_set_table_features(void)
#endif
{
//FIXME:not finish
    int ret = RT_ERR_OK;
    struct ofp_table_features n_table_feature;

    /* 1. Check input parameters */
#ifdef of_table_features
    RT_PARAM_CHK(((n_table < MIN_TABLE_NUM) || (HAL_TABLE_NUM < n_table)), RT_ERR_OUT_OF_RANGE);
    RT_PARAM_CHK((NULL == tf), RT_ERR_NULL_POINTER);
#endif

    /* 2. Get current features from Database */
#ifdef of_table_features
    ret = sdn_db_get_table_features(n_table, &n_table_feature);
#endif

    /* 3.Apply new feature to hardware layer */
    ret = sdn_hal_table_features_set(n_table, tf);

    /* 4. Apply new features to Database */
#ifdef of_table_features
    memcpy(&n_table_feature, tf, sizeof(n_table_feature));
    ret = sdn_db_set_table_features(n_table, &n_table_feature);
#endif

    return ret;
}

#ifdef of_table_features
int rtk_sdn_get_features(uint8_t n_table,
                                struct ofp_table_features *tf)
#else
int rtk_sdn_get_features(void)
#endif
{
//FIXME:not finish
    int ret = 0;

	/* 1. Check input parameters */
#ifdef of_table_features
    RT_PARAM_CHK(((n_table < (MIN_TABLE_NUM - 1) ) || (HAL_TABLE_NUM < n_table)), RT_ERR_OUT_OF_RANGE);
    RT_PARAM_CHK((NULL == tf), RT_ERR_NULL_POINTER);
#endif

	/* 2. Get current features from Database */
#ifdef of_table_features
    if (sdn_db_get_table_features(n_table, tf) != SDN_DB_RETURN_OK)
    {
        ret = -1;
    }
#endif

    return ret;
}
#ifdef CONFIG_SYS_LIB_OVS_LINKCHG
static void rtk_sdn_linkChange_reader(void)
{
    sdn_hal_port_linkChange_t change;
    uint32_t phy_port;
    uint32_t ofp_port;
    uint32_t unit;

    memset(&change, 0, sizeof(sdn_hal_port_linkChange_t));
    while (is_linkreader_terminated)
    {
        sdn_hal_portLinkChange_get(&change);

        for(unit = 0; unit <  (sizeof(sdn_hal_port_linkChange_t)/sizeof(sdn_hal_portmask_t)); unit++)
        {
            PORTMASK_SCAN(change.portmask[unit], phy_port)
            {
                sdn_hal_phyPort2UserPort(unit, phy_port, &ofp_port);
                sdn_db_port_state_update(ofp_port);
            }
            memset(&change, 0, sizeof(sdn_hal_port_linkChange_t));
        }
    }

}
#endif

#ifndef CONFIG_TURNKEY_NIC
#define NIC_HANDLER_QUEUE_LENGTH    (5)
#define RTNIC_MAX_PKTLEN            (1600)
#endif
#ifdef CONFIG_SYS_LIB_PKT_IN_OUT
static void rtk_sdn_pkt_reader(void)
{
    ski_hook_pins_t *read_pkt_in_datas_p = NULL;
    uint32 pkt_id = 0;
    uint32 usr_port_id = 0;

    read_pkt_in_datas_p = (ski_hook_pins_t *)malloc(sizeof(ski_hook_pins_t));
    if(!read_pkt_in_datas_p)
        RTK_SDN_MSG("\r\n[%s:%d] reader malloc failed \r\n", __FUNCTION__, __LINE__);

    while (is_terminated)
    {
        memset(read_pkt_in_datas_p, 0, sizeof(ski_hook_pins_t));
        for (pkt_id = 0; pkt_id < NIC_HANDLER_QUEUE_LENGTH; pkt_id++)
        {
            read_pkt_in_datas_p->pins[pkt_id].tag.rx_tag.source_port = 0xff;
        }

        drv_nic_pkt_get(0, read_pkt_in_datas_p);

        if (read_pkt_in_datas_p->pins[0].tag.rx_tag.source_port == 0xff)
        {
            RTK_SDN_MSG("\r\n[%s:%d] get packet-in packets failed\r\n", __FUNCTION__, __LINE__);
            continue;
        }

        if ((g_rtk_sdn_dt_rear + NIC_HANDLER_QUEUE_LENGTH) % RTK_SDN_PKT_QUEUE_SIZE == g_rtk_sdn_dt_front)
        {
            usleep(500);
            continue;
        }

        for (pkt_id = 0; pkt_id < NIC_HANDLER_QUEUE_LENGTH; pkt_id++)
        {
            if(read_pkt_in_datas_p->pins[pkt_id].tag.rx_tag.source_port == 0xff)
            {
                break;
            }

            g_rtk_sdn_dt_rear = (g_rtk_sdn_dt_rear + 1) % RTK_SDN_PKT_QUEUE_SIZE;
            memset(&g_rtk_sdn_pkt_queue[g_rtk_sdn_dt_rear], 0, sizeof(struct rtk_sdn_pktin_data_s));
            memcpy(&g_rtk_sdn_pkt_queue[g_rtk_sdn_dt_rear].pkt_buf, read_pkt_in_datas_p->pins[pkt_id].pkt,  read_pkt_in_datas_p->pins[pkt_id].pkt_len);

            g_rtk_sdn_pkt_queue[g_rtk_sdn_dt_rear].tun_id = 0;
            g_rtk_sdn_pkt_queue[g_rtk_sdn_dt_rear].metadata = 0;
            g_rtk_sdn_pkt_queue[g_rtk_sdn_dt_rear].reason   = (read_pkt_in_datas_p->pins[pkt_id].tag.rx_tag.reason == 3 ? EN_OFPR_NO_MATCH : EN_OFPR_ACTION);

            sdn_hal_phyPort2UserPort(0, read_pkt_in_datas_p->pins[pkt_id].tag.rx_tag.source_port, &usr_port_id);
            g_rtk_sdn_pkt_queue[g_rtk_sdn_dt_rear].in_port= usr_port_id;
            g_rtk_sdn_pkt_queue[g_rtk_sdn_dt_rear].pktin_len = read_pkt_in_datas_p->pins[pkt_id].pkt_len;
            /*pkt_len will be modify at last to prevent race condtion.*/

        }
    }

    if (read_pkt_in_datas_p)
    {
        free(read_pkt_in_datas_p);
    }
}
#endif

int rtk_sdn_pkt_rx(void *buf_p,
   size_t pkt_buf_size,
   size_t *pkt_total_len_p,
   int    *reason_p,
   uint64 *tun_id_p,
   uint64 *metada_p,
   uint16_t *in_port_p)
{
    int rc = -1;

    if (g_rtk_sdn_pkt_queue[g_rtk_sdn_dt_front].pktin_len == 0)
    {
        goto packet_end;
    }

    if (g_rtk_sdn_pkt_queue[g_rtk_sdn_dt_front].pktin_len > pkt_buf_size)
    {
        goto packet_end;
    }

    /* TODO:shall add phyport to logicport converter
     */

    osal_memcpy(buf_p, g_rtk_sdn_pkt_queue[g_rtk_sdn_dt_front].pkt_buf, g_rtk_sdn_pkt_queue[g_rtk_sdn_dt_front].pktin_len);

    *pkt_total_len_p = g_rtk_sdn_pkt_queue[g_rtk_sdn_dt_front].pktin_len;
    *reason_p        = g_rtk_sdn_pkt_queue[g_rtk_sdn_dt_front].reason;
    *tun_id_p        = g_rtk_sdn_pkt_queue[g_rtk_sdn_dt_front].tun_id;
    *metada_p        = g_rtk_sdn_pkt_queue[g_rtk_sdn_dt_front].metadata;
    *in_port_p       = g_rtk_sdn_pkt_queue[g_rtk_sdn_dt_front].in_port;
    g_rtk_sdn_pkt_queue[g_rtk_sdn_dt_front].pktin_len = 0;
    rc = 0;

packet_end:
    g_rtk_sdn_dt_front = (g_rtk_sdn_dt_front + 1) % RTK_SDN_PKT_QUEUE_SIZE;

    if (rc == 0)
    {
        RTK_SDN_MSG("\r\n[%s:%d] get packet-in packets upto ovs\r\n", __FUNCTION__, __LINE__);
    }
    return rc;
}


/* Tx packet */
int rtk_sdn_pkt_tx(char *dev_name,
    void *pkt_p,
    size_t pkt_size,
    uint32_t ofp_in_port,
    uint32_t port_no)
{

    //uint32 phy_port_id = 0;

    RTK_SDN_CHECK_IS_NULL(dev_name);
    RTK_SDN_CHECK_IS_NULL(pkt_p);

    //sdn_hal_userPort2PhyPort(ofp_in_port, &phy_port_id);
    if (sdn_db_pkt_tx((char *)dev_name, (void *)pkt_p, (uint32_t) pkt_size, ofp_in_port, port_no) != SDN_DB_RETURN_OK)
    {
        return -1;
    }

    return 0;
}

#if 0
int rtk_sdn_pkt_rx_cb_register(int32 type_id, RTK_SDN_RX_CB_T func_p)
{
    RTK_SDN_CHECK_IS_NULL(func_p);

    g_rx_cb_p[type_id] = func_p;

    return RT_ERR_OK;
}
#endif

#ifdef  CONFIG_SDK_RTL9310
//new group
int32 rtk_sdn_get_groupfeature(EN_OFP_GROUP_TYPE_T type)
{

    return sdn_hal_get_groupfeature(type);
}

int rtk_sdn_add_groupentry(sdn_db_group_entry_t *add_group_entry_p)
{
    if (!add_group_entry_p)
    {
        RTK_SDN_MSG("\r\n[%s:%d] group entry is null \r\n", __FUNCTION__, __LINE__);
        return -1;
    }

   //TODO: apply into sdn_hal and sdn_db
   if ( sdn_hal_add_groupentry(add_group_entry_p) != SDN_HAL_RETURN_OK)
   {
       RTK_SDN_MSG("\r\n[%s:%d] group entry to hal is failed \r\n", __FUNCTION__, __LINE__);
       return -1;
   }
   //apply to db

   if (sdn_db_group_add_group_entry(add_group_entry_p) != SDN_DB_RETURN_OK)
   {
       RTK_SDN_MSG("\r\n[%s:%d] group entry to db is failed \r\n", __FUNCTION__, __LINE__);
       return -1;
   }

    RTK_SDN_MSG("\r\n[%s:%d] group entry join into hal done!!!\n", __FUNCTION__, __LINE__);
    return 0;
}

int rtk_sdn_modify_groupentry(sdn_db_group_entry_t *group_entry_p)
{
    sdn_db_group_entry_t *get_group_entry_p = NULL;

    if (!group_entry_p)
    {
        return -1;
    }

    //TODO del hal group entry
    get_group_entry_p = sdn_db_group_get_entrybygroupid(group_entry_p->entry_id);

   if (!get_group_entry_p)
   {
       RTK_SDN_MSG("\r\n[%s:%d] group(%d) entry is null !!!\n", __FUNCTION__, __LINE__, group_entry_p->entry_id);
       return -1;
   }

   RTK_SDN_MSG("\r\n[%s:%d] try to del group(%d) entry!!!\n", __FUNCTION__, __LINE__, group_entry_p->entry_id);

   if (sdn_hal_del_groupentry(get_group_entry_p) != SDN_HAL_RETURN_OK)
   {
       RTK_SDN_MSG("\r\n[%s:%d] group(%d) entry is deleted failed !!!\n", __FUNCTION__, __LINE__, group_entry_p->entry_id);
       return -1;
   }

   if (sdn_db_group_del_group_entry(get_group_entry_p->entry_id) != SDN_DB_RETURN_OK)
   {
       RTK_SDN_MSG("\r\n[%s:%d] group(%d) entry is deleted failed !!!\n", __FUNCTION__, __LINE__, group_entry_p->entry_id);
       return -1;
   }

   //apply to hal
   if (sdn_hal_add_groupentry(group_entry_p) != SDN_HAL_RETURN_OK )
   {
       RTK_SDN_MSG("\r\n[%s:%d] group(%d) entry is add failed !!!\n", __FUNCTION__, __LINE__, group_entry_p->entry_id);
       return -1;
   }

   //apply to db
   if (sdn_db_group_add_group_entry(group_entry_p) != SDN_DB_RETURN_OK)
   {
       RTK_SDN_MSG("\r\n[%s:%d] group(%d) entry is add failed !!!\n", __FUNCTION__, __LINE__, group_entry_p->entry_id);
       return -1;
   }

    return 0;
}

int rtk_sdn_del_groupentry(uint32_t group_id)
{
    sdn_db_group_entry_t *get_group_entry_p = NULL;

    RTK_SDN_MSG("\r\n[%s:%d] try to del group(%d) entry!!!\n", __FUNCTION__, __LINE__, group_id);

    //TODO del hal group entry
    get_group_entry_p = sdn_db_group_get_entrybygroupid(group_id);

   if (!get_group_entry_p)
   {
       RTK_SDN_MSG("\r\n[%s:%d] group(%d) entry is null !!!\n", __FUNCTION__, __LINE__, group_id);
       return -1;
   }

   RTK_SDN_MSG("\r\n[%s:%d] try to del group(%d) entry!!!\n", __FUNCTION__, __LINE__, group_id);

   if (sdn_hal_del_groupentry(get_group_entry_p) != SDN_HAL_RETURN_OK)
   {
       return -1;
   }

   if (sdn_db_group_del_group_entry(group_id) != SDN_DB_RETURN_OK)
   {
       return -1;
   }
    return 0;
}
//new group

int rtk_sdn_add_group(uint32 group_id, rtk_of_groupType_t type, sdn_hal_apply_actions_t *applied_action_list_p)
{
    if (applied_action_list_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    //TODO: convert of bucket to hal


    if (sdn_db_add_group(group_id, type, applied_action_list_p) != RT_ERR_OK)
    {
        return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
}

int rtk_sdn_modify_group(uint32 group_id, rtk_of_groupType_t *type, sdn_hal_apply_actions_t *applied_action_list_p)
{
    return RT_ERR_OK;
}

int rtk_sdn_delete_group(uint32 group_id, sdn_hal_apply_actions_t *applied_action_list_p)
{
    if (applied_action_list_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    if (sdn_db_delete_group(group_id, applied_action_list_p) != RT_ERR_OK)
    {
        return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
}

/* Classifier */
int rtk_sdn_set_classifier(rtk_of_classifierType_t type, rtk_of_classifierData_t data)
{
//FIXME:not finish
    int ret = RT_ERR_OK;

    /* 1. Check input parameters */

    /* 2. Set classifier to HAL */
    ret = sdn_hal_classifier_set(type, data);

    /* 3. Save classifier to Database */
    ret = sdn_db_set_classifier(type, data);

    return ret;
}
#endif

/* Counter */
int rtk_sdn_get_flow_stats(uint8_t table_id,
                           uint16_t priority,
                           uint8_t match_num,
                           sdn_db_match_field_t *match,
                           /*ofp_match *match,*/
                           uint64_t *packet_count,
                           uint64_t *byte_count)
{
    uint32_t flow_id = 0;
    uint32_t is_found= 0 ;
    /* 1. Check input parameters */
    /* 2. Get flow counter from Database */

#if 1
    /*Brandon:
        1. confirm the flow id exists in the Database is caller's responsibility.
    */

    int ret = RT_ERR_FAILED;

    //if (match == NULL)
    //    return -1;

    //TODO  shall get applied flow from database via match
    if (rtk_sdn_flow_entry_search(table_id, priority,
        match_num, match, &flow_id, &is_found) != 0 )
    {
        return -1;
    }

    IF_TABLE_COUNTER_IS_NOT_AVAILABLE(table_id)
    {
        *packet_count = ~(0);
        *byte_count   = ~(0);

        return 0;
    }

    if ((ret = sdn_db_get_flow_stats( table_id,  flow_id, packet_count, byte_count)) != SDN_DB_RETURN_OK)
    {
        RTK_SDN_MSG("[%s][%d] get flow stats fial, ret=[%d]. flow(%d)\n", __FUNCTION__, __LINE__, ret, flow_id);
        return -1;
    }

    return 0;

#endif
}

int rtk_sdn_apply_flow_entry_counter(uint8_t table_id,
                                     uint32_t flow_id,
                                     uint32_t counter_flag)
{
    /* 1. Check input parameters */
    /* 2. Apply flow counter to HAL */
    /* 3. Save flow counter to Database */
    /*Brandon:
                    1. counter number is far less than flow number. so check if having availible counter for flow_id first.
                    2. from rtk_of_flowCntMode_set(uint32 unit, rtk_of_flowtable_phase_t phase, rtk_of_flow_id_t entry_idx, rtk_of_flowCntMode_t mode);
                        we know count_id in this function is not neccesary.

                    usage:
                                1. rtk_sdn_add_flow()  and get count_id from db.
                                2. call this rtk_sdn_apply_flow_entry_counter to set mode.

    */
    uint32_t count_id = 0xff;
    /*Brandon: get availible counter_id from Database. */
    int ret = RT_ERR_FAILED;

    IF_TABLE_COUNTER_IS_NOT_AVAILABLE(table_id)
    {
        return RT_ERR_OK;
    }

    if ((ret = sdn_db_get_availible_counter_id(table_id, flow_id, &count_id)) != SDN_DB_RETURN_OK ||
            count_id == INVALID_COUNTER_INDEX )
    {
        RTK_SDN_MSG("[%s][%d] failed to get availible counter_id ,ret=[%d].\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    if((ret = sdn_hal_flow_entry_counter_apply(table_id, flow_id, count_id, counter_flag)) != SDN_HAL_RETURN_OK )
    {
        RTK_SDN_MSG("[%s][%d] failed to apply counter_id fort table_id=[%d], flow_id=[%d] to hal, ret=[%d].\n", __FUNCTION__, __LINE__, table_id, flow_id, ret);
        return -1;
    }

    if((ret = sdn_db_apply_flow_entry_counter(table_id, flow_id, count_id, counter_flag)) != SDN_DB_RETURN_OK )
    {
        RTK_SDN_MSG("[%s][%d] fail to apply counter_id fort table_id=[%d], flow_id=[%d] to database , ret=[%d].\n", __FUNCTION__, __LINE__, table_id, flow_id, ret);
        return ret;
    }

    return ret;

}

int rtk_sdn_del_flow_entry_counter(uint8_t table_id,
                                   uint32_t flow_id,
                                   uint32_t count_id)
{
    /* 1. Check input parameters */
    /* 2. Get flow counter in Database */
    /* 3. Delete flow counter to HAL */
    /* 4. Delete flow counter in Database */

#if 1
    /*Brandon:
                    1. every flow entry can directly apply a counter id?   the number of counter is far less than the number of flow entries.
                    2. Can openflow appy flow entry a counter id ?
                    3. from rtk_of_flowCntMode_set(uint32 unit, rtk_of_flowtable_phase_t phase, rtk_of_flow_id_t entry_idx, rtk_of_flowCntMode_t mode);
                        we know count_id in this function is not neccesary.

                    usage:
                                1. call rtk_sdn_del_flow()
                                2. clear  count_id from db.  rtk_sdn_clear_flow_entry_counter
                                3. delete count_id from db.

    */

    int ret = RT_ERR_FAILED;

    IF_TABLE_COUNTER_IS_NOT_AVAILABLE(table_id)
    {
        return RT_ERR_OK;
    }

    if((ret = sdn_db_get_counter_id_by_table_flow(table_id, flow_id, &count_id)) != RT_ERR_OK)
    {
        RTK_SDN_MSG("[%s][%d] fail to get counter_id fort table_id=[%d], flow_id=[%d] from database , ret=[%d].\n", __FUNCTION__, __LINE__, table_id, flow_id, ret);
        return ret;
    }

    if((ret = sdn_hal_flow_entry_counter_delete(table_id, flow_id, count_id)) != RT_ERR_OK)
    {
        RTK_SDN_MSG("[%s][%d] fail to set counter_id fort table_id=[%d], flow_id=[%d] to HAL, ret=[%d] .\n", __FUNCTION__, __LINE__, table_id, flow_id, ret);
        return -1;
    }

    if((ret = sdn_db_del_flow_entry_counter(table_id, flow_id, count_id)) != RT_ERR_OK)
    {
        RTK_SDN_MSG("[%s][%d] fail to set counter_id fort table_id=[%d], flow_id=[%d] to database, ret=[%d] .\n", __FUNCTION__, __LINE__, table_id, flow_id, ret);
        return ret;
    }

    return ret;
#endif

}

int rtk_sdn_clear_flow_entry_counter(uint8_t table_id,
                                     uint32_t flow_id,
                                     uint32_t count_id)
{
    /* 1. Check input parameters */
    /* 2. Get flow counter in Database */
    /* 3. Clear flow counter to HAL */
    /* 4. Clear flow counter in Database */

    int ret = -1;

    IF_TABLE_COUNTER_IS_NOT_AVAILABLE(table_id)
    {
        return RT_ERR_OK;
    }

    if((ret = sdn_db_get_counter_id_by_table_flow(table_id, flow_id, &count_id)) != SDN_DB_RETURN_OK)
    {
        RTK_SDN_MSG("[%s][%d] fail to get counter_id fort table_id=[%d], flow_id=[%d] from database . ret=[%d]\n", __FUNCTION__, __LINE__, table_id, flow_id, ret);
        return 0;
//        return ret;
    }

    if((ret = sdn_hal_flow_entry_counter_clear(table_id, flow_id, count_id)) != SDN_HAL_RETURN_OK)
    {
        RTK_SDN_MSG("[%s][%d] fail to clear counter_id fort table_id=[%d], flow_id=[%d] to HAL . ret=[%d]\n", __FUNCTION__, __LINE__, table_id, flow_id, ret);
        return -1;
    }

    if((ret = sdn_db_clear_flow_entry_counter(table_id, flow_id, count_id)) != SDN_DB_RETURN_OK)
    {
        RTK_SDN_MSG("[%s][%d] fail to clear counter_id fort table_id=[%d], flow_id=[%d] to HAL . ret=[%d]\n", __FUNCTION__, __LINE__, table_id, flow_id, ret);
        return 0;
//        return ret;
    }

    return 0;
}

int rtk_sdn_get_flow_table_stats(uint8_t table_id, rtk_of_flowTblCntType_t type, uint32 *pCnt)
{
    /* 1. Check input parameters */
    /* 2. Get flow table counter from Database */
#if 0
    /* Body of reply to OFPMP_TABLE request. */
    struct ofp_table_stats
    {
        uint8_t table_id; /* Identifier of table. Lower numbered tables are consulted first. */
        uint8_t pad[3]; /* Align to 32-bits. */
        uint32_t active_count; /* Number of active entries. */
        uint64_t lookup_count; /* Number of packets looked up in table. */
        uint64_t matched_count; /* Number of packets that hit table. */
    };
#endif

    /*
    Brandon: this pCnt should be matched_count(uint64) or lookup_count(uint64) ? need to modify *pCnt datatype.
                       9310 has lookup_count and matched_count getting function.
                       So rtk_of_flowTblCntType_t means lookup_count or matched_count ? why get both of them at the same time?
                       Or rtk_of_flowTblCntType_t means byte_base or packet_base?  openflow only defines packet_base.
    */

    int ret = RT_ERR_FAILED;
    uint32_t table_count = 0;

    if (pCnt == NULL)
        return RT_ERR_NULL_POINTER;

    if((ret = sdn_db_get_flow_table_stats(table_id, type, &table_count)) != SDN_DB_RETURN_OK)
    {
        RTK_SDN_MSG("[%s][%d] fail to get stats for table_id=[%d] from database ,ret=[%d].\n", __FUNCTION__, __LINE__, table_id, ret);
        return -1;
    }

    *pCnt = table_count;
    RTK_SDN_MSG("[%s:%d] table(%d) type(%s) counter(%d) \n", __FUNCTION__, __LINE__, table_id, (type == OF_FLOW_TBL_CNT_TYPE_LOOKUP ? "look_up" : "match"), table_count);

    return 0;
}

int rtk_sdn_get_flow_counter(uint8_t table_id,
                           uint16_t priority,
                           uint8_t match_num,
                           sdn_db_match_field_t *match,
                           /*ofp_match *match,*/
                           uint64_t *packet_count,
                           uint64_t *byte_count)
{
    uint32_t flow_id = 0;
    uint32_t is_found= 0 ;
    /* 1. Check input parameters */
    /* 2. Get flow counter from Database */

#if 1
    /*Brandon:
        1. confirm the flow id exists in the Database is caller's responsibility.
    */

    int ret = RT_ERR_FAILED;

    if (match == NULL)
        return -1;

    //TODO  shall get applied flow from database via match
    if (rtk_sdn_flow_entry_search(table_id, priority,
        match_num, match, &flow_id, &is_found) != 0 )
    {
        return RT_ERR_FAILED;
    }

    if(is_found != TRUE)
    {
        return RT_ERR_ENTRY_NOTFOUND;
    }

    IF_TABLE_COUNTER_IS_NOT_AVAILABLE(table_id)
    {
        RTK_SDN_MSG("\n[%s][%d] Table not Support!!!\n",__FUNCTION__,__LINE__);
        *packet_count = ~(0);
        *byte_count   = ~(0);

        return RT_ERR_CHIP_NOT_SUPPORTED;
    }

    if ((ret = sdn_hal_flow_entry_counter_get(table_id, flow_id, packet_count, byte_count)) != SDN_HAL_RETURN_OK)
    {
        RTK_SDN_MSG("[%s][%d] Failed, ret=[%d]. flow(%d)\n", __FUNCTION__, __LINE__, ret, flow_id);
        return RT_ERR_FAILED;
    }

    return RT_ERR_OK;

#endif
}


#ifdef CONFIG_ENTRY_HIT_BIT_CHECK /* Add by DPA */
/* Timer thread for polling flow counter from chip to db */
/* should move to .c file */
static int rtk_sdn_timer(void)
{
    /* 1. wake up per timer event, suppose is 1sec */
    {
        /* 1-1. Get flow table counter from HAL */
        /* 1-2. Get flow entry counter from HAL */
    }

    int ret = RT_ERR_FAILED;

    if ((ret=rtk_sdn_task_init()) != RT_ERR_OK)
    {
        RTK_SDN_MSG("[%s][%d] task init failed, ret=[%d].\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    return ret;
}
#endif
int rtk_sdn_get_port_stats(uint32_t port_no, hal_port_stats_t *port_stats_data)
{
    int ret = RT_ERR_FAILED;

    if((ret = sdn_db_get_port_stats(port_no, port_stats_data)) != SDN_DB_RETURN_OK)
    {
        RTK_SDN_MSG("[%s][%d] fail to get port=[%d] stats  from database . ret=[%d]\n", __FUNCTION__, __LINE__, port_no, ret);
        return -1;
    }

    return 0;
}
void rtk_sdn_thread_stop(void)
{
    is_terminated = FALSE;
    is_linkreader_terminated = FALSE;
    //drv_pkt2usr_stop();
    //usleep(1000000);
    //rtk_ovs_portLinkChange_stop();
    //usleep(1000000);
}

int rtk_sdn_destroy(void)
{
    RTK_SDN_MSG("[%s:%d] start \n", __FUNCTION__, __LINE__);
    rtk_sdn_task_destroy();
    rtk_sdn_thread_stop();
    sdn_db_destroy();
    sdn_hal_destroy();

    RTK_SDN_MSG("[%s:%d] end \n", __FUNCTION__, __LINE__);
    return 0;
}

/*
 * Get netdev etheraddr
 */
int rtk_sdn_eth_addr_get(uint32_t ofp_port, uint8 mac[ETH_ADDR_LEN])
{
    sdn_db_port_t port_desc;

    sdn_db_port_description_get(ofp_port, &port_desc);

    memcpy(mac, port_desc.hw_addr, ETH_ADDR_LEN);
    return 0;
}

int rtk_sdn_port_feature_get(uint32_t ofp_port,
    uint32_t *current, /* enum netdev_features */
    uint32_t *advertised,
    uint32_t *supported,
    uint32_t *peer)
{
    int rc;
    sdn_db_port_t port_desc;

    rc = sdn_db_port_description_get(ofp_port, &port_desc);
    if (rc != SDN_DB_RETURN_OK)
    {
        DBG_SDN("[ERR] [%s] Failed to call sdn_db_port_description_get, rc=%d\n", __FUNCTION__, rc);
        return -1;
    }
    *current = port_desc.feature.curr;
    *advertised = port_desc.feature.advertised;
    *supported = port_desc.feature.supported;
    *peer = port_desc.feature.peer;

    return 0;
}

//TODO:some stuct defination shall not declared from ovs

int rtk_sdn_port_max_get(void)
{
    return sdn_db_port_max_get();
}

int rtk_sdn_ofpport_getByName(const char *name, uint32_t *ofp_port)
{
    uint32_t port;

    sdn_db_ofpport_getByName(name, &port);
    *ofp_port = port;

    return 0;
}

int rtk_sdn_port_add(uint32_t ofp_port)
{
    int32 rc = 0;
    sdn_db_port_t port_desc;
    RTK_SDN_OPENFLOW_RANGE_CHECK(ofp_port);

    rc = sdn_db_port_description_get(ofp_port, &port_desc);
    if(rc != SDN_DB_RETURN_OK)
    {
        DBG_SDN("[ERR] [%s] Failed to call sdn_db_port_description_get, rc=%d\n", __FUNCTION__, rc);
        return -1;
    }

    if(SDN_HAL_RETURN_OK != (rc = sdn_hal_port_add(port_desc.port_no)))
    {
        DBG_SDN("Failed to add port %d as Openflow port.\n", port_desc.port_no);
        return -1;
    }

    /*Update port description to database.*/
    if(SDN_DB_RETURN_OK != (rc = sdn_db_port_add(ofp_port)))
    {
        DBG_SDN("Failed to add port %d as Openflow port.\n", port_desc.port_no);
        return -1;
    }

    return 0;
}

int rtk_sdn_port_del(uint32_t ofp_port)
{
    int32 rc = 0;
    sdn_db_port_t port_desc;
    RTK_SDN_CHECK_IS_OPENFLOW_PORT(ofp_port);

    rc = sdn_db_port_description_get(ofp_port, &port_desc);
    if(rc != SDN_DB_RETURN_OK)
    {
        DBG_SDN("[ERR] [%s] Failed to call sdn_db_port_description_get, rc=%d\n", __FUNCTION__, rc);
        return -1;
    }

    if(SDN_HAL_RETURN_OK != (rc = sdn_hal_port_delete(port_desc.port_no)))
    {
        DBG_SDN("Failed to delete Openflow port %d.\n", port_desc.port_no);
        return -1;
    }

    /*Update port description to database.*/
    if(SDN_DB_RETURN_OK != (rc = sdn_db_port_del(ofp_port)))
    {
        DBG_SDN("Failed to delete Openflow port %d.\n", port_desc.port_no);
        return -1;
    }

    return 0;
}

int rtk_sdn_meter_feature_get(sdn_db_meter_table_features_t *feature)
{
    int rc;

    rc = sdn_db_meter_table_features_get(feature);
    if (rc != SDN_DB_RETURN_OK) {
        DBG_SDN("[ERR] Failed to call opal-get-meter-table-features, rc=%d\n", rc);
        return -1;
    }

    DBG_SDN("RTK SDN meter features : max meters %d\n", feature->max_meters);
    DBG_SDN("RTK SDN meter features : band type bitmaps %d\n", feature->band_types);
    DBG_SDN("RTK SDN meter features : capabilities bitmaps %d\n", feature->capabilities);
    DBG_SDN("RTK SDN meter features : max bands %d\n", feature->max_bands);
    DBG_SDN("RTK SDN meter features : max color %d\n", feature->max_colors);

    return 0;
}

int rtk_sdn_set_flow_entry_usedtime(uint8_t table_id,
    uint16_t priority,
    uint8_t match_num,
    sdn_db_match_field_t *match_p,
    uint32_t used_time)
{
    uint32_t flow_id  = 0;
    uint32_t is_found = 0;

    rtk_sdn_flow_entry_search(table_id, priority,
        match_num, match_p, &flow_id, &is_found);

    if (SDN_DB_RETURN_OK != sdn_db_update_flow_entry_idletimeout(table_id, flow_id, used_time))
    {
        return -1;
    }

    return 0;
}

int rtk_db_get_flow_entry_usedtime(uint8_t table_id,
    uint16_t priority,
    uint8_t match_num,
    sdn_db_match_field_t *match_p,
    uint32_t *used_time_p)
{
    uint32_t flow_id  = 0;
    uint32_t is_found = 0;

    if (used_time_p == NULL)
    {
        return -1;
    }

    rtk_sdn_flow_entry_search(table_id, priority,
        match_num, match_p, &flow_id, &is_found);

    if(SDN_DB_RETURN_OK != sdn_db_get_flow_entry_usedtime(table_id, flow_id, used_time_p))
    {
        return -1;
    }

    return 0;
}

int rtk_sdn_set_table_miss_action(uint8_t table_id, rtk_of_tblMissAct_t action)
{
    RTK_SDN_CHECK_RANGE_IS_VALID(table_id, (rtk_sdn_get_table_max() - 1));
    RTK_SDN_CHECK_RANGE_IS_VALID(action, OF_TBLMISS_ACT_END);

    if (sdn_hal_table_miss_action_set(table_id, action) != SDN_HAL_RETURN_OK)
    {
        return -1;
    }

    if(sdn_db_set_table_miss_action(table_id, action) !=SDN_DB_RETURN_OK)
     {
        return -1;
    }

    return 0;
}

int rtk_sdn_port_config_set(uint32_t ofp_port, sdn_db_port_config_t port_config)
{
    int rc;
    sdn_db_port_t port_desc;
    sdn_hal_port_config_t hal_port_config;

    rc = sdn_db_port_description_get(ofp_port, &port_desc);
    if(rc != SDN_DB_RETURN_OK)
    {
        DBG_SDN("[ERR] [%s] Failed to call sdn_db_port_description_get, rc=%d\n", __FUNCTION__, rc);
        return -1;
    }

    hal_port_config.config = port_config.config;
    hal_port_config.config_mask = port_config.config_mask;

    if (SDN_HAL_RETURN_OK != sdn_hal_port_config_set(port_desc.port_no, &hal_port_config))
    {
        DBG_SDN("Failed to config port.\n");
        return -1;
    }

    return 0;
}
int rtk_sdn_port_carrier_get(uint32_t ofp_port, bool *carrier)
{
    sdn_db_port_t port_desc;
    int rc;

    rc = sdn_db_port_description_get(ofp_port, &port_desc);
    if(rc)
    {
        DBG_SDN("Failed to get port description.\n");
        return -1;
    }

    if (0 == strcmp("br0", (const char*)port_desc.name) ||
        0 == strcmp("lo",  (const char*)port_desc.name) ||
        0 == strcmp("eth0", (const char*)port_desc.name)) {
        *carrier = true;
        return 0;
    }

    if (port_desc.state & EN_OFPPS_LINK_DOWN) {
        *carrier = false;
    } else {
        *carrier = true;
    }
    return 0;
}
static int32 _flow_hash_calc(int priority, int len_match, sdn_db_match_field_t *match_field)
{
    uint32 crc = 0;
    uint32 calc_size = 0;

    /* Calculate the crc sum of priority and match fields (calc_size == 0 is ok) */
    crc = crc32(crc, (void*)&priority, sizeof(priority));
    calc_size = len_match * sizeof(*match_field);
    crc = crc32(crc, (void*)match_field, calc_size);

    return crc;
}

void rtk_sdn_flow_free(sdn_db_flow_entry_t *flow)
{
    uint32 i;

    if(!flow)
    {
        return;
    }

    /* Release match field */
    if(flow->match_field_p)
    {
        free(flow->match_field_p);
    }

    /* Release instruction, if there are any actions or matches, release them */

    for(i = 0; i < flow->len_inst; i++)
    {
        switch(flow->instruction_p[i].type)
        {
            sdn_db_inst_write_actions_t *data_write;
            sdn_db_inst_apply_actions_t *data_apply;
            sdn_db_action_t *action;

            case EN_OFPIT_WRITE_ACTIONS:
            {
                data_write = (sdn_db_inst_write_actions_t*)flow->instruction_p[i].data_p;
                /* For safety check again */
                if(!data_write)
                {
                    break;
                }
                action = data_write->action_set;
                /* For safety check again */
                if(action)
                {
                    free(action);
                }
                break;
            }
            case EN_OFPIT_APPLY_ACTIONS:
            {
                data_apply = (sdn_db_inst_apply_actions_t*)flow->instruction_p[i].data_p;
                /* For safety check again */
                if(!data_apply)
                {
                    break;
                }
                action = data_apply->action_set;
                /* For safety check again */
                if(action)
                {
                    free(action);
                }
                break;
            }
            case EN_OFPIT_CLEAR_ACTIONS:
            case EN_OFPIT_WRITE_METADATA:
            case EN_OFPIT_GOTO_TABLE:
            case EN_OFPIT_METER:
            default:
            {
                break;
            }
        }
        /* The instruction data pointer should also be released */
        if(flow->instruction_p[i].data_p)
        {
            free(flow->instruction_p[i].data_p);
        }
    }

    if(flow->instruction_p)
    {
        free(flow->instruction_p);
    }

    /* Finally clear the root structure */
    memset(flow, 0, sizeof(sdn_db_flow_entry_t));

}

int rtk_sdn_modify_flow(sdn_db_flow_entry_t *flow, bool reset_counter)
{
    int32 ret = 0;
    uint32 is_found = 0;
    uint32 flow_id;
    uint32 hash = 0;
    sdn_db_flow_table_t *table = NULL;

    table = sdn_db_get_flow_table(flow->table_id);
    RTK_SDN_PARAM_CHK(!table, RTK_SDN_RETURN_BAD_TABLE_ID);

    if(!flow)
    {
        return RTK_SDN_RETURN_NULL_DATA;
    }

    hash = _flow_hash_calc(flow->priority, flow->len_match, flow->match_field_p);
    rtk_sdn_flow_entry_search(flow->table_id, flow->priority,
        flow->len_match, flow->match_field_p, &flow_id, &is_found);

    if(is_found != TRUE)
    {
        /* Not found! */
        return -1;
    }
    flow->flow_index = flow_id;
    flow->hash_val = hash;

    ret = sdn_hal_flow_entry_modify(flow);
    if (ret != SDN_HAL_RETURN_OK) {
        DBG_SDN("[ERR] rule modify actions : failed to call sdn_hal_flow_entry_modify, %d\n", ret);
        return sdn_hal_2_rtk_sdn_err(ret);
    }

    if (reset_counter) {
        ret = rtk_sdn_clear_flow_entry_counter(flow->table_id, flow->flow_index, flow->counter_index);
        if (ret != RTK_SDN_RETURN_OK) {
            DBG_SDN("[ERR] rule modify actions : failed to call rtk_sdn_clear_flow_entry_counter, %d\n", ret);
            return -1;
        }
    }

    /* Modify data (database) */
    /* Should free resources in the flow before copying new data */

    rtk_sdn_flow_free(&table->flow_entry[flow->flow_index]);

    if(sdn_db_flow_copy(&table->flow_entry[flow->flow_index], flow) != SDN_DB_RETURN_OK)
    {
        /* Even if the update fails, the record should remain */
        DBG_SDN("[ERR] Modify table_id %d flow_index %d succeeds but update db fails\n", flow->table_id, flow->flow_index);
    }
    return 0;
}

int rtk_sdn_delete_flow(uint8 table_id, uint16 priority, uint32 len_match, sdn_db_match_field_t* flow_match)
{
    int32 ret = 0;
    uint32 flow_count = 0;
    uint32 is_found = 0;
    uint32 flow_id;
    uint32 i = 0;
    sdn_db_flow_table_t *table = NULL;

    table = sdn_db_get_flow_table(table_id);
    RTK_SDN_PARAM_CHK(!table, RTK_SDN_RETURN_BAD_TABLE_ID);

    flow_count = table->flow_count;

    rtk_sdn_flow_entry_search(table_id, priority,
        len_match, flow_match, &flow_id, &is_found);

    if(is_found != TRUE)
    {
        RTK_SDN_MSG("[%s][%d] flow NOT FOUND!!!\n",__FUNCTION__,__LINE__);
        /* Not found! */
        return -1;
    }
    ret = rtk_sdn_clear_flow_entry_counter(table_id, flow_id, table->flow_entry[flow_id].counter_index);
    if (ret != 0) {
        DBG_SDN("[ERR] rule modify actions : failed to call rtk_sdn_clear_flow_entry_counter, %d\n", ret);
        return -1;
    }

    ret = sdn_hal_flow_entry_delete(&table->flow_entry[flow_id]);
    if (ret != SDN_HAL_RETURN_OK) {
        DBG_SDN("[ERR] rule delete : failed to call sdn_hal_flow_entry_delete, %d\n", ret);
        return -1;
    }

    /* Move data (SDN HAL) */
    /* If flow_index == flow_count-1, the last entry is removed
     * Then there is no need to move any entries */
    if(flow_id < (flow_count-1))
    {
        ret = sdn_hal_flow_entry_move(table_id, flow_id, flow_id + 1, (flow_count - flow_id - 1));
        if(ret != SDN_HAL_RETURN_OK)
        {
            DBG_SDN("[SDN] SDN HAL move flow fails, table_id %d, flow_index %d->%d, should move %d entries\n", table_id, flow_id + 1, flow_id, flow_count - flow_id - 1);
            return -1;
        }
    }

    /* Release the resources in the deleted flow */
    rtk_sdn_flow_free(&table->flow_entry[flow_id]);

    /* Move data (database) */
    for(i = flow_id; i < (flow_count - 1); i++)
    {
        /* Here we do not use opal_flow_copy() since we do not need to alloc new space for existing flows */
        memcpy(&table->flow_entry[i], &table->flow_entry[i + 1], sizeof(sdn_db_flow_entry_t));
        /* Update flow index */
        table->flow_entry[i].flow_index = i;
    }
    /* Delete data (database) */
    /* Do not use rtk_sdn_flow_free(). This entry is just moved. The resources in it is still active */
    memset(&table->flow_entry[i], 0, sizeof(sdn_db_flow_entry_t));

    /* Update flow count for future maintenance */
    table->flow_count--;

    return 0;
}

int rtk_sdn_add_flow(sdn_db_flow_entry_t *flow)
{
    uint32 flow_id;
    uint32 is_found = 0;
    uint32 flow_count = 0;
    uint32 caled_flow_id = 0;
    uint32 i = 0;
    uint32 hash = 0;
    int32 ret = 0;
    sdn_db_flow_table_t *table = NULL;
    /*need to check flow */

    table = sdn_db_get_flow_table(flow->table_id);
    RTK_SDN_PARAM_CHK(!table, RTK_SDN_RETURN_BAD_TABLE_ID);

    flow_count = table->flow_count;
    RTK_SDN_MSG("table->flow_count%d \n", table->flow_count);
    RTK_SDN_MSG("table->table_feature.max_entries%d \n", table->table_feature.max_entries);
    RTK_SDN_MSG("table->table_feature.table_id%d \n", table->table_feature.table_id);


    if(!flow || (flow->table_id > SDN_DB_SWITCH_TABLE_NUM_MAX)
        || (flow_count >= table->table_feature.max_entries))
    {
        return RTK_SDN_RETURN_FULL_TABLE;
    }
    hash = _flow_hash_calc(flow->priority, flow->len_match, flow->match_field_p);

    rtk_sdn_flow_entry_search(flow->table_id, flow->priority,
        flow->len_match, flow->match_field_p, &flow_id, &is_found);

    if(is_found == TRUE)
    {
        RTK_SDN_MSG("[%s][%d] flow DUPLICATE!!!\n",__FUNCTION__,__LINE__);
        /* duplicate flow! */
        return -1;
    }

    /* Find an appropriate insert position */
    for(i = 0; i < flow_count; i++)
    {
        if(flow->priority > table->flow_entry[i].priority)
        {
            break;
        }
        else if ((flow->priority ==0)
            && (i == (flow_count -1))    //Treat if  occur collsion to table-miss entry
            && (table->flow_entry[i].priority == 0)
            && (table->flow_entry[i].len_match == 0))
        {
            break;
        }
    }
    /* If the priority of the new flow is the smallest, it will be inserted at the last position */
    caled_flow_id = flow_id;
    flow_id = i;
    flow->flow_index = i;
    flow->hash_val = hash;
    RTK_SDN_MSG("[%s:%d]  flow_id%d \n", __FUNCTION__, __LINE__, flow_id);

    /* Move data ASIC*/
    /* If flow_index == flow_count, the entry is inserted at the last position
     * Then there is no need to move any entries */
    if(flow_id < flow_count)
    {
        ret = sdn_hal_flow_entry_move(flow->table_id, flow_id + 1, flow_id, (flow_count - flow_id));
        if(ret != SDN_HAL_RETURN_OK)
        {
            DBG_SDN("[SDN] HAL move flow fails, table_id %d, flow_index %d->%d, should move %d entries\n", flow->table_id, flow_id, flow_id + 1, flow_count - flow_id);
            return -1;
        }
        RTK_SDN_MSG("sdn_hal_flow_entry_move done! \n");
    }

#ifdef RTK_SDN_DISABLE_COUNTER
    flow->flags |= EN_OFPFF_NO_PKT_COUNTS;
    flow->flags |= EN_OFPFF_NO_BYT_COUNTS;
#endif
    /* Add new data (ASIC) */
    ret = sdn_hal_flow_entry_add(flow);
    if(ret != SDN_HAL_RETURN_OK)
    {
        RTK_SDN_MSG("sdn_hal_flow_entry_add failed! ret = %d\n", ret);
        DBG_SDN("[SDN] SDN HAL add flow fails, table_id %d, flow_index %d, redo the move action\n", flow->table_id, flow_id);
        if(flow_id < flow_count)
        {
            sdn_hal_flow_entry_move(flow->table_id, flow_id, flow_id + 1, flow_count - flow_id);
        }
        return sdn_hal_2_rtk_sdn_err(ret);
    }
    RTK_SDN_MSG("sdn_hal_flow_entry_add done! \n");

    if (rtk_sdn_is_assign_counter(flow) == RTK_SDN_RETURN_OK)
    {
       //TODO:assign counter
       if (rtk_sdn_apply_flow_entry_counter(flow->table_id, caled_flow_id, flow->flags))
       {
           return -1;
       }
    }

    RTK_SDN_MSG("rtk_sdn_apply_flow_entry_counter done! \n");

    /* Move data (database) */
    /* We have made sure flow_count < max_entry. This will be safe */
    for(i = flow_count; i > flow_id; i--)
    {
        /* Here we do not use opal_flow_copy() since we do not need to alloc new space for existing flows */
        memcpy(&table->flow_entry[i], &table->flow_entry[i-1], sizeof(sdn_db_flow_entry_t));
        /* Update flow index */
        table->flow_entry[i].flow_index = i;
    }

    /* Add new data (database) */
    if(sdn_db_flow_copy(&table->flow_entry[flow_id], flow) != SDN_DB_RETURN_OK)
    {
        /* Even if the update fails, the record should remain */
        DBG_SDN("[SDN] Add table_id %d flow_index %d succeeds but update db fails\n", flow->table_id, flow_id);
    }

    /* Update flow count for future maintenance */
    table->flow_count++;
    RTK_SDN_MSG("table->flow_count%d \n", table->flow_count);

    return 0;

}

int matchFiled_duplicate_check(sdn_db_match_field_t *match, int match_num)
{
    int pos;
    int64 match_bitmap = 0;

    for(pos = 0; pos < match_num; pos++)
    {
        if( (1 < match[pos].oxm_type) & match_bitmap)
            return -1;
        else
            match_bitmap |= (1 < match[pos].oxm_type);
    }

    return 0;
}

int prereq_position_check(sdn_db_match_field_t *match, int match_num)
{
    int pos;
    int eth_type_pos = 0;
    int ip_proto_pos = 0;
    int icmpv6_pos = 0;

    /*Find pre-requisite position.*/
    for(pos = 0; pos < match_num; pos++)
    {
        if(match[pos].oxm_type == EN_OFPXMT_OFB_ETH_TYPE)
        {
            eth_type_pos = pos;
        }

        if(match[pos].oxm_type == EN_OFPXMT_OFB_IP_PROTO)
        {
            ip_proto_pos = pos;
        }

        if(match[pos].oxm_type == EN_OFPXMT_OFB_ICMPV6_TYPE)
        {
            icmpv6_pos = pos;
        }

    }

    for(pos = 0; pos < match_num; pos++)
    {
        switch(match[pos].oxm_type)
        {
            case EN_OFPXMT_OFB_IP_DSCP:
            case EN_OFPXMT_OFB_IP_ECN:
            case EN_OFPXMT_OFB_IP_PROTO:
            case EN_OFPXMT_OFB_IPV4_SRC:
            case EN_OFPXMT_OFB_IPV4_DST:
            case EN_OFPXMT_OFB_ARP_OP:
            case EN_OFPXMT_OFB_ARP_SPA:
            case EN_OFPXMT_OFB_ARP_TPA:
            case EN_OFPXMT_OFB_ARP_SHA:
            case EN_OFPXMT_OFB_ARP_THA:
            case EN_OFPXMT_OFB_IPV6_SRC:
            case EN_OFPXMT_OFB_IPV6_DST:
            case EN_OFPXMT_OFB_IPV6_FLABEL:
            case EN_OFPXMT_OFB_MPLS_LABEL:
            case EN_OFPXMT_OFB_MPLS_TC:
            case EN_OFPXMT_OFB_MPLS_BOS:
            case EN_OFPXMT_OFB_IPV6_EXTHDR:
            case EN_OFPXMT_OFB_PBB_ISID:
            {
                if (pos < eth_type_pos)
                    return -1;
                break;
            }
            case EN_OFPXMT_OFB_TCP_SRC:
            case EN_OFPXMT_OFB_TCP_DST:
            case EN_OFPXMT_OFB_UDP_SRC:
            case EN_OFPXMT_OFB_UDP_DST:
            case EN_OFPXMT_OFB_SCTP_SRC:
            case EN_OFPXMT_OFB_SCTP_DST:
            case EN_OFPXMT_OFB_ICMPV6_TYPE:
            case EN_OFPXMT_OFB_ICMPV6_CODE:
            case EN_OFPXMT_OFB_ICMPV4_TYPE:
            case EN_OFPXMT_OFB_ICMPV4_CODE:
            case EN_OFPXMT_RTK_EXP_GTP_TEID:
            {
                if (pos < ip_proto_pos)
                    return -1;
                break;
            }
            case EN_OFPXMT_OFB_IPV6_ND_TARGET:
            case EN_OFPXMT_OFB_IPV6_ND_SLL:
            case EN_OFPXMT_OFB_IPV6_ND_TLL:
            {
                if (pos < icmpv6_pos)
                    return -1;
                break;
            }
            default:
                continue;

        }

    }
    return 0;
}
int require_match_field_check(void)
{
    int i;
    int64 required_match;
    int64 table_match = 0;
    struct ofp_table_features n_table_feature;

    required_match = (1 << EN_OFPXMT_OFB_IN_PORT)\
        | (1 << EN_OFPXMT_OFB_ETH_DST)\
        | (1 << EN_OFPXMT_OFB_ETH_SRC)\
        | (1 << EN_OFPXMT_OFB_ETH_TYPE)\
        | (1 << EN_OFPXMT_OFB_IP_PROTO)\
        | (1 << EN_OFPXMT_OFB_IPV4_SRC)\
        | (1 << EN_OFPXMT_OFB_IPV4_DST)\
        | (1 << EN_OFPXMT_OFB_TCP_SRC)\
        | (1 << EN_OFPXMT_OFB_TCP_DST)\
        | (1 << EN_OFPXMT_OFB_UDP_SRC)\
        | (1 << EN_OFPXMT_OFB_UDP_DST);

    for (i = 0; i < HAL_TABLE_NUM; i++)
    {
        sdn_db_get_table_features(i, &n_table_feature);
        table_match |= n_table_feature.metadata_match;
    }

    if ((table_match & required_match) != required_match)
        return -1;
    return 0;
}


int rtk_sdn_flow_entry_search(uint32 table_id, uint32 priority, uint32 len_match, sdn_db_match_field_t* flow_match, uint32 *flow_index, uint32 *is_found)
{
    uint32  flow_count = 0;
    uint32  hash = 0;
    uint32  i;
    sdn_db_flow_table_t *table = NULL;

    table = sdn_db_get_flow_table(table_id);
    RTK_SDN_PARAM_CHK(!table, RTK_SDN_RETURN_BAD_TABLE_ID);

    flow_count = table->flow_count;

    hash = _flow_hash_calc(priority, len_match, flow_match);

    for(i = 0; i < flow_count; i++)
    {
        if(table->flow_entry[i].hash_val == hash)
        {
            /* Considering hash collision, we should check all keys */
            if((table->flow_entry[i].priority == priority) &&
                (table->flow_entry[i].len_match == len_match) &&
                (!memcmp(table->flow_entry[i].match_field_p, flow_match, len_match * sizeof(sdn_db_match_field_t))))
            {
                if (is_found)
                {
                   *is_found = TRUE;
                }
                break;
            }
        }
    }

    *flow_index = i;

    return 0;
}

#ifdef RTK_CODE

int rtk_sdn_switch_capabilities_get(switch_capabilities *capabilities)
{
    return 0;
}

int rtk_sdn_switch_config_get(ofp_config_flags *sw_flags)
{
    return 0;
}

int rtk_sdn_switch_config_set(ofp_config_flags sw_flags)
{
    return 0;
}
#endif

/*
 * Gets number of implemented OpenFlow tables
 */
uint8_t rtk_sdn_get_table_max()
{
    if (SDN_DB_SWITCH)
    {
        if (SDN_DB_SWITCH->virtual_table_exist) {
            return SDN_DB_SWITCH_TABLE_NUM_MAX - 1;
        } else {
            return SDN_DB_SWITCH_TABLE_NUM_MAX;
        }
    }
    else
    {
        //if null return a value
        return 5;
    }
}

int rtk_sdn_add_meter(const sdn_db_meter_mod_t *mm)
{
    int rc;
    if((!mm) || METER_ID_IS_INVALID(mm->meter_id))
    {
        return -1;
    }

    /* Add new data (SDN HAL) */
    rc = sdn_hal_meter_entry_add(mm);
    if(rc != SDN_HAL_RETURN_OK)
    {
        DBG_SDN("[SDN_DB] add meter fails, meter_id %d\n", meter_id);
        return -1;
    }

    rc = sdn_db_meter_entry_add(mm);
    if (rc != SDN_DB_RETURN_OK) {
        DBG_SDN("[ERR] failed to call sdn_db_meter_entry_add, %d\n", rc);
        return -1;
    }

    return 0;

}

int rtk_sdn_modify_meter(const sdn_db_meter_mod_t *mm)
{
    int rc;

    if((!mm) || METER_ID_IS_INVALID(mm->meter_id))
    {
        return -1;
    }
    /* Modify data (HAL) */
    rc = sdn_hal_meter_entry_modify(mm);
    if(rc != SDN_HAL_RETURN_OK)
    {
        DBG_SDN("[SDN_DB] add meter fails, meter_id %d\n", meter_id);
        return -1;
    }

    rc = sdn_db_meter_entry_modify(mm);
    if (rc != SDN_DB_RETURN_OK) {
        DBG_SDN("[ERR] failed to call sdn_db_meter_entry_modify, %d\n", rc);
        return -1;
    }

    return 0;

}
int rtk_sdn_delete_meter(uint32_t meter_id)
{
    int rc = 0;

    if(METER_ID_IS_INVALID(meter_id))
    {
        return -1;
    }

    /* Delete data (SDN HAL) */
    rc = sdn_hal_meter_entry_del(meter_id);
    if(rc != SDN_HAL_RETURN_OK)
    {
        DBG_SDN("[SDN_DB] add meter fails, meter_id %d\n", meter_id);
        return -1;
    }

    rc = sdn_db_meter_entry_delete(meter_id);
    if (rc != SDN_DB_RETURN_OK) {
        DBG_SDN("[ERR] failed to call opal-meter-delete, %d\n", rc);
        return -1;
    }

    return 0;

}

void rtk_sdn_free_ofp_inst(sdn_db_flow_entry_t *pFlow)
{
    unsigned int i;
    if (pFlow == NULL)
    {
        return;
    }

    if (pFlow->match_field_p)
    {
        free(pFlow->match_field_p);
    }

    if (pFlow->instruction_p)
    {
        for (i = 0; i < pFlow->len_inst; ++i)
        {
            if (pFlow->instruction_p[i].type == EN_OFPIT_WRITE_ACTIONS
                || pFlow->instruction_p[i].type == EN_OFPIT_APPLY_ACTIONS)
            {
                free(pFlow->instruction_p[i].data_p);
                pFlow->instruction_p[i].data_p = NULL;
            }
        }
    }

    if (pFlow->instruction_p)
    {
        free(pFlow->instruction_p);
    }
}
static int32 rtk_sdn_is_assign_counter(sdn_db_flow_entry_t *flow_p)
{
    if (flow_p == NULL)
    {
        return  RTK_SDN_RETURN_NULL_DATA;
    }

    if ((flow_p->flags & EN_OFPFF_NO_PKT_COUNTS) && (flow_p->flags  & EN_OFPFF_NO_BYT_COUNTS))
    {
       return  RTK_SDN_RETURN_FAILED;
    }

    return  RTK_SDN_RETURN_OK;
}

int rtk_sdn_get_switch_feature(uint32_t *switch_capabilities_p)
{
    if (switch_capabilities_p)
    {
        if(sdn_db_get_switch_feature(switch_capabilities_p) != SDN_DB_RETURN_OK)
        {
            return RTK_SDN_RETURN_FAILED;
        }

        return  RTK_SDN_RETURN_OK;
    }

    return RTK_SDN_RETURN_FAILED;
}

int rtk_sdn_get_port_queueconfig(uint32_t port, uint32_t queue_id, uint32_t *max_rate_p, uint32_t *min_rate_p)
{
    uint32_t phy_port = 0xff;
    //uint32_t queue_id = 0xff;
    uint32_t max_rate = 0xff;
    uint32_t min_rate = 0xff;

    if ( /*(port_p == NULL)
         || (queue_id_p == NULL)
         ||*/ (max_rate_p == NULL)
         || (min_rate_p == NULL)
       )
    {
        return RTK_SDN_RETURN_FAILED;
    }

    sdn_hal_userPort2PhyPort(port, &phy_port);
    //queue_id = *queue_id_p;
    RTK_SDN_MSG("[%s:%d] port(%d), phy_port(%d), queue(%d) max rate(%08x) min rate(%08x)\n ", __FUNCTION__, __LINE__, port, phy_port , queue_id, max_rate, min_rate);


    if (SDN_HAL_RETURN_OK != sdn_hal_get_port_queueconfig(phy_port, queue_id, &max_rate, &min_rate))
    {
        return RTK_SDN_RETURN_FAILED;
    }

    *max_rate_p = max_rate;
    *min_rate_p = min_rate;

    RTK_SDN_MSG("[%s:%d] port(%d), phy_port(%d), queue(%d) max rate(%08x) min rate(%08x)\n ", __FUNCTION__, __LINE__, port, phy_port, queue_id, max_rate, min_rate);

    return RTK_SDN_RETURN_OK;
}

int rtk_sdn_get_port_queuestats(uint32_t port, uint32_t queue_id, uint32_t *tx_packets_p, uint32_t *tx_error_p)
{
    uint32_t phy_port = 0xff;

    if ( /*(port_p == NULL)
         || (queue_id_p == NULL)
         ||*/ (tx_packets_p == NULL)
         || (tx_error_p == NULL)
       )
    {
        return RTK_SDN_RETURN_FAILED;
    }

    //sdn_hal_userPort2PhyPort(*port_p, &phy_port);
    phy_port = port;

    RTK_SDN_MSG("[%s:%d] try to get port(%d) queue(%d) stat\n", __FUNCTION__, __LINE__, phy_port, queue_id);
    if (SDN_HAL_RETURN_OK != sdn_hal_get_port_queuestats(phy_port, queue_id, tx_packets_p, tx_error_p))
    {
        return RTK_SDN_RETURN_FAILED;
    }

    return RTK_SDN_RETURN_OK;
}

//qos code ++
int rtk_sdn_set_qos_entry(sdn_db_qos_entry_t qos_entry)
{
    RTK_SDN_MSG("[%s:%d] try to set queue(%d) profile\n", __FUNCTION__, __LINE__, qos_entry.qid);

    if (SDN_DB_RETURN_OK != sdn_db_set_qos_entry(qos_entry))
    {
        RTK_SDN_MSG("[%s:%d] try to set queue(%d) profile failed\n", __FUNCTION__, __LINE__, qos_entry.qid);
        return RTK_SDN_RETURN_FAILED;
    }

    if (SDN_HAL_RETURN_OK != sdn_hal_set_queueconfig(qos_entry))
    {
        RTK_SDN_MSG("[%s:%d] try to set queue(%d) profile failed\n", __FUNCTION__, __LINE__, qos_entry.qid);
        return RTK_SDN_RETURN_FAILED;
    }

    return RTK_SDN_RETURN_OK;
}

int rtk_sdn_del_qos_entry(sdn_db_qos_entry_t qos_entry)
{
    sdn_db_qos_entry_t local_qos_entry;

    if (SDN_DB_RETURN_OK != sdn_db_del_qos_entry(qos_entry))
    {
        RTK_SDN_MSG("[%s:%d] try to del queue(%d) profile failed\n", __FUNCTION__, __LINE__, qos_entry.qid);
        return RTK_SDN_RETURN_FAILED;
    }

    memset(&local_qos_entry, 0, sizeof(local_qos_entry));
    local_qos_entry.qid = qos_entry.qid;

    if (SDN_DB_RETURN_OK != sdn_db_get_qos_entry(&local_qos_entry))
    {
        return RTK_SDN_RETURN_FAILED;
    }

    if (SDN_HAL_RETURN_OK != sdn_hal_set_queueconfig(local_qos_entry))
    {
        return RTK_SDN_RETURN_FAILED;
    }

    return RTK_SDN_RETURN_OK;
}

int rtk_sdn_get_qos_entry(sdn_db_qos_entry_t *qos_entry_p)
{
    if (SDN_DB_RETURN_OK != sdn_db_get_qos_entry(qos_entry_p))
    {
        return RTK_SDN_RETURN_FAILED;
    }

    return RTK_SDN_RETURN_OK;
}

int rtk_sdn_set_port_qos(uint32_t lport, uint32_t q_id)
{
    uint32_t phy_port = 0xff;
    sdn_db_qos_entry_t *qos_entry_p = NULL;

    sdn_hal_userPort2PhyPort(lport, &phy_port);

    RTK_SDN_MSG("[%s:%d] try to set queue(%d) profile in port(%d)\n", __FUNCTION__, __LINE__, q_id, phy_port);
    if (SDN_DB_RETURN_OK != sdn_db_set_port_qos(phy_port, q_id))
    {
        RTK_SDN_MSG("[%s:%d] try to set queue(%d) profile failed\n", __FUNCTION__, __LINE__, q_id);
        return RTK_SDN_RETURN_FAILED;
    }

    qos_entry_p = sdn_db_get_port_qos(phy_port);

    if (qos_entry_p == NULL)
    {
        return RTK_SDN_RETURN_FAILED;
    }

    //TODO set qos into port via hal
    if (SDN_HAL_RETURN_OK != sdn_hal_set_port_queueconfig(phy_port, (*qos_entry_p)) )
    {
        RTK_SDN_MSG("[%s:%d] try to set queue(%d) profile failed\n", __FUNCTION__, __LINE__, q_id);
        return RTK_SDN_RETURN_FAILED;
    }

    return RTK_SDN_RETURN_OK;
}

int rtk_sdn_del_port_qos(uint32_t lport, uint32_t q_id)
{
    uint32_t phy_port = 0xff;
    sdn_db_qos_entry_t *qos_entry_p = NULL;
    sdn_db_qos_entry_t qos_entry;

    sdn_hal_userPort2PhyPort(lport, &phy_port);

    if (SDN_DB_RETURN_OK != sdn_db_del_port_qos(phy_port, q_id))
    {
        return RTK_SDN_RETURN_FAILED;
    }

    qos_entry_p = sdn_db_get_port_qos(phy_port);

    if (qos_entry_p == NULL)
    {
        memset(&qos_entry, 0, sizeof(qos_entry));
        qos_entry.qid = q_id;
        sdn_db_get_qos_entry(&qos_entry);
        qos_entry_p = &qos_entry;
        //return RTK_SDN_RETURN_FAILED;
    }

    if (SDN_HAL_RETURN_OK != sdn_hal_del_port_queueconfig(phy_port, (*qos_entry_p)) )
    {
        return RTK_SDN_RETURN_FAILED;
    }

    return RTK_SDN_RETURN_OK;
}

int cht_sdn_get_block_info(uint16_t priority, uint32_t *location, uint32_t *size)
{
    switch(priority){
        case CHT_L2FWD_NORMAL_PRI:
            *location = CHT_L2FWD_NORMAL_LOCATION;
            *size = CHT_L2FWD_NORMAL_SIZE;
            break;
        case CHT_L2FWD_DEF_PRI:
            *location = CHT_L2FWD_DEF_LOCATION;
            *size = CHT_L2FWD_DEF_SIZE;
            break;
        case CHT_TEID_NORMAL_PRI:
            *location = CHT_TEID_NORMAL_LOCATION;
            *size = CHT_TEID_NORMAL_SIZE;
            break;
        case CHT_TEID_DEF_PRI:
            *location = CHT_TEID_DEF_LOCATION;
            *size = CHT_TEID_DEF_SIZE;
            break;
        default:
            RTK_SDN_MSG("[%s:%d] failed\n", __FUNCTION__, __LINE__);
            return RTK_SDN_RETURN_FAILED;

    }
    return RTK_SDN_RETURN_OK;
}

/* Function Name:
 *      cht_sdn_flow_entry_search
 * Description:
 *      Delete entry from TEID_LEARN/L2_FWD table
 * Input:
 *      table_id        - ASIC table id
 *      priority        - entry priority
 *      len_match       - match field number
 *      flow_match      - match field date
 * Output:
 *      flow_index      - index of found enrty
 *      is_found        - result
 * Return:
 *      SDN_HAL_RETURN_OK
 *      SDN_HAL_RETURN_FAILED
 * Applicable:
 *      9310
 * Note:
 *
 * Changes:
 *      None
 */
int cht_sdn_flow_entry_search(uint32 table_id, uint32 priority, uint32 len_match, sdn_db_match_field_t* flow_match, uint32 *flow_index, uint32 *is_found)
{
    uint32  hash = 0;
    uint32  i;
    sdn_db_flow_table_t *table = NULL;
    uint32_t blk_location = 0;
    uint32_t blk_size = 0;

    table = sdn_db_get_flow_table(table_id);
    RTK_SDN_PARAM_CHK(!table, RTK_SDN_RETURN_BAD_TABLE_ID);

    cht_sdn_get_block_info(priority, &blk_location,&blk_size);
    RTK_SDN_MSG("[%s:%d] blk_location = %d, blk_size = %d\n", __FUNCTION__, __LINE__,blk_location,blk_size);
    hash = _flow_hash_calc(priority, len_match, flow_match);

    for(i = blk_location; i < (blk_location + blk_size); i++)
    {
        if(table->flow_entry[i].hash_val == hash)
        {
            /* Considering hash collision, we should check all keys */
            if((table->flow_entry[i].priority == priority) &&
                (table->flow_entry[i].len_match == len_match) &&
                (!memcmp(table->flow_entry[i].match_field_p, flow_match, len_match * sizeof(sdn_db_match_field_t))))
            {
                if (is_found)
                {
                   *is_found = TRUE;
                }
                break;
            }
        }
    }

    *flow_index = i;

    return RTK_SDN_RETURN_OK;
}


int cht_sdn_flow_add_search(uint32 table_id, uint32 priority, uint32 *location, uint32 *is_found)
{
    uint32  i;
    sdn_db_flow_table_t *table = NULL;
    uint32_t blk_location = 0;
    uint32_t blk_size = 0;
    int32 ret = RTK_SDN_RETURN_FAILED;

    *is_found = FALSE;

    table = sdn_db_get_flow_table(table_id);
    RTK_SDN_PARAM_CHK(!table, RTK_SDN_RETURN_BAD_TABLE_ID);

    ret = cht_sdn_get_block_info(priority, &blk_location,&blk_size);
    if(ret != RTK_SDN_RETURN_OK)
        return ret;

    RTK_SDN_MSG("[%s:%d] blk_location = %d, blk_size = %d\n", __FUNCTION__, __LINE__,blk_location,blk_size);

    for(i = blk_location; i < (blk_location + blk_size); i++)
    {
        if(table->flow_entry[i].priority < priority)
        {
            if (is_found)
                *is_found = TRUE;
            *location = i;
            break;
        }
    }
    RTK_SDN_MSG("[%s:%d] location = %d, is_found = %d\n", __FUNCTION__, __LINE__,*location,*is_found);

    return RTK_SDN_RETURN_OK;
}


/* Function Name:
 *      cht_sdn_add_flow
 * Description:
 *      Add entry into TEID_LEARN/L2_FWD table
 * Input:
 *      flow            - entry data
 * Output:
 *      None
 * Return:
 *      SDN_HAL_RETURN_OK
 *      SDN_HAL_RETURN_FAILED
 * Applicable:
 *      9310
 * Note:
 *
 * Changes:
 *      None
 */
int cht_sdn_add_flow(sdn_db_flow_entry_t *flow)
{
    uint32 flow_id;
    uint32 is_found = 0;
    uint32 flow_count = 0;
    uint32 hash = 0;
    int32 ret = 0;
    sdn_db_flow_table_t *table = NULL;
    uint32 add_location = 0;

    table = sdn_db_get_flow_table(flow->table_id);
    RTK_SDN_PARAM_CHK(!table, RTK_SDN_RETURN_BAD_TABLE_ID);

    flow_count = table->flow_count;
    RTK_SDN_MSG("table->flow_count%d \n", table->flow_count);
    RTK_SDN_MSG("table->table_feature.max_entries%d \n", table->table_feature.max_entries);
    RTK_SDN_MSG("table->table_feature.table_id%d \n", table->table_feature.table_id);


    if(!flow || (flow->table_id > SDN_DB_SWITCH_TABLE_NUM_MAX)
        || (flow_count >= table->table_feature.max_entries))
    {
        return RTK_SDN_RETURN_FAILED;
    }
    hash = _flow_hash_calc(flow->priority, flow->len_match, flow->match_field_p);

    ret = cht_sdn_flow_entry_search(flow->table_id, flow->priority,
        flow->len_match, flow->match_field_p, &flow_id, &is_found);
    if(ret != RTK_SDN_RETURN_OK)
        return RTK_SDN_RETURN_FAILED;

    if(is_found == TRUE)
    {
        RTK_SDN_MSG("[%s][%d] flow DUPLICATE!!!\n",__FUNCTION__,__LINE__);
        /* duplicate flow! */
        return RTK_SDN_RETURN_FAILED;
    }

    ret = cht_sdn_flow_add_search(flow->table_id, flow->priority,
        &add_location, &is_found);
    if(is_found != TRUE)
    {
        RTK_SDN_MSG("[%s][%d] Table(%d) FULL!!!\n",__FUNCTION__,__LINE__,flow->table_id);
        /* duplicate flow! */
        return RTK_SDN_RETURN_FULL_TABLE;
    }
    /* If the priority of the new flow is the smallest, it will be inserted at the last position */
    flow_id = add_location;
    flow->flow_index = add_location;
    flow->hash_val = hash;
    RTK_SDN_MSG("[%s:%d]  flow_id%d \n", __FUNCTION__, __LINE__, flow_id);

    /* Add new data (ASIC) */
    ret = sdn_hal_flow_entry_add(flow);
    if(ret != SDN_HAL_RETURN_OK)
    {
        return sdn_hal_2_rtk_sdn_err(ret);
    }
    RTK_SDN_MSG("[%s]sdn_hal_flow_entry_add done! \n",__FUNCTION__);

    /* Add new data (database) */
    if(sdn_db_flow_copy(&table->flow_entry[flow_id], flow) != SDN_DB_RETURN_OK)
    {
        /* Even if the update fails, the record should remain */
        DBG_SDN("[%s:%d] Add table_id %d flow_index %d succeeds but update db fails\n", __FUNCTION__, __LINE__,flow->table_id, flow_id);
    }

    /* Update flow count for future maintenance */
    table->flow_count++;
    RTK_SDN_MSG("table->flow_count%d \n", table->flow_count);

    return RTK_SDN_RETURN_OK;

}
/* Function Name:
 *      cht_sdn_delete_flow
 * Description:
 *      Delete entry from TEID_LEARN/L2_FWD table
 * Input:
 *      table_id        - ASIC table id
 *      priority        - entry priority
 *      len_match       - match field number
 *      flow_match      - match field date
 * Output:
 *      None
 * Return:
 *      SDN_HAL_RETURN_OK
 *      SDN_HAL_RETURN_FAILED
 * Applicable:
 *      9310
 * Note:
 *
 * Changes:
 *      None
 */
int cht_sdn_delete_flow(uint8 table_id, uint16 priority, uint32 len_match, sdn_db_match_field_t* flow_match)
{
    int32 ret = RTK_SDN_RETURN_OK;
    uint32 is_found = 0;
    uint32 flow_id;
    sdn_db_flow_table_t *table = NULL;

    table = sdn_db_get_flow_table(table_id);
    RTK_SDN_PARAM_CHK(!table, RTK_SDN_RETURN_BAD_TABLE_ID);

    ret = cht_sdn_flow_entry_search(table_id, priority,
        len_match, flow_match, &flow_id, &is_found);
    if(ret != RTK_SDN_RETURN_OK)
        return RTK_SDN_RETURN_FAILED;

    if(is_found != TRUE)
    {
        RTK_SDN_MSG("[%s][%d] flow NOT FOUND!!!\n",__FUNCTION__,__LINE__);
        /* Not found! */
        return RTK_SDN_RETURN_FAILED;
    }

    ret = sdn_hal_flow_entry_delete(&table->flow_entry[flow_id]);
    if (ret != SDN_HAL_RETURN_OK) {
        DBG_SDN("[ERR] rule delete : failed to call sdn_hal_flow_entry_delete, %d\n", ret);
        return RTK_SDN_RETURN_FAILED;
    }

    /* Release the resources in the deleted flow */
    rtk_sdn_flow_free(&table->flow_entry[flow_id]);

    memset(&table->flow_entry[flow_id], 0, sizeof(sdn_db_flow_entry_t));

    /* Update flow count for future maintenance */
    table->flow_count--;

    return RTK_SDN_RETURN_OK;
}


