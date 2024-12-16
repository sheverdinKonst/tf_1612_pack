#include  <stdarg.h>
#include "syslog.h"
#include "i2c_lib/i2c_module.h"
#include "utils_lib/utils_module.h"
#include "events_handlers.h"



static uint16_t get_16bitValue(const uint8_t *val1);
static uint32_t get_32bitValue(const uint8_t *val1);
static event_t check_out_of_range(uint32_t value, uint32_t prevValue, uint32_t max, uint32_t min);
static event_t check_changed_state(uint32_t value, uint32_t preValue, uint32_t state_1, uint32_t state_2);
static void i2c_sendError(i2c_event_data_t *i2cData, REGISTER_ADDR addr, uint32_t value,event_t i2c_event);


__attribute__ ((format (printf, 1, 2)))
static void send_log(const char *str,...)
{
    va_list args;
    va_start(args, str);

    openlog("tfortis", LOG_PID, LOG_USER);
    syslog(LOG_INFO, str,args);
    closelog();

    if(isDebugMode()) {
        vprintf(str, args);
    }

    va_end(args);
}




event_t handler_ADC_CH1(i2c_event_data_t *i2cData)
{
    
    static uint16_t ch1_prevValue = 0xFFFF;
    uint16_t value = get_16bitValue(&i2cData->value[0]);
    event_t i2c_event = I2C_ERR_OK;

    i2c_event = check_out_of_range(value, ch1_prevValue, REG_ADC_CH1_max, REG_ADC_CH1_min);

    if (i2c_event == I2C_ERR_CHANGE_VALUE)
    {
        ch1_prevValue = value;
    }

    if (i2c_event == I2C_OVER_MAX || i2c_event == I2C_LESS_MIN)
    {
        send_log("ADC CH1 out of range: %d", value);
        ch1_prevValue = value;
    }
    return i2c_event;
}
event_t handler_ADC_CH2(i2c_event_data_t *i2cData)
{
    static uint16_t ch2_prevValue = 0xFFFF;
    uint16_t value = get_16bitValue(&i2cData->value[0]);
    event_t i2c_event = I2C_ERR_OK;

    i2c_event = check_out_of_range(value,ch2_prevValue,REG_ADC_CH2_max,REG_ADC_CH2_min);

    if (i2c_event == I2C_ERR_CHANGE_VALUE) {
        ch2_prevValue = value;
    }

    if (i2c_event == I2C_OVER_MAX || i2c_event == I2C_LESS_MIN)
    {
        send_log("ADC CH2 out of range: %d", value);
        ch2_prevValue = value;
    }

    return i2c_event;
}
 event_t handler_ADC_CH3(i2c_event_data_t *i2cData)
{
    static uint16_t ch3_prevValue = 0xFFFF;
    uint16_t value = get_16bitValue(&i2cData->value[0]);
    event_t event = I2C_ERR_OK;

    event = check_out_of_range(value,ch3_prevValue,REG_ADC_CH3_max,REG_ADC_CH3_min);

    if (event == I2C_ERR_CHANGE_VALUE) {
        ch3_prevValue = value;
    }

    if (event == I2C_OVER_MAX || event == I2C_LESS_MIN)
    {
        send_log("ADC CH3 out of range: %d", value);
        ch3_prevValue = value;
    }
    return event;
}

// SENSORS
event_t handler_TAMPER(i2c_event_data_t *i2cData)
{
    static uint8_t tamper1_prevValue = 0xFF;
    event_t i2c_event = I2C_ERR_OK;
    uint8_t value = i2cData->value[0];

    if (value != tamper1_prevValue)
    {
        if(tamper1_prevValue != 0xFF)
        {
            i2c_event = check_changed_state(value, tamper1_prevValue, REG_TAMPER_close, REG_TAMPER_open);
        }
        tamper1_prevValue = value;
    }

    if (i2c_event != I2C_ERR_OK) {
        if (i2c_event == I2C_CHANGED_TO_CLOSE ) {
            send_log("TAMPER changed to Close\n");
        }
        else if(i2c_event == I2C_CHANGED_TO_OPEN)
        {
            send_log("TAMPER changed to Open\n");
        }
    }
    return i2c_event;
}

