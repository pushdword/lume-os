#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>

static unsigned checks;
static int open_error;
static bool bash_available;
static unsigned group_calls, supplementary_count;
static gid_t supplementary[8];
static int group_error;
int accounts_test_setgroups(int, const gid_t *);

#define T(expression)                                                                              \
    do                                                                                             \
    {                                                                                              \
        assert(expression);                                                                        \
        ++checks;                                                                                  \
    } while (0)

static FILE *database_open(const char *path, const char *mode)
{
    if (open_error)
    {
        errno = open_error;
        return NULL;
    }
    return fopen(path, mode);
}

#define fopen database_open
#define access accounts_test_access
#define getuid accounts_test_getuid
#define setgroups accounts_test_setgroups
#define LUME_PASSWD_PATH "accounts-passwd.tmp"
#define LUME_GROUP_PATH "accounts-group.tmp"
#include "../../libc/posix/accounts.c"
#undef fopen

int accounts_test_access(const char *path, int mode)
{
    T(!strcmp(path, "/data/bin/bash") && mode == X_OK);
    return bash_available ? 0 : -1;
}

uid_t accounts_test_getuid(void)
{
    return 1000;
}

int accounts_test_setgroups(int count, const gid_t *groups)
{
    T(count > 0 && count <= 8);
    ++group_calls;
    if (group_error)
    {
        errno = group_error;
        return -1;
    }
    memcpy(supplementary, groups, (size_t)count * sizeof(*groups));
    supplementary_count = (unsigned)count;
    return 0;
}

static void database_write(const char *path, const char *data)
{
    FILE *stream = fopen(path, "wb");
    T(stream != NULL);
    size_t bytes = strlen(data);
    T(fwrite(data, 1, bytes, stream) == bytes);
    T(fclose(stream) == 0);
}

static void bootstrap(void)
{
    remove(LUME_PASSWD_PATH);
    remove(LUME_GROUP_PATH);
    struct passwd *pw = getpwuid(0);
    T(pw && !strcmp(pw->pw_name, "root") && !strcmp(pw->pw_shell, "/bin/sh"));
    T(getpwuid(1000) == NULL);
    bash_available = true;
    pw = getpwnam("root");
    T(pw && !strcmp(pw->pw_shell, "/data/bin/bash"));
    T(getpwnam("missing") == NULL);
    struct group *gr = getgrgid(0);
    T(gr && !strcmp(gr->gr_name, "root") && !strcmp(gr->gr_mem[0], "root"));
    T(gr->gr_mem[1] == NULL && getgrnam("missing") == NULL);
    setpwent();
    T(getpwent() != NULL && getpwent() == NULL);
    setgrent();
    T(getgrent() != NULL && getgrent() == NULL);
    open_error = EACCES;
    errno = 0;
    T(getpwuid(0) == NULL && errno == EACCES);
    T(getgrgid(0) == NULL && errno == EACCES);
    setpwent();
    T(getpwent() == NULL && errno == EACCES);
    setgrent();
    T(getgrent() == NULL && errno == EACCES);
    open_error = 0;
}

static void databases(void)
{
    database_write(LUME_PASSWD_PATH, "# accounts\n\ninvalid\n:x:1:1:Invalid:/home:/bin/sh\n"
                                     "overflow:x:4294967296:1:Invalid:/home:/bin/sh\n"
                                     "negative:x:-1:1:Invalid:/home:/bin/sh\n"
                                     "wide:x:65536:1:Invalid:/home:/bin/sh\n"
                                     "root:x:0:0:System:/root:/bin/custom\r\n"
                                     "alex:x:1000:100:Alex:/home/alex:/bin/sh\n"
                                     "service:!:65534:100:Service:/srv:/bin/false");
    database_write(LUME_GROUP_PATH, "# groups\ninvalid\nroot:x:0:root\n"
                                    "bad:x:4294967296:alex\ninvalid:x:20:alex,,root\n"
                                    "users:x:100:alex,service\nempty:x:101:\n");
    struct passwd *pw = getpwnam("alex");
    T(pw && pw->pw_uid == 1000 && pw->pw_gid == 100);
    T(!strcmp(pw->pw_dir, "/home/alex") && !strcmp(pw->pw_gecos, "Alex"));
    T(getpwuid(65534) && !strcmp(getpwuid(65534)->pw_name, "service"));
    T(getpwnam("overflow") == NULL && getpwnam("negative") == NULL);
    T(getpwnam("") == NULL && getpwuid(1) == NULL && getpwnam("wide") == NULL);
    T(!strcmp(getpwuid(0)->pw_shell, "/bin/custom"));
    T(!strcmp(getlogin(), "alex"));
    struct group *gr = getgrnam("users");
    T(gr && gr->gr_gid == 100 && !strcmp(gr->gr_mem[0], "alex"));
    T(!strcmp(gr->gr_mem[1], "service") && gr->gr_mem[2] == NULL);
    T(getgrnam("invalid") == NULL && getgrnam("bad") == NULL);
    gr = getgrgid(101);
    T(gr && gr->gr_mem[0] == NULL);
    setpwent();
    T(!strcmp(getpwent()->pw_name, "root"));
    T(getpwuid(65534) != NULL);
    T(!strcmp(getpwent()->pw_name, "alex"));
    T(!strcmp(getpwent()->pw_name, "service"));
    T(getpwent() == NULL);
    setpwent();
    T(!strcmp(getpwent()->pw_name, "root"));
    errno = EDOM;
    endpwent();
    T(errno == EDOM);
    T(!strcmp(getpwent()->pw_name, "root"));
    endpwent();
    setgrent();
    T(!strcmp(getgrent()->gr_name, "root"));
    T(getgrgid(101) != NULL);
    T(!strcmp(getgrent()->gr_name, "users"));
    T(!strcmp(getgrent()->gr_name, "empty"));
    T(getgrent() == NULL);
    errno = EDOM;
    endgrent();
    T(errno == EDOM);
    T(!strcmp(getgrent()->gr_name, "root"));
    endgrent();
}

