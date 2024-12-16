#ifndef _REGISTERS_I2C_H
#define _REGISTERS_I2C_H

#define I2C_OPCODE_READ 			1
#define I2C_OPCODE_WRITE			2
#define I2C_OPCODE_RESPONSE			3

#define TYPE_UINT					1
#define TYPE_PSEUDO_FLOAT			2

#define RTL_GPIO_SYS_LED 		0
#define RTL_GPIO_SYS_RESET		1
#define RTL_GPIO_IOMCU_RESET	11
#define RTL_GPIO_POE_RESET		12
#define RTL_GPIO_BOOT0			14

#define CMD_GET					0
#define CMD_SET 				1


//Main
#define REG_INTSTAT					0
#define REG_INTMASK					1
#define REG_HW_VERS					2
#define REG_SW_VERS					3
#define REG_ADC_CH1					4
#define REG_ADC_CH2					5
#define REG_ADC_CH3					6



#define REG_TAMPER					10
#define REG_SENSOR1					11
#define REG_SENSOR2					12
#define REG_RELAY1					13
#define REG_DEFAULT_BUTTON			14
#define REG_DEFAULT_LED				15
#define REG_DEFAULT_LONG_PRESSED 	16
//UPS
#define REG_RPS_CONNECTED			20
#define REG_RPS_HW_VERS				21
#define REG_RPS_SW_VERS				22
#define REG_RPS_VAC					23
#define REG_RPS_BAT_VOLTAGE			24
#define REG_RPS_CHRG_VOLTAGE		25
#define REG_RPS_BAT_CURRENT			26
#define REG_RPS_TEMPER				27
#define REG_RPS_LED_STATE			28
#define REG_RPS_BAT_KEY				29
#define REG_RPS_CHRG_KEY			30
#define REG_RPS_REL_STATE			31
#define REG_RPS_MIN_VOLTAGE			32
#define REG_RPS_DISCH_VOLTAGE		33
#define REG_RPS_REMAIN_TIME			34
#define REG_RPS_TEST_OK				35
#define REG_RPS_CPU_ID				36
#define REG_RPS_LTC4151_OK			37
#define REG_RPS_ADC_BAT_VOLT		38
#define REG_RPS_ADC_BAT_CURR		39
#define REG_RPS_TEST_MODE			40
//SHT
#define REG_SHT_CONNECTED			50
#define REG_SHT_TYPE				51
#define REG_SHT_TEMPERATURE			52
#define REG_SHT_HUMIDITY			53
//SFP1
#define REG_SFP1_PRESENT			60
#define REG_SFP1_LOS				61
#define REG_SFP1_VENDOR				62
#define REG_SFP1_VENDOR_OUI			63
#define REG_SFP1_VENDOR_PN			64
#define REG_SFP1_VENDOR_REV			65
#define REG_SFP1_IDENTIFIER			66
#define REG_SFP1_CONNECTOR			67
#define REG_SFP1_TYPE				68
#define REG_SFP1_LINK_LEN			69
#define REG_SFP1_FIBER_TEC			70
#define REG_SFP1_MEDIA				71
#define REG_SFP1_SPEED				72
#define REG_SFP1_ENCODING			73
#define REG_SFP1_WAVELEN			74
#define REG_SFP1_NBR				75
#define REG_SFP1_LEN9				76
#define REG_SFP1_LEN50				77
#define REG_SFP1_LEN62				78
#define REG_SFP1_LENC				79
#define REG_SFP1_TEMPER				80
#define REG_SFP1_VOLTAGE			81
#define REG_SFP1_CURRENT			82
#define REG_SFP1_TX_BIAS			83
#define REG_SFP1_TX_POWER			84
#define REG_SFP1_RX_POWER			85

//SFP2
#define REG_SFP2_PRESENT			90
#define REG_SFP2_LOS				91
#define REG_SFP2_VENDOR				92
#define REG_SFP2_VENDOR_OUI			93
#define REG_SFP2_VENDOR_PN			94
#define REG_SFP2_VENDOR_REV			95
#define REG_SFP2_IDENTIFIER			96
#define REG_SFP2_CONNECTOR			97
#define REG_SFP2_TYPE				98
#define REG_SFP2_LINK_LEN			99
#define REG_SFP2_FIBER_TEC			100
#define REG_SFP2_MEDIA				101
#define REG_SFP2_SPEED				102
#define REG_SFP2_ENCODING			103
#define REG_SFP2_WAVELEN			104
#define REG_SFP2_NBR				105
#define REG_SFP2_LEN9				106
#define REG_SFP2_LEN50				107
#define REG_SFP2_LEN62				108
#define REG_SFP2_LENC				109
#define REG_SFP2_TEMPER				110
#define REG_SFP2_VOLTAGE			111
#define REG_SFP2_CURRENT			112
#define REG_SFP2_TX_BIAS			113
#define REG_SFP2_TX_POWER			114
#define REG_SFP2_RX_POWER			115

