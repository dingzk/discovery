#ifndef DISCOVERY_HTTP_H
#define DISCOVERY_HTTP_H

#include <string>
#include <unordered_map>
#include <map>
#include <mutex>

#include "connectionpool.h"
#include "url.h"
#include "common/threadonce.h"

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
    uint64_t request_id;
    std::string method;
    int timeout;
    std::string identify;
    Connection *conn;
public:
    Request(){};
    Request(std::string &url, std::string &method = "GET", int timeout = kDefaultTimeout, uint64_t rid);
    ~Request();
    std::string identify();
};

class Response {
public:
    uint64_t request_id;
    std::string body;
    int ret_code;
public:
    Response() {};
};

class Http {
private:
    std::mutex pool_lock{};
    std::mutex buffer_lock{};
    std::map<uint64_t, Request> requests{};
    std::unordered_map<uint64_t, Response> responses{};
    ThreadOnce<Epoll> event_once;
private:
    int add_pool(Request &r);
    std::shared_ptr<ConnectionPool> select_pool(Request &r);
    static void on_read(int sock, short event, void *arg);
    bool set_read_buffer(uint64_t request_id, std::string &data);
    Response & get_read_buffer(uint64_t request_id);

public:
    bool add_url(std::string &url, std::string &method, int timeout);
    std::map<uint64_t, Response> do_call(std::map<uint64_t, Response> &resp);
    Http();
    ~Http();

};


#endif //DISCOVERY_HTTP_H
