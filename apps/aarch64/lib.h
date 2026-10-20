#ifndef LUME_USER_LIB_H
#define LUME_USER_LIB_H
#include "lume/abi.h"
#include <stddef.h>
#ifdef LUME_USER_HOST_TEST
int64_t call(uint64_t,uint64_t,uint64_t,uint64_t);
#else
static inline int64_t call(uint64_t n,uint64_t a,uint64_t b,uint64_t c)
{
    register uint64_t x8 __asm__("x8")=n;
    register uint64_t x0 __asm__("x0")=a;
    register uint64_t x1 __asm__("x1")=b;
    register uint64_t x2 __asm__("x2")=c;
    __asm__ volatile("svc #0x4c55":"+r"(x0):"r"(x8),"r"(x1),"r"(x2):"memory","cc");
    return (int64_t)x0;
}
#endif
static size_t len(const char *s) {size_t n=0;while(s[n])++n;return n;}
static int eq(const char *a,const char *b) {while(*a && *a==*b){++a;++b;}return *a==*b;}
static int64_t open(const char *p,uint64_t flags) {return call(LUME_SYS_OPEN,(uintptr_t)p,flags,0);}
static int64_t close(int fd) {return call(LUME_SYS_CLOSE,(uint64_t)fd,0,0);}
static int64_t read(int fd,void *p,size_t n) {return call(LUME_SYS_READ,(uint64_t)fd,(uintptr_t)p,n);}
static int64_t write(int fd,const void *p,size_t n) {return call(LUME_SYS_WRITE,(uint64_t)fd,(uintptr_t)p,n);}
static void puts(const char *s) {(void)write(1,s,len(s));}
#endif
