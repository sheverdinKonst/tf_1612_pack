/* DMHash - create linux user and generates new MD5 hash for TFortis Device Manager. Creates /etc/dmhash file
* arg1 [-a] - add/modify user
* arg2 - username
* arg3 - password
* arg4(optional) - user level

* arg1 [-d] - delete user
* arg2 - username

* return:
*   EXIT_SUCCESS
*   EXIT_FAILURE


*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <mbedtls/base64.h>
#include <mbedtls/md5.h>
#include <fcntl.h>
#include <uci.h>


#define errcheck(rc, funcName) \
	if (rc == -1) { perror(funcName); return EXIT_FAILURE; }

#define OUTPUT_MAX_LENGTH 256
#define DM_TOK_BUFSIZE 64

#define DEBUGLEVEL    1  //0- no debug, 1 - print debug


typedef struct
{
    char **tokens;
    uint8_t splitLineMax;
}splited_line_t;

int split_line(char *line, const char* delim, splited_line_t *splitedLine)
{
    int bufsize = DM_TOK_BUFSIZE;
    splitedLine->tokens = malloc(bufsize * sizeof(char*));
    splitedLine->splitLineMax = 0;
    char *token;

    if (!splitedLine->tokens)
    {
        fprintf(stderr, "memory error 1\n");
        return EXIT_FAILURE;
    }
    token = strtok(line, delim);

    while (token != NULL)
    {
        splitedLine->tokens[splitedLine->splitLineMax] = token;
        splitedLine->splitLineMax++;

        if (splitedLine->splitLineMax >= bufsize)
        {
            bufsize += DM_TOK_BUFSIZE;
            splitedLine->tokens = realloc(splitedLine->tokens, bufsize * sizeof(char*));
            if (!splitedLine->tokens)
            {
                fprintf(stderr, "memory error 2\n");
                return EXIT_FAILURE;
            }
        }
        token = strtok(NULL, delim);
    }
    splitedLine->tokens[splitedLine->splitLineMax] = NULL;
    splitedLine->splitLineMax--;
    return EXIT_SUCCESS;
}


static FILE *openPipe(const char *cmd)
{
    FILE *pipe = popen(cmd, "r");
    if (!pipe)
    {
        printf("Error to run command\n");
    }
    return pipe;
}

static void closePipe(FILE *pipe)
{
    int status = pclose(pipe);
    if (status == -1)
    {
        printf("Error close process\n");
    }
    else if (status != 0)
    {
        //printf("any error, Status != 0\n");
    }
}

static unsigned int calculate_md5(unsigned char* inbuf, unsigned int inbuf_size,unsigned char* outbuf)
{
    mbedtls_md5_context md5_ctx;
    mbedtls_md5_init(&md5_ctx);
    mbedtls_md5_starts_ret(&md5_ctx);    
    mbedtls_md5_update_ret(&md5_ctx, inbuf, inbuf_size);
    mbedtls_md5_finish_ret(&md5_ctx, outbuf);
    mbedtls_md5_free(&md5_ctx);
    return 0;   
}

int getMACinfo(char *mac)
{
    char find_interface[] = "ethaddr";
    char output[OUTPUT_MAX_LENGTH];
    FILE *pipe = openPipe("fw_printenv ethaddr");
    while (fgets(output, OUTPUT_MAX_LENGTH, pipe) != NULL)
    {
        if (strstr(output, find_interface) != NULL)
        {
            splited_line_t splitLineMAC;
            if(split_line(output, "=", &splitLineMAC)!=EXIT_SUCCESS){
                printf("Error: not found '='\n");
                return -1;
            }
            if(split_line(splitLineMAC.tokens[1], ":", &splitLineMAC)!=EXIT_SUCCESS){
                printf("Error split mac\n");
                return -1;
            }
            sprintf(mac,"%02lX%02lX%02lX%02lX%02lX%02lX",strtol(splitLineMAC.tokens[0], NULL, 16),strtol(splitLineMAC.tokens[1], NULL, 16),strtol(splitLineMAC.tokens[2], NULL, 16),
            strtol(splitLineMAC.tokens[3], NULL, 16),strtol(splitLineMAC.tokens[4], NULL, 16),strtol(splitLineMAC.tokens[5], NULL, 16));
        }
        else{
            printf("Error: not found 'ethaddr'. Check 'fw_printenv' command\n");
            closePipe(pipe);
            return EXIT_FAILURE;
        }
    }
    closePipe(pipe);
    return EXIT_SUCCESS;
}

void write_hash(char *username, const char *hash) {
    char path[128];
    char buffer[80];
    struct uci_ptr ptr;
    struct uci_context *c = uci_alloc_context();

    if (!c) return;

    sprintf(path,"rpcd.%s.hash",username);

    if ((uci_lookup_ptr(c, &ptr, path, true) != UCI_OK) ||
        (ptr.o == NULL || ptr.o->v.string == NULL)) {
        uci_free_context(c);
        printf("Error: fail to uci_lookup_ptr\n");
        return;
    }

    // setting UCI values
    ptr.value = hash;
    if(uci_set(c, &ptr) != UCI_OK)
    {
        printf("[%s][%d] uci_set error.\n", __FILE__, __LINE__);
        uci_free_context(c);
        return;
    }
    // commit changes
    if (uci_commit(c, &ptr.p, false) != UCI_OK) {
        uci_free_context(c);
        uci_perror(c, "UCI Error");
        return;
    }

    printf("UCI section %s: write successful\n",path);
    uci_free_context(c);
}
int main(int argc, char **argv) {
    char buffer[1024];
    char mac[20];
    unsigned char md5[16];
    char opmode;
    char *username;
    char *password;
    char adminlevel = 0;
    char user_found = 0;
    FILE *fp;
    ssize_t cnt;
    int fd;
    char output[OUTPUT_MAX_LENGTH];
    FILE *pipe;

    if(argc < 3){
        printf("DMHASH: incorrect arguments count\n");
		return EXIT_FAILURE;
    }
    if(strcmp(argv[1],"-a")==0){
        opmode = 1;
        username = argv[2];
        password = argv[3];
        if(argc == 5){
            if(strcmp(argv[4],"full")==0){
                adminlevel = 1;
            }
        }
    }
     if(strcmp(argv[1],"-d")==0){
        opmode = 2;
        username = argv[2];
     }


    if(opmode == 1){
        //add user
        if(adminlevel)
            sprintf(buffer,"useradd -d /var/home/%s -m -N -s /bin/runClishAdmin %s",username,username);
        else
            sprintf(buffer,"useradd -d /var/home/%s -m -N -s /bin/runClishUser %s",username,username);
        pipe = openPipe(buffer);
        if(fgets(output, OUTPUT_MAX_LENGTH, pipe) != NULL){
            if (strstr(output, "useradd:") == NULL){
                closePipe(pipe);
                printf("Error: useradd command not found\n",username);
                return EXIT_FAILURE;
            }
            if (strstr(output, "unrecognized") != NULL){
                closePipe(pipe);
                printf("Error: %s\n",output);
                return EXIT_FAILURE;
            }
        }
        closePipe(pipe);
        usleep(1000);
        //edit user group
        sprintf(buffer,"usermod -u 0 -o %s", username);
        pipe = openPipe(buffer);
        if(fgets(output, OUTPUT_MAX_LENGTH, pipe) != NULL){
            if (strstr(output, "usermod:") == NULL){
                closePipe(pipe);
                printf("Error: usermod command not found\n",username);
                return EXIT_FAILURE;
            }
        }
        closePipe(pipe);
        usleep(1000);
        //edit password
        sprintf(buffer,"echo \"%s:%s\" | chpasswd", username,password);
        pipe = openPipe(buffer);
            if(fgets(output, OUTPUT_MAX_LENGTH, pipe) != NULL){
            if (strstr(output, "chpasswd:") == NULL){
                closePipe(pipe);
                printf("Error: chpasswd: command not found\n",username);
                return EXIT_FAILURE;
            }
        }
        closePipe(pipe);


        //create hash if adminlevel
        if(adminlevel){
            getMACinfo(mac);
            printf("MAC: %s\n",mac);

            sprintf(buffer,"%s+%s+%s",mac,username,password);
            calculate_md5((unsigned char*)buffer,strlen(buffer),md5);
            char md5_str[128];
            int i=0;
            int index = 0;
            for (i=0; i<16; i++)
                index += sprintf(&md5_str[index], "%02x", md5[i]);

            printf("%s\n",md5_str);

            //write_hash(username,md5_str);

        }
    }

    //delete user
    if(opmode == 2){
        sprintf(buffer,"userdel -r -f  %s",username);
        pipe = openPipe(buffer);
        if(fgets(output, OUTPUT_MAX_LENGTH, pipe) != NULL){
            if (strstr(output, "userdel:") == NULL){
                closePipe(pipe);
                printf("Error: userdel command not found\n",username);
                return EXIT_FAILURE;
            }
        }
        closePipe(pipe);
    }

	return EXIT_SUCCESS;
}



