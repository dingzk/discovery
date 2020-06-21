#include <stdio.h>
#include <iostream>
#include "url.h"
#include "http.h"


void  test_url(void)
{
    const char *url = "https://weibo.com/dasdad/d/?dsada=dad";
    URL *Url = url_parse(url);
    assert(Url);
    printf("scheme %s\n", Url->scheme);
    printf("hostname %s\n", Url->hostname);
    printf("port %d\n", Url->port);
    printf("path %s\n", Url->path);
    printf("query %s\n", Url->query);
    url_free(Url);
    return;
}

void test_http_single()
{
    Http http;
    http.add_url("https://s.weibo.com", "GET", 1000);

    std::map<uint64_t, Response> resp;
    http.do_call(&resp);

    for (auto i = resp.begin(); i != resp.end(); ++i) {
        std::cout << "requestid: " << i->first << " resp : " << i->second->ret_code << std::endl;
    }

}

void test_http_multi()
{

}


int main() {
    test_http_single();

    return 0;
}