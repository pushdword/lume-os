#include "lume/app_support.h"
#include "lume/process.h"
#include "lume/credential.h"
#include <fd.h>

static struct lume_fd_info descriptors;
static struct lume_memory_snapshot memory;
static struct lume_system_features features;

static void number(uint64_t value)
{
    char digits[21];
    unsigned at = sizeof(digits);
    do
    {
        digits[--at] = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    (void)write_all(1, digits + at, sizeof(digits) - at);
}

int main(void)
{
    struct lume_fd_request request = {.version = LUME_FD_VERSION,
                                      .bytes = sizeof(request),
                                      .buffer = (uint32_t)(uintptr_t)&descriptors};
    if (call(LUME_SYS_FD, LUME_FD_INFO, (uintptr_t)&request, sizeof(request)) ||
        call(LUME_SYS_MEMORY, (uintptr_t)&memory, sizeof(memory), 0) ||
        call(LUME_SYS_FEATURES, (uintptr_t)&features, sizeof(features), LUME_FEATURE_VERSION))
        return 1;
    puts("RESOURCES open_descriptions=");
    number(descriptors.objects);
    puts(" pipes=");
    number(descriptors.pipes);
    puts(" local_descriptors=");
    number(descriptors.open);
    struct lume_limit_request limit = {
        .version = LUME_CREDENTIAL_VERSION, .bytes = sizeof(limit), .resource = LUME_LIMIT_FILES};
    if (!call(LUME_SYS_CREDENTIAL, LUME_CREDENTIAL_LIMIT_GET, (uintptr_t)&limit, sizeof(limit)))
    {
        puts(" descriptor_soft=");
        number(limit.soft);
        puts(" descriptor_hard=");
        number(limit.hard);
    }
    puts(" free_pages=");
    number(memory.free_pages);
    puts(" total_pages=");
    number(memory.total_pages);
    unsigned states[7] = {0};
    for (unsigned i = 0; i < features.task_limit; ++i)
    {
        struct lume_task_info task;
        if (call(LUME_SYS_PROCESS, i, (uintptr_t)&task, sizeof(task)) > 0 && task.state < 7)
            ++states[task.state];
    }
    for (unsigned i = 1; i < 7; ++i)
    {
        puts(" state");
        number(i);
        puts("=");
        number(states[i]);
    }
    puts("\n");
    return 0;
}
