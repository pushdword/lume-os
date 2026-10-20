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
