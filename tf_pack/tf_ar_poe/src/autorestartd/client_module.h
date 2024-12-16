//
// Created by sheverdin on 6/24/24.
//

#ifndef TF_AR_POE_CLIENT_MODULE_H
#define TF_AR_POE_CLIENT_MODULE_H

#include "socket_lib/socket_module.h"
#include "../global_includs.h"

CLIENT_STATE clientHandler(int fd);
uint8_t *is_ReceivedMsg(void);
void set_notRecivedMsg(void);

#endif //TF_AR_POE_CLIENT_MODULE_H

