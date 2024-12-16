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

#include <fcntl.h>
//#include "rtcore/rtcore.h" /* Mask by SD6 */
#include "private/drv/nic/nic_common.h"
#include "sdn_util.h"
#include "common/type.h"
#include <common/util/rt_util.h>

int32 bitop_findLastBitInAaray(rtk_bitmap_t *pArray, uint32 arraySize)
{
    rtk_bitmap_t    temp;
    int32           index, i;

    if ((NULL == pArray)){
        return -1;
    }

    /* find last bit in Array */
    for (index = arraySize - 1; index >= 0; index--)
    {
        if (0 == pArray[index])
        { /* this word is 0, no need futher process */
            continue;
        }

        temp = (pArray[index]);
        for (i = (BITMAP_WIDTH-1); i >= 0; i--)
        {
            if (temp & (1 << i))
                return ((index * BITMAP_WIDTH) + i);
        }
    }

    /* not found, return -1 */
    return -1;
} /* end of rt_bitop_findLastBitInAaray */

 /* Function Name:
  *      bitop_numberOfSetBitsInArray
  * Description:
  *      caculate how much bit is set in this array
  * Input:
  *      pArray      - the array to be handled
  *      arraySize   - Size of Array
  * Output:
  *
  * Return:
  *      number bits are set
  * Note:
  *      None
  */
uint32 bitop_numberOfSetBitsInArray(rtk_bitmap_t *pArray, uint32 arraySize)
{
    uint32          index;
    rtk_bitmap_t    value;
    uint32          numOfBits;

    if (NULL == pArray)
    {/* if null pointer, return 0 bits */
        return 0;
    }

    numOfBits = 0;

    for (index = 0; index < arraySize; index++)
    {
        value = pArray[index];

        value = value - ((value >> 1) & 0x55555555);
        value = (value & 0x33333333) + ((value >> 2) & 0x33333333);
        numOfBits += (((((value + (value >> 4)) & 0xF0F0F0F)) * 0x1010101) >> 24);
    }

    return numOfBits;
} /* end of bitop_numberOfSetBitsInArray */

int32
ipv6IsAllOnes_ret(uint8 *pIp6)
{
    int32 i;

    for (i = 0; i < IPV6_ADDR_LEN; i++)
    {
        if (0xFF != pIp6[i])
            return FALSE;
    }

    return TRUE;
}

int32
ipv4IsAllOnes_ret(uint32 ip4_mask)
{
    if(ip4_mask == 0xFFFFFFFF)
        return TRUE;

    return FALSE;
}

int32 macIsAllOnes_ret(uint8 *mac_mask)
{
    int32 i;

    for (i = 0; i < MAC_LEN; i++)
    {
        if (0xFF != mac_mask[i])
            return FALSE;
    }

    return TRUE;
}