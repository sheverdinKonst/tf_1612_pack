//#include <config.h>

#if defined(CONFIG_SDK_RTL8390)
confcode_prv_t construct_rtl8214fc_6276A_rtl8390_perchip_serdes[] = {
    /* Serdes */
    { 0, 0x1e, 8},
    { 0, 0x1f, 0x405}, { 0, 0x14, 0x8EC},     //# S0 default 0x8E4, bit 3 = 1 => S0 40 bit, 0 --- S0 20 bits.

    { 0, 0x1f, 0x404}, { 0, 0x17, 0x5359},//# S0 RXCLKWR change to neg edge

    { 0, 0x1f, 0x424}, { 0, 0x17, 0x5359},    //# S1 RXCLKWR change to neg edge

    //## Analog AFE patch
    { 0, 0x1f, 0x42C},     //# 0x2161 0x4000
    { 0, 0x11, 0x4000},
    { 0, 0x12, 0x2020},  //# S1 KI=0, KP = 3

    { 0, 0x1f, 0x42D},     //# 0x2169 0xC014
    { 0, 0x11, 0xC014},

    { 0, 0x1f, 0x464},     //# 0x2325 0x202a
    { 0, 0x15, 0x202a},     //Rsel = 10G/FF, kp1_1 = 2
    { 0, 0x12, 0x1fe0},    //#configure VGA=0x3

    { 0, 0x1f, 0x404},    // # 0x232a 0x40ff
    { 0, 0x10, 0x9703},

    { 0, 0x1f, 0x42E}, //    # 0x2171 0x218F
    { 0, 0x11, 0x218F},

    { 0, 0x1f, 0x044C}, //# 0x2261 = 0x4000, Turn off RING PLL
    { 0, 0x11, 0x4000},

    {0, 31, 0x0460}, {0, 0x10, 0x4800},
    {0, 31, 0x0462}, {0, 0x12, 0x96b3},
    {0, 0x15, 0x6e58},  //#enable cmu lbw for Icp
    {0, 31, 0x0464}, {0, 0x13, 0x3def}, {0, 0x16, 0xf072},
    {0, 31, 0x0465}, {0, 0x10, 0x4208}, {0, 0x11, 0x3a08}, {0, 0x12, 0x4007},

    {0, 31, 0x0464}, {0, 0x17, 0x84f5}, {0, 0x17, 0x04f5},

    { 0, 0x1f, 0x44C},     //# 2021.01.18 add for reduce fiber port jitter
    { 0, 0x11, 0x4000},
    { 0, 0x12, 0x2020},  //# S1 KI=0, KP = 3

    { 0, 0x1f, 0x44D},     //# 0x2169 0xC014
    { 0, 0x11, 0xC014},
    { 0, 0x1f, 0x44E}, //    # 0x2171 0x218F
    { 0, 0x11, 0x218F}, //# 2021.01.18 end of reduce fiber port jitter patch
};

confcode_phy_patch_t construct_rtl8214fc_6276A_rtl8390_perchip_serdes2[] = {
    {30, 15, 0, 0x8},
    //#15-11:Filter_0, 10-7:1=force, 0=calib
    {31, 15, 0, 0x0464},
    {0x12, 15, 11, 0x3},
    {0x12, 10, 7, 0xf},
    //#14-10:fitler_3, 9-5:filter_2, 4-0:filter_1
    {31, 15, 0, 0x0464},
    {0x13, 14, 10, 0xf},
    {0x13, 9, 5, 0x11},
    {0x13, 4, 0, 0x10},
    //#eanble offset calibration
    {31, 15, 0, 0x0462},
    {0x10, 13, 12, 0x3},
    //#2013/8/13, RL6276B S0,S1 disable impedance auto-calibration, force R=0x8;
    //#RL6276B S1 impedance was controlled by S0
    //#page.0x462,reg.0x13: [20]=0, [25]=0, [19:16]=0x8, [24:21]=0x8
    {31, 15, 0, 0x0462},
    {0x13, 15, 0, 0x5108},
};

confcode_prv_t construct_rtl8214fc_6276A_rtl8390_perchip_serdes3[] = {
    /* cmu reset */
    {0, 31, 0x0467}, {0, 0x14, 0x143d}, {0, 0x14, 0x3c15}, {0, 0x14, 0x3c17}, {0, 0x14, 0x0},
    /* digital soft reset */
    {0, 31, 0x0404}, {0, 0x13, 0x7146}, {0, 0x13, 0x7106},

    { 0, 0x1f, 0x42D},//     # 0x2169 bit0 = S1 ana reset
    { 0, 0x11, 0xC015}, { 0, 0x11, 0xC014},

    { 0, 0x1e, 0},

    /* 18B serdes force qsgmii */
    {0, 30, 0x0008},
    {0, 31, 0x404 },
    {0, 20, 0xd749 },
    {0, 31, 0x424 },
    {0, 20, 0xd749 },

    { 0, 0x001e, 0x0000 },
};

confcode_rv_t construct_rtl8214fc_6276A_rtl8390_perport[] = {\
    {0x1e, 0x0001}, {0x1f, 0xa5d}, {0x10, 0x0},\
    {0x1f, 0x0bc0}, {0x16, 0x0c00}, {0x1f, 0x0a43}, \
    {0x11, 0x0043}, \
    /* 1000M green short cable change viterbi from partial viterbi to full viterbi */
    {0x13, 0x809f}, {0x14, 0x6b20}, \
    {0x1b, 0x809a}, {0x1c, 0x8933},
    {0x1e, 0x0000}, \
    /*8214FC fiber down speed patch*/
    {0x1e, 0x0003}, {0x1f, 0x0003}, {0x15, 0xE47F}, \
    {0x1f, 0x0009}, {0x15, 0x46F4}, {0x1e, 0x0000}, \
    };

confcode_prv_t construct_rtl8214fc_6276A_rtl8390_fiber_perchip[] = {\
    {0, 0x1e, 0x0008}, {0, 0x1f, 0x0260}, {0, 0x14, 0x032c}, \
    {0, 0x1f, 0x0280}, {0, 0x10, 0xf0bb}, {0, 0x1f, 0x0266}, {0, 0x10, 0x0f95}, \
    {0, 0x13, 0x0f95}, {0, 0x14, 0x0f95}, {0, 0x15, 0x0f95}, {0, 0x1e, 0x0003}, \
    //fiber rx change to neg. edge 0x17
    //One Giga Nway disable linkon 0x14
    //130719 Disable fiber Fib100 down speed feature, default 0x0f20  0x11
    {0, 0x1f, 0x0008}, {0, 0x17, 0x5359}, {0, 0x14, 0x074D}, {0, 0x11, 0x0f00}, \
    {0, 0x1e, 0x0000}, {1, 0x1e, 0x0003}, \
    {1, 0x1f, 0x0008}, {1, 0x17, 0x5359}, {1, 0x14, 0x074D}, {1, 0x11, 0x0f00}, \
    {1, 0x1e, 0x0000}, {2, 0x1e, 0x0003}, \
    {2, 0x1f, 0x0008}, {2, 0x17, 0x5359}, {2, 0x14, 0x074D}, {2, 0x11, 0x0f00}, \
    {2, 0x1e, 0x0000}, {3, 0x1e, 0x0003}, \
    {3, 0x1f, 0x0008}, {3, 0x17, 0x5359}, {3, 0x14, 0x074D}, {3, 0x11, 0x0f00}, \
    {3, 0x1e, 0x0000} \
    };

