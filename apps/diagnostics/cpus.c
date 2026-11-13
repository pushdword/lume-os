#include "lume/app_support.h"
#include <cpu.h>

static struct lume_cpu_snapshot snapshot;

static void number(uint64_t value, unsigned base)
{
    static const char digits[] = "0123456789abcdef";
    char text[32];
    unsigned at = sizeof(text);
    do
    {
        text[--at] = digits[value % base];
        value /= base;
    } while (value);
    (void)write_all(1, text + at, sizeof(text) - at);
}

int main(void)
{
    if (call(LUME_SYS_CPU, LUME_CPU_SNAPSHOT, (uintptr_t)&snapshot, sizeof(snapshot)))
        return 1;
    if (snapshot.version != LUME_CPU_SNAPSHOT_VERSION || snapshot.bytes != sizeof(snapshot) ||
        snapshot.count > LUME_CPU_SNAPSHOT_CORES)
        return 1;
    puts("CPUS counter=");
    number(snapshot.counter, 10);
    puts(" frequency=");
    number(snapshot.frequency, 10);
    puts("\n");
    for (unsigned c = 0; c < snapshot.count; ++c)
    {
        const struct lume_cpu_state *state = snapshot.cores + c;
        puts("CPU");
        number(c, 10);
        puts(" flags=");
        number(state->flags, 10);
        puts(" task=");
        number(state->task, 10);
        puts(" calls=");
        number(state->calls, 10);
        puts(" irq=");
        number(state->interrupts, 10);
        puts(" pc=0x");
        number(state->pc, 16);
        puts(" status=0x");
        number(state->status, 16);
        puts(" trap=");
        number(state->trap_at, 10);
        puts(" wait=");
        number(state->wait_since, 10);
        puts("\n");
    }
    return 0;
}
