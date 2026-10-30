/* SPDX-License-Identifier: BSD-3-Clause */
#include "config.h"
#include <glob.h>
#include <stdio.h>
extern "C"
{
    int getloadavg(double *, int);
    const char *getprogname(void);
    FILE *popen(const char *, const char *);
    int pclose(FILE *);
}
