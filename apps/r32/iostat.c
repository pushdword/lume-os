/* Kernel I/O diagnostics; independent of PERF/top observation sessions. */
#include "api.h"
#include "io.h"
#include <stdint.h>

static void value(const char *name, uint32_t v)
{
    app_puts(name);
    app_number(v);
}

int app_main(unsigned argc, char **argv)
{
    if (argc > 1)
    {
        unsigned enable = app_equal(argv[1], "on");
        if (argc != 2 || (!enable && !app_equal(argv[1], "off")))
        {
            app_puts("usage: iostat [on|off]\n");
            return 2;
        }
        int r = lume_syscall(LUME_IO, 1, enable, 0);
        if (r < 0)
        {
            app_puts("iostat: kernel control unavailable\n");
            return 1;
        }
    }
    struct lume_io_snapshot s;
    int r = lume_syscall(LUME_IO, 0, (uint32_t)(uintptr_t)&s, sizeof s);
    if (r < 0 || s.version != LUME_IO_VERSION || s.bytes != sizeof s)
    {
        app_puts("iostat: kernel snapshot unavailable\n");
        return 1;
    }
    struct lume_io_services services;
    int sampled = lume_syscall(LUME_IO, 2, (uint32_t)(uintptr_t)&services, sizeof services);
    value("IOSTAT enabled=", s.enabled);
    value(" clock=", s.clock_hz);
    value(" ms=", s.uptime_ms);
    app_puts("\n");
    for (unsigned core = 0; core < 2; core++)
    {
        const struct lume_io_lock *m = &s.lock[core];
        value("LOCK core=", core);
        value(" acquire=", m->acquisitions);
        value(" contended=", m->contended);
        value(" CAS=", m->cas_attempts);
        value(" wait_ticks=", m->wait_ticks);
        value(" max_wait=", m->max_wait_ticks);
        value(" hold_ticks=", m->hold_ticks);
        value(" max_hold=", m->max_hold_ticks);
        app_puts("\n");
    }
    value("UART active=", s.uart.active);
    value(" logical=", s.uart.logical_bytes);
    value(" fifo=", s.uart.fifo_bytes);
    value(" requests=", s.uart.requests);
    value(" high=", s.uart.high_water);
    value(" pressure=", s.uart.backpressure);
    value(" services=", s.uart.service_calls);
    value(" ticks=", s.uart.service_ticks);
    value(" max_service=", s.uart.max_service_ticks);
    value(" overrun=", s.uart.overrun);
    value(" framing=", s.uart.framing);
    value(" stalls=", s.uart.stalls);
    app_puts("\n");
    value("DIR opens=", s.directory_opens);
    value(" reads=", s.directory_reads);
    value(" replay=", s.directory_replays);
    value(" invalid=", s.directory_invalidations);
    app_puts("\n");
    if (!sampled && services.version == 1 && services.bytes == sizeof services)
        for (unsigned core = 0; core < 2; core++)
        {
            value("SERVICE core=", core);
            value(" irq=", services.irq[core]);
            value(" timer=", services.timer[core]);
            value(" lock=", services.lock_wait[core]);
            value(" checkpoint=", services.checkpoint[core]);
            app_puts("\n");
        }
    return 0;
}
