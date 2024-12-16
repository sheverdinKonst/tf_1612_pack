//
// Created by sheverdin on 6/24/24.
//

#ifndef TF_THREAD_MODULE_H
#define TF_THREAD_MODULE_H

#include <pthread.h>
#include <stdint.h>
#include "stdio.h"
#include <stdlib.h>

typedef void *(*mainThreads_t)(void*);
uint8_t createThread(uint8_t maxThreads, mainThreads_t *func);
uint8_t joinThread(uint8_t maxThreads);

#endif //TF_THREAD_MODULE_H
