/* SPDX-License-Identifier: BSD-3-Clause */
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "wayland-server-core.h"
#include "wayland-server-private.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define SOURCE_LIMIT 254u
#define SIGNAL_LOOPS 32u
#define SLOT_ACTIVE 1u
#define SLOT_READER 2u

enum source_kind
{
    SOURCE_CURSOR,
    SOURCE_FD,
    SOURCE_TIMER,
    SOURCE_SIGNAL,
    SOURCE_IDLE
};

struct wl_event_source
{
    struct wl_list link, retired;
    struct wl_event_loop *loop;
    enum source_kind kind;

    union
    {
        wl_event_loop_fd_func_t fd;
        wl_event_loop_timer_func_t timer;
        wl_event_loop_signal_func_t signal;
        wl_event_loop_idle_func_t idle;
    } callback;

    void *data;
    uint64_t deadline;
    int fd, original, number;
    uint32_t mask;
    bool removed, checked;
};

struct wl_event_loop
{
    struct wl_list sources, retired;
    struct wl_priv_signal destroy;
    struct pollfd polls[SOURCE_LIMIT + 1];
    struct wl_event_source *owners[SOURCE_LIMIT + 1];
    unsigned poll_count, fd_count, dispatching;
    int wake[2], notification[2], signal_slot;
    bool dirty, monitor_started;
    pthread_t monitor;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    struct pollfd observed[SOURCE_LIMIT + 1];
    unsigned observed_count;
    uint64_t observed_deadline;
    bool stopping, pending;
};

static struct signal_slot
{
    unsigned state, mask, pending;
    int fd;
} signal_slots[SIGNAL_LOOPS];

static pthread_mutex_t signal_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct sigaction previous_actions[NSIG];
static unsigned signal_users[NSIG];

