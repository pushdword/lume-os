#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "lume/app.h"
#include "lume/io.h"

static unsigned ticks, waits, boot_calls, selected, reads;
static int key = -1, process_id = 1, select_result;
static unsigned flags = LUME_BOOT_CAP_PARTITION, trial;
static const char *configuration = "timeout=5\n";

int32_t lume_syscall(uint32_t op, uint32_t a, uint32_t b, uint32_t c)
{
    (void)c;
    if (op == LUME_PID)
        return process_id;
    if (op == LUME_INFO)
    {
        assert(a == LUME_UPTIME);
        return (int32_t)ticks;
    }
    if (op == LUME_GETCHAR)
        return key;
    assert(op == LUME_TERMINAL && a == 5 && b <= 30000);
    ++waits;
    ticks += b;
    return 0;
}

int app_boot(unsigned op, struct lume_boot_request *request)
{
    assert(request->version == LUME_IO_VERSION);
    ++boot_calls;
    if (op == LUME_BOOT_INFO)
    {
        request->total = trial;
        return 0;
    }
    if (op == LUME_BOOT_CAPABILITIES)
    {
        request->total = flags;
        request->active = 3;
        return 0;
    }
    assert(op == LUME_BOOT_SELECT_PARTITION && request->active == 3 && !request->total);
    assert(!request->count && !request->offset && !request->crc);
    ++selected;
    return select_result;
}

int app_open(const char *path, unsigned mode)
{
    assert(!strcmp(path, "/etc/bootmenu.conf") && mode == LUME_RDONLY);
    return 7;
}

int app_read(int fd, void *out, unsigned bytes)
{
    assert(fd == 7);
    ++reads;
    unsigned count = (unsigned)strlen(configuration);
    if (count > bytes)
        count = bytes;
    memcpy(out, configuration, count);
    return (int)count;
}

int app_close(int fd)
{
    assert(fd == 7);
    return 0;
}

void app_puts(const char *text)
{
    assert(text);
}

void app_error(const char *name, int error)
{
    assert(!strcmp(name, "boot") && error < 0);
}

#include "../../apps/shell/boot_menu.h"

int main(void)
{
    assert(!shell_boot_menu() && waits == 1 && ticks == 5000 && !selected);
    key = '1';
    assert(!shell_boot_menu() && !selected);
    key = '2';
    assert(shell_boot_menu() == 1 && selected == 1);
    select_result = -16;
    assert(!shell_boot_menu() && selected == 2);
    select_result = 0;
    unsigned before = boot_calls;
    process_id = 2;
    assert(!shell_boot_menu() && boot_calls == before);
    assert(!shell_boot_linux() && boot_calls == before);
    process_id = 1;
    trial = LUME_BOOT_STATE_TRIAL;
    before = reads;
    assert(!shell_boot_menu() && reads == before);
    trial = 0;
    flags = 0;
    assert(!shell_boot_menu() && reads == before);
    flags = LUME_BOOT_CAP_PARTITION;
    const char *invalid[] = {"timeout=0\n", "timeout=31\n", "timeout=5x\n", "timeout=x\n",
                             "bad=5\n"};
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
    {
        configuration = invalid[i];
        assert(!shell_boot_menu());
    }
    configuration = "timeout=5\n";
    key = -1;
    ticks = UINT32_MAX - 50;
    before = waits;
    assert(!shell_boot_menu() && waits == before + 1);
    puts(
        "PASS boot menu default, Linux selection, capability, trial, console ownership and timeout");
    return 0;
}
