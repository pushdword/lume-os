#ifndef LUME_SELECT_TYPES_H
#define LUME_SELECT_TYPES_H
#if !defined(FD_SETSIZE) && (defined(__aarch64__) || defined(__x86_64__) ||                        \
                             defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64))
#define FD_SETSIZE 256
#endif
#include_next <sys/_select.h>
#endif