confcode_prv_t construct_rtl8214fc_6276C_rtl8390_perchip_savpow_1[] = {
    /* per chip  */
    {0, 0x1e, 0x0001},{1, 0x1e, 0x0001},{2, 0x1e, 0x0001},{3, 0x1e, 0x0001},
    {0, 0x1f, 0x0bc0},{1, 0x1f, 0x0bc0},{2, 0x1f, 0x0bc0},{3, 0x1f, 0x0bc0},
    {0, 0x12, 0x0000},{1, 0x12, 0x0000},{2, 0x12, 0x0000},{3, 0x12, 0x0000},
    {0, 0x13, 0x01d0},{1, 0x13, 0x01d0},{2, 0x13, 0x01d0},{3, 0x13, 0x01d0},
    {0, 0x1e, 0x0008},{0, 0x1f, 0x0266},{0, 0x12, 0x000b},
};

confcode_prv_t construct_rtl8214fc_6276C_rtl8390_perchip_savpow_1_1[] = {
    {0, 0x1f, 0x0261},{0, 0x10, 0x00ff},{0, 0x1e, 0x0000},
};

confcode_prv_t construct_rtl8214fc_6276C_rtl8390_perchip_savpow_2[] = {
    /* per chip  */
    {0, 0x1e, 0x0008},{0, 0x1f, 0x0266},{0, 0x12, 0x0003},
    {0, 0x1e, 0x0001},{1, 0x1e, 0x0001},{2, 0x1e, 0x0001},{3, 0x1e, 0x0001},
    {0, 0x1f, 0x0bc0},{1, 0x1f, 0x0bc0},{2, 0x1f, 0x0bc0},{3, 0x1f, 0x0bc0},
    {0, 0x13, 0x01c0},{1, 0x13, 0x01c0},{2, 0x13, 0x01c0},{3, 0x13, 0x01c0},
    {0, 0x1e, 0x0000},{1, 0x1e, 0x0000},{2, 0x1e, 0x0000},{3, 0x1e, 0x0000},
};
#endif

#if defined(CONFIG_SDK_RTL8380)
confcode_prv_t construct_rtl8214fc_0455B_rtl8380_perchip[] = {
    {0, 0x1f, 0x0000},{0, 0x1d, 0x0008},{0, 0x1f, 0x0460},{0, 0x10, 0x5800},\
    {0, 0x11, 0x4000},{0, 0x12, 0x5400},{0, 0x13, 0x7ffb},{0, 0x14, 0x001f},\
    {0, 0x15, 0x60c1},{0, 0x16, 0x60c1},{0, 0x17, 0xffff},{0, 0x1f, 0x0461},\
    {0, 0x10, 0xffff},{0, 0x11, 0xcc6f},{0, 0x12, 0x1004},{0, 0x13, 0x14a5},\
    {0, 0x14, 0xf514},{0, 0x15, 0x0514},{0, 0x16, 0x8a41},{0, 0x17, 0x48c0},\
    {0, 0x1f, 0x0462},{0, 0x10, 0xf216},{0, 0x11, 0xf04a},{0, 0x12, 0x97b3},\
    {0, 0x13, 0x5318},{0, 0x14, 0x0f03},{0, 0x15, 0xc814},{0, 0x16, 0x7860},\
    {0, 0x17, 0xbdef},{0, 0x1f, 0x0463},{0, 0x10, 0xffff},{0, 0x11, 0xc60a},\
    {0, 0x12, 0xa052},{0, 0x13, 0x00f5},{0, 0x14, 0x4205},{0, 0x15, 0x7e19},\
    {0, 0x16, 0x44a2},{0, 0x17, 0xc3e0},{0, 0x1f, 0x0464},{0, 0x10, 0x601c},\
    {0, 0x11, 0xc9ae},{0, 0x12, 0x3fe0},{0, 0x13, 0x3e50},{0, 0x14, 0xffff},\
    {0, 0x15, 0x7803},{0, 0x16, 0x007a},{0, 0x17, 0x04f5},{0, 0x1f, 0x0465},\
    {0, 0x10, 0x41ff},{0, 0x11, 0x39ff},{0, 0x12, 0x4010},{0, 0x13, 0x007f},\
    {0, 0x14, 0x619f},{0, 0x15, 0x29fb},{0, 0x16, 0x7f60},{0, 0x1f, 0x0467},\
    {0, 0x16, 0x060b},{0, 0x14, 0xfe3d},{0, 0x14, 0xfe3f},{0, 0x1f, 0x0405},\
    {0, 0x14, 0x08ec},{0, 0x1f, 0x042c},{0, 0x10, 0xc803},{0, 0x11, 0x4000},\
    {0, 0x14, 0xd588},{0, 0x1f, 0x042d},{0, 0x16, 0x201f},{0, 0x1f, 0x044c},\
    {0, 0x10, 0xc803},{0, 0x11, 0x4000},{0, 0x14, 0xd588},{0, 0x1f, 0x044d},\
    {0, 0x16, 0x201f},{0, 0x1f, 0x0467},{0, 0x14, 0xfe0f},{0, 0x14, 0xfe3f},\
    {0, 0x1f, 0x0405},{0, 0x12, 0x0fa4},{0, 0x1f, 0x040a},{0, 0x10, 0x0616},\
    {0, 0x1f, 0x0425},{0, 0x12, 0x0fa4},{0, 0x1f, 0x042a},{0, 0x10, 0x0616},\
    {0, 0x1f, 0x0278},{0, 0x12, 0x0455},\
    {0, 0x1f, 0x0268},{0, 0x16, 0x0014},{0, 0x1f, 0x0260},{0, 0x14, 0x032e},\
    {0, 0x15, 0x00f0},{0, 0x1f, 0x0261},{0, 0x10, 0x300f},{0, 0x1f, 0x0266},\
    {0, 0x10, 0x0395},{0, 0x13, 0x0395},{0, 0x14, 0x0395},{0, 0x15, 0x0395},\
    {0, 0x1f, 0x0000},{0, 0x1d, 0x0000},\
    };

