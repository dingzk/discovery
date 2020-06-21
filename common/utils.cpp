//
// Created by zhachen on 2018/11/15.
//

#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "common/utils.h"

long motan_util::get_current_time_ms() {
    timeval tv{};
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

long motan_util::get_current_time_us() {
    timeval tv{};
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

std::string motan_util::get_local_time() {
    time_t now;
    now = time(nullptr);
    struct tm *local;
    local = localtime(&now);
    std::string ret;
    ret = std::to_string(local->tm_year + 1900) + std::to_string(local->tm_mon + 1) + std::to_string(local->tm_mday);
    ret += '-' + std::to_string(local->tm_hour) + std::to_string(local->tm_min) + std::to_string(local->tm_sec);
    return ret;
}

std::string motan_util::get_local_ip() {
    char hostname[256];
    char ip[256];
    auto error = gethostname(hostname, sizeof(hostname));
    if (error) {
        return "";
    }
    auto *host = gethostbyname(hostname);
    if (host == nullptr) {
        return "";
    }
    strcpy(ip, inet_ntoa(*(in_addr *) *host->h_addr_list));
    return ip;
}

const char *motan_util::get_last_substring(const std::string &str, const std::string &sep) {
    return str.data() + str.find_last_of(sep) + 1;
}