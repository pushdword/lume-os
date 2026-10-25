#include <stdarg.h>
#define LOG_AUTH 32
#define LOG_AUTHPRIV 80
#define LOG_INFO 6
#define LOG_ERR 3
#define LOG_WARNING 4
#define LOG_NOTICE 5
#define LOG_PID 1
void openlog(const char *, int, int);
void syslog(int, const char *, ...);
void closelog(void);

#define LOG_DEBUG 7
#define LOG_DAEMON 24
#define LOG_USER 8
#define LOG_LOCAL0 128
#define LOG_LOCAL1 136
#define LOG_LOCAL2 144
#define LOG_LOCAL3 152
#define LOG_LOCAL4 160
#define LOG_LOCAL5 168
#define LOG_LOCAL6 176
#define LOG_LOCAL7 184
#define LOG_CRIT 2
void vsyslog(int, const char *, va_list);
