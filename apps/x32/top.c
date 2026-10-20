/* Native bounded Lume monitor, not an upstream htop port. */
#include "api.h"
#include "perf.h"

static struct lume_perf_snapshot samples[2];
static char screen[3072];
static unsigned used;

static void text(const char *s)
{
    while (*s && used < sizeof screen)
        screen[used++] = *s++;
}

static void number(uint64_t value, unsigned width)
{
    char digits[21];
    unsigned n = 0;
    do
    {
        digits[n++] = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    while (width > n)
    {
        text(" ");
        width--;
    }
    while (n && used < sizeof screen)
        screen[used++] = digits[--n];
}

static void fixed(uint64_t value, unsigned width)
{
    number(value / 10, width);
    text(".");
    number(value % 10, 0);
}

static uint32_t delta(uint32_t a, uint32_t b)
{
    return a - b;
}

static uint64_t millis(uint32_t ticks, uint32_t hz)
{
    return (uint64_t)ticks * 1000u / hz;
}

static void percent(uint32_t ticks, uint64_t total)
{
    if (total)
    {
        fixed((uint64_t)ticks * 1000u / total, 3);
        text("%");
    }
    else
        text("  n/a ");
}

static int write_screen(void)
{
    unsigned at = 0;
    while (at < used)
    {
        int n = app_write(1, screen + at, used - at);
        if (n == -4)
            continue;
        if (n <= 0 || (unsigned)n > used - at)
            return 0;
        at += (unsigned)n;
    }
    return 1;
}

static const char *state(uint32_t value)
{
    switch (value)
    {
    case LUME_READY:
        return "READY";
    case LUME_RUNNING:
        return "RUN";
    case LUME_WAITING:
        return "WAIT";
    case LUME_ZOMBIE:
        return "ZOMB";
    case LUME_SLEEPING:
        return "SLEEP";
    default:
        return "?";
    }
}

static int snapshot(struct lume_perf_snapshot *s)
{
    int n = lume_syscall(LUME_PERF_SYSCALL, (uint32_t)(uintptr_t)s, sizeof *s, LUME_PERF_VERSION);
    if (n < 0)
    {
        app_error("top: telemetry", n);
        return 0;
    }
    if (s->version != LUME_PERF_VERSION || s->bytes != sizeof *s || !s->clock_hz ||
        !s->core_count || s->core_count > LUME_PERF_CORES || s->process_count > LUME_PERF_PROCESSES)
    {
        app_puts("top: unsupported telemetry layout\n");
        return 0;
    }
    return 1;
}

static const struct lume_perf_process *previous_process(const struct lume_perf_snapshot *s,
                                                        uint32_t pid)
{
    for (unsigned i = 0; i < s->process_count; i++)
        if (s->processes[i].pid == pid)
            return &s->processes[i];
    return 0;
}

static void metric(const char *label, uint32_t after, uint32_t before)
{
    text(label);
    number(delta(after, before), 0);
}

static void cost(const char *label, uint32_t after, uint32_t before, uint32_t hz)
{
    text(label);
    number(millis(delta(after, before), hz), 0);
}

static int render(const struct lume_perf_snapshot *now, const struct lume_perf_snapshot *old,
                  int batch)
{
    uint32_t elapsed = delta(now->uptime_ms, old->uptime_ms);
    used = 0;
    if (!batch)
        text("\033[2J\033[H");
    text("Lume top  uptime ");
    number(now->uptime_ms / 1000u, 0);
    text("s  window ");
    number(elapsed, 0);
    text("ms  RR ");
    number(now->quantum_ms, 0);
    text("ms\n");
    /* A 32-bit hardware counter wraps in 143s at 30MHz. Suppress invalid
     * deltas after a long pause instead of publishing plausible percentages. */
    if (now->clock_hz != old->clock_hz || now->core_count != old->core_count || !elapsed ||
        (uint64_t)elapsed * now->clock_hz >= UINT64_C(4294967296) * 1000u)
    {
        text("Sampling interval invalid; counters resampled.\n");
        return write_screen();
    }
    for (unsigned c = 0; c < now->core_count; c++)
    {
        const struct lume_perf_core *a = &now->cores[c], *b = &old->cores[c];
        uint64_t total = 0;
        for (unsigned m = 0; m < LUME_PERF_MODES; m++)
            total += delta(a->ticks[m], b->ticks[m]);
        text("CPU");
        number(c, 0);
        text(" user ");
        percent(delta(a->ticks[LUME_PERF_USER], b->ticks[LUME_PERF_USER]), total);
        text(" kernel ");
        percent(delta(a->ticks[LUME_PERF_KERNEL], b->ticks[LUME_PERF_KERNEL]), total);
        text(" idle ");
        percent(delta(a->ticks[LUME_PERF_IDLE], b->ticks[LUME_PERF_IDLE]), total);
        text(" lock ");
        percent(delta(a->ticks[LUME_PERF_LOCKWAIT], b->ticks[LUME_PERF_LOCKWAIT]), total);
        metric(" calls ", a->syscalls, b->syscalls);
        metric(" irq ", a->interrupts, b->interrupts);
        metric(" switches ", a->switches, b->switches);
        text("\n");
    }
    text("  PID STATE MASK    CPU%   LOCK% READYms MAXms RUNS NAME\n");
    uint32_t wall_ticks = delta(now->cores[0].counter, old->cores[0].counter);
    for (unsigned i = 0; i < now->process_count; i++)
    {
        const struct lume_perf_process *a = &now->processes[i];
        const struct lume_perf_process *b = previous_process(old, a->pid);
        number(a->pid, 5);
        text(" ");
        text(state(a->state));
        for (unsigned n = app_length(state(a->state)); n < 5; n++)
            text(" ");
        text(" ");
        number(a->cores, 4);
        text(" ");
        if (b)
        {
            uint64_t cpu = (uint64_t)delta(a->user_ticks, b->user_ticks) +
                           delta(a->kernel_ticks, b->kernel_ticks);
            if (wall_ticks)
            {
                fixed(cpu * 1000u / wall_ticks, 3);
                text("%");
            }
            else
                text("  n/a ");
            text(" ");
            percent(delta(a->lock_ticks, b->lock_ticks), wall_ticks);
            text(" ");
            number(millis(delta(a->ready_ticks, b->ready_ticks), now->clock_hz), 7);
        }
        else
            text("   new     new     new");
        text(" ");
        number(millis(a->max_ready_ticks, now->clock_hz), 5);
        text(" ");
        number(a->dispatches, 4);
        text(" ");
        for (unsigned n = 0; n < sizeof a->name && a->name[n]; n++)
        {
            unsigned char ch = (unsigned char)a->name[n];
            char safe[2] = {(char)(ch >= 32 && ch <= 126 ? ch : '?'), 0};
            text(safe);
        }
        text("\n");
    }
    const struct lume_perf_totals *a = &now->totals, *b = &old->totals;
    text("Delta:");
    metric(" SDread ", a->sd_reads, b->sd_reads);
    metric(" SDwrite ", a->sd_writes, b->sd_writes);
    metric(" cache-hit ", a->cache_hits, b->cache_hits);
    metric(" miss ", a->cache_misses, b->cache_misses);
    metric(" ELF ", a->load_calls, b->load_calls);
    metric(" read-bytes ", a->read_bytes, b->read_bytes);
    metric(" zero-bytes ", a->zero_bytes, b->zero_bytes);
    text("\n");
    text("Cost ms:");
    cost(" SD ", a->sd_ticks, b->sd_ticks, now->clock_hz);
    cost(" ELF ", a->load_ticks, b->load_ticks, now->clock_hz);
    cost(" spawn ", a->spawn_ticks, b->spawn_ticks, now->clock_hz);
    cost(" read ", a->read_ticks, b->read_ticks, now->clock_hz);
    cost(" copy ", a->copy_ticks, b->copy_ticks, now->clock_hz);
    cost(" zero ", a->zero_ticks, b->zero_ticks, now->clock_hz);
    cost(" console ", a->console_ticks, b->console_ticks, now->clock_hz);
    metric(" bytes ", a->console_bytes, b->console_bytes);
    text("\n");
    text("CPU: one core=100%; READY delta, MAX session; top overhead included.\n");
    if (!batch)
        text("q quits; sampling sleeps until timer wakeup.\n");
    return write_screen();
}

static int positive(const char *s, unsigned max, unsigned *out)
{
    unsigned n = 0;
    if (!*s)
        return 0;
    for (; *s; s++)
    {
        if (*s < '0' || *s > '9' || n > max / 10u)
            return 0;
        n = n * 10u + (unsigned)(*s - '0');
        if (n > max)
            return 0;
    }
    if (!n)
        return 0;
    *out = n;
    return 1;
}

static void usage(void)
{
    app_puts("usage: top [-b] [-n 1..1000] [-d 1..60]\n"
             "  -b batch (default one sample); -n samples; -d seconds (default 1)\n"
             "       top -r /absolute/program.x32 [arguments...]\n"
             "       top --status (query accounting without starting it)\n");
}

static int run_command(int argc, char **argv, char **envp)
{
    if (argc < 3 || argv[2][0] != '/' || argc - 2 > (int)LUME_ARGS)
    {
        usage();
        return 2;
    }
    unsigned envc = 0;
    if (envp)
        while (envp[envc])
        {
            if (envc == LUME_ENVS)
            {
                app_puts("top: too many environment entries\n");
                return 2;
            }
            envc++;
        }
    /* Output starts after the child completes; its output and load are part
     * of the measurement, while rendering this report is outside it. */
    if (!snapshot(&samples[0]))
        return 1;
    int pid = app_spawn_env(argv[2], (unsigned)argc - 2, (const char *const *)(argv + 2), envc,
                            (const char *const *)envp);
    if (pid < 0)
    {
        app_error("top: spawn", pid);
        return 1;
    }
    struct lume_wait_status status;
    int waited = app_wait(pid, &status);
    if (waited < 0)
    {
        app_error("top: wait", waited);
        return 1;
    }
    if (waited != pid || status.pid != (unsigned)pid)
    {
        app_puts("top: inconsistent child status\n");
        return 1;
    }
    if (!snapshot(&samples[1]) || !render(&samples[1], &samples[0], 1))
        return 1;
    used = 0;
    text("Child PID ");
    number((unsigned)pid, 0);
    text(" status ");
    number(status.code, 0);
    text(" elapsed ");
    number(delta(samples[1].uptime_ms, samples[0].uptime_ms), 0);
    text("ms; load, wait and child output included; report excluded.\n");
    if (!write_screen())
        return 1;
    return (int)status.code;
}

int app_main(int argc, char **argv, char **envp)
{
    if (argc == 2 && app_equal(argv[1], "--status"))
    {
        int status = lume_syscall(LUME_INFO, LUME_PERF_STATUS, 0, 0);
        if (status == 0x100)
        {
            app_puts("Accounting: off (on demand)\n");
            return 0;
        }
        if (status == 0x101)
        {
            app_puts("Accounting: on\n");
            return 0;
        }
        app_puts("top: accounting status unsupported\n");
        return 1;
    }
    if (argc > 1 && app_equal(argv[1], "-r"))
        return run_command(argc, argv, envp);
    unsigned count = 0, delay = 1;
    int batch = 0;
    for (int i = 1; i < argc; i++)
    {
        if (app_equal(argv[i], "--help"))
        {
            usage();
            return 0;
        }
        if (app_equal(argv[i], "-b"))
            batch = 1;
        else if (app_equal(argv[i], "-n") || app_equal(argv[i], "-d"))
        {
            int interval = app_equal(argv[i], "-d");
            unsigned value;
            if (++i == argc || !positive(argv[i], interval ? 60u : 1000u, &value))
            {
                usage();
                return 2;
            }
            if (interval)
                delay = value;
            else
                count = value;
        }
        else
        {
            usage();
            return 2;
        }
    }
    if (batch && !count)
        count = 1;
    if (!snapshot(&samples[0]))
        return 1;
    unsigned previous = 0, displayed = 0;
    while (!count || displayed < count)
    {
        if (!batch)
        {
            unsigned char key;
            int n = app_read(0, &key, 1);
            if (n == 1 && (key == 'q' || key == 'Q' || key == 3 || key == 27))
                return 0;
            if (n < 0 && n != -11)
            {
                app_error("top: input", n);
                return 1;
            }
        }
        int n = lume_syscall(LUME_SLEEP, delay * 1000u, 0, 0);
        if (n < 0)
        {
            app_error("top: sleep", n);
            return 1;
        }
        unsigned next = previous ^ 1u;
        if (!snapshot(&samples[next]) || !render(&samples[next], &samples[previous], batch))
            return 1;
        previous = next;
        displayed++;
    }
    return 0;
}
