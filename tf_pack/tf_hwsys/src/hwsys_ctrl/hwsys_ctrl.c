//
// Created by sheverdin on 3/19/24.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>

#include "gpio_lib/gpio_module.h"
#include "i2c_lib/i2c_module.h"
#include "socket_lib/socket_module.h"
#include "hwsys_ctrl.h"

i2c_param i2CParam;
int sock_fd;
cmd_type_t cmd_type;
REGISTER_ADDR reg_addr;
GPIO_ADDR gpio_offset;
uint8_t value = 0;
INTERFACE interfaceType = MAX_IF;

static int getOpt(int argc, char **argv);
static cmd_type_t getCmdType(char * arg_2);
static void i2c_handler(void);

static int i2c_getHandler(int socket, i2c_param *i2C_param);
static int i2c_setHandler(int socket, i2c_param *i2C_param, uint8_t val);

static void gpio_handler(void);

typedef void (*intreface_handler_t)(void);

intreface_handler_t intrefaceHandler[MAX_IF] =
{
     &i2c_handler,
     &gpio_handler,
};

const unsigned char  cmdTypeArr[CTRL_MAX_CMD_TYPE][6] = {
    {"get"},
    {"set"},
    {"reset"}
};

int main(int argc, char **argv)
{
    //printf("======================= Hello HWSYS_CTRL - VERSION: %s.%s \n", PKG_VERSION, PKG_RELEASE);

    //gpio_test();
    //i2c_test();
    //socket_test();

    // =================================
    getOpt(argc, argv);

    if (interfaceType == MAX_IF) {
        printf("i2c reg or gpio_lib not found\n");
        return EXIT_FAILURE;
    }
    else{
        //printf("interfaceType %d\n ", interfaceType);
        intrefaceHandler[interfaceType]();
    }
    return EXIT_SUCCESS;
}

static int getOpt(int argc, char **argv)
{
    if(argc < 2){
        printf("Incorrect arguments\n");
        exit (1);
    }
    else
    {
        interfaceType = CHIP;
        gpio_offset = get_gpio_addr_by_name(argv[1]);
        if (gpio_offset == GPIO_IDLE) {
            interfaceType = I2C;
            reg_addr = get_i2c_addr_by_name(argv[1], &i2CParam);
            if (reg_addr == SENSOR_IDLE) {
                printf("I2C param not found\n");
                interfaceType = MAX_IF;
            }
        }
        cmd_type = getCmdType(argv[2]);
        if (cmd_type == CTRL_MAX_CMD_TYPE)
        {
            printf("Command not found\n");
            exit(1);
        }

        if(cmd_type == CTRL_SET)
        {
            if (argv[3] == NULL)
            {
                printf("Need set value for this command\n");
                exit(1);
            }
            else if (argv[3] != NULL)
            {
                char *end = NULL;
                value = (uint8_t)strtol(argv[3], &end, 10);
            }
        }
    }
    return 0;
}

void i2c_handler(void)
{
    socket_client(&sock_fd);

    switch (cmd_type)
    {
        case CTRL_GET:
        {
            i2c_getHandler(sock_fd, &i2CParam);
        }
        break;

        case CTRL_SET:
        {
            i2c_setHandler(sock_fd, &i2CParam, value);
        }
        break;

        default:
            printf("Incorrect Type\n");
            close(sock_fd);
            exit(1);
    }
}

static void gpio_handler(void)
{
    //printf("GPIO HANDLER\n");
    gpio_param_t  gpioParam;
    struct gpiod_chip *chip = NULL;
    struct gpiod_line *line = NULL;
    chip = gpio_chipOpen(GPIO_PATH);
    //printf("gpio_chipOpen - gpio_offset = %d\n", gpio_offset);
    line = gpio_getChipLine(chip, gpio_offset);
    //printf("gpio_getChipLine\n");

    gpioParam.chip = chip;
    gpioParam.line = line;
    gpioParam.offset = gpio_offset;
    gpioParam.value = value;
    
    switch (cmd_type)
    {
        case CTRL_GET:
        {
            //printf("cmd_type = GET\n");
            gpio_getHandler( &gpioParam);
        }
        break;
        case CTRL_SET:
        {
            //printf("cmd_type = SET\n");
            gpio_setHandler(&gpioParam);
        }
        break;
        case CTRL_RESET:
        {
            /*socket_client(&sock_fd);
            printf("cmd_type = RESET\n");
            i2c_param i2C_param;
            int len = (int) sizeof(i2C_param.i2c_data);
            i2C_param.i2c_data.lenMSG =  len;
            i2C_param.i2c_data.opcode =  I2C_OPCODE_RESET;
            i2C_param.i2c_data.addr = SENSOR_IDLE;
            i2C_param.i2c_data.eventsHandler = NULL;
            i2C_param.i2c_data.type = 0;

            send_msg(sock_fd, i2C_param.socketMsg, len);
            //CLIENT_STATE clientState = receive_msg(socket, i2C_param->socketMsg);
             */
            gpio_resetHandler(&gpioParam);
        }
        break;
        default:
            printf("Incorrect Type\n");
            close(sock_fd);
            exit(1);
    }
}

static cmd_type_t getCmdType(char *arg_2)
{
    cmd_type_t cmdType = CTRL_MAX_CMD_TYPE;
    for(int i = 0; i < CTRL_MAX_CMD_TYPE; i++)
    {
        if (strcmp(arg_2, (const char *) cmdTypeArr[i]) == 0)
        {
            //printf("Cmd type found\n");
            cmdType = (cmd_type_t) i;
            i = CTRL_MAX_CMD_TYPE;
        }
    }
    return cmdType;
}

