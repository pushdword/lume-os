/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_TEST_COREUTILS_STDIO_BUFIO_H
#define LUME_TEST_COREUTILS_STDIO_BUFIO_H

#define __SRD 0x01
#define __SWR 0x02
#define __SERR 0x04
#define __SEOF 0x08
#define __SBUF 0x40

typedef struct
{
    unsigned short unget;
    unsigned char flags;
} FILE;

struct __file_bufio
{
    FILE file;
    unsigned char dir;
    long pos;
    int len;
    int off;
};

#endif
