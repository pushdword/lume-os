#ifndef LUME_HTOP_CONFIG_H
#define LUME_HTOP_CONFIG_H
#define _DEFAULT_SOURCE 1
#define _GNU_SOURCE 1
#define _POSIX_C_SOURCE 200809L
#define _POSIX_MONOTONIC_CLOCK 200809L
#define HAVE_CONFIG_H 1
#define PACKAGE "htop"
#define PACKAGE_NAME "htop"
#define PACKAGE_VERSION "3.5.3"
#if defined(LUME_ARCH_AMD64)
#define LUME_HTOP_MACHINE "AMD64"
#elif defined(LUME_ARCH_AARCH64)
#define LUME_HTOP_MACHINE "AArch64"
#elif defined(LUME_ARCH_R32)
#define LUME_HTOP_MACHINE "R32"
#else
#define LUME_HTOP_MACHINE "X32"
#endif
#define PACKAGE_STRING "htop 3.5.3 / Lume " LUME_HTOP_MACHINE
#define VERSION "3.5.3"
#define HTOP_UNSUPPORTED 1
#define HTOP_LUME 1
#define HAVE_CURSES_H 1
#define HAVE_TERM_H 1
#define HAVE_CLOCK_GETTIME 1
#define HAVE_STRNLEN 1
#define HAVE_SET_ESCDELAY 1
#define HAVE_GETTIMEOFDAY 0
#define NDEBUG 1
#define SYSCONFDIR "/etc"
#define CONFIGDIR "/etc"
#define COPYRIGHT "Copyright (C) 2004-2026 htop developers; Lume backend port"
#define DEFAULT_SETTINGS ""
#define HTOP_CONFIG_DIR "/root"
#define HTOP_NCURSES_REENTRANT 0
#endif
