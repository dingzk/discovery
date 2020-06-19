//
// Created by zhenkai on 2020/6/16.
//

#include <iostream>
#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <csignal>
#include <poll.h>
#include <unistd.h>
#include <netinet/in.h>
#include <algorithm>
#include <exception>
#include <sstream>

#include "http.h"

static uint64_t generate_request_id()
{
    static std::atomic<uint64_t> id_offset;
    struct timespec tn{};
    clock_gettime(CLOCK_REALTIME, &tn);
    return (tn.tv_nsec & kPMask) | (id_offset++ & kSMask);
}

Request::Request(std::string &url, std::string &me, int t) : Url(nullptr), conn(nullptr), request_id(generate_request_id()), timeout(t), method(me)
{
    Url = url_parse(url.c_str());
    if (Url == nullptr) {
        throw std::exception("Invalid Parameters url. parse URL error");
    }
    transform(method.begin(), method.end(), method.begin(), ::toupper);
    if (memcmp(method.c_str(), "GET", 3) != 0) {
        method = "POST";
    }
}

std::string & Request::identify()
{
    if (identify.empty()) {
        identify = Url->hostname + ":" + std::to_string(Url->port);
    }

    return identify;
}

Request::~Request()
{
    url_free(Url);
}

int Http::add_pool(Request *rhs)
{
    std::string &identify = rhs->identify();
    if (identify.size() <= 0) {
        return -1;
    }
    std::lock_guard<std::mutex> guard(pool_lock);
    if (pool_map.find(identify) == pool_map.end()) {
        pool_map[identify] = std::shared_ptr<motan_channel::ConnectionPool> (new (std::nothrow) ConnectionPool(rhs->Url->hostname, rhs->Url->port, kDefaultPoolSize, kDefaultKeepAliveTimeout, kDefaultConnectTimeout));
        return 1;
    }

#ifdef DEBUG
    std::cout << "add pool " + identify << std::endl;
#endif

    return 0;
}

std::shared_ptr<ConnectionPool> Http::select_pool(Request *rhs)
{
    std::lock_guard<std::mutex> guard(pool_lock);
    std::string &identify = rhs->identify();
    if (pool_map.find(identify) == pool_map.end()) {
        return nullptr;
    }

    return pool_map_[identify];
}

bool Http::add_url(std::string &url, std::string &method, int timeout)
{
    Request *req = new (std::nothrow) Request(url, method, timeout);
    unit64_t request_id = req->request_id;
    requests.insert({request_id, req});

    add_pool(req);

    ID_MAP *id_map_p = new (std::nothrow) ID_MAP;
    id_map_p->request_id = request_id;
    id_map_p->http = this;
    id_map.insert({request_id, id_map_p});
}

void Request::http_build_query(std::string &data)
{
    bool isget = false;
    if (memcmp(method.c_str(), "GET") == 0) {
        isget = true;
        data = method + " " + Url->path + "?" + Url->query + " HTTP/1.1" + "\r\n";
    } else {
        data = method + " " + Url->path + " HTTP/1.1" + "\r\n";
    }
    data += "Host:" + Url->hostname + "\r\n";
    data =  "Connection:keep-alive\r\n";
    data =  "User-Agent: wbsearch\r\n";
    data =  "Accept: */*\r\n";
    if (!isget) {
        data =  "Content-Length:" + std::to_string(strlen(Url->query)) + "\r\n";
        data =  "Content-Type: application/x-www-form-urlencoded\r\n";
    }
    data += "\r\n";
    if (!isget) {
        data += Url->query;
    }
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

static bool http_read_n(int sock, char *buffer, int len, std::string &eof)
{
    if (sock <= 0 || buffer == nullptr) {
        return;
    }
    memset(buffer, 0, len);
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
        temp_len = read(sock, buffer + read_pos, len - read_pos);
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

        if (eof != nullptr) {
            std::string buffer_temp = buffer;
            if (buffer_temp.find(eof) != std::string::npos) {
                return true;
            }
        }
    }

    return (read_pos == len);
}

bool Response::read_header(int sock, std::string &raw_header)
{
    result.reserve(1024);
    char buffer[1024] = {0};
    int pos = 0;
    std::string eof = "\r\n\r\n";
    while (true) {
        bool flag = http_read_n(sock, buffer, 1023, eof);
        result += buffer;
        if ((pos = result.find(eof)) != std::string::npos) {
            raw_header = result.substr(0, pos);
            result.erase(0, pos + strlen(eof));
            return true;
        }
        if (!flag) {
            return false;
        }
    }
}

static void format_string(std::string &str)
{
    int s = str.find_first_not_of(" ");
    int e = str.find_last_not_of(" ");
    str = str.substr(s, e - s + 1);
    transform(str.begin(), str.end(), str.begin(), ::tolower);
}

bool Response::parse_header(const std::string &raw_header)
{
    if (raw_header.empty()) {
        return false;
    }
    const char *eof = "\r\n";
    const char *split = ":";
    int pos = 0;
    if ((pos = raw_header.find(eof)) == std::string::npos) {
        return false;
    }
    // HTTP/1.1 200 OK
    std::string line = raw_header.substr(0, pos);
    std::istringstream  iss(line);
    iss >> protocolVersion >> ret_code >> descr;

    // key:value\r\n
    int offset = pos + strlen(eof) + 1;
    while ((pos = raw_header.find(eof, offset)) != std::string::npos) {
        auto index = raw_header.find_first_of(split, offset, pos - offset);
        if (index != std::string::npos) {
            auto name = raw_header.substr(offset, index);
            auto value = raw_header.substr(index + 1, pos - offset - index + 2);
            header[format_string(name)] = format_string(value);
        }

        offset = pos + strlen(eof) + 1;
    }

}

