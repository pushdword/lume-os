/* SPDX-License-Identifier: BSD-3-Clause */
#include "lume/abi.h"
#include "lume/io.h"
#include "lume/syscall_native.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

int main(int argc, char **argv)
{
    struct lume_boot_request request = {.version = LUME_IO_VERSION};
    int64_t result = lume_native_call(LUME_SYS_BOOT, LUME_BOOT_CAPABILITIES, (uintptr_t)&request,
                                      sizeof(request));
    printf("BOOT_CAPABILITIES result=%lld flags=%u\n", (long long)result, request.total);
    request = (struct lume_boot_request){.version = LUME_IO_VERSION};
    result = lume_native_call(LUME_SYS_BOOT, LUME_BOOT_INFO, (uintptr_t)&request, sizeof(request));
    printf("BOOT_INFO result=%lld active=%u state=%u\n", (long long)result, request.active,
           request.total);
    if (argc == 2 && !strcmp(argv[1], "--stable-confirm"))
    {
        if (result < 0 || (request.total & LUME_BOOT_STATE_TRIAL))
            return 1;
        request = (struct lume_boot_request){.version = LUME_IO_VERSION};
        result = lume_native_call(LUME_SYS_BOOT, LUME_BOOT_CONFIRM, (uintptr_t)&request,
                                  sizeof(request));
        printf("BOOT_STABLE_CONFIRM result=%lld\n", (long long)result);
        return result != -LUME_EBUSY;
    }
    return result < 0;
}
