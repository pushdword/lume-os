/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_AUTH_FILES_H
#define LUME_AUTH_FILES_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/stat.h>

int auth_trusted_directory(int parent, const char *name, bool create, mode_t mode);
bool auth_permissions_volume(int directory, bool writable);
bool auth_close_descriptors(void);
bool auth_random(unsigned char *buffer, size_t bytes);
int auth_install_sudo(void);

#endif
