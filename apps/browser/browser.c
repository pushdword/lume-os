/* SPDX-License-Identifier: BSD-3-Clause */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static int owned_directory(const char *path, unsigned private)
{
    struct stat info;
    if (stat(path, &info))
        return -1;
    if (!S_ISDIR(info.st_mode) || info.st_uid != geteuid() ||
        (info.st_mode & (private ? 0077 : 0022)))
    {
        errno = EACCES;
        return -1;
    }
    return 0;
}

static int chromium_temporary_directory(const char *home)
{
    char root[PATH_MAX], cache[PATH_MAX], temporary[PATH_MAX];
    if (!realpath(home && *home ? home : "/data", root))
        return -1;
    if (strncmp(root, "/data", 5) || (root[5] && root[5] != '/') || owned_directory(root, 0))
    {
        errno = EACCES;
        return -1;
    }
    int bytes = snprintf(cache, sizeof(cache), "%s/.cache", root);
    if (bytes < 0 || (size_t)bytes >= sizeof(cache))
    {
        errno = ENAMETOOLONG;
        return -1;
    }
    if ((mkdir(cache, 0700) && errno != EEXIST) || owned_directory(cache, 0))
        return -1;
    bytes = snprintf(temporary, sizeof(temporary), "%s/chromium-tmp", cache);
    if (bytes < 0 || (size_t)bytes >= sizeof(temporary))
    {
        errno = ENAMETOOLONG;
        return -1;
    }
    if ((mkdir(temporary, 0700) && errno != EEXIST) || owned_directory(temporary, 1))
        return -1;
    return setenv("TMPDIR", temporary, 1);
}

int main(int argc, char **argv)
{
    const char *home = getenv("HOME");
    if (home && *home && mkdir(home, 0700) && errno != EEXIST)
        perror("browser: home");
    struct timespec utc;
    if (clock_gettime(CLOCK_REALTIME, &utc))
    {
        pid_t child = fork();
        if (!child)
        {
            execlp("clock", "clock", "sync", (char *)NULL);
            _exit(127);
        }
        if (child > 0)
        {
            int status;
            while (waitpid(child, &status, 0) < 0 && errno == EINTR)
            {
            }
        }
    }
    if (mkdir("/data/downloads", 0700) && errno != EEXIST)
        perror("browser: downloads");
    const char *chromium = "/data/libexec/chromium/chromium";
    if (!access(chromium, X_OK))
    {
        if (clock_gettime(CLOCK_REALTIME, &utc))
        {
            perror("browser: system time");
            return 1;
        }
        if (chromium_temporary_directory(home))
        {
            perror("browser: temporary directory");
            return 1;
        }
        if (setenv("FONTCONFIG_FILE", "/data/libexec/chromium/fonts.conf", 1) ||
            setenv("FONTCONFIG_PATH", "/data/libexec/chromium", 1))
        {
            perror("browser: font configuration");
            return 1;
        }
        char **arguments = calloc((size_t)argc + 4, sizeof(*arguments));
        if (!arguments)
        {
            perror("browser: arguments");
            return 1;
        }
        arguments[0] = "chromium";
        arguments[1] = "--ozone-platform=x11";
        arguments[2] = "--disable-gpu";
        arguments[3] = "--no-zygote";
        for (int i = 1; i < argc; ++i)
            arguments[i + 3] = argv[i];
        execv(chromium, arguments);
        perror("browser: chromium");
        free(arguments);
        return 1;
    }
    argv[0] = "dillo";
    (void)argc;
    execvp("dillo", argv);
    perror("browser: dillo");
    return 1;
}
