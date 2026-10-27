#define _DEFAULT_SOURCE 1
#define _POSIX_C_SOURCE 200809L
#include "api.h"
#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    static const char output[] =
        "SDK_STDOUT_BEGIN|0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz"
        "0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz|SDK_STDOUT_END\n";
    static const char error[] =
        "SDK_STDERR_BEGIN|0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz"
        "0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz|SDK_STDERR_END\n";
    if (app_write_all(1, output, sizeof(output) - 1) != sizeof(output) - 1 ||
        app_write_all(2, error, sizeof(error) - 1) != sizeof(error) - 1)
        return 1;
    void *start = sbrk(0);
    if (start == (void *)-1 || sbrk(1) != start || sbrk(0) != (char *)start + 1 ||
        sbrk(-1) != (char *)start + 1 || sbrk(0) != start)
        return 2;
    DIR *directory = opendir("/bin");
    if (!directory)
        return 3;
    int flags = fcntl(directory->fd, F_GETFD);
    if (flags < 0 || !(flags & FD_CLOEXEC))
        return 3;
    struct dirent *entry = readdir(directory);
    if (!entry || directory->count < 2 || directory->next != 1)
        return 4;
    char name[sizeof(entry->d_name)];
    memcpy(name, entry->d_name, sizeof(name));
    rewinddir(directory);
    entry = readdir(directory);
    if (!entry || strcmp(entry->d_name, name) || closedir(directory))
        return 5;
    int descriptors[2];
    if (pipe(descriptors))
        return 6;
    struct pollfd waiting = {.fd = descriptors[0], .events = POLLIN};
    if (poll(&waiting, 1, 10) != 0 || write(descriptors[1], "q", 1) != 1 ||
        poll(&waiting, 1, 100) != 1 || !(waiting.revents & POLLIN))
        return 7;
    char byte;
    if (read(descriptors[0], &byte, 1) != 1 || byte != 'q' || close(descriptors[0]) ||
        close(descriptors[1]))
        return 8;
    puts("SDK_EFFICIENCY_PASS write_all/heap/directory/events");
    return 0;
}
