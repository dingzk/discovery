//
// Created by zhenkai on 2020/7/6.
//

#ifndef DISCOVERY_LOG_H
#define DISCOVERY_LOG_H

#define LEVEL_FATAL					5
#define LEVEL_ERROR					4
#define LEVEL_WARN				    3
#define LEVEL_INFO					2
#define LEVEL_TRACE					1
#define LEVEL_DEBUG					0

#define LOG_FATAL(format, ...)     write_log(__FILE__, __LINE__, LEVEL_FATAL, format, ## __VA_ARGS__)
#define LOG_ERR(format, ...)       write_log(__FILE__, __LINE__, LEVEL_ERROR, format, ## __VA_ARGS__)
#define LOG_WARN(format, ...)      write_log(__FILE__, __LINE__, LEVEL_WARN, format, ## __VA_ARGS__)
#define LOG_INFO(format, ...)      write_log(__FILE__, __LINE__, LEVEL_INFO, format, ## __VA_ARGS__)
#define LOG_TRACE(format, ...)     write_log(__FILE__, __LINE__, LEVEL_TRACE, format, ## __VA_ARGS__)
#define LOG_DEBUG(format, ...)     write_log(__FILE__, __LINE__, LEVEL_DEBUG, format, ## __VA_ARGS__)

void write_log(const char* file_name, int line_no, int ll, const char* fmt, ...);

void log_init(const char *logfile, int level);
int log_destroy();

#endif //DISCOVERY_LOG_H
