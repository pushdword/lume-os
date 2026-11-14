/* SPDX-License-Identifier: BSD-3-Clause */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define NS_PER_MS UINT64_C(1000000)
#define CHUNK 65536u
#define MAX_SAMPLES 65536u

struct observation
{
    uint64_t deadline, observed;
};

static int timestamp(clockid_t clock, uint64_t *out)
{
    struct timespec t;
    if (clock_gettime(clock, &t))
        return errno ? errno : EIO;
    if (t.tv_sec < 0 || t.tv_nsec < 0 || t.tv_nsec >= 1000000000)
        return EOVERFLOW;
    *out = (uint64_t)t.tv_sec * UINT64_C(1000000000) + (uint64_t)t.tv_nsec;
    return 0;
}

static struct timespec timespec_ns(uint64_t ns)
{
    return (struct timespec){.tv_sec = (time_t)(ns / UINT64_C(1000000000)),
                             .tv_nsec = (long)(ns % UINT64_C(1000000000))};
}

static int compare_u64(const void *a, const void *b)
{
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}

static uint64_t percentile(const uint64_t *values, unsigned n, unsigned percent)
{
    return values[((uint64_t)n * percent + 99) / 100 - 1];
}

static void report(const char *name, const struct observation *samples, unsigned n, uint64_t period,
                   uint64_t resolution, int error)
{
    uint64_t *late = calloc(n ? n : 1, sizeof(*late));
    if (!late)
    {
        puts("{\"type\":\"error\",\"error\":\"allocation\"}");
        return;
    }
    unsigned missed = 0, stalls100 = 0, stalls250 = 0;
    for (unsigned i = 0; i < n; ++i)
    {
        late[i] = samples[i].observed > samples[i].deadline
                      ? samples[i].observed - samples[i].deadline
                      : 0;
        missed += late[i] >= period;
        stalls100 += late[i] >= 100 * NS_PER_MS;
        stalls250 += late[i] >= 250 * NS_PER_MS;
        printf("{\"type\":\"sample\",\"case\":\"%s\",\"index\":%u,"
               "\"deadline_ns\":%llu,\"observed_ns\":%llu,\"lateness_ns\":%llu}\n",
               name, i, (unsigned long long)samples[i].deadline,
               (unsigned long long)samples[i].observed, (unsigned long long)late[i]);
    }
    qsort(late, n, sizeof(*late), compare_u64);
    printf("{\"type\":\"summary\",\"case\":\"%s\",\"samples\":%u,"
           "\"clock_resolution_ns\":%llu,\"period_ns\":%llu,\"missed_deadlines\":%u,"
           "\"lateness_over_100ms\":%u,\"lateness_over_250ms\":%u,\"error\":%d,"
           "\"p50_ns\":%llu,\"p95_ns\":",
           name, n, (unsigned long long)resolution, (unsigned long long)period, missed, stalls100,
           stalls250, error, (unsigned long long)(n ? percentile(late, n, 50) : 0));
    if (n >= 20)
        printf("%llu", (unsigned long long)percentile(late, n, 95));
    else
        fputs("null", stdout);
    fputs(",\"p99_ns\":", stdout);
    if (n >= 100)
        printf("%llu", (unsigned long long)percentile(late, n, 99));
    else
        fputs("null", stdout);
    printf(",\"max_ns\":%llu}\n", (unsigned long long)(n ? late[n - 1] : 0));
    free(late);
}

