#include "service_platform.h"
#include <errno.h>
#include <pthread.h>
#include <spawn.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void *loop(void *unused)
{
    (void)unused;
    for (;;)
        service_sleep(100);
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    puts("SERVICE_PROBE_STARTED");
    if (!strcmp(argv[1], "once"))
        return 0;
    if (!strcmp(argv[1], "fail"))
        return 7;
    if (!strcmp(argv[1], "deny"))
    {
        struct lume_supervision_request request = {.version = LUME_SUPERVISION_VERSION};
        if (service_call(LUME_SV_REGISTER, &request) != -EPERM ||
            service_call(LUME_SV_SUBMIT, &request) != -EPERM)
            return 1;
        puts("SERVICE_CAPABILITIES_PASS");
        return 0;
    }
    if (!strcmp(argv[1], "child") || !strcmp(argv[1], "exit-child") ||
        !strcmp(argv[1], "fail-child") || !strcmp(argv[1], "tree"))
    {
        pid_t pid;
        char *args[] = {argv[0], !strcmp(argv[1], "tree") ? "child" : "threads", NULL};
        char *environment[] = {"PATH=/data/bin:/bin:/usr/bin", NULL};
        posix_spawn_file_actions_t actions;
        if (posix_spawn_file_actions_init(&actions))
            return 1;
        int result = posix_spawn(&pid, argv[0], &actions, NULL, args, environment);
        (void)posix_spawn_file_actions_destroy(&actions);
        if (result)
            return 1;
        printf("SERVICE_CHILD_PID=%d\n", (int)pid);
        fflush(stdout);
        service_sleep(500);
        if (!strcmp(argv[1], "exit-child"))
            return 0;
        if (!strcmp(argv[1], "fail-child"))
            return 7;
    }
    else if (!strcmp(argv[1], "threads"))
    {
        pthread_t thread;
        if (pthread_create(&thread, NULL, loop, NULL))
            return 1;
        puts("SERVICE_THREAD_STARTED");
    }
    else if (strcmp(argv[1], "loop"))
        return 2;
    fflush(stdout);
    (void)loop(NULL);
    return 0;
}
