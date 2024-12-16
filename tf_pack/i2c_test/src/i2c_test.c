#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/un.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

#include "registers.h"

#define errcheck(rc, funcName) \
	if (rc == -1) { perror(funcName); return EXIT_FAILURE; }

#define I2C_ADAPTER     "/dev/i2c-0"
#define I2C_ADDR		0x20

static int read_buffer(int fd, unsigned char regaddr, int len, unsigned char *buffer);
static int write_buffer(int fd, unsigned char regaddr, int len, unsigned char *buffer);

void printf_arr(uint8_t *buff,uint32_t size){
char temp[16];
char printf_buff[128];	
	//печатаем по 20 символов в строке
	for(uint8_t i=0;i<(size/10);i++){
		printf_buff[0] = 0;
		for(uint8_t j=0;j<10;j++){
			sprintf(temp,"%02X ",buff[i*10+j]);
			strcat(printf_buff,temp);
		}
		printf("%s\r\n",printf_buff);
	}
	//для остатка
	printf_buff[0] = 0;
	for(uint8_t i=0;i<size%10;i++){
		sprintf(temp,"%02X ",buff[(size/10)*10+i]);
		strcat(printf_buff,temp);
	}
	printf("%s\r\n",printf_buff);
}

int main(int argc, char **argv) {
    int i2c_fd;
	unsigned char read_buf[1000] = { 0x00 };
	unsigned char msg[1000] = {0};	

	printf("Start I2C\n");

	/*
	 * Open I2C file descriptor.
	 */
	i2c_fd = open(I2C_ADAPTER, O_RDWR);
	if (i2c_fd < 0) {
		printf("Unable to open i2c file\n");
		return EXIT_FAILURE;
	}
	printf("open i2c interface\n");

    uint32_t cnt=0;
    while(1){
        if (read_buffer(i2c_fd, 141, 137, msg) == EXIT_SUCCESS) {
            printf("read i2c HI:\n");     
            //printf_arr(msg,481);   
        } 
        if(cnt % 10 == 0){
            if (read_buffer(i2c_fd, 143, 612, msg) == EXIT_SUCCESS) {
                printf("read i2c LOW:\n");     
                //printf_arr(msg,481);   
            } 
        }
        cnt++;
        sleep(3);
    }
                
    return 0;
}



//read I2C data
static int read_buffer(int fd, unsigned char regaddr, int len, unsigned char *buffer)
{
        struct i2c_rdwr_ioctl_data data;
        struct i2c_msg messages[2];
        unsigned char regaddr_[2];
        regaddr_[0] = regaddr;
        regaddr_[1] = len;

        /* 
         * .addr - 
         * .flags -  (0 - w, 1 - r)
         * .len - 
         * .buf - 
         */
        messages[0].addr = I2C_ADDR;
        messages[0].flags = 0;//write
        messages[0].len = 2;
        messages[0].buf = regaddr_;

        messages[1].addr = I2C_ADDR;
        messages[1].flags = 1;//read
        messages[1].len = len;
        messages[1].buf = buffer;
 
        data.msgs = messages;
        data.nmsgs = 2;
 
        if (ioctl(fd, I2C_RDWR, &data) < 0){
            printf("Read: cant send data!\n");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
}

//write I2C data
static int write_buffer(int fd, unsigned char regaddr, int len, unsigned char *buffer)
{
        unsigned char tmp[10];
        struct i2c_rdwr_ioctl_data data;
        struct i2c_msg messages[1];
       
        /* 
         * .addr - 
         * .flags -  (0 - w, 1 - r)
         * .len - 
         * .buf - 
         */


        messages[0].addr = I2C_ADDR;
        messages[0].flags = 0;//write
        messages[0].len = len+2;
        messages[0].buf = tmp;
        messages[0].buf[0] = regaddr;
        messages[0].buf[1] = len;
        for(uint8_t i=0;i<len;i++)
         	messages[0].buf[i+2] = buffer[i]; 
 
        data.msgs = messages;
        data.nmsgs = 1;

        if (ioctl(fd, I2C_RDWR, &data) < 0){
            printf("Write: cant send data!\n");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
}