confcode_rv_t construct_rtl8214fc_0455B_rtl8380_perport[] = {\
  /* Set patch_key & patch_lock */
  {0x1f, 0x0000},{0x1b, 0x8146},{0x1c, 0x5501},{0x1b, 0xb82e},\
  {0x1c, 0x0001},\
  //#nc_patch.0371B.121022.txamp.cfg
  {0x1f, 0x0000},{0x1b, 0xb820},{0x1c, 0x0090},{0x1b, 0xa012},\
  {0x1c, 0x0000},{0x1b, 0xa014},{0x1c, 0x2c04},{0x1c, 0x2c0e},\
  {0x1c, 0x2c12},{0x1c, 0x2c13},{0x1c, 0xcc00},{0x1c, 0x1c08},\
  {0x1c, 0xd502},{0x1c, 0x2359},{0x1c, 0xce20},{0x1c, 0xcf01},\
  {0x1c, 0xd705},{0x1c, 0x4005},{0x1c, 0xcf02},{0x1c, 0x0800},\
  {0x1c, 0x15b7},{0x1c, 0xcc02},{0x1c, 0x1c08},{0x1c, 0x2286},\
  {0x1c, 0x2894},{0x1b, 0xa01a},{0x1c, 0x0000},{0x1b, 0xa006},\
  {0x1c, 0x0fff},{0x1b, 0xa004},{0x1c, 0x0008},{0x1b, 0xa002},\
  {0x1c, 0x0285},{0x1b, 0xa000},{0x1c, 0x7358},{0x1b, 0xb820},\
  {0x1c, 0x0010},\
  //#uc_patch.0371B.121022.txamp.cfg
  {0x1f, 0x0000},{0x1b, 0x8368},{0x1c, 0xaf83},{0x1c, 0x74af},\
  {0x1c, 0x838c},{0x1c, 0xaf83},{0x1c, 0x95af},{0x1c, 0x8398},\
  {0x1c, 0x0719},{0x1c, 0xd907},{0x1c, 0xbf3a},{0x1c, 0x2f02},\
  {0x1c, 0x373c},{0x1c, 0x0719},{0x1c, 0xd907},{0x1c, 0xbf3a},\
  {0x1c, 0x2c02},{0x1c, 0x373c},{0x1c, 0x07af},{0x1c, 0x0e1a},\
  {0x1c, 0x0204},{0x1c, 0x9102},{0x1c, 0x843e},{0x1c, 0xaf04},\
  {0x1c, 0x1baf},{0x1c, 0x8395},{0x1c, 0x0106},{0x1c, 0xe081},\
  {0x1c, 0x48af},{0x1c, 0x329e},{0x1c, 0xee80},{0x1c, 0x127f},\
  {0x1c, 0xee80},{0x1c, 0x1508},{0x1c, 0x0283},{0x1c, 0xe9ee},\
  {0x1c, 0x8073},{0x1c, 0x14ee},{0x1c, 0x807e},{0x1c, 0xffee},\
  {0x1c, 0x8087},{0x1c, 0xffee},{0x1c, 0x8090},{0x1c, 0x55ee},\
  {0x1c, 0x8099},{0x1c, 0x55ee},{0x1c, 0x809a},{0x1c, 0x89ee},\
  {0x1c, 0x809b},{0x1c, 0x11ee},{0x1c, 0x80a0},{0x1c, 0x00ee},\
  {0x1c, 0x80a2},{0x1c, 0xffee},{0x1c, 0x80a3},{0x1c, 0x92ee},\
  {0x1c, 0x80a4},{0x1c, 0x33ee},{0x1c, 0x80a9},{0x1c, 0x22ee},\
  {0x1c, 0x80ab},{0x1c, 0xffee},{0x1c, 0x80b4},{0x1c, 0xffd2},\
  {0x1c, 0x0002},{0x1c, 0x0cdc},{0x1c, 0x04f8},{0x1c, 0xf9fa},\
  {0x1c, 0xcefa},{0x1c, 0xef69},{0x1c, 0xfbbf},{0x1c, 0x8414},\
  {0x1c, 0xd700},{0x1c, 0x04d8},{0x1c, 0x19d9},{0x1c, 0x19da},\
  {0x1c, 0x19db},{0x1c, 0x1907},{0x1c, 0xef95},{0x1c, 0x0237},\
  {0x1c, 0x3c07},{0x1c, 0x3f00},{0x1c, 0x049f},{0x1c, 0xecff},\
  {0x1c, 0xef96},{0x1c, 0xfec6},{0x1c, 0xfefd},{0x1c, 0xfc04},\
  {0x1c, 0x001a},{0x1c, 0x3b3d},{0x1c, 0xf8f9},{0x1c, 0xfaef},\
  {0x1c, 0x69e0},{0x1c, 0x8015},{0x1c, 0xad23},{0x1c, 0x15bf},\
  {0x1c, 0x3819},{0x1c, 0x0237},{0x1c, 0x7aac},{0x1c, 0x2a0c},\
  {0x1c, 0xac29},{0x1c, 0x04d2},{0x1c, 0x01ae},{0x1c, 0x02d2},\
  {0x1c, 0x0302},{0x1c, 0x0cdc},{0x1c, 0xef96},{0x1c, 0xfefd},\
  {0x1c, 0xfc04},{0x1c, 0xf8fa},{0x1c, 0xef69},{0x1c, 0xe082},\
  {0x1c, 0x3cac},{0x1c, 0x2511},{0x1c, 0xe082},{0x1c, 0x31ad},\
  {0x1c, 0x251b},{0x1c, 0x0284},{0x1c, 0x18d1},{0x1c, 0x01bf},\
  {0x1c, 0x382b},{0x1c, 0x0237},{0x1c, 0x3ce0},{0x1c, 0x823c},\
  {0x1c, 0xf625},{0x1c, 0xe482},{0x1c, 0x3ce0},{0x1c, 0x8231},\
  {0x1c, 0xf625},{0x1c, 0xe482},{0x1c, 0x31ef},{0x1c, 0x96fe},\
  {0x1c, 0xfc04},{0x1b, 0xb818},{0x1c, 0x0e19},{0x1b, 0xb81a},\
  {0x1c, 0x0418},{0x1b, 0xb81c},{0x1c, 0xfffd},{0x1b, 0xb81e},\
  {0x1c, 0x329b},{0x1b, 0xb820},{0x1c, 0x001b},\
  /* clear patch_key & patch_lock */
  {0x1f, 0x0000},{0x1b, 0x0000},{0x1c, 0x0000},{0x1f, 0x0b82},\
  {0x17, 0x0000},{0x1f, 0x0000},{0x1b, 0x8146},{0x1c, 0x0000},\
  /* release patch request */
  {0x1f, 0x0b82},{0x10, 0x000b},\
    };

