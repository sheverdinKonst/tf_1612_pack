//
// Created by sheverdin on 6/24/24.
//

#ifndef TF_AR_POE_INIT_MODULE_H
#define TF_AR_POE_INIT_MODULE_H

#include "../global_includs.h"

typedef enum
{
    TEST_DELAY = 0,
    TEST_NUM   = 1,
    MAX_RESET  = 2,
    MAIN_TIME  = 3,
    MAX_PARM   = 4
}DEMON_PARAM_e;

typedef struct {
    char    *optionName;
    uint8_t value;
}config_type;

void getConfig(void);
void getPortsParam(void);
void rebootParamInit(void);
void runTimer(void);
void getConfigOptions(config_type *configOptions, uint8_t index);
void getPortParam(uint8_t index, uint8_t isReboot);

config_type* init_getConfig(void);

#endif //TF_AR_POE_INIT_MODULE_H
