//
// Created by sheverdin on 3/19/24.
//

#ifndef TF_HWSYS_GPIO_MODULE_H
#define TF_HWSYS_GPIO_MODULE_H

#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <string.h>

#define GPIO_PATH "/dev/gpiochip0"

void gpio_test(void);

typedef enum {
    RTL_GPIO_SYS_LED 	    = 0,
    RTL_GPIO_SYS_RESET	    = 1,
    RTL_IDLE2               = 2,
    RTL_IDLE3               = 3,
    RTL_IDLE4               = 4,
    RTL_IDLE5               = 5,
    RTL_IDLE6               = 6,
    RTL_IDLE7               = 7,
    RTL_IDLE8               = 8,
    RTL_IDLE9               = 9,
    RTL_IDLE10              = 10,
    RTL_GPIO_IOMCU_RESET    = 11,
    RTL_GPIO_POE_RESET	    = 12,
    RTL_IDLE13              = 13,
    RTL_GPIO_BOOT0		    = 14,
    MAX_GPIO,
    GPIO_IDLE
}GPIO_ADDR;


typedef struct {
    GPIO_ADDR offset;
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    uint8_t value;
}gpio_param_t;

GPIO_ADDR get_gpio_addr_by_name(char *gpio_reg);
struct gpiod_chip* gpio_chipOpen(char *path);
struct gpiod_line* gpio_getChipLine(struct gpiod_chip *chip, int offset);
int gpio_getHandler(gpio_param_t *gpio_param);
int gpio_setHandler(gpio_param_t *gpio_param);
int gpio_resetHandler(gpio_param_t *gpio_param);
void gpio_resetIoMcu();



#endif //TF_HWSYS_GPIO_MODULE_H


