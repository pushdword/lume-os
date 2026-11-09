#ifndef LUME_NET_TOOLS_BACKEND_H
#define LUME_NET_TOOLS_BACKEND_H
#include <stdio.h>
FILE *lume_net_fopen(const char *, const char *);
int lume_net_fclose(FILE *);
int lume_net_ioctl(int, unsigned long, ...);
int lume_net_socket(int, int, int);
#define fopen lume_net_fopen
#define fclose lume_net_fclose
#define ioctl lume_net_ioctl
#define socket lume_net_socket
#endif