confcode_prv_t construct_rtl8214fc_6276A_rtl8380_perchip[] = {
    {0, 0x1f, 0x0000},{0, 0x1e, 0x0008},{0, 0x1f, 0x0405},{0, 0x14, 0x08ec},\
    {0, 0x1f, 0x0404},{0, 0x17, 0x5359},{0, 0x1f, 0x0424},{0, 0x17, 0x5359},\
    {0, 0x1f, 0x042c},{0, 0x11, 0x4000},{0, 0x12, 0x2020},\
    {0, 0x1f, 0x044c},{0, 0x11, 0x4000},{0, 0x12, 0x2020},\
     /*add for disable s0 ring*/
    {0, 0x1f, 0x040c},{0, 0x11, 0x4000},\
     /* end add 20130206*/
    {0, 0x1f, 0x0464},\
    {0, 0x13, 0x3def},{0, 0x15, 0x4c2a},{0, 0x1f, 0x0460},{0, 0x10, 0x4800},\
    {0, 0x1f, 0x0465},{0, 0x12, 0x40ff},{0, 0x1f, 0x0462},{0, 0x12, 0x9693},\
    {0, 0x13, 0x5308},{0, 0x15, 0x4d58},{0, 0x1f, 0x042E},{0, 0x11, 0x308d},\
    {0, 0x1f, 0x044E},{0, 0x11, 0x308d},\
    {0, 0x1f, 0x0464},{0, 0x17, 0x84f5},{0, 0x17, 0x04f5},\
    {0, 0x1f, 0x042D},{0, 0x11, 0xC015},{0, 0x11, 0xC014},\
    {0, 0x1f, 0x044D},{0, 0x11, 0xC015},{0, 0x11, 0xC014},\
    {0, 0x1f, 0x0000},{0, 0x1e, 0x0000},\

    /*##--Combo Mode Fiber Port Patch
       ## fiber port enable N-way to force link (default is 0x0749) ## update 20130402 */
    {0, 0x1f, 0x0000},{0, 0x1e, 0x0008},{0, 0x1f, 0x0260},\
    {0, 0x13, 0x5820},\
    {0, 0x14, 0x032c},\
    {0, 0x1f, 0x0266},{0, 0x10, 0x0395},{0, 0x13, 0x0395},\
    {0, 0x14, 0x0395},{0, 0x15, 0x0395},{0, 0x1f, 0x0000},{0, 0x1e, 0x0000},\
    {0, 0x1f, 0x0000},{0, 0x1e, 0x0003},{0, 0x1f, 0x0008},{0, 0x17, 0x5359},{0, 0x14, 0x074d},\
    {0, 0x1f, 0x0000},{0, 0x1e, 0x0000},{1, 0x1f, 0x0000},{1, 0x1e, 0x0003},\
    {1, 0x1f, 0x0008},{1, 0x17, 0x5359},{1, 0x14, 0x074d},{1, 0x1f, 0x0000},{1, 0x1e, 0x0000},\
    {2, 0x1f, 0x0000},{2, 0x1e, 0x0003},{2, 0x1f, 0x0008},{2, 0x17, 0x5359},{2, 0x14, 0x074d},\
    {2, 0x1f, 0x0000},{2, 0x1e, 0x0000},{3, 0x1f, 0x0000},{3, 0x1e, 0x0003},\
    {3, 0x1f, 0x0008},{3, 0x17, 0x5359},{3, 0x14, 0x074d},{3, 0x1f, 0x0000},{3, 0x1e, 0x0000},\
};

confcode_rv_t construct_rtl8214fc_6276A_rtl8380_perport[] = {\
};

confcode_prv_t construct_rtl8214fc_6276B_rtl8380_perchip[] =  {\
  {0, 0x1e, 0x0008},{0, 0x1f, 0x0405},{0, 0x14, 0x08ec},{0, 0x1f, 0x0404}, \
  {0, 0x17, 0x5359},{0, 0x1f, 0x0424},{0, 0x17, 0x5359},{0, 0x1f, 0x042c}, \
  {0, 0x11, 0x4000},{0, 0x12, 0x2020},{0, 0x17, 0x34ac},{0, 0x1f, 0x042d}, \
  {0, 0x12, 0x6078},{0, 0x1f, 0x042e},{0, 0x11, 0x2189},{0, 0x1f, 0x044c}, \
  {0, 0x11, 0x4000},{0, 0x12, 0x2020},{0, 0x17, 0x34ac},{0, 0x1f, 0x044d}, \
  {0, 0x12, 0x6078},{0, 0x1f, 0x044e},{0, 0x11, 0x2189},{0, 0x1f, 0x0460}, \
  {0, 0x10, 0x4800},{0, 0x1f, 0x0464},{0, 0x12, 0x1fb0},{0, 0x13, 0x3e0f}, \
  {0, 0x15, 0x202a},{0, 0x16, 0xf072},{0, 0x1f, 0x0465},{0, 0x10, 0x4208}, \
  {0, 0x11, 0x3a08},{0, 0x13, 0x8068},{0, 0x15, 0x29fb},{0, 0x12, 0x4007}, \
  {0, 0x14, 0x619f},{0, 0x1f, 0x0462},{0, 0x10, 0xf206},{0, 0x13, 0x530f}, \
  {0, 0x15, 0x2a58},{0, 0x12, 0x97b3},{0, 0x1f, 0x0464},{0, 0x17, 0x80f5}, \
  {0, 0x17, 0x00f5},{0, 0x1f, 0x042d},{0, 0x11, 0xc015},{0, 0x11, 0xc014}, \
  {0, 0x1f, 0x044d},{0, 0x11, 0xc015},{0, 0x11, 0xc014}, \
  {0, 0x1f, 0x0467},{0, 0x14, 0x143d},{0, 0x14, 0x3c15},{0, 0x14, 0x3c17}, \
  {0, 0x14, 0x0000},{0, 0x1f, 0x0404},{0, 0x13, 0x7146},{0, 0x13, 0x7106}, \
  {0, 0x1f, 0x0424},{0, 0x13, 0x7146},{0, 0x13, 0x7106},{0, 0x1f, 0x0261}, \
  {0, 0x10, 0x6000},{0, 0x10, 0x0000},\
  {0, 0x1f, 0x0260},{0, 0x13, 0x5820},{0, 0x14, 0x032c},{0, 0x1f, 0x0280}, \
  {0, 0x10, 0xf0bb},{0, 0x1f, 0x0266},{0, 0x10, 0x0f95},{0, 0x13, 0x0f95}, \
  {0, 0x14, 0x0f95},{0, 0x15, 0x0f95},{0, 0x1f, 0x0a42},{0, 0x1e, 0x0000}, \
  {0, 0x1e, 0x0003},{0, 0x1f, 0x0008},{0, 0x17, 0x5359},{0, 0x14, 0x974d},\
  {0, 0x1e, 0x0000},{1, 0x1e, 0x0003},{1, 0x1f, 0x0008},{1, 0x17, 0x5359},\
  {1, 0x14, 0x974d},{1, 0x1e, 0x0000},{2, 0x1e, 0x0003},{2, 0x1f, 0x0008},\
  {2, 0x17, 0x5359},{2, 0x14, 0x974d},{2, 0x1e, 0x0000},{3, 0x1e, 0x0003},\
  {3, 0x1f, 0x0008},{3, 0x17, 0x5359},{3, 0x14, 0x974d},{3, 0x1e, 0x0000},\
  {0, 0x1e, 0x0001},{1, 0x1e, 0x0001},{2, 0x1e, 0x0001},{3, 0x1e, 0x0001},\
  {0, 0x00, 0x1340},{1, 0x00, 0x1340},{2, 0x00, 0x1340},{3, 0x00, 0x1340},\
  {0, 0x1e, 0x0003},{1, 0x1e, 0x0003},{2, 0x1e, 0x0003},{3, 0x1e, 0x0003},\
  {0, 0x1f, 0x0000},{1, 0x1f, 0x0000},{2, 0x1f, 0x0000},{3, 0x1f, 0x0000},\
  {0, 0x10, 0x1340},{1, 0x10, 0x1340},{2, 0x10, 0x1340},{3, 0x10, 0x1340},\
  {0, 0x1e, 0x0000},{1, 0x1e, 0x0000},{2, 0x1e, 0x0000},{3, 0x1e, 0x0000},\
  {0, 0x1f, 0x0a42},{1, 0x1f, 0x0a42},{2, 0x1f, 0x0a42},{3, 0x1f, 0x0a42},\
};

