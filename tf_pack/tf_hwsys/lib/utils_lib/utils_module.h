//
// Created by sheverdin on 3/21/24.
//


#ifndef TF_HWSYS_UTILS_MODULE_H
#define TF_HWSYS_UTILS_MODULE_H

#include <stdint.h>
typedef unsigned long clock_time_t;
struct timer {
  clock_time_t start;
  clock_time_t interval;
};

void setTestMode(char testMode);
uint8_t isDebugMode(void);
void printf_array(uint8_t *buff,uint32_t size);
void runTimer(uint32_t *timeNow, uint8_t delay_sec);

void timer_set(struct timer *t, clock_time_t interval);
void timer_reset(struct timer *t);
int timer_expired(struct timer *t);

#endif //TF_HWSYS_UTILS_MODULE_H
