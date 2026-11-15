/* SPDX-License-Identifier: BSD-3-Clause */
#define _GNU_SOURCE 1
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#ifdef __lume__
#include "lume/abi.h"
#include "lume/syscall_native.h"
#include "perf.h"
#include "lume/fd_timing.h"
#endif

#define MIB (1024u * 1024u)
#define RANDOM_COUNT 256u
#define META_COUNT 32u
#define SAMPLE_NS UINT64_C(60000000000)
#define SEED UINT32_C(0x6d2b79f5)

static size_t file_bytes = 8u * MIB;
static unsigned repetitions = 3;
static uint64_t resolution, deadline, transferred;
static int root = -1;
static unsigned char *expected, *observed;
static const char *selection = "all";

struct sample
{
    const char *name;
    unsigned repetition, workers, block, operations;
    uint64_t bytes, started, elapsed, sync_ns, verify_ns, process_cpu;
    bool cpu_valid;
#ifdef __lume__
    struct lume_perf_snapshot before;
    uint64_t ticks[LUME_PERF_MODES];
    unsigned clock_hz;
    bool telemetry;
#endif
};

static uint64_t now(void)
{
    struct timespec value;
    if (clock_gettime(CLOCK_MONOTONIC, &value))
    {
        perror("storage-bench: monotonic clock");
        exit(1);
    }
    return (uint64_t)value.tv_sec * UINT64_C(1000000000) + value.tv_nsec;
}

static uint64_t cpu_now(bool *valid)
{
#ifdef CLOCK_PROCESS_CPUTIME_ID
    struct timespec value;
    *valid = !clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &value);
    return *valid ? (uint64_t)value.tv_sec * UINT64_C(1000000000) + value.tv_nsec : 0;
#else
    *valid = false;
    return 0;
#endif
}

static int failure(const char *operation)
{
    int error = errno;
    fprintf(stderr, "storage-bench: %s: %s\n", operation, strerror(error));
    printf("{\"type\":\"error\",\"operation\":\"%s\",\"error\":%d}\n", operation, error);
    fflush(stdout);
    return 1;
}

static uint32_t mix(uint32_t value)
{
    value ^= value >> 16;
    value *= UINT32_C(0x7feb352d);
    value ^= value >> 15;
    value *= UINT32_C(0x846ca68b);
    return value ^ (value >> 16);
}

static uint32_t checksum(const void *memory, size_t bytes)
{
    const unsigned char *p = memory;
    uint32_t value = UINT32_C(2166136261);
    for (size_t i = 0; i < bytes; ++i)
        value = (value ^ p[i]) * UINT32_C(16777619);
    return value;
}

static int new_file(const char *name)
{
    return openat(root, name, O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC | O_NOFOLLOW, 0600);
}

