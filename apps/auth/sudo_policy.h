/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_SUDO_POLICY_H
#define LUME_SUDO_POLICY_H

#include <stdbool.h>
#include <sys/types.h>

bool sudo_policy_allows(const char *name, gid_t group, char *const command[]);
bool sudo_trusted_command(const char *path);

#endif
