#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lume_git.h"

extern char **environ;

static size_t key_bytes(const char *value)
{
    const char *end = strchr(value, '=');
    return end ? (size_t)(end - value) : strlen(value);
}

pid_t lume_git_spawn(const char *path, const char **arguments, char **changes,
                     const char *directory, int input, int output, int error)
{
    if (directory)
    {
        errno = ENOTSUP;
        return -1;
    }
    char *environment[65];
    unsigned count = 0;
    for (unsigned i = 0; environ && environ[i]; ++i)
    {
        if (count == 64)
            goto too_many;
        environment[count++] = environ[i];
    }
    for (unsigned i = 0; changes && changes[i]; ++i)
    {
        size_t bytes = key_bytes(changes[i]);
        for (unsigned j = 0; j < count;)
        {
            if (key_bytes(environment[j]) == bytes && !memcmp(environment[j], changes[i], bytes))
                environment[j] = environment[--count];
            else
                ++j;
        }
        if (changes[i][bytes] == '=')
        {
            if (count == 64)
                goto too_many;
            environment[count++] = changes[i];
        }
    }
    environment[count] = NULL;
    posix_spawn_file_actions_t actions;
    int result = posix_spawn_file_actions_init(&actions);
    if (result)
        goto failed;
    const int source[] = {input, output, error};
    for (unsigned i = 0; i < 3 && !result; ++i)
        result = posix_spawn_file_actions_adddup2(&actions, source[i], (int)i);
    for (int i = 3; i < 16 && !result; ++i)
        if (fcntl(i, F_GETFD) >= 0)
            result = posix_spawn_file_actions_addclose(&actions, i);
    pid_t pid = -1;
    if (!result)
        result = posix_spawnp(&pid, path, &actions, NULL, (char *const *)arguments, environment);
    posix_spawn_file_actions_destroy(&actions);
    if (!result)
        return pid;
failed:
    errno = result;
    return -1;
too_many:
    errno = E2BIG;
    return -1;
}
