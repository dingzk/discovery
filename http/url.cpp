//
// Created by zhenkai on 2020/6/15.
//

#include "url.h"

URL *url_parse(const char *url)
{
    URL *Url = (URL *)malloc(sizeof(URL));
    if (!Url) {
        free(Url);
        return NULL;
    }
    memset(Url, 0, sizeof(URL));
    char *p = strdup(url);
    char *p_ori = p;
    if (memcmp(p, "https://", 8) == 0) {
        Url->scheme = "https";
        p += 8;
    } else if (memcmp(url, "http://", 7) == 0) {
        Url->scheme = "http";
        p += 7;
    }
    if (!Url->scheme) {
        free(p_ori);
        free(Url);
        return NULL;
    }
    char *temp;
    if ((temp = strchr(p, '/')) != NULL) {
        assert(temp > p);
        Url->hostname = strndup(p, temp - p);
        p = temp;
        if ((temp = strchr(p, '?')) != NULL) {
            Url->path = strndup(p, temp - p);
            if (*(temp + 1) != '\0') {
                Url->query = strdup(temp + 1);
            }
        } else {
            if (*p != '\0') {
                Url->path = strdup(p);
            }
        }
    } else if ((temp = strchr(p, '?')) != NULL) {
        assert(temp > p);
        Url->hostname = strndup(p, temp - p);
        p = temp + 1;
        if (*p != '\0') {
            Url->query = strdup(p);
        }
    } else {
        if (strlen(p) > 0) {
            Url->hostname = strdup(p);
        }
    }
    assert(Url->hostname);
    if ((temp = strchr(Url->hostname, ':')) != NULL) {
        *temp = '\0';
        Url->port = atoi(temp + 1);
    }
    Url->port = Url->port == 0 ? 80 : Url->port;
    Url->path = Url->path == NULL ? strdup("/") : Url->path;

    free(p_ori);

    return Url;
}

void url_free(URL *Url)
{
    if (!Url) {
        return;
    }
    if (Url->hostname) free(Url->hostname);
    if (Url->path) free(Url->path);
    if (Url->query) free(Url->query);

    free(Url);
}