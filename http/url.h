//
// Created by zhenkai on 2020/6/15.
//

#ifndef DISCOVERY_URL_H
#define DISCOVERY_URL_H

#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct _URL {
    const char    *scheme;
    char    *hostname;
    char    *path;
    char    *query;
    int     port;
} URL;

URL *url_parse(const char *url);

void url_free(URL *Url);

#endif //DISCOVERY_URL_H
