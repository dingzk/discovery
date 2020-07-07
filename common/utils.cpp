#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include "common/utils.h"

long get_current_time_ms() {
    timeval tv{};
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

long get_current_time_us() {
    timeval tv{};
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

std::string get_local_time() {
    time_t now;
    now = time(nullptr);
    struct tm *local;
    local = localtime(&now);
    std::string ret;
    ret = std::to_string(local->tm_year + 1900) + std::to_string(local->tm_mon + 1) + std::to_string(local->tm_mday);
    ret += '-' + std::to_string(local->tm_hour) + std::to_string(local->tm_min) + std::to_string(local->tm_sec);
    return ret;
}

std::string get_local_ip() {
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

const char *get_last_substring(const std::string &str, const std::string &sep) {
    return str.data() + str.find_last_of(sep) + 1;
}

/* {{{ stripslashes
 *
 * be careful, this edits the string in-place */
void stripslashes(std::string &str)
{
    char *s, *t;
    size_t l;

    if (str.empty()) {
        return;
    }
    char *temp = strdup(str.c_str());
    s = temp;
    t = s;
    l = str.size();

    while (l > 0) {
        if (*t == '\\') {
            t++;                /* skip the slash */
            l--;
            if (l > 0) {
                if (*t == '0') {
                    *s++='\0';
                    t++;
                } else {
                    *s++ = *t++;    /* preserve the next character */
                }
                l--;
            }
        } else {
            *s++ = *t++;
            l--;
        }
    }
    if (s != t) {
        *s = '\0';
    }

    str.assign(temp);
    free(temp);
}
/* }}} */

int get_file_size(const char *filepath)
{
    struct stat buff;

    if (filepath != NULL) {
        stat(filepath, &buff);

        return buff.st_size;
    }

    return -1;
}

