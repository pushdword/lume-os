/* Native hosted POSIX boundary; raw Lume ABI numbers remain unchanged. */
#ifndef LUME_NATIVE_ERRNO_MAP_H
#define LUME_NATIVE_ERRNO_MAP_H
#include <stdint.h>
struct lume_stat;
#ifdef __cplusplus
extern "C" {
#endif
int lume_errno_from_syscall(unsigned);
int32_t lume_posix_syscall(uint32_t,uint32_t,uint32_t,uint32_t);
int lume_posix_open(const char *,unsigned);
int lume_posix_close(int);
int lume_posix_read(int,void *,unsigned);
int lume_posix_write(int,const void *,unsigned);
int lume_posix_seek(int,int32_t,unsigned);
int lume_posix_sync(int);
int lume_posix_stat(const char *,struct lume_stat *);
#ifdef __cplusplus
}
#endif
#endif
