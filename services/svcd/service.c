#include "service_platform.h"
#include "service_policy.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef SERVICE_ROOT
#define SERVICE_ROOT "/data"
#endif
#define SERVICE_INSTALLED SERVICE_ROOT "/share/services"
#define SERVICE_OVERRIDE SERVICE_ROOT "/etc/services"
#define SERVICE_ENABLED SERVICE_ROOT "/etc/services/enabled"
#define SERVICE_LOG SERVICE_ROOT "/var/log"

static struct supervised_service services[LUME_SUPERVISION_COUNT];
static unsigned service_count;

static bool valid_name(const char *name)
{
    unsigned n = 0;
    for (; name[n]; ++n)
        if (n + 1 >= LUME_SUPERVISION_NAME ||
            !((name[n] >= 'a' && name[n] <= 'z') || (name[n] >= '0' && name[n] <= '9') ||
              name[n] == '-' || name[n] == '_'))
            return false;
    return n != 0;
}

static int directory(const char *path)
{
    struct stat stat;
    if (!mkdir(path, 0700))
        return 0;
    if (errno != EEXIST || lstat(path, &stat) || !S_ISDIR(stat.st_mode))
        return -errno;
    return 0;
}

static int prepare(void)
{
    static const char *const paths[] = {SERVICE_ROOT "/etc", SERVICE_OVERRIDE, SERVICE_ENABLED,
                                        SERVICE_ROOT "/var", SERVICE_LOG};
    for (unsigned i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i)
    {
        int result = directory(paths[i]);
        if (result)
            return result;
    }
    int fd = open(SERVICE_ROOT "/var/empty", O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0)
        return -errno;
    return close(fd) ? -errno : 0;
}

static bool is_enabled(const char *name)
{
    char path[128];
    struct stat stat;
    snprintf(path, sizeof(path), "%s/%s", SERVICE_ENABLED, name);
    return !lstat(path, &stat) && S_ISREG(stat.st_mode);
}

static int enabled(struct supervised_service *service, bool enable)
{
    char path[128];
    snprintf(path, sizeof(path), "%s/%s", SERVICE_ENABLED, service->status.name);
    if (enable)
    {
        int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
        if (fd < 0)
            return errno == EEXIST && is_enabled(service->status.name) ? 0 : -errno;
        int result = fsync(fd) ? -errno : 0;
        if (close(fd) && !result)
            result = -errno;
        if (result)
            (void)unlink(path);
        return result;
    }
    return !unlink(path) || errno == ENOENT ? 0 : -errno;
}

static int configuration(struct supervised_service *service)
{
    char path[128], text[SERVICE_TEXT_BYTES + 1];
    snprintf(path, sizeof(path), "%s/%s.svc", SERVICE_OVERRIDE, service->status.name);
    FILE *file = fopen(path, "rb");
    if (!file && errno == ENOENT)
    {
        snprintf(path, sizeof(path), "%s/%s.svc", SERVICE_INSTALLED, service->status.name);
        file = fopen(path, "rb");
    }
    if (!file)
        return -errno;
    size_t count = fread(text, 1, sizeof(text), file);
    bool failed = ferror(file);
    int closed = fclose(file);
    if (failed || closed)
        return -EIO;
    return service_config_parse(&service->config, text, count) ? 0 : -EINVAL;
}

static struct supervised_service *find(const char *name)
{
    for (unsigned i = 0; i < service_count; ++i)
        if (!strcmp(services[i].status.name, name))
            return &services[i];
    return NULL;
}

static int discover(const char *path)
{
    DIR *directory = opendir(path);
    if (!directory)
        return errno == ENOENT ? 0 : -errno;
    int result = 0;
    struct dirent *entry;
    errno = 0;
    while ((entry = readdir(directory)) != NULL)
    {
        size_t n = strlen(entry->d_name);
        if (n <= 4 || n - 4 >= LUME_SUPERVISION_NAME || strcmp(entry->d_name + n - 4, ".svc"))
            continue;
        char name[LUME_SUPERVISION_NAME] = {0};
        memcpy(name, entry->d_name, n - 4);
        if (!valid_name(name) || find(name))
            continue;
        if (service_count == LUME_SUPERVISION_COUNT)
        {
            result = -ENOSPC;
            break;
        }
        struct supervised_service *service = &services[service_count++];
        memcpy(service->status.name, name, sizeof(name));
        service->status.enabled = is_enabled(name);
        int error = configuration(service);
        if (error)
        {
            service->status.state = LUME_SV_FAILED;
            service->status.error = error;
        }
        errno = 0;
    }
    if (!result && errno)
        result = -errno;
    if (closedir(directory) && !result)
        result = -errno;
    return result;
}

