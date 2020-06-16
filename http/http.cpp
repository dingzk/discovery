//
// Created by zhenkai on 2020/6/16.
//

#include "http.h"
#include <iostream>
#include "events/epoll.h"

#include <string.h>
#include <sys/socket.h>
#include <csignal>
#include <poll.h>
#include <unistd.h>
#include <netinet/in.h>
#include <algorithm>

static uint64_t generate_request_id()
{
    static std::atomic<uint64_t> id_offset;
    struct timespec tn{};
    clock_gettime(CLOCK_REALTIME, &tn);
    return (tn.tv_nsec & kPMask) | (id_offset++ & kSMask);
}

Request::Request(std::string &url, std::string &m, int t, uint64_t rid) : request_id(rid), method(m), timeout(t), conn(
        nullptr)
{
    Url = url_parse(url.c_str());
    if (Url == nullptr) {
        throw ;
    }
    transform(method.begin(), method.end(), method.begin(), ::toupper);
    if (memcmp(method.c_str(), "GET", 3) != 0) {
        method = "POST";
    }
}

std::string & Request::identify()
{
    if (identify.empty()) {
        identify = Url->hostname + ":" + std::to_string(Url->port)
    }
    return identify;
}

Request::~Request()
{
    url_free(Url);
}

static void clear_map_data()
{
    requests.clear();
    requests.rehash(0);
    responses.clear();
    responses.rehash(0);
}

int Http::add_pool(Request &r)
{
    std::string &identify = r.identify();
    if (identify.size() <= 0) {
        return -1;
    }
    if (pool_map_.find(identify) == pool_map_.end()) {
        pool_map_[identify] = std::shared_ptr<motan_channel::ConnectionPool> (new motan_channel::ConnectionPool(r.Url->hostname, r.Url->port, kDefaultPoolSize, kDefaultKeepAliveTimeout, kDefaultConnectTimeout));
        return 1;
    }

#ifdef DEBUG
    std::cout << "add pool " + identify << std::endl;
#endif

    return 0;
}

std::shared_ptr<ConnectionPool> Http::select_pool(Request &r)
{
    std::lock_guard<std::mutex> guard(pool_lock);
    std::string &identify = r.identify();
    if (pool_map_.find(identify) == pool_map_.end()) {
        return nullptr;
    }

    return pool_map_[identify];
}

bool Http::add_url(std::string &url, std::string &method, int timeout)
{
    uint64_t request_id = generate_request_id();
    Request req(url, method, timeout, request_id);
    requests.insert({request_id, req});
    add_pool(req);
}

static void parse_request_body (std::string &data, Request &req)
{
    bool isget = false;
    if (memcmp(req.method.c_str(), "GET") == 0) {
        isget = true;
        data = req.method + " " + req.Url->path + "?" + req.Url->query + " HTTP/1.1" + "\r\n";
    } else {
        data = req.method + " " + req.Url->path + " HTTP/1.1" + "\r\n";
    }
    data += "Host:" + req.Url->hostname + "\r\n";
    data =  "Connection:keep-alive\r\n";
    data =  "User-Agent: wbsearch\r\n";
    data =  "Accept: */*\r\n";
    if (!isget) {
        data =  "Content-Length:" + std::to_string(strlen(req.Url->query)) + "\r\n";
        data =  "Content-Type: application/x-www-form-urlencoded\r\n";
    }
    data += "\r\n";
    if (!isget) {
        data += req.Url->query;
    }

}

static void parse_response_body (std::string &data, Response &resp)
{

}

static void write_sock(int fd, const char * buffer_in, int len)
{
    signal(SIGPIPE, SIG_IGN);
    ssize_t write_pos = 0, temp_len = 0;
    while (write_pos < len) {
        if ((temp_len = write(fd, buffer_in + write_pos, len - write_pos)) <= 0) {
            if (temp_len < 0) {
                if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) {
                    continue;
                }
//                LOG_ERROR("Write socket error errno : {} !", strerror(errno));
                return -1;
            } else {
//                LOG_ERROR("Write socket error return : {} !", temp_len);
                return -1;
            }
        }

        write_pos += temp_len;
    }
    return 0;
}

