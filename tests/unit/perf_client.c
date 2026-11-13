#include "perf_client.h"
#include <assert.h>
#include <stdio.h>

static unsigned entries = 73, calls;
static int failure;

int32_t lume_syscall(uint32_t number, uint32_t address, uint32_t bytes, uint32_t operation)
{
    assert(number == LUME_PERF_SYSCALL && operation == LUME_PERF_TASK_PAGE);
    assert(bytes == sizeof(struct lume_perf_task_page));
    ++calls;
    if (failure == 1)
        return -22;
    struct lume_perf_task_page *page = (void *)(uintptr_t)address;
    unsigned cursor = page->cursor;
    assert(cursor <= entries);
    *page = (struct lume_perf_task_page){.cursor = cursor, .slots = 128};
    for (unsigned i = cursor; i < entries && page->count < LUME_PERF_PAGE_PROCESSES; ++i)
        page->processes[page->count++] = (struct lume_perf_process){.pid = i + 1};
    page->next = cursor + page->count < entries ? cursor + page->count : 0;
    if (failure == 2)
        page->next = cursor;
    if (failure == 3)
        page->count = LUME_PERF_PAGE_PROCESSES + 1;
    return 0;
}

int main(void)
{
    struct lume_perf_task_list list = {0};
    struct lume_perf_snapshot fallback = {.process_count = 1, .processes = {{.pid = 77}}};
    assert(!lume_perf_tasks_read(&list, &fallback));
    assert(calls == 3 && list.count == entries && list.capacity >= entries);
    for (unsigned i = 0; i < entries; ++i)
        assert(list.entries[i].pid == i + 1);
    entries = 0;
    assert(!lume_perf_tasks_read(&list, &fallback) && !list.count);
    failure = 1;
    assert(!lume_perf_tasks_read(&list, &fallback) && list.count == 1 && list.entries[0].pid == 77);
    failure = 3;
    assert(lume_perf_tasks_read(&list, &fallback) == -5);
    free(list.entries);
    puts("PERF_CLIENT_PASS pagination=73 empty legacy malformed");
    return 0;
}
