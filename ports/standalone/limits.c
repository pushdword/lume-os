#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <lume/abi.h>

long sysconf(int name)
{
    switch (name)
    {
    case _SC_ARG_MAX:
        return LUME_ARG_BYTES;
    case _SC_OPEN_MAX:
        return LUME_FD_MAX;
    case _SC_PAGESIZE:
        return 4096;
    default:
        errno = EINVAL;
        return -1;
    }
}

clock_t clock(void)
{
    errno = ENOSYS;
    return (clock_t)-1;
}

int getloadavg(double *samples, int count)
{
    (void)samples;
    (void)count;
    errno = ENOSYS;
    return -1;
}
