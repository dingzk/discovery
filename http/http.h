#ifndef DISCOVERY_HTTP_H
#define DISCOVERY_HTTP_H

#include <string>
#include <unordered_map>
#include <map>
#include <mutex>

#include "common/threadonce.h"
#include "connectionpool.h"
#include "events/epoll.h"
#include "url.h"

static const auto kPMask = 0xfffffffffff00000;
static const auto kSMask = 0x000fffff;

const int kDefaultTimeout = 1000; //ms
const int kDefaultConnectTimeout = 100; //ms
const int kDefaultPoolSize = 3;
const int kDefaultKeepAliveTimeout = 10; //s
const int kDefaultWaitTime = 10; //ms

class Request {
public:
    URL *Url;
    Connection *conn;
    uint64_t request_id;
    int timeout;
    std::string method;
private:
    std::string identify;
public:
    Request(std::string &url, std::string &me = "GET", int t = kDefaultTimeout);
    ~Request();
    std::string &identify();
    void http_build_query(std::string &data);
};

class Response {
public:
    uint64_t request_id;
    std::unordered_map<std::string, std::string> header;
    std::string body;

    std::string protocolVersion;
    std::string descr;
    int ret_code;

public:
    bool read_header(int sock, std::string &raw_header);
    bool parse_header(const std::string &raw_header);
    bool read_body(int sock, std::string &raw_body);
    bool parse_body(const std::string &raw_body);

private:
    std::string result;

};

class Http {
private:
    std::mutex pool_lock{};
    std::mutex buffer_lock{};
    std::map<uint64_t, Request *> requests{};
    std::unordered_map<uint64_t, Response *> responses{};
    std::unordered_map<std::string, std::shared_ptr<ConnectionPool>> pool_map{};
    ThreadOnce<Epoll> event_once;
    typedef struct {
        uint64_t request_id;
        Http *http;
    } ID_MAP;
    std::unordered_map<uint64_t, ID_MAP *> id_map{};
private:

    int add_pool(Request *rhs);
    std::shared_ptr<ConnectionPool> select_pool(Request *rhs);
    static void on_read(int sock, short event, void *arg);
    bool set_read_buffer(uint64_t request_id, Response *rhs);
    Response* get_read_buffer(uint64_t request_id);

public:
    bool add_url(std::string &url, std::string &method, int timeout);
    int do_call(std::map<uint64_t, Response> &resp);
    Http();
    ~Http();

};


#endif //DISCOVERY_HTTP_H
