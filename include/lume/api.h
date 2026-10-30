#ifndef LUME_APP_API_H
#define LUME_APP_API_H
#include <lume/syscall.h>
#ifdef __cplusplus
extern "C"
{
#endif
    int32_t lume_syscall(uint32_t, uint32_t, uint32_t, uint32_t);
    void app_puts(const char *);
    void app_number(uint32_t);
    unsigned app_length(const char *);
    int app_equal(const char *, const char *);
    _Noreturn void app_exit(unsigned);
    int app_open(const char *, unsigned);
    int app_close(int);
    int app_read(int, void *, unsigned);
    int app_write(int, const void *, unsigned);
    /* Retry interrupted writes and preserve progress before other incomplete results.
     * Requests larger than INT32_MAX fail without writing. */
    int app_write_all(int, const void *, unsigned);
    int app_seek(int, int32_t, unsigned);
    int app_sync(int);
    struct lume_boot_request;
    int app_boot(unsigned, struct lume_boot_request *);
    int app_stat(const char *, struct lume_stat *);
    /* The runtime keeps raw stdio mode separately for each process. Kernel console input
     * supplies bytes. */
    void lume_stdio_raw(int);
    /* Flush standard streams before changing their descriptors, then discard cached bytes
     * associated with the previous descriptors. */
    void lume_stdio_reset(void);
    int app_spawn(const char *, unsigned, const char *const *);
    int app_spawn_env(const char *, unsigned, const char *const *, unsigned, const char *const *);
    int app_wait(int, struct lume_wait_status *);
    void app_error(const char *, int);
#ifdef __cplusplus
}
#endif
#endif
