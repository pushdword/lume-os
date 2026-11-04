/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_RESOLV_H
#define LUME_RESOLV_H
#include <arpa/nameser.h>
#ifdef __cplusplus
extern "C"
{
#endif
    int res_query(const char *, int, int, unsigned char *, int);
    int dn_expand(const unsigned char *, const unsigned char *, const unsigned char *, char *, int);
    int dn_skipname(const unsigned char *, const unsigned char *);
#ifdef __cplusplus
}
#endif
#endif