static int launch(struct supervised_service *service)
{
    int result = configuration(service);
    if (result)
    {
        service->status.error = result;
        service->status.state = LUME_SV_FAILED;
        service->desired = false;
        return result;
    }
    char *argv[SERVICE_ARGUMENTS + 1];
    argv[0] = service->config.program;
    for (unsigned i = 0; i < service->config.arguments; ++i)
        argv[i + 1] = service->config.argument[i];
    argv[service->config.arguments + 1] = NULL;
    char *envp[] = {"PATH=/data/bin:/bin:/usr/bin", "HOME=/data/home", "USER=root", "TERM=dumb",
                    NULL};
    char log[128];
    snprintf(log, sizeof(log), "%s/%s.log", SERVICE_LOG, service->status.name);
    posix_spawn_file_actions_t actions;
    result = posix_spawn_file_actions_init(&actions);
    if (!result)
    {
        result =
            posix_spawn_file_actions_addopen(&actions, 0, SERVICE_ROOT "/var/empty", O_RDONLY, 0);
        if (!result)
            result = posix_spawn_file_actions_addopen(&actions, 1, log,
                                                      O_WRONLY | O_CREAT | O_APPEND, 0600);
        if (!result)
            result = posix_spawn_file_actions_adddup2(&actions, 1, 2);
        pid_t pid;
        if (!result)
            result = posix_spawn(&pid, argv[0], &actions, NULL, argv, envp);
        (void)posix_spawn_file_actions_destroy(&actions);
        if (!result)
        {
            service_started(service, (unsigned)pid, service_now());
            return 0;
        }
    }
    service->status.error = -result;
    service->started = service_now();
    service_exited(service, 127, service->started);
    return -result;
}

static int stop(struct supervised_service *service)
{
    if (!service->status.pid)
    {
        service->status.state = service->restart_requested ? LUME_SV_BACKOFF : LUME_SV_STOPPED;
        service->deadline = service_now();
        return 0;
    }
    struct lume_supervision_request request = {.version = LUME_SUPERVISION_VERSION,
                                               .command = service->status.pid};
    service->status.state = LUME_SV_STOPPING;
    int result = service_call(LUME_SV_STOP_CHILD, &request);
    if (result == -EBUSY)
        return 0;
    if (result)
        service->status.error = result;
    return result;
}

static void tick(void)
{
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
        for (unsigned i = 0; i < service_count; ++i)
            if (services[i].status.pid == (unsigned)pid)
                service_exited(&services[i], WEXITSTATUS(status), service_now());
    for (unsigned i = 0; i < service_count; ++i)
    {
        struct supervised_service *service = &services[i];
        if (service->status.state == LUME_SV_STOPPING && service->status.pid)
            (void)stop(service);
        if (service->status.state == LUME_SV_BACKOFF && service->desired &&
            service_now() >= service->deadline)
        {
            ++service->status.restarts;
            (void)launch(service);
        }
    }
}

