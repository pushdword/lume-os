/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_FDISK_ERR_H
#define LUME_FDISK_ERR_H

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline void fdisk_message(int error, const char *format, va_list arguments)
{
    int saved = errno;
    fputs("fdisk: ", stderr);
    if (format)
    {
        vfprintf(stderr, format, arguments);
        if (error)
        {
            fputs(": ", stderr);
        }
    }
    if (error)
    {
        fputs(strerror(saved), stderr);
    }
    fputc('\n', stderr);
    errno = saved;
}

static inline void warn(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    fdisk_message(1, format, arguments);
    va_end(arguments);
}

static inline void warnx(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    fdisk_message(0, format, arguments);
    va_end(arguments);
}

static inline _Noreturn void err(int status, const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    fdisk_message(1, format, arguments);
    va_end(arguments);
    exit(status);
}

static inline _Noreturn void errx(int status, const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    fdisk_message(0, format, arguments);
    va_end(arguments);
    exit(status);
}

#endif
