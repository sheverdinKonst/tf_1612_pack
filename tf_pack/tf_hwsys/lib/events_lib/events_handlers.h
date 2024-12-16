#ifndef TF_HWSYS_EVENTS_HANDLERS_H
#define TF_HWSYS_EVENTS_HANDLERS_H
typedef enum
{
    CRITICAL_IDLE   = 0,
    firstBoot       = 0x001,
    reBoot          = 0x002,
    iomcu_reset     = 0x004,
}CRITICAL_EVENT_t;



// void critical_REG_INTSTAT (void);
// void critical_REG_INTSTAT (void);
// void critical_REG_INTMASK (void);
// void critical_REG_HW_VERS (void);
// void critical_REG_SW_VERS (void);
event_t handler_ADC_CH1 (i2c_event_data_t *i2cData);
event_t handler_ADC_CH2 (i2c_event_data_t *i2cData);
event_t handler_ADC_CH3 (i2c_event_data_t *i2cData);

// SENSORS
event_t handler_TAMPER                (i2c_event_data_t *i2cData);
event_t handler_SENSOR1               (i2c_event_data_t *i2cData);
event_t handler_SENSOR2               (i2c_event_data_t *i2cData);
event_t handler_RELAY1                (i2c_event_data_t *i2cData);
//void critical_REG_DEFAULT_BUTTON        (void);
//void critical_REG_DEFAULT_LED           (void);
event_t handler_DEFAULT_LONG_PRESSED  (i2c_event_data_t *i2cData);

// UPS
// void critical_REG_RPS_CONNECTED          (void);
// void critical_REG_RPS_HW_VERS            (void);
// void critical_REG_RPS_SW_VERS            (void);
event_t handler_RPS_VAC           (i2c_event_data_t *i2cData);
event_t handler_RPS_BAT_VOLTAGE   (i2c_event_data_t *i2cData);
event_t handler_RPS_CHRG_VOLTAGE  (i2c_event_data_t *i2cData);
event_t handler_RPS_BAT_CURRENT   (i2c_event_data_t *i2cData);
event_t handler_RPS_TEMPER        (i2c_event_data_t *i2cData);
//void critical_REG_RPS_LED_STATE          (void);
//void critical_REG_RPS_BAT_KEY            (void);
//void critical_REG_RPS_CHRG_KEY           (void);
//void critical_REG_RPS_REL_STATE          (void);
//void critical_REG_RPS_MIN_VOLTAGE        (void);
//void critical_REG_RPS_DISCH_VOLTAGE      (void);
//void critical_REG_RPS_REMAIN_TIME        (void);
//void critical_REG_RPS_TEST_OK            (void);
//void critical_REG_RPS_CPU_ID             (void);
//void critical_REG_RPS_LTC4151_OK         (void);
//void critical_REG_RPS_ADC_BAT_VOLT       (void);
//void critical_REG_RPS_ADC_BAT_CURR       (void);
//void critical_REG_RPS_TEST_MODE          (void);

// SHT
// void critical_REG_SHT_CONNECTED (void);
event_t handler_SHT_TYPE         (i2c_event_data_t *i2cData);
event_t handler_SHT_CONNECTED(i2c_event_data_t *i2cData);
event_t handler_SHT_TEMPERATURE  (i2c_event_data_t *i2cData);
event_t handler_SHT_HUMIDITY     (i2c_event_data_t *i2cData);

// SPF_1
event_t handler_SFP1_PRESENT     (i2c_event_data_t *i2cData);
event_t handler_SFP2_PRESENT     (i2c_event_data_t *i2cData);
event_t handler_SFP1_LOS         (i2c_event_data_t *i2cData);
event_t handler_SFP2_LOS         (i2c_event_data_t *i2cData);
//void critical_REG_SFP_VENDOR            (void);
//void critical_REG_SFP_VENDOR_OUI        (void);
//void critical_REG_SFP_VENDOR_PN         (void);
//void critical_REG_SFP_VENDOR_REV        (void);
//void critical_REG_SFP_IDENTIFIER        (void);
//void critical_REG_SFP_CONNECTOR         (void);
//void critical_REG_SFP_TYPE              (void);
//void critical_REG_SFP_LINK_LEN          (void);
//void critical_REG_SFP_FIBER_TEC         (void);
//void critical_REG_SFP_MEDIA             (void);
//void critical_REG_SFP_SPEED             (void);
//void critical_REG_SFP_ENCODING          (void);
//void critical_REG_SFP_WAVELEN           (void);
//void critical_REG_SFP_NBR               (void);
//void critical_REG_SFP_LEN9              (void);
//void critical_REG_SFP_LEN50             (void);
//void critical_REG_SFP_LEN62             (void);
//void critical_REG_SFP_LENC              (void);
event_t handler_SFP1_TEMPER      (i2c_event_data_t *i2cData);
event_t handler_SFP2_TEMPER      (i2c_event_data_t *i2cData);
event_t handler_SFP1_VOLTAGE     (i2c_event_data_t *i2cData);
event_t handler_SFP2_VOLTAGE     (i2c_event_data_t *i2cData);
event_t handler_SFP1_CURRENT     (i2c_event_data_t *i2cData);
event_t handler_SFP2_CURRENT     (i2c_event_data_t *i2cData);
event_t handler_SFP1_TX_BIAS     (i2c_event_data_t *i2cData);
event_t handler_SFP2_TX_BIAS     (i2c_event_data_t *i2cData);
event_t handler_SFP1_TX_POWER_DB (i2c_event_data_t *i2cData);
event_t handler_SFP2_TX_POWER_DB (i2c_event_data_t *i2cData);
event_t handler_SFP1_RX_POWER_DB (i2c_event_data_t *i2cData);
event_t handler_SFP2_RX_POWER_DB (i2c_event_data_t *i2cData);

// RTC
// void critical_REG_RTC_STATUS             (void);
// void critical_REG_RTC_YEAR               (void);
// void critical_REG_RTC_MONTH              (void);
// void critical_REG_RTC_DAY                (void);
// void critical_REG_RTC_WEEKDAY            (void);
// void critical_REG_RTC_HOUR               (void);
// void critical_REG_RTC_MINUTE             (void);
// void critical_REG_RTC_SECOND             (void);
//
// // POE
// void critical_REG_POE_ID                 (void);
// void critical_REG_POE_STATE              (void);
// void critical_REG_POE_BANK               (void);
// void critical_REG_POE_MODE               (void);



#endif