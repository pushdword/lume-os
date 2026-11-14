/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_XKBCOMMON_CONFIG_H
#define LUME_XKBCOMMON_CONFIG_H
#define _GNU_SOURCE 1
#define HAVE_UNISTD_H 1
#define HAVE_ASPRINTF 1
#define HAVE_STRNDUP 1
#define HAVE_MMAP 1
#define HAVE_GETUID 1
#define HAVE_GETEUID 1
#define HAVE_GETGID 1
#define HAVE_GETEGID 1
#define HAVE_FOPEN_E 1
#define DEFAULT_XKB_RULES "evdev"
#define DEFAULT_XKB_MODEL "pc105"
#define DEFAULT_XKB_LAYOUT "us"
#define DEFAULT_XKB_VARIANT NULL
#define DEFAULT_XKB_OPTIONS NULL
#define DFLT_XKB_CONFIG_ROOT "/usr/share/X11/xkb"
#define DFLT_XKB_CONFIG_EXTRA_PATH "/etc/xkb"
#define XLOCALEDIR "/usr/share/X11/locale"
#endif
