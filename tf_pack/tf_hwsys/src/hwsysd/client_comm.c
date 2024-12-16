//
// Created by sheverdin on 3/22/24.
//

#include "client_comm.h"


CLIENT_STATE clientHandler(int socket_fd)
{
    i2c_param i2cRec;
    i2c_param i2cDataSend;

    CLIENT_STATE clientState = receive_msg(socket_fd, i2cRec.socketMsg);
    //printf("clientHandler\n");
    if (i2cRec.i2c_data.opcode == I2C_OPCODE_READ)
    {
        //printf("i2c_data.opcode == I2C_OPCODE_READ\n");
        //printf("clientState = %d\n", clientState);
        //printf("i2CParam.i2c_data.lenMSG     = %d\n", i2cRec.i2c_data.lenMSG);
        //printf("i2CParam.i2c_data.addr       = %d\n", i2cRec.i2c_data.addr);
        //printf("i2CParam.i2c_data.opcode     = %d\n", i2cRec.i2c_data.opcode);
        //printf("i2CParam.i2c_data.type       = %d\n", i2cRec.i2c_data.type);
        //printf("i2CParam.i2c_data.lenData    = %d\n", i2cRec.i2c_data.lenData);
        if (clientState == SOCKET_READ_OK)
        {
            i2cRec.i2c_data.opcode = I2C_OPCODE_RESPONSE;
        }
    }
    if (i2cRec.i2c_data.opcode == I2C_OPCODE_RESPONSE)
    {
        //fprintf(stderr, "i2c_data.opcode == I2C_OPCODE_RESPONSE\n");
        i2cRec.i2c_data.lenMSG = sizeof(i2cRec.i2c_data);

        //printf("i2c_data.lenMSG = %d\n", i2cRec.i2c_data.lenMSG);
        i2c_getData(&i2cDataSend.i2c_data, i2cRec.i2c_data.addr, i2cRec.i2c_data.lenMSG);
        //printf("addr =  %d -- i2c_value %d\n", i2cRec.i2c_data.addr,  i2cDataSend.i2c_data.value[0] );
        i2cDataSend.i2c_data.opcode = I2C_OPCODE_RESPONSE;
        //printf("i2cDataSend.i2c_data.lenMSG     = %d\n", i2cDataSend.i2c_data.lenMSG);
        //printf("i2cDataSend.i2c_data.addr       = %d\n", i2cDataSend.i2c_data.addr);
        //printf("i2cDataSend.i2c_data.opcode     = %d\n", i2cDataSend.i2c_data.opcode);
        //printf("i2cDataSend.i2c_data.type       = %d\n", i2cDataSend.i2c_data.type);
        //printf("i2cDataSend.i2c_data.lenData    = %d\n", i2cDataSend.i2c_data.lenData);
        send_msg(socket_fd, i2cDataSend.socketMsg, i2cDataSend.i2c_data.lenMSG);
    }
    if (i2cRec.i2c_data.opcode == I2C_OPCODE_WRITE)
    {
        i2cRec.i2c_data.lenMSG = sizeof(i2cRec.i2c_data);
        //printf("i2c_data.opcode == I2C_OPCODE_WRITE\n");
        i2c_setData(&i2cRec, i2cRec.i2c_data.addr, i2cRec.i2c_data.lenMSG);
    }
    if (i2cRec.i2c_data.opcode == I2C_OPCODE_RESET)
    {
        //printf( "i2c_data.opcode == I2C_OPCODE_RESET\n");
        clientState = SOCKET_RESET;
        //gpio_param_t  gpioParam;
        //gpio_resetHandler(&gpioParam);
    }
    return clientState;
}