event_t handler_SENSOR1(i2c_event_data_t *i2cData)
{
    static uint8_t sen1_prevValue = 0xFF;
    event_t event = I2C_ERR_OK;
    uint8_t value = i2cData->value[0];
    if (value != sen1_prevValue)
    {
        if(sen1_prevValue != 0xFF)
        {
            event = check_changed_state(value, sen1_prevValue, REG_TAMPER_close, REG_TAMPER_open );
        }
        sen1_prevValue = value;
    }

    if (event == I2C_CHANGED_TO_CLOSE ) {
        send_log("SENSOR1 changed to Close\n");
    }
    else if(event == I2C_CHANGED_TO_OPEN)
    {
        send_log("SENSOR1 changed to Open\n");
    }
    return event;
}
 event_t handler_SENSOR2(i2c_event_data_t *i2cData)
{
    static uint8_t sen2_prevValue = 0xFF;
    event_t event = I2C_ERR_OK;
    uint8_t value = i2cData->value[0];
    if (value != sen2_prevValue)
    {
        if(sen2_prevValue != 0xFF)
        {
            event = check_changed_state(value, sen2_prevValue, REG_TAMPER_close, REG_TAMPER_open );
        }
        sen2_prevValue = value;
    }

    if (event == I2C_CHANGED_TO_CLOSE ) {
        send_log("SENSOR2 changed to Close\n");
    }
    else if(event == I2C_CHANGED_TO_OPEN)
    {
        send_log("SENSOR2 changed to Open\n");
    }
    return event;
}
 event_t handler_RELAY1(i2c_event_data_t *i2cData)
{
    static uint8_t sen3_prevValue = 0xFF;
    event_t event = I2C_ERR_OK;
    uint8_t value = i2cData->value[0];
    if (value != sen3_prevValue)
    {
        if(sen3_prevValue != 0xFF)
        {
            event = check_changed_state(value, sen3_prevValue, REG_TAMPER_close, REG_TAMPER_open );
        }
        sen3_prevValue = value;
    }

    if (event == I2C_CHANGED_TO_CLOSE ) {
        send_log("RELAY changed to Close\n");
    }
    else if(event == I2C_CHANGED_TO_OPEN)
    {
        send_log("RELAY changed to Open\n");
    }
    return event;
}
 event_t handler_DEFAULT_LONG_PRESSED(i2c_event_data_t *i2cData)
{
    static uint16_t longPress_prevValue = 0xFFFF;
    event_t event = I2C_ERR_OK;
    uint16_t value = get_16bitValue(&i2cData->value[0]);
    if (value != longPress_prevValue)
    {
        if(longPress_prevValue != 0xFF)
        {
            event = check_changed_state(value, longPress_prevValue, REG_DEFAULT_LONG_PRESSED_reset, 0);
        }
        longPress_prevValue = value;
    }

    if (event != I2C_ERR_OK) {
        event = (event==I2C_CHANGED_TO_CLOSE)?I2C_LONG_PRESSED:I2C_ERR_OK;
        if(event == I2C_LONG_PRESSED) {
            send_log("DEFAULT SETTINGS by DEFAULT BUTTON\n");
        }
    }
    return event;
}
 event_t handler_RPS_VAC(i2c_event_data_t *i2cData)
{
    static uint8_t vac_prevValue = 0xFF;
    event_t event = I2C_ERR_OK;
    uint8_t value = i2cData->value[0];

    if (value != vac_prevValue)
    {
        if(vac_prevValue != 0xFF)
        {
            event = check_changed_state(value, vac_prevValue, REG_RPS_VAC_vac, REG_RPS_VAC_battary);
        }
        vac_prevValue = value;
    }
    if (event != I2C_ERR_OK) {
        event = (event==I2C_CHANGED_TO_CLOSE)?I2C_CHANGED_TO_VAC:I2C_CHANGED_TO_BAT;
        if (event == I2C_CHANGED_TO_VAC ) {
            send_log("UPS: power changed to VAC\n");
        }
        else if(event == I2C_CHANGED_TO_BAT)
        {
            send_log("UPS: power changed to Battery\n");
        }
    }
    return event;
}
 event_t handler_RPS_BAT_VOLTAGE(i2c_event_data_t *i2cData)
{
    static uint16_t batVolt_prevValue = 0xFFFF;
    event_t event = I2C_ERR_OK;
    uint16_t value = get_16bitValue(&i2cData->value[0]);
    if (value != batVolt_prevValue)
    {
        event = check_out_of_range(value,batVolt_prevValue,REG_RPS_BAT_VOLTAGE_max,REG_RPS_BAT_VOLTAGE_min);
    }

    if (event == I2C_ERR_CHANGE_VALUE) {
        batVolt_prevValue = value;
    }

    if (event == I2C_LESS_MIN)
    {
        send_log("UPS: Battery voltage is LOW (%d)",value);
        batVolt_prevValue = value;
    }
    if(event == I2C_OVER_MAX){
        send_log("UPS: Battery voltage reaches max voltage (%d)",value);
        batVolt_prevValue = value;
    }
    return event;
}
 event_t handler_RPS_CHRG_VOLTAGE(i2c_event_data_t *i2cData)
{
    static uint16_t chgVolt_prevValue = 0xFFFF;
    event_t event = I2C_ERR_OK;
    uint16_t value = get_16bitValue(&i2cData->value[0]);

    event = check_out_of_range(value, chgVolt_prevValue,REG_RPS_CHRG_VOLTAGE_max,REG_RPS_CHRG_VOLTAGE_min);

    if (event == I2C_ERR_CHANGE_VALUE) {
        chgVolt_prevValue = value;
    }

    if (event == I2C_LESS_MIN)
    {
        send_log("UPS: Charge voltage is LOW (%d)",value);
        chgVolt_prevValue = value;
    }
    if(event == I2C_OVER_MAX){
        send_log("UPS: Charge voltage reaches max voltage (%d)",value);
        chgVolt_prevValue = value;
    }
    return event;
}
 event_t handler_RPS_BAT_CURRENT(i2c_event_data_t *i2cData)
{
    static uint16_t batCur_prevValue = 0xFFFF;
    event_t event = I2C_ERR_OK;
    uint16_t value = get_16bitValue(&i2cData->value[0]);
    return event;
}
 event_t handler_RPS_TEMPER(i2c_event_data_t *i2cData)
{
    static uint8_t temper_prevValue = 0xFF;
    event_t event = I2C_ERR_OK;
    uint8_t value = i2cData->value[0];

    event = check_out_of_range(value, temper_prevValue,REG_RPS_TEMPER_max,REG_RPS_TEMPER_min);

    if (event == I2C_ERR_CHANGE_VALUE) {
        temper_prevValue = value;
    }

    if(event == I2C_OVER_MAX){
        send_log("UPS: RPS board overheat (%d)\n",value);
        temper_prevValue = value;
    }
    return event;
}

