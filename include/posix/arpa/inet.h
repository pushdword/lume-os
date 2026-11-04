#ifdef __cplusplus
extern "C"
{
#endif
#include <netinet/in.h>
    int inet_pton(int, const char *, void *);
    int inet_aton(const char *, struct in_addr *);
    const char *inet_ntop(int, const void *, char *, socklen_t);
    char *inet_ntoa(struct in_addr);
    in_addr_t inet_addr(const char *);
#ifdef __cplusplus
}
#endif
