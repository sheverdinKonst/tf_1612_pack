//
// Created by sheverdin on 3/19/24.
//

#include "gpio_module.h"

const char gpio_name[MAX_GPIO][16] =
{
    {"SYS_LED"},        // 0
    {"SYS_RESET"},      // 1
    {"empty"},          // 2
    {"empty"},          // 3
    {"empty"},          // 4
    {"empty"},          // 5
    {"empty"},          // 6
    {"empty"},          // 7
    {"empty"},          // 8
    {"empty"},          // 9
    {"empty"},          // 10
    {"IOMCU_RESET"},    // 11
    {"empty"},          // 12
    {"POE_RESET"},      // 13
    {"BOOT0"},          // 14
};


gpio_param_t gpio_addrArr[MAX_GPIO] =
{
    {.offset = RTL_GPIO_SYS_LED},
    {.offset = RTL_GPIO_SYS_RESET},
    {.offset = RTL_IDLE2 },
    {.offset = RTL_IDLE3 },
    {.offset = RTL_IDLE4 },
    {.offset = RTL_IDLE5 },
    {.offset = RTL_IDLE6 },
    {.offset = RTL_IDLE7 },
    {.offset = RTL_IDLE8 },
    {.offset = RTL_IDLE9 },
    {.offset = RTL_IDLE10},
    {.offset = RTL_GPIO_IOMCU_RESET},
    {.offset = RTL_GPIO_POE_RESET},
    {.offset = RTL_IDLE13},
    {.offset = RTL_GPIO_BOOT0},
};

void gpio_test(void)
{
    printf("Hello from GPIO\n");
}

GPIO_ADDR get_gpio_addr_by_name(char *gpio_reg)
{
    GPIO_ADDR gpioAdr = GPIO_IDLE;
    for(int i = 0; i < MAX_GPIO; i++)
    {
        //printf("i = %d gpio_name = %s\n", i, gpio_name[i]);
        if(strcmp(gpio_reg, gpio_name[i]) == 0)
        {
            //printf("FOUND i = %d gpio_name = %s\n", i, gpio_name[i]);
            gpioAdr = (GPIO_ADDR) i;
            i = MAX_GPIO;
        }
    }
    return gpioAdr;
}

struct gpiod_chip* gpio_chipOpen(char *path)
{
    struct gpiod_chip *chip = gpiod_chip_open(path);
    if (!chip) {
        openlog("gpio_ctrl", LOG_PID, LOG_USER);
        syslog(LOG_ERR, "ERROR gpiod_chip_open" );
        closelog();
        exit (EXIT_FAILURE);
    }
    return chip;
}

struct gpiod_line* gpio_getChipLine(struct gpiod_chip *chip, int offset)
{
    //printf("gpiod_chip_get_line before\n");
    struct gpiod_line *line = gpiod_chip_get_line(chip, offset);
    if (!line) {
        openlog("gpio_ctrl", LOG_PID, LOG_USER);
        syslog(LOG_ERR, "ERROR gpiod_chip_get_line %d\n", offset);
        closelog();
        gpiod_chip_close(chip);
        exit (EXIT_FAILURE);
    }
    return line;
}

int gpio_getHandler(gpio_param_t *gpio_param)
{
    int value, req;
    req = gpiod_line_request_input(gpio_param->line, "gpio_lib");
    if (req) {
        openlog("gpio_ctrl", LOG_PID, LOG_USER);
        syslog(LOG_ERR, "ERROR gpiod_line_request_input %d\n", gpio_param->offset);
        closelog();
        gpiod_chip_close(gpio_param->chip);
        return -1;
    }
    value = gpiod_line_get_value(gpio_param->line);
    req = gpiod_line_request_output(gpio_param->line, "gpio_lib", value);
    printf("%d\n",value);
    gpiod_chip_close(gpio_param->chip);
    return 0;
}

int gpio_setHandler(gpio_param_t *gpio_param)
{
    int req;
    req = gpiod_line_request_output(gpio_param->line, "gpio_lib", 0);
    if (req) {
        openlog("gpio_ctrl", LOG_PID, LOG_USER);
        syslog(LOG_ERR, "ERROR gpiod_line_request_output %d\n",gpio_param->offset);
        closelog();
        gpiod_chip_close(gpio_param->chip);
        return -1;
    }
    gpiod_line_set_value(gpio_param->line, gpio_param->value);
    //printf("ok\n");
    gpiod_chip_close(gpio_param->chip);
    return 0;
}

int gpio_resetHandler(gpio_param_t *gpio_param) {
    int req;
    req = gpiod_line_request_output(gpio_param->line, "gpio", 0);
    if (req) {
        openlog("gpio_ctrl", LOG_PID, LOG_USER);
        syslog(LOG_ERR, "ERROR gpiod_line_request_output %d\n", gpio_param->offset);
        closelog();
        gpiod_chip_close(gpio_param->chip);
        return -1;
    }
    gpiod_line_set_value(gpio_param->line, 0);
    sleep(1);
    gpiod_line_set_value(gpio_param->line, 1);
    //printf("OK \n");
    gpiod_chip_close(gpio_param->chip);
    return 0;
}

void gpio_resetIoMcu()
{
    gpio_param_t gpioParam;
    GPIO_ADDR gpioAddr = RTL_GPIO_IOMCU_RESET;
    struct gpiod_chip *chip = NULL;
    struct gpiod_line *line = NULL;
    chip = gpio_chipOpen(GPIO_PATH);
    //printf("RESET event gpio_chipOpen - gpio_offset = %d\n", gpioAddr);
    line = gpio_getChipLine(chip, gpioAddr);
    gpioParam.chip = chip;
    gpioParam.line = line;
    gpioParam.offset = gpioAddr;
    gpioParam.value = 0;

    int req = gpiod_line_request_output(gpioParam.line, "gpio", 0);
    if (req) {
        openlog("gpio_ctrl", LOG_PID, LOG_USER);
        syslog(LOG_ERR, "ERROR gpiod_line_request_output %d\n", gpioParam.offset);
        closelog();
        gpiod_chip_close(gpioParam.chip);
    }
    else
    {
        gpiod_line_set_value(gpioParam.line, 0);
        sleep(1);
        gpiod_line_set_value(gpioParam.line, 1);
        printf("io_mcu reset OK \n");
        gpiod_chip_close(gpioParam.chip);
    }
}


