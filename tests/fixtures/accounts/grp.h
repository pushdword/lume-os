#ifndef LUME_ACCOUNTS_TEST_GRP_H
#define LUME_ACCOUNTS_TEST_GRP_H
#include "pwd.h"

struct group
{
    char *gr_name, *gr_passwd;
    gid_t gr_gid;
    char **gr_mem;
};
struct group *getgrgid(gid_t);
struct group *getgrnam(const char *);
struct group *getgrent(void);
void setgrent(void);
void endgrent(void);
int getgrgid_r(gid_t, struct group *, char *, size_t, struct group **);
int getgrnam_r(const char *, struct group *, char *, size_t, struct group **);
int initgroups(const char *, gid_t);
int setgroups(int, const gid_t *);
#endif
