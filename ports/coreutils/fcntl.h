/* SPDX-License-Identifier: BSD-3-Clause */
#include_next <fcntl.h>

#if defined(LUME_NATIVE_ABI)
#undef O_EXEC
#undef O_SEARCH
#define O_EXEC O_RDONLY
#define O_SEARCH O_RDONLY
#endif
