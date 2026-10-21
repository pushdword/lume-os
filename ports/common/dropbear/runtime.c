/* Reuse the legacy stdio implementation without changing installed apps or
 * the raw public API. Only this hosted translation unit maps ABI error values.
 */
#include "api.h"
#include "platform.h"
#include "errno_map.h"
#define lume_syscall lume_posix_syscall
#define app_open lume_posix_open
#define app_close ssh_fdclose
#define app_read ssh_fdread
#define app_write ssh_fdwrite
#define app_seek lume_posix_seek
#define app_sync lume_posix_sync
#define app_stat lume_posix_stat
#include "runtime-base.inc"