// SHT
event_t handler_SHT_TYPE(i2c_event_data_t *i2cData)
{
    //printf("REG_SHT_TYPE value = %s\n", sock_msgArr[REG_SHT_TYPE].value);
    return 0;
}
 event_t handler_SHT_CONNECTED(i2c_event_data_t *i2cData)
{
    static uint8_t sht_connected_prevValue = 0xFF;
    event_t event = I2C_ERR_OK;
    uint8_t  value = i2cData->value[0];

    if (value != sht_connected_prevValue)
    {
        if(sht_connected_prevValue != 0xFF)
        {
            event = check_changed_state(value, sht_connected_prevValue,REG_SHT_CONNECTED_yes,REG_SHT_CONNECTED_no);
        }
        sht_connected_prevValue = value;
    }

    if (event != I2C_ERR_OK) {
        if(value == REG_SHT_CONNECTED_yes)
            send_log("SHT: connection status %d\n",value);
    }

    return event;

}
 event_t handler_SHT_TEMPERATURE(i2c_event_data_t *i2cData)
{
    static uint16_t shtTemper_prevValue = 0xFFFF;
    event_t event = I2C_ERR_OK;
    i2c_data_t data;
    i2c_getData(&data,REG_SHT_CONNECTED,1);

    if (data.value[0] == REG_SHT_CONNECTED_yes)
    {

        int16_t value = get_16bitValue(&i2cData->value[0]);
        event = check_out_of_range(value, shtTemper_prevValue, REG_SHT_TEMPERATURE_max, REG_SHT_TEMPERATURE_min);

        if (event == I2C_OVER_MAX){
            send_log("SHT: temperature is higher than maximum %d\n",value);
            shtTemper_prevValue = value;
        }
        if(event == I2C_LESS_MIN)
        {
            send_log("SHT: temperature is lower than minimum %d\n",value);
            shtTemper_prevValue = value;
        }
    }
    return event;
}
 event_t handler_SHT_HUMIDITY(i2c_event_data_t *i2cData)
{
    static uint8_t shtHim_prevValue = 0xFF;
    event_t event = I2C_ERR_OK;

    i2c_data_t data;
    i2c_getData(&data,REG_SHT_CONNECTED,1);

    if (data.value[0] == REG_SHT_CONNECTED_yes)
    {

        uint8_t value = i2cData->value[0];
        event = check_out_of_range(value, shtHim_prevValue, REG_SHT_HUMIDITY_max, REG_SHT_HUMIDITY_min);

        if (event == I2C_ERR_CHANGE_VALUE)
        {
            shtHim_prevValue = value;
        }

        if (event == I2C_OVER_MAX){
            send_log("SHT: humidity is higher than maximum %d\n",value);
            shtHim_prevValue = value;
        }
        if(event == I2C_LESS_MIN)
        {
            send_log("SHT: humidity is lower than minimum %d\n",value);
            shtHim_prevValue = value;
        }
    }
    return event;
}


