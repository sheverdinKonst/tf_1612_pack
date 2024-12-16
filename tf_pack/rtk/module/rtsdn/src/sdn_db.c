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

#include <pthread.h>
#include <string.h>
#include "sdn_db.h"
#include <sdn_hal.h>
static struct ofp_table_features g_tf[HAL_TABLE_NUM];


/*
*  Brandon: assume all tables share common counter size. And design the below database.
*                 If not, need to redesign database for saving memory.
*/

/*Brandon:
                Designing a common database for RTL9300 & RTL9310 is the best.
                If having common database , we only have a different HAL layer API by different chip mapper.
                The above sub-database(not main flow entry database) are designed to a common database.
                For man flow entry database, malloc by number of table defined in switch feature of HAL, will satisfy RTL9300 & RTL9310.
*/

/*This table binding flow and counter should be replace by "man flow_entry database"*/

#ifdef  CONFIG_SDK_RTL9310
//static SDN_DB_TABLE_FLOW_INFO_T  table_flow_info_db;
static sdn_db_output_type_t g_sdn_output_type_db[9] =
{
    { EN_OFPP_MAX       , OF_OUTPUT_TYPE_MULTI_EGR_PORT       },
    { EN_OFPP_IN_PORT   , OF_OUTPUT_TYPE_IN_PORT              },
    { EN_OFPP_TABLE     , OF_OUTPUT_TYPE_END                  },
    { EN_OFPP_NORMAL    , OF_OUTPUT_TYPE_LB                   },
    { EN_OFPP_FLOOD     , OF_OUTPUT_TYPE_FLOOD                },
    { EN_OFPP_ALL       , OF_OUTPUT_TYPE_PHY_PORT_EX_SRC_PORT },
    { EN_OFPP_CONTROLLER, OF_OUTPUT_TYPE_END                  },
    { EN_OFPP_LOCAL     , OF_OUTPUT_TYPE_END                  },
    { EN_OFPP_ANY       , OF_OUTPUT_TYPE_END                  },
};
static SDN_DB_TABLE_STATS_T    table_stats_db[FT_PHASE_END];
#else
static SDN_DB_TABLE_STATS_T    table_stats_db[HAL_TABLE_NUM];
#endif

//qos code ++
static sdn_db_qos_entry_t sdn_db_qos_table[8];
//qos code --

static SDN_DB_FLOW_STATS_T  table_flow_stats_db[HAL_RULE_COUNTER_SIZE];
/*Brandon: all tables share common counter size.*/
/*static SDN_DB_FLOW_STATS_T  table_flow_meter_db[HAL_RULE_COUNTER_SIZE];*/  /*Brandon:maybe need a global variable to record meter usage which all tables share.*/
static pthread_mutex_t       db_stats_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t       db_port_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t       db_table_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t       db_meter_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t       db_qos_mutex   = PTHREAD_MUTEX_INITIALIZER;

uint32_t sdn_db_max_port = 0;
sdn_db_port_t *port_info = NULL;
sdn_db_switch_features_t *switch_info = NULL;
sdn_db_flow_table_t *table_info = NULL;
sdn_db_meter_table_t *meter_table_info = NULL;

#define SDN_DB_LOCK(_sem) \
{ \
   pthread_mutex_lock(_sem); \
}while(0)

#define SDN_DB_UNLOCK(_sem) \
{ \
   pthread_mutex_unlock(_sem); \
}while(0)

#ifdef  CONFIG_SDK_RTL9310
static sdn_db_group_tbl_t   *g_group_table_db_p = NULL;
static sdn_hal_group_table_t *g_group_table_p = NULL;
static rtk_of_classifierType_t g_classifier_type;
static rtk_of_classifierData_t g_classifier_data;

static sdn_db_group_entry_t *sdn_db_group_get_validentryid(uint32_t group_id);

static int32 sdn_db_actionsBucket_create(sdn_hal_apply_actions_t *src_act_p, sdn_hal_apply_actions_t **dst_act_p);
static int32 sdn_db_actionsBucert_insert(sdn_hal_apply_actions_t *src_act_p, sdn_hal_apply_actions_t **dst_act_p);
static int32 sdn_db_actionsBucert_remove(uint32 bucket_id, sdn_hal_apply_actions_t **src_act_p);
static sdn_hal_apply_actions_t *sdn_db_GetActionBucket(uint32 bucket_id, sdn_hal_apply_actions_t *src_act_p);
static int32 sdn_db_localconveroutputmode(EN_OFP_PORT_NO_T of_type, rtk_of_outputType_t *rtk_of_type_p);

