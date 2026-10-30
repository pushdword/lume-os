#ifndef LUME_POSIX_SIGNAL_H
#define LUME_POSIX_SIGNAL_H
#include <stdint.h>
#include <sys/types.h>
#include <time.h>
#ifndef _SIGSET_T_DECLARED
#define _SIGSET_T_DECLARED
typedef uint32_t sigset_t;
#endif
typedef int sig_atomic_t;
typedef void (*sighandler_t)(int);
#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)
#define SIG_ERR ((sighandler_t) - 1)
#define SIGHUP 1
#define SIGINT 2
#define SIGQUIT 3
#define SIGILL 4
#define SIGTRAP 5
#define SIGABRT 6
#define SIGIOT SIGABRT
#define SIGEMT 7
#define SIGFPE 8
#define SIGKILL 9
#define SIGBUS 10
#define SIGSEGV 11
#define SIGSYS 12
#define SIGPIPE 13
#define SIGALRM 14
#define SIGTERM 15
#define SIGURG 16
#define SIGSTOP 17
#define SIGTSTP 18
#define SIGCONT 19
#define SIGCHLD 20
#define SIGCLD SIGCHLD
#define SIGTTIN 21
#define SIGTTOU 22
#define SIGIO 23
#define SIGPOLL SIGIO
#define SIGXCPU 24
#define SIGXFSZ 25
#define SIGVTALRM 26
#define SIGPROF 27
#define SIGWINCH 28
#define SIGLOST 29
#define SIGUSR1 30
#define SIGUSR2 31
#define NSIG 32
#define _NSIG NSIG
#define SIG_SETMASK 0
#define SIG_BLOCK 1
#define SIG_UNBLOCK 2
#define SA_NOCLDSTOP 1
#define SA_RESETHAND 4
#define SA_RESTART 8
#define SA_NODEFER 64

struct sigaction
{
    sighandler_t sa_handler;
    sigset_t sa_mask;
    int sa_flags;
};
#ifdef __cplusplus
extern "C"
{
#endif
    int sigaction(int, const struct sigaction *, struct sigaction *);
    int sigprocmask(int, const sigset_t *, sigset_t *);
    int sigemptyset(sigset_t *);
    int sigfillset(sigset_t *);
    int sigaddset(sigset_t *, int);
    int sigdelset(sigset_t *, int);
    int sigismember(const sigset_t *, int);
    sighandler_t signal(int, sighandler_t);
    int raise(int);
    int kill(pid_t, int);
    unsigned alarm(unsigned);
    int pause(void);
    int sigsuspend(const sigset_t *);
#ifdef __cplusplus
}
#endif
#endif
