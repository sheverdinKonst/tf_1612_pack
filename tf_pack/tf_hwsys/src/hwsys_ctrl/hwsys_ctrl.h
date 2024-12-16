//
// Created by sheverdin on 3/19/24.
//

#ifndef TF_HWSYS_HWSYS_CTRL_H
#define TF_HWSYS_HWSYS_CTRL_H

#define PKG_VERSION "0.1"
#define PKG_RELEASE "0"

typedef enum    {
    CTRL_GET            = 0,
    CTRL_SET            = 1,
    CTRL_RESET          = 2,
    CTRL_MAX_CMD_TYPE   = 3
}cmd_type_t;

typedef enum {
    I2C     = 0,
    CHIP    = 1,
    MAX_IF
}INTERFACE;


#endif //TF_HWSYS_HWSYS_CTRL_H
