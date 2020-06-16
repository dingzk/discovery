#include <stdio.h>
#include "url.h"

int main() {
    const char *url = "https://weibo.com/dasdad/d/?dsada=dad";
    URL *Url = url_parse(url);
    assert(Url);
    printf("scheme %s\n", Url->scheme);
    printf("hostname %s\n", Url->hostname);
    printf("port %d\n", Url->port);
    printf("path %s\n", Url->path);
    printf("query %s\n", Url->query);
    url_free(Url);
    return 0;
}