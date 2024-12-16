/*
 * Copyright (C) 2009-2020 Realtek Semiconductor Corp.
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
 * $Date: 2020-01-10 13:46:55 +0800  $
 *
 * Purpose : Definition for CHT SDN Resource
 *
 * Feature : The file includes the following modules and sub-modules
 *
 */

#ifndef CHT_SDN_RESOURCE_H
#define CHT_SDN_RESOURCE_H

/*
 * Include Files
 */

/*
 * Symbol Definition
 */

#define CHT_L2FWD_NORMAL_LOCATION       0
#define CHT_L2FWD_NORMAL_SIZE           150
#define CHT_L2FWD_DEF_LOCATION          150
#define CHT_L2FWD_DEF_SIZE              174
#define CHT_TEID_NORMAL_LOCATION        324
#define CHT_TEID_NORMAL_SIZE            3200
#define CHT_TEID_DEF_LOCATION           3524
#define CHT_TEID_DEF_SIZE               444

#define CHT_L2FWD_NORMAL_PRI            2000    /* RTK FT phase 0 */
#define CHT_L2FWD_DEF_PRI               1500    /* RTK FT phase 0 */
#define CHT_TEID_NORMAL_PRI             1000    /* RTK FT phase 0 */
#define CHT_TEID_DEF_PRI                500     /* RTK FT phase 0 */

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
int cht_sdn_add_flow(sdn_db_flow_entry_t *flow);
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
int cht_sdn_delete_flow(uint8 table_id, uint16 priority, uint32 len_match, sdn_db_match_field_t* flow_match);
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
int cht_sdn_flow_entry_search(uint32 table_id, uint32 priority, uint32 len_match, sdn_db_match_field_t* flow_match, uint32 *flow_index, uint32 *is_found);



#endif /* CHT_SDN_RESOURCE_H */