static int response(const char *name, unsigned duration_ms, unsigned period_ms, clockid_t clock)
{
    unsigned count = duration_ms / period_ms;
    if (!count || count > MAX_SAMPLES)
        return EINVAL;
    struct observation *samples = calloc(count, sizeof(*samples));
    if (!samples)
        return ENOMEM;
    struct timespec precision = {0};
    int error = clock_getres(CLOCK_MONOTONIC, &precision) ? errno : 0;
    uint64_t resolution = (uint64_t)precision.tv_sec * UINT64_C(1000000000) + precision.tv_nsec;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    pthread_condattr_t attr;
    int descriptors[2] = {-1, -1};
    bool cond = !strcmp(name, "condvar"), poll_fd = !strcmp(name, "poll");
    bool mutex_ready = false, cond_ready = false, attr_ready = false, locked = false;
    if (!error && cond)
    {
        error = pthread_mutex_init(&mutex, NULL);
        mutex_ready = !error;
        if (!error)
        {
            error = pthread_condattr_init(&attr);
            attr_ready = !error;
        }
        if (!error)
            error = pthread_condattr_setclock(&attr, clock);
        if (!error)
        {
            error = pthread_cond_init(&condition, &attr);
            cond_ready = !error;
        }
        if (!error)
        {
            error = pthread_mutex_lock(&mutex);
            locked = !error;
        }
    }
    if (!error && poll_fd && pipe(descriptors))
        error = errno;
    uint64_t start = 0, now = 0, period = (uint64_t)period_ms * NS_PER_MS;
    if (!error)
        error = timestamp(CLOCK_MONOTONIC, &start);
    unsigned done = 0;
    for (; !error && done < count; ++done)
    {
        uint64_t deadline = start + (done + 1) * period;
        for (;;)
        {
            error = timestamp(CLOCK_MONOTONIC, &now);
            if (error || now >= deadline)
                break;
            uint64_t remaining = deadline - now;
            if (cond)
            {
                uint64_t current;
                error = timestamp(clock, &current);
                if (error)
                    break;
                struct timespec end = timespec_ns(current + remaining);
                error = pthread_cond_timedwait(&condition, &mutex, &end);
                if (error == ETIMEDOUT)
                    error = 0;
            }
            else if (poll_fd)
            {
                struct pollfd descriptor = {.fd = descriptors[0], .events = POLLIN};
                int ready = poll(&descriptor, 1, (int)((remaining + NS_PER_MS - 1) / NS_PER_MS));
                if (ready > 0)
                    error = EIO;
                else if (ready < 0 && errno != EINTR)
                    error = errno;
            }
            else
            {
                struct timespec delay = timespec_ns(remaining);
                if (nanosleep(&delay, NULL) && errno != EINTR)
                    error = errno;
            }
            if (error)
                break;
        }
        if (error)
            break;
        samples[done] = (struct observation){deadline, now};
    }
    if (locked)
        (void)pthread_mutex_unlock(&mutex);
    if (cond_ready)
        (void)pthread_cond_destroy(&condition);
    if (attr_ready)
        (void)pthread_condattr_destroy(&attr);
    if (mutex_ready)
        (void)pthread_mutex_destroy(&mutex);
    for (unsigned i = 0; i < 2; ++i)
        if (descriptors[i] >= 0)
            close(descriptors[i]);
    report(name, samples, done, period, resolution, error);
    free(samples);
    return error;
}

struct transfer_result
{
    uint64_t bytes;
    int error;
};

