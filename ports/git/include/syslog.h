#ifndef LUME_GIT_SYSLOG_H
#define LUME_GIT_SYSLOG_H
#include <stdarg.h>
#define LOG_EMERG 0
#define LOG_ALERT 1
#define LOG_CRIT 2
#define LOG_ERR 3
#define LOG_WARNING 4
#define LOG_NOTICE 5
#define LOG_INFO 6
#define LOG_DEBUG 7
#define LOG_USER 8
#define LOG_DAEMON 24
#define LOG_PID 1
#define LOG_CONS 2
#define LOG_NDELAY 8
#define LOG_PERROR 32
void openlog(const char *, int, int);
void closelog(void);
void syslog(int, const char *, ...);
void vsyslog(int, const char *, va_list);
#endif