confcode_rv_t construct_rtl8214fc_6276B_rtl8380_perport[] =  {\
  {0x1f, 0x0b82},{0x10, 0x0000},{0x1f, 0x0a44},{0x11, 0x0418},\
  {0x1f, 0x0bc0},{0x16, 0x0c00},{0x1f, 0x0a43},{0x11, 0x0043},\
  {0x1f, 0x0000},{0x1b, 0x809a},{0x1c, 0x8933},{0x1b, 0x80a3},\
  {0x1c, 0x9233},{0x1b, 0x80ac},{0x1c, 0xa444},{0x1b, 0x809f},\
  {0x1c, 0x6b20},{0x1b, 0x80a8},{0x1c, 0x6b22},{0x1b, 0x80b1},\
  {0x1c, 0x6b23},\
  /*8214FC fiber down speed patch*/
  {0x1f, 0x0000},{0x1e, 0x0003},{0x1f, 0x0003},{0x15, 0xe47f},\
  {0x1f, 0x0009},{0x15, 0x46f4},{0x1f, 0x0000},{0x1e, 0x0000},\
  /*End 8214FC fiber down speed patch*/
};

confcode_prv_t construct_rtl8214fc_6276B_rtl8380_perchip_savpow[] = {
    /*20210218 add for save 14FC power, do this init after per port PHY init  */
  {0, 0x1e, 0x0001},{1, 0x1e, 0x0001},{2, 0x1e, 0x0001},{3, 0x1e, 0x0001}, \
  {0, 0x1f, 0x0bc0},{1, 0x1f, 0x0bc0},{2, 0x1f, 0x0bc0},{3, 0x1f, 0x0bc0}, \
  {0, 0x12, 0x0000},{1, 0x12, 0x0000},{2, 0x12, 0x0000},{3, 0x12, 0x0000}, \
  {0, 0x13, 0x01d0},{1, 0x13, 0x01d0},{2, 0x13, 0x01d0},{3, 0x13, 0x01d0}, \
  {0, 0x1e, 0x0008},{0, 0x1f, 0x0266},{0, 0x12, 0x000b}, \
};

confcode_prv_t construct_rtl8214fc_6276B_rtl8380_perchip_savpow_2[] = {
  /* wait 20ms */
  {0, 0x1f, 0x0261}, \
  {0, 0x10, 0x00ff}, \
};

confcode_prv_t construct_rtl8214fc_6276B_rtl8380_perchip_savpow_3[] = {
  /* wait 20ms */
  {0, 0x1e, 0x0008},{0, 0x1f, 0x0266},{0, 0x12, 0x0003},{0, 0x1e, 0x0001}, \
  {0, 0x1f, 0x0bc0},{0, 0x13, 0x01c0},{1, 0x13, 0x01c0},{2, 0x13, 0x01c0}, \
  {3, 0x13, 0x01c0},{0, 0x1e, 0x0000},{1, 0x1e, 0x0000},{2, 0x1e, 0x0000}, \
  {3, 0x1e, 0x0000}, \
};

#endif
#if defined(CONFIG_SDK_RTL9300)

