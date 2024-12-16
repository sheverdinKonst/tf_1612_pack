//
// Created by sheverdin on 12/28/23.
//

#ifndef TF_POE_RESTART_CTRL_PARSING_COMMAND_H
#define TF_POE_RESTART_CTRL_PARSING_COMMAND_H

#include "../global_includs.h"

#define COMMAND_LENGTH (10)

uint8_t getCmdType(char *cmd, CMD_TYPE_e *cmdTypeE);
void getPortNum(char *portStr, uint8_t *portNum);
void printBufferJson(resetPort_U *resetStatic);




#endif //TF_POE_RESTART_CTRL_PARSING_COMMAND_H
