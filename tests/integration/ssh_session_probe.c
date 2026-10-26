#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

int main(int argc, char **argv)
{
    if (getuid() || geteuid() || getgid() || getegid())
        return 1;
    if (argc == 4 && !strcmp(argv[1], "mode"))
    {
        char *end;
        unsigned mode = strtoul(argv[3], &end, 8);
        return *end || mode > 0777 || chmod(argv[2], mode) ? 1 : 0;
    }
    if (argc == 2 && !strcmp(argv[1], "sleep"))
        for (;;)
        {
            unsigned char byte;
            if (read(0, &byte, 1) <= 0)
                break;
        }
    char buffer[4096];
    size_t total = 0;
    for (;;)
    {
        ssize_t count = read(0, buffer, sizeof(buffer));
        if (count < 0 && errno == EINTR)
            continue;
        if (count < 0)
            return 2;
        if (!count)
            break;
        size_t done = 0;
        while (done < (size_t)count)
        {
            ssize_t sent = write(1, buffer + done, count - done);
            if (sent < 0 && errno == EINTR)
                continue;
            if (sent <= 0)
                return 3;
            done += sent;
        }
        total += count;
    }
    fprintf(stderr, "SSH_STDIN_PASS bytes=%zu\n", total);
    return 17;
}