confcode_prv_t construct_rtl8214fc_6276B_rtl9300_perchip[] =  {\
  {0, 0x1e, 0x0008},{0, 0x1f, 0x0405},{0, 0x14, 0x08ec},{0, 0x1f, 0x0404}, \
  {0, 0x17, 0x5359},{0, 0x1f, 0x0424},{0, 0x17, 0x5359},{0, 0x1f, 0x042c}, \
  {0, 0x11, 0x4000},{0, 0x12, 0x2020},{0, 0x17, 0x34ac},{0, 0x1f, 0x042d}, \
  {0, 0x12, 0x6078},{0, 0x1f, 0x042e},{0, 0x11, 0x2189},{0, 0x1f, 0x044c}, \
  {0, 0x11, 0x4000},{0, 0x12, 0x2020},{0, 0x17, 0x34ac},{0, 0x1f, 0x044d}, \
  {0, 0x12, 0x6078},{0, 0x1f, 0x044e},{0, 0x11, 0x2189},{0, 0x1f, 0x0460}, \
  {0, 0x10, 0x4800},{0, 0x1f, 0x0464},{0, 0x12, 0x1fb0},{0, 0x13, 0x3e0f}, \
  {0, 0x15, 0x202a},{0, 0x16, 0xf072},{0, 0x1f, 0x0465},{0, 0x10, 0x4208}, \
  {0, 0x11, 0x3a08},{0, 0x13, 0x8068},{0, 0x15, 0x29fb},{0, 0x12, 0x4007}, \
  {0, 0x14, 0x619f},{0, 0x1f, 0x0462},{0, 0x10, 0xf206},{0, 0x13, 0x530f}, \
  {0, 0x15, 0x2a58},{0, 0x12, 0x97b3},{0, 0x1f, 0x0464},{0, 0x17, 0x80f5}, \
  {0, 0x17, 0x00f5},{0, 0x1f, 0x042d},{0, 0x11, 0xc015},{0, 0x11, 0xc014}, \
  {0, 0x1f, 0x044d},{0, 0x11, 0xc015},{0, 0x11, 0xc014}, \
  {0, 0x1f, 0x0467},{0, 0x14, 0x143d},{0, 0x14, 0x3c15},{0, 0x14, 0x3c17}, \
  {0, 0x14, 0x0000},{0, 0x1f, 0x0404},{0, 0x13, 0x7146},{0, 0x13, 0x7106}, \
  {0, 0x1f, 0x0424},{0, 0x13, 0x7146},{0, 0x13, 0x7106},{0, 0x1f, 0x0261}, \
  {0, 0x10, 0x6000},{0, 0x10, 0x0000},\
  {0, 0x1f, 0x0260},{0, 0x13, 0x5820},{0, 0x14, 0x032c},{0, 0x1f, 0x0280}, \
  {0, 0x10, 0xf0bb},{0, 0x1f, 0x0266},{0, 0x10, 0x0f95},{0, 0x13, 0x0f95}, \
  {0, 0x14, 0x0f95},{0, 0x15, 0x0f95},{0, 0x1f, 0x0a42},{0, 0x1e, 0x0000}, \
  {0, 0x1e, 0x0003},{0, 0x1f, 0x0008},{0, 0x17, 0x5359},{0, 0x14, 0x974d},\
  {0, 0x1e, 0x0000},{1, 0x1e, 0x0003},{1, 0x1f, 0x0008},{1, 0x17, 0x5359},\
  {1, 0x14, 0x974d},{1, 0x1e, 0x0000},{2, 0x1e, 0x0003},{2, 0x1f, 0x0008},\
  {2, 0x17, 0x5359},{2, 0x14, 0x974d},{2, 0x1e, 0x0000},{3, 0x1e, 0x0003},\
  {3, 0x1f, 0x0008},{3, 0x17, 0x5359},{3, 0x14, 0x974d},{3, 0x1e, 0x0000},\
  {0, 0x1e, 0x0001},{1, 0x1e, 0x0001},{2, 0x1e, 0x0001},{3, 0x1e, 0x0001},\
  {0, 0x00, 0x1340},{1, 0x00, 0x1340},{2, 0x00, 0x1340},{3, 0x00, 0x1340},\
  {0, 0x1e, 0x0003},{1, 0x1e, 0x0003},{2, 0x1e, 0x0003},{3, 0x1e, 0x0003},\
  {0, 0x1f, 0x0000},{1, 0x1f, 0x0000},{2, 0x1f, 0x0000},{3, 0x1f, 0x0000},\
  {0, 0x10, 0x1340},{1, 0x10, 0x1340},{2, 0x10, 0x1340},{3, 0x10, 0x1340},\
  {0, 0x1e, 0x0000},{1, 0x1e, 0x0000},{2, 0x1e, 0x0000},{3, 0x1e, 0x0000},\
  {0, 0x1f, 0x0a42},{1, 0x1f, 0x0a42},{2, 0x1f, 0x0a42},{3, 0x1f, 0x0a42},\
};

confcode_rv_t construct_rtl8214fc_6276B_rtl9300_perport[] =  {\
  {0x1f, 0x0b82},{0x10, 0x0000},{0x1f, 0x0a44},{0x11, 0x0418},\
  {0x1f, 0x0bc0},{0x16, 0x0c00},{0x1f, 0x0a43},{0x11, 0x0043},\
  {0x1f, 0x0000},{0x1b, 0x809a},{0x1c, 0x8933},{0x1b, 0x80a3},\
  {0x1c, 0x9233},{0x1b, 0x80ac},{0x1c, 0xa444},{0x1b, 0x809f},\
  {0x1c, 0x6b20},{0x1b, 0x80a8},{0x1c, 0x6b22},{0x1b, 0x80b1},\
  {0x1c, 0x6b23},\
  /*8214FC fiber down speed patch*/
  {0x1f, 0x0000},{0x1e, 0x0003},{0x1f, 0x0003},{0x15, 0xe47f},\
  {0x1f, 0x0009},{0x15, 0x46f4},{0x1f, 0x0000},{0x1e, 0x0000},\
  /*End 8214FC fiber down speed patch*/
};

confcode_prv_t construct_rtl8214fc_6276B_rtl9300_perchip_savpow[] =  {\
    /*20200218 add for save 14FC power, do this init after per port PHY init  */
  {0, 0x1e, 0x0001},{1, 0x1e, 0x0001},{2, 0x1e, 0x0001},{3, 0x1e, 0x0001}, \
  {0, 0x1f, 0x0bc0},{1, 0x1f, 0x0bc0},{2, 0x1f, 0x0bc0},{3, 0x1f, 0x0bc0}, \
  {0, 0x12, 0x0000},{1, 0x12, 0x0000},{2, 0x12, 0x0000},{3, 0x12, 0x0000}, \
  {0, 0x13, 0x01d0},{1, 0x13, 0x01d0},{2, 0x13, 0x01d0},{3, 0x13, 0x01d0}, \
  {0, 0x1e, 0x0008},{0, 0x1f, 0x0266},{0, 0x12, 0x000b},\
};

confcode_prv_t construct_rtl8214fc_6276B_rtl9300_perchip_savpow_2[] =  {\
    /* wait 20ms */
  {0, 0x1f, 0x0261}, \
  {0, 0x10, 0x00ff}, \
};

confcode_prv_t construct_rtl8214fc_6276B_rtl9300_perchip_savpow_3[] =  {\
    /* wait 20ms */
  {0, 0x1e, 0x0008},{0, 0x1f, 0x0266},{0, 0x12, 0x0003},{0, 0x1e, 0x0001}, \
  {0, 0x1f, 0x0bc0},{0, 0x13, 0x01c0},{1, 0x13, 0x01c0},{2, 0x13, 0x01c0}, \
  {3, 0x13, 0x01c0},{0, 0x1e, 0x0000},{1, 0x1e, 0x0000},{2, 0x1e, 0x0000}, \
  {3, 0x1e, 0x0000}, \
};

#endif

