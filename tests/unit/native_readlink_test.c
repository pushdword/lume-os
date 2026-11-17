/* SPDX-License-Identifier: BSD-3-Clause */
#include "lume/abi.h"
#include "files_client.h"
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

static struct lume_files_request observed;
static int error;
static unsigned calls;

int64_t lume_native_files_call(unsigned operation, struct lume_files_request *request)
{
    assert(operation == LUME_FILES_READLINK);
    observed = *request;
    ++calls;
    if (error)
        return -error;
    const char target[] = "../target";
    size_t count = sizeof target - 1;
    if (count > request->count)
        count = request->count;
    memcpy((void *)(uintptr_t)request->buffer, target, count);
    return (int64_t)count;
}

int main(void)
{
    char target[32];
    memset(target, 0xaa, sizeof target);
    assert(readlink("/link", target, 3) == 3);
    assert(!memcmp(target, "../", 3) && (unsigned char)target[3] == 0xaa);
    assert(observed.fd == LUME_FILES_AT_CWD && observed.count == 3);
    assert(!strcmp((const char *)(uintptr_t)observed.path, "/link"));
    assert(readlinkat(7, "link", target, sizeof target) == 9);
    assert(observed.fd == 7 && !memcmp(target, "../target", 9));
    assert((unsigned char)target[9] == 0xaa);
    error = LUME_ELOOP;
    assert(readlink("loop", target, sizeof target) == -1 && errno == ELOOP);
    error = LUME_ENOENT;
    assert(readlinkat(8, "missing", target, sizeof target) == -1 && errno == ENOENT);
    unsigned before = calls;
    assert(readlink("link", target, 0) == -1 && errno == EINVAL && calls == before);
    return 0;
}
