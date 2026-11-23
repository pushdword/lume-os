/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_AUTH_TERMINAL_H
#define LUME_AUTH_TERMINAL_H

#include <stdbool.h>
#include <stddef.h>

bool auth_terminal_ready(void);
bool auth_terminal_interrupted(void);
bool auth_terminal_is_pty(void);
int auth_read_line(const char *prompt, char *text, size_t capacity, bool echo);

#endif
