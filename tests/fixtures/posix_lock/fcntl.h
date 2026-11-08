#ifndef LUME_TEST_POSIX_LOCK_FCNTL_H
#define LUME_TEST_POSIX_LOCK_FCNTL_H
#include_next <fcntl.h>
#include <stdint.h>

/* Fixed-width fields exercise the target LP64 lock layout on an LLP64 host. */
struct flock
{
    short l_type, l_whence;
#ifdef LUME_TEST_LOCK_NARROW
    int32_t l_start, l_len;
#else
    int64_t l_start, l_len;
#endif
    short l_pid, l_xxx;
};

#define F_DUPFD 0
#define F_GETFD 1
#define F_SETFD 2
#define F_GETFL 3
#define F_SETFL 4
#define F_GETLK 7
#define F_SETLK 8
#define F_SETLKW 9
#define F_OFD_GETLK 36
#define F_OFD_SETLK 37
#define F_RDLCK 1
#define F_WRLCK 2
#define F_UNLCK 3
#define FD_CLOEXEC 1
#ifndef O_CLOEXEC
#define O_CLOEXEC 0x80000
#endif
#ifndef O_NONBLOCK
#define O_NONBLOCK 0x4000
#endif
#endif
