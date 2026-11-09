#ifndef LUME_NET_TOOLS_NETDB_H
#define LUME_NET_TOOLS_NETDB_H
#include_next <netdb.h>
#include <stdint.h>

struct netent
{
    char *n_name;
    char **n_aliases;
    int n_addrtype;
    uint32_t n_net;
};

struct protoent
{
    char *p_name;
    char **p_aliases;
    int p_proto;
};
struct netent *getnetbyaddr(uint32_t, int);
struct netent *getnetbyname(const char *);
struct servent *getservbyport(int, const char *);
struct protoent *getprotobynumber(int);
struct protoent *getprotobyname(const char *);
void setservent(int);
struct servent *getservent(void);
void endservent(void);
void setprotoent(int);
struct protoent *getprotoent(void);
void endprotoent(void);
void herror(const char *);
const char *hstrerror(int);
#endif
