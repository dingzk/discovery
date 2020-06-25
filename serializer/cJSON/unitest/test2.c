//
// Created by zhenkai on 2020/2/25.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cJSON.h"


int main(void)
{
    const char *string = "{\"status\":0,\"msg\":\"\",\"data\":[\"10.75.30.84\",\"10.75.30.85\",\"10.75.30.89\"]}";


    cJSON* cjson = cJSON_Parse(string);

    char *print;

    if (cjson == NULL) {
        printf("cjson error…");
        exit(1);
    }


    cJSON* test_arr = cJSON_GetObjectItem(cjson, "data");

    if (test_arr == NULL) {
        exit(1);
    }

    print = cJSON_Print(test_arr);
    printf("%s\n", print);
    free(print);

    cJSON *element = NULL;

    char *ip;
    cJSON_ArrayForEach(element, test_arr) {
        ip = cJSON_GetStringValue(element);
        if (ip != NULL) {
            printf("%s\n", ip);
        }
    }


    cJSON_Delete(cjson);

    return 0;

}