#if defined(CONFIG_SDK_RTL8214FC)
confcode_rv_t construct_rtl8214fc_basic_sds_perchip[] = {
  {0x1e, 0x0008},
  {0x1f, 0x0405}, /* QGMII */
  {0x14, 0x08EC},
  {0x1f, 0x0404},
  {0x17, 0x5359},
  {0x1f, 0x0424},
  {0x17, 0x5359},
  {0x1f, 0x042C},
  {0x11, 0x4000},
  {0x12, 0x2020},
  {0x1f, 0x042D},
  {0x11, 0xC014},
  {0x12, 0x6078},
  {0x1f, 0x042E},
  {0x11, 0x218F},

  {0x1f, 0x044C},
  {0x11, 0x4000},
  {0x12, 0x2020},
  {0x1f, 0x044D},
  {0x11, 0xC014},
  {0x12, 0x6078},
  {0x1f, 0x044E},
  {0x11, 0x218F},

  {0x1f, 0x0460},
  {0x10, 0x4800},
  {0x1f, 0x0464},
  {0x12, 0x1fe0},
  {0x13, 0x3e30},
  {0x15, 0x202a},
  {0x16, 0xf072},
  {0x1f, 0x0465},
  {0x10, 0x4208},
  {0x11, 0x3a08},
  {0x12, 0x4007},
  {0x14, 0x619f},
  {0x1f, 0x0462},
  {0x10, 0xf206},
  {0x13, 0x5308},
  {0x15, 0x6a58},
  {0x12, 0x96b3},
  {0x1f, 0x0464},
  {0x17, 0x80f5},
  {0x17, 0x00f5},
  {0x1f, 0x042D},
  {0x11, 0xC015},
  {0x11, 0xC014},
  {0x1f, 0x0261},
  {0x10, 0x6000},
  {0x10, 0x0000},
  {0x1f, 0x0404},
  {0x13, 0x7146},
  {0x13, 0x7106},
  {0x1f, 0x0424},
  {0x13, 0x7146},
  {0x13, 0x7106},

  {0x1f, 0x0260}, /* Fiber */
  {0x13, 0x4020}, /* b12-8 = Start PHY address. do not modified it! */
  {0x14, 0x032C},
  {0x1f, 0x0280},
  {0x10, 0xF0BB},
  {0x1f, 0x0266},
  {0x10, 0x0f95},
  {0x13, 0x0f95},
  {0x14, 0x0f95},
  {0x15, 0x0f95},
  {0x1e, 0x0000},
};

confcode_rv_t construct_rtl8214fc_basic_fiber_perport[] = {
  {0x1e, 0x0003},
  {0x1f, 0x0008},
  {0x17, 0x5359},
  {0x14, 0x074D},
  {0x11, 0x0f00},
  {0x1f, 0x0000},
  {0x10, 0x1340},
  {0x1e, 0x0000},
  {0x1f, 0x0a42},
};

confcode_rv_t construct_rtl8214fc_basic_gphy_perport[] = {
  {0x1e, 0x0001},
  {0x1f, 0x0a44},
  {0x11, 0x0418},
  {0x1f, 0x0bc0},
  {0x16, 0x0c00},
  {0x1f, 0x0a43},
  {0x11, 0x0043},
  {0x1b, 0x809a},
  {0x1c, 0x8933},
  {0x1b, 0x80a3},
  {0x1c, 0x9233},
  {0x1b, 0x80ac},
  {0x1c, 0xa444},
  {0x1b, 0x809f},
  {0x1c, 0x6b20},
  {0x1b, 0x80a8},
  {0x1c, 0x6b22},
  {0x1b, 0x80b1},
  {0x1c, 0x6b23},
  {0x1e, 0x0000},
};

