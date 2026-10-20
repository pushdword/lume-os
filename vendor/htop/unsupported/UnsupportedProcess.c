/*
htop - UnsupportedProcess.c
(C) 2015 Hisham H. Muhammad
Released under the GNU GPLv2+, see the COPYING file
in the source distribution for its full text.
*/

#include "config.h" // IWYU pragma: keep

#include "unsupported/UnsupportedProcess.h"

#include <stdlib.h>

#include "CRT.h"
#include "Process.h"

const ProcessFieldData Process_fields[LAST_PROCESSFIELD] = {
    [0] =
        {
            .name = "",
            .title = NULL,
            .description = NULL,
            .flags = 0,
        },
    [PID] =
        {
            .name = "PID",
            .title = "PID",
            .description = "Process/thread ID",
            .flags = 0,
            .pidColumn = true,
        },
    [COMM] =
        {
            .name = "Command",
            .title = "Command ",
            .description = "Command line (insert as last column only)",
            .flags = 0,
        },
    [STATE] =
        {
            .name = "STATE",
            .title = "S ",
            .description =
                "Process state (S sleeping, R running, D disk, Z zombie, T traced, W paging)",
            .flags = 0,
        },
    [PPID] =
        {
            .name = "PPID",
            .title = "PPID",
            .description = "Parent process ID",
            .flags = 0,
            .pidColumn = true,
        },
    [ST_UID] =
        {
            .name = "ST_UID",
            .title = "UID",
            .description = "User ID of the process owner",
            .flags = 0,
        },
    [PERCENT_CPU] =
        {
            .name = "PERCENT_CPU",
            .title = " CPU%",
            .description = "Percentage of the CPU time the process used in the last sampling",
            .flags = 0,
            .defaultSortDesc = true,
            .autoWidth = true,
            .autoTitleRightAlign = true,
        },
    [USER] =
        {
            .name = "USER",
            .title = "USER       ",
            .description =
                "Username of the process owner (or user ID if name cannot be determined)",
            .flags = 0,
        },
    [TIME] =
        {
            .name = "TIME",
            .title = "  TIME+  ",
            .description = "Total time the process has spent in user and system time",
            .flags = 0,
            .defaultSortDesc = true,
        },
    [NLWP] =
        {
            .name = "NLWP",
            .title = "NLWP ",
            .description = "Number of threads in the process",
            .flags = 0,
        },
    [TGID] =
        {
            .name = "TGID",
            .title = "TGID",
            .description = "Thread group ID (i.e. process ID)",
            .flags = 0,
            .pidColumn = true,
        },
};

Process *UnsupportedProcess_new(const Machine *host)
{
    Process *this = xCalloc(1, sizeof(UnsupportedProcess));
    Object_setClass(this, Class(UnsupportedProcess));
    Process_init(this, host);
    return this;
}

void Process_delete(Object *cast)
{
    Process *super = (Process *)cast;
    Process_done(super);
    // free platform-specific fields here
    free(cast);
}

static void UnsupportedProcess_rowWriteField(const Row *super, RichString *str, ProcessField field)
{
    const UnsupportedProcess *up = (const UnsupportedProcess *)super;

    bool coloring = super->host->settings->highlightMegabytes;
    char buffer[256];
    buffer[255] = '\0';
    int attr = CRT_colors[DEFAULT_COLOR];
    size_t n = sizeof(buffer) - 1;

    (void)coloring;
    (void)n;

    switch (field)
    {
    /* Add platform specific fields */
    default:
        Process_writeField(&up->super, str, field);
        return;
    }

    RichString_appendWide(str, attr, buffer);
}

static int UnsupportedProcess_compareByKey(const Process *v1, const Process *v2, ProcessField key)
{
    const UnsupportedProcess *p1 = (const UnsupportedProcess *)v1;
    const UnsupportedProcess *p2 = (const UnsupportedProcess *)v2;

    (void)p1;
    (void)p2;

    switch (key)
    {
    /* Add platform specific fields */
    default:
        return Process_compareByKey_Base(v1, v2, key);
    }
}

const ProcessClass UnsupportedProcess_class = {
    .super = {.super = {.extends = Class(Process),
                        .display = Row_display,
                        .delete = Process_delete,
                        .compare = Process_compare},
              .isHighlighted = Process_rowIsHighlighted,
              .isVisible = Process_rowIsVisible,
              .matchesFilter = Process_rowMatchesFilter,
              .compareByParent = Process_compareByParent,
              .sortKeyString = Process_rowGetSortKey,
              .writeField = UnsupportedProcess_rowWriteField},
    .compareByKey = UnsupportedProcess_compareByKey};
