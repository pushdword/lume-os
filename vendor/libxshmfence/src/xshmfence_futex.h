/*
 * Copyright © 2013 Keith Packard
 * Copyright © 2013 Jung-uk Kim <jkim@FreeBSD.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#ifndef _XSHMFENCE_FUTEX_H_
#define _XSHMFENCE_FUTEX_H_

#include <errno.h>

#if defined(__lume__)

#include "lume_futex.h"

#elif defined(HAVE_UMTX)

#include <sys/types.h>
#include <sys/umtx.h>
#include <limits.h>

static inline int sys_futex(void *addr, int op, int32_t val)
{
	return _umtx_op(addr, op, (uint32_t)val, NULL, NULL) == -1 ? errno : 0;
}

static inline int futex_wake(int32_t *addr) {
	return sys_futex(addr, UMTX_OP_WAKE, INT_MAX);
}

static inline int futex_wait(int32_t *addr, int32_t value) {
	return sys_futex(addr, UMTX_OP_WAIT_UINT, value);
}

#else

#include <stdint.h>
#include <values.h>
#include <linux/futex.h>
#include <sys/time.h>
#include <sys/syscall.h>

#ifndef SYS_futex
#define SYS_futex SYS_futex_time64
#endif

static inline long sys_futex(void *addr1, int op, int val1, struct timespec *timeout, void *addr2, int val3)
{
	return syscall(SYS_futex, addr1, op, val1, timeout, addr2, val3);
}

static inline int futex_wake(int32_t *addr) {
	return sys_futex(addr, FUTEX_WAKE, MAXINT, NULL, NULL, 0);
}

static inline int futex_wait(int32_t *addr, int32_t value) {
	return sys_futex(addr, FUTEX_WAIT, value, NULL, NULL, 0);
}

#endif

#define barrier() __asm__ __volatile__("": : :"memory")

static inline void atomic_store(int32_t *f, int32_t v)
{
#if defined(__lume__)
	__atomic_store_n(f, v, __ATOMIC_RELEASE);
#else
	barrier();
	*f = v;
	barrier();
#endif
}

static inline int32_t atomic_fetch(int32_t *a)
{
#if defined(__lume__)
	return __atomic_load_n(a, __ATOMIC_ACQUIRE);
#else
	int32_t v;
	barrier();
	v = *a;
	barrier();
	return v;
#endif
}
	
struct xshmfence {
    int32_t     v;
};

#define xshmfence_init(fd)

#endif /* _XSHMFENCE_FUTEX_H_ */
