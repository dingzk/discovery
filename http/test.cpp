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

void test_http_single(Http &http)
{
    http.add_url("http://s.weibo.com", "GET", 1000);
    http.add_url("http://register.kailash.weibo.com/naming/service?action=lookup&service=for-test&cluster=test.for%2Fservice&sign=3bb444b2aae425d4e31dc04f717134fd", "GET", 1000);
    http.add_url("http://register.kailash.weibo.com/naming/service?action=lookup&service=unify-search-weibo-com&cluster=com.weibo.search.unify.v2%2Fservice", "GET", 1000);

    std::map<uint64_t, Response> resp;
    http.do_call(resp);

    std::cout << resp.size() << std::endl;

    for (auto i = resp.begin(); i != resp.end(); ++i) {
        Response &res = i->second;
        std::unordered_map<std::string, std::string> &header = res.header;
        std::string &body = res.body;
        std::cout << "requestid: " << i->first << " code: " << res.ret_code << std::endl;

        for (auto iter = header.begin(); iter != header.end(); ++iter) {
            std::cout << iter->first << " : " << iter->second << std::endl;
        }

        std::cout << "requestid: " << i->first << " resp body: " << res.body << std::endl;
    }

}

void test_http_thread(Http &http)
{

}


int main() {
    Http http;
    int i = 10;
    while (i--) {
        test_http_single(http);
    }

    return 0;
}