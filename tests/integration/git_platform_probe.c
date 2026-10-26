#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int create_file(const char *path, const char *data)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd < 0)
        return -1;
    size_t bytes = strlen(data);
    int failed = write(fd, data, bytes) != (ssize_t)bytes || fsync(fd);
    if (close(fd))
        failed = 1;
    return failed ? -1 : 0;
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    char source[128], target[128], pack[128];
    if (snprintf(source, sizeof(source), "%s/source", argv[1]) >= sizeof(source) ||
        snprintf(target, sizeof(target), "%s/target", argv[1]) >= sizeof(target) ||
        snprintf(pack, sizeof(pack), "%s/pack-0123456789012345678901234567890123456789.pack",
                 argv[1]) >= sizeof(pack))
        return 2;
    unsigned missing = 0;
    struct timespec now = {0};
    int result = clock_gettime(CLOCK_REALTIME, &now);
    printf("GIT_PLATFORM_UTC result=%d errno=%d seconds=%lld\n", result, result ? errno : 0,
           (long long)now.tv_sec);
    if (result || now.tv_sec < 946684800)
        missing |= 1;
    if (create_file(source, "new") || create_file(target, "old"))
        return 2;
    int duplicate = open(source, O_WRONLY | O_CREAT | O_EXCL, 0600);
    int exclusive = duplicate < 0 && errno == EEXIST;
    if (duplicate >= 0)
        close(duplicate);
    printf("GIT_PLATFORM_EXCLUSIVE result=%d\n", exclusive);
    if (!exclusive)
        missing |= 2;
    result = rename(source, target);
    int saved = errno;
    char data[4] = {0};
    int fd = open(target, O_RDONLY);
    ssize_t bytes = fd < 0 ? -1 : read(fd, data, 3);
    if (fd >= 0)
        close(fd);
    printf("GIT_PLATFORM_REPLACE result=%d errno=%d content=%s\n", result, result ? saved : 0,
           data);
    if (result || bytes != 3 || memcmp(data, "new", 3))
        missing |= 4;
    result = create_file(pack, "pack");
    printf("GIT_PLATFORM_PACK_NAME result=%d errno=%d\n", result, result ? errno : 0);
    if (result)
        missing |= 8;
    unlink(source);
    unlink(target);
    unlink(pack);
    printf("GIT_PLATFORM_%s missing=%u\n", missing ? "INCOMPLETE" : "PASS", missing);
    return missing ? 1 : 0;
}
