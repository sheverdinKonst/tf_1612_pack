//
// Created by sheverdin on 4/3/24.
//

//typedef union
//{
//    struct
//    {
//        uint32_t REG_INTSTAT                 : 1
//        uint32_t REG_INTMASK                 : 1
//        uint32_t REG_HW_VERS                 : 8
//        uint32_t REG_SW_VERS                 : 8 //18
//        uint32_t REG_TAMPER,                 : 1
//        uint32_t REG_SENSOR1,                : 1
//        uint32_t REG_SENSOR2,                : 1
//        uint32_t REG_RELAY1,                 : 1
//        uint32_t REG_DEFAULT_BUTTON,         : 1
//        uint32_t REG_DEFAULT_LED,            : 1 //24
//        uint32_t REG_DEFAULT_LONG_PRESSED    : 8 // 32 bit
//    }S1;
//
//    struct {
//        uint32_t REG_ADC_CH1                  : 16
//        uint32_t REG_ADC_CH2                  : 16  // 32 bit
//    }s2
//
//    uint8_t i2c_array[8];
//}i2c_param;
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//REG_RPS_CONNECTED,      .lenData = 1,
//REG_RPS_HW_VERS,        .lenData = 1,
//REG_RPS_SW_VERS,        .lenData = 1,
//REG_RPS_VAC,            .lenData = 1,
//REG_RPS_BAT_VOLTAGE,    .lenData = 2,
//REG_RPS_CHRG_VOLTAGE,   .lenData = 2,
//REG_RPS_BAT_CURRENT,    .lenData = 2,
//REG_RPS_TEMPER,         .lenData = 1,
//REG_RPS_LED_STATE,      .lenData = 1,
//REG_RPS_BAT_KEY,        .lenData = 1,
//REG_RPS_CHRG_KEY,       .lenData = 1,
//REG_RPS_REL_STATE,      .lenData = 1,
//REG_RPS_MIN_VOLTAGE,    .lenData = 2,
//REG_RPS_DISCH_VOLTAGE,  .lenData = 2,
//REG_RPS_REMAIN_TIME,    .lenData = 2,
//REG_RPS_TEST_OK,        .lenData = 1,
//REG_RPS_CPU_ID,         .lenData = 2,
//REG_RPS_LTC4151_OK,     .lenData = 1,
//REG_RPS_ADC_BAT_VOLT,   .lenData = 2,
//REG_RPS_ADC_BAT_CURR,   .lenData = 2,
//REG_RPS_TEST_MODE,      .lenData = 1,
//
//REG_SHT_CONNECTED,    .lenData = 1,
//REG_SHT_TYPE,         .lenData = 10,
//REG_SHT_TEMPERATURE,  .lenData = 2,
//REG_SHT_HUMIDITY,     .lenData = 1,
//
//
//REG_SFP1_PRESENT,     .lenData = 1
//REG_SFP1_LOS,         .lenData = 1
//REG_SFP1_VENDOR,      .lenData = 1
//REG_SFP1_VENDOR_OUI,  .lenData = 3
//REG_SFP1_VENDOR_PN,   .lenData = 1
//REG_SFP1_VENDOR_REV,  .lenData = 4
//REG_SFP1_IDENTIFIER,  .lenData = 4
//REG_SFP1_CONNECTOR,   .lenData = 3
//REG_SFP1_TYPE,        .lenData = 1
//REG_SFP1_LINK_LEN,    .lenData = 2
//REG_SFP1_FIBER_TEC,   .lenData = 3
//REG_SFP1_MEDIA,       .lenData = 3
//REG_SFP1_SPEED,       .lenData = 1
//REG_SFP1_ENCODING,    .lenData = 1
//REG_SFP1_WAVELEN,     .lenData = 2
//REG_SFP1_NBR,         .lenData = 2
//REG_SFP1_LEN9,        .lenData = 2
//REG_SFP1_LEN50,       .lenData = 2
//REG_SFP1_LEN62,       .lenData = 2
//REG_SFP1_LENC,        .lenData = 2
//REG_SFP1_TEMPER,      .lenData = 2
//REG_SFP1_VOLTAGE,     .lenData = 2
//REG_SFP1_CURRENT,     .lenData = 2
//REG_SFP1_TX_BIAS,     .lenData = 2
//REG_SFP1_TX_POWER,    .lenData = 2
//REG_SFP1_RX_POWER,    .lenData = 2
//
//REG_SFP2_PRESENT,      .lenData = 1,
//REG_SFP2_LOS,          .lenData = 1,
//REG_SFP2_VENDOR,       .lenData = 16
//REG_SFP2_VENDOR_OUI,   .lenData = 3,
//REG_SFP2_VENDOR_PN,    .lenData = 16
//REG_SFP2_VENDOR_REV,   .lenData = 4,
//REG_SFP2_IDENTIFIER,   .lenData = 4,
//REG_SFP2_CONNECTOR,    .lenData = 30
//REG_SFP2_TYPE,         .lenData = 16
//REG_SFP2_LINK_LEN,     .lenData = 25
//REG_SFP2_FIBER_TEC,    .lenData = 32
//REG_SFP2_MEDIA,        .lenData = 32
//REG_SFP2_SPEED,        .lenData = 16
//REG_SFP2_ENCODING,     .lenData = 16
//REG_SFP2_WAVELEN,      .lenData = 2,
//REG_SFP2_NBR,          .lenData = 2,
//REG_SFP2_LEN9,         .lenData = 2,
//REG_SFP2_LEN50,        .lenData = 2,
//REG_SFP2_LEN62,        .lenData = 2,
//REG_SFP2_LENC,         .lenData = 2,
//REG_SFP2_TEMPER,       .lenData = 2,
//REG_SFP2_VOLTAGE,      .lenData = 2,
//REG_SFP2_CURRENT,      .lenData = 2,
//REG_SFP2_TX_BIAS,      .lenData = 2,
//REG_SFP2_TX_POWER,      .lenData = 2
//REG_SFP2_RX_POWER,      .lenData = 2
//
//REG_RTC_STATUS,    .lenData = 1,
//REG_RTC_YEAR,      .lenData = 1,
//REG_RTC_MONTH,     .lenData = 1,
//REG_RTC_DAY,       .lenData = 1,
//REG_RTC_WEEKDAY,   .lenData = 1,
//REG_RTC_HOUR,      .lenData = 1,
//REG_RTC_MINUTE,    .lenData = 1,
//REG_RTC_SECOND,    .lenData = 1,
//
//REG_IDLE128,  .lenData = 0,
//REG_IDLE129,  .lenData = 0,
//
//REG_POE_ID,    .lenData = 1,
//REG_POE_STATE, .lenData = 1,
//REG_POE_BANK,  .lenData = 1,
//REG_POE_MODE,  .lenData = 1,
//
//}i2cParam;
//
//i2c_data_t sock_msgArr[MAX_SENSORS] =
//        {
//                // MAIN
//                {.addr = REG_INTSTAT,   .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_INTMASK,   .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_HW_VERS,   .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_SW_VERS,   .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_ADC_CH1,   .lenData = 2, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = critical_REG_ADC_CH1 },
//                {.addr = REG_ADC_CH2,   .lenData = 2, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = critical_REG_ADC_CH2 },
//                {.addr = REG_ADC_CH3,   .lenData = 2, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = critical_REG_ADC_CH3 },
//
//                {.addr = REG_IDLE7,     .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_IDLE8,     .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_IDLE9,     .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//
//                // SENSOR
//                {.addr = REG_TAMPER,               .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = critical, .criticalHandler = critical_REG_TAMPER},
//                {.addr = REG_SENSOR1,              .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,  .criticalHandler = critical_REG_SENSOR1},
//                {.addr = REG_SENSOR2,              .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,  .criticalHandler = critical_REG_SENSOR2},
//                {.addr = REG_RELAY1,               .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,  .criticalHandler = critical_REG_RELAY1},
//                {.addr = REG_DEFAULT_BUTTON,       .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,  .criticalHandler = NULL},
//                {.addr = REG_DEFAULT_LED,          .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,  .criticalHandler = NULL},
//                {.addr = REG_DEFAULT_LONG_PRESSED, .lenData = 2, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = critical, .criticalHandler = critical_REG_DEFAULT_LONG_PRESSED},
//
//                {.addr = REG_IDLE17,    .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_IDLE18,    .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_IDLE19,    .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//
//                // UPS
//                {.addr = REG_RPS_CONNECTED,      .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,      .criticalHandler = NULL     },
//                {.addr = REG_RPS_HW_VERS,        .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,      .criticalHandler = NULL       },
//                {.addr = REG_RPS_SW_VERS,        .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,      .criticalHandler = NULL       },
//                {.addr = REG_RPS_VAC,            .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,      .criticalHandler = critical_REG_RPS_VAC           },
//                {.addr = REG_RPS_BAT_VOLTAGE,    .lenData = 2, .type = TYPE_PSEUDO_FLOAT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,      .criticalHandler = critical_REG_RPS_BAT_VOLTAGE   },
//                {.addr = REG_RPS_CHRG_VOLTAGE,   .lenData = 2, .type = TYPE_PSEUDO_FLOAT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,      .criticalHandler = critical_REG_RPS_CHRG_VOLTAGE  },
//                {.addr = REG_RPS_BAT_CURRENT,    .lenData = 2, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,      .criticalHandler = critical_REG_RPS_BAT_CURRENT   },
//                {.addr = REG_RPS_TEMPER,         .lenData = 1, .type = TYPE_UINT,         .value = 123, .opcode = I2C_OPCODE_IDLE, .isCritical = critical,   .criticalHandler = critical_REG_RPS_TEMPER        },
//                {.addr = REG_RPS_LED_STATE,      .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,      .criticalHandler = NULL     },
//                {.addr = REG_RPS_BAT_KEY,        .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,      .criticalHandler = NULL },
//                {.addr = REG_RPS_CHRG_KEY,       .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,      .criticalHandler = NULL },
//                {.addr = REG_RPS_REL_STATE,      .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,      .criticalHandler = NULL },
//                {.addr = REG_RPS_MIN_VOLTAGE,    .lenData = 2, .type = TYPE_PSEUDO_FLOAT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,      .criticalHandler = NULL },
//                {.addr = REG_RPS_DISCH_VOLTAGE,  .lenData = 2, .type = TYPE_PSEUDO_FLOAT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,      .criticalHandler = NULL },
//                {.addr = REG_RPS_REMAIN_TIME,    .lenData = 2, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = critical,     .criticalHandler = NULL },
//                {.addr = REG_RPS_TEST_OK,        .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,      .criticalHandler = NULL },
//                {.addr = REG_RPS_CPU_ID,         .lenData = 2, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,      .criticalHandler = NULL },
//                {.addr = REG_RPS_LTC4151_OK,     .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,      .criticalHandler = NULL },
//                {.addr = REG_RPS_ADC_BAT_VOLT,   .lenData = 2, .type = TYPE_PSEUDO_FLOAT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,      .criticalHandler = NULL },
//                {.addr = REG_RPS_ADC_BAT_CURR,   .lenData = 2, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,      .criticalHandler = NULL },
//                {.addr = REG_RPS_TEST_MODE,      .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,      .criticalHandler = NULL },
//
//                {.addr = REG_IDLE41,    .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_IDLE42,    .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_IDLE43,    .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_IDLE44,    .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_IDLE45,    .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_IDLE46,    .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_IDLE47,    .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_IDLE48,    .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_IDLE49,    .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//
//                // SHT
//                {.addr = REG_SHT_CONNECTED,    .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,  .criticalHandler = critical_REG_SHT_CONNECTED   },
//                {.addr = REG_SHT_TYPE,         .lenData = 10, .type = TYPE_UINT,        .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,  .criticalHandler = critical_REG_SHT_TYPE        },
//                {.addr = REG_SHT_TEMPERATURE,  .lenData = 2, .type = TYPE_PSEUDO_FLOAT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = critical, .criticalHandler = critical_REG_SHT_TEMPERATURE },
//                {.addr = REG_SHT_HUMIDITY,     .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,  .criticalHandler = critical_REG_SHT_HUMIDITY    },
//
//                {.addr = REG_IDLE54,    .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_IDLE55,    .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_IDLE56,    .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_IDLE57,    .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_IDLE58,    .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_IDLE59,    .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//
//                // SPF_1
//                {.addr = REG_SFP1_PRESENT,     .lenData = 1,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = critical_REG_SFP1_PRESENT },
//                {.addr = REG_SFP1_LOS,         .lenData = 1,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = critical_REG_SFP1_LOS     },
//                {.addr = REG_SFP1_VENDOR,      .lenData = 16, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_SFP1_VENDOR_OUI,  .lenData = 3,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_SFP1_VENDOR_PN,   .lenData = 16, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_SFP1_VENDOR_REV,  .lenData = 4,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_SFP1_IDENTIFIER,  .lenData = 4,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_SFP1_CONNECTOR,   .lenData = 30, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_SFP1_TYPE,        .lenData = 16, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_SFP1_LINK_LEN,    .lenData = 25, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_SFP1_FIBER_TEC,   .lenData = 32, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_SFP1_MEDIA,       .lenData = 32, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_SFP1_SPEED,       .lenData = 16, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_SFP1_ENCODING,    .lenData = 16, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_SFP1_WAVELEN,     .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_SFP1_NBR,         .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_SFP1_LEN9,        .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_SFP1_LEN50,       .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_SFP1_LEN62,       .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_SFP1_LENC,        .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_SFP1_TEMPER,      .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = critical_REG_SFP1_TEMPER   },
//                {.addr = REG_SFP1_VOLTAGE,     .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = critical_REG_SFP1_VOLTAGE  },
//                {.addr = REG_SFP1_CURRENT,     .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = critical_REG_SFP1_CURRENT  },
//                {.addr = REG_SFP1_TX_BIAS,     .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = critical_REG_SFP1_TX_BIAS  },
//                {.addr = REG_SFP1_TX_POWER,    .lenData = 2, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = critical_REG_SFP1_TX_POWER },
//                {.addr = REG_SFP1_RX_POWER,    .lenData = 2, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = critical_REG_SFP1_RX_POWER },
//
//                {.addr = REG_IDLE86,    .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_IDLE87,    .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_IDLE88,    .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_IDLE89,    .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//
//                //SPF_2
//                {.addr = REG_SFP2_PRESENT,      .lenData = 1,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler =  critical_REG_SFP2_PRESENT },
//                {.addr = REG_SFP2_LOS,          .lenData = 1,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler =  critical_REG_SFP2_LOS     },
//                {.addr = REG_SFP2_VENDOR,       .lenData = 16, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL  },
//                {.addr = REG_SFP2_VENDOR_OUI,   .lenData = 3,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL  },
//                {.addr = REG_SFP2_VENDOR_PN,    .lenData = 16, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL  },
//                {.addr = REG_SFP2_VENDOR_REV,   .lenData = 4,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL  },
//                {.addr = REG_SFP2_IDENTIFIER,   .lenData = 4,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL  },
//                {.addr = REG_SFP2_CONNECTOR,    .lenData = 30, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL  },
//                {.addr = REG_SFP2_TYPE,         .lenData = 16, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL  },
//                {.addr = REG_SFP2_LINK_LEN,     .lenData = 25, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL  },
//                {.addr = REG_SFP2_FIBER_TEC,    .lenData = 32, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,  .criticalHandler = NULL },
//                {.addr = REG_SFP2_MEDIA,        .lenData = 32, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,  .criticalHandler = NULL },
//                {.addr = REG_SFP2_SPEED,        .lenData = 16, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,  .criticalHandler = NULL },
//                {.addr = REG_SFP2_ENCODING,     .lenData = 16, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,  .criticalHandler = NULL },
//                {.addr = REG_SFP2_WAVELEN,      .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,  .criticalHandler = NULL },
//                {.addr = REG_SFP2_NBR,          .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,  .criticalHandler = NULL },
//                {.addr = REG_SFP2_LEN9,         .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,  .criticalHandler = NULL },
//                {.addr = REG_SFP2_LEN50,        .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,  .criticalHandler = NULL },
//                {.addr = REG_SFP2_LEN62,        .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,  .criticalHandler = NULL },
//                {.addr = REG_SFP2_LENC,         .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,  .criticalHandler = NULL },
//                {.addr = REG_SFP2_TEMPER,       .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,  .criticalHandler = critical_REG_SFP2_TEMPER   },
//                {.addr = REG_SFP2_VOLTAGE,      .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,  .criticalHandler = critical_REG_SFP2_VOLTAGE  },
//                {.addr = REG_SFP2_CURRENT,      .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,  .criticalHandler = critical_REG_SFP2_CURRENT  },
//                {.addr = REG_SFP2_TX_BIAS,      .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,  .criticalHandler = critical_REG_SFP2_TX_BIAS  },
//                {.addr = REG_SFP2_TX_POWER,      .lenData = 2, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,  .criticalHandler = critical_REG_SFP2_TX_POWER },
//                {.addr = REG_SFP2_RX_POWER,      .lenData = 2, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular,  .criticalHandler = critical_REG_SFP2_RX_POWER },
//
//                {.addr = REG_IDLE116,   .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_IDLE117,   .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_IDLE118,   .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr = REG_IDLE119,   .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//
//                // RTC
//                {.addr = REG_RTC_STATUS,    .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_RTC_YEAR,      .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_RTC_MONTH,     .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_RTC_DAY,       .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_RTC_WEEKDAY,   .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_RTC_HOUR,      .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_RTC_MINUTE,    .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_RTC_SECOND,    .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//
//                {.addr =  REG_IDLE128,  .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//                {.addr =  REG_IDLE129,  .lenData = 0, .type = TYPE_IDLE, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL},
//
//                // POE
//                {.addr = REG_POE_ID,    .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_POE_STATE, .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_POE_BANK,  .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//                {.addr = REG_POE_MODE,  .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE, .isCritical = regular, .criticalHandler = NULL },
//        };