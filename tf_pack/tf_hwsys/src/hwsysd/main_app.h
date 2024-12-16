//
// Created by sheverdin on 3/21/24.
//

#ifndef TF_HWSYS_MAIN_APP_H
#define TF_HWSYS_MAIN_APP_H

#endif //TF_HWSYS_MAIN_APP_H
#include "utils_lib/utils_module.h"
#include "thread_lib/thread_module.h"

#define VERSION "0.0.4"
#define MAX_I2C_ERROR (5)

typedef struct {
    uint8_t debugMode;
    uint8_t delay;
    uint32_t timeAlarm;
    uint8_t delayHiPriority;
    uint8_t delayLowPriority;
}app_param;

void mainInit(int argc, char **argv);
void mainApp(void);

