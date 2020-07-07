#ifndef DISCOVERY_UTILS_H
#define DISCOVERY_UTILS_H

#include <string>
#include <sys/time.h>

long get_current_time_ms();  //ms

long get_current_time_us();  //us

std::string get_local_time();

std::string get_local_ip();

const char *get_last_substring(const std::string &str, const std::string &sep);

void stripslashes(std::string &str);

int get_file_size(const char *filepath);

#endif //DISCOVERY_UTILS_H