event_t handler_SFP1_PRESENT(i2c_event_data_t *i2cData)
{
    static uint8_t spf1_present_prevValue = 0xFF;
    event_t event = I2C_ERR_OK;
    uint8_t value = i2cData->value[0];

    if (value != spf1_present_prevValue)
    {
        if(spf1_present_prevValue != 0xFF)
        {
            event = check_changed_state(value, spf1_present_prevValue,REG_SFP_PRESENT_yes,REG_SFP_PRESENT_no);
        }
        spf1_present_prevValue = value;
    }
    if (event != I2C_ERR_OK) {
        if(value)
            send_log("SFP1: in chassis\n");
        else
            send_log("SFP1: removed from chassis\n");
    }
    return event;
}

event_t handler_SFP2_PRESENT(i2c_event_data_t *i2cData)
{
    static uint8_t spf2_present_prevValue = 0xFF;
    event_t event = I2C_ERR_OK;
    uint8_t value = i2cData->value[0];

    if (value != spf2_present_prevValue)
    {
        if(spf2_present_prevValue != 0xFF)
        {
            event = check_changed_state(value, spf2_present_prevValue,REG_SFP_PRESENT_yes,REG_SFP_PRESENT_no);
        }
        spf2_present_prevValue = value;
    }
    if (event != I2C_ERR_OK) {
        if(value)
            send_log("SFP2: in chassis\n");
        else
            send_log("SFP2: removed from chassis\n");
    }
    return event;
}


