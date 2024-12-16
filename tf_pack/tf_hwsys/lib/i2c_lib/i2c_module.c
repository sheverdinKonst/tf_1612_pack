//
// Created by sheverdin on 3/19/24.
//

#include "i2c_module.h"
#include "time.h"
#include "syslog.h"
#include "utils_lib/utils_module.h"
#include "events_lib/events_handlers.h"

struct timer hi_prio_timer,low_prio_timer;

static uint8_t i2c_raw_data[1000];
static uint16_t hi_prio_len;
static uint16_t low_prio_len;

static void i2c_parsingData(REGISTER_ADDR addr);

static event_t check_out_of_range(uint32_t value, uint32_t prevValue, uint32_t max, uint32_t min);
//static void i2c_sendError(I2C_ERROR_t *i2cData, REGISTER_ADDR addr, uint32_t value, I2C_EVENT_t i2c_event);

static event_t check_changed_state(uint32_t value, uint32_t preValue, uint32_t state_1, uint32_t state_2);
static uint16_t get_16bitValue(const uint8_t *val1);
static uint32_t get_32bitValue(const uint8_t *val1);

static uint8_t i2c_parse_tlv(uint16_t len);



const char i2c_eventDict[I2C_MAX_EVENT][I2C_ERROR_NAME_MAX_LEN] = {
    "\n",
    "ERR_OVER_MAX\n",
    "ERR_LESS_MIN\n",
    "LONG_PRESSED\n",
    "CHANGED_TO_VAC\n",
    "CHANGED_TO_BAT\n",
    "CHANGED_TO_OPEN\n",
    "CHANGED_TO_CLOSE\n",
    "empty\n",
    "empty\n",
    "CRITICAL_ERROR - RESET\n"
};

i2c_write_t i2c_write;

