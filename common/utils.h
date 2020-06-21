//
// Created by zhachen on 2018/11/15.
//

#ifndef MOTAN_CPP_UTILS_H
#define MOTAN_CPP_UTILS_H

#include <string>
#include <sys/time.h>

namespace motan_util {
    long get_current_time_ms();  //ms

    long get_current_time_us();  //us

    std::string get_local_time();

    std::string get_local_ip();

    const char *get_last_substring(const std::string &str, const std::string &sep);
};

#endif //MOTAN_CPP_UTILS_H