//RTC
#define REG_RTC_STATUS				120
#define REG_RTC_YEAR				121
#define REG_RTC_MONTH				122
#define REG_RTC_DAY					123
#define REG_RTC_WEEKDAY				124
#define REG_RTC_HOUR				125
#define REG_RTC_MINUTE				126
#define REG_RTC_SECOND				127

//POE
#define REG_POE_ID					130
#define REG_POE_STATE				131
#define REG_POE_BANK				132
#define REG_POE_MODE				133





const char REG_INTSTAT_NAME[] = 			"INTSTAT";
const char REG_INTMASK_NAME[] = 			"INTMASK";
const char REG_HW_VERS_NAME[] = 			"HW_VERS";
const char REG_SW_VERS_NAME[] = 			"SW_VERS";
const char REG_ADC_CH1_NAME[] = 			"ADC_CH1";
const char REG_ADC_CH2_NAME[] = 			"ADC_CH2";
const char REG_ADC_CH3_NAME[] = 			"ADC_CH3";


const char REG_TAMPER_NAME[] = 				"TAMPER";
const char REG_SENSOR1_NAME[] = 			"SENSOR1";
const char REG_SENSOR2_NAME[] = 			"SENSOR2";
const char REG_RELAY1_NAME[] = 				"RELAY1";
const char REG_DEFAULT_BUTTON_NAME[] = 		"DEFAULT_BUTTON";
const char REG_DEFAULT_LED_NAME[] = 		"DEFAULT_LED";
const char REG_DEFAULT_LONG_PRESSED_NAME[]= "DEFAULT_LONG_PRESSED";
//UPS
const char REG_RPS_CONNECTED_NAME[] = 		"RPS_CONNECTED";
const char REG_RPS_HW_VERS_NAME[] = 		"RPS_HW_VERS";
const char REG_RPS_SW_VERS_NAME[] = 		"RPS_SW_VERS";
const char REG_RPS_VAC_NAME[] = 			"RPS_VAC";
const char REG_RPS_BAT_VOLTAGE_NAME[] = 	"RPS_BAT_VOLTAGE";
const char REG_RPS_CHRG_VOLTAGE_NAME[] = 	"RPS_CHRG_VOLTAGE";
const char REG_RPS_BAT_CURRENT_NAME[] = 	"RPS_BAT_CURRENT";
const char REG_RPS_TEMPER_NAME[] = 			"RPS_TEMPER";
const char REG_RPS_LED_STATE_NAME[] = 		"RPS_LED_STATE";
const char REG_RPS_BAT_KEY_NAME[] = 		"RPS_BAT_KEY";
const char REG_RPS_CHRG_KEY_NAME[] = 		"RPS_CHRG_KEY";
const char REG_RPS_REL_STATE_NAME[] = 		"RPS_REL_STATE";
const char REG_RPS_MIN_VOLTAGE_NAME[] = 	"RPS_MIN_VOLTAGE";
const char REG_RPS_DISCH_VOLTAGE_NAME[] = 	"RPS_DISCH_VOLTAGE";
const char REG_RPS_REMAIN_TIME_NAME[] = 	"RPS_REMAIN_TIME";
const char REG_RPS_TEST_OK_NAME[] = 		"RPS_TEST_OK";
const char REG_RPS_CPU_ID_NAME[] = 			"RPS_CPU_ID";
const char REG_RPS_LTC4151_OK_NAME[] = 		"RPS_LTC4151_OK";
const char REG_RPS_ADC_BAT_VOLT_NAME[] = 	"RPS_ADC_BAT_VOLT";
const char REG_RPS_ADC_BAT_CURR_NAME[] = 	"RPS_ADC_BAT_CURR";
const char REG_RPS_TEST_MODE_NAME[] = 		"RPS_TEST_MODE";
//SHT
const char REG_SHT_CONNECTED_NAME[] = 		"SHT_CONNECTED";
const char REG_SHT_TYPE_NAME[] = 			"SHT_TYPE";
const char REG_SHT_TEMPERATURE_NAME[] = 	"SHT_TEMPERATURE";
const char REG_SHT_HUMIDITY_NAME[] = 		"SHT_HUMIDITY";
//SFP1
const char REG_SFP1_PRESENT_NAME[] = 		"SFP1_PRESENT";
const char REG_SFP1_LOS_NAME[] = 			"SFP1_LOS";
const char REG_SFP1_VENDOR_NAME[] = 		"SFP1_VENDOR";
const char REG_SFP1_VENDOR_OUI_NAME[] = 	"SFP1_VENDOR_OUI";
const char REG_SFP1_VENDOR_PN_NAME[] = 		"SFP1_VENDOR_PN";
const char REG_SFP1_VENDOR_REV_NAME[] = 	"SFP1_VENDOR_REV";
const char REG_SFP1_IDENTIFIER_NAME[] = 	"SFP1_IDENTIFIER";
const char REG_SFP1_CONNECTOR_NAME[] = 		"SFP1_CONNECTOR";
const char REG_SFP1_TYPE_NAME[] = 			"SFP1_TYPE";
const char REG_SFP1_LINK_LEN_NAME[] = 		"SFP1_LINK_LEN";
const char REG_SFP1_FIBER_TEC_NAME[] = 		"SFP1_FIBER_TEC";
const char REG_SFP1_MEDIA_NAME[] = 			"SFP1_MEDIA";
const char REG_SFP1_SPEED_NAME[] = 			"SFP1_SPEED";
const char REG_SFP1_ENCODING_NAME[] = 		"SFP1_ENCODING";
const char REG_SFP1_WAVELEN_NAME[] = 		"SFP1_WAVELEN";
const char REG_SFP1_NBR_NAME[] = 			"SFP1_NBR";
const char REG_SFP1_LEN9_NAME[] = 			"SFP1_LEN9";
const char REG_SFP1_LEN50_NAME[] = 			"SFP1_LEN50";
const char REG_SFP1_LEN62_NAME[] = 			"SFP1_LEN62";
const char REG_SFP1_LENC_NAME[] = 			"SFP1_LENC";
const char REG_SFP1_TEMPER_NAME[] = 		"SFP1_TEMPER";
const char REG_SFP1_VOLTAGE_NAME[] = 		"SFP1_VOLTAGE";
const char REG_SFP1_CURRENT_NAME[] = 		"SFP1_CURRENT";
const char REG_SFP1_TX_BIAS_NAME[] = 		"SFP1_TX_BIAS";
const char REG_SFP1_TX_POWER_NAME[] = 		"SFP1_TX_POWER";
const char REG_SFP1_RX_POWER_NAME[] = 		"SFP1_RX_POWER";