confcode_rv_t construct_rtl8214fc_basic_uc_perport[] = {
  {0x1f, 0x0a43},
  {0x1b, 0xB820},
  {0x1c, 0x0090},
  {0x1b, 0xA012},
  {0x1c, 0x0000},
  {0x1b, 0xA014},
  {0x1c, 0x2c04},
  {0x1c, 0x2c3f},
  {0x1c, 0x2c41},
  {0x1c, 0x2c45},
  {0x1c, 0xd70c},
  {0x1c, 0x6111},
  {0x1c, 0x6090},
  {0x1c, 0xd075},
  {0x1c, 0xd188},
  {0x1c, 0x2c13},
  {0x1c, 0xd076},
  {0x1c, 0xd188},
  {0x1c, 0x2c13},
  {0x1c, 0x6090},
  {0x1c, 0xd077},
  {0x1c, 0xd188},
  {0x1c, 0x2c13},
  {0x1c, 0xd078},
  {0x1c, 0xd188},
  {0x1c, 0xd03b},
  {0x1c, 0xd198},
  {0x1c, 0xd700},
  {0x1c, 0x3220},
  {0x1c, 0x3c3e},
  {0x1c, 0xd024},
  {0x1c, 0xd18b},
  {0x1c, 0xd012},
  {0x1c, 0xd19b},
  {0x1c, 0x3231},
  {0x1c, 0x4c3e},
  {0x1c, 0xd70c},
  {0x1c, 0x6212},
  {0x1c, 0x6111},
  {0x1c, 0x6090},
  {0x1c, 0xd07a},
  {0x1c, 0xd189},
  {0x1c, 0x2c3d},
  {0x1c, 0xd07b},
  {0x1c, 0xd189},
  {0x1c, 0x2c3d},
  {0x1c, 0x6090},
  {0x1c, 0xd07c},
  {0x1c, 0xd189},
  {0x1c, 0x2c3d},
  {0x1c, 0xd07d},
  {0x1c, 0xd189},
  {0x1c, 0x2c3d},
  {0x1c, 0x6111},
  {0x1c, 0x6090},
  {0x1c, 0xd07e},
  {0x1c, 0xd189},
  {0x1c, 0x2c3d},
  {0x1c, 0xd07f},
  {0x1c, 0xd189},
  {0x1c, 0x2c3d},
  {0x1c, 0x6090},
  {0x1c, 0xd080},
  {0x1c, 0xd189},
  {0x1c, 0x2c3d},
  {0x1c, 0xd081},
  {0x1c, 0xd189},
  {0x1c, 0x2511},
  {0x1c, 0x251b},
  {0x1c, 0xd090},
  {0x1c, 0x2276},
  {0x1c, 0xc040},
  {0x1c, 0x4000},
  {0x1c, 0xa20c},
  {0x1c, 0x2ab0},
  {0x1b, 0xA01A},
  {0x1c, 0x0000},
  {0x1b, 0xA006},
  {0x1c, 0x0fff},
  {0x1b, 0xA004},
  {0x1c, 0x0aaf},
  {0x1b, 0xA002},
  {0x1c, 0x0275},
  {0x1b, 0xA000},
  {0x1c, 0x7504},
  {0x1b, 0xB820},
  {0x1c, 0x0010},

  {0x1b, 0x8373},
  {0x1c, 0xaf83},
  {0x1c, 0x7faf},
  {0x1c, 0x849c},
  {0x1c, 0xaf84},
  {0x1c, 0x9caf},
  {0x1c, 0x849c},
  {0x1c, 0xe182},
  {0x1c, 0x52ac},
  {0x1c, 0x2b12},
  {0x1c, 0xe082},
  {0x1c, 0x47ac},
  {0x1c, 0x2303},
  {0x1c, 0xaf0c},
  {0x1c, 0x0f02},
  {0x1c, 0x83a0},
  {0x1c, 0x0213},
  {0x1c, 0xffaf},
  {0x1c, 0x0c0f},
  {0x1c, 0x0284},
  {0x1c, 0x4d02},
  {0x1c, 0x1658},
  {0x1c, 0xaf0c},
  {0x1c, 0x0ff8},
  {0x1c, 0xf9fa},
  {0x1c, 0xef69},
  {0x1c, 0xfbe0},
  {0x1c, 0x8016},
  {0x1c, 0xad23},
  {0x1c, 0x02ae},
  {0x1c, 0x03af},
  {0x1c, 0x8446},
  {0x1c, 0xbf3a},
  {0x1c, 0x7402},
  {0x1c, 0x3972},
  {0x1c, 0xad28},
  {0x1c, 0xf4e0},
  {0x1c, 0x878a},
  {0x1c, 0xac20},
  {0x1c, 0xeef7},
  {0x1c, 0x20e4},
  {0x1c, 0x878a},
  {0x1c, 0xe087},
  {0x1c, 0x82e1},
  {0x1c, 0x8783},
  {0x1c, 0x1f01},
  {0x1c, 0x9e77},
  {0x1c, 0xee87},
  {0x1c, 0x8800},
  {0x1c, 0xee87},
  {0x1c, 0x891f},
  {0x1c, 0xd200},
  {0x1c, 0x0284},
  {0x1c, 0x6ebf},
  {0x1c, 0x8784},
  {0x1c, 0x1a92},
  {0x1c, 0xdde0},
  {0x1c, 0x8788},
  {0x1c, 0x1b01},
  {0x1c, 0xab03},
  {0x1c, 0xe587},
  {0x1c, 0x88e0},
  {0x1c, 0x8789},
  {0x1c, 0x1b01},
  {0x1c, 0xaa03},
  {0x1c, 0xe587},
  {0x1c, 0x8912},
  {0x1c, 0xa204},
  {0x1c, 0xdfe0},
  {0x1c, 0x8780},
  {0x1c, 0xe187},
  {0x1c, 0x881b},
  {0x1c, 0x01ab},
  {0x1c, 0x42e0},
  {0x1c, 0x8789},
  {0x1c, 0xe187},
  {0x1c, 0x881b},
  {0x1c, 0x10e0},
  {0x1c, 0x8781},
  {0x1c, 0x1b01},
  {0x1c, 0xab33},
  {0x1c, 0xd100},
  {0x1c, 0xbf84},
  {0x1c, 0xa802},
  {0x1c, 0x3934},
  {0x1c, 0xd101},
  {0x1c, 0xbf84},
  {0x1c, 0xab02},
  {0x1c, 0x3934},
  {0x1c, 0xbf87},
  {0x1c, 0x90e0},
  {0x1c, 0x8783},
  {0x1c, 0x0c02},
  {0x1c, 0x1a90},
  {0x1c, 0xef79},
  {0x1c, 0xbf87},
  {0x1c, 0x84d0},
  {0x1c, 0x0402},
  {0x1c, 0x392d},
  {0x1c, 0xe087},
  {0x1c, 0x8310},
  {0x1c, 0xe487},
  {0x1c, 0x83e0},
  {0x1c, 0x878a},
  {0x1c, 0xf721},
  {0x1c, 0xe487},
  {0x1c, 0x8aff},
  {0x1c, 0xef96},
  {0x1c, 0xfefd},
  {0x1c, 0xfc04},
  {0x1c, 0xf8fa},
  {0x1c, 0xef69},
  {0x1c, 0xe080},
  {0x1c, 0x16ad},
  {0x1c, 0x2312},
  {0x1c, 0xe087},
  {0x1c, 0x8aee},
  {0x1c, 0x878a},
  {0x1c, 0x00ad},
  {0x1c, 0x2108},
  {0x1c, 0xd100},
  {0x1c, 0xbf84},
  {0x1c, 0xab02},
  {0x1c, 0x3934},
  {0x1c, 0xef96},
  {0x1c, 0xfefc},
  {0x1c, 0x04f9},
  {0x1c, 0xfaef},
  {0x1c, 0x69a2},
  {0x1c, 0x0005},
  {0x1c, 0xbf84},
  {0x1c, 0x9cae},
  {0x1c, 0x1aa2},
  {0x1c, 0x0105},
  {0x1c, 0xbf84},
  {0x1c, 0x9fae},
  {0x1c, 0x12a2},
  {0x1c, 0x0205},
  {0x1c, 0xbf84},
  {0x1c, 0xa2ae},
  {0x1c, 0x0aa2},
  {0x1c, 0x0305},
  {0x1c, 0xbf84},
  {0x1c, 0xa5ae},
  {0x1c, 0x02ae},
  {0x1c, 0x0302},
  {0x1c, 0x3972},
  {0x1c, 0xef96},
  {0x1c, 0xfefd},
  {0x1c, 0x0495},
  {0x1c, 0xa8e8},
  {0x1c, 0x95a9},
  {0x1c, 0xe895},
  {0x1c, 0xaae8},
  {0x1c, 0x95ab},
  {0x1c, 0xe8bb},
  {0x1c, 0xa502},
  {0x1c, 0xcca5},
  {0x1c, 0x0200},
  {0x1b, 0x8780},
  {0x1c, 0x160c},
  {0x1b, 0x8782},
  {0x1c, 0x0500},
  {0x1b, 0xb818},
  {0x1c, 0x0bf9},
  {0x1b, 0xb81a},
  {0x1c, 0xfffd},
  {0x1b, 0xb81c},
  {0x1c, 0xfffd},
  {0x1b, 0xb81e},
  {0x1c, 0xfffd},
  {0x1b, 0xb820},
  {0x1c, 0x0011},
  {0x1f, 0x0000},

  {0x1b, 0x0000},
  {0x1c, 0x0000},
  {0x1f, 0x0b82},
  {0x17, 0x0000},
  {0x1f, 0x0a43},
  {0x1b, 0x8146},
  {0x1c, 0x0000},
};
#endif