static int32 sdn_db_localconveroutputmode(EN_OFP_PORT_NO_T of_type, rtk_of_outputType_t *rtk_of_type_p)
{
    uint32_t type_idx = 0;

    if (rtk_of_type_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    for (type_idx = 0; type_idx < _countof(g_sdn_output_type_db); type_idx++)
    {
        if (g_sdn_output_type_db[type_idx].of_output_type == of_type)
        {
            *rtk_of_type_p = g_sdn_output_type_db[type_idx].rtk_output_type;
            return ((*rtk_of_type_p == OF_OUTPUT_TYPE_END) ? SDN_DB_RETURN_FAILED : SDN_DB_RETURN_OK);
        }
    }

    return SDN_DB_RETURN_OK;
}

static int32 sdn_db_actionsBucket_create(sdn_hal_apply_actions_t *src_act_p, sdn_hal_apply_actions_t **dst_act_p)
{
    if (src_act_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    if (*(dst_act_p) == NULL)
    {
        *(dst_act_p) = (sdn_hal_apply_actions_t *)malloc(sizeof(sdn_hal_apply_actions_t));
    }

    return SDN_DB_RETURN_OK;
}

static int32 sdn_db_actionsBucert_insert(sdn_hal_apply_actions_t *src_act_p, sdn_hal_apply_actions_t **dst_act_p)
{
    uint32 curr_bucket_id = 0xff;
    sdn_hal_apply_actions_t *current_act_p = NULL;
    sdn_hal_apply_actions_t *prev_act_p = NULL;
    uint32 action_id      = 0;

    if (src_act_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    current_act_p = *dst_act_p;
    if (current_act_p == NULL)
    {
        sdn_db_actionsBucket_create(src_act_p, &current_act_p);
        if (current_act_p)
        {
            current_act_p->action_p = (sdn_hal_action_t *) malloc(src_act_p->action_len * sizeof(sdn_hal_action_t));
            current_act_p->bucket_id  = src_act_p->bucket_id;
            current_act_p->action_len = src_act_p->action_len;
            for(action_id = 0; action_id < src_act_p->action_len; action_id++)
            {
                memcpy(&current_act_p->action_p[action_id], &src_act_p->action_p[action_id], sizeof(current_act_p->action_p[action_id]));
            }
            current_act_p->next_act_p = NULL;

            *dst_act_p = current_act_p;

            return SDN_DB_RETURN_OK;
        }
        else
        {
            return RT_ERR_NULL_POINTER;
        }
    }

    prev_act_p = current_act_p;
    curr_bucket_id = prev_act_p->bucket_id;
    while (prev_act_p != NULL)
    {
        prev_act_p = prev_act_p->next_act_p;
        curr_bucket_id++;
    }

    if (prev_act_p == NULL)
    {
        prev_act_p = (sdn_hal_apply_actions_t *)malloc(sizeof(sdn_hal_apply_actions_t));
        prev_act_p->bucket_id  = src_act_p->bucket_id;
        prev_act_p->action_len = src_act_p->action_len;
        prev_act_p->action_p = (sdn_hal_action_t *) malloc(src_act_p->action_len * sizeof(sdn_db_action_t));
        for(action_id = 0; action_id < src_act_p->action_len; action_id++)
        {
            memcpy(&prev_act_p->action_p[action_id], &src_act_p->action_p[action_id], sizeof(prev_act_p->action_p[action_id]));
        }
        prev_act_p->next_act_p    = current_act_p->next_act_p;
        current_act_p->next_act_p = prev_act_p;
    }

    *dst_act_p = current_act_p;

    return SDN_DB_RETURN_OK;
}

static int32 sdn_db_actionsBucert_remove(uint32 bucket_id, sdn_hal_apply_actions_t **src_act_p)
{
    sdn_hal_apply_actions_t *current_node_p= NULL;
    sdn_hal_apply_actions_t *prev_p = NULL;

    if (src_act_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    prev_p        = *src_act_p;
#if 0
    if (action_p != NULL)
    {
        if (memcmp(action_p, prev_p->action_p, sizeof(*action_p)) == 0)
        {
            current_node_p = prev_p;
            prev_p         = prev_p->next_act_p;
            *src_act_p     = prev_p;
            if (current_node_p)
            {
                current_node_p->action_p = NULL;
                free(current_node_p);
            }

            return SDN_DB_RETURN_OK;
        }
    }
#endif
    while(prev_p != NULL)
    {
        if (prev_p->bucket_id == bucket_id)
        {
            current_node_p = prev_p;
            prev_p         = prev_p->next_act_p;
            *src_act_p     = prev_p;
            if (current_node_p)
            {
                if (current_node_p->action_p)
                {
                    free(current_node_p->action_p);
                }
                free(current_node_p);
                current_node_p = NULL;
            }
            return SDN_DB_RETURN_OK;
        }
        prev_p = prev_p->next_act_p;
    }

    return SDN_DB_RETURN_FAILED;
}

static sdn_hal_apply_actions_t *sdn_db_GetActionBucket(uint32 bucket_id, sdn_hal_apply_actions_t *src_act_p)
{
    sdn_hal_apply_actions_t *current_node_p = src_act_p;
    while (current_node_p)
    {
        if (current_node_p->bucket_id == bucket_id)
        {
            return current_node_p;
        }

        current_node_p = current_node_p->next_act_p;
    }

    return NULL;
}

int32 sdn_db_add_group(uint32 group_id, rtk_of_groupType_t type, sdn_hal_apply_actions_t *applied_action_list_p)
{
    sdn_hal_apply_actions_t *prev_p = applied_action_list_p;
    sdn_hal_apply_actions_t *get_bucket_p = NULL;
    sdn_hal_apply_actions_t insert_data;
    uint32 action_id = 0;

    if (prev_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    //TODO:add actons into a group
    if (g_group_table_p != NULL && applied_action_list_p != NULL)
    {
        if (g_group_table_p[group_id - 1].action_buket_num > 128)
        {
            return SDN_DB_RETURN_FAILED;
        }

        get_bucket_p = sdn_db_GetActionBucket(applied_action_list_p->bucket_id, g_group_table_p[group_id - 1].action_buket_p);
        g_group_table_p[group_id - 1].action_buket_num ++;
        g_group_table_p[group_id - 1].group_type = type;

        if (get_bucket_p == NULL)
        {
            while(prev_p != NULL)
            {
                memset(&insert_data, 0, sizeof(insert_data));
                memcpy(&insert_data, applied_action_list_p, sizeof(insert_data));
                insert_data.next_act_p = NULL;
                if (sdn_db_actionsBucert_insert(&insert_data, &g_group_table_p[group_id - 1].action_buket_p) != SDN_DB_RETURN_OK)
                {
                    return SDN_DB_RETURN_FAILED;
                }
                prev_p = prev_p->next_act_p;
            }
        }
        else
        {
            for(action_id = 0; action_id < applied_action_list_p->action_len; action_id++)
            {
                memcpy(&g_group_table_p[group_id - 1].action_buket_p->action_p[action_id], &applied_action_list_p->action_p[action_id], sizeof(g_group_table_p[group_id - 1].action_buket_p->action_p[action_id]));
            }
        }
    }

    if (sdn_hal_add_group(group_id, type, &g_group_table_p[group_id - 1]) != SDN_DB_RETURN_OK)
    {
        return SDN_DB_RETURN_FAILED;
    }

    return SDN_DB_RETURN_OK;
}

int32 sdn_db_set_group(uint32 group_id, rtk_of_groupType_t type, sdn_hal_apply_actions_t *applied_action_list_p)
{
    //TODO:add actons into a group
    if (g_group_table_p != NULL && applied_action_list_p != NULL)
    {
        if (g_group_table_p[group_id - 1].action_buket_num > 128)
        {
            return SDN_DB_RETURN_FAILED;
        }

        g_group_table_p[group_id - 1].action_buket_num ++;
        g_group_table_p[group_id - 1].group_type = type;

        if (sdn_db_actionsBucert_insert(applied_action_list_p, &g_group_table_p[group_id - 1].action_buket_p) == SDN_DB_RETURN_OK)
        {
            return SDN_DB_RETURN_OK;
        }
    }

    return RT_ERR_NULL_POINTER;
}

int32 sdn_db_get_group(uint32 group_id, rtk_of_groupType_t *type, uint32 *bucket_num_p, sdn_hal_apply_actions_t **applied_action_list_p)
{
    sdn_hal_group_table_t *table_p = &g_group_table_p[group_id - 1];
    if (table_p != NULL)
    {
        if (table_p->action_buket_p != NULL)
        {
            *applied_action_list_p = table_p->action_buket_p;
            *type         = (rtk_of_groupType_t)table_p->group_type;
            *bucket_num_p = (uint32)table_p->action_buket_num;
            return SDN_DB_RETURN_OK;
        }
        else
        {
            *applied_action_list_p = table_p->action_buket_p;
            *type         = (rtk_of_groupType_t)table_p->group_type;
            *bucket_num_p = (uint32)table_p->action_buket_num;
            return RT_ERR_NULL_POINTER;
        }
    }

    return RT_ERR_NULL_POINTER;
}

int32 sdn_db_delete_group(uint32 group_id, sdn_hal_apply_actions_t *applied_action_list_p)
{
    sdn_hal_group_table_t *table_p = &g_group_table_p[group_id - 1];

    //TODO:add actons into a group

    if (applied_action_list_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    if (table_p != NULL && applied_action_list_p != NULL)
    {

        if(sdn_db_actionsBucert_remove(table_p->action_buket_p->bucket_id, &table_p->action_buket_p) == SDN_DB_RETURN_OK)
        {
            if (table_p->action_buket_num > 0)
            {
                table_p->action_buket_num --;
            }
        }
    }

    if (table_p->action_buket_p == NULL)
    {
        table_p->group_type     = OF_GROUP_TYPE_END;
    }

    return SDN_DB_RETURN_OK;
}

/* Classifier */
int sdn_db_set_classifier(rtk_of_classifierType_t type, rtk_of_classifierData_t data)
{
    int ret = SDN_DB_RETURN_OK;

    g_classifier_type = type;
    memcpy(&g_classifier_data, &data, sizeof(g_classifier_data));

    return ret;
}
#endif
static int _sdn_db_meter_copy(sdn_db_meter_mod_t *dst, const sdn_db_meter_mod_t *src)
{
    uint32_t len_band = 0;

    if(!src || !dst)
    {
        return SDN_DB_RETURN_INVALID_PARAM;
    }

    /* First copy the whole structure */
    memcpy(dst, src, sizeof(sdn_db_meter_mod_t));

    /* Assign new space for bands */
    len_band = src->n_bands;
    dst->bands = (sdn_db_meter_band_t*)malloc(len_band * sizeof(sdn_db_meter_band_t));
    //dst->band = (OPDB_METER_BAND_T *)mem_handle_alloc(MEM_HANDLE_ID_OPAL, len_band * sizeof(OPDB_METER_BAND_T), __FUNCTION__,__LINE__);
    if(!dst->bands)
    {
        /* Space not enough */
        return SDN_DB_RETURN_MALLOC_FAIL;
    }
    memcpy(dst->bands, src->bands, len_band * sizeof(sdn_db_meter_band_t));

    return SDN_DB_RETURN_OK;
}

static void _sdn_db_meter_free(sdn_db_meter_mod_t *meter)
{
    if(!meter)
    {
        return;
    }

    if(meter->bands)
    {
        free(meter->bands);
        //mem_handle_free(meter->bands);
    }

    /* Finally clear the root structure */
    memset(meter, 0, sizeof(*meter));
}

int sdn_db_pkt_tx(char *dev_name,
                  void *pkt_p,
                  uint32_t pkt_size,
                  uint32_t ofp_in_port,
                  uint32_t port_no)
{
    int rc = SDN_DB_RETURN_OK;
    rtk_of_outputType_t out_type;
    uint32 port_id = 0;
    uint32 phy_port_id = 0;

    if (pkt_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }


#ifdef CONFIG_SDK_RTL9310
    if (sdn_db_localconveroutputmode(port_no, &out_type) != SDN_DB_RETURN_OK)
    {
        return SDN_DB_RETURN_FAILED;
    }
#endif

    switch (port_no | 0xffff0000)
    {
        case EN_OFPP_ALL:
            for (port_id = 1; port_id <= SDN_DB_PORT_MAX; port_id++)
            {
                if (port_id == ofp_in_port || !SDN_DB_OF_PORT_EXIST(port_id))
                {
                    continue;
                }

                sdn_hal_userPort2PhyPort(port_id, &phy_port_id);
                if (sdn_hal_pkt_tx((char *)dev_name, (void *)pkt_p, pkt_size,  &out_type, phy_port_id) != SDN_HAL_RETURN_OK)
                {
                    rc =  SDN_DB_RETURN_FAILED;
                }
            }
        break;

        case EN_OFPP_IN_PORT:
            sdn_hal_userPort2PhyPort(ofp_in_port, &phy_port_id);
            if (sdn_hal_pkt_tx((char *)dev_name, (void *)pkt_p, pkt_size,  &out_type, phy_port_id) != SDN_HAL_RETURN_OK)
            {
                return SDN_DB_RETURN_FAILED;
            }
        break;

        case EN_OFPP_TABLE:
        case EN_OFPP_FLOOD:
        case EN_OFPP_CONTROLLER:
        case EN_OFPP_LOCAL:
            return SDN_DB_RETURN_FAILED;

        default:
            if (port_no > SDN_DB_PORT_MAX || port_no < 1)
            {
                return SDN_DB_RETURN_FAILED;
            }

            sdn_hal_userPort2PhyPort(port_no, &phy_port_id);
            if (sdn_hal_pkt_tx((char *)dev_name, (void *)pkt_p, pkt_size,  &out_type, phy_port_id) != SDN_HAL_RETURN_OK)
            {
                return SDN_DB_RETURN_FAILED;
            }
    }

    return rc;
}

#ifdef of_table_features
int sdn_db_init(struct ofp_table_features tf[HAL_TABLE_NUM])
#else
int sdn_db_init()
#endif
{
#ifdef of_table_features
    uint8_t n_table = 0;
#endif
#ifdef  CONFIG_SDK_RTL9310
    uint32 group_id = 0;
#endif
    int i = 0;

    sdn_db_init_port();
    //sdn_db_init_table();
    sdn_db_init_switch();
    sdn_db_init_flow_table();
    sdn_db_init_meter_table();

    /* 1. Init all database */
#ifdef  CONFIG_SDK_RTL9310
    g_classifier_type = DFLT_SDN_CLASSIFIER_TYPE;
    memset(&g_classifier_data, 0, sizeof(g_classifier_data));
#endif
    memset(g_tf, 0, sizeof(struct ofp_table_features) * HAL_TABLE_NUM);


    /* 2. Apply init features */
#ifdef of_table_features
    for(n_table = 0; n_table < HAL_TABLE_NUM; n_table++)
    {
        sdn_db_set_table_features(n_table, &tf[n_table]);
    }
#endif


    /*Brandon : the initialization of coutner idx, counter_type, meter_idx which used by a flow tables.*/

#ifdef  CONFIG_SDK_RTL9310
    for (i = FT_PHASE_IGR_FT_0; i < FT_PHASE_END ; i++)
    {
        table_stats_db[i].table_id = FT_PHASE_END;
        table_stats_db[i].active_count =  ~0;  /*Brandon: RTK doesn't support active_count. */
        table_stats_db[i].lookup_count =  0;
        table_stats_db[i].matched_count = 0;
    }
#else
    for (i = 0; i < HAL_TABLE_NUM ; i++)
    {
        table_stats_db[i].table_id = INVALID_TABLE_INDEX;
        table_stats_db[i].active_count =  ~0;  /*Brandon: RTK doesn't support active_count. */
        table_stats_db[i].lookup_count =  0;
        table_stats_db[i].matched_count = 0;
    }

#endif

    /*Brandon : the initialization of coutner idx, counter_type, meter_idx which used by a flow entry.*/
#if 0
    for(i = 0; i < HAL_INGRESS_FULL_MATCH_TABLE0_RULE_NUM; i++)
    {
        table_flow_info_db.egress_table0_info[i].counter_index = HAL_RULE_INVAILD_COUNTER_ID;
        table_flow_info_db.egress_table0_info[i].counter_type = ((hal_rule_stats_type_t)0);
        table_flow_info_db.egress_table0_info[i].meter_index = INVALID_METER_INDEX;
    }

    for(i = 0; i < HAL_INGRESS_L2_TABLE1_RULE_NUM; i++)
    {
        table_flow_info_db.ingress_table1_info[i].counter_index = HAL_RULE_INVAILD_COUNTER_ID;
        table_flow_info_db.ingress_table1_info[i].counter_type = ((hal_rule_stats_type_t)0);
        table_flow_info_db.ingress_table1_info[i].meter_index = INVALID_METER_INDEX;
    }

    for(i = 0; i < HAL_INGRESS_L3_TABLE2_RULE_NUM; i++)
    {
        table_flow_info_db.ingress_table2_info[i].counter_index = HAL_RULE_INVAILD_COUNTER_ID;
        table_flow_info_db.ingress_table2_info[i].counter_type = ((hal_rule_stats_type_t)0);
        table_flow_info_db.ingress_table2_info[i].meter_index = INVALID_METER_INDEX;
    }

    for(i = 0; i < HAL_INGRESS_FULL_MATCH_TABLE3_RULE_NUM; i++)
    {
        table_flow_info_db.ingress_table3_info[i].counter_index = HAL_RULE_INVAILD_COUNTER_ID;
        table_flow_info_db.ingress_table3_info[i].counter_type = ((hal_rule_stats_type_t)0);
        table_flow_info_db.ingress_table3_info[i].meter_index = INVALID_METER_INDEX;
    }

    for(i = 0; i < HAL_EGRESS_TABLE0_RULE_NUM; i++)
    {
        table_flow_info_db.egress_table0_info[i].counter_index = HAL_RULE_INVAILD_COUNTER_ID;
        table_flow_info_db.egress_table0_info[i].counter_type = ((hal_rule_stats_type_t)0);
        table_flow_info_db.egress_table0_info[i].meter_index = INVALID_METER_INDEX;
    }
#endif

    /*Brandon:
    *               initialization of statistics of each flow entry.
    *               in future, if knowing each couter size of table, need to modify here.
    *               Assume all full match tables share common counter.
    */
    for(i = 0; i < HAL_RULE_COUNTER_SIZE; i++)
    {
        table_flow_stats_db[i].flow_id = INVALID_FLOW_INDEX;

#ifdef  CONFIG_SDK_RTL9310
        table_flow_stats_db[i].table_id = FT_PHASE_END;
#else
        table_flow_stats_db[i].table_id = INVALID_TABLE_INDEX;
#endif

        table_flow_stats_db[i].priority = 0xffff;
        //table_flow_stats_db[i].cnt_mode = OF_FLOW_CNTMODE_END;  /*Brandon: in future, need to change to rtk count mode.*/
        table_flow_stats_db[i].packet_count = ~0;
        table_flow_stats_db[i].byte_count   = ~0;
        table_flow_stats_db[i].counter_flag &= 0;
    }


#ifdef  CONFIG_SDK_RTL9310
    g_group_table_p = (sdn_hal_group_table_t*)malloc(SDN_DB_MAX_TABLE_ID * sizeof(sdn_hal_group_table_t));

    if (g_group_table_p == NULL)
    {
        return RT_ERR_NULL_POINTER;
    }

    memset((void *)g_group_table_p, 0, SDN_DB_MAX_TABLE_ID * sizeof(g_group_table_p));

    for (group_id = 0; group_id < SDN_DB_MAX_TABLE_ID; group_id++)
    {
        g_group_table_p[group_id].action_buket_num = 0;
        g_group_table_p[group_id].action_buket_p = NULL;
    }
#endif

    return SDN_DB_RETURN_OK;
}

/* Table Feature */
int sdn_db_set_table_features(uint8_t n_table,
                                struct ofp_table_features *tf)
{
    int ret = SDN_DB_RETURN_OK;

    if (NULL == tf)
        return RT_ERR_NULL_POINTER;

    memcpy(&table_info[n_table].table_feature, tf, sizeof(struct ofp_table_features));

    return ret;
}

int sdn_db_get_table_features(uint8_t n_table,
                                struct ofp_table_features *tf)
{
    int ret = SDN_DB_RETURN_OK;

    if (NULL == tf)
        return RT_ERR_NULL_POINTER;


    memcpy(tf, &table_info[n_table].table_feature, sizeof(struct ofp_table_features));

    return ret;
}

/* Counter */
int sdn_db_get_flow_stats(uint8_t table_id,
                          uint32_t flow_id,
                          uint64_t *packet_count,
                          uint64_t *byte_count)
{

#if 1
    /*Brandon:
       1. match_num needs to convert to flow id
       2. get counter_id from table_flow_info_db by table_id & flow_id.
       3. get data from DataBase
    */
    int i = 0;

    //if (match == NULL)
    //    return RT_ERR_NULL_POINTER;

    //TODO:get flow id via table_id/priority/match_num/*match
#if 0
    uint32_t counter_id = HAL_RULE_INVAILD_COUNTER_ID;

    switch  (table_id)   /* flow_id:  zero_base?  */
    {
        case    FT_PHASE_IGR_FT_0:
            for ( flow_id = 0 ; flow_id < HAL_INGRESS_FULL_MATCH_TABLE0_RULE_NUM ; flow_id++)
            {
                counter_id = table_flow_info_db.ingress_table0_info[flow_id].counter_index ;
                break;
            }
            break;
        case    FT_PHASE_IGR_FT_1:
            for ( flow_id = 0 ; flow_id < HAL_INGRESS_L2_TABLE1_RULE_NUM ; flow_id++)
            {
                counter_id = table_flow_info_db.ingress_table1_info[flow_id].counter_index ;
                break;
            }
            break;
        case    FT_PHASE_IGR_FT_2:
            for ( flow_id = 0 ; flow_id < HAL_INGRESS_L3_TABLE2_RULE_NUM ; flow_id++)
            {
                counter_id = table_flow_info_db.ingress_table2_info[flow_id].counter_index ;
                break;
            }
            break;
        case    FT_PHASE_IGR_FT_3:
            for ( flow_id = 0 ; flow_id < HAL_INGRESS_FULL_MATCH_TABLE3_RULE_NUM ; flow_id++)
            {
                counter_id = table_flow_info_db.ingress_table3_info[flow_id].counter_index ;
                break;
            }
            break;
        case    FT_PHASE_EGR_FT_0:
            for ( flow_id = 0 ; flow_id < HAL_EGRESS_TABLE0_RULE_NUM ; flow_id++)
            {
                counter_id = table_flow_info_db.egress_table0_info[flow_id].counter_index ;
                break;
            }
            break;
        default:
            return SDN_DB_RETURN_FAILED;
    }

    if ( counter_id == HAL_RULE_INVAILD_COUNTER_ID)
    {
        DBG_SDN("[%s][%d] fail to get counter_id by table_id=[%d] ,flow_id=[%d] \n", __FUNCTION__, __LINE__, table_id, flow_id);
        return SDN_DB_RETURN_FAILED;
    }
#endif


    /*Brandon: does need to get counter_type from match_field to decide what kinds of count(packet/byte) to get ?
                        according to arguments of API, both of count are sent to.
    */
    for (i = 0 ; i < HAL_RULE_COUNTER_SIZE ; i++)
    {
        if(table_flow_stats_db[i].table_id == table_id && table_flow_stats_db[i].flow_id == flow_id)
        {
            if (!(table_flow_stats_db[i].counter_flag & EN_OFPFF_NO_PKT_COUNTS) && !(table_flow_stats_db[i].counter_flag & EN_OFPFF_NO_BYT_COUNTS))
            {
                *packet_count = table_flow_stats_db[i].packet_count;
                *byte_count = table_flow_stats_db[i].byte_count ;
                DBG_SDN("\r\n[%s:%d] counter id(%d) pkt(%lld) byte(%lld) flag(%d)\r\n", __FUNCTION__, __LINE__, i, *packet_count, *byte_count, table_flow_stats_db[i].counter_flag);
                break;
            }
            else if (!(table_flow_stats_db[i].counter_flag & EN_OFPFF_NO_PKT_COUNTS) && (table_flow_stats_db[i].counter_flag & EN_OFPFF_NO_BYT_COUNTS))
            {
                *packet_count = table_flow_stats_db[i].packet_count;
                *byte_count   = ~(0);
                DBG_SDN("\r\n[%s:%d] counter id(%d) pkt(%lld) byte(%lld) flag(%d)\r\n", __FUNCTION__, __LINE__, i, *packet_count, *byte_count, table_flow_stats_db[i].counter_flag);
                //pthread_mutex_unlock(&db_stats_mutex);
                break;
            }
            if ((table_flow_stats_db[i].counter_flag & EN_OFPFF_NO_PKT_COUNTS) && !(table_flow_stats_db[i].counter_flag & EN_OFPFF_NO_BYT_COUNTS))
            {
                *packet_count = ~(0);
                *byte_count   = table_flow_stats_db[i].byte_count ;
                DBG_SDN("\r\n[%s:%d] counter id(%d) pkt(%lld) byte(%lld) flag(%d)\r\n", __FUNCTION__, __LINE__, i, *packet_count, *byte_count, table_flow_stats_db[i].counter_flag);
                break;
            }
            else
            {
                *packet_count = ~(0);
                *byte_count   = ~(0);
                DBG_SDN("\r\n[%s:%d]  counter id (%d) pkt(%lld) byte(%lld) flag(%d)\r\n", __FUNCTION__, __LINE__, i, *packet_count, *byte_count, table_flow_stats_db[i].counter_flag);
                break;
            }

        }
    }

    if (i == HAL_RULE_COUNTER_SIZE)
    {
        *packet_count = ~(0);
        *byte_count   = ~(0);
        DBG_SDN("\r\n[%s:%d] COUNTER NOT FOUND \r\n", __FUNCTION__, __LINE__);
        /*this table_id, flow_id doesn't bind a counter.*/
        return SDN_DB_RETURN_FAILED;
    }


#endif

    return SDN_DB_RETURN_OK;
}


int sdn_db_apply_flow_entry_counter(uint8_t table_id,
                                    uint32_t flow_id,
                                    uint32_t count_id,
                                    uint32_t cnt_flag)
{

    /*Brandon :
                       Must consider the flow apply new counter_id or change another counter_id.
    */

    SDN_DB_LOCK(&db_stats_mutex);

    table_flow_stats_db[count_id].table_id = table_id;
    table_flow_stats_db[count_id].flow_id = flow_id;
    table_flow_stats_db[count_id].counter_flag |= cnt_flag;
    SDN_DB_UNLOCK(&db_stats_mutex);

    return SDN_DB_RETURN_OK;
}


int sdn_db_del_flow_entry_counter(uint8_t table_id,
                                  uint32_t flow_id,
                                  uint32_t count_id)
{
#if 1
    /*Brandon*/
    if ( table_flow_stats_db[count_id].table_id == table_id && table_flow_stats_db[count_id].flow_id == flow_id)
    {
        SDN_DB_LOCK(&db_stats_mutex);
        table_flow_stats_db[count_id].flow_id = INVALID_FLOW_INDEX;

    #ifdef  CONFIG_SDK_RTL9310
        table_flow_stats_db[count_id].table_id = FT_PHASE_END;
    #else
        table_flow_stats_db[count_id].table_id = INVALID_TABLE_INDEX;
    #endif

        table_flow_stats_db[count_id].priority = 0Xffff;
        //table_flow_stats_db[count_id].cnt_mode = OF_FLOW_CNTMODE_END;  /*Brandon: in future, need to change to rtk count mode.*/
        table_flow_stats_db[count_id].packet_count = ~0;
        table_flow_stats_db[count_id].byte_count   = ~0;
        table_flow_stats_db[count_id].counter_flag &= 0;
        SDN_DB_UNLOCK(&db_stats_mutex);
        return SDN_DB_RETURN_OK;
    }
#endif

    return SDN_DB_RETURN_FAILED;
}


int sdn_db_clear_flow_entry_counter(uint8_t table_id,
                                    uint32_t flow_id,
                                    uint32_t count_id)
{
#if 1
    /*Brandon*/
    if ( table_flow_stats_db[count_id].table_id == table_id && table_flow_stats_db[count_id].flow_id == flow_id)
    {
        SDN_DB_LOCK(&db_stats_mutex);
        table_flow_stats_db[count_id].packet_count = ~0;
        table_flow_stats_db[count_id].byte_count   = ~0;
        SDN_DB_UNLOCK(&db_stats_mutex);
        return SDN_DB_RETURN_OK;
    }
#endif

    return SDN_DB_RETURN_FAILED;
}

sdn_db_flow_table_t * sdn_db_get_flow_table(uint8_t table_id)
{
    int i;

    SDN_DB_LOCK(&db_table_mutex);
    for (i = 0; i < SDN_DB_SWITCH_TABLE_NUM_MAX; i++)
    {
        if (table_info[i].table_feature.table_id == table_id)
        {
            SDN_DB_UNLOCK(&db_table_mutex);
            return &table_info[i];
        }
    }
    SDN_DB_UNLOCK(&db_table_mutex);

    return NULL;
}

int sdn_db_get_flow_entry_usedtime(uint8_t table_id, uint32_t flow_id, uint32_t *used_time_p)
{
    if (used_time_p == NULL)
    {
        return SDN_DB_RETURN_FAILED;
    }

    if (table_info[table_id].flow_entry == NULL)
    {
        return SDN_DB_RETURN_FAILED;
    }

    SDN_DB_LOCK(&db_table_mutex);
    *used_time_p = table_info[table_id].flow_entry[flow_id].idle_timeout.used_time;
    SDN_DB_UNLOCK(&db_table_mutex);

    return SDN_DB_RETURN_OK;
}

int sdn_db_update_flow_entry_idletimeout(uint8_t table_id, uint32_t flow_id, uint32_t idle_time)
{
    if (table_info[table_id].flow_entry == NULL)
    {
        return SDN_DB_RETURN_FAILED;
    }

    SDN_DB_LOCK(&db_table_mutex);
    table_info[table_id].flow_entry[flow_id].idle_timeout.used_time = idle_time;
    SDN_DB_UNLOCK(&db_table_mutex);

    return SDN_DB_RETURN_OK;
}

/* If this function fails, caller should call opal_flow_free() to release memory allocated by this function */
int sdn_db_flow_copy(sdn_db_flow_entry_t *dst, const sdn_db_flow_entry_t *src)
{
    uint32    len_match = 0;
    uint32    len_inst = 0;
    uint32    i;

    if(!src || !dst)
    {
        return -1;
    }

    /* First copy the whole structure */
    memcpy(dst, src, sizeof(sdn_db_flow_entry_t));

    /* Assign new space for match fields */
    len_match = src->len_match;
    if(len_match == 0)
    {
        /* To avoid that src->match_field is really a valid pointer */
        dst->match_field_p = NULL;
    }
    else
    {
        dst->match_field_p = (sdn_db_match_field_t*)malloc(len_match * sizeof(sdn_db_match_field_t));
        if(!dst->match_field_p)
        {
            /* Space not enough */
            return -1;
        }
        memcpy(dst->match_field_p, src->match_field_p, len_match * sizeof(sdn_db_match_field_t));
    }

    /* Assign new space for instructions */
    len_inst = src->len_inst;
    if(len_inst == 0)
    {
        /* To avoid that src->instruction is really a valid pointer */
        dst->instruction_p = NULL;
    }
    else
    {
        dst->instruction_p = (sdn_db_instruction_t*)malloc(len_inst * sizeof(sdn_db_instruction_t));
        if(!dst->instruction_p)
        {
            /* Space not enough */
            return -1;
        }
        memcpy(dst->instruction_p, src->instruction_p, len_inst * sizeof(sdn_db_instruction_t));
    }

    /* Assign new space for instruction data */
    for(i = 0; i < len_inst; i++)
    {
        switch(src->instruction_p[i].type)
        {
            case EN_OFPIT_GOTO_TABLE:
            {
                if(!src->instruction_p[i].data_p)
                {
                    return SDN_DB_RETURN_FAILED;
                }
                dst->instruction_p[i].data_p = malloc(sizeof(sdn_db_inst_goto_table_t));
                if(!dst->instruction_p[i].data_p)
                {
                    return SDN_DB_RETURN_FAILED;
                }
                memcpy(dst->instruction_p[i].data_p, src->instruction_p[i].data_p, sizeof(sdn_db_inst_goto_table_t));
                break;
            }
            case EN_OFPIT_WRITE_METADATA:
            {
                if(!src->instruction_p[i].data_p)
                {
                    return SDN_DB_RETURN_FAILED;
                }
                dst->instruction_p[i].data_p = malloc(sizeof(sdn_db_inst_write_metadata_t));
                if(!dst->instruction_p[i].data_p)
                {
                    return SDN_DB_RETURN_FAILED;
                }
                memcpy(dst->instruction_p[i].data_p, src->instruction_p[i].data_p, sizeof(sdn_db_inst_write_metadata_t));
                break;
            }
            case EN_OFPIT_APPLY_ACTIONS:
            {
                uint32 len_action;
                sdn_db_action_t *action_src, *action_dst;
                sdn_db_inst_apply_actions_t *data_apply_src, *data_apply_dst;

                if(!src->instruction_p[i].data_p)
                {
                    return SDN_DB_RETURN_FAILED;
                }
                dst->instruction_p[i].data_p = malloc(sizeof(sdn_db_inst_write_actions_t));
                if(!dst->instruction_p[i].data_p)
                {
                    return SDN_DB_RETURN_FAILED;
                }
                memcpy(dst->instruction_p[i].data_p, src->instruction_p[i].data_p, sizeof(sdn_db_inst_apply_actions_t));

                /* Assign new space for actions */
                data_apply_src = (sdn_db_inst_apply_actions_t*)src->instruction_p[i].data_p;
                data_apply_dst = (sdn_db_inst_apply_actions_t*)dst->instruction_p[i].data_p;
                len_action = data_apply_src->len;
                action_src = data_apply_src->action_set;
                if(len_action == 0)
                {
                    data_apply_dst->action_set = NULL;
                    break;
                }
                if(!action_src)
                {
                    return SDN_DB_RETURN_FAILED;
                }

                data_apply_dst->action_set = (sdn_db_action_t*)malloc(len_action * sizeof(sdn_db_action_t));
                action_dst = data_apply_dst->action_set;
                if(!action_dst)
                {
                    return SDN_DB_RETURN_FAILED;
                }
                memcpy(action_dst, action_src, len_action * sizeof(sdn_db_action_t));
                break;
            }
            case EN_OFPIT_WRITE_ACTIONS:
            {
                uint32 len_action;
                sdn_db_action_t *action_src, *action_dst;
                sdn_db_inst_write_actions_t *data_write_src, *data_write_dst;

                if(!src->instruction_p[i].data_p)
                {
                    return SDN_DB_RETURN_FAILED;
                }
                dst->instruction_p[i].data_p = malloc(sizeof(sdn_db_inst_write_actions_t));
                if(!dst->instruction_p[i].data_p)
                {
                    return SDN_DB_RETURN_FAILED;
                }
                memcpy(dst->instruction_p[i].data_p, src->instruction_p[i].data_p, sizeof(sdn_db_inst_write_actions_t));

                /* Assign new space for actions */
                data_write_src = (sdn_db_inst_write_actions_t*)src->instruction_p[i].data_p;
                data_write_dst = (sdn_db_inst_write_actions_t*)dst->instruction_p[i].data_p;
                len_action = data_write_src->len;
                action_src = data_write_src->action_set;
                if(len_action == 0)
                {
                    data_write_dst->action_set = NULL;
                    break;
                }
                if(!action_src)
                {
                    return SDN_DB_RETURN_FAILED;
                }

                data_write_dst->action_set = (sdn_db_action_t*)malloc(len_action * sizeof(sdn_db_action_t));
                action_dst = data_write_dst->action_set;
                if(!action_dst)
                {
                    return SDN_DB_RETURN_FAILED;
                }
                memcpy(action_dst, action_src, len_action * sizeof(sdn_db_action_t));
                break;
            }
            case EN_OFPIT_CLEAR_ACTIONS:
            {
                if(!src->instruction_p[i].data_p)
                {
                    return SDN_DB_RETURN_FAILED;
                }
                dst->instruction_p[i].data_p = malloc(sizeof(sdn_db_inst_clear_actions_t));
                if(!dst->instruction_p[i].data_p)
                {
                    return SDN_DB_RETURN_FAILED;
                }
                memcpy(dst->instruction_p[i].data_p, src->instruction_p[i].data_p, sizeof(sdn_db_inst_clear_actions_t));
                break;

            }
            case EN_OFPIT_METER:
            {
                if(!src->instruction_p[i].data_p)
                {
                    return SDN_DB_RETURN_FAILED;
                }
                dst->instruction_p[i].data_p = malloc(sizeof(sdn_db_inst_meter_t));
                if(!dst->instruction_p[i].data_p)
                {
                    return SDN_DB_RETURN_FAILED;
                }
                memcpy(dst->instruction_p[i].data_p, src->instruction_p[i].data_p, sizeof(sdn_db_inst_meter_t));
                break;
            }
            default:
            {
                return SDN_DB_RETURN_FAILED;
            }
        }
    }

    return SDN_DB_RETURN_OK;
}

int sdn_db_get_flow_table_stats(uint8_t table_id, rtk_of_flowTblCntType_t type, uint32 *pCnt)
{

#if 1
    /*Brandon*/

    if (pCnt == NULL)
        return SDN_DB_RETURN_FAILED;

    switch (type)
    {
#if 0 /*not support*/
        case OF_FLOW_TBL_CNT_TYPE_ACTIVE:
            *pCnt = table_stats_db[table_id].active_count;
            break;
#endif

        case OF_FLOW_TBL_CNT_TYPE_LOOKUP:
            *pCnt = (uint32)table_stats_db[table_id].lookup_count;
            DBG_SDN("[%s:%d] table(%d) type(%s) counter(%lld) \n", __FUNCTION__, __LINE__, table_id, (type == OF_FLOW_TBL_CNT_TYPE_LOOKUP ? "look_up" : "match"), table_stats_db[table_id].lookup_count);
            break;
        case OF_FLOW_TBL_CNT_TYPE_MATCH:
            *pCnt = (uint32)table_stats_db[table_id].matched_count;
            DBG_SDN("[%s:%d] table(%d) type(%s) counter(%lld) \n", __FUNCTION__, __LINE__, table_id, (type == OF_FLOW_TBL_CNT_TYPE_LOOKUP ? "look_up" : "match"), table_stats_db[table_id].matched_count);
            break;
        default:
            *pCnt = 0;
            break;
    }

    return SDN_DB_RETURN_OK;

#endif

}


#if 1   //Brandon add:

int sdn_db_get_availible_counter_id(uint8_t table_id, uint32_t flow_id, uint32_t *pCounter_id)
{
    int counter_index = 0;
    int ret = SDN_DB_RETURN_FAILED;
    /*
    Brandon:
        should pay attention to get a new counter_id, or
        this (table, flow) had a counter_id but just wanna change counter mode. if this, only return the same counter id.
    */
    if (pCounter_id == NULL)
        return RT_ERR_NULL_POINTER;

    for (counter_index = 0 ; counter_index < HAL_RULE_COUNTER_SIZE ; counter_index++)
    {
        if(table_flow_stats_db[counter_index].table_id == table_id && table_flow_stats_db[counter_index].flow_id == flow_id)
        {
            /*this (table,flow) already had a counter_id.*/
            *pCounter_id = counter_index;
            return SDN_DB_RETURN_OK;
        }
    }

    /*get a new counter_id*/
    SDN_DB_LOCK(&db_stats_mutex);
    for (counter_index = 0 ; counter_index < HAL_RULE_COUNTER_SIZE ; counter_index++)
    {
        if(table_flow_stats_db[counter_index].flow_id == INVALID_FLOW_INDEX)
        {
            *pCounter_id = counter_index;
            SDN_DB_UNLOCK(&db_stats_mutex);
            return SDN_DB_RETURN_OK;
        }
    }
    SDN_DB_UNLOCK(&db_stats_mutex);

    return ret;
}

int sdn_db_get_counter_id_by_table_flow(uint8_t table_id,
                                        uint32_t flow_id,
                                        uint32_t *pCount_id)
{
    int i = 0;
    int ret = SDN_DB_RETURN_FAILED;

    if (pCount_id == NULL)
        return RT_ERR_NULL_POINTER;

    for (i = 0 ; i < HAL_RULE_COUNTER_SIZE ; i++)
    {
        if(table_flow_stats_db[i].table_id == table_id  && table_flow_stats_db[i].flow_id == flow_id)
        {
            *pCount_id = i;
            return SDN_DB_RETURN_OK;
        }
    }

    return ret;

}

int sdn_db_get_flow_by_counter_id(uint8_t table_id, uint32_t counter_id , uint16_t *pflow_id)
{
    //int i = 0;
    int ret = SDN_DB_RETURN_FAILED;

    if (pflow_id == NULL)
        return RT_ERR_NULL_POINTER;
    //i = 0;
#if 0
    switch  (table_id)   /* flow_id:  zero_base?  */
    {
        case    FT_PHASE_IGR_FT_0:
            for ( i = 0 ; i < HAL_INGRESS_FULL_MATCH_TABLE0_RULE_NUM ; i++)
            {
                if (table_flow_info_db.ingress_table0_info[i].counter_index == counter_id)
                {
                    *pflow_id = i;
                    return SDN_DB_RETURN_OK;
                }
            }
            break;
        case    FT_PHASE_IGR_FT_1:
            for ( i = 0 ; i < HAL_INGRESS_L2_TABLE1_RULE_NUM ; i++)
            {
                if (table_flow_info_db.ingress_table1_info[i].counter_index == counter_id)
                {
                    *pflow_id = i;
                    return SDN_DB_RETURN_OK;
                }
            }
            break;
        case    FT_PHASE_IGR_FT_2:
            for ( i = 0 ; i < HAL_INGRESS_L3_TABLE2_RULE_NUM ; i++)
            {
                if (table_flow_info_db.ingress_table2_info[i].counter_index == counter_id)
                {
                    *pflow_id = i;
                    return SDN_DB_RETURN_OK;
                }
            }
            break;
        case    FT_PHASE_IGR_FT_3:
            for ( i = 0 ; i < HAL_INGRESS_FULL_MATCH_TABLE3_RULE_NUM ; i++)
            {
                if (table_flow_info_db.ingress_table3_info[i].counter_index == counter_id)
                {
                    *pflow_id = i;
                    return SDN_DB_RETURN_OK;
                }
            }
            break;
        case    FT_PHASE_EGR_FT_0:
            for ( i = 0 ; i < HAL_EGRESS_TABLE0_RULE_NUM ; i++)
            {
                if (table_flow_info_db.egress_table0_info[i].counter_index == counter_id)
                {
                    *pflow_id = i;
                    return SDN_DB_RETURN_OK;
                }
            }
            break;
        default:
            return SDN_DB_RETURN_FAILED;
    }
#endif


    if (table_flow_stats_db[counter_id].table_id == table_id)
    {
        *pflow_id = table_flow_stats_db[counter_id].flow_id;
        return SDN_DB_RETURN_OK;
    }

    return ret;

}


int sdn_db_get_port_count()
{
    return HAL_RULE_PORT_NUM;
}

int sdn_db_get_table_count()
{
    return HAL_TABLE_NUM;
}

int sdn_db_get_flow_count(uint8_t table_id)
{
    /* get flow count of table from insert table database. */
    return SDN_DB_RETURN_OK;
}

int sdn_db_set_port_stats(uint32_t port_no, hal_port_stats_t *pPort_stats_data)
{
    int idx;

    if (pPort_stats_data == NULL)
        return RT_ERR_NULL_POINTER;
    for (idx = 0; idx < sdn_db_max_port; idx++)
    {
        if (port_no == SDN_DB_PORT_NUMBER(idx))
        {
            SDN_DB_LOCK(&db_stats_mutex);
            port_info[port_no].statistic.rx_packets = pPort_stats_data->rx_packets;
            port_info[port_no].statistic.tx_packets = pPort_stats_data->tx_packets;
            port_info[port_no].statistic.rx_bytes = pPort_stats_data->rx_bytes;
            port_info[port_no].statistic.tx_bytes = pPort_stats_data->tx_bytes;
            port_info[port_no].statistic.rx_drops = pPort_stats_data->rx_drops;
            port_info[port_no].statistic.tx_drops = pPort_stats_data->tx_drops;
            port_info[port_no].statistic.rx_errors = pPort_stats_data->rx_errors;
            port_info[port_no].statistic.tx_errors = pPort_stats_data->tx_errors;
            port_info[port_no].statistic.rx_frame_align_err = pPort_stats_data->rx_frame_align_err;
            port_info[port_no].statistic.rx_overrun_errors = pPort_stats_data->rx_overrun_errors;
            port_info[port_no].statistic.rx_crc_errors = pPort_stats_data->rx_crc_errors;
            port_info[port_no].statistic.collisions = pPort_stats_data->collisions;
            SDN_DB_UNLOCK(&db_stats_mutex);

            return SDN_DB_RETURN_OK;
        }
    }

    return SDN_DB_RETURN_FAILED;
}


int sdn_db_get_port_stats(uint32_t port_no, hal_port_stats_t *pPort_stats_data)
{
    int idx;

    if (pPort_stats_data == NULL)
        return RT_ERR_NULL_POINTER;

    for (idx = 0; idx < sdn_db_max_port; idx++)
    {
        if (port_no == SDN_DB_PORT_NUMBER(idx))
        {
            pPort_stats_data->rx_packets = port_info[port_no].statistic.rx_packets;
            pPort_stats_data->tx_packets = port_info[port_no].statistic.tx_packets;
            pPort_stats_data->rx_bytes = port_info[port_no].statistic.rx_bytes;
            pPort_stats_data->tx_bytes = port_info[port_no].statistic.tx_bytes;
            pPort_stats_data->rx_drops = port_info[port_no].statistic.rx_drops;
            pPort_stats_data->tx_drops = port_info[port_no].statistic.tx_drops;
            pPort_stats_data->rx_errors = port_info[port_no].statistic.rx_errors;
            pPort_stats_data->tx_errors = port_info[port_no].statistic.tx_errors;
            pPort_stats_data->rx_frame_align_err = port_info[port_no].statistic.rx_frame_align_err;
            pPort_stats_data->rx_overrun_errors = port_info[port_no].statistic.rx_overrun_errors;
            pPort_stats_data->rx_crc_errors = port_info[port_no].statistic.rx_crc_errors;
            pPort_stats_data->collisions = port_info[port_no].statistic.collisions;

            return SDN_DB_RETURN_OK;
        }
    }

    return SDN_DB_RETURN_FAILED;
}

int sdn_db_set_flow_stats(uint8_t table_id,
                          uint32_t flow_id,
                          uint32_t counter_id,
                          uint64_t packet_count,
                          uint64_t byte_count)
{
    //int i = 0;
    int ret = SDN_DB_RETURN_FAILED;

    if (table_id >= FT_PHASE_END)
        return ret;
    //i = 0;
#if 0
    switch  (table_id)   /* flow_id:  zero_base?  */
    {
        case    FT_PHASE_IGR_FT_0:
            if (flow_id > HAL_INGRESS_FULL_MATCH_TABLE0_RULE_NUM)
                return SDN_DB_RETURN_FAILED;
            counter_id = table_flow_info_db.ingress_table0_info[flow_id].counter_index;
            break;
        case    FT_PHASE_IGR_FT_1:
            if (flow_id > HAL_INGRESS_L2_TABLE1_RULE_NUM)
                return SDN_DB_RETURN_FAILED;
            counter_id = table_flow_info_db.ingress_table1_info[flow_id].counter_index;
            break;
        case    FT_PHASE_IGR_FT_2:
            if (flow_id > HAL_INGRESS_L3_TABLE2_RULE_NUM)
                return SDN_DB_RETURN_FAILED;
            counter_id = table_flow_info_db.ingress_table2_info[flow_id].counter_index;
            break;
        case    FT_PHASE_IGR_FT_3:
            if (flow_id > HAL_INGRESS_FULL_MATCH_TABLE3_RULE_NUM)
                return SDN_DB_RETURN_FAILED;
            counter_id = table_flow_info_db.ingress_table3_info[flow_id].counter_index;
            break;
        case    FT_PHASE_EGR_FT_0:
            if (flow_id > HAL_EGRESS_TABLE0_RULE_NUM)
                return SDN_DB_RETURN_FAILED;
            counter_id = table_flow_info_db.egress_table0_info[flow_id].counter_index;
            break;
        default:
            return SDN_DB_RETURN_FAILED;
    }
#endif

    /*get counter_id from table_flow_stats_db */

    SDN_DB_LOCK(&db_stats_mutex);
    table_flow_stats_db[counter_id].table_id     = table_id;
    table_flow_stats_db[counter_id].flow_id      = flow_id;
    table_flow_stats_db[counter_id].packet_count = ( (packet_count >= UINT32MAX_VAL) ? 0 : packet_count);
    table_flow_stats_db[counter_id].byte_count   = ( (byte_count >= UINT32MAX_VAL) ? 0 : byte_count);
    //table_flow_stats_db[counter_id].counter_flag &= 0;
    SDN_DB_UNLOCK(&db_stats_mutex);


    return SDN_DB_RETURN_OK;
}

int sdn_db_set_flow_table_stats(uint8_t table_id, rtk_of_flowTblCntType_t type, uint64_t table_counter)
{
    int ret = SDN_DB_RETURN_FAILED;

    //if (pCnt == NULL)
    //    return RT_ERR_NULL_POINTER;

#ifdef  CONFIG_SDK_RTL9310
    if (table_id >= FT_PHASE_END)
        return ret;
#else
    if (table_id >= HAL_TABLE_NUM)
        return ret;
#endif

    //DBG_SDN("[%s:%d] table(%d) type(%s) setv_value(%lld)\n", __FUNCTION__, __LINE__, table_id, (type == OF_FLOW_TBL_CNT_TYPE_LOOKUP ? "look_up" : "match"), table_counter);
    SDN_DB_LOCK(&db_stats_mutex);
    switch (type)
    {
#if 0
        case OF_FLOW_TABLE_CNT_TYPE_ACTIVE:
            table_stats_db[table_id].active_count = *pCnt;
            break;
#endif
        case OF_FLOW_TBL_CNT_TYPE_LOOKUP:
            table_stats_db[table_id].lookup_count += table_counter;
            table_stats_db[table_id].lookup_count = ( (table_stats_db[table_id].lookup_count >= UINT32MAX_VAL) ? 0 : table_stats_db[table_id].lookup_count);
            //DBG_SDN("[%s:%d] table(%d) type(%s) counter(%lld) setv_value(%lld)\n", __FUNCTION__, __LINE__, table_id, (type == OF_FLOW_TBL_CNT_TYPE_LOOKUP ? "look_up" : "match"), table_stats_db[table_id].lookup_count, table_counter);
            break;
        case OF_FLOW_TBL_CNT_TYPE_MATCH:
            table_stats_db[table_id].matched_count += table_counter;
            table_stats_db[table_id].matched_count = ( (table_stats_db[table_id].matched_count >= UINT32MAX_VAL) ? 0 : table_stats_db[table_id].matched_count);
            //DBG_SDN("[%s:%d] table(%d) type(%s) counter(%lld) set-value(%lld) \n", __FUNCTION__, __LINE__, table_id, (type == OF_FLOW_TBL_CNT_TYPE_LOOKUP ? "look_up" : "match"), table_stats_db[table_id].matched_count,table_counter);
            break;
        default:
            break;
    }
    SDN_DB_UNLOCK(&db_stats_mutex);

    return SDN_DB_RETURN_OK;
}


#endif
int sdn_db_destroy(void)
{
    uint32_t entry_id  = 0;
    uint32_t bucket_id = 0;
#ifdef  CONFIG_SDK_RTL9310
    if (g_group_table_p)
    {
        free(g_group_table_p);
        g_group_table_p = NULL;
    }
//new group code
    if (g_group_table_db_p)
    {
        if (g_group_table_db_p->group_entry_p)
        {
            for(entry_id = 0; entry_id < SDN_HAL_MAX_GROUP_ENTRY_NUM; entry_id++)
            {
                if (g_group_table_db_p->group_entry_p[entry_id].action_bucket_p)
                {
                    for (bucket_id = 0; bucket_id < g_group_table_db_p->group_entry_p[entry_id].bucket_len; bucket_id++)
                    {
                        if (g_group_table_db_p->group_entry_p[entry_id].action_bucket_p[bucket_id].actions_p)
                        {
                            free(g_group_table_db_p->group_entry_p[entry_id].action_bucket_p[bucket_id].actions_p);
                            g_group_table_db_p->group_entry_p[entry_id].action_bucket_p[bucket_id].actions_p = NULL;
                            g_group_table_db_p->group_entry_p[entry_id].action_bucket_p[bucket_id].action_len = 0;
                        }
                    }
                    free(g_group_table_db_p->group_entry_p[entry_id].action_bucket_p);
                    g_group_table_db_p->group_entry_p[entry_id].action_bucket_p = NULL;
                    g_group_table_db_p->group_entry_p[entry_id].bucket_len      = 0;
                    g_group_table_db_p->group_entry_p[entry_id].counters        = 0;
                    g_group_table_db_p->group_entry_p[entry_id].group_type      = 0;
                }
            }
            free(g_group_table_db_p->group_entry_p);
            g_group_table_db_p->group_entry_p = NULL;
            g_group_table_db_p->total_counter = 0;

        }
        free(g_group_table_db_p);
        g_group_table_db_p = NULL;
    }
//new group code
#endif
    if (port_info)
    {
        free(port_info);
        port_info = NULL;
    }
    if (switch_info)
    {
        free(switch_info);
        switch_info = NULL;
    }
    if (table_info)
    {
        free(table_info);
        table_info = NULL;
    }
    if (meter_table_info)
    {
        free(meter_table_info);
        meter_table_info = NULL;
    }

    pthread_mutex_destroy(&db_stats_mutex);
    pthread_mutex_destroy(&db_port_mutex);
    pthread_mutex_destroy(&db_table_mutex);

    sdn_db_destroy_qos_table();

    return SDN_DB_RETURN_OK;
}

int sdn_db_init_switch(void)
{
    uint32_t    error_code = 0;

    switch_info = (sdn_db_switch_features_t *)malloc(sizeof(sdn_db_switch_features_t));

    if(switch_info == NULL)        /* Space not enough */
    {
        return SDN_DB_RETURN_FAILED;
    }

    memset(switch_info, 0, sizeof(sdn_db_switch_features_t));
    if (SDN_HAL_RETURN_OK != (error_code = sdn_hal_switch_features_get(switch_info)))
    {
        return SDN_DB_RETURN_FAILED;
    }
    return SDN_DB_RETURN_OK;

}

int sdn_db_init_port(void)
{
    uint32_t    i   = 0;
    uint32_t    ret = SDN_DB_RETURN_OK;
    uint32_t    error_code = 0;
    sdn_db_port_feature_t feature;

    memset(&feature, 0, sizeof(sdn_db_port_feature_t));

    pthread_mutex_init(&db_port_mutex, NULL);

    ret = sdn_hal_port_init();
    if(ret != SDN_HAL_RETURN_OK)
    {
        DBG_SDN("[sdn_db] init port failed.\n");
        return 1;
    }
    /* Get from HAL */
    sdn_db_max_port = sdn_hal_port_max_get();

    if(sdn_db_max_port == 0)
    {
        DBG_SDN("[sdn_db] init port but sdn_db_max_port == 0\n");
        return 1;
    }

    port_info = (sdn_db_port_t *)malloc((sdn_db_max_port + 1)* sizeof(sdn_db_port_t));
    if(port_info == NULL)        /* Space not enough */
    {
        return SDN_DB_RETURN_FAILED;
    }

    memset(port_info, 0, (sdn_db_max_port + 1) * sizeof(sdn_db_port_t));

    sprintf((char *)(port_info[0].name), "%s", "br0");
    port_info[0].port_no = 0xfffe;
    port_info[0].exist = 1;

    /* initial port informamtion form hal layer.*/
    for(i = 1; i < sdn_db_max_port; i++)
    {
        if(sdn_hal_port_exist(i))
        {
            port_info[i].port_no = i;
            port_info[i].ofp_exist = 0;
            port_info[i].exist = 1;
            sprintf((char *)port_info[i].name, "eth%d", i);
            error_code = sdn_hal_eth_addr_get(i, port_info[i].hw_addr);
            error_code = sdn_hal_port_feature_get(i, &port_info[i].feature);
            error_code = sdn_hal_port_state_get(i, &port_info[i].state);

            /*initialization of port statistic */
            memset(&port_info[i].statistic, 0, sizeof(sdn_db_port_stats_t));
            memset(&port_info[i].config, 0, sizeof(sdn_db_port_config_t));
            //memcpy(&port_info[i].feature, &feature, sizeof(sdn_db_port_feature_t));
            //DBG_SDN("curr%d supported%d peer%d advertised%d \n", port_info[i].feature.curr, port_info[i].feature.supported, port_info[i].feature.peer, port_info[i].feature.advertised);
        }
        else
        {
            port_info[i].port_no = 0xFFFF;
        }
    }
//#endif
    if (error_code)
    {
        //do nothing;
    }
    return SDN_DB_RETURN_OK;
}
int sdn_db_init_flow_table(void)
{
    /* Call this function after opal_init_switch_features() */
    uint8_t     n_tables = 0;
    uint32_t    max_entries = 0;
    uint32_t    i = 0;
    uint32_t    error_code = SDN_DB_RETURN_OK;
    sdn_hal_flow_table_feature_t table_features;

    pthread_mutex_init(&db_table_mutex, NULL);

#if 1   /*todo:need to get table number from switch capabilities.*/
    if (SDN_DB_SWITCH)
    {
        n_tables = SDN_DB_SWITCH_TABLE_NUM_MAX;
    }
    //n_tables = switch_info->n_tables;
#else
    n_tables = HAL_TABLE_NUM;
#endif
    if(n_tables == 0)
    {
        DBG_SDN("[SDN_DB] init flow table but n_tables == 0\n");
        return SDN_DB_RETURN_FAILED;
    }
    table_info = (sdn_db_flow_table_t*)malloc(n_tables * sizeof(sdn_db_flow_table_t));
    if(!table_info)
    {
        /* Space not enough */
        return SDN_DB_RETURN_FAILED;
    }
    memset(table_info, 0, n_tables * sizeof(sdn_db_flow_table_t));

    for(i = 0; i < n_tables; i++)
    {
        /* Get table features from SDN HAL */
        error_code = sdn_hal_table_features_get(i, &table_features);
        if(error_code != SDN_HAL_RETURN_OK)
        {
            DBG_SDN("[sdn db] table feature of table %d can't be got from SDN HAL\n", i);
            return error_code;
        }
        max_entries = table_features.max_entry_nbr;

        table_info[i].flow_entry = (sdn_db_flow_entry_t*)malloc(max_entries * sizeof(sdn_db_flow_entry_t));
        if(!table_info[i].flow_entry)
        {
            /* Space not enough */
            return SDN_DB_RETURN_MALLOC_FAIL;
        }
        memset(table_info[i].flow_entry, 0, max_entries * sizeof(sdn_db_flow_entry_t));
        table_info[i].flow_count = 0;
        table_info[i].table_feature.table_id = i;
        table_info[i].table_feature.match_bitmap         = table_features.match_fields_bit_map     ;
        table_info[i].table_feature.wildcards_bitmap     = table_features.wildcard_fields_bit_map  ;
        table_info[i].table_feature.apply_actions_bitmap = table_features.action_bit_map           ;
        table_info[i].table_feature.apply_setfield_bitmap= table_features.set_table_fields_bit_map ;
        table_info[i].table_feature.inst_bitmap          = table_features.instruction_bit_map      ;
        table_info[i].table_feature.max_entries          = table_features.max_entry_nbr            ;
    }

    return error_code;
}


int sdn_db_port_state_update(uint32_t ofp_port)
{
    if (port_info[ofp_port].ofp_exist)
    {
        SDN_DB_LOCK(&db_port_mutex);
        sdn_hal_port_feature_get(ofp_port, &port_info[ofp_port].feature);
        sdn_hal_port_state_get(ofp_port, &port_info[ofp_port].state);
        SDN_DB_UNLOCK(&db_port_mutex);
    }
    return SDN_DB_RETURN_OK;
}

int sdn_db_port_description_get(uint32_t ofp_port, sdn_db_port_t *port_desc)
{
    uint32 idx;

    for (idx = 0; idx < sdn_db_max_port; idx++)
    {
        if (ofp_port == SDN_DB_PORT_NUMBER(idx))
        {
            SDN_DB_LOCK(&db_port_mutex);
            memcpy(port_desc, &SDN_DB_PORT(idx), sizeof(sdn_db_port_t));
            SDN_DB_UNLOCK(&db_port_mutex);
            return SDN_DB_RETURN_OK;
        }
    }

    return SDN_DB_RETURN_FAILED;
}

int sdn_db_port_max_get(void)
{
    //DBG_SDN("[%s:%d] total port(%d) \n", __FUNCTION__, __LINE__,  sdn_db_max_port);
    return sdn_db_max_port;
}

int sdn_db_ofpport_getByName(const char *name, uint32_t *ofp_port)
{
    int idx;

    for (idx= 0; idx < sdn_db_max_port; idx++)
    {
        SDN_DB_LOCK(&db_port_mutex);

        if ((0 == strcmp((const char *)SDN_DB_PORT_NAME(idx), name)) && SDN_DB_PORT_EXIST(idx))
        {
            *ofp_port = SDN_DB_PORT_NUMBER(idx);
            SDN_DB_UNLOCK(&db_port_mutex);

            return SDN_DB_RETURN_OK;
        }
        SDN_DB_UNLOCK(&db_port_mutex);
    }
    return SDN_DB_RETURN_FAILED;

}

int sdn_db_port_add(uint32_t ofp_port)
{
    int idx;

    for (idx= 0; idx < sdn_db_max_port; idx++)
    {
        SDN_DB_LOCK(&db_port_mutex);
        if (ofp_port == SDN_DB_PORT_NUMBER(idx))
        {
            SDN_DB_OF_PORT_EXIST(ofp_port) = true;
            SDN_DB_UNLOCK(&db_port_mutex);
            return SDN_DB_RETURN_OK;
        }
        SDN_DB_UNLOCK(&db_port_mutex);
    }

    return SDN_DB_RETURN_FAILED;
}
int sdn_db_port_del(uint32_t ofp_port)
{
    int idx;

    for (idx= 0; idx < sdn_db_max_port; idx++)
    {
        SDN_DB_LOCK(&db_port_mutex);
        if (ofp_port == SDN_DB_PORT_NUMBER(idx))
        {
            SDN_DB_OF_PORT_EXIST(ofp_port) = false;
            SDN_DB_UNLOCK(&db_port_mutex);
            return SDN_DB_RETURN_OK;
        }
        SDN_DB_UNLOCK(&db_port_mutex);
    }

    return SDN_DB_RETURN_OK;
}

int sdn_db_meter_entry_add(const sdn_db_meter_mod_t *meter)
{
    uint32_t meter_id = 0;
    meter_id = meter->meter_id;

    /* Add new data (database) */
    SDN_DB_LOCK(&db_meter_mutex);
    _sdn_db_meter_copy(&meter_table_info->meter_entry[meter_id - 1], meter);
    SDN_DB_UNLOCK(&db_meter_mutex);

    return SDN_DB_RETURN_OK;
}

int sdn_db_meter_entry_delete(uint32_t meter_id)
{
    /* Delete data (database) */
    SDN_DB_LOCK(&db_meter_mutex);
    _sdn_db_meter_free(&meter_table_info->meter_entry[meter_id - 1]);
    SDN_DB_UNLOCK(&db_meter_mutex);

    return SDN_DB_RETURN_OK;
}

int sdn_db_meter_entry_modify(const sdn_db_meter_mod_t *meter)
{
    uint32_t meter_id = 0;

    meter_id = meter->meter_id;

    /* Modify data (database) */
    /* Should free resources in the meter before copying new data */
    SDN_DB_LOCK(&db_meter_mutex);
    _sdn_db_meter_free(&meter_table_info->meter_entry[meter_id-1]);
    _sdn_db_meter_copy(&meter_table_info->meter_entry[meter_id-1], meter);
    SDN_DB_UNLOCK(&db_meter_mutex);
    return SDN_DB_RETURN_OK;
}

int sdn_db_meter_table_features_get(sdn_db_meter_table_features_t *feature)
{
    if(!feature)
    {
        return SDN_DB_RETURN_FAILED;
    }

    /* Get from database since meter features are normally constant */
    memcpy(feature, &meter_table_info->meter_features, sizeof(*feature));

    return SDN_DB_RETURN_OK;

}

int sdn_db_init_meter_table(void)
{

    uint32_t    max_meters;

    pthread_mutex_init(&db_meter_mutex, NULL);

    meter_table_info = (sdn_db_meter_table_t*)malloc(sizeof(sdn_db_meter_table_features_t));
    memset(&meter_table_info->meter_features, 0, sizeof(sdn_db_meter_table_features_t));

    if(!meter_table_info)
    {
        /* Space not enough */
        return SDN_DB_RETURN_MALLOC_FAIL;
    }

    /* Get from HAL */
    if(SDN_HAL_RETURN_OK != sdn_hal_meter_table_features_get(&meter_table_info->meter_features))
    {
        DBG_SDN("[OPAL] meter feature can't be got from SDN HAL\n");
        return SDN_DB_RETURN_FAILED;
    }
    max_meters = meter_table_info->meter_features.max_meters;
    if(max_meters == 0)
    {
        DBG_SDN("init meter but max_meters is 0");
        return SDN_DB_RETURN_FAILED;
    }
    meter_table_info->meter_entry = (sdn_db_meter_mod_t*)malloc(max_meters * sizeof(sdn_db_meter_mod_t));

    if(!meter_table_info->meter_entry)
    {
        /* Space not enough */
        return SDN_DB_RETURN_MALLOC_FAIL;
    }
    memset(meter_table_info->meter_entry, 0, max_meters * sizeof(sdn_db_meter_mod_t));

    return SDN_DB_RETURN_OK;
}

void sdn_db_destroy_meter_table(void)
{

    uint32 max_meters;
    uint32 i, j;

    max_meters = meter_table_info->meter_features.max_meters;

    if(!meter_table_info->meter_entry || !meter_table_info->meter_entry->bands)
    {
        return;
    }

    /* Free bands in all meter entries */
    for(i = 0; i < max_meters; i++)
    {
        for(j = 0; j < max_meters; j++)
        {
            free(&meter_table_info->meter_entry->bands[j]);
        }
        free(&meter_table_info->meter_entry[i]);
    }

    memset(&meter_table_info, 0, sizeof(meter_table_info));

}

int sdn_db_set_table_miss_action(uint8_t table_id, rtk_of_tblMissAct_t action)
{
    if (table_info == NULL)
    {
        DBG_SDN("[SDN_DB] tables are not created!!!\n");

        return SDN_DB_RETURN_FAILED;
    }

    table_info[table_id].miss_action = (uint32) action;

    return SDN_DB_RETURN_OK;
}

int sdn_db_init_table_stat(void)
{
    uint32 table_id = 0;
    uint32 counter_id  = 0;

    DBG_SDN("\r\n[%s:%d] init start....\r\n", __FUNCTION__, __LINE__);
    pthread_mutex_init(&db_stats_mutex, NULL);

    /*Brandon : the initialization of coutner idx, counter_type, meter_idx which used by a flow tables.*/

#ifdef  CONFIG_SDK_RTL9310
    for (table_id = FT_PHASE_IGR_FT_0; table_id < FT_PHASE_END ; table_id++)
    {
        table_stats_db[table_id].table_id = FT_PHASE_END;
        table_stats_db[table_id].active_count =  ~0;  /*Brandon: RTK doesn't support active_count. */
        table_stats_db[table_id].lookup_count =  0;
        table_stats_db[table_id].matched_count = 0;
    }
#else
    for (i = 0; i < HAL_TABLE_NUM ; i++)
    {
        table_stats_db[table_id].table_id = INVALID_TABLE_INDEX;
        table_stats_db[table_id].active_count =  ~0;  /*Brandon: RTK doesn't support active_count. */
        table_stats_db[table_id].lookup_count =  0;
        table_stats_db[table_id].matched_count = 0;
    }

#endif

    for(counter_id = 0; counter_id < HAL_RULE_COUNTER_SIZE; counter_id++)
    {
        table_flow_stats_db[counter_id].flow_id = INVALID_FLOW_INDEX;

#ifdef  CONFIG_SDK_RTL9310
        table_flow_stats_db[counter_id].table_id = FT_PHASE_END;
#else
        table_flow_stats_db[counter_id].table_id = INVALID_TABLE_INDEX;
#endif

        table_flow_stats_db[counter_id].priority = 0xffff;
        //table_flow_stats_db[i].cnt_mode = OF_FLOW_CNTMODE_END;  /*Brandon: in future, need to change to rtk count mode.*/
        table_flow_stats_db[counter_id].packet_count = ~0;
        table_flow_stats_db[counter_id].byte_count   = ~0;
    }

     DBG_SDN("\r\n[%s:%d] init OK....\r\n", __FUNCTION__, __LINE__);
     return SDN_DB_RETURN_OK;
}

int sdn_db_get_switch_feature(uint32_t *switch_capabilities_p)
{
    if (switch_capabilities_p)
    {
        *switch_capabilities_p = SDN_DB_SWITCH_CAPAILITY;

        return SDN_DB_RETURN_OK;
    }

    return SDN_DB_RETURN_FAILED;
}

//new group code
static sdn_db_group_entry_t *sdn_db_group_get_validentryid(uint32_t group_id)
{
    uint32_t entry_id = 0;

    for (entry_id = 0; entry_id < SDN_HAL_MAX_GROUP_ENTRY_NUM; entry_id++)
    {
        if (g_group_table_db_p->group_entry_p[entry_id].action_bucket_p)
        {
            if(g_group_table_db_p->group_entry_p[entry_id].action_bucket_p[0].group_id == group_id)
            {
                g_group_table_db_p->group_entry_p[entry_id].entry_id = entry_id;
                return &g_group_table_db_p->group_entry_p[entry_id];
            }
        }
        else if (g_group_table_db_p->group_entry_p[entry_id].action_bucket_p == NULL)
        {
            g_group_table_db_p->group_entry_p[entry_id].entry_id = entry_id;
            return &g_group_table_db_p->group_entry_p[entry_id];
        }
    }

    return NULL;
}

sdn_db_group_entry_t *sdn_db_group_get_entrybygroupid(uint32_t group_id)
{
    uint32_t entry_id = 0;

    for (entry_id = 0; entry_id < SDN_HAL_MAX_GROUP_ENTRY_NUM; entry_id++)
    {
        if (g_group_table_db_p->group_entry_p[entry_id].action_bucket_p && g_group_table_db_p->group_entry_p[entry_id].action_bucket_p->group_id == group_id)
        {
            return &g_group_table_db_p->group_entry_p[entry_id];
        }
    }

    return NULL;
}

int32 sdn_db_group_table_init(void)
{
    uint32_t entry_id  = 0;

    g_group_table_db_p = (sdn_db_group_tbl_t *)malloc(SDN_HAL_MAX_GROUP_TABLE_NUM * sizeof(sdn_db_group_tbl_t));

    if (!g_group_table_db_p)
    {
        return SDN_DB_RETURN_FAILED;
    }

    memset(g_group_table_db_p, 0, SDN_HAL_MAX_GROUP_TABLE_NUM * sizeof(sdn_db_group_tbl_t));
    g_group_table_db_p->group_entry_p = NULL;

    g_group_table_db_p->group_entry_p = (sdn_db_group_entry_t *)malloc(SDN_HAL_MAX_GROUP_ENTRY_NUM * sizeof(sdn_db_group_entry_t));
    g_group_table_db_p->total_counter = 0;

    if (!g_group_table_db_p->group_entry_p)
    {
        free(g_group_table_db_p);
        g_group_table_db_p = NULL;
        return SDN_DB_RETURN_FAILED;
    }

    memset(g_group_table_db_p->group_entry_p, 0, SDN_HAL_MAX_GROUP_ENTRY_NUM * sizeof(sdn_db_group_entry_t));

    for (entry_id = 0; entry_id < SDN_HAL_MAX_GROUP_ENTRY_NUM; entry_id++)
    {
        g_group_table_db_p->group_entry_p[entry_id].action_bucket_p = NULL;
    }

    return SDN_DB_RETURN_OK;
}

int32 sdn_db_group_add_group_entry(sdn_db_group_entry_t *group_entry_p)
{
    uint32_t bucket_id = 0;
    sdn_db_group_entry_t *get_entry_p = NULL;

    DBG_SDN("\r\n[%s:%d] start \r\n", __FUNCTION__, __LINE__);
    if (group_entry_p == NULL)
    {
        DBG_SDN("\r\n[%s:%d] group entry is null \r\n", __FUNCTION__, __LINE__);
        return SDN_DB_RETURN_FAILED;
    }

    if (group_entry_p->action_bucket_p == NULL)
    {
        DBG_SDN("\r\n[%s:%d] action bucket is null\r\n", __FUNCTION__, __LINE__);
        return SDN_DB_RETURN_FAILED;
    }

    if (group_entry_p->bucket_len > SDN_HAL_MAX_GROUP_BUCKET_LEN)
    {
        DBG_SDN("\r\n[%s:%d] group entry action bucket is exceed \r\n", __FUNCTION__, __LINE__);
        return SDN_HAL_RETURN_FAILED;
    }

    if (g_group_table_db_p->total_counter > SDN_HAL_MAX_GROUP_ENTRY_NUM)
    {
        DBG_SDN("\r\n[%s:%d] total group entry to is exceed (%d)\r\n", __FUNCTION__, __LINE__, g_group_table_db_p->total_counter);
        return SDN_DB_RETURN_FAILED;
    }

    DBG_SDN("\r\n[%s:%d] try to get new group entry id (%d)!!! \r\n", __FUNCTION__, __LINE__, group_entry_p->entry_id);
    if ((get_entry_p = sdn_db_group_get_validentryid(group_entry_p->entry_id /*group_entry_p->action_bucket_p[0].group_id*/)) == NULL)
    {
        DBG_SDN("\r\n[%s:%d] group entry is allocated failed \r\n", __FUNCTION__, __LINE__);
        return SDN_DB_RETURN_FAILED;
    }

    if (get_entry_p->action_bucket_p == NULL)
    {
        DBG_SDN("\r\n[%s:%d] group entry is not existed!!! \r\n", __FUNCTION__, __LINE__);
        get_entry_p->action_bucket_p = (sdn_db_group_action_bucket_t *)malloc(group_entry_p->bucket_len * sizeof(sdn_db_group_action_bucket_t));
        if (!get_entry_p->action_bucket_p)
        {
           DBG_SDN("\r\n[%s:%d] bucket is null \r\n", __FUNCTION__, __LINE__);
            return SDN_DB_RETURN_FAILED;
        }

        memset(get_entry_p->action_bucket_p, 0, group_entry_p->bucket_len * sizeof(sdn_db_group_action_bucket_t));
        DBG_SDN("\r\n[%s:%d] get new bucket done!!! \r\n", __FUNCTION__, __LINE__);
    }
    else
    {
        DBG_SDN("\r\n[%s:%d] group entry is existed!!! \r\n", __FUNCTION__, __LINE__);
        for(bucket_id = 0; bucket_id < group_entry_p->bucket_len; bucket_id++)
        {
            if (get_entry_p->action_bucket_p[bucket_id].actions_p)
            {
                free(get_entry_p->action_bucket_p[bucket_id].actions_p);
                get_entry_p->action_bucket_p[bucket_id].actions_p = NULL;
            }
        }
        g_group_table_db_p->total_bucket_num -= group_entry_p->bucket_len;
        free(get_entry_p->action_bucket_p);
        get_entry_p->action_bucket_p = NULL;
        get_entry_p->action_bucket_p = (sdn_db_group_action_bucket_t *)malloc(group_entry_p->bucket_len * sizeof(sdn_db_group_action_bucket_t));

        if (!get_entry_p->action_bucket_p)
        {
            DBG_SDN("\r\n[%s:%d] bucket is null \r\n", __FUNCTION__, __LINE__);
            return SDN_DB_RETURN_FAILED;
        }
        memset(get_entry_p->action_bucket_p, 0, group_entry_p->bucket_len * sizeof(sdn_db_group_action_bucket_t));
        g_group_table_db_p->total_counter--;
    }
    DBG_SDN("\r\n[%s:%d] bucket done!!! \r\n", __FUNCTION__, __LINE__);

    for(bucket_id = 0; bucket_id < group_entry_p->bucket_len; bucket_id++)
    {
        get_entry_p->action_bucket_p[bucket_id].actions_p = (sdn_db_action_t *)malloc(group_entry_p->action_bucket_p[bucket_id].action_len * sizeof(sdn_db_action_t));
        get_entry_p->action_bucket_p[bucket_id].group_id = group_entry_p->action_bucket_p[bucket_id].group_id;
        memcpy(get_entry_p->action_bucket_p[bucket_id].actions_p, group_entry_p->action_bucket_p[bucket_id].actions_p, group_entry_p->action_bucket_p[bucket_id].action_len *sizeof(sdn_db_action_t));
        group_entry_p->action_bucket_p[bucket_id].action_len = group_entry_p->action_bucket_p[bucket_id].action_len;
        get_entry_p->action_bucket_p[bucket_id].action_len = group_entry_p->action_bucket_p[bucket_id].action_len;
    }
    DBG_SDN("\r\n[%s:%d] actions done!!! \r\n", __FUNCTION__, __LINE__);
    get_entry_p->entry_id   = group_entry_p->entry_id;
    get_entry_p->counters   = group_entry_p->counters;
    get_entry_p->bucket_len = group_entry_p->bucket_len;
    get_entry_p->group_type = group_entry_p->group_type;
    g_group_table_db_p->total_counter++;
    g_group_table_db_p->total_bucket_num += group_entry_p->bucket_len;

    DBG_SDN("\r\n[%s:%d] done!!! \r\n", __FUNCTION__, __LINE__);
    return SDN_DB_RETURN_OK;
}

int32 sdn_db_group_del_group_entry(uint32_t group_id)
{
    uint32_t bucket_id = 0;
    sdn_db_group_entry_t *get_entry_p = NULL;

    if ((get_entry_p = sdn_db_group_get_entrybygroupid(group_id))== NULL)
    {
        return SDN_DB_RETURN_FAILED;
    }

    if (get_entry_p->action_bucket_p)
    {
        for (bucket_id = 0; bucket_id < get_entry_p->bucket_len; bucket_id++)
        {
            if (get_entry_p->action_bucket_p[bucket_id].actions_p)
            {
                free(get_entry_p->action_bucket_p[bucket_id].actions_p);
                get_entry_p->action_bucket_p[bucket_id].action_len = 0;
                get_entry_p->action_bucket_p[bucket_id].actions_p = NULL;
            }
        }
        free(get_entry_p->action_bucket_p);
        get_entry_p->action_bucket_p = NULL;
    }
    g_group_table_db_p->total_bucket_num -= get_entry_p->bucket_len;
    DBG_SDN("[%s:%d] del group entry(%d)  from db is ok \n", __FUNCTION__, __LINE__,get_entry_p->entry_id);
    memset(get_entry_p, 0, sizeof(sdn_db_group_entry_t));
    g_group_table_db_p->total_counter--;
    DBG_SDN("[%s:%d] del group entry(%d) is ok \n", __FUNCTION__, __LINE__, group_id);
    return SDN_DB_RETURN_OK;
}
//new group code

//qos code++
int sdn_db_init_qos_table(void)
{
    uint32_t q_id = 0;

    memset(sdn_db_qos_table, 0, sizeof(sdn_db_qos_table));

    for (q_id = 0; q_id < _countof(sdn_db_qos_table); q_id++)
    {
         sdn_db_qos_table[q_id].is_applied = false;
         sdn_db_qos_table[q_id].qid      = q_id;
         sdn_db_qos_table[q_id].qos_type = SDN_DB_QOS_TYPE_HTB;
         sdn_db_qos_table[q_id].priority = 0;
         sdn_db_qos_table[q_id].burst_size = 0x8000;
         sdn_db_qos_table[q_id].max_rate   = 0xfffff;
         sdn_db_qos_table[q_id].min_rate   = 0xfffff;
         sdn_db_qos_table[q_id].low_dst_port_bitmap = 0;
         sdn_db_qos_table[q_id].hi_dst_port_bitmap  = 0;
    }

    pthread_mutex_init(&db_qos_mutex, NULL);
    return SDN_DB_RETURN_OK;
}

int sdn_db_destroy_qos_table(void)
{
    uint32_t q_id = 0;
    memset(sdn_db_qos_table, 0, sizeof(sdn_db_qos_table));

    for (q_id = 0; q_id < _countof(sdn_db_qos_table); q_id++)
    {
         sdn_db_qos_table[q_id].is_applied = false;
         sdn_db_qos_table[q_id].qid      = q_id;
         sdn_db_qos_table[q_id].qos_type = SDN_DB_QOS_TYPE_HTB;
         sdn_db_qos_table[q_id].priority = 0;
         sdn_db_qos_table[q_id].burst_size = 0x8000;
         sdn_db_qos_table[q_id].max_rate   = 0xfffff;
         sdn_db_qos_table[q_id].min_rate   = 0xfffff;
         sdn_db_qos_table[q_id].low_dst_port_bitmap = 0;
         sdn_db_qos_table[q_id].hi_dst_port_bitmap  = 0;
    }

    pthread_mutex_destroy(&db_qos_mutex);

    return SDN_DB_RETURN_OK;
}

int sdn_db_set_qos_entry(sdn_db_qos_entry_t qos_entry)
{
    uint32_t q_id = 0;

    if ( (qos_entry.qid < SDN_MIN_QUEUE_ID) || (qos_entry.qid >= SDN_MAX_QUEUE_ID) )
    {
        return SDN_DB_RETURN_FAILED;
    }

    q_id = qos_entry.qid;

    SDN_DB_LOCK(&db_qos_mutex);
    memcpy(&sdn_db_qos_table[q_id], &qos_entry, sizeof(sdn_db_qos_table[q_id]));
    sdn_db_qos_table[q_id].is_applied = true;
    SDN_DB_UNLOCK(&db_qos_mutex);

    return SDN_DB_RETURN_OK;
}

int sdn_db_del_qos_entry(sdn_db_qos_entry_t qos_entry)
{
    uint32_t q_id  = 0;

    if ( (qos_entry.qid < SDN_MIN_QUEUE_ID) || (qos_entry.qid >= SDN_MAX_QUEUE_ID) )
    {
        return SDN_DB_RETURN_FAILED;
    }

    q_id = qos_entry.qid;

    SDN_DB_LOCK(&db_qos_mutex);
    memset(&sdn_db_qos_table[q_id], 0, sizeof(sdn_db_qos_table[q_id]));
    sdn_db_qos_table[q_id].is_applied = false;
    sdn_db_qos_table[q_id].qid = q_id;
    sdn_db_qos_table[q_id].qos_type = SDN_DB_QOS_TYPE_HTB;
    sdn_db_qos_table[q_id].priority = 0;
    sdn_db_qos_table[q_id].burst_size = 0x8000;
    sdn_db_qos_table[q_id].max_rate   = 0xfffff;
    sdn_db_qos_table[q_id].min_rate   = 0xfffff;
    sdn_db_qos_table[q_id].low_dst_port_bitmap = 0;
    sdn_db_qos_table[q_id].hi_dst_port_bitmap  = 0;
    SDN_DB_UNLOCK(&db_qos_mutex);

    return SDN_DB_RETURN_OK;
}

int sdn_db_get_qos_entry(sdn_db_qos_entry_t *qos_entry_p)
{
    uint32_t q_id = 0;

    if (qos_entry_p == NULL)
    {
        return SDN_DB_RETURN_FAILED;
    }

    if ( (qos_entry_p->qid < SDN_MIN_QUEUE_ID) || (qos_entry_p->qid >= SDN_MAX_QUEUE_ID) )
    {
        return SDN_DB_RETURN_FAILED;
    }

    q_id = qos_entry_p->qid;

    qos_entry_p->qos_type   = sdn_db_qos_table[q_id].qos_type;
    qos_entry_p->burst_size = sdn_db_qos_table[q_id].burst_size;
    qos_entry_p->priority   = sdn_db_qos_table[q_id].priority;
    qos_entry_p->max_rate   = sdn_db_qos_table[q_id].max_rate;
    qos_entry_p->min_rate   = sdn_db_qos_table[q_id].min_rate;
    qos_entry_p->low_dst_port_bitmap = sdn_db_qos_table[q_id].low_dst_port_bitmap;
    qos_entry_p->hi_dst_port_bitmap  = sdn_db_qos_table[q_id].hi_dst_port_bitmap;

    return SDN_DB_RETURN_OK;
}

int sdn_db_set_port_qos(uint32_t lport, uint32_t q_id)
{
    if ( (q_id < SDN_MIN_QUEUE_ID) || (q_id >= SDN_MAX_QUEUE_ID) )
    {
        return SDN_DB_RETURN_FAILED;
    }

    SDN_DB_LOCK(&db_qos_mutex);

    if (lport < 32)
    {
        SDN_FLAG_ON(sdn_db_qos_table[q_id].low_dst_port_bitmap, lport);
        //DBG_SDN("[%s:%d] q(%d) low port list(%08x) \n", __FUNCTION__, __LINE__, q_id, sdn_db_qos_table[q_id].low_dst_port_bitmap);
    }
    else
    {
        SDN_FLAG_ON(sdn_db_qos_table[q_id].hi_dst_port_bitmap, (1 << (lport - 32)) );
        //DBG_SDN("[%s:%d] q(%d) high port list(%08x) \n", __FUNCTION__, __LINE__, q_id, sdn_db_qos_table[q_id].hi_dst_port_bitmap);
    }
    SDN_DB_UNLOCK(&db_qos_mutex);

    return SDN_DB_RETURN_OK;
}

sdn_db_qos_entry_t *sdn_db_get_port_qos(uint32_t lport)
{
    uint32_t q_id = 0;

    for (q_id = 0; q_id < _countof(sdn_db_qos_table); q_id++)
    {
        //DBG_SDN("[%s:%d] q(%d) low port list(%08x) check port(%d) \n", __FUNCTION__, __LINE__, q_id, sdn_db_qos_table[q_id].low_dst_port_bitmap, lport);
        if (SDN_FLAG_IS_ON(sdn_db_qos_table[q_id].low_dst_port_bitmap, lport))
        {
            //DBG_SDN("[%s:%d] q(%d) low port list(%08x) \n", __FUNCTION__, __LINE__, q_id, sdn_db_qos_table[q_id].low_dst_port_bitmap);
            return &sdn_db_qos_table[q_id];
        }
        else if (SDN_FLAG_IS_ON(sdn_db_qos_table[q_id].hi_dst_port_bitmap, (1 << (lport - 32)) ) )
        {
            //DBG_SDN("[%s:%d] q(%d) hi port list(%08x) \n", __FUNCTION__, __LINE__, q_id, sdn_db_qos_table[q_id].hi_dst_port_bitmap);
            return &sdn_db_qos_table[q_id];
        }
    }

    return NULL;
}

int sdn_db_del_port_qos(uint32_t lport, uint32_t q_id)
{
    if ( (q_id < SDN_MIN_QUEUE_ID) || (q_id >= SDN_MAX_QUEUE_ID) )
    {
        return SDN_DB_RETURN_FAILED;
    }

    SDN_DB_LOCK(&db_qos_mutex);

    if (lport < 32)
    {
        SDN_FLAG_OFF(sdn_db_qos_table[q_id].low_dst_port_bitmap, lport);
    }
    else
    {
        SDN_FLAG_OFF(sdn_db_qos_table[q_id].hi_dst_port_bitmap, (1 << (lport - 32)) );
    }

    SDN_DB_UNLOCK(&db_qos_mutex);

    return SDN_DB_RETURN_OK;
}
//qos cod--
