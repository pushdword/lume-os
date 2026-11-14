/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef WESTON_VERSION_H
#define WESTON_VERSION_H
#define WESTON_VERSION_MAJOR 14
#define WESTON_VERSION_MINOR 0
#define WESTON_VERSION_MICRO 3
#define WESTON_VERSION "14.0.3"
#define WESTON_VERSION_AT_LEAST(major, minor, micro)                                               \
    (WESTON_VERSION_MAJOR > (major) ||                                                             \
     (WESTON_VERSION_MAJOR == (major) && WESTON_VERSION_MINOR > (minor)) ||                        \
     (WESTON_VERSION_MAJOR == (major) && WESTON_VERSION_MINOR == (minor) &&                        \
      WESTON_VERSION_MICRO >= (micro)))
#endif