i2c_data_t sock_msgArr[MAX_SENSORS] =
{
    // MAIN
    {.addr = REG_INTSTAT, .name="INTSTAT",   .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL},
    {.addr = REG_INTMASK, .name="INTMASK",   .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL},
    {.addr = REG_HW_VERS, .name="HW_VERS",   .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL},
    {.addr = REG_SW_VERS, .name="SW_VERS",   .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL},
    {.addr = REG_ADC_CH1, .name="ADC_CH1",   .lenData = 2, .type = TYPE_PSEUDO_FLOAT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = handler_ADC_CH1 },
    {.addr = REG_ADC_CH2, .name="ADC_CH2",   .lenData = 2, .type = TYPE_PSEUDO_FLOAT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = handler_ADC_CH2 },
    {.addr = REG_ADC_CH3, .name="ADC_CH3",   .lenData = 2, .type = TYPE_PSEUDO_FLOAT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = handler_ADC_CH3 },

    // SENSOR
    {.addr = REG_TAMPER,               .name="tamper",               .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = handler_TAMPER},
    {.addr = REG_SENSOR1,              .name="sensor1",              .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = handler_SENSOR1},
    {.addr = REG_SENSOR2,              .name="sensor2",              .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = handler_SENSOR2},
    {.addr = REG_RELAY1,              .name="relay",               .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = handler_RELAY1},
    {.addr = REG_DEFAULT_BUTTON,      .name="DEFAULT_BUTTON",      .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = NULL},
    {.addr = REG_DEFAULT_LED,         .name="DEFAULT_LED",         .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = NULL},
    {.addr = REG_DEFAULT_LONG_PRESSED,.name="DEFAULT_LONG_PRESSED",.lenData = 2, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = handler_DEFAULT_LONG_PRESSED},

    // UPS
    {.addr = REG_RPS_CONNECTED,     .name="upsModeAvalible",  .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE,       .eventsHandler = NULL     },
    {.addr = REG_RPS_HW_VERS,       .name="RPS_HW_VERS",      .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE,       .eventsHandler = NULL       },
    {.addr = REG_RPS_SW_VERS,       .name="RPS_SW_VERS",      .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE,       .eventsHandler = NULL       },
    {.addr = REG_RPS_VAC,           .name="upsPwrSource",     .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE,       .eventsHandler = handler_RPS_VAC           },
    {.addr = REG_RPS_BAT_VOLTAGE,   .name="upsBatteryVoltage",.lenData = 2, .type = TYPE_PSEUDO_FLOAT, .value = 0, .opcode = I2C_OPCODE_IDLE,       .eventsHandler = handler_RPS_BAT_VOLTAGE   },
    {.addr = REG_RPS_CHRG_VOLTAGE,  .name="RPS_CHRG_VOLTAGE", .lenData = 2, .type = TYPE_PSEUDO_FLOAT, .value = 0, .opcode = I2C_OPCODE_IDLE,       .eventsHandler = handler_RPS_CHRG_VOLTAGE  },
    {.addr = REG_RPS_BAT_CURRENT,   .name="RPS_BAT_CURRENT",  .lenData = 2, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE,       .eventsHandler = handler_RPS_BAT_CURRENT   },
    {.addr = REG_RPS_TEMPER,        .name="RPS_TEMPER",       .lenData = 1, .type = TYPE_UINT,         .value = 123, .opcode = I2C_OPCODE_IDLE,     .eventsHandler = handler_RPS_TEMPER        },
    {.addr = REG_RPS_LED_STATE,     .name="RPS_LED_STATE",    .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE,       .eventsHandler = NULL     },
    {.addr = REG_RPS_BAT_KEY,       .name="RPS_BAT_KEY",      .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE,       .eventsHandler = NULL },
    {.addr = REG_RPS_CHRG_KEY,      .name="RPS_CHRG_KEY",     .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE,       .eventsHandler = NULL },
    {.addr = REG_RPS_REL_STATE,     .name="RPS_REL_STATE",    .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE,       .eventsHandler = NULL },
    {.addr = REG_RPS_MIN_VOLTAGE,   .name="RPS_MIN_VOLTAGE",  .lenData = 2, .type = TYPE_PSEUDO_FLOAT, .value = 0, .opcode = I2C_OPCODE_IDLE,       .eventsHandler = NULL },
    {.addr = REG_RPS_DISCH_VOLTAGE, .name="RPS_DISCH_VOLTAGE",.lenData = 2, .type = TYPE_PSEUDO_FLOAT, .value = 0, .opcode = I2C_OPCODE_IDLE,       .eventsHandler = NULL },
    {.addr = REG_RPS_REMAIN_TIME,   .name="upsBatteryTime",   .lenData = 2, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE,       .eventsHandler = NULL },
    {.addr = REG_RPS_TEST_OK,       .name="RPS_TEST_OK",      .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE,       .eventsHandler = NULL },
    {.addr = REG_RPS_CPU_ID,        .name="RPS_CPU_ID",       .lenData = 2, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE,       .eventsHandler = NULL },
    {.addr = REG_RPS_LTC4151_OK,    .name="RPS_LTC4151_OK",   .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE,       .eventsHandler = NULL },
    {.addr = REG_RPS_ADC_BAT_VOLT,  .name="RPS_ADC_BAT_VOLT", .lenData = 2, .type = TYPE_PSEUDO_FLOAT, .value = 0, .opcode = I2C_OPCODE_IDLE,       .eventsHandler = NULL },
    {.addr = REG_RPS_ADC_BAT_CURR,  .name="RPS_ADC_BAT_CURR", .lenData = 2, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE,       .eventsHandler = NULL },
    {.addr = REG_RPS_TEST_MODE,     .name="RPS_TEST_MODE",    .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE,       .eventsHandler = NULL },

    // SHT
    {.addr = REG_SHT_CONNECTED,     .name="sensorConnected",  .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = handler_SHT_CONNECTED   },
    {.addr = REG_SHT_TYPE,          .name="SHT_TYPE",         .lenData = 10, .type = TYPE_UINT,        .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = handler_SHT_TYPE        },
    {.addr = REG_SHT_TEMPERATURE,   .name="sensorTemperature",.lenData = 2, .type = TYPE_FLOAT_SFP, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = handler_SHT_TEMPERATURE },
    {.addr = REG_SHT_HUMIDITY,      .name="sensorHumidity",   .lenData = 1, .type = TYPE_UINT,         .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = handler_SHT_HUMIDITY    },


    // SFP_1
    {.addr = REG_SFP1_PRESENT,      .name="portSfpPresent_1",     .lenData = 1,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = handler_SFP1_PRESENT },
    {.addr = REG_SFP1_LOS,          .name="portSfpSignalDetect_1",.lenData = 1,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = handler_SFP1_LOS     },
    {.addr = REG_SFP1_VENDOR,       .name="portSfpVendor_1",      .lenData = 16, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_SFP1_VENDOR_OUI,   .name="portSfpOui_1",         .lenData = 3,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_SFP1_VENDOR_PN,    .name="portSfpPartNumber_1",  .lenData = 16, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_SFP1_VENDOR_REV,   .name="portSfpRevision_1",    .lenData = 4,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_SFP1_IDENTIFIER,   .name="IDENTIFIER_1",         .lenData = 4,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_SFP1_CONNECTOR,    .name="CONNECTOR_1",          .lenData = 30, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_SFP1_TYPE,         .name="TYPE_1",               .lenData = 16, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_SFP1_LINK_LEN,     .name="LINK_LEN_1",           .lenData = 25, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_SFP1_FIBER_TEC,    .name="FIBER_TEC_1",          .lenData = 32, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_SFP1_MEDIA,        .name="MEDIA_1",              .lenData = 32, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_SFP1_SPEED,        .name="SPEED_1",              .lenData = 16, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_SFP1_ENCODING,     .name="ENCODING_1",           .lenData = 16, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_SFP1_LENC,         .name="LENC_1",               .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_SFP1_WAVELEN,      .name="WAVELEN_1",            .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_SFP1_NBR,          .name="NBR_1",                .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_SFP1_LEN9,         .name="LEN9_1",               .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_SFP1_LEN50,        .name="LEN50_1",              .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_SFP1_LEN62,        .name="LEN62_1",              .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_SFP1_TEMPER,       .name="portSfpTemperature_1", .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = handler_SFP1_TEMPER   },
    {.addr = REG_SFP1_VOLTAGE,      .name="portSfpVoltage_1",     .lenData = 2,  .type = TYPE_PSEUDO_FLOAT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = handler_SFP1_VOLTAGE  },
    {.addr = REG_SFP1_CURRENT,      .name="CURRENT_1",            .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_SFP1_TX_BIAS,      .name="portSfpBiasCurrent_1", .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL  },
    {.addr = REG_SFP1_TX_POWER_DB,  .name="portSfpTxOutPowerDb_1",.lenData = 2, .type = TYPE_FLOAT_SFP, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = handler_SFP1_TX_POWER_DB },
    {.addr = REG_SFP1_RX_POWER_DB,  .name="portSfpRxOutPowerDb_1",.lenData = 2, .type = TYPE_FLOAT_SFP, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = handler_SFP1_RX_POWER_DB },
    {.addr = REG_SFP1_TX_POWER,     .name="portSfpTxOutPower_1",  .lenData = 2, .type = TYPE_UINT,   .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_SFP1_RX_POWER,     .name="portSfpRxOutPower_1",  .lenData = 2, .type = TYPE_UINT,   .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    //SPF_2
    {.addr = REG_SFP2_PRESENT,      .name="portSfpPresent_2",       .lenData = 1,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler =  handler_SFP2_PRESENT },
    {.addr = REG_SFP2_LOS,          .name="portSfpSignalDetect_2",  .lenData = 1,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler =  handler_SFP2_LOS     },
    {.addr = REG_SFP2_VENDOR,       .name="portSfpVendor_2",        .lenData = 16, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL  },
    {.addr = REG_SFP2_VENDOR_OUI,   .name="portSfpOui_2",           .lenData = 3,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL  },
    {.addr = REG_SFP2_VENDOR_PN,    .name="portSfpPartNumber_2",    .lenData = 16, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL  },
    {.addr = REG_SFP2_VENDOR_REV,   .name="portSfpRevision_2",      .lenData = 4,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL  },
    {.addr = REG_SFP2_IDENTIFIER,   .name="IDENTIFIER_2",           .lenData = 4,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL  },
    {.addr = REG_SFP2_CONNECTOR,    .name="CONNECTOR_2",            .lenData = 30, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL  },
    {.addr = REG_SFP2_TYPE,         .name="TYPE_2",                 .lenData = 16, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL  },
    {.addr = REG_SFP2_LINK_LEN,     .name="LINK_LEN_2",             .lenData = 25, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL  },
    {.addr = REG_SFP2_FIBER_TEC,    .name="FIBER_TEC_2",            .lenData = 32, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = NULL },
    {.addr = REG_SFP2_MEDIA,        .name="MEDIA_2",                .lenData = 32, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = NULL },
    {.addr = REG_SFP2_SPEED,        .name="SPEED_2",                .lenData = 16, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = NULL },
    {.addr = REG_SFP2_ENCODING,     .name="ENCODING_2",             .lenData = 16, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = NULL },
    {.addr = REG_SFP2_WAVELEN,      .name="WAVELEN_2",              .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = NULL },
    {.addr = REG_SFP2_NBR,          .name="NBR_2",                  .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = NULL },
    {.addr = REG_SFP2_LEN9,         .name="LEN9_2",                 .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = NULL },
    {.addr = REG_SFP2_LEN50,        .name="LEN50_2",                .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = NULL },
    {.addr = REG_SFP2_LEN62,        .name="LEN62_2",                .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = NULL },
    {.addr = REG_SFP2_LENC,         .name="LENC_2",                 .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = NULL },
    {.addr = REG_SFP2_TEMPER,       .name="portSfpTemperature_2",   .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = handler_SFP2_TEMPER   },
    {.addr = REG_SFP2_VOLTAGE,      .name="portSfpVoltage_2",       .lenData = 2,  .type = TYPE_PSEUDO_FLOAT, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = handler_SFP2_VOLTAGE  },
    {.addr = REG_SFP2_CURRENT,      .name="CURRENT_2",              .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = NULL },
    {.addr = REG_SFP2_TX_BIAS,      .name="portSfpBiasCurrent_2",   .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = NULL },
    {.addr = REG_SFP2_TX_POWER_DB,  .name="portSfpTxOutPowerDb_2",  .lenData = 2,  .type = TYPE_FLOAT_SFP, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = handler_SFP2_TX_POWER_DB },
    {.addr = REG_SFP2_RX_POWER_DB,  .name="portSfpRxOutPowerDb_2",  .lenData = 2,  .type = TYPE_FLOAT_SFP, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = handler_SFP2_RX_POWER_DB },
    {.addr = REG_SFP2_TX_POWER,     .name="portSfpTxOutPower_2",    .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = NULL },
    {.addr = REG_SFP2_RX_POWER,     .name="portSfpRxOutPower_2",    .lenData = 2,  .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,   .eventsHandler = NULL },

    // RTC
    {.addr = REG_RTC_STATUS, .name="RTC_STATUS",    .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_RTC_YEAR,   .name="RTC_YEAR",      .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_RTC_MONTH,  .name="RTC_MONTH",     .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_RTC_DAY,    .name="RTC_DAY",       .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_RTC_WEEKDAY,.name="RTC_WEEKDAY",   .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_RTC_HOUR,   .name="RTC_HOUR",      .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_RTC_MINUTE, .name="RTC_MINUTE",    .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_RTC_SECOND, .name="RTC_SECOND",    .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },

    // POE
    {.addr = REG_POE_ID,   .name="POE_ID",   .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_POE_STATE,.name="POE_STATE",.lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_POE_BANK, .name="POE_BANK", .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
    {.addr = REG_POE_MODE, .name="POE_MODE", .lenData = 1, .type = TYPE_UINT, .value = 0, .opcode = I2C_OPCODE_IDLE,  .eventsHandler = NULL },
};

void i2c_test(void)
{
    printf("Hello from I2C\n");
}

REGISTER_ADDR get_i2c_addr_by_name(char *reg_name, i2c_param *i2CParam)
{
    for (uint8_t i = 0; i < sizeof(sock_msgArr)/sizeof(i2c_data_t); i++) {
        //printf("i = %d reg_name = %s -- i2c_name %s\n", i, reg_name, sock_msgArr[i].name);
        if (strcmp(reg_name, sock_msgArr[i].name) == 0) {
            //printf("FOUND i = %d gpio_name = %s\n", i, sock_msgArr[i].name);
            i2CParam->i2c_data.opcode = sock_msgArr[i].opcode;
            i2CParam->i2c_data.addr = sock_msgArr[i].addr;
            i2CParam->i2c_data.lenData = sock_msgArr[i].lenData;
            i2CParam->i2c_data.type = sock_msgArr[i].type;
            return sock_msgArr[i].addr;
        }
    }
}

void open_i2c(int *i2c_fd)
{
    *i2c_fd = open(I2C_ADAPTER, O_RDWR);
    if (*i2c_fd < 0) {
        perror("open i2c");
        printf("Unable to open i2c file\n");
        exit (EXIT_FAILURE);
    }
    printf("open i2c interface\n");

    timer_set(&hi_prio_timer,3);
    timer_set(&low_prio_timer,0);

    read_buffer(*i2c_fd,REG_HI_PRIO_LEN,2,i2c_raw_data);
    hi_prio_len = i2c_raw_data[1]<<8 | i2c_raw_data[0];


    read_buffer(*i2c_fd,REG_LOW_PRIO_LEN,2,i2c_raw_data);
    low_prio_len = i2c_raw_data[1]<<8 | i2c_raw_data[0];


    if(isDebugMode()) {
        printf("hi_prio_len = %d\n",hi_prio_len);
        printf("low_prio_len = %d\n",low_prio_len);
    }

}

uint8_t read_i2c(int *i2c_fd){
    uint8_t tmp,errorCode = 0;

    if(timer_expired(&hi_prio_timer)){
        errorCode = read_buffer(*i2c_fd,REG_HI_PRIO_DUMP,hi_prio_len,i2c_raw_data);
        if(errorCode == 0){
            errorCode = i2c_parse_tlv(hi_prio_len);
            if(isDebugMode()) {
                printf("Read Hi prio %d\n", hi_prio_len);
            }
        }
        timer_set(&hi_prio_timer,3);
    }
    else if(timer_expired(&low_prio_timer)){
        errorCode = read_buffer(*i2c_fd,REG_LOW_PRIO_DUMP,low_prio_len,i2c_raw_data);
        if(errorCode == 0){
            errorCode = i2c_parse_tlv(low_prio_len);
            if(isDebugMode()) {
                printf("Read Low prio %d\n", low_prio_len);
            }
        }
        timer_set(&low_prio_timer,30);
    }
    else if(i2c_write.write_flag){
        write_buffer(*i2c_fd, i2c_write.i2c_data.addr, i2c_write.i2c_data.lenData, i2c_write.i2c_data.value);
        i2c_write.write_flag = 0;
    }
    else
       sleep(1);

    return errorCode;
}

static uint8_t  get_id_by_addr(REGISTER_ADDR addr){
    for (uint8_t i = 0; i < sizeof(sock_msgArr)/sizeof(i2c_data_t); i++) {
        if(sock_msgArr[i].addr == addr)
            return i;
    }
}


static uint8_t i2c_parse_tlv(uint16_t len){
    uint16_t offset = 0;
    uint8_t id;
    i2c_tlv *tlv;
    while(offset < len){
        tlv = (i2c_tlv *)((uint8_t *) i2c_raw_data + offset);
        if(tlv->regaddr > MAX_SENSORS){
            printf("Read: reg addr overflow!\n");
            return EXIT_FAILURE;
        }
        id = get_id_by_addr(tlv->regaddr);
        if(tlv->len < sizeof(sock_msgArr[id].value)){
            memcpy(sock_msgArr[id].value,tlv->data,tlv->len);
        }
        offset += tlv->len+2;
    }
    return EXIT_SUCCESS;
}



//read I2C data
int read_buffer(int fd, unsigned char regaddr, int len, unsigned char *buffer)
{
        struct i2c_rdwr_ioctl_data data;
        struct i2c_msg messages[2];
        unsigned char regaddr_[2];
        regaddr_[0] = regaddr;
        regaddr_[1] = len;

        /*
         * .addr -
         * .flags -  (0 - w, 1 - r)
         * .len -
         * .buf -
         */
        messages[0].addr = I2C_ADDR;
        messages[0].flags = 0;//write
        messages[0].len = 2;
        messages[0].buf = regaddr_;

        messages[1].addr = I2C_ADDR;
        messages[1].flags = 1;//read
        messages[1].len = len;
        messages[1].buf = buffer;

        data.msgs = messages;
        data.nmsgs = 2;
   
        if (ioctl(fd, I2C_RDWR, &data) < 0){
            printf("Read: cant send data!\n");
            return EXIT_FAILURE;
        }        
        if (buffer[0] == 0xAA && buffer[1] == 0xAA)
        {
            printf("I2C: error response\n");
            return EXIT_FAILURE;
        }
  
        return EXIT_SUCCESS;
}


//write I2C data
int write_buffer(int fd, unsigned char regaddr, int len, unsigned char *buffer)
{
    unsigned char tmp[10];
    struct i2c_rdwr_ioctl_data data;
    struct i2c_msg messages[1];

    if(isDebugMode()) {
        printf("write_buffer\n");
        printf("addr = %d\n", regaddr);
        printf("len = %d\n", len);
        printf("value[0] = %d\n", buffer[0]);
    }

    //printf("value[1] = %d\n",buffer[1]);
    //printf("value[2] = %d\n",buffer[2]);
    //printf("value[3] = %d\n",buffer[3]);

    /*
     * .addr -
     * .flags -  (0 - w, 1 - r)
     * .len -
     * .buf -
     */

    messages[0].addr = I2C_ADDR;
    messages[0].flags = 0;//write
    messages[0].len = len+2;
    messages[0].buf = tmp;
    messages[0].buf[0] = regaddr;
    messages[0].buf[1] = len;
    for(int i=0; i < len; i++)
        messages[0].buf[i+2] = buffer[i];
    
    data.msgs = messages;
    data.nmsgs = 1;
    
    if (ioctl(fd, I2C_RDWR, &data) < 0){
        printf("Write: cant send data!\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int i2c_get_MsgSize(void)
{
    int len = sizeof(i2c_data_t);
    return len;
}

void i2c_getData(i2c_data_t *i2CData, REGISTER_ADDR addr, int len)
{
    uint8_t id;
    id = get_id_by_addr(addr);
    sock_msgArr[id].lenMSG = len;
    memcpy(i2CData, &sock_msgArr[id], len);
}

//todo
void i2c_setData(i2c_data_t *i2CData, REGISTER_ADDR addr, int len)
{
    if(isDebugMode()) {
//        printf("i2c_setData\n");
//        printf("addr = %d\n",addr);
//        printf("len = %d\n",len);
//        printf("value[0] = %d\n",i2CData->value[0]);
//        printf("value[1] = %d\n",i2CData->value[1]);
//        printf("value[2] = %d\n",i2CData->value[2]);
//        printf("value[3] = %d\n",i2CData->value[3]);
    }
    uint8_t id;
    id = get_id_by_addr(addr);
    sock_msgArr[id].lenMSG = len;
    memcpy(&i2c_write.i2c_data, i2CData, len);
    memcpy(&sock_msgArr[id],&i2c_write.i2c_data, len);
    i2c_write.write_flag = 1;
}

void i2c_events_handler() {
    static uint8_t i;
    static uint8_t delayStart = 0;
    i2c_event_data_t i2cData;
    uint8_t pause_sec = 3;
    for (i = 0; i < sizeof(sock_msgArr)/sizeof(i2c_data_t); i++) {
        i2cData.addr = sock_msgArr[i].addr;
        memcpy(i2cData.value, sock_msgArr[i].value, sizeof(i2cData.value));
        if(sock_msgArr[i].eventsHandler != NULL)
            sock_msgArr[i].eventsHandler(&i2cData);
    }
}






static void i2c_parsingData(REGISTER_ADDR addr)
{
    uint8_t id;
    id = get_id_by_addr(addr);
    switch (sock_msgArr[id].lenData)
    {
        case 1:
            //uint8
            printf("%d\n", sock_msgArr[id].value[0]);
            break;
        case 2:
        {
            uint16_t tmp16;
            if (sock_msgArr[id].type == TYPE_PSEUDO_FLOAT)
            {
                tmp16 = sock_msgArr[id].value[0] | sock_msgArr[id].value[1] << 8;
                if (tmp16 < 1000)
                    printf("%d.%d\n", tmp16 / 10, tmp16 % 10);
                else
                    printf("%d.%d\n", tmp16 / 1000, tmp16 % 1000);
            }
            else
            {
                //uint16
                printf("%d\n", sock_msgArr[id].value[0] | sock_msgArr[id].value[1] << 8);
            }
        }
            break;
        case 4:
            //uint32
            printf("%d\n", (uint32_t) (sock_msgArr[id].value[0] |
                                       sock_msgArr[id].value[1] << 8 |
                                       sock_msgArr[id].value[2] << 16 |
                                       sock_msgArr[id].value[3] << 24));
            break;
        default:
            printf("%s\n", sock_msgArr[id].value);
    }
}







