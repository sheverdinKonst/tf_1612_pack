#include <stdio.h>
#include "dal/rtrpc/rtrpc_diag.h"
#include "rtk/phy.h"
#include <stdint.h>
#include <stdlib.h>

static void getNum(char *str, uint8_t *num);

int main(int argc, char **argv)
{
    printf("Hello, World!\n");
    uint8_t unit = 0;
    uint8_t port = 0;
    uint8_t linktype = 0;

    getNum(argv[1], &unit);
    getNum(argv[2], &port);
    getNum(argv[3], &linktype);

    printf("unit = %d, port = %d, linktype = %d\n", unit,port, linktype);

    int32 res_init = rtk_diag_init(unit);
    printf("res_init = %d\n", res_init);
    rtk_rtctResult_t rtctRes;
    rtctRes.linkType = linktype;
    uint32 res = rtrpc_diag_portRtctResult_get(unit, port, &rtctRes);
    printf("res = %d\n", res);
    if (linktype == PORT_SPEED_1000M)
    {
        printf("----------------- channel A ------------------------\n");
        printf("A Short         =  %d\n", rtctRes.un.ge_result.channelAShort);
        printf("A Open          =  %d\n", rtctRes.un.ge_result.channelAOpen);
        printf("A Mismatch      =  %d\n", rtctRes.un.ge_result.channelAMismatch);
        printf("A Linedriver    =  %d\n", rtctRes.un.ge_result.channelALinedriver);
        printf("A HiImpedance   =  %d\n", rtctRes.un.ge_result.channelAHiImpedance);
        printf("A Cross         =  %d\n", rtctRes.un.ge_result.channelACross);
        printf("A PartialCross  =  %d\n", rtctRes.un.ge_result.channelAPartialCross);
        printf("A PairBusy      =  %d\n", rtctRes.un.ge_result.channelAPairBusy);
        printf("A Len           =  %d\n", rtctRes.un.ge_result.channelALen);

        printf("----------------- channel B ------------------------\n");
        printf("B Short         =  %d\n", rtctRes.un.ge_result.channelBShort);
        printf("B Open          =  %d\n", rtctRes.un.ge_result.channelBOpen);
        printf("B Mismatch      =  %d\n", rtctRes.un.ge_result.channelBMismatch);
        printf("B Linedriver    =  %d\n", rtctRes.un.ge_result.channelBLinedriver);
        printf("B HiImpedance   =  %d\n", rtctRes.un.ge_result.channelBHiImpedance);
        printf("B Cross         =  %d\n", rtctRes.un.ge_result.channelBCross);
        printf("B PartialCross  =  %d\n", rtctRes.un.ge_result.channelBPartialCross);
        printf("B PairBusy      =  %d\n", rtctRes.un.ge_result.channelBPairBusy);
        printf("B Len           =  %d\n", rtctRes.un.ge_result.channelBLen);

        printf("----------------- channel C ------------------------\n");
        printf("C Short         =  %d\n", rtctRes.un.ge_result.channelCShort);
        printf("C Open          =  %d\n", rtctRes.un.ge_result.channelCOpen);
        printf("C Mismatch      =  %d\n", rtctRes.un.ge_result.channelCMismatch);
        printf("C Linedriver    =  %d\n", rtctRes.un.ge_result.channelCLinedriver);
        printf("C HiImpedance   =  %d\n", rtctRes.un.ge_result.channelCHiImpedance);
        printf("C Cross         =  %d\n", rtctRes.un.ge_result.channelCCross);
        printf("C PartialCross  =  %d\n", rtctRes.un.ge_result.channelCPartialCross);
        printf("C PairBusy      =  %d\n", rtctRes.un.ge_result.channelCPairBusy);
        printf("C Len           =  %d\n", rtctRes.un.ge_result.channelCLen);

        printf("----------------- channel D ------------------------\n");
        printf("D Short         =  %d\n", rtctRes.un.ge_result.channelDShort);
        printf("D Open          =  %d\n", rtctRes.un.ge_result.channelDOpen);
        printf("D Mismatch      =  %d\n", rtctRes.un.ge_result.channelDMismatch);
        printf("D Linedriver    =  %d\n", rtctRes.un.ge_result.channelDLinedriver);
        printf("D HiImpedance   =  %d\n", rtctRes.un.ge_result.channelDHiImpedance);
        printf("D Cross         =  %d\n", rtctRes.un.ge_result.channelDCross);
        printf("D PartialCross  =  %d\n", rtctRes.un.ge_result.channelDPartialCross);
        printf("D PairBusy      =  %d\n", rtctRes.un.ge_result.channelDPairBusy);
        printf("D Len           =  %d\n", rtctRes.un.ge_result.channelDLen);
    }
    else if (linktype != PORT_SPEED_1000M)
    {
        printf("isRxShort           =  %d\n", rtctRes.un.fe_result.isRxShort);
        printf("isTxShort           =  %d\n", rtctRes.un.fe_result.isTxShort);
        printf("isRxOpen            =  %d\n", rtctRes.un.fe_result.isRxOpen);
        printf("isTxOpen            =  %d\n", rtctRes.un.fe_result.isTxOpen);
        printf("isRxMismatch        =  %d\n", rtctRes.un.fe_result.isRxMismatch);
        printf("isTxMismatch        =  %d\n", rtctRes.un.fe_result.isTxMismatch);
        printf("isRxLinedriver      =  %d\n", rtctRes.un.fe_result.isRxLinedriver);
        printf("isTxLinedriver      =  %d\n", rtctRes.un.fe_result.isTxLinedriver);
        printf("rxLen               =  %d\n", rtctRes.un.fe_result.rxLen);
        printf("txLen               =  %d\n", rtctRes.un.fe_result.txLen);
    }

    printf("A channelShort          = %d\n ", rtctRes.un.channels_result.a.channelShort);
    printf("A channelOpen           = %d\n ", rtctRes.un.channels_result.a.channelOpen);
    printf("A channelLowMismatch    = %d\n ", rtctRes.un.channels_result.a.channelLowMismatch);
    printf("A channelHighMismatch   = %d\n ", rtctRes.un.channels_result.a.channelHighMismatch);
    printf("A channelCrossoverA     = %d\n ", rtctRes.un.channels_result.a.channelCrossoverA);
    printf("A channelCrossoverB     = %d\n ", rtctRes.un.channels_result.a.channelCrossoverB);
    printf("A channelCrossoverC     = %d\n ", rtctRes.un.channels_result.a.channelCrossoverC);
    printf("A channelCrossoverD     = %d\n ", rtctRes.un.channels_result.a.channelCrossoverD);
    printf("A channelLen            = %d\n ", rtctRes.un.channels_result.a.channelLen);

    printf("B channelShort          = %d\n ", rtctRes.un.channels_result.b.channelShort);
    printf("B channelOpen           = %d\n ", rtctRes.un.channels_result.b.channelOpen);
    printf("B channelLowMismatch    = %d\n ", rtctRes.un.channels_result.b.channelLowMismatch);
    printf("B channelHighMismatch   = %d\n ", rtctRes.un.channels_result.b.channelHighMismatch);
    printf("B channelCrossoverA     = %d\n ", rtctRes.un.channels_result.b.channelCrossoverA);
    printf("B channelCrossoverB     = %d\n ", rtctRes.un.channels_result.b.channelCrossoverB);
    printf("B channelCrossoverC     = %d\n ", rtctRes.un.channels_result.b.channelCrossoverC);
    printf("B channelCrossoverD     = %d\n ", rtctRes.un.channels_result.b.channelCrossoverD);
    printf("B channelLen            = %d\n ", rtctRes.un.channels_result.b.channelLen);

    printf("C channelShort          = %d\n ", rtctRes.un.channels_result.c.channelShort);
    printf("C channelOpen           = %d\n ", rtctRes.un.channels_result.c.channelOpen);
    printf("C channelLowMismatch    = %d\n ", rtctRes.un.channels_result.c.channelLowMismatch);
    printf("C channelHighMismatch   = %d\n ", rtctRes.un.channels_result.c.channelHighMismatch);
    printf("C channelCrossoverA     = %d\n ", rtctRes.un.channels_result.c.channelCrossoverA);
    printf("C channelCrossoverB     = %d\n ", rtctRes.un.channels_result.c.channelCrossoverB);
    printf("C channelCrossoverC     = %d\n ", rtctRes.un.channels_result.c.channelCrossoverC);
    printf("C channelCrossoverD     = %d\n ", rtctRes.un.channels_result.c.channelCrossoverD);
    printf("C channelLen            = %d\n ", rtctRes.un.channels_result.c.channelLen);

    printf("D channelShort          = %d\n ", rtctRes.un.channels_result.d.channelShort);
    printf("D channelOpen           = %d\n ", rtctRes.un.channels_result.d.channelOpen);
    printf("D channelLowMismatch    = %d\n ", rtctRes.un.channels_result.d.channelLowMismatch);
    printf("D channelHighMismatch   = %d\n ", rtctRes.un.channels_result.d.channelHighMismatch);
    printf("D channelCrossoverA     = %d\n ", rtctRes.un.channels_result.d.channelCrossoverA);
    printf("D channelCrossoverB     = %d\n ", rtctRes.un.channels_result.d.channelCrossoverB);
    printf("D channelCrossoverC     = %d\n ", rtctRes.un.channels_result.d.channelCrossoverC);
    printf("D channelCrossoverD     = %d\n ", rtctRes.un.channels_result.d.channelCrossoverD);
    printf("D channelLen            = %d\n ", rtctRes.un.channels_result.d.channelLen);
    return 0;
}

static void getNum(char *str, uint8_t *num)
{
    uint8_t intValue  = 0;
    intValue = (uint8_t)(strtol(str, NULL, 10));
    if (intValue < 1 || intValue > 8)
    {
        *num    = 0;
    } else {
        *num = intValue;
    }
}
