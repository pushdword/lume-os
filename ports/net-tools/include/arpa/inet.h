#ifndef LUME_NET_TOOLS_INET_H
#define LUME_NET_TOOLS_INET_H
#include_next <arpa/inet.h>
#include <netinet/in.h>
int inet_aton(const char *, struct in_addr *);
char *inet_ntoa(struct in_addr);
#endif