static void reentrant(void)
{
    struct passwd a, b, *pa, *pb;
    char first[512], second[512];
    T(getpwnam_r("alex", &a, first, sizeof(first), &pa) == 0 && pa == &a);
    T(getpwuid_r(65534, &b, second, sizeof(second), &pb) == 0 && pb == &b);
    T(!strcmp(a.pw_name, "alex") && !strcmp(b.pw_name, "service"));
    T(getpwnam_r("alex", &b, second, 8, &pb) == ERANGE && pb == NULL);
    T(getpwnam_r(NULL, &b, second, sizeof(second), &pb) == EINVAL && pb == NULL);
    T(getpwuid_r(0, NULL, second, sizeof(second), &pb) == EINVAL && pb == NULL);
    T(getpwuid_r(0, &b, NULL, sizeof(second), &pb) == EINVAL && pb == NULL);
    T(getpwuid_r(0, &b, second, sizeof(second), NULL) == EINVAL);
    struct group ga, gb, *pga, *pgb;
    T(getgrnam_r("users", &ga, first + 1, sizeof(first) - 1, &pga) == 0 && pga == &ga);
    T(!((uintptr_t)ga.gr_mem & (_Alignof(char *) - 1)));
    T(getgrgid_r(101, &gb, second, sizeof(second), &pgb) == 0 && pgb == &gb);
    T(!strcmp(ga.gr_mem[0], "alex") && gb.gr_mem[0] == NULL);
    T(getgrnam_r("users", &gb, second, 25, &pgb) == ERANGE && pgb == NULL);
    T(getgrnam_r(NULL, &gb, second, sizeof(second), &pgb) == EINVAL && pgb == NULL);
    T(getgrgid_r(100, &gb, NULL, sizeof(second), &pgb) == EINVAL && pgb == NULL);
    T(getgrgid_r(100, &gb, second, sizeof(second), NULL) == EINVAL);
    open_error = EIO;
    T(getpwuid_r(0, &a, first, sizeof(first), &pa) == EIO && pa == NULL);
    T(getgrgid_r(0, &ga, first, sizeof(first), &pga) == EIO && pga == NULL);
    open_error = 0;
    database_write(LUME_PASSWD_PATH, "");
    database_write(LUME_GROUP_PATH, "");
    T(getpwuid(0) == NULL && getgrgid(0) == NULL);
    remove(LUME_PASSWD_PATH);
    remove(LUME_GROUP_PATH);
    T(getpwuid_r(0, &a, first, 4, &pa) == ERANGE && pa == NULL);
    T(getgrgid_r(0, &ga, first, 4, &pga) == ERANGE && pga == NULL);
}

static void memberships(void)
{
    T(initgroups(NULL, 100) == -1 && errno == EINVAL && group_calls == 0);
    T(initgroups("", 100) == -1 && errno == EINVAL && group_calls == 0);
    T(initgroups("alex", 65535) == -1 && errno == EINVAL && group_calls == 0);
    T(initgroups("alex", 100) == 0 && supplementary_count == 1 && supplementary[0] == 100);
    database_write(LUME_GROUP_PATH,
                   "users:x:100:alex,service\nwheel:x:20:alex\nduplicate:x:20:alex\n");
    T(initgroups("alex", 100) == 0 && supplementary_count == 2);
    T(supplementary[0] == 100 && supplementary[1] == 20);
    T(initgroups("service", 100) == 0 && supplementary_count == 1);
    group_error = EPERM;
    T(initgroups("alex", 100) == -1 && errno == EPERM);
    group_error = 0;
    unsigned before = group_calls;
    open_error = EIO;
    T(initgroups("alex", 100) == -1 && errno == EIO && group_calls == before);
    open_error = 0;
    database_write(LUME_GROUP_PATH, "a:x:1:alex\nb:x:2:alex\nc:x:3:alex\nd:x:4:alex\n"
                                    "e:x:5:alex\nf:x:6:alex\ng:x:7:alex\nh:x:8:alex\n");
    T(initgroups("alex", 100) == -1 && errno == EINVAL && group_calls == before);
    remove(LUME_GROUP_PATH);
}

int main(void)
{
    bootstrap();
    databases();
    reentrant();
    memberships();
    endpwent();
    endgrent();
    printf("Account databases: %u checks passed\n", checks);
    return 0;
}