void Http::on_read(int sock, short event, void *arg)
{
    ssize_t read_pos = 0, temp_len = 0;

    struct pollfd pfds[1];
    pfds[0].fd = sock;
    pfds[0].events = POLLIN | POLLHUP | POLLERR;
    pfds[0].revents = 0;
    while (read_pos < len) {
        if (poll(pfds, 1, kDefaultWaitTime) <= 0) {
            if (errno == EINTR) {
                continue;
            }
//            LOG_ERROR("Read socket error:{}, poll for read timeout!", strerror(errno));
            return false;
        }
        if (!(pfds[0].revents & POLLIN)) {
            return false;
        }
        temp_len = read(sock, receive_buf->buffer_ + receive_buf->write_pos_, len - read_pos);
        if (temp_len == 0) {
//            LOG_ERROR("peer closed socket: {}!", sock);
            return false;
        } else if (temp_len < 0) {
            if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) {
                continue;
            }
//            LOG_ERROR("Read socket error:{} !", strerror(errno));
            return false;
        }
        read_pos += temp_len;
        receive_buf->write_pos_ += temp_len;
    }

    return (read_pos == len);
}

std::map<uint64_t, Response> Http::do_call(std::map<uint64_t, Response> &resp)
{
    int request_num = requests.size();
    if (request_num <= 0) {
        return -1;
    }
    Event *event_ = event_once.fetch();
    uint64_t request_id;
    struct event_s ev[request_num];
    std::shared_ptr<ConnectionPool> pool_;
    int max_timeout = 0;
    int need_poll_fds = 0;
    for (auto iter = requests.begin(); iter != requests.end(); ++ iter) {
        Request &req = *iter;
        pool_ = select_pool(req);
        if (pool_ == nullptr) {
            std::cout << "get a null connection pool" << std::endl;
            continue;
        }
        Connection * conn = pool_->fetch();
        if (conn == nullptr) {
            std::cout << "get null connection" << std::endl;
            continue;
        }
        req.conn = conn;
        int fd = conn->get_connect_sock();
#ifdef DEBUG
        char ipstr[INET_ADDRSTRLEN];
        conn->sock_to_ip(ipstr, INET_ADDRSTRLEN);
        std::cout << req.identify() <<  " pool_ : " << pool_ << " conn_ : " << conn  << " peer: " << ipstr << " fd: " << fd << std::endl;
#endif
        std::string header;
        parse_request_body(header, req);
        write_sock(fd, header.c_str(), header.size());

        // add event
        event_.set(ev + need_poll_fds, fd, EV_READ, on_read, (void *)this);
        event_.add(ev + need_poll_fds);
        need_poll_fds++;

        max_timeout = max_timeout > req.timeout ? max_timeout : req.timeout;
    }
    // epoll
    event_->dispatch(max_timeout);

    for (auto iter = requests.begin(); iter != requests.end(); ++ iter) {
        Request &req = *iter;
        uint64_t request_id = req.request_id;
        pool_ = select_pool(req);
        if (pool_ == nullptr) {
            continue;
        }
        pool_->release(req.conn);
        if (need_poll_fds == 0) {
            continue;
        }
        event_->remove(ev + i++);

        resp[request_id] = get_read_buffer(request_id);
    }

//    clear_map_data();

    return resp;
}

bool Http::set_read_buffer(uint64_t request_id, std::string &data)
{
    std::lock_guard<std::mutex> guard(buffer_lock);
    Response resp;
    parse_response_body(data, resp);
    responses[request_id] = resp;
}

Response& Http::get_read_buffer(uint64_t request_id)
{
    std::lock_guard<std::mutex> guard(buffer_lock);
    if (responses.find(request_id) == responses.end()) {
        return nullptr;
    }
    BytesBuffer *ret = responses[request_id];
    receive_buffers_.erase(request_id);

    return ret;
}

Http::Http()
{
    event_once = ThreadOnce<Epoll>::init();
}

Http::~Http()
{
    clear_map_data();
}