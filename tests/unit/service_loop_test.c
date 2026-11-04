#include "service_platform.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static unsigned pending, receives, replies, responses, ticks, sleeps, delay;
static unsigned fail_at;
static unsigned waits, cancels;
static uint64_t clock_ms;
static bool client_mode, client_ready, late_ready, wait_timeout;
static int wait_error;

static void tick(void)
{
    ++ticks;
}

static void respond(struct lume_supervision_request *request)
{
    assert(request->sequence == responses + 1);
    ++responses;
}

int service_call(unsigned operation, struct lume_supervision_request *request)
{
    assert(request->version == LUME_SUPERVISION_VERSION);
    if (client_mode)
    {
        assert(request->sequence == 9);
        if (operation == LUME_SV_CANCEL)
        {
            ++cancels;
            return 0;
        }
        assert(operation == LUME_SV_RESULT);
        return client_ready ? 0 : -EAGAIN;
    }
    if (operation == LUME_SV_RECEIVE)
    {
        struct lume_supervision_request empty = {.version = LUME_SUPERVISION_VERSION};
        assert(!memcmp(request, &empty, sizeof(empty)));
        ++receives;
        if (receives == fail_at)
            return -EIO;
        if (!pending)
            return -EAGAIN;
        --pending;
        request->sequence = responses + 1;
        return 0;
    }
    assert(operation == LUME_SV_REPLY && request->sequence == replies + 1);
    ++replies;
    return 0;
}

void service_sleep(unsigned milliseconds)
{
    ++sleeps;
    delay = milliseconds;
    clock_ms += milliseconds;
    if (client_mode)
        client_ready = true;
}

uint64_t service_now(void)
{
    return clock_ms;
}

int service_wait(uint64_t sequence, uint64_t timeout)
{
    ++waits;
    delay = (unsigned)timeout;
    if (wait_error)
        return wait_error;
    if (!client_mode)
    {
        assert(!sequence && timeout == 100);
        return 0;
    }
    assert(sequence == 9 && timeout == 30000);
    if (wait_timeout || late_ready)
        clock_ms += timeout + late_ready;
    client_ready = !wait_timeout;
    return wait_timeout ? 0 : 1;
}

#include "service_loop.h"

static void reset(unsigned count)
{
    pending = count;
    receives = replies = responses = ticks = sleeps = delay = fail_at = 0;
    waits = cancels = 0;
    clock_ms = 0;
    client_mode = client_ready = late_ready = wait_timeout = false;
    wait_error = 0;
}

int main(void)
{
    reset(0);
    assert(!service_step() && receives == 1 && !replies && ticks == 1 && delay == 100 &&
           waits == 1 && !sleeps);
    reset(3);
    assert(!service_step() && receives == 4 && replies == 3 && ticks == 1 && delay == 100);
    reset(LUME_SUPERVISION_COUNT);
    assert(!service_step() && receives == LUME_SUPERVISION_COUNT &&
           replies == LUME_SUPERVISION_COUNT && ticks == 1 && sleeps == 1 && !delay && !waits);
    reset(LUME_SUPERVISION_COUNT + 3);
    assert(!service_step() && pending == 3 && ticks == 1 && !delay);
    assert(!service_step() && !pending && replies == LUME_SUPERVISION_COUNT + 3 &&
           receives == LUME_SUPERVISION_COUNT + 4 && ticks == 2 && sleeps == 1 && waits == 1 &&
           delay == 100);
    reset(4);
    fail_at = 3;
    assert(service_step() == -EIO && replies == 2 && ticks == 1 && !sleeps);
    reset(0);
    wait_error = -EINVAL;
    assert(!service_step() && waits == 1 && sleeps == 1 && delay == 100);
    reset(0);
    struct lume_supervision_request request = {.version = 1, .sequence = 9};
    client_mode = true;
    late_ready = true;
    assert(!service_result_wait(&request) && waits == 1 && clock_ms == 30001 && !cancels);
    reset(0);
    client_mode = true;
    wait_timeout = true;
    assert(service_result_wait(&request) == -ETIMEDOUT && cancels == 1 && !sleeps);
    reset(0);
    client_mode = true;
    wait_error = -EINTR;
    assert(service_result_wait(&request) == -EINTR && cancels == 1);
    reset(0);
    client_mode = true;
    wait_error = -ENOSYS;
    assert(!service_result_wait(&request) && waits == 1 && sleeps == 1 && delay == 50);
    puts("PASS service request batches, maintenance, idle waits and errors");
    return 0;
}
