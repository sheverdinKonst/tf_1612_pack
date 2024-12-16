//
// Created by sheverdin on 3/26/24.
//

#ifndef TF_HWSYS_THREAD_MODULE_H
#define TF_HWSYS_THREAD_MODULE_H

#include <pthread.h>
#include <stdint.h>
#include "stdio.h"
#include <stdlib.h>

#define THREADS_NUM (3)

typedef enum {
    ERR_OK              = 0,
    ERR_CREATE_THREAD   = 1
}error_code_t;

typedef void *(*mainThreads_t)(void*);
uint8_t createThread(uint8_t maxThreads, mainThreads_t *func);
uint8_t joinThread(uint8_t maxThreads);


#endif //TF_HWSYS_THREAD_MODULE_H
