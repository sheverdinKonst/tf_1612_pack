//
// Created by sheverdin on 6/24/24.
//

#ifndef TF_UTILS_MODULE_H
#define TF_UTILS_MODULE_H

#include <time.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "syslog.h"

typedef unsigned long clock_time_t;

struct timer {
    clock_time_t start;
    clock_time_t interval;
};

void setTestMode(char testMode);
uint8_t* isDebugMode(void);
void printf_array(uint8_t *buff,uint32_t size);

void timer_set(struct timer *t, clock_time_t interval);
void timer_reset(struct timer *t);
int timer_expired(struct timer *t);

FILE *openPipe(const char *cmdStr);
void closePipe(FILE *pipe, char *str);
int getValueFromJson(char *json, char *name, char *value);
int getData_formJson(char* json_data, char *option, char *data);
void toString(int num, char str[]);
void checkValidIp(char ipStr[], int *ipIsValid);
int isValidTime(const char* timeString, struct tm * time);
void getTimeDate(char timeDateStr[32]);

#endif //TF_UTILS_MODULE_H