static int input_file(const char *name)
{
    return openat(root, name, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
}

static int transfer(int fd, void *memory, size_t bytes, off_t offset, bool writing, unsigned *calls)
{
    unsigned char *p = memory;
    while (bytes)
    {
        if (!(*calls % 64) && now() >= deadline)
        {
            errno = ETIMEDOUT;
            return -1;
        }
        ssize_t done = writing ? pwrite(fd, p, bytes, offset) : pread(fd, p, bytes, offset);
        ++*calls;
        if (done < 0 && errno == EINTR)
            continue;
        if (done <= 0 || (size_t)done > bytes)
        {
            if (!done)
                errno = EIO;
            return -1;
        }
        if (writing)
            transferred += (size_t)done;
        p += done;
        offset += done;
        bytes -= done;
    }
    return 0;
}

static void begin(struct sample *sample)
{
#ifdef __lume__
    memset(sample->ticks, 0, sizeof(sample->ticks));
    sample->telemetry = !lume_native_call(LUME_SYS_TELEMETRY, (uintptr_t)&sample->before,
                                          sizeof(sample->before), LUME_PERF_VERSION);
#endif
    sample->process_cpu = cpu_now(&sample->cpu_valid);
    sample->started = now();
    deadline = sample->started + SAMPLE_NS;
}

static void end(struct sample *sample)
{
    sample->elapsed = now() - sample->started;
    bool valid;
    uint64_t cpu = cpu_now(&valid);
    sample->cpu_valid &= valid;
    sample->process_cpu = cpu - sample->process_cpu;
#ifdef __lume__
    struct lume_perf_snapshot after;
    sample->telemetry &=
        !lume_native_call(LUME_SYS_TELEMETRY, (uintptr_t)&after, sizeof(after), LUME_PERF_VERSION);
    if (sample->telemetry)
    {
        sample->clock_hz = after.clock_hz;
        for (unsigned core = 0; core < after.core_count; ++core)
            for (unsigned mode = 0; mode < LUME_PERF_MODES; ++mode)
                sample->ticks[mode] += (uint32_t)(after.cores[core].ticks[mode] -
                                                  sample->before.cores[core].ticks[mode]);
    }
#endif
}

static void report(const struct sample *sample, uint32_t hash)
{
    printf("{\"type\":\"sample\",\"case\":\"%s\",\"sample\":%u,\"workers\":%u,"
           "\"block_bytes\":%u,\"bytes\":%llu,\"io_calls\":%u,\"elapsed_ns\":%llu,"
           "\"fsync_ns\":%llu,\"verify_ns\":%llu,\"checksum\":%u,"
           "\"timer_limited\":%s,\"process_cpu_ns\":",
           sample->name, sample->repetition, sample->workers, sample->block,
           (unsigned long long)sample->bytes, sample->operations,
           (unsigned long long)sample->elapsed, (unsigned long long)sample->sync_ns,
           (unsigned long long)sample->verify_ns, hash,
           sample->elapsed < resolution * 100 ? "true" : "false");
    if (sample->cpu_valid)
        printf("%llu", (unsigned long long)sample->process_cpu);
    else
        printf("null");
#ifdef __lume__
    if (sample->telemetry)
        printf(",\"system_clock_hz\":%u,\"system_user_ticks\":%llu,\"system_kernel_ticks\":%llu,"
               "\"system_idle_ticks\":%llu,\"system_lock_ticks\":%llu",
               sample->clock_hz, (unsigned long long)sample->ticks[LUME_PERF_USER],
               (unsigned long long)sample->ticks[LUME_PERF_KERNEL],
               (unsigned long long)sample->ticks[LUME_PERF_IDLE],
               (unsigned long long)sample->ticks[LUME_PERF_LOCKWAIT]);
#endif
    puts(",\"status\":\"verified\"}");
    fflush(stdout);
}

static int check_file(int fd)
{
    unsigned calls = 0;
    deadline = now() + SAMPLE_NS;
    if (transfer(fd, observed, file_bytes, 0, false, &calls))
        return -1;
    if (memcmp(observed, expected, file_bytes))
    {
        errno = EILSEQ;
        return -1;
    }
    return 0;
}

static int seed_file(int fd)
{
    unsigned calls = 0;
    deadline = now() + SAMPLE_NS;
    return transfer(fd, expected, file_bytes, 0, true, &calls) || fsync(fd) ? -1 : 0;
}

static int discard(int fd, const char *name)
{
    return close(fd) || unlinkat(root, name, 0) || fsync(root) ? -1 : 0;
}

static int sequential(const char *name, unsigned block, unsigned rep, bool writing)
{
    struct sample sample = {
        .name = name, .block = block, .repetition = rep, .workers = 1, .bytes = file_bytes};
    int fd = writing ? new_file("sequential.bin") : input_file("fixture.bin");
    if (fd < 0)
        return -1;
    memset(observed, 0, file_bytes);
    begin(&sample);
    for (size_t offset = 0; offset < file_bytes; offset += block)
        if (transfer(fd, (writing ? expected : observed) + offset, block, offset, writing,
                     &sample.operations))
            return -1;
    if (writing)
    {
        uint64_t started = now();
        if (fsync(fd))
            return -1;
        sample.sync_ns = now() - started;
    }
    end(&sample);
    uint64_t verify = now();
    if (writing ? check_file(fd) : memcmp(expected, observed, file_bytes))
    {
        if (!errno)
            errno = EILSEQ;
        return -1;
    }
    sample.verify_ns = now() - verify;
    report(&sample, checksum(observed, file_bytes));
    return writing ? discard(fd, "sequential.bin") : close(fd);
}

static unsigned random_index(unsigned number)
{
    return (number * 73u + 19u) % (file_bytes / 4096);
}

static int random_io(unsigned rep, bool writing, bool synchronous)
{
    const char *name = synchronous ? "write-sync-4k"
                       : writing   ? "write-random-4k"
                                   : "read-random-4k";
    unsigned count = synchronous ? 16 : RANDOM_COUNT;
    struct sample sample = {
        .name = name, .block = 4096, .workers = 1, .repetition = rep, .bytes = count * 4096u};
    int fd = writing ? new_file("random.bin") : input_file("fixture.bin");
    if (fd < 0 || (writing && seed_file(fd)))
        return -1;
    if (writing)
        for (unsigned n = 0; n < count; ++n)
            expected[random_index(n) * 4096] ^= 0x5a;
    begin(&sample);
    for (unsigned n = 0; n < count; ++n)
    {
        size_t offset = (size_t)random_index(n) * 4096;
        void *buffer = writing ? expected + offset : observed + (size_t)n * 4096;
        if (transfer(fd, buffer, 4096, offset, writing, &sample.operations))
            return -1;
        if (synchronous)
        {
            uint64_t started = now();
            if (fsync(fd))
                return -1;
            sample.sync_ns += now() - started;
        }
    }
    if (writing && !synchronous)
    {
        uint64_t started = now();
        if (fsync(fd))
            return -1;
        sample.sync_ns = now() - started;
    }
    end(&sample);
    uint64_t verify = now();
    if (writing)
    {
        if (check_file(fd))
            return -1;
    }
    else
        for (unsigned n = 0; n < count; ++n)
            if (memcmp(observed + (size_t)n * 4096, expected + random_index(n) * 4096, 4096))
            {
                errno = EILSEQ;
                return -1;
            }
    sample.verify_ns = now() - verify;
    report(&sample, checksum(observed, writing ? file_bytes : count * 4096u));
    if (writing)
        for (unsigned n = 0; n < count; ++n)
            expected[random_index(n) * 4096] ^= 0x5a;
    return writing ? discard(fd, "random.bin") : close(fd);
}

static int vector_io(unsigned rep, bool writing)
{
    struct sample sample = {.name = writing ? "writev-64k" : "readv-64k",
                            .block = 65536,
                            .workers = 1,
                            .repetition = rep,
                            .bytes = file_bytes};
    int fd = writing ? new_file("vector.bin") : input_file("fixture.bin");
    if (fd < 0)
        return -1;
    memset(observed, 0, file_bytes);
    begin(&sample);
    for (size_t offset = 0; offset < file_bytes; offset += 65536)
    {
        struct iovec vectors[4];
        unsigned char *buffer = (writing ? expected : observed) + offset;
        for (unsigned n = 0; n < 4; ++n)
            vectors[n] = (struct iovec){buffer + n * 16384, 16384};
        unsigned first = 0;
        while (first < 4)
        {
            if (now() >= deadline)
            {
                errno = ETIMEDOUT;
                return -1;
            }
            ssize_t done = writing ? writev(fd, vectors + first, 4 - first)
                                   : readv(fd, vectors + first, 4 - first);
            ++sample.operations;
            if (done < 0 && errno == EINTR)
                continue;
            if (done <= 0)
            {
                if (!done)
                    errno = EIO;
                return -1;
            }
            transferred += writing ? (size_t)done : 0;
            while (done && first < 4)
            {
                size_t take =
                    (size_t)done < vectors[first].iov_len ? (size_t)done : vectors[first].iov_len;
                vectors[first].iov_base = (unsigned char *)vectors[first].iov_base + take;
                vectors[first].iov_len -= take;
                done -= take;
                if (!vectors[first].iov_len)
                    ++first;
            }
            if (done)
            {
                errno = EIO;
                return -1;
            }
        }
    }
    if (writing)
    {
        uint64_t started = now();
        if (fsync(fd))
            return -1;
        sample.sync_ns = now() - started;
    }
    end(&sample);
    uint64_t verify = now();
    if (writing ? check_file(fd) : memcmp(expected, observed, file_bytes))
    {
        errno = EILSEQ;
        return -1;
    }
    sample.verify_ns = now() - verify;
    report(&sample, checksum(observed, file_bytes));
    return writing ? discard(fd, "vector.bin") : close(fd);
}

static int stdio_io(unsigned rep, bool writing)
{
    struct sample sample = {.name = writing ? "fwrite-4k-buffer64k" : "fread-4k-buffer64k",
                            .block = 4096,
                            .workers = 1,
                            .repetition = rep,
                            .bytes = file_bytes};
    int fd = writing ? new_file("stdio.bin") : input_file("fixture.bin");
    if (fd < 0)
        return -1;
    FILE *stream = fdopen(fd, writing ? "w" : "r");
    if (!stream)
        return -1;
    char *buffer = malloc(65536);
    if (!buffer || setvbuf(stream, buffer, _IOFBF, 65536))
        return -1;
    memset(observed, 0, file_bytes);
    begin(&sample);
    for (size_t offset = 0; offset < file_bytes; offset += 4096)
    {
        if (now() >= deadline)
        {
            errno = ETIMEDOUT;
            return -1;
        }
        size_t done = writing ? fwrite(expected + offset, 1, 4096, stream)
                              : fread(observed + offset, 1, 4096, stream);
        ++sample.operations;
        if (done != 4096)
        {
            errno = EIO;
            return -1;
        }
        transferred += writing ? done : 0;
    }
    if (writing)
    {
        uint64_t started = now();
        if (fflush(stream) || fsync(fd))
            return -1;
        sample.sync_ns = now() - started;
    }
    end(&sample);
    uint64_t verify = now();
    if (writing ? check_file(fd) : memcmp(expected, observed, file_bytes))
    {
        errno = EILSEQ;
        return -1;
    }
    sample.verify_ns = now() - verify;
    report(&sample, checksum(observed, file_bytes));
    if (fclose(stream))
        return -1;
    free(buffer);
    return writing && (unlinkat(root, "stdio.bin", 0) || fsync(root)) ? -1 : 0;
}

struct worker
{
    pthread_t thread;
    pthread_mutex_t *mutex;
    pthread_cond_t *condition;
    unsigned *ready;
    bool *go, *cancel;
    size_t first, bytes;
    unsigned calls;
    int error;
};

static void *read_worker(void *opaque)
{
    struct worker *worker = opaque;
    int fd = input_file("fixture.bin");
    if (fd < 0)
        worker->error = errno;
    pthread_mutex_lock(worker->mutex);
    ++*worker->ready;
    pthread_cond_broadcast(worker->condition);
    while (!*worker->go)
        pthread_cond_wait(worker->condition, worker->mutex);
    bool cancel = *worker->cancel;
    pthread_mutex_unlock(worker->mutex);
    for (size_t n = 0; !cancel && !worker->error && n < worker->bytes; n += 65536)
        if (transfer(fd, observed + worker->first + n, 65536, worker->first + n, false,
                     &worker->calls))
            worker->error = errno;
    if (fd >= 0 && close(fd) && !worker->error)
        worker->error = errno;
    return NULL;
}

static int parallel(unsigned rep, unsigned count)
{
    struct sample sample = {.name = "read-parallel-64k",
                            .workers = count,
                            .block = 65536,
                            .repetition = rep,
                            .bytes = file_bytes};
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
    struct worker workers[4];
    unsigned ready = 0, created = 0;
    bool go = false, cancel = false;
    int error = 0;
    memset(observed, 0, file_bytes);
    for (; created < count; ++created)
    {
        workers[created] = (struct worker){.mutex = &mutex,
                                           .condition = &condition,
                                           .ready = &ready,
                                           .go = &go,
                                           .cancel = &cancel,
                                           .first = created * (file_bytes / count),
                                           .bytes = file_bytes / count};
        error = pthread_create(&workers[created].thread, NULL, read_worker, workers + created);
        if (error)
            break;
    }
    pthread_mutex_lock(&mutex);
    while (ready < created)
        pthread_cond_wait(&condition, &mutex);
    cancel = error != 0;
    begin(&sample);
    go = true;
    pthread_cond_broadcast(&condition);
    pthread_mutex_unlock(&mutex);
    for (unsigned n = 0; n < created; ++n)
    {
        int joined = pthread_join(workers[n].thread, NULL);
        if (!error)
            error = joined ? joined : workers[n].error;
        sample.operations += workers[n].calls;
    }
    end(&sample);
    pthread_cond_destroy(&condition);
    pthread_mutex_destroy(&mutex);
    if (error)
    {
        errno = error;
        return -1;
    }
    uint64_t verify = now();
    if (memcmp(expected, observed, file_bytes))
    {
        errno = EILSEQ;
        return -1;
    }
    sample.verify_ns = now() - verify;
    report(&sample, checksum(observed, file_bytes));
    return 0;
}

static int metadata(unsigned rep)
{
    char name[32], renamed[32];
    for (unsigned phase = 0; phase < 4; ++phase)
    {
        static const char *names[] = {"metadata-create", "metadata-stat", "metadata-rename",
                                      "metadata-unlink"};
        struct sample sample = {.name = names[phase], .repetition = rep, .workers = 1};
        begin(&sample);
        for (unsigned n = 0; n < META_COUNT; ++n)
        {
            snprintf(name, sizeof(name), "meta-%02u", n);
            snprintf(renamed, sizeof(renamed), "moved-%02u", n);
            if (!phase)
            {
                int fd = new_file(name);
                if (fd < 0 || close(fd))
                    return -1;
            }
            else if (phase == 1)
            {
                struct stat st;
                if (fstatat(root, name, &st, AT_SYMLINK_NOFOLLOW) || !S_ISREG(st.st_mode) ||
                    st.st_size)
                {
                    if (!errno)
                        errno = EIO;
                    return -1;
                }
            }
            else if (phase == 2 ? renameat(root, name, root, renamed) : unlinkat(root, renamed, 0))
                return -1;
            ++sample.operations;
        }
        uint64_t started = now();
        if (phase != 1 && fsync(root))
            return -1;
        sample.sync_ns = now() - started;
        end(&sample);
        report(&sample, 0);
    }
    return 0;
}

struct heartbeat
{
    uint32_t ready, go, done;
    int descriptors[2], error;
    unsigned samples;
    uint64_t last, maximum_gap, maximum_io;
};

static void *heartbeat(void *context)
{
    struct heartbeat *beat = context;
    struct timespec interval = {.tv_nsec = 1000000};
    __atomic_store_n(&beat->ready, 1, __ATOMIC_RELEASE);
    while (!__atomic_load_n(&beat->go, __ATOMIC_ACQUIRE))
        nanosleep(&interval, NULL);
    while (!__atomic_load_n(&beat->done, __ATOMIC_ACQUIRE))
    {
        char sent = 'i', received = 0;
        uint64_t before = now();
        if (write(beat->descriptors[1], &sent, 1) != 1 ||
            read(beat->descriptors[0], &received, 1) != 1 || received != sent)
        {
            beat->error = EIO;
            break;
        }
        uint64_t after = now();
        if (__atomic_load_n(&beat->done, __ATOMIC_ACQUIRE))
            break;
        uint64_t gap = after - beat->last, io = after - before;
        if (gap > beat->maximum_gap)
            beat->maximum_gap = gap;
        if (io > beat->maximum_io)
            beat->maximum_io = io;
        beat->last = after;
        ++beat->samples;
        nanosleep(&interval, NULL);
    }
    return NULL;
}

static int responsiveness(unsigned rep)
{
    struct heartbeat beat = {0};
    pthread_t thread;
    int fd = new_file("response.bin");
    if (fd < 0 || pipe(beat.descriptors))
        return -1;
    int result = pthread_create(&thread, NULL, heartbeat, &beat);
    if (result)
    {
        errno = result;
        return -1;
    }
    struct timespec interval = {.tv_nsec = 1000000};
    while (!__atomic_load_n(&beat.ready, __ATOMIC_ACQUIRE))
        nanosleep(&interval, NULL);
    struct sample sample = {.name = "write-with-heartbeat",
                            .repetition = rep,
                            .workers = 2,
                            .block = 65536,
                            .bytes = file_bytes};
    begin(&sample);
    beat.last = sample.started;
    __atomic_store_n(&beat.go, 1, __ATOMIC_RELEASE);
    result = transfer(fd, expected, file_bytes, 0, true, &sample.operations);
    uint64_t started = now();
    if (!result)
        result = fsync(fd);
    sample.sync_ns = now() - started;
    end(&sample);
    __atomic_store_n(&beat.done, 1, __ATOMIC_RELEASE);
    uint64_t finished = now();
    int joined = pthread_join(thread, NULL);
    close(beat.descriptors[0]);
    close(beat.descriptors[1]);
    if (result || joined || beat.error || !beat.samples)
    {
        errno = result ? errno : joined ? joined : beat.error ? beat.error : EBUSY;
        return -1;
    }
    uint64_t tail = finished - beat.last;
    if (tail > beat.maximum_gap)
        beat.maximum_gap = tail;
    started = now();
    if (check_file(fd))
        return -1;
    sample.verify_ns = now() - started;
    report(&sample, checksum(observed, file_bytes));
    printf("{\"type\":\"responsiveness\",\"sample\":%u,\"samples\":%u,"
           "\"period_ns\":1000000,\"maximum_gap_ns\":%llu,\"maximum_io_ns\":%llu,"
           "\"status\":\"verified\"}\n",
           rep, beat.samples, (unsigned long long)beat.maximum_gap,
           (unsigned long long)beat.maximum_io);
    return discard(fd, "response.bin");
}

static int control_responsiveness(unsigned rep)
{
    int ready[2], go[2], done[2], local[2];
    if (pipe(ready) || pipe(go) || pipe(done) || pipe(local))
        return -1;
    pid_t writer = fork();
    if (writer < 0)
        return -1;
    if (!writer)
    {
        char byte = 1;
        int fd = new_file("control.bin");
        if (fd < 0 || write(ready[1], &byte, 1) != 1 || read(go[0], &byte, 1) != 1)
            _exit(1);
        struct sample sample = {.name = "write-with-fd-control",
                                .repetition = rep,
                                .workers = 2,
                                .block = 65536,
                                .bytes = file_bytes};
        begin(&sample);
        int error = transfer(fd, expected, file_bytes, 0, true, &sample.operations);
        uint64_t started = now();
        if (!error)
            error = fsync(fd);
        sample.sync_ns = now() - started;
        end(&sample);
        if (write(done[1], &byte, 1) != 1)
            error = -1;
        started = now();
        if (!error)
            error = check_file(fd);
        sample.verify_ns = now() - started;
        if (!error)
        {
            report(&sample, checksum(observed, file_bytes));
            error = discard(fd, "control.bin");
        }
        fflush(stdout);
        _exit(error ? 1 : 0);
    }
    char byte;
    if (read(ready[0], &byte, 1) != 1 || fcntl(done[0], F_SETFL, O_NONBLOCK) < 0)
        return -1;
    struct timespec interval = {.tv_nsec = 1000000};
    unsigned samples = 0;
    uint64_t started = now(), last = started, maximum_gap = 0, maximum_control = 0;
    bool detailed = !strcmp(selection, "control-detail");
    uint64_t maximum_dup = 0, maximum_flags = 0, maximum_close = 0;
#ifdef __lume__
    struct lume_fd_timing_snapshot timing = {0};
    if (detailed && lume_native_call(LUME_SYS_TELEMETRY, (uintptr_t)&timing, sizeof(timing),
                                     LUME_FD_TIMING_START))
        return -1;
#endif
    if (write(go[1], &byte, 1) != 1)
        return -1;
    for (;;)
    {
        ssize_t received = read(done[0], &byte, 1);
        if (received == 1)
            break;
        if (received >= 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
            return -1;
        uint64_t before = now();
        int copy = dup(local[0]);
        uint64_t duplicated = detailed ? now() : 0;
        int flags = copy < 0 ? -1 : fcntl(copy, F_GETFL);
        uint64_t queried = detailed ? now() : 0;
        if (copy < 0 || flags < 0 || close(copy))
            return -1;
        uint64_t after = now(), gap = after - last, control = after - before;
        if (detailed)
        {
            if (duplicated - before > maximum_dup)
                maximum_dup = duplicated - before;
            if (queried - duplicated > maximum_flags)
                maximum_flags = queried - duplicated;
            if (after - queried > maximum_close)
                maximum_close = after - queried;
        }
        if (gap > maximum_gap)
            maximum_gap = gap;
        if (control > maximum_control)
            maximum_control = control;
        last = after;
        ++samples;
        if (after - started > SAMPLE_NS)
        {
            errno = ETIMEDOUT;
            return -1;
        }
        nanosleep(&interval, NULL);
    }
    uint64_t elapsed = now() - started;
#ifdef __lume__
    if (detailed)
    {
        if (lume_native_call(LUME_SYS_TELEMETRY, (uintptr_t)&timing, sizeof(timing),
                             LUME_FD_TIMING_STOP))
            return -1;
        printf("{\"type\":\"control-kernel-timing\",\"sample\":%u,\"clock_hz\":%llu,"
               "\"calls\":%llu,\"inline\":%llu,\"queued\":%llu,\"max_entry_wait\":%llu,"
               "\"max_body\":%llu,\"max_queue\":%llu,\"max_return\":%llu,\"fallback\":[",
               rep, (unsigned long long)timing.clock_hz, (unsigned long long)timing.calls,
               (unsigned long long)timing.inline_calls, (unsigned long long)timing.queued_calls,
               (unsigned long long)timing.maximum_entry_wait,
               (unsigned long long)timing.maximum_body, (unsigned long long)timing.maximum_queue,
               (unsigned long long)timing.maximum_return);
        for (unsigned reason = 0; reason < LUME_FD_TIMING_REASONS; ++reason)
            printf("%s%llu", reason ? "," : "", (unsigned long long)timing.fallback[reason]);
        puts("]}");
    }
#endif
    int status;
    if (waitpid(writer, &status, 0) != writer || !WIFEXITED(status) || WEXITSTATUS(status) ||
        !samples)
    {
        errno = EIO;
        return -1;
    }
    transferred += file_bytes;
    int *channels[] = {ready, go, done, local};
    for (unsigned n = 0; n < sizeof(channels) / sizeof(channels[0]); ++n)
        if (close(channels[n][0]) || close(channels[n][1]))
            return -1;
    printf("{\"type\":\"control-responsiveness\",\"sample\":%u,\"samples\":%u,"
           "\"elapsed_ns\":%llu,\"maximum_gap_ns\":%llu,\"maximum_control_ns\":%llu,"
           "\"status\":\"verified\"}\n",
           rep, samples, (unsigned long long)elapsed, (unsigned long long)maximum_gap,
           (unsigned long long)maximum_control);
    if (detailed)
        printf("{\"type\":\"control-components\",\"sample\":%u,\"maximum_dup_ns\":%llu,"
               "\"maximum_flags_ns\":%llu,\"maximum_close_ns\":%llu,"
               "\"instrumentation\":\"monotonic samples between calls\"}\n",
               rep, (unsigned long long)maximum_dup, (unsigned long long)maximum_flags,
               (unsigned long long)maximum_close);
    return 0;
}

static int selected(const char *name)
{
    return !strcmp(selection, "all") || !strcmp(selection, name);
}

static int run(void)
{
    if (selected("read") && sequential("read-first-64k", 65536, 0, false))
        return -1;
    for (unsigned rep = 1; rep <= repetitions; ++rep)
    {
        if (!strcmp(selection, "responsiveness") && responsiveness(rep))
            return -1;
        if ((!strcmp(selection, "control") || !strcmp(selection, "control-detail")) &&
            control_responsiveness(rep))
            return -1;
        for (unsigned block = 4096; block <= MIB; block *= 16)
        {
            if (selected("read") && sequential("read-repeat", block, rep, false))
                return -1;
            if (selected("write") && sequential("write-sequential-fsync", block, rep, true))
                return -1;
        }
        if ((selected("random-read") && random_io(rep, false, false)) ||
            (selected("random-write") && random_io(rep, true, false)) ||
            (selected("sync") && random_io(rep, true, true)) ||
            (selected("vector") && (vector_io(rep, false) || vector_io(rep, true))) ||
            (selected("stdio") && (stdio_io(rep, false) || stdio_io(rep, true))) ||
            (selected("parallel") && (parallel(rep, 1) || parallel(rep, 4))) ||
            (selected("metadata") && metadata(rep)))
            return -1;
    }
    return 0;
}

static int marker(bool create)
{
    char text[96], value[96] = {0};
    int length = snprintf(text, sizeof(text), "STORAGE_FIXTURE_1 %zu %u\n", file_bytes, SEED);
    int fd = create ? new_file("fixture.info") : input_file("fixture.info");
    if (fd < 0)
        return -1;
    ssize_t done = create ? write(fd, text, length) : read(fd, value, sizeof(value));
    int error = done != length || (!create && memcmp(value, text, length));
    if (create && fsync(fd))
        error = 1;
    if (close(fd))
        error = 1;
    if (error)
        errno = EINVAL;
    return error ? -1 : 0;
}

int main(int argc, char **argv)
{
    const char *directory = NULL, *action = "run";
    for (int n = 1; n < argc; ++n)
    {
        if (n + 1 == argc)
            return 2;
        const char *option = argv[n++], *value = argv[n];
        if (!strcmp(option, "--directory"))
            directory = value;
        else if (!strcmp(option, "--action"))
            action = value;
        else if (!strcmp(option, "--case"))
            selection = value;
        else if (!strcmp(option, "--mib") || !strcmp(option, "--repetitions"))
        {
            char *end;
            unsigned long number = strtoul(value, &end, 10);
            if (*end || !number || number > (!strcmp(option, "--mib") ? 32u : 5u))
                return 2;
            if (!strcmp(option, "--mib"))
                file_bytes = number * MIB;
            else
                repetitions = number;
        }
        else
            return 2;
    }
    if (!directory || directory[0] != '/' || strlen(directory) > 240 ||
        (strcmp(action, "prepare") && strcmp(action, "run") && strcmp(action, "verify") &&
         strcmp(action, "cleanup")))
        return 2;
    if (strcmp(selection, "all") && strcmp(selection, "read") && strcmp(selection, "write") &&
        strcmp(selection, "random-read") && strcmp(selection, "random-write") &&
        strcmp(selection, "sync") && strcmp(selection, "vector") && strcmp(selection, "stdio") &&
        strcmp(selection, "parallel") && strcmp(selection, "metadata") &&
        strcmp(selection, "responsiveness") && strcmp(selection, "control") &&
        strcmp(selection, "control-detail"))
        return 2;
    struct timespec clock_resolution;
    if (clock_getres(CLOCK_MONOTONIC, &clock_resolution))
        return failure("clock resolution");
    resolution =
        (uint64_t)clock_resolution.tv_sec * UINT64_C(1000000000) + clock_resolution.tv_nsec;
    if (!resolution)
        resolution = 1;
    if (!strcmp(action, "prepare") && mkdir(directory, 0700))
        return failure("create benchmark directory");
    root = open(directory, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (root < 0)
        return failure("open benchmark directory");
    struct statvfs space;
    if (fstatvfs(root, &space) || !space.f_blocks ||
        ((!strcmp(action, "prepare") || !strcmp(action, "run")) &&
         (uint64_t)space.f_bavail * space.f_frsize < file_bytes * 2 + MIB))
    {
        errno = ENOSPC;
        return failure("benchmark filesystem space");
    }
    if (posix_memalign((void **)&expected, 4096, file_bytes) ||
        posix_memalign((void **)&observed, 4096, file_bytes))
    {
        errno = ENOMEM;
        return failure("allocate benchmark buffers");
    }
    for (size_t n = 0; n < file_bytes / 4; ++n)
        ((uint32_t *)expected)[n] = mix((uint32_t)n ^ SEED);
    memset(observed, 0, file_bytes);
    struct utsname identity;
    if (uname(&identity))
        return failure("identify operating system");
    printf("{\"type\":\"identity\",\"schema\":1,\"pid\":%ld,\"os\":\"%s\",\"machine\":\"%s\","
           "\"file_bytes\":%zu,\"repetitions\":%u,\"clock_resolution_ns\":%llu,"
           "\"cache_control\":\"none\",\"storage_block_bytes\":%lu,\"free_blocks\":%llu}\n",
           (long)getpid(), identity.sysname, identity.machine, file_bytes, repetitions,
           (unsigned long long)resolution, space.f_frsize, (unsigned long long)space.f_bavail);
    fflush(stdout);
    if (!strcmp(action, "prepare"))
    {
        int fd = new_file("fixture.bin");
        if (fd < 0 || seed_file(fd) || check_file(fd) || close(fd) || marker(true) || fsync(root))
            return failure("prepare fixture");
    }
    else
    {
        struct stat st;
        if (marker(false) || fstatat(root, "fixture.bin", &st, AT_SYMLINK_NOFOLLOW) ||
            !S_ISREG(st.st_mode) || (uint64_t)st.st_size != file_bytes)
        {
            errno = EINVAL;
            return failure("validate fixture identity");
        }
        if (!strcmp(action, "run"))
        {
            if (run())
                return failure("benchmark workload");
        }
        else if (!strcmp(action, "verify"))
        {
            int fd = input_file("fixture.bin");
            if (fd < 0 || check_file(fd) || close(fd))
                return failure("verify fixture");
        }
        else
        {
            DIR *dir = opendir(directory);
            if (!dir)
                return failure("open cleanup directory");
            struct dirent *entry;
            errno = 0;
            while ((entry = readdir(dir)))
                if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..") &&
                    strcmp(entry->d_name, "fixture.bin") && strcmp(entry->d_name, "fixture.info"))
                {
                    errno = EEXIST;
                    return failure("unexpected benchmark file");
                }
            int error = errno;
            if (closedir(dir) || error || unlinkat(root, "fixture.bin", 0) ||
                unlinkat(root, "fixture.info", 0) || fsync(root) || close(root) || rmdir(directory))
                return failure("remove benchmark directory");
            root = -1;
        }
    }
    printf("{\"type\":\"summary\",\"action\":\"%s\",\"status\":\"verified\",\"checksum\":%u,"
           "\"payload_bytes_written\":%llu}\n",
           action, checksum(expected, file_bytes), (unsigned long long)transferred);
    if (root >= 0 && close(root))
        return failure("close benchmark directory");
    free(expected);
    free(observed);
    return 0;
}
