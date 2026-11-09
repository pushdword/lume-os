#ifndef LUME_NET_TOOLS_CONFIG_H
#define LUME_NET_TOOLS_CONFIG_H
#define HAVE_AFINET 1
#define HAVE_AFUNIX 0
#define HAVE_HWETHER 1
#define HAVE_HWLOOP 1
#define HAVE_HOSTNAME_TOOLS 1
#define HAVE_ARP_TOOLS 1
#define I18N 0
#define HAVE_SELINUX 0
#define RELEASE "net-tools 2.10 (Lume)"
#define MAXHOSTNAMELEN 255
#define SOCK_RAW 3
#define SOCK_RDM 4
#define SOCK_SEQPACKET 5
#include <stddef.h>
int setdomainname(const char *, size_t);
#endif
