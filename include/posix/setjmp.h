/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_POSIX_SETJMP_H
#define LUME_POSIX_SETJMP_H
#include_next <setjmp.h>
#include <signal.h>

typedef struct
{
    jmp_buf registers;
    sigset_t mask;
    int saved;
} sigjmp_buf[1];

#define sigsetjmp(buffer, save)                                                                    \
    ((buffer)[0].saved = (save),                                                                   \
     (save) ? (void)sigprocmask(SIG_SETMASK, NULL, &(buffer)[0].mask) : (void)0,                   \
     setjmp((buffer)[0].registers))
#ifdef __cplusplus
extern "C"
{
#endif
    void siglongjmp(sigjmp_buf, int) __attribute__((noreturn));
#ifdef __cplusplus
}
#endif
#endif