static void respond(struct lume_supervision_request *request)
{
    request->count = 0;
    request->result = discover(SERVICE_OVERRIDE);
    if (!request->result)
        request->result = discover(SERVICE_INSTALLED);
    if (request->result)
        return;
    if (request->command == LUME_SV_LIST)
    {
        request->count = service_count;
        for (unsigned i = 0; i < service_count; ++i)
            request->service[i] = services[i].status;
        return;
    }
    struct supervised_service *service = find(request->name);
    if (!service)
    {
        request->result = -ENOENT;
        return;
    }
    switch (request->command)
    {
    case LUME_SV_START:
        if (service->status.state == LUME_SV_STOPPING)
            request->result = -EBUSY;
        else if (!service->status.pid)
        {
            service->desired = true;
            service->failures = 0;
            request->result = launch(service);
        }
        break;
    case LUME_SV_STOP:
    case LUME_SV_RESTART:
        service->desired = service->restart_requested = request->command == LUME_SV_RESTART;
        request->result = stop(service);
        break;
    case LUME_SV_ENABLE:
    case LUME_SV_DISABLE:
        request->result = enabled(service, request->command == LUME_SV_ENABLE);
        service->status.enabled = is_enabled(service->status.name);
        break;
    case LUME_SV_STATUS:
        break;
    default:
        request->result = -EINVAL;
        break;
    }
    request->service[0] = service->status;
    request->count = 1;
}

static int daemon_main(void)
{
    struct lume_supervision_request request = {.version = LUME_SUPERVISION_VERSION};
    int result = service_call(LUME_SV_REGISTER, &request);
    if (!result)
        result = prepare();
    if (!result)
        result = discover(SERVICE_OVERRIDE);
    if (!result)
        result = discover(SERVICE_INSTALLED);
    if (result)
    {
        fprintf(stderr, "svcd: initialization failed (%d)\n", result);
        return 1;
    }
    for (unsigned i = 0; i < service_count; ++i)
        if (services[i].status.enabled && !services[i].status.error)
        {
            services[i].desired = true;
            (void)launch(&services[i]);
        }
    for (;;)
    {
        tick();
        request = (struct lume_supervision_request){.version = LUME_SUPERVISION_VERSION};
        result = service_call(LUME_SV_RECEIVE, &request);
        if (!result)
        {
            respond(&request);
            (void)service_call(LUME_SV_REPLY, &request);
        }
        else if (result != -EAGAIN)
        {
            fprintf(stderr, "svcd: control channel failed (%d)\n", result);
            return 1;
        }
        service_sleep(100);
    }
}

int main(int argc, char **argv)
{
    const char *name = strrchr(argv[0], '/');
    name = name ? name + 1 : argv[0];
    if ((!strcmp(name, "svcd") || !strcmp(name, "svcd.elf")) && argc == 1)
        return daemon_main();
    static const char *const commands[] = {"list",    "status", "start",  "stop",
                                           "restart", "enable", "disable"};
    unsigned command;
    for (command = 0; command < sizeof(commands) / sizeof(commands[0]); ++command)
        if (argc >= 2 && !strcmp(commands[command], argv[1]))
            break;
    if (command == sizeof(commands) / sizeof(commands[0]) ||
        argc != (command == LUME_SV_LIST ? 2 : 3) ||
        (command != LUME_SV_LIST && !valid_name(argv[2])))
    {
        fputs("usage: service list | status|start|stop|restart|enable|disable NAME\n", stderr);
        return 2;
    }
    struct lume_supervision_request request = {.version = LUME_SUPERVISION_VERSION,
                                               .command = command};
    if (command != LUME_SV_LIST)
        strcpy(request.name, argv[2]);
    int result = service_call(LUME_SV_SUBMIT, &request);
    if (!result)
    {
        uint64_t started = service_now();
        while ((result = service_call(LUME_SV_RESULT, &request)) == -EAGAIN)
        {
            if (service_now() - started >= 30000)
            {
                (void)service_call(LUME_SV_CANCEL, &request);
                fputs("service: request timed out. Check status before retrying a change.\n",
                      stderr);
                return 1;
            }
            service_sleep(50);
        }
        if (!result)
            result = request.result;
    }
    if (result)
    {
        fprintf(stderr, "service: operation failed (%d)\n", result);
        return 1;
    }
    static const char *const states[] = {"stopped", "running", "stopping", "backoff", "failed"};
    for (unsigned i = 0; i < request.count; ++i)
    {
        const struct lume_service_status *service = &request.service[i];
        printf("%s %s pid=%u enabled=%u restarts=%u exit=%d error=%d\n", service->name,
               service->state < sizeof(states) / sizeof(states[0]) ? states[service->state]
                                                                   : "unknown",
               service->pid, service->enabled, service->restarts, service->exit_code,
               service->error);
    }
    return 0;
}
