// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// libc++ external threading API for one execution context per Lume process.
// Mutexes enforce ownership within one execution context. LLVM is built with
// LLVM_ENABLE_THREADS=OFF and does not request thread creation.
#ifndef LUME_LIBCPP_EXTERNAL_THREADING
#define LUME_LIBCPP_EXTERNAL_THREADING
#include <__config>
#include <__chrono/duration.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>
extern "C" int32_t lume_syscall(uint32_t, uint32_t, uint32_t, uint32_t);
_LIBCPP_BEGIN_NAMESPACE_STD
using __libcpp_timespec_t = ::timespec;
using __libcpp_mutex_t = unsigned;
using __libcpp_recursive_mutex_t = unsigned;
#define _LIBCPP_MUTEX_INITIALIZER 0

inline int __libcpp_mutex_lock(unsigned *M)
{
    if (*M)
        return EDEADLK;
    *M = 1;
    return 0;
}

inline bool __libcpp_mutex_trylock(unsigned *M)
{
    if (*M)
        return false;
    *M = 1;
    return true;
}

inline int __libcpp_mutex_unlock(unsigned *M)
{
    if (!*M)
        return EPERM;
    *M = 0;
    return 0;
}

inline int __libcpp_mutex_destroy(unsigned *)
{
    return 0;
}

inline int __libcpp_recursive_mutex_init(unsigned *M)
{
    *M = 0;
    return 0;
}

inline int __libcpp_recursive_mutex_lock(unsigned *M)
{
    ++*M;
    return 0;
}

inline bool __libcpp_recursive_mutex_trylock(unsigned *M)
{
    ++*M;
    return true;
}

inline int __libcpp_recursive_mutex_unlock(unsigned *M)
{
    if (!*M)
        return EPERM;
    --*M;
    return 0;
}

inline int __libcpp_recursive_mutex_destroy(unsigned *)
{
    return 0;
}

using __libcpp_condvar_t = unsigned;
#define _LIBCPP_CONDVAR_INITIALIZER 0

inline int __libcpp_condvar_signal(unsigned *)
{
    return 0;
}

inline int __libcpp_condvar_broadcast(unsigned *)
{
    return 0;
}

inline int __libcpp_condvar_wait(unsigned *, unsigned *)
{
    return ENOSYS;
}

inline int __libcpp_condvar_timedwait(unsigned *, unsigned *, timespec *)
{
    return ENOSYS;
}

inline int __libcpp_condvar_destroy(unsigned *)
{
    return 0;
}

using __libcpp_exec_once_flag = unsigned;
#define _LIBCPP_EXEC_ONCE_INITIALIZER 0

inline int __libcpp_execute_once(unsigned *F, void (*Run)())
{
    if (*F == 1)
        return EDEADLK;
    if (!*F)
    {
        *F = 1;
        Run();
        *F = 2;
    }
    return 0;
}

using __libcpp_thread_id = unsigned;
using __libcpp_thread_t = unsigned;
#define _LIBCPP_NULL_THREAD 0

inline bool __libcpp_thread_id_equal(unsigned A, unsigned B)
{
    return A == B;
}

inline bool __libcpp_thread_id_less(unsigned A, unsigned B)
{
    return A < B;
}

inline bool __libcpp_thread_isnull(const unsigned *T)
{
    return !*T;
}

inline int __libcpp_thread_create(unsigned *, void *(*)(void *), void *)
{
    return ENOSYS;
}

inline unsigned __libcpp_thread_get_current_id()
{
    return 1;
}

inline unsigned __libcpp_thread_get_id(const unsigned *T)
{
    return *T;
}

inline int __libcpp_thread_join(unsigned *)
{
    return ENOSYS;
}

inline int __libcpp_thread_detach(unsigned *)
{
    return ENOSYS;
}

inline void __libcpp_thread_yield()
{
    lume_syscall(3, 0, 0, 0);
}

inline void __libcpp_thread_sleep_for(const chrono::nanoseconds &N)
{
    auto Ms = chrono::duration_cast<chrono::milliseconds>(N).count();
    if (Ms > 0)
        lume_syscall(34, (uint32_t)(Ms > 60000 ? 60000 : Ms), 0, 0);
}

#define _LIBCPP_TLS_DESTRUCTOR_CC
using __libcpp_tls_key = unsigned;
int __libcpp_tls_create(unsigned *, void (*)(void *));
void *__libcpp_tls_get(unsigned);
int __libcpp_tls_set(unsigned, void *);
_LIBCPP_END_NAMESPACE_STD
#endif
