/* Reuse the legacy stdio implementation without changing installed apps or
 * the raw public API. Only this hosted translation unit maps ABI error values.
 */
#include "api.h"
#include "errno_map.h"
#define lume_syscall lume_posix_syscall
#define app_open lume_posix_open
#define app_close lume_posix_close
#define app_read lume_posix_read
#define app_write lume_posix_write
#define app_seek lume_posix_seek
#define app_sync lume_posix_sync
#define app_stat lume_posix_stat
#include "../runtime/runtime.c"
