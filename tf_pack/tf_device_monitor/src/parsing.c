//
// Created by sheverdin on 10/4/23.
//

#include "parsing.h"
#include "dm_mainHeader.h"
#include <jansson.h>


#define DEVICE_TYPE_LEN (15)

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

void replaceSymbols(char *str, char oldSym, char newSym)
{
    for (int i = 0; str[i] != '\0'; i++)
    {
        if (str[i] == oldSym)
        {
            str[i] = newSym;
        }
    }
}

void getDeviceType(search_out_msg_t *searchOutMsg, char *modelType)
{
    //uint8_t deviceTypeFlag = 0;
    //for (int i = 0; i < MAX_DEVICE_TYPE; i++)
    //{
    //    if (strcmp(modelType, modelTable[i]) == 0)
    //    {
    //        deviceTypeFlag = 1;
    //        searchOutMsg->struct1.dev_type = i;
    //        i = MAX_DEVICE_TYPE;
    //    }
    //}
    //if(!deviceTypeFlag)
    //{
    //    openlog("tfdm", LOG_PID, LOG_USER);
    //    syslog(LOG_ERR, "Device Type not Found");
    //    closelog();
    //}
}

void extractValue(const char* str, char *extractedStr)
{
    size_t len = strlen((char*)str);
    int start = -1;
    int end = -1;

    for (int i = 0; i < len; i++)
    {
        if (str[i] == '[')
        {
            start = i + 1;
        }
        else if (str[i] == ']')
        {
            end = i;
            break;
        }
    }

    if (start != -1 && end != -1 && end > start)
    {
        extractedStr[SETTINGS_VALUE_LENGTH];
        strncpy((char*)extractedStr, (char*)&str[start], end - start);
        extractedStr[end - start] = '\0';
    }
}

void removeCharacter(char *str, char removeChar)
{
    int length = strlen(str);
    int currentIndex = 0;

    for (int i = 0; i < (length); i++)
    {
        if (str[i] != removeChar)
        {
            str[currentIndex] = str[i];
            currentIndex++;
        }
    }
    str[currentIndex] = '\0';
}
int getData_formJson(char* str, char *option, char *data)
{
    int len = strlen(str);
    str[len-2] = '\0';
    strcpy(data, "");

    printf("str in = %s\n", str);
    char json_str[128];
    strcpy(json_str, "{");
    strcat(json_str, str);
    strcat(json_str, "}");
    json_t *root;
    json_error_t error;
    printf("json_str = %s\n", json_str);
    root = json_loads(json_str, 0, &error);
    if (!root) {
        fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
        return 1;
    }

    if(!json_is_array(root))
    {
        const char *hash_str;
        json_t *hash;
        hash = json_object_get(root, option);
        if(!json_is_string(hash))
        {
            json_decref(root);
            return 2;
        }
        hash_str = json_string_value(hash);
        strcpy(data, hash_str);
    }
    json_decref(root);
    return 8;
}
