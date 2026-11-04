/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_POSIX_SPAWN_H
#define LUME_POSIX_SPAWN_H
#include_next <spawn.h>
#ifndef POSIX_SPAWN_SETSID
#define POSIX_SPAWN_SETSID 0x40
#endif
#define POSIX_SPAWN_CLOEXEC_DEFAULT 0x4000
#ifdef __cplusplus
extern "C"
{
#endif
    int posix_spawn_file_actions_addchdir(posix_spawn_file_actions_t *, const char *);
    int posix_spawn_file_actions_addchdir_np(posix_spawn_file_actions_t *, const char *);
#ifdef __cplusplus
}
#endif
#endif
