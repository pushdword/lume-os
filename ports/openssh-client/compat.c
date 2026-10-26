#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

unsigned sleep(unsigned seconds)
{
    struct timespec delay = {seconds, 0}, remaining = {0, 0};
    return nanosleep(&delay, &remaining) ? (unsigned)remaining.tv_sec + !!remaining.tv_nsec : 0;
}

speed_t cfgetispeed(const struct termios *settings)
{
    return settings->c_ispeed;
}

speed_t cfgetospeed(const struct termios *settings)
{
    return settings->c_ospeed;
}

int getrrsetbyname(const char *host, unsigned rdclass, unsigned rdtype, unsigned flags,
                   struct rrsetinfo **result)
{
    (void)host;
    (void)rdclass;
    (void)rdtype;
    (void)flags;
    *result = NULL;
    return ERRSET_FAIL;
}

void freerrset(struct rrsetinfo *result)
{
    if (!result)
        return;
    for (unsigned i = 0; i < result->rri_nrdatas; ++i)
        free(result->rri_rdatas[i].rdi_data);
    for (unsigned i = 0; i < result->rri_nsigs; ++i)
        free(result->rri_sigs[i].rdi_data);
    free(result->rri_rdatas);
    free(result->rri_sigs);
    free(result->rri_name);
    free(result);
}

static ssize_t vector_io(int fd, const struct iovec *vector, int count, int writing)
{
    if (count < 0 || count > 1024 || (!vector && count))
    {
        errno = EINVAL;
        return -1;
    }
    size_t total = 0;
    for (int i = 0; i < count; ++i)
    {
        if (vector[i].iov_len > (size_t)INT_MAX - total)
        {
            errno = EINVAL;
            return -1;
        }
        total += vector[i].iov_len;
    }
    if (!total)
        return 0;
    unsigned char *buffer = malloc(total);
    if (!buffer)
        return -1;
    size_t used = 0;
    if (writing)
        for (int i = 0; i < count; ++i)
        {
            memcpy(buffer + used, vector[i].iov_base, vector[i].iov_len);
            used += vector[i].iov_len;
        }
    ssize_t result = writing ? write(fd, buffer, total) : read(fd, buffer, total);
    if (!writing && result > 0)
    {
        used = 0;
        for (int i = 0; i < count && used < (size_t)result; ++i)
        {
            size_t take = vector[i].iov_len;
            if (take > (size_t)result - used)
                take = (size_t)result - used;
            memcpy(vector[i].iov_base, buffer + used, take);
            used += take;
        }
    }
    free(buffer);
    return result;
}

ssize_t readv(int fd, const struct iovec *vector, int count)
{
    return vector_io(fd, vector, count, 0);
}

ssize_t writev(int fd, const struct iovec *vector, int count)
{
    return vector_io(fd, vector, count, 1);
}
