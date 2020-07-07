//
// Created by zhenkai on 2020/7/6.
//
#include <unistd.h>
#include <pthread.h>
#include <stdarg.h>

#include "log.h"
#include "common/utils.h"

#define MAX_FILE_SIZE (1*1024*1024)

static FILE *log_fp = stderr;
static const char *log_file = nullptr;
static int log_level = LEVEL_INFO;
static pthread_mutex_t  log_mutex = PTHREAD_MUTEX_INITIALIZER;

static void set_log_file(const char *logfile)
{
    log_file = logfile;
}

static void set_log_level(int ll)
{
    log_level = ll;
}

void log_init(const char *logfile, int level)
{
    set_log_file(logfile);
    set_log_level(level);

    if (log_file != NULL) {
        log_fp = fopen(log_file, "a+");
    }
    if (!log_fp) {
        log_fp = stderr;
    }
}

int log_destroy()
{
    if (log_fp != stderr) {
        fclose(log_fp);
    }
}

static const char *get_log_level_info(int ll)
{
    switch(ll) {
        case LEVEL_DEBUG:
            return "DEBUG";
        case LEVEL_TRACE:
            return "TRACE";
        case LEVEL_INFO:
            return "INFO";
        case LEVEL_WARN:
            return "WARN";
        case LEVEL_ERROR:
            return "ERROR";
        case LEVEL_FATAL:
            return "FATAL";
        default:
            return "UNKNOWN";
    }
}

void write_log(const char* file_name, int line_no, int ll, const char* fmt, ...)
{
    va_list     ap;
    time_t      timestamp;
    struct tm cur_tm = {0};
    if (ll < log_level) {
        return ;
    }
    pthread_mutex_lock(&log_mutex);
    if ((get_file_size(log_file)) > MAX_FILE_SIZE) {
        truncate(log_file, 0);
    }
    timestamp = time(NULL);

    time(&timestamp);
    localtime_r(&timestamp, &cur_tm);

    fprintf(log_fp, "%4d/%02d/%02d %02d:%02d:%02d - [%d][%s][file:%s line:%d] - ",
            cur_tm.tm_year + 1900,
            cur_tm.tm_mon + 1,
            cur_tm.tm_mday,
            cur_tm.tm_hour,
            cur_tm.tm_min,
            cur_tm.tm_sec,
            getpid(),
            get_log_level_info(ll),
            file_name,
            line_no);
    va_start(ap, fmt);
    vfprintf(log_fp, fmt, ap);
    va_end(ap);
    fprintf(log_fp, "\n");
    fflush(log_fp);

    pthread_mutex_unlock(&log_mutex);
}