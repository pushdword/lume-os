#ifndef LUME_PERF_CLIENT_H
#define LUME_PERF_CLIENT_H
#include "api.h"
#include "perf.h"
#include <stdlib.h>
#include <string.h>

struct lume_perf_task_list
{
    struct lume_perf_process *entries;
    unsigned count, capacity;
};

static int lume_perf_process_compare(const void *left, const void *right)
{
    uint32_t a = ((const struct lume_perf_process *)left)->pid;
    uint32_t b = ((const struct lume_perf_process *)right)->pid;
    return a > b ? 1 : a < b ? -1 : 0;
}

static const struct lume_perf_process *
lume_perf_process_find(const struct lume_perf_task_list *list, uint32_t pid)
{
    const struct lume_perf_process key = {.pid = pid};
    return list->count
               ? bsearch(&key, list->entries, list->count, sizeof(key), lume_perf_process_compare)
               : NULL;
}

static int lume_perf_tasks_read(struct lume_perf_task_list *list,
                                const struct lume_perf_snapshot *fallback)
{
    static struct lume_perf_task_page page;
    page = (struct lume_perf_task_page){0};
    list->count = 0;
    do
    {
        unsigned cursor = page.cursor;
        int result = lume_syscall(LUME_PERF_SYSCALL, (uint32_t)(uintptr_t)&page, sizeof(page),
                                  LUME_PERF_TASK_PAGE);
        if (result == -22 && !cursor)
        {
            page.count = fallback->process_count;
            page.next = 0;
            page.slots = LUME_PERF_PROCESSES;
            memcpy(page.processes, fallback->processes, page.count * sizeof(page.processes[0]));
        }
        else if (result)
            return result;
        if (page.count > LUME_PERF_PAGE_PROCESSES || page.slots > 65536 ||
            page.count > page.slots || list->count > page.slots - page.count ||
            (page.next && (page.next <= cursor || page.next > page.slots)))
            return -5;
        unsigned count = list->count + page.count;
        if (count > list->capacity)
        {
            unsigned capacity =
                (count + LUME_PERF_PAGE_PROCESSES - 1) & ~(LUME_PERF_PAGE_PROCESSES - 1);
            void *entries = realloc(list->entries, capacity * sizeof(page.processes[0]));
            if (!entries)
                return -12;
            list->entries = entries;
            list->capacity = capacity;
        }
        if (page.count)
            memcpy(list->entries + list->count, page.processes,
                   page.count * sizeof(page.processes[0]));
        list->count = count;
        page.cursor = page.next;
    } while (page.next);
    for (unsigned i = 0; i < list->count; ++i)
        list->entries[i].name[sizeof(list->entries[i].name) - 1] = 0;
    if (list->count)
        qsort(list->entries, list->count, sizeof(list->entries[0]), lume_perf_process_compare);
    return 0;
}
#endif