event_t handler_SFP1_LOS(i2c_event_data_t *i2cData)
{
    static uint8_t spf1Los_prevValue = 0xFF;
    event_t event = I2C_ERR_OK;
    uint8_t value = i2cData->value[0];

    if (value != spf1Los_prevValue)
    {
        if(spf1Los_prevValue != 0xFF)
        {
            event = check_changed_state(value, spf1Los_prevValue,REG_SFP_LOS_no,REG_SFP_LOS_yes);
        }
        spf1Los_prevValue = value;
    }
    if (event != I2C_ERR_OK) {
        if(value)
            send_log("SFP1: lost optical signal\n");
        else {
            i2c_data_t data;
            i2c_getData(&data,REG_SFP1_RX_POWER,2);
            uint16_t power_value = get_16bitValue(&data.value[0]);
            send_log("SFP1: optical signal UP (%d dBm)\n",power_value);
        }
    }
    return event;
}
event_t handler_SFP2_LOS(i2c_event_data_t *i2cData)
{
    static uint8_t spf2Los_prevValue = 0xFF;
    event_t event = I2C_ERR_OK;
    uint8_t value = i2cData->value[0];

    if (value != spf2Los_prevValue)
    {
        if(spf2Los_prevValue != 0xFF)
        {
            event = check_changed_state(value, spf2Los_prevValue,REG_SFP_LOS_no,REG_SFP_LOS_yes);
        }
        spf2Los_prevValue = value;
    }
    if (event != I2C_ERR_OK) {
        if(value)
            send_log("SFP2: lost optical signal\n");
        else {
            i2c_data_t data;
            i2c_getData(&data,REG_SFP2_RX_POWER,2);
            uint16_t power_value = get_16bitValue(&data.value[0]);
            send_log("SFP2: optical signal UP (%d dBm)\n",power_value);
        }
    }
    return event;
}


 event_t handler_SFP1_TEMPER(i2c_event_data_t *i2cData)
{
    static uint16_t spf1_temper_prevValue = 0xFFFF;
    event_t event = I2C_ERR_OK;
    i2c_data_t data;
    i2c_getData(&data,REG_SFP1_PRESENT,1);

    if (data.value[0] == REG_SFP_PRESENT_yes)
    {
        uint16_t value = get_16bitValue(&i2cData->value[0]);
        event = check_out_of_range(value, spf1_temper_prevValue, REG_SFP_TEMPER_max, REG_SFP_TEMPER_min);

        if (event == I2C_ERR_CHANGE_VALUE)
        {
            spf1_temper_prevValue = value;
        }

        if (event == I2C_OVER_MAX)
        {
            send_log("SFP1: overheating (%d)\n",value);
            spf1_temper_prevValue = value;
        }
    }
    return event;
}
event_t handler_SFP2_TEMPER(i2c_event_data_t *i2cData)
{
    static uint16_t spf2_temper_prevValue = 0xFFFF;
    event_t event = I2C_ERR_OK;
    i2c_data_t data;
    i2c_getData(&data,REG_SFP2_PRESENT,1);
    if (data.value[0] == REG_SFP_PRESENT_yes)
    {
        uint16_t value = get_16bitValue(&i2cData->value[0]);
        event = check_out_of_range(value, spf2_temper_prevValue, REG_SFP_TEMPER_max, REG_SFP_TEMPER_min);

        if (event == I2C_ERR_CHANGE_VALUE)
        {
            spf2_temper_prevValue = value;
        }

        if (event == I2C_OVER_MAX)
        {
            send_log("SFP2: overheating (%d)\n",value);
            spf2_temper_prevValue = value;
        }
    }
    return event;
}
 event_t handler_SFP1_VOLTAGE(i2c_event_data_t *i2cData)
{
    static uint16_t spf1Volt_prevValue = 0xFFFF;
    event_t event = I2C_ERR_OK;

    i2c_data_t data;
    i2c_getData(&data,REG_SFP1_PRESENT,1);
    if (data.value[0] == REG_SFP_PRESENT_yes)
    {
        uint16_t value = get_16bitValue(&i2cData->value[0]);
        event = check_out_of_range(value, spf1Volt_prevValue, REG_SFP_VOLTAGE_max, REG_SFP_VOLTAGE_min);
        if (event == I2C_ERR_CHANGE_VALUE)
        {
            spf1Volt_prevValue = value;
        }

        if (event == I2C_OVER_MAX || event == I2C_LESS_MIN)
        {
            send_log("SFP1: voltage not in range (%d)\n",value);
            spf1Volt_prevValue = value;
        }
    }
    return event;
}
event_t handler_SFP2_VOLTAGE(i2c_event_data_t *i2cData)
{
    static uint16_t spf2Volt_prevValue = 0xFFFF;
    event_t event = I2C_ERR_OK;

    i2c_data_t data;
    i2c_getData(&data,REG_SFP2_PRESENT,1);
    if (data.value[0] == REG_SFP_PRESENT_yes)
    {
        uint16_t value = get_16bitValue(&i2cData->value[0]);
        event = check_out_of_range(value, spf2Volt_prevValue, REG_SFP_VOLTAGE_max, REG_SFP_VOLTAGE_min);
        if (event == I2C_ERR_CHANGE_VALUE)
        {
            spf2Volt_prevValue = value;
        }

        if (event == I2C_OVER_MAX || event == I2C_LESS_MIN)
        {
            send_log("SFP2: voltage not in range (%d)\n",value);
            spf2Volt_prevValue = value;
        }
    }
    return event;
}