//SFP2
const char REG_SFP2_PRESENT_NAME[] = 		"SFP2_PRESENT";
const char REG_SFP2_LOS_NAME[] = 			"SFP2_LOS";
const char REG_SFP2_VENDOR_NAME[] = 		"SFP2_VENDOR";
const char REG_SFP2_VENDOR_OUI_NAME[] = 	"SFP2_VENDOR_OUI";
const char REG_SFP2_VENDOR_PN_NAME[] = 		"SFP2_VENDOR_PN";
const char REG_SFP2_VENDOR_REV_NAME[] = 	"SFP2_VENDOR_REV";
const char REG_SFP2_IDENTIFIER_NAME[] = 	"SFP2_IDENTIFIER";
const char REG_SFP2_CONNECTOR_NAME[] = 		"SFP2_CONNECTOR";
const char REG_SFP2_TYPE_NAME[] = 			"SFP2_TYPE";
const char REG_SFP2_LINK_LEN_NAME[] = 		"SFP2_LINK_LEN";
const char REG_SFP2_FIBER_TEC_NAME[] = 		"SFP2_FIBER_TEC";
const char REG_SFP2_MEDIA_NAME[] = 			"SFP2_MEDIA";
const char REG_SFP2_SPEED_NAME[] = 			"SFP2_SPEED";
const char REG_SFP2_ENCODING_NAME[] = 		"SFP2_ENCODING";
const char REG_SFP2_WAVELEN_NAME[] = 		"SFP2_WAVELEN";
const char REG_SFP2_NBR_NAME[] = 			"SFP2_NBR";
const char REG_SFP2_LEN9_NAME[] = 			"SFP2_LEN9";
const char REG_SFP2_LEN50_NAME[] = 			"SFP2_LEN50";
const char REG_SFP2_LEN62_NAME[] = 			"SFP2_LEN62";
const char REG_SFP2_LENC_NAME[] = 			"SFP2_LENC";
const char REG_SFP2_TEMPER_NAME[] = 		"SFP2_TEMPER";
const char REG_SFP2_VOLTAGE_NAME[] = 		"SFP2_VOLTAGE";
const char REG_SFP2_CURRENT_NAME[] = 		"SFP2_CURRENT";
const char REG_SFP2_TX_BIAS_NAME[] = 		"SFP2_TX_BIAS";
const char REG_SFP2_TX_POWER_NAME[] = 		"SFP2_TX_POWER";
const char REG_SFP2_RX_POWER_NAME[] = 		"SFP2_RX_POWER";

//RTC
const char REG_RTC_STATUS_NAME[] = 			"RTC_STATUS";
const char REG_RTC_YEAR_NAME[] = 			"RTC_YEAR";
const char REG_RTC_MONTH_NAME[] = 			"RTC_MONTH";
const char REG_RTC_DAY_NAME[] = 			"RTC_DAY";
const char REG_RTC_WEEKDAY_NAME[] = 		"RTC_WEEKDAY";
const char REG_RTC_HOUR_NAME[] = 			"RTC_HOUR";
const char REG_RTC_MINUTE_NAME[] = 			"RTC_MINUTE";
const char REG_RTC_SECOND_NAME[] = 			"RTC_SECOND";

//POE
const char REG_POE_ID_NAME[] = 				"POE_ID";
const char REG_POE_STATE_NAME[] = 			"POE_STATE";
const char REG_POE_BANK_NAME[] = 			"POE_BANK";
const char REG_POE_MODE_NAME[] = 			"POE_MODE";

#endif
