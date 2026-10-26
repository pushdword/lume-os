#include "service_policy.h"
#include <assert.h>
#include <stdio.h>

static void reject(const char *text)
{
    struct service_config before = {.program = "/unchanged"}, config = before;
    assert(!service_config_parse(&config, text, strlen(text)));
    assert(!memcmp(&config, &before, sizeof(config)));
}

int main(void)
{
    struct service_config config;
    const char text[] = "# foreground service\r\nexec=/data/bin/server\r\narg=-D\r\n"
                        "arg=two words\r\narg=\r\nrestart=always\r\nrestart-delay-ms=250";
    assert(service_config_parse(&config, text, sizeof(text) - 1));
    assert(!strcmp(config.program, "/data/bin/server"));
    assert(config.arguments == 3 && !strcmp(config.argument[1], "two words"));
    assert(!config.argument[2][0]);
    assert(config.restart == SERVICE_RESTART_ALWAYS && config.delay_ms == 250);
    reject("");
    reject("arg=x\n");
    reject("exec=relative\n");
    reject("exec=/\n");
    reject("exec=/data//bin/server\n");
    reject("exec=/data/../bin/server\n");
    reject("exec=/data/./bin/server\n");
    reject("exec=/data/bin/server/\n");
    reject("exec=/data/bin/a\nexec=/data/bin/b\n");
    reject("exec=/data/bin/a\nrestart=always\nrestart=never\n");
    reject("exec=/data/bin/a\nrestart=sometimes\n");
    reject("exec=/data/bin/a\nrestart-delay-ms=99\n");
    reject("exec=/data/bin/a\nrestart-delay-ms=60001\n");
    reject("exec=/data/bin/a\nrestart-delay-ms=999999999999999999999999999999\n");
    reject("exec=/data/bin/a\nrestart-delay-ms=-1\n");
    reject("exec=/data/bin/a\nunknown=value\n");
    const char binary[] = "exec=/data/bin/a\0arg=x";
    assert(!service_config_parse(&config, binary, sizeof(binary) - 1));
    char many[1024] = "exec=/data/bin/a\n";
    for (unsigned i = 0; i < SERVICE_ARGUMENTS - 1; ++i)
        strcat(many, "arg=x\n");
    assert(service_config_parse(&config, many, strlen(many)));
    strcat(many, "arg=x\n");
    reject(many);

    struct supervised_service service = {
        .config = {.restart = SERVICE_RESTART_FAILURE, .delay_ms = 1000}, .desired = true};
    service_started(&service, 4, 0);
    service_exited(&service, 1, 10);
    assert(!service.status.pid && service.status.state == LUME_SV_BACKOFF);
    assert(service.deadline == 1010 && service.failures == 1);
    service_started(&service, 4, 1010);
    service_exited(&service, 1, 1020);
    assert(service.deadline == 3020 && service.failures == 2);
    for (unsigned i = 0; i < 20; ++i)
    {
        uint64_t at = service.deadline;
        service_started(&service, 4, at);
        service_exited(&service, 1, at + 1);
        assert(service.deadline - at - 1 <= 60000);
    }
    service_started(&service, 4, 1000000);
    service_exited(&service, 1, 1002000);
    assert(service.failures == 0 && service.deadline == 1003000);
    service_started(&service, 4, 2000000);
    service_exited(&service, 0, 2000001);
    assert(service.status.state == LUME_SV_STOPPED);
    service.config.restart = SERVICE_RESTART_NEVER;
    service_started(&service, 4, 3000000);
    service_exited(&service, 1, 3000001);
    assert(service.status.state == LUME_SV_FAILED);
    service_started(&service, 4, 4000000);
    service.desired = service.restart_requested = true;
    service_exited(&service, 143, 4000001);
    assert(service.status.state == LUME_SV_BACKOFF && service.deadline == 4000001);
    service_started(&service, 4, 5000000);
    service.desired = false;
    service_exited(&service, 143, 5000001);
    assert(service.status.state == LUME_SV_STOPPED);
    puts("PASS service configuration, bounded arguments and restart lifecycle");
    return 0;
}
