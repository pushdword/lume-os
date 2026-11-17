/* SPDX-License-Identifier: BSD-3-Clause */
#include <assert.h>
#include "../../ports/coreutils/stdio_support.h"

int main(void)
{
    FILE plain = {.flags = __SRD};
    assert(lume_coreutils_reading(&plain));
    assert(lume_coreutils_read_ahead(&plain) == 0);
    plain.unget = 0x100;
    assert(lume_coreutils_read_ahead(&plain) == 1);
    assert(lume_coreutils_write_pending(&plain) == 0);
    assert(lume_coreutils_purge(&plain) == 0);
    assert(plain.unget == 0);
    assert(plain.flags == __SRD);
    plain.flags = __SWR;
    assert(!lume_coreutils_reading(&plain));

    struct __file_bufio buffer = {
        .file = {.flags = __SRD | __SWR | __SBUF},
        .dir = __SRD,
        .pos = 100,
        .len = 80,
        .off = 30,
    };
    assert(lume_coreutils_reading(&buffer.file));
    assert(lume_coreutils_read_ahead(&buffer.file) == 50);
    buffer.file.unget = 0x141;
    assert(lume_coreutils_read_ahead(&buffer.file) == 51);
    assert(lume_coreutils_write_pending(&buffer.file) == 0);
    buffer.file.flags |= __SERR | __SEOF;
    unsigned flags = buffer.file.flags;
    assert(lume_coreutils_purge(&buffer.file) == 0);
    assert(buffer.len == 0 && buffer.off == 0 && buffer.file.unget == 0);
    assert(buffer.pos == 100 && buffer.dir == __SRD && buffer.file.flags == flags);

    buffer.dir = __SWR;
    buffer.len = 42;
    buffer.off = 5;
    assert(!lume_coreutils_reading(&buffer.file));
    assert(lume_coreutils_read_ahead(&buffer.file) == 0);
    assert(lume_coreutils_write_pending(&buffer.file) == 42);
    assert(lume_coreutils_purge(&buffer.file) == 0);
    assert(lume_coreutils_write_pending(&buffer.file) == 0);
    assert(buffer.pos == 100 && buffer.dir == __SWR && buffer.file.flags == flags);
    buffer.dir = 0;
    assert(!lume_coreutils_reading(&buffer.file));
    buffer.file.flags &= ~__SWR;
    assert(lume_coreutils_reading(&buffer.file));
    return 0;
}
