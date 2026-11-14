/* SPDX-License-Identifier: BSD-3-Clause */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include "wayland-client.h"
#include "wayland-server.h"

static void require(int valid, const char *name)
{
    if (!valid)
    {
        fprintf(stderr, "WAYLAND_CORE_FAIL %s errno=%d\n", name, errno);
        exit(1);
    }
}

static uint64_t nanoseconds(void)
{
    struct timespec now;
    require(!clock_gettime(CLOCK_MONOTONIC, &now), "clock");
    return (uint64_t)now.tv_sec * 1000000000 + now.tv_nsec;
}

static void *serve(void *display)
{
    wl_display_run(display);
    return NULL;
}

static void global(void *data, struct wl_registry *registry, uint32_t name, const char *interface,
                   uint32_t version)
{
    (void)version;
    if (!strcmp(interface, "wl_shm"))
        *(struct wl_shm **)data = wl_registry_bind(registry, name, &wl_shm_interface, 1);
}

static void global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
    (void)data;
    (void)registry;
    (void)name;
}

static void counters(void)
{
    int fd = eventfd(2, EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE);
    require(fd >= 0 && (fcntl(fd, F_GETFD) & FD_CLOEXEC), "eventfd create");
    eventfd_t value;
    require(!eventfd_read(fd, &value) && value == 1, "eventfd semaphore first");
    require(!eventfd_read(fd, &value) && value == 1, "eventfd semaphore second");
    require(eventfd_read(fd, &value) < 0 && errno == EAGAIN, "eventfd empty");
    require(!eventfd_write(fd, UINT64_MAX - 1), "eventfd maximum");
    require(eventfd_write(fd, 1) < 0 && errno == EAGAIN, "eventfd overflow");
    require(eventfd_write(fd, UINT64_MAX) < 0 && errno == EINVAL, "eventfd invalid value");
    int copy = dup(fd);
    require(copy >= 0 && !eventfd_read(copy, &value) && value == 1, "eventfd shared description");
    require(!close(copy) && !close(fd), "eventfd close");
    struct timespec immediate = {0};
    require(!ppoll(NULL, 0, &immediate, NULL), "ppoll immediate");
    struct timespec invalid = {.tv_nsec = -1};
    require(ppoll(NULL, 0, &invalid, NULL) < 0 && errno == EINVAL, "ppoll invalid timeout");
}

int main(int argc, char **argv)
{
    counters();
    struct wl_display *server = wl_display_create();
    require(server != NULL, "server display");
    require(!wl_display_init_shm(server), "server shared memory");
    int sockets[2];
    require(!socketpair(AF_UNIX, SOCK_STREAM, 0, sockets), "socketpair");
    struct wl_client *peer = wl_client_create(server, sockets[0]);
    require(peer != NULL, "server client");
    pthread_t thread;
    require(!pthread_create(&thread, NULL, serve, server), "server thread");
    struct wl_display *client = wl_display_connect_to_fd(sockets[1]);
    require(client != NULL, "client display");
    struct wl_registry *registry = wl_display_get_registry(client);
    struct wl_shm *shm = NULL;
    static const struct wl_registry_listener listener = {global, global_remove};
    require(!wl_registry_add_listener(registry, &listener, &shm), "registry listener");
    require(wl_display_roundtrip(client) >= 0 && shm != NULL, "registry roundtrip");
    int fd = memfd_create("wayland-buffer", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    require(fd >= 0 && !ftruncate(fd, 8192), "buffer object");
    uint32_t *pixels = mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    require(pixels != MAP_FAILED, "client buffer mapping");
    for (unsigned i = 0; i < 2048; ++i)
        pixels[i] = 0xff000000 | (i * 7919);
    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, 8192);
    struct wl_buffer *buffer =
        wl_shm_pool_create_buffer(pool, 0, 64, 32, 256, WL_SHM_FORMAT_XRGB8888);
    require(buffer != NULL && wl_display_roundtrip(client) >= 0, "shared buffer descriptor");
    uint32_t id = wl_proxy_get_id((struct wl_proxy *)buffer);
    unsigned iterations = argc > 1 && !strcmp(argv[1], "--bench") ? 20000 : 64;
    uint64_t start = nanoseconds();
    for (unsigned i = 0; i < iterations; ++i)
        require(wl_display_roundtrip(client) >= 0, "protocol roundtrip");
    uint64_t elapsed = nanoseconds() - start;
    wl_display_terminate(server);
    require(!pthread_join(thread, NULL), "server termination");
    struct wl_resource *resource = wl_client_get_object(peer, id);
    require(resource != NULL, "server buffer resource");
    struct wl_shm_buffer *shared = wl_shm_buffer_get(resource);
    require(shared != NULL && wl_shm_buffer_get_width(shared) == 64 &&
                wl_shm_buffer_get_height(shared) == 32 && wl_shm_buffer_get_stride(shared) == 256,
            "server buffer geometry");
    wl_shm_buffer_begin_access(shared);
    const uint32_t *mapped = wl_shm_buffer_get_data(shared);
    for (unsigned i = 0; i < 2048; ++i)
        require(mapped[i] == (0xff000000 | (i * 7919)), "shared buffer contents");
    wl_shm_buffer_end_access(shared);
#ifdef __lume__
    require(ftruncate(fd, 0) < 0 && errno == EBUSY, "mapped extent protection");
#endif
    wl_client_destroy(peer);
    require(!ftruncate(fd, 0), "mapped extent release");
    wl_display_disconnect(client);
    wl_display_destroy(server);
    require(!munmap(pixels, 8192) && !close(fd), "buffer cleanup");
    printf("WAYLAND_CORE_PASS version=%s roundtrips=%u elapsed_ns=%llu\n", WAYLAND_VERSION,
           iterations, (unsigned long long)elapsed);
    return 0;
}
