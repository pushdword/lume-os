/* SPDX-License-Identifier: BSD-3-Clause */
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <sys/eventfd.h>
#include <sys/types.h>
#include <unistd.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>
_Static_assert(sizeof(off_t) == 8, "native SDK file offsets");

int main(void)
{
    struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    int fd = eventfd(3, EFD_CLOEXEC | EFD_NONBLOCK);
    eventfd_t value = 0;
    if (!context || fd < 0 || eventfd_read(fd, &value) || value != 3 ||
        xkb_keysym_from_name("Return", XKB_KEYSYM_NO_FLAGS) != XKB_KEY_Return)
        return 1;
    close(fd);
    xkb_context_unref(context);
    printf("WAYLAND_SDK_PASS version=%s pointer_bytes=%u offset_bytes=%u\n", WAYLAND_VERSION,
           (unsigned)sizeof(void *), (unsigned)sizeof(off_t));
    return 0;
}
