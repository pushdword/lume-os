#ifndef LUME_GIT_UNIX_SOCKET_H
#define LUME_GIT_UNIX_SOCKET_H
#include "lume_net.h"

struct sockaddr_un
{
    sa_family_t sun_family;
    char sun_path[108];
};
#endif