bool Response::read_body(int sock, std::string &raw_body)
{
    char buffer[1024] = {0};
    int pos = 0;
    std::string eof = "\r\n\r\n";
    if (header.find("transfer-encoding") != header.end()) {
        if (header["transfer-encoding"].compare("chunked") == 0) {
            eof = "\r\n0\r\n\r\n";
        }
    }
//    if (header.find("Connection") != header.end()) {
//        if (header["Connection"].compare("close") == 0) {
//            eof = "\r\n\r\n";
//        }
//    }
//    if (header.find("Content-Length") != header.end()) {
//        eof = "\r\n\r\n";
//    }

    if ((pos = result.find(eof)) != std::string::npos) {
        raw_body = result.substr(0, pos);
        return true;
    }
    while (true) {
        bool flag = http_read_n(sock, buffer, 1023, eof);
        result += buffer;
        if ((pos = result.find(eof)) != std::string::npos) {
            raw_body = result.substr(0, pos);
            return true;
        }
        if (!flag) {
            return false;
        }
    }

}

bool Response::parse_body(const std::string &raw_body)
{
    const char *eof = "\r\n0\r\n\r\n";
    const char *crlf = "\r\n";
    int pos = 0;

    // 3\r\nabc\r\n0\r\n\r\n
    if ((pos = raw_body.find(eof)) != std::string::npos) {
        int chunklen = 0;
        int offset = 0;
        while ((pos = raw_body.find(crlf, offset)) != 0 && pos != std::string::npos) {
            chunklen = atoi(raw_body.substr(offset, pos).c_str());
            body += raw_body.substr(pos + strlen(crlf), chunklen);

            offset = pos + 2 * strlen(crlf) + chunklen;
        }
    } else {
        // abc\r\n\r\n
        body = raw_body.substr(0, pos);
    }

    return 0;
}

void Http::on_read(int sock, short which, void *arg)
{
    if (!(which & EV_READ)) {
        return;
    }
    ID_MAP *id_map_p = (ID_MAP *) arg;
    uint64_t request_id = id_map_p->request_id;
    Http * http = id_map_p->http;

    std::string raw_header, raw_body;
    Response *resptr = new (std::nothrow) Response;
    resptr->request_id = request_id;

    resptr->read_header(sock, &raw_header);
    resptr->parse_header(raw_header);
    resptr->read_body(sock, &raw_body);
    resptr->parse_body(raw_body);

    http->set_read_buffer(request_id, resptr);
}

int Http::do_call(std::map<uint64_t, Response> &resp)
{
    int request_num = requests.size();
    if (request_num <= 0) {
        return -1;
    }
    Event *event_ = event_once.fetch();
    uint64_t request_id;
    Request *reqptr;
    Response *resptr;
    struct event_s ev[request_num];
    std::shared_ptr<ConnectionPool> pool_;
    int max_timeout = 0;
    int need_poll_fds = 0;
    for (auto iter = requests.begin(); iter != requests.end(); ++ iter) {
        reqptr = iter->second;
        pool_ = select_pool(*req);
        if (pool_ == nullptr) {
            std::cout << "get a null connection pool" << std::endl;
            continue;
        }
        Connection * conn = pool_->fetch();
        if (conn == nullptr) {
            std::cout << "get null connection" << std::endl;
            continue;
        }
        reqptr->conn = conn;
        int fd = conn->get_connect_sock();
#ifdef DEBUG
        char ipstr[INET_ADDRSTRLEN];
        conn->sock_to_ip(ipstr, INET_ADDRSTRLEN);
        std::cout << req.identify() <<  " pool_ : " << pool_ << " conn_ : " << conn  << " peer: " << ipstr << " fd: " << fd << std::endl;
#endif
        std::string header;
        reqptr->http_build_query(header);
        write_sock(fd, header.c_str(), header.size());

        // add event
        event_.set(ev + need_poll_fds, fd, EV_READ, on_read, (void *)id_map[request_id]);
        event_.add(ev + need_poll_fds);
        need_poll_fds++;

        max_timeout = max_timeout > req.timeout ? max_timeout : req.timeout;
    }
    // epoll
    event_->dispatch(max_timeout);
    auto i = 0;
    for (auto iter = requests.begin(); iter != requests.end(); ++ iter) {
        reqptr = iter->second;
        request_id = reqptr->request_id;
        pool_ = select_pool(reqptr);
        if (pool_ == nullptr) {
            continue;
        }
        pool_->release(reqptr->conn);
        if (need_poll_fds == 0) {
            continue;
        }
        event_->remove(ev + i++);

        resptr = get_read_buffer(request_id);
        if (resptr != nullptr) {
            resp[request_id] = *resptr;
            delete resptr;
        }
        delete requests[request_id];
        delete id_map[request_id];
    }
    requests.clear();
    id_map.clear();

    return need_poll_fds;
}

bool Http::set_read_buffer(uint64_t request_id, Response *rhs)
{
    std::lock_guard<std::mutex> guard(buffer_lock);
    responses[request_id] = rhs;
}

Response* Http::get_read_buffer(uint64_t request_id)
{
    std::lock_guard<std::mutex> guard(buffer_lock);
    if (responses.find(request_id) == responses.end()) {
        return nullptr;
    }
    Response *resptr = responses[request_id];
    responses.erase(request_id);

    return resptr;
}

Http::Http()
{
    event_once = ThreadOnce<Epoll>::init();
}

Http::~Http()
{
}