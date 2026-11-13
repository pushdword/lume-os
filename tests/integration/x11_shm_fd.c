/* SPDX-License-Identifier: BSD-3-Clause */
#include <xcb/xcb.h>
#include <xcb/shm.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static xcb_connection_t *connect_display(void)
{
    DIR *directory = opendir("/data/tmp");
    if (!directory)
        return NULL;
    struct dirent *entry;
    xcb_connection_t *connection = NULL;
    while ((entry = readdir(directory)))
    {
        if (strncmp(entry->d_name, "xauth-", 6))
            continue;
        char path[320];
        snprintf(path, sizeof(path), "/data/tmp/%s", entry->d_name);
        setenv("XAUTHORITY", path, 1);
        connection = xcb_connect("unix/:0", NULL);
        if (!xcb_connection_has_error(connection))
            break;
        xcb_disconnect(connection);
        connection = NULL;
    }
    closedir(directory);
    return connection;
}

static int checked(xcb_connection_t *connection, xcb_void_cookie_t request, unsigned expected)
{
    xcb_generic_error_t *error = xcb_request_check(connection, request);
    unsigned actual = error ? error->error_code : 0;
    if (actual != expected)
        printf("X11_SHM_FD_ERROR actual=%u expected=%u major=%u minor=%u\n", actual, expected,
               error ? error->major_code : 0, error ? error->minor_code : 0);
    free(error);
    return actual == expected ? 0 : -1;
}

int main(void)
{
    xcb_connection_t *connection = connect_display();
    if (!connection)
        return 1;
    xcb_shm_query_version_reply_t *version =
        xcb_shm_query_version_reply(connection, xcb_shm_query_version(connection), NULL);
    if (!version || version->major_version < 1 ||
        (version->major_version == 1 && version->minor_version < 2))
        return 2;
    free(version);
    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
    if (!screen || screen->root_depth != 24)
        return 3;

    enum
    {
        WIDTH = 64,
        HEIGHT = 64,
        BYTES = WIDTH * HEIGHT * 4
    };

    int fd = memfd_create("x11-shm-check", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (fd < 0 || ftruncate(fd, BYTES))
        return 4;
    uint32_t *pixels = mmap(NULL, BYTES, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (pixels == MAP_FAILED)
        return 5;
    xcb_shm_seg_t segment = xcb_generate_id(connection);
    if (checked(connection, xcb_shm_attach_fd_checked(connection, segment, dup(fd), 1), XCB_ACCESS))
        return 6;
    int sealed = fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK);
    int seal_error = errno;
    int seals = fcntl(fd, F_GET_SEALS);
    int truncated = ftruncate(fd, 4096);
    printf("X11_SHM_FD_SEALS add=%d errno=%d current=%d truncate=%d errno=%d\n", sealed,
           seal_error, seals, truncated, errno);
    if (sealed || truncated != -1 ||
        checked(connection, xcb_shm_attach_fd_checked(connection, segment, dup(fd), 1), 0))
        return 7;
    xcb_pixmap_t pixmap = xcb_generate_id(connection);
    xcb_gcontext_t gc = xcb_generate_id(connection);
    if (checked(connection,
                xcb_create_pixmap_checked(connection, 24, pixmap, screen->root, WIDTH, HEIGHT),
                0) ||
        checked(connection, xcb_create_gc_checked(connection, gc, pixmap, 0, NULL), 0))
        return 8;
    for (unsigned frame = 0; frame < 3; ++frame)
    {
        for (unsigned i = 0; i < WIDTH * HEIGHT; ++i)
            pixels[i] = (i * 65537u + frame * 0x3157u) & 0xffffffu;
        if (checked(connection,
                    xcb_shm_put_image_checked(connection, pixmap, gc, WIDTH, HEIGHT, 0, 0, WIDTH,
                                              HEIGHT, 0, 0, 24, XCB_IMAGE_FORMAT_Z_PIXMAP, 1,
                                              segment, 0),
                    0))
            return 9;
        xcb_get_image_reply_t *image =
            xcb_get_image_reply(connection,
                                xcb_get_image(connection, XCB_IMAGE_FORMAT_Z_PIXMAP, pixmap, 0, 0,
                                              WIDTH, HEIGHT, 0xffffffu),
                                NULL);
        if (!image || xcb_get_image_data_length(image) != BYTES)
            return 10;
        const uint32_t *actual = (const uint32_t *)xcb_get_image_data(image);
        for (unsigned i = 0; i < WIDTH * HEIGHT; ++i)
            if ((actual[i] & 0xffffffu) != pixels[i])
                return 11;
        free(image);
    }
    unsigned completions = 0;
    xcb_generic_event_t *event;
    const xcb_query_extension_reply_t *extension = xcb_get_extension_data(connection, &xcb_shm_id);
    while ((event = xcb_poll_for_event(connection)))
    {
        if ((event->response_type & 127) == extension->first_event + XCB_SHM_COMPLETION)
            ++completions;
        free(event);
    }
    if (completions != 3 || checked(connection, xcb_shm_detach_checked(connection, segment), 0))
        return 12;
    xcb_free_gc(connection, gc);
    xcb_free_pixmap(connection, pixmap);
    xcb_disconnect(connection);
    munmap(pixels, BYTES);
    close(fd);
    puts(
        "X11_SHM_FD_PASS frames=3 pixels=12288 completions=3 unsealed_rejected=1 shrink_blocked=1");
    return 0;
}
