#ifndef LUME_BASH_PLATFORM_H
#define LUME_BASH_PLATFORM_H
#define _POSIX_VERSION 200809L
#define _POSIX_JOB_CONTROL 1
#define HAVE_POSIX_SIGNALS 1
#define JOB_CONTROL 1
#define PROGRAMMABLE_COMPLETION 1
#undef JOB_CONTROL_MISSING
#undef HAVE_GETLOCALENAME_L
#undef HAVE_GOOD_USELOCALE
#undef HAVE_WORKING_USELOCALE
#undef HAVE_USELOCALE
#undef HAVE_NEWLOCALE
#undef HAVE_ICONV
#undef COMMAND_TIMING
#undef HAVE_LONG_DOUBLE
#undef HAVE_TERMCAP_H
#undef HAVE_DECL_PUTCHAR_UNLOCKED
#define HAVE_DECL_PUTCHAR_UNLOCKED 0
#undef STANDARD_UTILS_PATH
#define STANDARD_UTILS_PATH "/bin:/usr/bin"
#undef DEFAULT_PATH_VALUE
#define DEFAULT_PATH_VALUE "/data/bin:/bin:/usr/bin"
#define DEFAULT_TMPDIR "/data/tmp"
#define CONF_OSTYPE "lume"
#if defined(LUME_ARCH_AMD64)
#define CONF_HOSTTYPE "x86_64"
#define CONF_MACHTYPE "x86_64-unknown-lume"
#elif defined(LUME_ARCH_AARCH64)
#define CONF_HOSTTYPE "aarch64"
#define CONF_MACHTYPE "aarch64-unknown-lume"
#else
#define CONF_HOSTTYPE "i686"
#define CONF_MACHTYPE "i686-unknown-lume"
#endif
#define PACKAGE "bash"
#define LOCALEDIR "/usr/share/locale"
#define TCOOFF 0
#define TCOON 1
#include <sys/types.h>
int tcflow(int, int);
int killpg(pid_t, int);
int getresuid(uid_t *, uid_t *, uid_t *);
int getresgid(gid_t *, gid_t *, gid_t *);
int setresuid(uid_t, uid_t, uid_t);
int setresgid(gid_t, gid_t, gid_t);
#endif