static int ipc_load(unsigned duration_ms)
{
    unsigned char *pattern = malloc(2 * CHUNK), *buffer = malloc(CHUNK);
    if (!pattern || !buffer)
        return ENOMEM;
    for (unsigned i = 0; i < 2 * CHUNK; ++i)
        pattern[i] = (unsigned char)(i * 13u + 7u);
    int sockets[2], report_pipe[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) || pipe(report_pipe))
        return errno;
    (void)signal(SIGPIPE, SIG_IGN);
    if (fcntl(sockets[0], F_SETFL, O_NONBLOCK) < 0 || fcntl(sockets[1], F_SETFL, O_NONBLOCK) < 0)
        return errno;
    uint64_t start;
    int error = timestamp(CLOCK_MONOTONIC, &start);
    if (error)
        return error;
    uint64_t deadline = start + (uint64_t)duration_ms * NS_PER_MS;
    pid_t child = fork();
    if (child < 0)
        return errno;
    if (!child)
    {
        close(sockets[0]);
        close(report_pipe[0]);
        struct transfer_result result = {0};
        for (;;)
        {
            ssize_t n = read(sockets[1], buffer, CHUNK);
            if (n > 0)
            {
                if (memcmp(buffer, pattern + result.bytes % CHUNK, (size_t)n))
                {
                    result.error = EIO;
                    break;
                }
                result.bytes += (uint64_t)n;
            }
            else if (!n)
                break;
            else if (errno == EAGAIN || errno == EINTR)
            {
                uint64_t now;
                if (timestamp(CLOCK_MONOTONIC, &now) || now >= deadline + 2 * UINT64_C(1000000000))
                {
                    result.error = ETIMEDOUT;
                    break;
                }
                struct pollfd p = {.fd = sockets[1], .events = POLLIN};
                if (poll(&p, 1, 100) < 0 && errno != EINTR)
                {
                    result.error = errno;
                    break;
                }
            }
            else
            {
                result.error = errno;
                break;
            }
        }
        close(sockets[1]);
        ssize_t n = write(report_pipe[1], &result, sizeof(result));
        _exit(n == (ssize_t)sizeof(result) && !result.error ? 0 : 1);
    }
    close(sockets[1]);
    close(report_pipe[1]);
    uint64_t bytes = 0, now = start;
    while (!error && now < deadline)
    {
        ssize_t n = write(sockets[0], pattern + bytes % CHUNK, CHUNK);
        if (n > 0)
            bytes += (uint64_t)n;
        else if (n < 0 && (errno == EAGAIN || errno == EINTR))
        {
            struct pollfd p = {.fd = sockets[0], .events = POLLOUT};
            if (poll(&p, 1, 20) < 0 && errno != EINTR)
                error = errno;
        }
        else
            error = n < 0 ? errno : EIO;
        if (!error)
            error = timestamp(CLOCK_MONOTONIC, &now);
    }
    close(sockets[0]);
    struct transfer_result result = {0};
    struct pollfd p = {.fd = report_pipe[0], .events = POLLIN};
    if (poll(&p, 1, 3000) <= 0 ||
        read(report_pipe[0], &result, sizeof(result)) != (ssize_t)sizeof(result))
        error = error ? error : EIO;
    close(report_pipe[0]);
    int status = 0;
    pid_t joined;
    while ((joined = waitpid(child, &status, 0)) < 0)
        if (errno != EINTR)
        {
            error = error ? error : errno;
            break;
        }
    if (joined == child && (!WIFEXITED(status) || WEXITSTATUS(status)))
        error = error ? error : EIO;
    if (result.error || result.bytes != bytes || !bytes)
        error = error ? error : result.error ? result.error : EIO;
    printf("{\"type\":\"summary\",\"case\":\"ipc-load\",\"elapsed_ns\":%llu,"
           "\"start_ns\":%llu,\"end_ns\":%llu,"
           "\"sent_bytes\":%llu,\"verified_bytes\":%llu,\"verification_in_timed_body\":true,"
           "\"error\":%d}\n",
           (unsigned long long)(now - start), (unsigned long long)start, (unsigned long long)now,
           (unsigned long long)bytes, (unsigned long long)result.bytes, error);
    free(buffer);
    free(pattern);
    return error;
}

static unsigned argument(const char *text, unsigned maximum)
{
    char *end;
    unsigned long value = strtoul(text, &end, 10);
    return *text && !*end && value && value <= maximum ? (unsigned)value : 0;
}

static volatile sig_atomic_t interrupted;