static uint64_t milliseconds(void)
{
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now))
        return 0;
    return (uint64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

static int time_left(uint64_t deadline)
{
    if (deadline == UINT64_MAX)
        return -1;
    uint64_t now = milliseconds();
    if (now >= deadline)
        return 0;
    uint64_t delay = deadline - now;
    return delay > INT_MAX ? INT_MAX : (int)delay;
}

static int make_pipe(int descriptors[2])
{
    if (pipe(descriptors))
        return -1;
    for (unsigned i = 0; i < 2; ++i)
    {
        int flags = fcntl(descriptors[i], F_GETFL);
        if (flags < 0 || fcntl(descriptors[i], F_SETFL, flags | O_NONBLOCK) < 0 ||
            fcntl(descriptors[i], F_SETFD, FD_CLOEXEC) < 0)
        {
            int saved = errno;
            close(descriptors[0]);
            close(descriptors[1]);
            errno = saved;
            return -1;
        }
    }
    return 0;
}

static void poke(int fd)
{
    int saved = errno;
    unsigned char byte = 1;
    ssize_t result;
    do
        result = write(fd, &byte, 1);
    while (result < 0 && errno == EINTR);
    errno = saved;
}

static void drain(int fd)
{
    unsigned char bytes[64];
    int saved = errno;
    while (read(fd, bytes, sizeof(bytes)) > 0 || errno == EINTR)
        errno = 0;
    errno = saved;
}

static void changed(struct wl_event_loop *loop)
{
    if (loop->monitor_started)
        poke(loop->wake[1]);
}

static uint64_t next_deadline(struct wl_event_loop *loop)
{
    uint64_t deadline = UINT64_MAX;
    struct wl_event_source *source;
    wl_list_for_each(source, &loop->sources, link)
    {
        if (source->kind == SOURCE_IDLE)
            return 0;
        if (source->kind == SOURCE_TIMER && source->deadline < deadline)
            deadline = source->deadline;
    }
    return deadline;
}

static void prepare(struct wl_event_loop *loop)
{
    if (!loop->dirty)
        return;
    loop->poll_count = 1;
    loop->polls[0] = (struct pollfd){.fd = loop->wake[0], .events = POLLIN};
    loop->owners[0] = NULL;
    struct wl_event_source *source;
    wl_list_for_each(source, &loop->sources, link)
    {
        if (source->kind != SOURCE_FD)
            continue;
        unsigned at = loop->poll_count++;
        loop->polls[at] =
            (struct pollfd){.fd = source->fd,
                            .events = (source->mask & WL_EVENT_READABLE ? POLLIN : 0) |
                                      (source->mask & WL_EVENT_WRITABLE ? POLLOUT : 0)};
        loop->owners[at] = source;
    }
    loop->dirty = false;
}

static void retire(struct wl_event_loop *loop)
{
    struct wl_event_source *source, *next;
    wl_list_for_each_safe(source, next, &loop->retired, retired)
    {
        wl_list_remove(&source->retired);
        free(source);
    }
}

static void signal_handler(int number)
{
    int saved = errno;
    unsigned bit = 1u << number;
    for (unsigned i = 0; i < SIGNAL_LOOPS; ++i)
    {
        struct signal_slot *slot = &signal_slots[i];
        unsigned state = __atomic_load_n(&slot->state, __ATOMIC_ACQUIRE);
        while ((state & SLOT_ACTIVE) && (__atomic_load_n(&slot->mask, __ATOMIC_ACQUIRE) & bit))
        {
            if (!__atomic_compare_exchange_n(&slot->state, &state, state + SLOT_READER, false,
                                             __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
                continue;
            if (!(__atomic_fetch_or(&slot->pending, bit, __ATOMIC_RELEASE) & bit))
                poke(slot->fd);
            __atomic_fetch_sub(&slot->state, SLOT_READER, __ATOMIC_RELEASE);
            break;
        }
    }
    errno = saved;
}

static struct wl_event_source *allocate(struct wl_event_loop *loop, enum source_kind kind,
                                        void *data)
{
    struct wl_event_source *source = calloc(1, sizeof(*source));
    if (!source)
        return NULL;
    source->loop = loop;
    source->kind = kind;
    source->data = data;
    source->fd = -1;
    source->deadline = UINT64_MAX;
    wl_list_insert(loop->sources.prev, &source->link);
    if (kind == SOURCE_FD)
        loop->dirty = true;
    changed(loop);
    return source;
}

WL_EXPORT struct wl_event_source *wl_event_loop_add_fd(struct wl_event_loop *loop, int fd,
                                                       uint32_t mask,
                                                       wl_event_loop_fd_func_t callback, void *data)
{
    if (loop->fd_count == SOURCE_LIMIT)
    {
        errno = EMFILE;
        return NULL;
    }
    int duplicate = fcntl(fd, F_DUPFD_CLOEXEC, 0);
    if (duplicate < 0)
        return NULL;
    struct wl_event_source *source = allocate(loop, SOURCE_FD, data);
    if (!source)
    {
        close(duplicate);
        return NULL;
    }
    source->fd = duplicate;
    source->original = fd;
    source->mask = mask;
    source->loop->dirty = true;
    source->callback.fd = callback;
    ++loop->fd_count;
    return source;
}

WL_EXPORT int wl_event_source_fd_update(struct wl_event_source *source, uint32_t mask)
{
    if (source->removed || source->kind != SOURCE_FD)
    {
        errno = EINVAL;
        return -1;
    }
    source->mask = mask;
    source->loop->dirty = true;
    changed(source->loop);
    return 0;
}

WL_EXPORT struct wl_event_source *
wl_event_loop_add_timer(struct wl_event_loop *loop, wl_event_loop_timer_func_t callback, void *data)
{
    struct wl_event_source *source = allocate(loop, SOURCE_TIMER, data);
    if (source)
        source->callback.timer = callback;
    return source;
}

WL_EXPORT int wl_event_source_timer_update(struct wl_event_source *source, int delay)
{
    if (source->removed || source->kind != SOURCE_TIMER || delay < 0)
    {
        errno = EINVAL;
        return -1;
    }
    source->deadline = delay ? milliseconds() + (unsigned)delay : UINT64_MAX;
    changed(source->loop);
    return 0;
}

WL_EXPORT struct wl_event_source *
wl_event_loop_add_idle(struct wl_event_loop *loop, wl_event_loop_idle_func_t callback, void *data)
{
    struct wl_event_source *source = allocate(loop, SOURCE_IDLE, data);
    if (source)
        source->callback.idle = callback;
    return source;
}

WL_EXPORT struct wl_event_source *wl_event_loop_add_signal(struct wl_event_loop *loop, int number,
                                                           wl_event_loop_signal_func_t callback,
                                                           void *data)
{
    if (number <= 0 || number >= NSIG || number >= 32 || number == SIGKILL || number == SIGSTOP)
    {
        errno = EINVAL;
        return NULL;
    }
    struct wl_event_source *source = allocate(loop, SOURCE_SIGNAL, data);
    if (!source)
        return NULL;
    pthread_mutex_lock(&signal_mutex);
    if (loop->signal_slot < 0)
        for (unsigned i = 0; i < SIGNAL_LOOPS; ++i)
            if (!__atomic_load_n(&signal_slots[i].state, __ATOMIC_ACQUIRE))
            {
                signal_slots[i].fd = loop->wake[1];
                signal_slots[i].mask = signal_slots[i].pending = 0;
                __atomic_store_n(&signal_slots[i].state, SLOT_ACTIVE, __ATOMIC_RELEASE);
                loop->signal_slot = (int)i;
                break;
            }
    int error = loop->signal_slot < 0 ? EMFILE : 0;
    if (!error && !signal_users[number])
    {
        struct sigaction action = {.sa_handler = signal_handler, .sa_flags = SA_RESTART};
        sigfillset(&action.sa_mask);
        if (sigaction(number, &action, &previous_actions[number]))
            error = errno;
    }
    if (!error)
    {
        source->callback.signal = callback;
        source->number = number;
        ++signal_users[number];
        __atomic_fetch_or(&signal_slots[loop->signal_slot].mask, 1u << number, __ATOMIC_RELEASE);
    }
    pthread_mutex_unlock(&signal_mutex);
    if (error)
    {
        wl_list_remove(&source->link);
        free(source);
        errno = error;
        return NULL;
    }
    return source;
}

WL_EXPORT void wl_event_source_check(struct wl_event_source *source)
{
    source->checked = true;
}

WL_EXPORT int wl_event_source_remove(struct wl_event_source *source)
{
    if (source->removed)
        return 0;
    struct wl_event_loop *loop = source->loop;
    source->removed = true;
    wl_list_remove(&source->link);
    if (source->kind == SOURCE_FD)
    {
        close(source->fd);
        --loop->fd_count;
        loop->dirty = true;
    }
    else if (source->kind == SOURCE_SIGNAL)
    {
        pthread_mutex_lock(&signal_mutex);
        bool remaining = false;
        struct wl_event_source *other;
        wl_list_for_each(other, &loop->sources, link) remaining |=
            other->kind == SOURCE_SIGNAL && other->number == source->number;
        if (!remaining)
            __atomic_fetch_and(&signal_slots[loop->signal_slot].mask, ~(1u << source->number),
                               __ATOMIC_RELEASE);
        if (!--signal_users[source->number])
            sigaction(source->number, &previous_actions[source->number], NULL);
        pthread_mutex_unlock(&signal_mutex);
    }
    changed(loop);
    if (loop->dispatching)
        wl_list_insert(loop->retired.prev, &source->retired);
    else
        free(source);
    return 0;
}

static struct wl_event_source *advance(struct wl_event_loop *loop, struct wl_event_source *cursor)
{
    if (cursor->link.next == &loop->sources)
        return NULL;
    struct wl_event_source *source = wl_container_of(cursor->link.next, source, link);
    wl_list_remove(&cursor->link);
    wl_list_insert(&source->link, &cursor->link);
    return source;
}

WL_EXPORT void wl_event_loop_dispatch_idle(struct wl_event_loop *loop)
{
    ++loop->dispatching;
    bool again;
    do
    {
        again = false;
        struct wl_event_source *source;
        struct wl_event_source cursor = {.kind = SOURCE_CURSOR};
        wl_list_insert(&loop->sources, &cursor.link);
        while ((source = advance(loop, &cursor)))
            if (!source->removed && source->kind == SOURCE_IDLE)
            {
                wl_event_loop_idle_func_t callback = source->callback.idle;
                void *data = source->data;
                wl_event_source_remove(source);
                callback(data);
                again = true;
            }
        wl_list_remove(&cursor.link);
    } while (again);
    if (!--loop->dispatching)
        retire(loop);
}

static int dispatch(struct wl_event_source *source, uint32_t mask)
{
    if (source->kind == SOURCE_FD)
        return source->callback.fd(source->original, mask, source->data);
    if (source->kind == SOURCE_SIGNAL)
        return source->callback.signal(source->number, source->data);
    if (source->kind == SOURCE_TIMER)
        return source->callback.timer(source->data);
    return 0;
}

static void observe(struct wl_event_loop *loop)
{
    loop->observed_count = 1;
    loop->observed[0] = (struct pollfd){.fd = loop->wake[0], .events = POLLIN};
    struct wl_event_source *source;
    wl_list_for_each(source, &loop->sources, link) if (source->kind == SOURCE_FD)
        loop->observed[loop->observed_count++] =
        (struct pollfd){.fd = source->fd,
                        .events = (source->mask & WL_EVENT_READABLE ? POLLIN : 0) |
                                  (source->mask & WL_EVENT_WRITABLE ? POLLOUT : 0)};
    loop->observed_deadline = next_deadline(loop);
}

static void update_monitor(struct wl_event_loop *loop)
{
    if (!loop->monitor_started || loop->dispatching)
        return;
    drain(loop->notification[0]);
    pthread_mutex_lock(&loop->mutex);
    observe(loop);
    loop->pending = false;
    pthread_cond_signal(&loop->condition);
    pthread_mutex_unlock(&loop->mutex);
}

WL_EXPORT int wl_event_loop_dispatch(struct wl_event_loop *loop, int timeout)
{
    if (loop->dispatching)
    {
        errno = EBUSY;
        return -1;
    }
    ++loop->dispatching;
    wl_event_loop_dispatch_idle(loop);
    prepare(loop);
    unsigned pending =
        loop->signal_slot < 0
            ? 0
            : __atomic_exchange_n(&signal_slots[loop->signal_slot].pending, 0, __ATOMIC_ACQ_REL);
    int timer_timeout = time_left(next_deadline(loop));
    if (pending)
        timeout = 0;
    else if (timer_timeout >= 0 && (timeout < 0 || timer_timeout < timeout))
        timeout = timer_timeout;
    int result = poll(loop->polls, loop->poll_count, timeout);
    if (result < 0 && errno != EINTR)
        goto finished;
    result = 0;
    if (loop->polls[0].revents)
    {
        drain(loop->wake[0]);
        if (loop->signal_slot >= 0)
            pending |=
                __atomic_exchange_n(&signal_slots[loop->signal_slot].pending, 0, __ATOMIC_ACQ_REL);
    }
    unsigned count = loop->poll_count;
    for (unsigned i = 1; i < count; ++i)
    {
        struct wl_event_source *source = loop->owners[i];
        unsigned events = loop->polls[i].revents;
        if (!events || source->removed)
            continue;
        uint32_t mask = (events & POLLIN ? WL_EVENT_READABLE : 0) |
                        (events & POLLOUT ? WL_EVENT_WRITABLE : 0) |
                        (events & (POLLERR | POLLNVAL) ? WL_EVENT_ERROR : 0) |
                        (events & POLLHUP ? WL_EVENT_HANGUP : 0);
        dispatch(source, mask);
    }
    uint64_t now = milliseconds();
    struct wl_event_source *source;
    struct wl_event_source cursor = {.kind = SOURCE_CURSOR};
    wl_list_insert(&loop->sources, &cursor.link);
    while ((source = advance(loop, &cursor)))
    {
        if (source->removed)
            continue;
        if (source->kind == SOURCE_TIMER && source->deadline <= now)
        {
            source->deadline = UINT64_MAX;
            dispatch(source, 0);
        }
        else if (source->kind == SOURCE_SIGNAL && (pending & (1u << source->number)))
            dispatch(source, 0);
    }
    wl_list_remove(&cursor.link);
    wl_event_loop_dispatch_idle(loop);
    bool again;
    do
    {
        again = false;
        wl_list_insert(&loop->sources, &cursor.link);
        while ((source = advance(loop, &cursor)))
            if (!source->removed && source->checked)
                again |= dispatch(source, 0) != 0;
        wl_list_remove(&cursor.link);
    } while (again);
finished:
    if (!--loop->dispatching)
        retire(loop);
    update_monitor(loop);
    return result;
}

static void *monitor(void *data)
{
    struct wl_event_loop *loop = data;
    struct pollfd polls[SOURCE_LIMIT + 1];
    pthread_mutex_lock(&loop->mutex);
    while (!loop->stopping)
    {
        unsigned count = loop->observed_count;
        memcpy(polls, loop->observed, count * sizeof(*polls));
        uint64_t deadline = loop->observed_deadline;
        pthread_mutex_unlock(&loop->mutex);
        int ready = poll(polls, count, time_left(deadline));
        pthread_mutex_lock(&loop->mutex);
        if (loop->stopping)
            break;
        if (ready < 0 && errno == EINTR)
            continue;
        loop->pending = true;
        poke(loop->notification[1]);
        while (loop->pending && !loop->stopping)
            pthread_cond_wait(&loop->condition, &loop->mutex);
    }
    pthread_mutex_unlock(&loop->mutex);
    return NULL;
}

WL_EXPORT int wl_event_loop_get_fd(struct wl_event_loop *loop)
{
    if (loop->monitor_started)
        return loop->notification[0];
    if (make_pipe(loop->notification))
        return -1;
    observe(loop);
    int error = pthread_create(&loop->monitor, NULL, monitor, loop);
    if (error)
    {
        close(loop->notification[0]);
        close(loop->notification[1]);
        errno = error;
        return -1;
    }
    loop->monitor_started = true;
    return loop->notification[0];
}

WL_EXPORT struct wl_event_loop *wl_event_loop_create(void)
{
    struct wl_event_loop *loop = calloc(1, sizeof(*loop));
    if (!loop)
        return NULL;
    if (make_pipe(loop->wake))
    {
        free(loop);
        return NULL;
    }
    loop->signal_slot = -1;
    loop->dirty = true;
    wl_list_init(&loop->sources);
    wl_list_init(&loop->retired);
    wl_priv_signal_init(&loop->destroy);
    pthread_mutex_init(&loop->mutex, NULL);
    pthread_cond_init(&loop->condition, NULL);
    return loop;
}

WL_EXPORT void wl_event_loop_destroy(struct wl_event_loop *loop)
{
    if (loop->monitor_started)
    {
        pthread_mutex_lock(&loop->mutex);
        loop->stopping = true;
        pthread_cond_signal(&loop->condition);
        pthread_mutex_unlock(&loop->mutex);
        poke(loop->wake[1]);
        pthread_join(loop->monitor, NULL);
        close(loop->notification[0]);
        close(loop->notification[1]);
    }
    wl_priv_signal_final_emit(&loop->destroy, loop);
    struct wl_event_source *source, *next;
    wl_list_for_each_safe(source, next, &loop->sources, link) wl_event_source_remove(source);
    retire(loop);
    if (loop->signal_slot >= 0)
    {
        struct signal_slot *slot = &signal_slots[loop->signal_slot];
        __atomic_fetch_and(&slot->state, ~SLOT_ACTIVE, __ATOMIC_ACQ_REL);
        while (__atomic_load_n(&slot->state, __ATOMIC_ACQUIRE))
            sched_yield();
    }
    close(loop->wake[0]);
    close(loop->wake[1]);
    pthread_cond_destroy(&loop->condition);
    pthread_mutex_destroy(&loop->mutex);
    free(loop);
}

WL_EXPORT void wl_event_loop_add_destroy_listener(struct wl_event_loop *loop,
                                                  struct wl_listener *listener)
{
    wl_priv_signal_add(&loop->destroy, listener);
}

WL_EXPORT struct wl_listener *wl_event_loop_get_destroy_listener(struct wl_event_loop *loop,
                                                                 wl_notify_func_t notify)
{
    return wl_priv_signal_get(&loop->destroy, notify);
}