event_t handler_SFP1_CURRENT(i2c_event_data_t *i2cData)
{
    static uint16_t spf1Current_prevValue = 0xFFFF;
    event_t event = I2C_ERR_OK;
    i2c_data_t data;
    i2c_getData(&data,REG_SFP1_PRESENT,1);
    if (data.value[0] == REG_SFP_PRESENT_yes)
    {
        uint16_t value = get_16bitValue(&i2cData->value[0]);
        event = check_out_of_range(value, spf1Current_prevValue, REG_SFP_CURRENT_max, REG_SFP_CURRENT_min);
        if (event == I2C_ERR_CHANGE_VALUE)
        {
            spf1Current_prevValue = value;
        }
        if (event == I2C_OVER_MAX || event == I2C_LESS_MIN)
        {
            send_log("SFP1: current not in range (%d)\n",value);
            spf1Current_prevValue = value;
        }
    }
    return event;
}
event_t handler_SFP2_CURRENT(i2c_event_data_t *i2cData)
{
    static uint16_t spf2Current_prevValue = 0xFFFF;
    event_t event = I2C_ERR_OK;
    i2c_data_t data;
    i2c_getData(&data,REG_SFP2_PRESENT,1);
    if (data.value[0] == REG_SFP_PRESENT_yes)
    {
        uint16_t value = get_16bitValue(&i2cData->value[0]);
        event = check_out_of_range(value, spf2Current_prevValue, REG_SFP_CURRENT_max, REG_SFP_CURRENT_min);
        if (event == I2C_ERR_CHANGE_VALUE)
        {
            spf2Current_prevValue = value;
        }
        if (event == I2C_OVER_MAX || event == I2C_LESS_MIN)
        {
            send_log("SFP2: current not in range (%d)\n",value);
            spf2Current_prevValue = value;
        }
    }
    return event;
}

event_t handler_SFP1_TX_BIAS(i2c_event_data_t *i2cData)
{
    static uint16_t spf1TXbias_prevValue = 0xFFFF;
    event_t event = I2C_ERR_OK;
    i2c_data_t data;
    i2c_getData(&data,REG_SFP1_PRESENT,1);
    if (data.value[0] == REG_SFP_PRESENT_yes)
    {
        uint16_t value = get_16bitValue(&i2cData->value[0]);
        event = check_out_of_range(value, spf1TXbias_prevValue, REG_SFP_TX_BIAS_max, REG_SFP_TX_BIAS_min);
        if (event == I2C_ERR_CHANGE_VALUE)
        {
            spf1TXbias_prevValue = value;
        }
        if (event == I2C_OVER_MAX || event == I2C_LESS_MIN)
        {
            send_log("SFP1: TX bias current not in range (%d)\n",value);
            spf1TXbias_prevValue = value;
        }
    }
    return event;
}


