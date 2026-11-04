#ifndef LUME_SERVICE_LOOP_H
#define LUME_SERVICE_LOOP_H

static int service_result_wait(struct lume_supervision_request *request)
{
    uint64_t started = service_now();
    int result;
    while ((result = service_call(LUME_SV_RESULT, request)) == -EAGAIN)
    {
        uint64_t elapsed = service_now() - started;
        if (elapsed >= 30000)
        {
            (void)service_call(LUME_SV_CANCEL, request);
            return -ETIMEDOUT;
        }
        result = service_wait(request->sequence, 30000 - elapsed);
        if (result == -EINVAL || result == -ENOSYS)
            service_sleep(elapsed > 29950 ? (unsigned)(30000 - elapsed) : 50);
        else if (result < 0)
        {
            (void)service_call(LUME_SV_CANCEL, request);
            return result;
        }
    }
    return result;
}

static int service_step(void)
{
    tick();
    for (unsigned handled = 0; handled < LUME_SUPERVISION_COUNT; ++handled)
    {
        struct lume_supervision_request request = {.version = LUME_SUPERVISION_VERSION};
        int result = service_call(LUME_SV_RECEIVE, &request);
        if (result == -EAGAIN)
        {
            result = service_wait(0, 100);
            if (result == -EINVAL || result == -ENOSYS)
                service_sleep(100);
            else if (result < 0 && result != -EINTR)
                return result;
            return 0;
        }
        if (result)
            return result;
        respond(&request);
        (void)service_call(LUME_SV_REPLY, &request);
    }
    service_sleep(0);
    return 0;
}

#endif
