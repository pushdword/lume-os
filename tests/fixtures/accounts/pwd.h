#ifndef LUME_ACCOUNTS_TEST_PWD_H
#define LUME_ACCOUNTS_TEST_PWD_H
#include <stddef.h>
#include <stdint.h>
typedef uint16_t uid_t;
typedef uint16_t gid_t;

struct passwd
{
    char *pw_name, *pw_passwd;
    uid_t pw_uid;
    gid_t pw_gid;
    char *pw_comment, *pw_gecos, *pw_dir, *pw_shell;
};
struct passwd *getpwuid(uid_t);
struct passwd *getpwnam(const char *);
struct passwd *getpwent(void);
void setpwent(void);
void endpwent(void);
int getpwuid_r(uid_t, struct passwd *, char *, size_t, struct passwd **);
int getpwnam_r(const char *, struct passwd *, char *, size_t, struct passwd **);
#endif
