#include "service_platform.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define SERVICE_OVERRIDE "overrides"
#define SERVICE_INSTALLED "installed"

static unsigned scans, override_scans, installed_scans, requests, replies, observed_count;
static unsigned available_count, snapshot_count, maintenance, waits;
static int override_error, reply_error;
static bool expect_installed;

static int discover(const char *path)
{
    ++scans;
    if (!strcmp(path, SERVICE_OVERRIDE))
    {
        ++override_scans;
        expect_installed = !override_error;
        return override_error;
    }
    assert(!strcmp(path, SERVICE_INSTALLED));
    assert(expect_installed);
    expect_installed = false;
    ++installed_scans;
    snapshot_count = available_count;
    return 0;
}

#include "service_discovery.h"

static void tick(void)
{
    ++maintenance;
}

static void respond(struct lume_supervision_request *request)
{
    request->result = discovery_refresh();
    request->count = request->result ? 0 : snapshot_count;
}

int service_call(unsigned operation, struct lume_supervision_request *request)
{
    if (operation == LUME_SV_RECEIVE)
    {
        if (!requests)
            return -EAGAIN;
        --requests;
        request->command = LUME_SV_LIST;
        return 0;
    }
    assert(operation == LUME_SV_REPLY);
    assert(request->result == reply_error);
    if (!reply_error)
        assert(request->count == observed_count);
    ++replies;
    ++available_count;
    return 0;
}

uint64_t service_now(void)
{
    return 0;
}

void service_sleep(unsigned milliseconds)
{
    assert(!milliseconds);
}

int service_wait(uint64_t sequence, uint64_t milliseconds)
{
    assert(!sequence && milliseconds == 100);
    ++waits;
    return 0;
}

#include "service_loop.h"

int main(void)
{
    assert(!service_step());
    assert(!scans && waits == 1 && maintenance == 1);
    available_count = observed_count = 1;
    requests = LUME_SUPERVISION_COUNT;
    assert(!service_step());
    assert(scans == 2 && replies == LUME_SUPERVISION_COUNT && snapshot_count == 1);
    observed_count = available_count;
    requests = 2;
    assert(!service_step());
    assert(scans == 4 && snapshot_count == observed_count && waits == 2);
    unsigned previous = scans;
    override_error = reply_error = -EIO;
    requests = LUME_SUPERVISION_COUNT;
    assert(!service_step());
    assert(scans == previous + 1 && replies == 2 * LUME_SUPERVISION_COUNT + 2);
    override_error = reply_error = 0;
    observed_count = available_count;
    requests = 1;
    assert(!service_step());
    assert(scans == previous + 3 && snapshot_count == observed_count);
    assert(override_scans == 4 && installed_scans == 3);
    puts("PASS service discovery snapshots, empty batches, new entries and error retry");
    return 0;
}