event_t handler_SFP2_TX_BIAS(i2c_event_data_t *i2cData)
{
    static uint16_t spf2TXbias_prevValue = 0xFFFF;
    event_t event = I2C_ERR_OK;
    i2c_data_t data;
    i2c_getData(&data,REG_SFP2_PRESENT,1);
    if (data.value[0] == REG_SFP_PRESENT_yes)
    {
        uint16_t value = get_16bitValue(&i2cData->value[0]);
        event = check_out_of_range(value, spf2TXbias_prevValue, REG_SFP_TX_BIAS_max, REG_SFP_TX_BIAS_min);
        if (event == I2C_ERR_CHANGE_VALUE)
        {
            spf2TXbias_prevValue = value;
        }
        if (event == I2C_OVER_MAX || event == I2C_LESS_MIN)
        {
            send_log("SFP2: TX bias current not in range (%d)\n",value);
            spf2TXbias_prevValue = value;
        }
    }
    return event;
}


event_t handler_SFP1_TX_POWER_DB(i2c_event_data_t *i2cData)
{
    static uint16_t spf1_TXpower_prevValue = 0xFFFF;
    event_t event = I2C_ERR_OK;
    i2c_data_t data;
    i2c_getData(&data,REG_SFP1_PRESENT,1);
    if (data.value[0] == REG_SFP_PRESENT_yes)
    {
        uint16_t value = get_16bitValue(&i2cData->value[0]);
        //printf("SFP1_TX_POWER = %d\n", value);
        event = check_out_of_range(value, spf1_TXpower_prevValue, REG_SFP_TX_POWER_max, REG_SFP_TX_POWER_min);

        if (event == I2C_ERR_CHANGE_VALUE)
        {
            spf1_TXpower_prevValue = value;
        }
        if (event == I2C_OVER_MAX || event == I2C_LESS_MIN)
        {
            send_log("SFP1: TX power not in range (%d)\n",value);
            spf1_TXpower_prevValue = value;
        }
    }
    return event;
}
event_t handler_SFP2_TX_POWER_DB(i2c_event_data_t *i2cData)
{
    static uint16_t spf2_TXpower_prevValue = 0xFFFF;
    event_t event = I2C_ERR_OK;
    i2c_data_t data;
    i2c_getData(&data,REG_SFP2_PRESENT,1);
    if (data.value[0] == REG_SFP_PRESENT_yes)
    {
        uint16_t value = get_16bitValue(&i2cData->value[0]);
        //printf("SFP1_TX_POWER = %d\n", value);
        event = check_out_of_range(value, spf2_TXpower_prevValue, REG_SFP_TX_POWER_max, REG_SFP_TX_POWER_min);

        if (event == I2C_ERR_CHANGE_VALUE)
        {
            spf2_TXpower_prevValue = value;
        }
        if (event == I2C_OVER_MAX || event == I2C_LESS_MIN)
        {
            send_log("SFP2: TX power not in range (%d)\n",value);
            spf2_TXpower_prevValue = value;
        }
    }
    return event;
}

event_t handler_SFP1_RX_POWER_DB(i2c_event_data_t *i2cData)
{
    static uint16_t spf1_RXpower_prevValue = 0xFFFF;
    event_t event = I2C_ERR_OK;
    i2c_data_t data;
    i2c_getData(&data,REG_SFP1_PRESENT,1);
    if (data.value[0] == REG_SFP_PRESENT_yes)
    {
        uint16_t value = get_16bitValue(&i2cData->value[0]);
        //printf("SFP1_TX_POWER = %d\n", value);
        event = check_out_of_range(value, spf1_RXpower_prevValue, REG_SFP_RX_POWER_max, REG_SFP_RX_POWER_min);

        if (event == I2C_ERR_CHANGE_VALUE)
        {
            spf1_RXpower_prevValue = value;
        }
        if (event == I2C_OVER_MAX || event == I2C_LESS_MIN)
        {
            send_log("SFP1: RX power not in range (%d)\n",value);
            spf1_RXpower_prevValue = value;
        }
    }
    return event;
}

