/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_POSIX_FTW_H
#define LUME_POSIX_FTW_H
#include <sys/stat.h>
#define FTW_F 0
#define FTW_D 1
#define FTW_DNR 2
#define FTW_NS 3
#define FTW_DP 4
#define FTW_SL 5
#define FTW_PHYS 1
#define FTW_MOUNT 2
#define FTW_DEPTH 4
#define FTW_CHDIR 8

struct FTW
{
    int base, level;
};
#ifdef __cplusplus
extern "C"
{
#endif
    int nftw(const char *, int (*)(const char *, const struct stat *, int, struct FTW *), int, int);
#ifdef __cplusplus
}
#endif
#endif
