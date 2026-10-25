#ifndef LUME_SERVICE_POLICY_H
#define LUME_SERVICE_POLICY_H
#include "service_config.h"
#include "lume/supervision.h"

struct supervised_service
{
    struct lume_service_status status;
    struct service_config config;
    bool desired, restart_requested;
    uint64_t started, deadline;
    unsigned failures;
};

static void service_started(struct supervised_service *service, unsigned pid, uint64_t now)
{
    service->status.pid = pid;
    service->status.state = LUME_SV_RUNNING;
    service->status.error = 0;
    service->started = now;
    service->restart_requested = false;
}

static void service_exited(struct supervised_service *service, int code, uint64_t now)
{
    service->status.pid = 0;
    service->status.exit_code = code;
    if (!service->desired || (!service->restart_requested &&
                              (service->config.restart == SERVICE_RESTART_NEVER ||
                               (service->config.restart == SERVICE_RESTART_FAILURE && !code))))
    {
        service->status.state = code && service->desired ? LUME_SV_FAILED : LUME_SV_STOPPED;
        service->failures = 0;
        return;
    }
    uint64_t delay = service->config.delay_ms;
    if (service->restart_requested)
        delay = 0;
    else
    {
        if (now - service->started >= 2000)
            service->failures = 0;
        else if (service->failures < 10)
            ++service->failures;
        delay <<= service->failures ? service->failures - 1 : 0;
        if (delay > 60000)
            delay = 60000;
    }
    service->deadline = now > UINT64_MAX - delay ? UINT64_MAX : now + delay;
    service->status.state = LUME_SV_BACKOFF;
}
#endif
