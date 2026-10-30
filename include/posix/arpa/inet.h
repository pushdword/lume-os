#ifdef __cplusplus
extern "C"
{
#endif
#include <netinet/in.h>
    int inet_pton(int, const char *, void *);
    const char *inet_ntop(int, const void *, char *, socklen_t);
    in_addr_t inet_addr(const char *);
#ifdef __cplusplus
}
#endif