int i2c_getHandler(int socket, i2c_param *i2C_param)
{
    uint16_t tmp16;
    int16_t itmp16;
    //printf("------------ >>>> getHandler\n");
    int len = (int) sizeof(i2C_param->i2c_data);
    //printf("i2C_param->i2c_data size = %d\n", len);
    i2C_param->i2c_data.lenMSG =  len;
    i2C_param->i2c_data.opcode =  I2C_OPCODE_READ;

    send_msg(socket, i2C_param->socketMsg, len);
    CLIENT_STATE clientState = receive_msg(socket, i2C_param->socketMsg);

    if (clientState == SOCKET_READ_OK) {
        if(i2C_param->i2c_data.opcode == I2C_OPCODE_RESPONSE)
        {
            if (i2C_param->i2c_data.value[0] == 0xAA && i2C_param->i2c_data.value[1] == 0xAA)
            {
                //printf("i2c_data.opcode == I2C_OPCODE_RESPONSE\n");
                printf("error response\n");
            }

            switch (i2C_param->i2c_data.lenData)
            {
                case 1:
                    //uint8
                    //printf("uint8\n");
                    //printf("addr = %d, %d\n",i2C_param->i2c_data.addr, i2C_param->i2c_data.value[0]);
                    printf("%d\n", i2C_param->i2c_data.value[0]);
                    break;
                case 2:
                   // printf("uint16\n");
                    if (i2C_param->i2c_data.type == TYPE_PSEUDO_FLOAT)
                    {
                        tmp16 = i2C_param->i2c_data.value[0] | i2C_param->i2c_data.value[1] << 8;
                        if (tmp16 < 1000)
                            //printf("addr - %d = %d.%d\n",i2C_param->i2c_data.addr, tmp16 / 10, tmp16 % 10);
                            printf("%d.%d\n", tmp16 / 10, tmp16 % 10);
                        else
                            //printf("addr - %d =  %d.%d\n",i2C_param->i2c_data.addr, tmp16 / 1000, tmp16 % 1000);
                            printf("%d.%d\n", tmp16 / 1000, tmp16 % 1000);
                    }
                    else  if (i2C_param->i2c_data.type == TYPE_FLOAT_SFP)
                    {
                        itmp16 = i2C_param->i2c_data.value[0] | i2C_param->i2c_data.value[1] << 8;
                        if (itmp16 < 1000)
                            //printf("addr - %d = %d.%d\n",i2C_param->i2c_data.addr, tmp16 / 10, tmp16 % 10);
                            printf("%d.%d\n", itmp16 / 10, abs(itmp16 % 10));
                        else
                            //printf("addr - %d =  %d.%d\n",i2C_param->i2c_data.addr, tmp16 / 1000, tmp16 % 1000);
                            printf("%d.%d\n", itmp16 / 1000, abs(itmp16 % 1000));
                    }
                    else
                    {
                        //uint16
                        //printf("addr - %d = %d\n", i2C_param->i2c_data.addr, i2C_param->i2c_data.value[0] | i2C_param->i2c_data.value[1] << 8);
                        printf("%d\n", i2C_param->i2c_data.value[0] | i2C_param->i2c_data.value[1] << 8);
                    }
                    break;
                case 4:
                    //uint32
                    //printf("uint32\n");
                    //printf("addr - %d == %d\n", i2C_param->i2c_data.addr, (uint32_t) (i2C_param->i2c_data.value[0] | i2C_param->i2c_data.value[1] << 8 | i2C_param->i2c_data.value[2] << 16 | i2C_param->i2c_data.value[3] << 24));
                    printf("%d\n",  (uint32_t) (i2C_param->i2c_data.value[0] | i2C_param->i2c_data.value[1] << 8 | i2C_param->i2c_data.value[2] << 16 | i2C_param->i2c_data.value[3] << 24));
                    break;
                default:
                    printf("%s\n", i2C_param->i2c_data.value);
            }
            //printf("recieve: len=%d  [0]=%x [1]=%x [2]=%x [3]=%x\n", sock_msg.len, sock_msg.value[0], sock_msg.value[1], sock_msg.value[2], sock_msg.value[3]);
        }
    }
    else{
        printf("error opcode: %x\n", i2C_param->i2c_data.opcode);
    }
    return 0;
}

int i2c_setHandler(int socket, i2c_param *i2C_param, uint8_t val)
{
    //printf("setHandler\n");
    int32_t i32val = (uint8_t) val;
    i2C_param->i2c_data.opcode = I2C_OPCODE_WRITE;

    i2C_param->i2c_data.value[0] = (uint8_t)i32val;
    i2C_param->i2c_data.value[1] = (uint8_t)(i32val << 8);
    i2C_param->i2c_data.value[2] = (uint8_t)(i32val << 16);
    i2C_param->i2c_data.value[3] = (uint8_t)(i32val << 24);


    //printf("addr = %d\n",i2C_param->i2c_data.addr);
    //printf("value[0] = %d\n",i2C_param->i2c_data.value[0]);
    //printf("value[1] = %d\n",i2C_param->i2c_data.value[1]);
    //printf("value[2] = %d\n",i2C_param->i2c_data.value[2]);
    //printf("value[3] = %d\n",i2C_param->i2c_data.value[3]);

    int len = (int) sizeof(i2C_param->i2c_data);
    send_msg(socket, i2C_param->socketMsg, len);
    return 0;
}



