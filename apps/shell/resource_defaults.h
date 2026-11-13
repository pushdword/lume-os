#ifndef LUME_SHELL_RESOURCE_DEFAULTS_H
#define LUME_SHELL_RESOURCE_DEFAULTS_H
#include "lume/credential.h"
#include "lume/syscall_numbers.h"
#include "lume/syscall_native.h"

/* The primary shell applies the boot default before its children inherit limits. */
static void shell_resource_defaults(void)
{
    int fd = app_open("/dev/limits/nofile-default", LUME_RDONLY);
    if (fd < 0)
        return;
    char text[32];
    int bytes = app_read(fd, text, sizeof(text));
    int closed = app_close(fd);
    if (bytes <= 0 || closed < 0)
        return;
    unsigned value = 0;
    for (int i = 0; i < bytes && text[i] != '\n'; ++i)
    {
        unsigned digit = (unsigned)(unsigned char)text[i] - '0';
        if (digit > 9 || value > (UINT32_MAX - digit) / 10)
            return;
        value = value * 10 + digit;
    }
    struct lume_limit_request request = {
        .version = LUME_CREDENTIAL_VERSION, .bytes = sizeof(request), .resource = LUME_LIMIT_FILES};
    if (lume_native_call(LUME_SYS_CREDENTIAL, LUME_CREDENTIAL_LIMIT_GET, (uintptr_t)&request,
                         sizeof(request)) ||
        value < 3 || value > request.hard)
        return;
    request.soft = value;
    int result = (int)lume_native_call(LUME_SYS_CREDENTIAL, LUME_CREDENTIAL_LIMIT_SET,
                                       (uintptr_t)&request, sizeof(request));
    if (result < 0)
        app_error("shell: descriptor limit", result);
}

#endif
