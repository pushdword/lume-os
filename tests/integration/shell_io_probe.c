#include "../../include/internal/lume/app_support.h"
#include <fd.h>

static void number(unsigned n)
{
    char digits[16];
    unsigned used = 0;
    do
    {
        digits[used++] = (char)('0' + n % 10);
        n /= 10;
    } while (n);
    while (used)
        (void)write(1, digits + --used, 1);
}

int main(int argc, char **argv)
{
    if (argc == 2 && eq(argv[1], "emit"))
    {
        (void)write(1, "OUT\n", 4);
        (void)write(2, "ERR\n", 4);
        return 7;
    }
    struct lume_fd_request request = {.version = LUME_FD_VERSION, .bytes = sizeof(request)};
    struct lume_fd_info info;
    request.buffer = (uint32_t)(uintptr_t)&info;
    if (call(LUME_SYS_FD, LUME_FD_INFO, (uintptr_t)&request, sizeof(request)) || info.open != 3)
        return 1;
    puts("FD open=");
    number(info.open);
    puts(" objects=");
    number(info.objects);
    puts(" pipes=");
    number(info.pipes);
    puts("\n");
    request.version = 0;
    if (call(LUME_SYS_FD, LUME_FD_INFO, (uintptr_t)&request, sizeof(request)) != -LUME_EINVAL)
        return 2;
    if (call(LUME_SYS_FD, LUME_FD_PIPE, UINT64_MAX, sizeof(request)) != -LUME_EFAULT)
        return 3;
    request = (struct lume_fd_request){
        .version = LUME_FD_VERSION, .bytes = sizeof(request), .fd = -1, .flags = LUME_FD_NONBLOCK};
    if (call(LUME_SYS_FD, LUME_FD_PIPE, (uintptr_t)&request, sizeof(request)))
        return 4;
    int reading = request.fd, writing = request.other;
    char byte;
    if (read(reading, &byte, 1) != -LUME_EAGAIN)
        return 5;
    struct lume_pollfd poll = {.fd = reading, .events = LUME_POLLIN};
    request = (struct lume_fd_request){.version = LUME_FD_VERSION,
                                       .bytes = sizeof(request),
                                       .fd = -1,
                                       .buffer = (uint32_t)(uintptr_t)&poll,
                                       .count = 1};
    if (call(LUME_SYS_FD, LUME_FD_POLL, (uintptr_t)&request, sizeof(request)))
        return 6;
    if (write(writing, "x", 1) != 1 ||
        call(LUME_SYS_FD, LUME_FD_POLL, (uintptr_t)&request, sizeof(request)) != 1 ||
        !(poll.revents & LUME_POLLIN) || read(reading, &byte, 1) != 1 || byte != 'x')
        return 7;
    (void)close(writing);
    if (call(LUME_SYS_FD, LUME_FD_POLL, (uintptr_t)&request, sizeof(request)) != 1 ||
        !(poll.revents & LUME_POLLHUP) || read(reading, &byte, 1))
        return 8;
    (void)close(reading);
    puts("SHELL_FD_ABI_PASS\n");
    return 0;
}
