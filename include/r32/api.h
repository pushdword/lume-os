#ifndef LUME_APP_API_H
#define LUME_APP_API_H
#include <lume/syscall.h>
#ifdef __cplusplus
extern "C" {
#endif
int32_t lume_syscall(uint32_t,uint32_t,uint32_t,uint32_t);
void app_puts(const char *);
void app_number(uint32_t);
unsigned app_length(const char *);
int app_equal(const char *,const char *);
_Noreturn void app_exit(unsigned);
int app_open(const char *,unsigned);
int app_close(int);
int app_read(int,void *,unsigned);
int app_write(int,const void *,unsigned);
int app_seek(int,int32_t,unsigned);
int app_sync(int);
int app_stat(const char *,struct lume_stat *);
/* Raw stdio is per-process runtime state; kernel console input is always bytes. */
void lume_stdio_raw(int);
int app_spawn(const char *,unsigned,const char *const *);
int app_spawn_env(const char *,unsigned,const char *const *,unsigned,const char *const *);
int app_wait(int,struct lume_wait_status *);
void app_error(const char *,int);
#ifdef __cplusplus
}
#endif
#endif