static int debug_relay(void)
{
    int connection = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address = {
        .sin_family = AF_INET, .sin_port = htons(9222), .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};
    if (connection < 0 || connect(connection, (struct sockaddr *)&address, sizeof(address)))
    {
        perror("DevTools loopback connection");
        return 1;
    }
    int fds[] = {STDIN_FILENO, connection, STDOUT_FILENO};
    for (unsigned i = 0; i < 3; ++i)
    {
        int flags = fcntl(fds[i], F_GETFL);
        if (flags < 0 || fcntl(fds[i], F_SETFL, flags | O_NONBLOCK))
        {
            perror("DevTools relay descriptor");
            return 1;
        }
    }

    struct leg
    {
        int input, output;
        unsigned used, offset;
        bool eof, closed;
        unsigned char data[32768];
    } legs[] = {{.input = STDIN_FILENO, .output = connection},
                {.input = connection, .output = STDOUT_FILENO}};

    uint64_t start, now;
    if (timestamp(CLOCK_MONOTONIC, &start))
        return 1;
    while (!timestamp(CLOCK_MONOTONIC, &now) && now - start < UINT64_C(90000000000))
    {
        struct pollfd ready[4];
        for (unsigned i = 0; i < 2; ++i)
        {
            struct leg *leg = legs + i;
            if (!leg->used && leg->eof && !leg->closed)
            {
                if (!i)
                    shutdown(connection, SHUT_WR);
                leg->closed = true;
            }
            ready[2 * i] =
                (struct pollfd){.fd = leg->eof || leg->used ? -1 : leg->input, .events = POLLIN};
            ready[2 * i + 1] =
                (struct pollfd){.fd = leg->used ? leg->output : -1, .events = POLLOUT};
        }
        if (legs[0].closed && legs[1].closed)
            return close(connection) != 0;
        if (poll(ready, 4, 1000) < 0 && errno != EINTR)
            return 1;
        for (unsigned i = 0; i < 2; ++i)
        {
            struct leg *leg = legs + i;
            if (ready[2 * i].revents & (POLLIN | POLLHUP))
            {
                ssize_t count = read(leg->input, leg->data, sizeof(leg->data));
                if (!count)
                    leg->eof = true;
                else if (count > 0)
                {
                    leg->used = (unsigned)count;
                    leg->offset = 0;
                }
                else if (errno != EAGAIN && errno != EINTR)
                    return 1;
            }
            if (ready[2 * i + 1].revents & POLLOUT)
            {
                ssize_t count = write(leg->output, leg->data + leg->offset, leg->used);
                if (count > 0)
                {
                    leg->offset += (unsigned)count;
                    leg->used -= (unsigned)count;
                }
                else if (count < 0 && errno != EAGAIN && errno != EINTR)
                    return 1;
            }
            if ((ready[2 * i].revents | ready[2 * i + 1].revents) & (POLLERR | POLLNVAL))
                return 1;
        }
    }
    close(connection);
    return 1;
}

static void stop(int number)
{
    interrupted = number;
}

int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "--devtools-relay"))
        return debug_relay();
    const char *name = "heartbeat";
    unsigned duration = 20000, period = 20;
    clockid_t clock = CLOCK_MONOTONIC;
    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--case") && i + 1 < argc)
            name = argv[++i];
        else if (!strcmp(argv[i], "--duration-ms") && i + 1 < argc)
            duration = argument(argv[++i], 60000);
        else if (!strcmp(argv[i], "--period-ms") && i + 1 < argc)
            period = argument(argv[++i], 60000);
        else if (!strcmp(argv[i], "--realtime"))
            clock = CLOCK_REALTIME;
        else
            return 2;
    }
    if (!duration || !period || period > duration)
        return 2;
    if (strcmp(name, "ipc-load") && strcmp(name, "heartbeat") && strcmp(name, "condvar") &&
        strcmp(name, "poll"))
        return 2;
    uint64_t start;
    if (timestamp(CLOCK_MONOTONIC, &start))
        return 1;
    pid_t child = fork();
    if (child < 0)
        return 1;
    if (!child)
    {
        if (setsid() < 0)
            _exit(1);
        int error = !strcmp(name, "ipc-load") ? ipc_load(duration)
                                              : response(name, duration, period, clock);
        fflush(stdout);
        _exit(error ? 1 : 0);
    }
    (void)signal(SIGINT, stop);
    (void)signal(SIGTERM, stop);
    (void)signal(SIGHUP, stop);
    uint64_t deadline = start + ((uint64_t)duration + 3000) * NS_PER_MS;
    int status = 0;
    for (;;)
    {
        pid_t joined = waitpid(child, &status, WNOHANG);
        if (joined == child)
            return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        if (joined < 0 && errno != EINTR)
            return 1;
        uint64_t now;
        if (interrupted || timestamp(CLOCK_MONOTONIC, &now) || now >= deadline)
        {
            (void)kill(-child, SIGKILL);
            (void)kill(child, SIGKILL);
            while (waitpid(child, &status, 0) < 0 && errno == EINTR)
            {
            }
            printf("{\"type\":\"supervisor\",\"case\":\"%s\",\"status\":\"invalid\","
                   "\"reason\":\"%s\"}\n",
                   name, interrupted ? "interrupted" : "deadline");
            return 1;
        }
        struct timespec delay = {.tv_nsec = 20000000};
        (void)nanosleep(&delay, NULL);
    }
}
