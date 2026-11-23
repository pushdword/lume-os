/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_AUTH_PASSWORD_H
#define LUME_AUTH_PASSWORD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define AUTH_NAME_MAX 63
#define AUTH_PASSWORD_MAX 256
#define AUTH_PASSWORD_MIN 15

bool auth_name_valid(const char *name);
bool auth_password_matches(const char *name, uint32_t uid, const char *password, size_t bytes);
int auth_password_store(const char *name, uint32_t uid, const char *password, size_t bytes);

#endif
