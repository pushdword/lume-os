#ifndef LUME_OPENSSH_UNISTD_H
#define LUME_OPENSSH_UNISTD_H
#include_next <unistd.h>
int setresuid(uid_t, uid_t, uid_t);
int setresgid(gid_t, gid_t, gid_t);
#endif