event_t handler_SFP2_RX_POWER_DB(i2c_event_data_t *i2cData)
{
    static uint16_t spf2_RXpower_prevValue = 0xFFFF;
    event_t event = I2C_ERR_OK;
    i2c_data_t data;
    i2c_getData(&data,REG_SFP2_PRESENT,1);
    if (data.value[0] == REG_SFP_PRESENT_yes)
    {
        uint16_t value = get_16bitValue(&i2cData->value[0]);
        //printf("SFP1_TX_POWER = %d\n", value);
        event = check_out_of_range(value, spf2_RXpower_prevValue, REG_SFP_RX_POWER_max, REG_SFP_RX_POWER_min);

        if (event == I2C_ERR_CHANGE_VALUE)
        {
            spf2_RXpower_prevValue = value;
        }
        if (event == I2C_OVER_MAX || event == I2C_LESS_MIN)
        {
            send_log("SFP2: RX power not in range (%d)\n",value);
            spf2_RXpower_prevValue = value;
        }
    }
    return event;
}


// RTC

// POE

static uint16_t get_16bitValue(const uint8_t *val1)
{
    uint16_t val16 = *val1 | *(val1+1) << 8;
    return val16;
}

static uint32_t get_32bitValue(const uint8_t *val1)
{
    uint32_t val32 = (uint32_t) (*val1 | *(val1+1) << 8 | *(val1+2) << 16 | *(val1+3) << 24);
    return val32;
}
//todo проверить на отрицательные значение
static event_t check_out_of_range(uint32_t value, uint32_t prevValue, uint32_t max, uint32_t min)
{
    event_t event = I2C_ERR_OK;
    uint16_t dif = 0;

    if (value >= prevValue)
    {
        dif = value - prevValue;
    } else
        dif = prevValue - value;
    if (dif > STABLE_VALUE)
    {
        event = I2C_ERR_CHANGE_VALUE;
        if (prevValue != 0xFFFF)
        {
            if (value > max)
            {
                //printf("ERR_OVER_MAX \n");
                //printf("value = %d -- max = %d\n", value, max);
                event = I2C_OVER_MAX;
            }
            else if (value < min)
            {
                //printf("ERR_LESS_MIN \n");
                //printf("value = %d -- min = %d\n", value, min);
                event = I2C_LESS_MIN;
            }
        }
    }
    return event;
}

static event_t check_changed_state(uint32_t value, uint32_t preValue, uint32_t state_1, uint32_t state_2)
{
    //printf("value - %d -- preValue - %d -- state_1 - %d -- state_2 - %d\n", value, preValue, state_1, state_2);
    if (value ==  state_1 && preValue == state_2)
    {
        //printf("CHANGED_TO_CLOSE\n");
        return I2C_CHANGED_TO_CLOSE;
    }
    if (value ==  state_2 && preValue == state_1)
    {
        //printf("CHANGED_TO_OPEN\n");
        return I2C_CHANGED_TO_OPEN;
    }
    return I2C_ERR_OK;
}

/*static void i2c_sendError(i2c_event_data_t *i2cData, REGISTER_ADDR addr, uint32_t value,event_t i2c_event)
{
    //i2cData->value = value;
    i2cData->event = i2c_event;
    i2cData->addr = addr;
    ///todo
    // memcpy(i2cData->name, i2c_name[addr], I2C_NAME_MAX_LEN);
    //memcpy(i2cData->errorName,i2c_eventDict[i2c_event],I2C_ERROR_NAME_MAX_LEN);
}
*/