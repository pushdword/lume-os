/* SPDX-License-Identifier: BSD-3-Clause */
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <wayland-server-core.h>

struct context
{
    struct wl_event_loop *loop;
    struct wl_event_source *remove;
    unsigned reads, writes, timers, signals, idles, checks;
};

#define CHECK(value)                                                                               \
    do                                                                                             \
    {                                                                                              \
        if (!(value))                                                                              \
        {                                                                                          \
            fprintf(stderr, "WAYLAND_LOOP_FAIL line=%d errno=%d\n", __LINE__, errno);              \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

static int fd_ready(int fd, uint32_t mask, void *data)
{
    struct context *context = data;
    if (mask & WL_EVENT_READABLE)
    {
        char byte;
        CHECK(read(fd, &byte, 1) == 1);
        ++context->reads;
    }
    if (mask & WL_EVENT_WRITABLE)
        ++context->writes;
    if (!mask)
        ++context->checks;
    if (context->remove)
    {
        wl_event_source_remove(context->remove);
        context->remove = NULL;
    }
    return 0;
}

static int timer_ready(void *data)
{
    struct context *context = data;
    ++context->timers;
    if (context->remove)
    {
        wl_event_source_remove(context->remove);
        context->remove = NULL;
    }
    return 0;
}

static void idle_ready(void *data)
{
    ++((struct context *)data)->idles;
}

static int signal_ready(int signal, void *data)
{
    CHECK(signal == SIGUSR1);
    ++((struct context *)data)->signals;
    return 0;
}

static uint64_t now(void)
{
    struct timespec ts;
    CHECK(!clock_gettime(CLOCK_MONOTONIC, &ts));
    return (uint64_t)ts.tv_sec * 1000000000u + ts.tv_nsec;
}

int main(int argc, char **argv)
{
    struct context context = {.loop = wl_event_loop_create()};
    CHECK(context.loop);
    int descriptors[2];
    CHECK(!pipe(descriptors));
    struct wl_event_source *source =
        wl_event_loop_add_fd(context.loop, descriptors[1], 0, fd_ready, &context);
    CHECK(source);
    CHECK(!wl_event_loop_dispatch(context.loop, 0));
    CHECK(!context.writes);
    CHECK(!wl_event_source_fd_update(source, WL_EVENT_WRITABLE));
    CHECK(!wl_event_loop_dispatch(context.loop, 0));
    CHECK(context.writes == 1);
    CHECK(!wl_event_source_remove(source));
    source =
        wl_event_loop_add_fd(context.loop, descriptors[0], WL_EVENT_READABLE, fd_ready, &context);
    CHECK(source);
    CHECK(write(descriptors[1], "x", 1) == 1);
    CHECK(!wl_event_loop_dispatch(context.loop, 100));
    CHECK(context.reads == 1);
    wl_event_source_check(source);
    CHECK(!wl_event_loop_dispatch(context.loop, 0));
    CHECK(context.checks == 1);
    CHECK(!wl_event_source_remove(source));
    struct wl_event_source *timer = wl_event_loop_add_timer(context.loop, timer_ready, &context);
    CHECK(timer);
    CHECK(!wl_event_source_timer_update(timer, 2));
    CHECK(!wl_event_loop_dispatch(context.loop, 100));
    CHECK(context.timers == 1);
    CHECK(!wl_event_source_timer_update(timer, 1));
    CHECK(!wl_event_source_timer_update(timer, 0));
    CHECK(!wl_event_loop_dispatch(context.loop, 3));
    CHECK(context.timers == 1);
    CHECK(wl_event_loop_add_idle(context.loop, idle_ready, &context));
    CHECK(!wl_event_loop_dispatch(context.loop, 0));
    CHECK(context.idles == 1);
    CHECK(!wl_event_source_timer_update(timer, 1));
    context.remove = wl_event_loop_add_timer(context.loop, timer_ready, &context);
    CHECK(context.remove);
    CHECK(!wl_event_source_timer_update(context.remove, 1));
    CHECK(!wl_event_loop_dispatch(context.loop, 100));
    CHECK(context.timers == 2 && !context.remove);

    struct sigaction previous;
    CHECK(!sigaction(SIGUSR1, NULL, &previous));
    sigset_t blocked, old;
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGUSR1);
    CHECK(!sigprocmask(SIG_UNBLOCK, &blocked, &old));
    struct wl_event_source *signal =
        wl_event_loop_add_signal(context.loop, SIGUSR1, signal_ready, &context);
    CHECK(signal);
    CHECK(!kill(getpid(), SIGUSR1));
    CHECK(!wl_event_loop_dispatch(context.loop, 100));
    CHECK(context.signals == 1);
    CHECK(!wl_event_source_remove(signal));
    struct sigaction restored;
    CHECK(!sigaction(SIGUSR1, NULL, &restored));
    CHECK(previous.sa_handler == restored.sa_handler);
    CHECK(!sigprocmask(SIG_SETMASK, &old, NULL));

    source =
        wl_event_loop_add_fd(context.loop, descriptors[0], WL_EVENT_READABLE, fd_ready, &context);
    CHECK(source);
    int aggregate = wl_event_loop_get_fd(context.loop);
    CHECK(aggregate >= 0);
    CHECK(write(descriptors[1], "y", 1) == 1);
    struct pollfd readiness = {.fd = aggregate, .events = POLLIN};
    CHECK(poll(&readiness, 1, 500) == 1);
    CHECK(!wl_event_loop_dispatch(context.loop, 0));
    CHECK(context.reads == 2);
    CHECK(!wl_event_source_timer_update(timer, 5));
    CHECK(poll(&readiness, 1, 500) == 1);
    CHECK(!wl_event_loop_dispatch(context.loop, 0));
    if (context.timers == 2)
    {
        CHECK(poll(&readiness, 1, 500) == 1);
        CHECK(!wl_event_loop_dispatch(context.loop, 0));
    }
    CHECK(context.timers == 3);
    wl_event_loop_destroy(context.loop);
    close(descriptors[0]);
    close(descriptors[1]);

    if (argc == 2 && !strcmp(argv[1], "--bench"))
    {
        context.loop = wl_event_loop_create();
        CHECK(context.loop);
        uint64_t started = now();
        for (unsigned i = 0; i < 100000; ++i)
            CHECK(!wl_event_loop_dispatch(context.loop, 0));
        uint64_t idle = now() - started;
        timer = wl_event_loop_add_timer(context.loop, timer_ready, &context);
        started = now();
        for (unsigned i = 0; i < 100000; ++i)
            CHECK(!wl_event_source_timer_update(timer, 1000));
        uint64_t updates = now() - started;
        printf("WAYLAND_LOOP_BENCH iterations=100000 idle_ns=%llu timer_update_ns=%llu\n",
               (unsigned long long)idle, (unsigned long long)updates);
        wl_event_loop_destroy(context.loop);
    }
    puts("WAYLAND_LOOP_PASS");
    return 0;
}
