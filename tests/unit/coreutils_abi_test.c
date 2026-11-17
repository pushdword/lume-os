/* SPDX-License-Identifier: BSD-3-Clause */
#include <config.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "dev-ino.h"

static_assert(sizeof(off_t) >= sizeof(int64_t));
static_assert(BITSIZEOF_PTRDIFF_T == sizeof(ptrdiff_t) * CHAR_BIT);
static_assert(BITSIZEOF_SIZE_T == sizeof(size_t) * CHAR_BIT);
static_assert(sizeof(((struct stat *)0)->st_ino) == sizeof(uint64_t));
static_assert(sizeof(struct stat) == sizeof(struct stat64));
static_assert(sizeof(((struct dev_ino *)0)->st_ino) >= sizeof(uint64_t));
