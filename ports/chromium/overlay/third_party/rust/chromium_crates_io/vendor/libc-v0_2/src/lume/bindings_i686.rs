use crate::prelude::*;
pub type sighandler_t = usize;

pub const _NEWLIB_VERSION: &[u8; 6] = b"4.3.0\0";
pub const _PICOLIBC_MINOR__: i32 = 8;
pub const _PICOLIBC_VERSION: &[u8; 7] = b"1.8.11\0";
pub const _PICOLIBC__: i32 = 1;
pub const __IO_DEFAULT: u8 = 100u8;
pub const __ELIX_LEVEL: i32 = 4;
pub const __NEWLIB_MINOR__: i32 = 3;
pub const __NEWLIB_PATCHLEVEL__: i32 = 0;
pub const __NEWLIB__: i32 = 4;
pub const __PICOLIBC_MINOR__: i32 = 8;
pub const __PICOLIBC_PATCHLEVEL__: i32 = 11;
pub const __PICOLIBC_VERSION__: &[u8; 7] = b"1.8.11\0";
pub const __PICOLIBC__: i32 = 1;
pub const _ATFILE_SOURCE: i32 = 1;
pub const _DEFAULT_SOURCE: i32 = 1;
pub const _ISOC99_SOURCE: i32 = 1;
pub const _ISOC11_SOURCE: i32 = 1;
pub const _ISOC23_SOURCE: i32 = 1;
pub const _POSIX_SOURCE: i32 = 1;
pub const _POSIX_C_SOURCE: i32 = 200809;
pub const _XOPEN_SOURCE: i32 = 700;
pub const _XOPEN_SOURCE_EXTENDED: i32 = 1;
pub const _LARGEFILE64_SOURCE: i32 = 1;
pub const __ATFILE_VISIBLE: i32 = 1;
pub const __BSD_VISIBLE: i32 = 1;
pub const __GNU_VISIBLE: i32 = 1;
pub const __ZEPHYR_VISIBLE: i32 = 0;
pub const __ISO_C_VISIBLE: i32 = 2023;
pub const __LARGEFILE_VISIBLE: i32 = 1;
pub const __LARGEFILE64_VISIBLE: i32 = 1;
pub const __MISC_VISIBLE: i32 = 1;
pub const __POSIX_VISIBLE: i32 = 202405;
pub const __SVID_VISIBLE: i32 = 1;
pub const __XSI_VISIBLE: i32 = 700;
pub const __SSP_FORTIFY_LEVEL: i32 = 0;
pub const __OBSOLETE_MATH_DEFAULT: i32 = 1;
pub const __OBSOLETE_MATH: i32 = 1;
pub const __RAND_MAX: i32 = 2147483647;
pub const __have_longlong64: i32 = 1;
pub const __have_long32: i32 = 1;
pub const __SCHAR_WIDTH__: i32 = 8;
pub const __LONG_LONG_WIDTH__: i32 = 64;
pub const ___int8_t_defined: i32 = 1;
pub const ___int16_t_defined: i32 = 1;
pub const ___int32_t_defined: i32 = 1;
pub const ___int64_t_defined: i32 = 1;
pub const ___int_least8_t_defined: i32 = 1;
pub const ___int_least16_t_defined: i32 = 1;
pub const ___int_least32_t_defined: i32 = 1;
pub const ___int_least64_t_defined: i32 = 1;
pub const __HAVE_BUILTIN_ADD_OVERFLOW: i32 = 1;
pub const __HAVE_BUILTIN_ALLOCA: i32 = 1;
pub const __HAVE_BUILTIN_COPYSIGN: i32 = 1;
pub const __HAVE_BUILTIN_COPYSIGNL: i32 = 1;
pub const __HAVE_BUILTIN_CTZ: i32 = 1;
pub const __HAVE_BUILTIN_CTZL: i32 = 1;
pub const __HAVE_BUILTIN_CTZLL: i32 = 1;
pub const __HAVE_BUILTIN_FFS: i32 = 1;
pub const __HAVE_BUILTIN_FFSL: i32 = 1;
pub const __HAVE_BUILTIN_FFSLL: i32 = 1;
pub const __HAVE_BUILTIN_ISFINITE: i32 = 1;
pub const __HAVE_BUILTIN_ISINF: i32 = 1;
pub const __HAVE_BUILTIN_ISNAN: i32 = 1;
pub const __HAVE_BUILTIN_MUL_OVERFLOW: i32 = 1;
pub const __GNUCLIKE_PRAGMA_DIAGNOSTIC: i32 = 1;
pub const __int8_t_defined: i32 = 1;
pub const __int16_t_defined: i32 = 1;
pub const __int32_t_defined: i32 = 1;
pub const __int64_t_defined: i32 = 1;
pub const __int20: i32 = 2;
pub const __int20__: i32 = 2;
pub const __INT8: &[u8; 3] = b"hh\0";
pub const __INT16: &[u8; 2] = b"h\0";
pub const __INT64: &[u8; 3] = b"ll\0";
pub const __FAST8: &[u8; 3] = b"hh\0";
pub const __FAST16: &[u8; 2] = b"h\0";
pub const __FAST64: &[u8; 3] = b"ll\0";
pub const __LEAST8: &[u8; 3] = b"hh\0";
pub const __LEAST16: &[u8; 2] = b"h\0";
pub const __LEAST64: &[u8; 3] = b"ll\0";
pub const __STDC_VERSION_STDINT_H__: i32 = 2106129;
pub const __int_least8_t_defined: i32 = 1;
pub const __int_least16_t_defined: i32 = 1;
pub const __int_least32_t_defined: i32 = 1;
pub const __int_least64_t_defined: i32 = 1;
pub const __int_fast8_t_defined: i32 = 1;
pub const __int_fast16_t_defined: i32 = 1;
pub const __int_fast32_t_defined: i32 = 1;
pub const __int_fast64_t_defined: i32 = 1;
pub const INT8_WIDTH: i32 = 8;
pub const UINT8_WIDTH: i32 = 8;
pub const INT32_WIDTH: i32 = 32;
pub const UINT32_WIDTH: i32 = 32;
pub const INT64_WIDTH: i32 = 64;
pub const UINT64_WIDTH: i32 = 64;
pub const _STDIO_H_: i32 = 1;
pub const LUME_LIBC_OFF_T_BITS: i32 = 64;
pub const __machine_off_t_defined: i32 = 1;
pub const __machine_fpos_t_defined: i32 = 1;
pub const __PICOLIBC_UNGETC_SIZE: i32 = 2;
pub const __SRD: i32 = 1;
pub const __SWR: i32 = 2;
pub const __SERR: i32 = 4;
pub const __SEOF: i32 = 8;
pub const __SCLOSE: i32 = 16;
pub const __SEXT: i32 = 32;
pub const __SBUF: i32 = 64;
pub const __SWIDE: i32 = 128;
pub const EOF: i32 = -1;
pub const _IOFBF: i32 = 0;
pub const _IOLBF: i32 = 1;
pub const _IONBF: i32 = 2;
pub const _FDEV_SETUP_READ: i32 = 1;
pub const _FDEV_SETUP_WRITE: i32 = 2;
pub const _FDEV_SETUP_RW: i32 = 3;
pub const _FDEV_ERR: i32 = -1;
pub const _FDEV_EOF: i32 = -2;
pub const __IO_VARIANT_DOUBLE: u8 = 100u8;
pub const __IO_VARIANT_FLOAT: u8 = 102u8;
pub const __IO_VARIANT_LLONG: u8 = 108u8;
pub const __IO_VARIANT_INTEGER: u8 = 105u8;
pub const __IO_VARIANT_MINIMAL: u8 = 109u8;
pub const _PICOLIBC_PRINTF: u8 = 100u8;
pub const _PICOLIBC_SCANF: u8 = 100u8;
pub const SEEK_SET: i32 = 0;
pub const SEEK_CUR: i32 = 1;
pub const SEEK_END: i32 = 2;
pub const BUFSIZ: i32 = 512;
pub const FOPEN_MAX: i32 = 32;
pub const FILENAME_MAX: i32 = 1024;
pub const L_tmpnam: i32 = 8;
pub const P_tmpdir: &[u8; 1] = b"\0";
pub const TMP_MAX: i32 = 32;
pub const WNOHANG: i32 = 1;
pub const WUNTRACED: i32 = 2;
pub const EXIT_FAILURE: i32 = 1;
pub const EXIT_SUCCESS: i32 = 0;
pub const RAND_MAX: i32 = 2147483647;
pub const EPERM: i32 = 1;
pub const ENOENT: i32 = 2;
pub const ESRCH: i32 = 3;
pub const EINTR: i32 = 4;
pub const EIO: i32 = 5;
pub const ENXIO: i32 = 6;
pub const E2BIG: i32 = 7;
pub const ENOEXEC: i32 = 8;
pub const EBADF: i32 = 9;
pub const ECHILD: i32 = 10;
pub const EAGAIN: i32 = 11;
pub const ENOMEM: i32 = 12;
pub const EACCES: i32 = 13;
pub const EFAULT: i32 = 14;
pub const ENOTBLK: i32 = 15;
pub const EBUSY: i32 = 16;
pub const EEXIST: i32 = 17;
pub const EXDEV: i32 = 18;
pub const ENODEV: i32 = 19;
pub const ENOTDIR: i32 = 20;
pub const EISDIR: i32 = 21;
pub const EINVAL: i32 = 22;
pub const ENFILE: i32 = 23;
pub const EMFILE: i32 = 24;
pub const ENOTTY: i32 = 25;
pub const ETXTBSY: i32 = 26;
pub const EFBIG: i32 = 27;
pub const ENOSPC: i32 = 28;
pub const ESPIPE: i32 = 29;
pub const EROFS: i32 = 30;
pub const EMLINK: i32 = 31;
pub const EPIPE: i32 = 32;
pub const EDOM: i32 = 33;
pub const ERANGE: i32 = 34;
pub const ENOMSG: i32 = 35;
pub const EIDRM: i32 = 36;
pub const ECHRNG: i32 = 37;
pub const EL2NSYNC: i32 = 38;
pub const EL3HLT: i32 = 39;
pub const EL3RST: i32 = 40;
pub const ELNRNG: i32 = 41;
pub const EUNATCH: i32 = 42;
pub const ENOCSI: i32 = 43;
pub const EL2HLT: i32 = 44;
pub const EDEADLK: i32 = 45;
pub const ENOLCK: i32 = 46;
pub const EBADE: i32 = 50;
pub const EBADR: i32 = 51;
pub const EXFULL: i32 = 52;
pub const ENOANO: i32 = 53;
pub const EBADRQC: i32 = 54;
pub const EBADSLT: i32 = 55;
pub const EDEADLOCK: i32 = 56;
pub const EBFONT: i32 = 57;
pub const ENOSTR: i32 = 60;
pub const ENODATA: i32 = 61;
pub const ETIME: i32 = 62;
pub const ENOSR: i32 = 63;
pub const ENONET: i32 = 64;
pub const ENOPKG: i32 = 65;
pub const EREMOTE: i32 = 66;
pub const ENOLINK: i32 = 67;
pub const EADV: i32 = 68;
pub const ESRMNT: i32 = 69;
pub const ECOMM: i32 = 70;
pub const EPROTO: i32 = 71;
pub const EMULTIHOP: i32 = 74;
pub const ELBIN: i32 = 75;
pub const EDOTDOT: i32 = 76;
pub const EBADMSG: i32 = 77;
pub const EFTYPE: i32 = 79;
pub const ENOTUNIQ: i32 = 80;
pub const EBADFD: i32 = 81;
pub const EREMCHG: i32 = 82;
pub const ELIBACC: i32 = 83;
pub const ELIBBAD: i32 = 84;
pub const ELIBSCN: i32 = 85;
pub const ELIBMAX: i32 = 86;
pub const ELIBEXEC: i32 = 87;
pub const ENOSYS: i32 = 88;
pub const ENOTEMPTY: i32 = 90;
pub const ENAMETOOLONG: i32 = 91;
pub const ELOOP: i32 = 92;
pub const EOPNOTSUPP: i32 = 95;
pub const EPFNOSUPPORT: i32 = 96;
pub const ECONNRESET: i32 = 104;
pub const ENOBUFS: i32 = 105;
pub const EAFNOSUPPORT: i32 = 106;
pub const EPROTOTYPE: i32 = 107;
pub const ENOTSOCK: i32 = 108;
pub const ENOPROTOOPT: i32 = 109;
pub const ESHUTDOWN: i32 = 110;
pub const ECONNREFUSED: i32 = 111;
pub const EADDRINUSE: i32 = 112;
pub const ECONNABORTED: i32 = 113;
pub const ENETUNREACH: i32 = 114;
pub const ENETDOWN: i32 = 115;
pub const ETIMEDOUT: i32 = 116;
pub const EHOSTDOWN: i32 = 117;
pub const EHOSTUNREACH: i32 = 118;
pub const EINPROGRESS: i32 = 119;
pub const EALREADY: i32 = 120;
pub const EDESTADDRREQ: i32 = 121;
pub const EMSGSIZE: i32 = 122;
pub const EPROTONOSUPPORT: i32 = 123;
pub const ESOCKTNOSUPPORT: i32 = 124;
pub const EADDRNOTAVAIL: i32 = 125;
pub const ENETRESET: i32 = 126;
pub const EISCONN: i32 = 127;
pub const ENOTCONN: i32 = 128;
pub const ETOOMANYREFS: i32 = 129;
pub const EPROCLIM: i32 = 130;
pub const EUSERS: i32 = 131;
pub const EDQUOT: i32 = 132;
pub const ESTALE: i32 = 133;
pub const ENOTSUP: i32 = 134;
pub const ENOMEDIUM: i32 = 135;
pub const EILSEQ: i32 = 138;
pub const EOVERFLOW: i32 = 139;
pub const ECANCELED: i32 = 140;
pub const ENOTRECOVERABLE: i32 = 141;
pub const EOWNERDEAD: i32 = 142;
pub const ESTRPIPE: i32 = 143;
pub const EHWPOISON: i32 = 144;
pub const EISNAM: i32 = 145;
pub const EKEYEXPIRED: i32 = 146;
pub const EKEYREJECTED: i32 = 147;
pub const EKEYREVOKED: i32 = 148;
pub const EWOULDBLOCK: i32 = 11;
pub const __ELASTERROR: i32 = 2000;
pub const _CLOCKS_PER_SEC_: i32 = 1000000;
pub const CLOCKS_PER_SEC: i32 = 1000000;
pub const TIME_UTC: i32 = 1;
pub const CLK_TCK: i32 = 1000000;
pub const TIMER_ABSTIME: i32 = 4;
pub const CLOCK_REALTIME_COARSE: clockid_t = 0 as clockid_t;
pub const CLOCK_REALTIME: clockid_t = 1 as clockid_t;
pub const CLOCK_MONOTONIC: clockid_t = 4 as clockid_t;
pub const CLOCK_MONOTONIC_RAW: clockid_t = 5 as clockid_t;
pub const CLOCK_MONOTONIC_COARSE: clockid_t = 6 as clockid_t;
pub const CLOCK_BOOTTIME: clockid_t = 7 as clockid_t;
pub const CLOCK_REALTIME_ALARM: clockid_t = 8 as clockid_t;
pub const CLOCK_BOOTTIME_ALARM: clockid_t = 9 as clockid_t;
pub const __ASCTIME_SIZE: i32 = 26;
pub const _FOPEN: i32 = -1;
pub const _FREAD: i32 = 1;
pub const _FWRITE: i32 = 2;
pub const _FAPPEND: i32 = 1024;
pub const _FMARK: i32 = 16;
pub const _FDEFER: i32 = 32;
pub const _FASYNC: i32 = 8;
pub const _FSHLOCK: i32 = 128;
pub const _FEXLOCK: i32 = 256;
pub const _FCREAT: i32 = 64;
pub const _FTRUNC: i32 = 512;
pub const _FEXCL: i32 = 2048;
pub const _FNBIO: i32 = 4096;
pub const _FSYNC: i32 = 8192;
pub const _FNONBLOCK: i32 = 16384;
pub const _FNDELAY: i32 = 16384;
pub const _FNOCTTY: i32 = 32768;
pub const _FNOINHERIT: i32 = 262144;
pub const _FDIRECT: i32 = 524288;
pub const _FNOFOLLOW: i32 = 1048576;
pub const _FDIRECTORY: i32 = 2097152;
pub const _FEXECSRCH: i32 = 4194304;
pub const O_RDONLY: i32 = 0;
pub const O_WRONLY: i32 = 1;
pub const O_RDWR: i32 = 2;
pub const O_APPEND: i32 = 1024;
pub const O_CREAT: i32 = 64;
pub const O_TRUNC: i32 = 512;
pub const O_EXCL: i32 = 2048;
pub const O_SYNC: i32 = 8192;
pub const O_NONBLOCK: i32 = 16384;
pub const O_NOCTTY: i32 = 32768;
pub const O_CLOEXEC: i32 = 262144;
pub const O_NOFOLLOW: i32 = 1048576;
pub const O_DIRECTORY: i32 = 2097152;
pub const O_EXEC: i32 = 4194304;
pub const O_SEARCH: i32 = 4194304;
pub const O_DIRECT: i32 = 524288;
pub const O_DSYNC: i32 = 8192;
pub const O_RSYNC: i32 = 8192;
pub const FAPPEND: i32 = 1024;
pub const FSYNC: i32 = 8192;
pub const FASYNC: i32 = 8;
pub const FNBIO: i32 = 4096;
pub const FNONBIO: i32 = 16384;
pub const FNDELAY: i32 = 16384;
pub const FREAD: i32 = 1;
pub const FWRITE: i32 = 2;
pub const FMARK: i32 = 16;
pub const FDEFER: i32 = 32;
pub const FSHLOCK: i32 = 128;
pub const FEXLOCK: i32 = 256;
pub const FOPEN: i32 = -1;
pub const FCREAT: i32 = 64;
pub const FTRUNC: i32 = 512;
pub const FEXCL: i32 = 2048;
pub const FNOCTTY: i32 = 32768;
pub const FNONBLOCK: i32 = 16384;
pub const FD_CLOEXEC: i32 = 1;
pub const F_DUPFD: i32 = 0;
pub const F_GETFD: i32 = 1;
pub const F_SETFD: i32 = 2;
pub const F_GETFL: i32 = 3;
pub const F_SETFL: i32 = 4;
pub const F_GETOWN: i32 = 5;
pub const F_SETOWN: i32 = 6;
pub const F_GETLK: i32 = 7;
pub const F_SETLK: i32 = 8;
pub const F_SETLKW: i32 = 9;
pub const F_RGETLK: i32 = 10;
pub const F_RSETLK: i32 = 11;
pub const F_CNVT: i32 = 12;
pub const F_RSETLKW: i32 = 13;
pub const F_DUPFD_CLOEXEC: i32 = 14;
pub const F_RDLCK: i32 = 1;
pub const F_WRLCK: i32 = 2;
pub const F_UNLCK: i32 = 3;
pub const F_UNLKSYS: i32 = 4;
pub const AT_FDCWD: i32 = -2;
pub const AT_EACCESS: i32 = 1;
pub const AT_SYMLINK_NOFOLLOW: i32 = 2;
pub const AT_SYMLINK_FOLLOW: i32 = 4;
pub const AT_REMOVEDIR: i32 = 8;
pub const AT_EMPTY_PATH: i32 = 16;
pub const _AT_NULL_PATHNAME_ALLOWED: i32 = 16384;
pub const LOCK_SH: i32 = 1;
pub const LOCK_EX: i32 = 2;
pub const LOCK_NB: i32 = 4;
pub const LOCK_UN: i32 = 8;
pub const F_ADD_SEALS: i32 = 1033;
pub const F_GET_SEALS: i32 = 1034;
pub const F_SEAL_SEAL: i32 = 1;
pub const F_SEAL_SHRINK: i32 = 2;
pub const F_SEAL_GROW: i32 = 4;
pub const F_SEAL_WRITE: i32 = 8;
pub const F_SEAL_FUTURE_WRITE: i32 = 16;
pub const F_OFD_GETLK: i32 = 36;
pub const F_OFD_SETLK: i32 = 37;
pub const F_ULOCK: i32 = 0;
pub const F_LOCK: i32 = 1;
pub const F_TLOCK: i32 = 2;
pub const F_TEST: i32 = 3;
pub const F_OK: i32 = 0;
pub const R_OK: i32 = 4;
pub const W_OK: i32 = 2;
pub const X_OK: i32 = 1;
pub const SEEK_DATA: i32 = 3;
pub const SEEK_HOLE: i32 = 4;
pub const STDIN_FILENO: i32 = 0;
pub const STDOUT_FILENO: i32 = 1;
pub const STDERR_FILENO: i32 = 2;
pub const _POSIX2_RE_DUP_MAX: i32 = 255;
pub const POSIX_CLOSE_RESTART: i32 = 1;
pub const _SC_ARG_MAX: i32 = 0;
pub const _SC_CHILD_MAX: i32 = 1;
pub const _SC_CLK_TCK: i32 = 2;
pub const _SC_NGROUPS_MAX: i32 = 3;
pub const _SC_OPEN_MAX: i32 = 4;
pub const _SC_JOB_CONTROL: i32 = 5;
pub const _SC_SAVED_IDS: i32 = 6;
pub const _SC_VERSION: i32 = 7;
pub const _SC_PAGESIZE: i32 = 8;
pub const _SC_PAGE_SIZE: i32 = 8;
pub const _SC_NPROCESSORS_CONF: i32 = 9;
pub const _SC_NPROCESSORS_ONLN: i32 = 10;
pub const _SC_PHYS_PAGES: i32 = 11;
pub const _SC_AVPHYS_PAGES: i32 = 12;
pub const _SC_MQ_OPEN_MAX: i32 = 13;
pub const _SC_MQ_PRIO_MAX: i32 = 14;
pub const _SC_RTSIG_MAX: i32 = 15;
pub const _SC_SEM_NSEMS_MAX: i32 = 16;
pub const _SC_SEM_VALUE_MAX: i32 = 17;
pub const _SC_SIGQUEUE_MAX: i32 = 18;
pub const _SC_TIMER_MAX: i32 = 19;
pub const _SC_TZNAME_MAX: i32 = 20;
pub const _SC_ASYNCHRONOUS_IO: i32 = 21;
pub const _SC_FSYNC: i32 = 22;
pub const _SC_MAPPED_FILES: i32 = 23;
pub const _SC_MEMLOCK: i32 = 24;
pub const _SC_MEMLOCK_RANGE: i32 = 25;
pub const _SC_MEMORY_PROTECTION: i32 = 26;
pub const _SC_MESSAGE_PASSING: i32 = 27;
pub const _SC_PRIORITIZED_IO: i32 = 28;
pub const _SC_REALTIME_SIGNALS: i32 = 29;
pub const _SC_SEMAPHORES: i32 = 30;
pub const _SC_SHARED_MEMORY_OBJECTS: i32 = 31;
pub const _SC_SYNCHRONIZED_IO: i32 = 32;
pub const _SC_TIMERS: i32 = 33;
pub const _SC_AIO_LISTIO_MAX: i32 = 34;
pub const _SC_AIO_MAX: i32 = 35;
pub const _SC_AIO_PRIO_DELTA_MAX: i32 = 36;
pub const _SC_DELAYTIMER_MAX: i32 = 37;
pub const _SC_THREAD_KEYS_MAX: i32 = 38;
pub const _SC_THREAD_STACK_MIN: i32 = 39;
pub const _SC_THREAD_THREADS_MAX: i32 = 40;
pub const _SC_TTY_NAME_MAX: i32 = 41;
pub const _SC_THREADS: i32 = 42;
pub const _SC_THREAD_ATTR_STACKADDR: i32 = 43;
pub const _SC_THREAD_ATTR_STACKSIZE: i32 = 44;
pub const _SC_THREAD_PRIORITY_SCHEDULING: i32 = 45;
pub const _SC_THREAD_PRIO_INHERIT: i32 = 46;
pub const _SC_THREAD_PRIO_PROTECT: i32 = 47;
pub const _SC_THREAD_PRIO_CEILING: i32 = 47;
pub const _SC_THREAD_PROCESS_SHARED: i32 = 48;
pub const _SC_THREAD_SAFE_FUNCTIONS: i32 = 49;
pub const _SC_GETGR_R_SIZE_MAX: i32 = 50;
pub const _SC_GETPW_R_SIZE_MAX: i32 = 51;
pub const _SC_LOGIN_NAME_MAX: i32 = 52;
pub const _SC_THREAD_DESTRUCTOR_ITERATIONS: i32 = 53;
pub const _SC_ADVISORY_INFO: i32 = 54;
pub const _SC_ATEXIT_MAX: i32 = 55;
pub const _SC_BARRIERS: i32 = 56;
pub const _SC_BC_BASE_MAX: i32 = 57;
pub const _SC_BC_DIM_MAX: i32 = 58;
pub const _SC_BC_SCALE_MAX: i32 = 59;
pub const _SC_BC_STRING_MAX: i32 = 60;
pub const _SC_CLOCK_SELECTION: i32 = 61;
pub const _SC_COLL_WEIGHTS_MAX: i32 = 62;
pub const _SC_CPUTIME: i32 = 63;
pub const _SC_EXPR_NEST_MAX: i32 = 64;
pub const _SC_HOST_NAME_MAX: i32 = 65;
pub const _SC_IOV_MAX: i32 = 66;
pub const _SC_IPV6: i32 = 67;
pub const _SC_LINE_MAX: i32 = 68;
pub const _SC_MONOTONIC_CLOCK: i32 = 69;
pub const _SC_RAW_SOCKETS: i32 = 70;
pub const _SC_READER_WRITER_LOCKS: i32 = 71;
pub const _SC_REGEXP: i32 = 72;
pub const _SC_RE_DUP_MAX: i32 = 73;
pub const _SC_SHELL: i32 = 74;
pub const _SC_SPAWN: i32 = 75;
pub const _SC_SPIN_LOCKS: i32 = 76;
pub const _SC_SPORADIC_SERVER: i32 = 77;
pub const _SC_SS_REPL_MAX: i32 = 78;
pub const _SC_SYMLOOP_MAX: i32 = 79;
pub const _SC_THREAD_CPUTIME: i32 = 80;
pub const _SC_THREAD_SPORADIC_SERVER: i32 = 81;
pub const _SC_TIMEOUTS: i32 = 82;
pub const _SC_TRACE: i32 = 83;
pub const _SC_TRACE_EVENT_FILTER: i32 = 84;
pub const _SC_TRACE_EVENT_NAME_MAX: i32 = 85;
pub const _SC_TRACE_INHERIT: i32 = 86;
pub const _SC_TRACE_LOG: i32 = 87;
pub const _SC_TRACE_NAME_MAX: i32 = 88;
pub const _SC_TRACE_SYS_MAX: i32 = 89;
pub const _SC_TRACE_USER_EVENT_MAX: i32 = 90;
pub const _SC_TYPED_MEMORY_OBJECTS: i32 = 91;
pub const _SC_V7_ILP32_OFF32: i32 = 92;
pub const _SC_V6_ILP32_OFF32: i32 = 92;
pub const _SC_XBS5_ILP32_OFF32: i32 = 92;
pub const _SC_V7_ILP32_OFFBIG: i32 = 93;
pub const _SC_V6_ILP32_OFFBIG: i32 = 93;
pub const _SC_XBS5_ILP32_OFFBIG: i32 = 93;
pub const _SC_V7_LP64_OFF64: i32 = 94;
pub const _SC_V6_LP64_OFF64: i32 = 94;
pub const _SC_XBS5_LP64_OFF64: i32 = 94;
pub const _SC_V7_LPBIG_OFFBIG: i32 = 95;
pub const _SC_V6_LPBIG_OFFBIG: i32 = 95;
pub const _SC_XBS5_LPBIG_OFFBIG: i32 = 95;
pub const _SC_XOPEN_CRYPT: i32 = 96;
pub const _SC_XOPEN_ENH_I18N: i32 = 97;
pub const _SC_XOPEN_LEGACY: i32 = 98;
pub const _SC_XOPEN_REALTIME: i32 = 99;
pub const _SC_STREAM_MAX: i32 = 100;
pub const _SC_PRIORITY_SCHEDULING: i32 = 101;
pub const _SC_XOPEN_REALTIME_THREADS: i32 = 102;
pub const _SC_XOPEN_SHM: i32 = 103;
pub const _SC_XOPEN_STREAMS: i32 = 104;
pub const _SC_XOPEN_UNIX: i32 = 105;
pub const _SC_XOPEN_VERSION: i32 = 106;
pub const _SC_2_CHAR_TERM: i32 = 107;
pub const _SC_2_C_BIND: i32 = 108;
pub const _SC_2_C_DEV: i32 = 109;
pub const _SC_2_FORT_DEV: i32 = 110;
pub const _SC_2_FORT_RUN: i32 = 111;
pub const _SC_2_LOCALEDEF: i32 = 112;
pub const _SC_2_PBS: i32 = 113;
pub const _SC_2_PBS_ACCOUNTING: i32 = 114;
pub const _SC_2_PBS_CHECKPOINT: i32 = 115;
pub const _SC_2_PBS_LOCATE: i32 = 116;
pub const _SC_2_PBS_MESSAGE: i32 = 117;
pub const _SC_2_PBS_TRACK: i32 = 118;
pub const _SC_2_SW_DEV: i32 = 119;
pub const _SC_2_UPE: i32 = 120;
pub const _SC_2_VERSION: i32 = 121;
pub const _SC_THREAD_ROBUST_PRIO_INHERIT: i32 = 122;
pub const _SC_THREAD_ROBUST_PRIO_PROTECT: i32 = 123;
pub const _SC_XOPEN_UUCP: i32 = 124;
pub const _SC_LEVEL1_ICACHE_SIZE: i32 = 125;
pub const _SC_LEVEL1_ICACHE_ASSOC: i32 = 126;
pub const _SC_LEVEL1_ICACHE_LINESIZE: i32 = 127;
pub const _SC_LEVEL1_DCACHE_SIZE: i32 = 128;
pub const _SC_LEVEL1_DCACHE_ASSOC: i32 = 129;
pub const _SC_LEVEL1_DCACHE_LINESIZE: i32 = 130;
pub const _SC_LEVEL2_CACHE_SIZE: i32 = 131;
pub const _SC_LEVEL2_CACHE_ASSOC: i32 = 132;
pub const _SC_LEVEL2_CACHE_LINESIZE: i32 = 133;
pub const _SC_LEVEL3_CACHE_SIZE: i32 = 134;
pub const _SC_LEVEL3_CACHE_ASSOC: i32 = 135;
pub const _SC_LEVEL3_CACHE_LINESIZE: i32 = 136;
pub const _SC_LEVEL4_CACHE_SIZE: i32 = 137;
pub const _SC_LEVEL4_CACHE_ASSOC: i32 = 138;
pub const _SC_LEVEL4_CACHE_LINESIZE: i32 = 139;
pub const _SC_POSIX_26_VERSION: i32 = 140;
pub const _PC_LINK_MAX: i32 = 0;
pub const _PC_MAX_CANON: i32 = 1;
pub const _PC_MAX_INPUT: i32 = 2;
pub const _PC_NAME_MAX: i32 = 3;
pub const _PC_PATH_MAX: i32 = 4;
pub const _PC_PIPE_BUF: i32 = 5;
pub const _PC_CHOWN_RESTRICTED: i32 = 6;
pub const _PC_NO_TRUNC: i32 = 7;
pub const _PC_VDISABLE: i32 = 8;
pub const _PC_ASYNC_IO: i32 = 9;
pub const _PC_PRIO_IO: i32 = 10;
pub const _PC_SYNC_IO: i32 = 11;
pub const _PC_FILESIZEBITS: i32 = 12;
pub const _PC_2_SYMLINKS: i32 = 13;
pub const _PC_SYMLINK_MAX: i32 = 14;
pub const _PC_ALLOC_SIZE_MIN: i32 = 15;
pub const _PC_REC_INCR_XFER_SIZE: i32 = 16;
pub const _PC_REC_MAX_XFER_SIZE: i32 = 17;
pub const _PC_REC_MIN_XFER_SIZE: i32 = 18;
pub const _PC_REC_XFER_ALIGN: i32 = 19;
pub const _PC_TIMESTAMP_RESOLUTION: i32 = 20;
pub const __BIT_TYPES_DEFINED__: i32 = 1;
pub const _LITTLE_ENDIAN: i32 = 1234;
pub const _BIG_ENDIAN: i32 = 4321;
pub const _PDP_ENDIAN: i32 = 3412;
pub const _BYTE_ORDER: i32 = 1234;
pub const _QUAD_HIGHWORD: i32 = 1;
pub const _QUAD_LOWWORD: i32 = 0;
pub const LITTLE_ENDIAN: i32 = 1234;
pub const BIG_ENDIAN: i32 = 4321;
pub const PDP_ENDIAN: i32 = 3412;
pub const BYTE_ORDER: i32 = 1234;
pub const LUME_FILES_VERSION: i32 = 1;
pub const LUME_FILES_PATH_MAX: i32 = 1024;
pub const LUME_FILES_NAME_MAX: i32 = 255;
pub const LUME_POSIX_DIRECTORY_NATIVE: i32 = 1;
pub const DT_UNKNOWN: c_uchar = 0 as c_uchar;
pub const DT_FIFO: c_uchar = 1 as c_uchar;
pub const DT_CHR: c_uchar = 2 as c_uchar;
pub const DT_DIR: c_uchar = 4 as c_uchar;
pub const DT_BLK: c_uchar = 6 as c_uchar;
pub const DT_REG: c_uchar = 8 as c_uchar;
pub const DT_LNK: c_uchar = 10 as c_uchar;
pub const DT_SOCK: c_uchar = 12 as c_uchar;
pub const MAXNAMLEN: i32 = 1024;
pub const SIGHUP: i32 = 1;
pub const SIGINT: i32 = 2;
pub const SIGQUIT: i32 = 3;
pub const SIGILL: i32 = 4;
pub const SIGTRAP: i32 = 5;
pub const SIGABRT: i32 = 6;
pub const SIGIOT: i32 = 6;
pub const SIGEMT: i32 = 7;
pub const SIGFPE: i32 = 8;
pub const SIGKILL: i32 = 9;
pub const SIGBUS: i32 = 10;
pub const SIGSEGV: i32 = 11;
pub const SIGSYS: i32 = 12;
pub const SIGPIPE: i32 = 13;
pub const SIGALRM: i32 = 14;
pub const SIGTERM: i32 = 15;
pub const SIGURG: i32 = 16;
pub const SIGSTOP: i32 = 17;
pub const SIGTSTP: i32 = 18;
pub const SIGCONT: i32 = 19;
pub const SIGCHLD: i32 = 20;
pub const SIGCLD: i32 = 20;
pub const SIGTTIN: i32 = 21;
pub const SIGTTOU: i32 = 22;
pub const SIGIO: i32 = 23;
pub const SIGPOLL: i32 = 23;
pub const SIGXCPU: i32 = 24;
pub const SIGXFSZ: i32 = 25;
pub const SIGVTALRM: i32 = 26;
pub const SIGPROF: i32 = 27;
pub const SIGWINCH: i32 = 28;
pub const SIGLOST: i32 = 29;
pub const SIGUSR1: i32 = 30;
pub const SIGUSR2: i32 = 31;
pub const NSIG: i32 = 32;
pub const _NSIG: i32 = 32;
pub const SIG_SETMASK: i32 = 0;
pub const SIG_BLOCK: i32 = 1;
pub const SIG_UNBLOCK: i32 = 2;
pub const SA_NOCLDSTOP: i32 = 1;
pub const SA_RESETHAND: i32 = 4;
pub const SA_RESTART: i32 = 8;
pub const SA_NODEFER: i32 = 64;
pub const SCHED_OTHER: i32 = 0;
pub const SCHED_FIFO: i32 = 1;
pub const SCHED_RR: i32 = 2;
pub const SCHED_IDLE: i32 = 5;
pub const SCHED_BATCH: i32 = 6;
pub const SCHED_RESET_ON_FORK: i32 = 1073741824;
pub const PTHREAD_CREATE_JOINABLE: i32 = 0;
pub const PTHREAD_CREATE_DETACHED: i32 = 1;
pub const PTHREAD_MUTEX_NORMAL: i32 = 0;
pub const PTHREAD_MUTEX_RECURSIVE: i32 = 1;
pub const PTHREAD_MUTEX_ERRORCHECK: i32 = 2;
pub const PTHREAD_MUTEX_DEFAULT: i32 = 0;
pub const PTHREAD_PROCESS_PRIVATE: i32 = 0;
pub const PTHREAD_PROCESS_SHARED: i32 = 1;
pub const PTHREAD_DESTRUCTOR_ITERATIONS: i32 = 4;
pub const POSIX_SPAWN_RESETIDS: i32 = 1;
pub const POSIX_SPAWN_SETPGROUP: i32 = 2;
pub const POSIX_SPAWN_SETSCHEDPARAM: i32 = 4;
pub const POSIX_SPAWN_SETSCHEDULER: i32 = 8;
pub const POSIX_SPAWN_SETSIGDEF: i32 = 16;
pub const POSIX_SPAWN_SETSIGMASK: i32 = 32;
pub const POSIX_SPAWN_SETSID: i32 = 64;
pub const POSIX_SPAWN_CLOEXEC_DEFAULT: i32 = 16384;
pub const POLLIN: c_short = 1 as c_short;
pub const POLLPRI: c_short = 2 as c_short;
pub const POLLOUT: c_short = 4 as c_short;
pub const POLLERR: c_short = 8 as c_short;
pub const POLLHUP: c_short = 16 as c_short;
pub const POLLNVAL: c_short = 32 as c_short;
pub const _LIBC_LIMITS_H_: i32 = 1;
pub const ARG_MAX: i32 = 65536;
pub const ATEXIT_MAX: i32 = 32;
pub const CHILD_MAX: i32 = 40;
pub const IOV_MAX: i32 = 1024;
pub const OPEN_MAX: i32 = 64;
pub const TZNAME_MAX: i32 = 10;
pub const LINK_MAX: i32 = 32767;
pub const MAX_CANON: i32 = 255;
pub const MAX_INPUT: i32 = 255;
pub const NAME_MAX: i32 = 255;
pub const PATH_MAX: i32 = 1024;
pub const PIPE_BUF: i32 = 512;
pub const COLL_WEIGHTS_MAX: i32 = 0;
pub const EXPR_NEST_MAX: i32 = 32;
pub const LINE_MAX: i32 = 2048;
pub const RE_DUP_MAX: i32 = 255;
pub const _POSIX_AIO_LISTIO_MAX: i32 = 2;
pub const _POSIX_AIO_MAX: i32 = 1;
pub const _POSIX_ARG_MAX: i32 = 4096;
pub const _POSIX_CHILD_MAX: i32 = 6;
pub const _POSIX_DELAYTIMER_MAX: i32 = 32;
pub const _POSIX_HOST_NAME_MAX: i32 = 255;
pub const _POSIX_LINK_MAX: i32 = 8;
pub const _POSIX_LOGIN_NAME_MAX: i32 = 9;
pub const _POSIX_MAX_CANON: i32 = 255;
pub const _POSIX_MAX_INPUT: i32 = 255;
pub const _POSIX_MQ_OPEN_MAX: i32 = 8;
pub const _POSIX_MQ_PRIO_MAX: i32 = 32;
pub const _POSIX_NAME_MAX: i32 = 14;
pub const _POSIX_NGROUPS_MAX: i32 = 0;
pub const _POSIX_OPEN_MAX: i32 = 16;
pub const _POSIX_PATH_MAX: i32 = 255;
pub const _POSIX_PIPE_BUF: i32 = 512;
pub const _POSIX_RE_DUP_MAX: i32 = 255;
pub const _POSIX_RTSIG_MAX: i32 = 8;
pub const _POSIX_SEM_NSEMS_MAX: i32 = 256;
pub const _POSIX_SEM_VALUE_MAX: i32 = 32767;
pub const _POSIX_SIGQUEUE_MAX: i32 = 32;
pub const _POSIX_SSIZE_MAX: i32 = 32767;
pub const _POSIX_SS_REPL_MAX: i32 = 4;
pub const _POSIX_STREAM_MAX: i32 = 8;
pub const _POSIX_SYMLINK_MAX: i32 = 255;
pub const _POSIX_SYMLOOP_MAX: i32 = 8;
pub const _POSIX_THREAD_DESTRUCTOR_ITERATIONS: i32 = 4;
pub const _POSIX_THREAD_KEYS_MAX: i32 = 128;
pub const _POSIX_THREAD_THREADS_MAX: i32 = 64;
pub const _POSIX_TIMER_MAX: i32 = 32;
pub const _POSIX_TRACE_EVENT_NAME_MAX: i32 = 30;
pub const _POSIX_TRACE_NAME_MAX: i32 = 8;
pub const _POSIX_TRACE_SYS_MAX: i32 = 8;
pub const _POSIX_TRACE_USER_EVENT_MAX: i32 = 32;
pub const _POSIX_TTY_NAME_MAX: i32 = 9;
pub const _POSIX_TZNAME_MAX: i32 = 3;
pub const _POSIX2_BC_BASE_MAX: i32 = 99;
pub const _POSIX2_BC_DIM_MAX: i32 = 2048;
pub const _POSIX2_BC_SCALE_MAX: i32 = 99;
pub const _POSIX2_BC_STRING_MAX: i32 = 1000;
pub const _POSIX2_CHARCLASS_NAME_MAX: i32 = 14;
pub const _POSIX2_COLL_WEIGHTS_MAX: i32 = 2;
pub const _POSIX2_EXPR_NEST_MAX: i32 = 32;
pub const _POSIX2_LINE_MAX: i32 = 2048;
pub const _XOPEN_IOV_MAX: i32 = 16;
pub const _XOPEN_NAME_MAX: i32 = 255;
pub const _XOPEN_PATH_MAX: i32 = 1024;
pub const __MB_LEN_MAX: i32 = 8;
pub const MB_LEN_MAX: i32 = 8;
pub const NL_ARGMAX: i32 = 32;
pub const NGROUPS_MAX: i32 = 8;
pub const AF_UNSPEC: i32 = 0;
pub const AF_UNIX: i32 = 1;
pub const AF_INET: i32 = 2;
pub const AF_INET6: i32 = 10;
pub const PF_INET: i32 = 2;
pub const PF_INET6: i32 = 10;
pub const PF_UNSPEC: i32 = 0;
pub const PF_UNIX: i32 = 1;
pub const SOCK_STREAM: i32 = 1;
pub const SOCK_DGRAM: i32 = 2;
pub const SOCK_SEQPACKET: i32 = 5;
pub const SOMAXCONN: i32 = 32;
pub const SOL_SOCKET: i32 = 1;
pub const SO_REUSEADDR: i32 = 2;
pub const SO_TYPE: i32 = 3;
pub const SO_ERROR: i32 = 4;
pub const SO_BROADCAST: i32 = 6;
pub const SO_KEEPALIVE: i32 = 9;
pub const SHUT_RD: i32 = 0;
pub const SHUT_WR: i32 = 1;
pub const SHUT_RDWR: i32 = 2;
pub const MSG_PEEK: i32 = 2;
pub const MSG_DONTWAIT: i32 = 64;
pub const MSG_NOSIGNAL: i32 = 16384;
pub const MSG_CMSG_CLOEXEC: i32 = 1073741824;
pub const SCM_RIGHTS: i32 = 1;
pub const MSG_CTRUNC: i32 = 8;
pub const MSG_TRUNC: i32 = 32;
pub const SOCK_CLOEXEC: i32 = 524288;
pub const SOCK_NONBLOCK: i32 = 2048;
pub const SO_RCVBUF: i32 = 8;
pub const SO_SNDBUF: i32 = 7;
pub const SO_RCVTIMEO: i32 = 20;
pub const SO_SNDTIMEO: i32 = 21;
pub const SO_OOBINLINE: i32 = 10;
pub const HOST_NOT_FOUND: i32 = 1;
pub const TRY_AGAIN: i32 = 2;
pub const NO_RECOVERY: i32 = 3;
pub const NO_DATA: i32 = 4;
pub const AI_PASSIVE: i32 = 1;
pub const AI_NUMERICHOST: i32 = 4;
pub const AI_CANONNAME: i32 = 2;
pub const NI_NUMERICHOST: i32 = 1;
pub const NI_NUMERICSERV: i32 = 2;
pub const NI_NAMEREQD: i32 = 8;
pub const NI_MAXHOST: i32 = 1025;
pub const NI_MAXSERV: i32 = 32;
pub const EAI_NONAME: i32 = -2;
pub const EAI_BADFLAGS: i32 = -1;
pub const EAI_FAIL: i32 = -4;
pub const EAI_FAMILY: i32 = -6;
pub const EAI_MEMORY: i32 = -10;
pub const AI_NUMERICSERV: i32 = 1024;
pub const AI_ADDRCONFIG: i32 = 32;
pub const EAI_AGAIN: i32 = -3;
pub const EAI_SYSTEM: i32 = -11;
pub const RRSET_VALIDATED: i32 = 1;
pub const ERRSET_SUCCESS: i32 = 0;
pub const ERRSET_NOMEMORY: i32 = 1;
pub const ERRSET_FAIL: i32 = 2;
pub const ERRSET_INVAL: i32 = 3;
pub const ERRSET_NONAME: i32 = 4;
pub const ERRSET_NODATA: i32 = 5;
pub const _IFMT: i32 = 61440;
pub const _IFDIR: i32 = 16384;
pub const _IFCHR: i32 = 8192;
pub const _IFBLK: i32 = 24576;
pub const _IFREG: i32 = 32768;
pub const _IFLNK: i32 = 40960;
pub const _IFSOCK: i32 = 49152;
pub const _IFIFO: i32 = 4096;
pub const S_BLKSIZE: i32 = 1024;
pub const S_ISUID: mode_t = 2048 as mode_t;
pub const S_ISGID: mode_t = 1024 as mode_t;
pub const S_ISVTX: mode_t = 512 as mode_t;
pub const S_IREAD: mode_t = 256 as mode_t;
pub const S_IWRITE: mode_t = 128 as mode_t;
pub const S_IEXEC: mode_t = 64 as mode_t;
pub const S_ENFMT: i32 = 1024;
pub const S_IFMT: mode_t = 61440 as mode_t;
pub const S_IFDIR: mode_t = 16384 as mode_t;
pub const S_IFCHR: mode_t = 8192 as mode_t;
pub const S_IFBLK: mode_t = 24576 as mode_t;
pub const S_IFREG: mode_t = 32768 as mode_t;
pub const S_IFLNK: mode_t = 40960 as mode_t;
pub const S_IFSOCK: mode_t = 49152 as mode_t;
pub const S_IFIFO: mode_t = 4096 as mode_t;
pub const S_IRUSR: mode_t = 256 as mode_t;
pub const S_IWUSR: mode_t = 128 as mode_t;
pub const S_IXUSR: mode_t = 64 as mode_t;
pub const S_IRGRP: mode_t = 32 as mode_t;
pub const S_IWGRP: mode_t = 16 as mode_t;
pub const S_IXGRP: mode_t = 8 as mode_t;
pub const S_IROTH: mode_t = 4 as mode_t;
pub const S_IWOTH: mode_t = 2 as mode_t;
pub const S_IXOTH: mode_t = 1 as mode_t;
pub const DEFFILEMODE: i32 = 438;
pub const ITIMER_REAL: i32 = 0;
pub const ITIMER_VIRTUAL: i32 = 1;
pub const ITIMER_PROF: i32 = 2;
pub const DST_NONE: i32 = 0;
pub const DST_USA: i32 = 1;
pub const DST_AUST: i32 = 2;
pub const DST_WET: i32 = 3;
pub const DST_MET: i32 = 4;
pub const DST_EET: i32 = 5;
pub const DST_CAN: i32 = 6;
pub const SBT_MAX: i64 = 9223372036854775807;
pub const RLIMIT_FSIZE: i32 = 1;
pub const RLIMIT_DATA: i32 = 2;
pub const RLIMIT_STACK: i32 = 3;
pub const RLIMIT_CORE: i32 = 4;
pub const RLIMIT_NPROC: i32 = 6;
pub const RLIMIT_NOFILE: i32 = 7;
pub const RLIMIT_AS: i32 = 9;
pub const RLIM_INFINITY: i32 = -1;
pub const PRIO_PROCESS: i32 = 0;
pub const PRIO_PGRP: i32 = 1;
pub const PRIO_USER: i32 = 2;
pub const WCONTINUED: i32 = 8;
pub const PROT_NONE: i32 = 0;
pub const PROT_READ: i32 = 1;
pub const PROT_WRITE: i32 = 2;
pub const PROT_EXEC: i32 = 4;
pub const MAP_SHARED: i32 = 1;
pub const MAP_PRIVATE: i32 = 2;
pub const MAP_FIXED: i32 = 16;
pub const MAP_NORESERVE: i32 = 16384;
pub const MAP_FIXED_NOREPLACE: i32 = 1048576;
pub const MAP_JIT: i32 = 2097152;
pub const MAP_ANON: i32 = 32;
pub const MAP_ANONYMOUS: i32 = 32;
pub const MADV_NORMAL: i32 = 0;
pub const MADV_RANDOM: i32 = 1;
pub const MADV_SEQUENTIAL: i32 = 2;
pub const MADV_WILLNEED: i32 = 3;
pub const MADV_DONTNEED: i32 = 4;
pub const MADV_FREE: i32 = 8;
pub const POSIX_MADV_NORMAL: i32 = 0;
pub const POSIX_MADV_RANDOM: i32 = 1;
pub const POSIX_MADV_SEQUENTIAL: i32 = 2;
pub const POSIX_MADV_WILLNEED: i32 = 3;
pub const POSIX_MADV_DONTNEED: i32 = 4;
pub const MFD_CLOEXEC: i32 = 1;
pub const MFD_ALLOW_SEALING: i32 = 2;
pub const ST_RDONLY: i32 = 1;
pub const ST_NOSUID: i32 = 2;
pub const TIOCSWINSZ: c_ulong = 21524 as c_ulong;
pub const TIOCGWINSZ: c_ulong = 21523 as c_ulong;
pub const TIOCSCTTY: c_ulong = 21518 as c_ulong;
pub const FIONREAD: c_ulong = 21531 as c_ulong;
pub const FIONBIO: c_ulong = 21537 as c_ulong;
pub const TIOCGPGRP: c_ulong = 21519 as c_ulong;
pub const TIOCSPGRP: c_ulong = 21520 as c_ulong;
pub const INADDR_ANY: i32 = 0;
pub const INADDR_LOOPBACK: i32 = 2130706433;
pub const INADDR_NONE: i64 = 4294967295;
pub const IPPROTO_IP: i32 = 0;
pub const IPPROTO_TCP: i32 = 6;
pub const IPPROTO_UDP: i32 = 17;
pub const IP_TOS: i32 = 1;
pub const IP_TTL: i32 = 2;
pub const IP_RECVTOS: i32 = 13;
pub const IP_MULTICAST_IF: i32 = 32;
pub const IP_MULTICAST_TTL: i32 = 33;
pub const IP_MULTICAST_LOOP: i32 = 34;
pub const IP_ADD_MEMBERSHIP: i32 = 35;
pub const IP_DROP_MEMBERSHIP: i32 = 36;
pub const IPV6_UNICAST_HOPS: i32 = 16;
pub const IPV6_MULTICAST_IF: i32 = 17;
pub const IPV6_MULTICAST_HOPS: i32 = 18;
pub const IPV6_MULTICAST_LOOP: i32 = 19;
pub const IPV6_JOIN_GROUP: i32 = 20;
pub const IPV6_LEAVE_GROUP: i32 = 21;
pub const IPV6_V6ONLY: i32 = 26;
pub const IPV6_TCLASS: i32 = 67;
pub const INET_ADDRSTRLEN: i32 = 16;
pub const INET6_ADDRSTRLEN: i32 = 46;
pub const IPPORT_RESERVED: i32 = 1024;
pub const IPPROTO_IPV6: i32 = 41;
pub const LUME_ACTIVITY_VERSION: i32 = 1;
pub const LUME_ACTIVITY_CPUS: i32 = 4;
pub const LUME_ACTIVITY_READ: i32 = 256;
pub const LUME_ACTIVITY_START: i32 = 257;
pub const LUME_ACTIVITY_STOP: i32 = 258;
pub const LUME_GPU_VERSION: i32 = 1;
pub const LUME_GPU_RECTS: i32 = 64;
pub const LUME_GPU_SURFACE_BYTES: i32 = 33554432;
pub const LUME_GRAPHICS_VERSION: i32 = 1;
pub const LUME_GRAPHICS_XRGB8888: i32 = 1;
pub const LUME_HARDWARE_VERSION: i32 = 1;
pub const LUME_HARDWARE_NAME: i32 = 64;
pub const LUME_HARDWARE_PATH: i32 = 160;
pub const LUME_HARDWARE_IDS: i32 = 4;
pub const LUME_HARDWARE_ID_BYTES: i32 = 96;
pub const LUME_HARDWARE_RESOURCES: i32 = 16;
pub const LUME_MODULE_ABI_VERSION: i32 = 1;
pub const LUME_SPAWN_VERSION: i32 = 1;
pub const LUME_SPAWN_OPTIONS_VERSION: i32 = 2;
pub const LUME_SPAWN_ACTION_MAX: i32 = 32;
pub const LUME_PTHREAD_COMPAT_STACK_DEFAULT: i32 = 131072;
pub const LUME_PTHREAD_COMPAT_STACK_MAX: i32 = 786432;
pub const LUME_PTHREAD_STACK_DEFAULT: i32 = 2097152;
pub const LUME_PTHREAD_STACK_MAX: i32 = 8388608;
pub const LUME_TTY_VERSION: i32 = 1;
pub const LUME_TTY_CC: i32 = 32;
pub const LUME_ABI_SVC: i32 = 19541;
pub const LUME_ARG_MAX: i32 = 256;
pub const LUME_ARG_BYTES: i32 = 16384;
pub const LUME_ENV_MAX: i32 = 64;
pub const LUME_ENV_BYTES: i32 = 8192;
pub const LUME_PATH_MAX: i32 = 128;
pub const LUME_NAME_MAX: i32 = 47;
pub const LUME_FD_MAX: i32 = 32;
pub const LUME_FILE_IO_MAX: i32 = 65536;
pub const LUME_NET_ROWS: i32 = 32;
pub const LUME_NET_TOOLS_ENOTSUP: i32 = 95;
pub const LUME_NET_IPV4_ADDRESS: i32 = 1;
pub const LUME_NET_IPV4_MASK: i32 = 2;
pub const LUME_NET_IPV4_GATEWAY: i32 = 4;
pub const LUME_NET_CONTROL_VERSION: i32 = 1;
pub const LUME_NET_INTERFACES: i32 = 4;
pub const LUME_NET_SCAN_MAX: i32 = 32;
pub const LUME_NET_EVENTS_MAX: i32 = 16;
pub const LUME_NET_STATS_VERSION: i32 = 1;
pub const LUME_BRK_EXTENDED: i32 = 1;
pub const LUME_CREDENTIAL_VERSION: i32 = 1;
pub const LUME_GROUP_MAX: i32 = 8;
pub const LUME_ID_MAX: i32 = 65534;
pub const LUME_FILE_IO_VERSION: i32 = 1;
pub const LUME_FILE_IO_VECTOR_MAX: i32 = 1024;
pub const LUME_FILE_LOCK_VERSION: i32 = 1;
pub const LUME_FILE_LOCK_LIMIT: i32 = 1024;
pub const LUME_FILE_MODE_VERSION: i32 = 1;
pub const LUME_IO_VERSION: i32 = 1;
pub const LUME_IO_BYTES: i32 = 4096;
pub const LUME_BOOT_CAP_RESTART_GUARD: i32 = 1;
pub const LUME_BOOT_CAP_TRIAL: i32 = 2;
pub const LUME_BOOT_CAP_PARTITION: i32 = 4;
pub const LUME_BOOT_STATE_TRIAL: i32 = 1;
pub const LUME_VM_VERSION: i32 = 1;
pub const LUME_VM_PAGE_BYTES: i32 = 4096;
pub const LUME_VM_NAME_BYTES: i32 = 128;
pub const LUME_PROT_NONE: i32 = 0;
pub const LUME_PROT_READ: i32 = 1;
pub const LUME_PROT_WRITE: i32 = 2;
pub const LUME_PROT_EXEC: i32 = 4;
pub const LUME_MAP_SHARED: i32 = 1;
pub const LUME_MAP_PRIVATE: i32 = 2;
pub const LUME_MAP_FIXED: i32 = 16;
pub const LUME_MAP_ANONYMOUS: i32 = 32;
pub const LUME_MAP_NORESERVE: i32 = 16384;
pub const LUME_MAP_FIXED_NOREPLACE: i32 = 1048576;
pub const LUME_MAP_JIT: i32 = 2097152;
pub const LUME_MADV_NORMAL: i32 = 0;
pub const LUME_MADV_RANDOM: i32 = 1;
pub const LUME_MADV_SEQUENTIAL: i32 = 2;
pub const LUME_MADV_WILLNEED: i32 = 3;
pub const LUME_MADV_DONTNEED: i32 = 4;
pub const LUME_MADV_FREE: i32 = 8;
pub const LUME_MEMFD_CLOEXEC: i32 = 1;
pub const LUME_MEMFD_ALLOW_SEALING: i32 = 2;
pub const LUME_SEAL_SEAL: i32 = 1;
pub const LUME_SEAL_SHRINK: i32 = 2;
pub const LUME_SEAL_GROW: i32 = 4;
pub const LUME_SEAL_WRITE: i32 = 8;
pub const LUME_SEAL_FUTURE_WRITE: i32 = 16;
pub const LUME_FEATURE_VERSION: i32 = 1;
pub const LUME_PROCESS_STOPPED: i32 = 6;
pub const LUME_REALTIME_VERSION: i32 = 1;
pub const LUME_SANDBOX_VERSION: i32 = 1;
pub const LUME_SANDBOX_DIRECTORY_LIMIT: i32 = 128;
pub const LUME_SERVICE_BYTES: i32 = 512;
pub const LUME_SERVICE_VERSION: i32 = 1;
pub const LUME_SERVICE_SIGNAL_EPOCH: i32 = 1;
pub const LUME_SIGNAL_VERSION: i32 = 1;
pub const LUME_SIGNAL_COUNT: i32 = 32;
pub const LUME_SOCKET_VERSION: i32 = 1;
pub const LUME_SOCKET_DATAGRAM_MAX: i32 = 65507;
pub const LUME_SOCKET_DATAGRAM_PEEK: i32 = 1;
pub const LUME_SOCKET_DATAGRAM_TRUNCATED: i32 = 2;
pub const LUME_SOCKET_DATAGRAM_TOS: i32 = 4;
pub const LUME_SOCKET_DATAGRAM_CONNECTED: i32 = 8;
pub const LUME_SOCKET_ERROR_PENDING: i32 = 1;
pub const LUME_SOCKET_RIGHTS_MAX: i32 = 128;
pub const LUME_SOCKET_MESSAGE_VERSION: i32 = 1;
pub const LUME_SOCKET_MESSAGE_CLOEXEC: i32 = 1;
pub const LUME_SOCKET_MESSAGE_PEEK: i32 = 2;
pub const LUME_SOCKET_MESSAGE_CTRUNC: i32 = 1;
pub const LUME_SOCKET_MESSAGE_TRUNCATED: i32 = 2;
pub const LUME_SUPERVISION_VERSION: i32 = 1;
pub const LUME_SUPERVISION_COUNT: i32 = 8;
pub const LUME_SUPERVISION_NAME: i32 = 24;
pub const LUME_SUPERVISION_SIGNAL_EPOCH: i32 = 1;
pub const TCP_NODELAY: i32 = 1;
pub const TCP_MAXSEG: i32 = 2;
pub const TCP_KEEPIDLE: i32 = 4;
pub const TCP_KEEPINTVL: i32 = 5;
pub const TCP_KEEPCNT: i32 = 6;
pub const _PATH_PASSWD: &[u8; 12] = b"/etc/passwd\0";
pub const _PASSWORD_LEN: i32 = 128;
pub type __int8_t = ::core::ffi::c_schar;
pub type __uint8_t = ::core::ffi::c_uchar;
pub type __int16_t = ::core::ffi::c_short;
pub type __uint16_t = ::core::ffi::c_ushort;
pub type __int32_t = ::core::ffi::c_int;
pub type __uint32_t = ::core::ffi::c_uint;
pub type __int64_t = ::core::ffi::c_longlong;
pub type __uint64_t = ::core::ffi::c_ulonglong;
pub type __int_least8_t = ::core::ffi::c_schar;
pub type __uint_least8_t = ::core::ffi::c_uchar;
pub type __int_least16_t = ::core::ffi::c_short;
pub type __uint_least16_t = ::core::ffi::c_ushort;
pub type __int_least32_t = ::core::ffi::c_int;
pub type __uint_least32_t = ::core::ffi::c_uint;
pub type __int_least64_t = ::core::ffi::c_longlong;
pub type __uint_least64_t = ::core::ffi::c_ulonglong;
pub type __intmax_t = ::core::ffi::c_longlong;
pub type __uintmax_t = ::core::ffi::c_ulonglong;
pub type __intptr_t = ::core::ffi::c_int;
pub type __uintptr_t = ::core::ffi::c_uint;
pub type intmax_t = __intmax_t;
pub type uintmax_t = __uintmax_t;
pub type int_least8_t = __int_least8_t;
pub type uint_least8_t = __uint_least8_t;
pub type int_least16_t = __int_least16_t;
pub type uint_least16_t = __uint_least16_t;
pub type int_least32_t = __int_least32_t;
pub type uint_least32_t = __uint_least32_t;
pub type int_least64_t = __int_least64_t;
pub type uint_least64_t = __uint_least64_t;
pub type int_fast8_t = ::core::ffi::c_schar;
pub type uint_fast8_t = ::core::ffi::c_uchar;
pub type int_fast16_t = ::core::ffi::c_short;
pub type uint_fast16_t = ::core::ffi::c_ushort;
pub type int_fast32_t = ::core::ffi::c_int;
pub type uint_fast32_t = ::core::ffi::c_uint;
pub type int_fast64_t = ::core::ffi::c_longlong;
pub type uint_fast64_t = ::core::ffi::c_ulonglong;
pub type wchar_t = ::core::ffi::c_int;
#[repr(C)]
#[repr(align(8))]
#[derive(Debug, Copy, Clone)]
pub struct max_align_t {
    pub __clang_max_align_nonce1: ::core::ffi::c_longlong,
    pub __clang_max_align_nonce2: f64,
}
pub type __gnuc_va_list = __builtin_va_list;
#[repr(C)]
#[derive(Debug)]
pub struct __lock {
    _unused: [u8; 0],
}
pub type _LOCK_T = *mut __lock;
unsafe extern "C" {
    pub fn __retarget_lock_init(lock: *mut _LOCK_T);
}
unsafe extern "C" {
    pub fn __retarget_lock_init_recursive(lock: *mut _LOCK_T);
}
unsafe extern "C" {
    pub fn __retarget_lock_close(lock: _LOCK_T);
}
unsafe extern "C" {
    pub fn __retarget_lock_close_recursive(lock: _LOCK_T);
}
unsafe extern "C" {
    pub fn __retarget_lock_acquire(lock: _LOCK_T);
}
unsafe extern "C" {
    pub fn __retarget_lock_acquire_recursive(lock: _LOCK_T);
}
unsafe extern "C" {
    pub fn __retarget_lock_release(lock: _LOCK_T);
}
unsafe extern "C" {
    pub fn __retarget_lock_release_recursive(lock: _LOCK_T);
}
unsafe extern "C" {
    pub static mut __lock___libc_recursive_mutex: __lock;
}
pub type wint_t = ::core::ffi::c_int;
pub type _off_t = ::core::ffi::c_longlong;
pub type _fpos_t = ::core::ffi::c_longlong;
pub type __blkcnt_t = ::core::ffi::c_long;
pub type __blkcnt64_t = __int64_t;
pub type __blksize_t = ::core::ffi::c_long;
pub type __fsblkcnt_t = __uint64_t;
pub type __fsfilcnt_t = __uint32_t;
pub type __pid_t = ::core::ffi::c_int;
pub type __dev_t = ::core::ffi::c_short;
pub type __uid_t = ::core::ffi::c_ushort;
pub type __gid_t = ::core::ffi::c_ushort;
pub type __id_t = __uint32_t;
pub type __ino_t = ::core::ffi::c_ushort;
pub type __ino64_t = __uint64_t;
pub type __mode_t = __uint32_t;
pub type _off64_t = ::core::ffi::c_longlong;
pub type __off_t = _off_t;
pub type __off64_t = __uint64_t;
pub type __loff_t = _off64_t;
pub type __key_t = ::core::ffi::c_long;
pub type _fpos64_t = _off64_t;
pub type __size_t = ::core::ffi::c_uint;
pub type _ssize_t = ::core::ffi::c_int;
pub type __ssize_t = _ssize_t;
#[repr(C)]
#[derive(Copy, Clone)]
pub struct _mbstate_t {
    pub __count: ::core::ffi::c_int,
    pub __value: _mbstate_t__bindgen_ty_1,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union _mbstate_t__bindgen_ty_1 {
    pub __wch: wint_t,
    pub __wchb: [::core::ffi::c_uchar; 4usize],
    pub __ucs: __uint32_t,
    pub __ucs2: __uint16_t,
}
#[repr(C)]
#[derive(Debug)]
pub struct __iconv_t {
    _unused: [u8; 0],
}
pub type _iconv_t = *mut __iconv_t;
pub type __clock_t = ::core::ffi::c_ulong;
pub type __time_t = __int_least64_t;
pub type __clockid_t = ::core::ffi::c_ulong;
pub type __daddr_t = ::core::ffi::c_long;
pub type __timer_t = ::core::ffi::c_ulong;
pub type __sa_family_t = __uint8_t;
pub type __socklen_t = __uint32_t;
pub type __nl_item = __int32_t;
pub type __nlink_t = ::core::ffi::c_ushort;
pub type __suseconds_t = ::core::ffi::c_long;
pub type __useconds_t = ::core::ffi::c_ulong;
pub type __wctype_t = __uint16_t;
pub type __ungetc_t = __uint16_t;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct __file {
    pub unget: __ungetc_t,
    pub flags: __uint8_t,
    pub put: ::core::option::Option<
        unsafe extern "C" fn(arg1: ::core::ffi::c_char, arg2: *mut __file) -> ::core::ffi::c_int,
    >,
    pub get: ::core::option::Option<unsafe extern "C" fn(arg1: *mut __file) -> ::core::ffi::c_int>,
    pub flush:
        ::core::option::Option<unsafe extern "C" fn(arg1: *mut __file) -> ::core::ffi::c_int>,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct __file_close {
    pub file: __file,
    pub close:
        ::core::option::Option<unsafe extern "C" fn(arg1: *mut __file) -> ::core::ffi::c_int>,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct __file_ext {
    pub cfile: __file_close,
    pub seek: ::core::option::Option<
        unsafe extern "C" fn(
            arg1: *mut __file,
            offset: __off_t,
            whence: ::core::ffi::c_int,
        ) -> __off_t,
    >,
    pub setvbuf: ::core::option::Option<
        unsafe extern "C" fn(
            arg1: *mut __file,
            buf: *mut ::core::ffi::c_char,
            mode: ::core::ffi::c_int,
            size: usize,
        ) -> ::core::ffi::c_int,
    >,
}
pub type __FILE = __file;
pub type FILE = __FILE;
unsafe extern "C" {
    pub static stdin: *mut FILE;
}
unsafe extern "C" {
    pub static stdout: *mut FILE;
}
unsafe extern "C" {
    pub static stderr: *mut FILE;
}
unsafe extern "C" {
    pub fn fdevopen(
        __put: ::core::option::Option<
            unsafe extern "C" fn(arg1: ::core::ffi::c_char, arg2: *mut FILE) -> ::core::ffi::c_int,
        >,
        __get: ::core::option::Option<unsafe extern "C" fn(arg1: *mut FILE) -> ::core::ffi::c_int>,
        __flush: ::core::option::Option<
            unsafe extern "C" fn(arg1: *mut FILE) -> ::core::ffi::c_int,
        >,
    ) -> *mut FILE;
}
unsafe extern "C" {
    pub fn fclose(__stream: *mut FILE) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fflush(stream: *mut FILE) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fputc(__c: ::core::ffi::c_int, __stream: *mut FILE) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn putc(__c: ::core::ffi::c_int, __stream: *mut FILE) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn putchar(__c: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn printf(__fmt: *const ::core::ffi::c_char, ...) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fprintf(
        __stream: *mut FILE,
        __fmt: *const ::core::ffi::c_char,
        ...
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn vprintf(__fmt: *const ::core::ffi::c_char, __ap: __gnuc_va_list) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn vfprintf(
        __stream: *mut FILE,
        __fmt: *const ::core::ffi::c_char,
        __ap: __gnuc_va_list,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn sprintf(
        __s: *mut ::core::ffi::c_char,
        __fmt: *const ::core::ffi::c_char,
        ...
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn snprintf(
        __s: *mut ::core::ffi::c_char,
        __n: usize,
        __fmt: *const ::core::ffi::c_char,
        ...
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn vsprintf(
        __s: *mut ::core::ffi::c_char,
        __fmt: *const ::core::ffi::c_char,
        ap: __gnuc_va_list,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn vsnprintf(
        __s: *mut ::core::ffi::c_char,
        __n: usize,
        __fmt: *const ::core::ffi::c_char,
        ap: __gnuc_va_list,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn asprintf(
        strp: *mut *mut ::core::ffi::c_char,
        fmt: *const ::core::ffi::c_char,
        ...
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn asnprintf(
        str_: *mut ::core::ffi::c_char,
        lenp: *mut usize,
        fmt: *const ::core::ffi::c_char,
        ...
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn vasprintf(
        strp: *mut *mut ::core::ffi::c_char,
        fmt: *const ::core::ffi::c_char,
        ap: __gnuc_va_list,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn vasnprintf(
        str_: *mut ::core::ffi::c_char,
        lenp: *mut usize,
        fmt: *const ::core::ffi::c_char,
        ap: __gnuc_va_list,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn fputs(__str: *const ::core::ffi::c_char, __stream: *mut FILE) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn puts(__str: *const ::core::ffi::c_char) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fwrite(
        __ptr: *const ::core::ffi::c_void,
        __size: usize,
        __nmemb: usize,
        __stream: *mut FILE,
    ) -> usize;
}
unsafe extern "C" {
    pub fn fgetc(__stream: *mut FILE) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn getc(__stream: *mut FILE) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn getchar() -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn ungetc(__c: ::core::ffi::c_int, __stream: *mut FILE) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn scanf(__fmt: *const ::core::ffi::c_char, ...) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fscanf(
        __stream: *mut FILE,
        __fmt: *const ::core::ffi::c_char,
        ...
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn vscanf(__fmt: *const ::core::ffi::c_char, __ap: __gnuc_va_list) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn vfscanf(
        __stream: *mut FILE,
        __fmt: *const ::core::ffi::c_char,
        __ap: __gnuc_va_list,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn sscanf(
        __buf: *const ::core::ffi::c_char,
        __fmt: *const ::core::ffi::c_char,
        ...
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn vsscanf(
        __buf: *const ::core::ffi::c_char,
        __fmt: *const ::core::ffi::c_char,
        ap: __gnuc_va_list,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fgets(
        __str: *mut ::core::ffi::c_char,
        __size: ::core::ffi::c_int,
        __stream: *mut FILE,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn fread(
        __ptr: *mut ::core::ffi::c_void,
        __size: usize,
        __nmemb: usize,
        __stream: *mut FILE,
    ) -> usize;
}
unsafe extern "C" {
    pub fn clearerr(__stream: *mut FILE);
}
unsafe extern "C" {
    pub fn ferror(__stream: *mut FILE) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn feof(__stream: *mut FILE) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn clearerr_unlocked(__stream: *mut FILE);
}
unsafe extern "C" {
    pub fn ferror_unlocked(__stream: *mut FILE) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn feof_unlocked(__stream: *mut FILE) -> ::core::ffi::c_int;
}
pub type fpos_t = _fpos_t;
pub type off_t = __off_t;
pub type off64_t = __off64_t;
pub type va_list = __gnuc_va_list;
unsafe extern "C" {
    pub fn dprintf(
        fd: ::core::ffi::c_int,
        fmt: *const ::core::ffi::c_char,
        ...
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn vdprintf(
        fd: ::core::ffi::c_int,
        fmt: *const ::core::ffi::c_char,
        ap: va_list,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fgetpos(stream: *mut FILE, pos: *mut fpos_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fopen(path: *const ::core::ffi::c_char, mode: *const ::core::ffi::c_char) -> *mut FILE;
}
unsafe extern "C" {
    pub fn freopen(
        path: *const ::core::ffi::c_char,
        mode: *const ::core::ffi::c_char,
        stream: *mut FILE,
    ) -> *mut FILE;
}
unsafe extern "C" {
    pub fn fdopen(arg1: ::core::ffi::c_int, arg2: *const ::core::ffi::c_char) -> *mut FILE;
}
unsafe extern "C" {
    pub fn fmemopen(
        buf: *mut ::core::ffi::c_void,
        size: usize,
        mode: *const ::core::ffi::c_char,
    ) -> *mut FILE;
}
unsafe extern "C" {
    pub fn fseek(
        stream: *mut FILE,
        offset: ::core::ffi::c_long,
        whence: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fseeko(
        stream: *mut FILE,
        offset: __off_t,
        whence: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fsetpos(stream: *mut FILE, pos: *const fpos_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn ftell(stream: *mut FILE) -> ::core::ffi::c_long;
}
unsafe extern "C" {
    pub fn ftello(stream: *mut FILE) -> __off_t;
}
unsafe extern "C" {
    pub fn fileno(stream: *mut FILE) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn perror(s: *const ::core::ffi::c_char);
}
unsafe extern "C" {
    pub fn remove(pathname: *const ::core::ffi::c_char) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn rename(
        oldpath: *const ::core::ffi::c_char,
        newpath: *const ::core::ffi::c_char,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn rewind(stream: *mut FILE);
}
unsafe extern "C" {
    pub fn setbuf(stream: *mut FILE, buf: *mut ::core::ffi::c_char);
}
unsafe extern "C" {
    pub fn setbuffer(stream: *mut FILE, buf: *mut ::core::ffi::c_char, size: usize);
}
unsafe extern "C" {
    pub fn setlinebuf(stream: *mut FILE);
}
unsafe extern "C" {
    pub fn setvbuf(
        stream: *mut FILE,
        buf: *mut ::core::ffi::c_char,
        mode: ::core::ffi::c_int,
        size: usize,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn tmpfile() -> *mut FILE;
}
unsafe extern "C" {
    pub fn tmpnam(s: *mut ::core::ffi::c_char) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn getline(
        lineptr: *mut *mut ::core::ffi::c_char,
        n: *mut usize,
        stream: *mut FILE,
    ) -> _ssize_t;
}
unsafe extern "C" {
    pub fn getdelim(
        lineptr: *mut *mut ::core::ffi::c_char,
        n: *mut usize,
        delim: ::core::ffi::c_int,
        stream: *mut FILE,
    ) -> _ssize_t;
}
unsafe extern "C" {
    pub fn funopen(
        cookie: *const ::core::ffi::c_void,
        readfn: ::core::option::Option<
            unsafe extern "C" fn(
                cookie: *mut ::core::ffi::c_void,
                buf: *mut ::core::ffi::c_void,
                n: usize,
            ) -> _ssize_t,
        >,
        writefn: ::core::option::Option<
            unsafe extern "C" fn(
                cookie: *mut ::core::ffi::c_void,
                buf: *const ::core::ffi::c_void,
                n: usize,
            ) -> _ssize_t,
        >,
        seekfn: ::core::option::Option<
            unsafe extern "C" fn(
                cookie: *mut ::core::ffi::c_void,
                off: __off_t,
                whence: ::core::ffi::c_int,
            ) -> __off_t,
        >,
        closefn: ::core::option::Option<
            unsafe extern "C" fn(cookie: *mut ::core::ffi::c_void) -> ::core::ffi::c_int,
        >,
    ) -> *mut FILE;
}
unsafe extern "C" {
    pub fn getc_unlocked(arg1: *mut FILE) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn getchar_unlocked() -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn flockfile(arg1: *mut FILE);
}
unsafe extern "C" {
    pub fn ftrylockfile(arg1: *mut FILE) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn funlockfile(arg1: *mut FILE);
}
unsafe extern "C" {
    pub fn putc_unlocked(arg1: ::core::ffi::c_int, arg2: *mut FILE) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn putchar_unlocked(arg1: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn popen(arg1: *const ::core::ffi::c_char, arg2: *const ::core::ffi::c_char) -> *mut FILE;
}
unsafe extern "C" {
    pub fn pclose(arg1: *mut FILE) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn open_memstream(arg1: *mut *mut ::core::ffi::c_char, arg2: *mut usize) -> *mut FILE;
}
pub type locale_t = ::core::ffi::c_int;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct div_t {
    pub quot: ::core::ffi::c_int,
    pub rem: ::core::ffi::c_int,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ldiv_t {
    pub quot: ::core::ffi::c_long,
    pub rem: ::core::ffi::c_long,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lldiv_t {
    pub quot: ::core::ffi::c_longlong,
    pub rem: ::core::ffi::c_longlong,
}
pub type __compar_fn_t = ::core::option::Option<
    unsafe extern "C" fn(
        arg1: *const ::core::ffi::c_void,
        arg2: *const ::core::ffi::c_void,
    ) -> ::core::ffi::c_int,
>;
unsafe extern "C" {
    pub fn __locale_mb_cur_max() -> usize;
}
unsafe extern "C" {
    pub fn free(arg1: *mut ::core::ffi::c_void);
}
unsafe extern "C" {
    pub fn _Exit(__status: ::core::ffi::c_int) -> !;
}
unsafe extern "C" {
    pub fn a64l(__input: *const ::core::ffi::c_char) -> ::core::ffi::c_long;
}
unsafe extern "C" {
    pub fn abort() -> !;
}
unsafe extern "C" {
    pub fn abs(arg1: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn aligned_alloc(arg1: usize, arg2: usize) -> *mut ::core::ffi::c_void;
}
unsafe extern "C" {
    pub fn arc4random() -> __uint32_t;
}
unsafe extern "C" {
    pub fn arc4random_uniform(arg1: __uint32_t) -> __uint32_t;
}
unsafe extern "C" {
    pub fn arc4random_buf(arg1: *mut ::core::ffi::c_void, arg2: usize);
}
unsafe extern "C" {
    pub fn at_quick_exit(
        arg1: ::core::option::Option<unsafe extern "C" fn()>,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn atexit(__func: ::core::option::Option<unsafe extern "C" fn()>) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn atof(__nptr: *const ::core::ffi::c_char) -> f64;
}
unsafe extern "C" {
    pub fn atoff(__nptr: *const ::core::ffi::c_char) -> f32;
}
unsafe extern "C" {
    pub fn atoi(__nptr: *const ::core::ffi::c_char) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn atol(__nptr: *const ::core::ffi::c_char) -> ::core::ffi::c_long;
}
unsafe extern "C" {
    pub fn atoll(__nptr: *const ::core::ffi::c_char) -> ::core::ffi::c_longlong;
}
unsafe extern "C" {
    pub fn bsearch(
        __key: *const ::core::ffi::c_void,
        __base: *const ::core::ffi::c_void,
        __nmemb: usize,
        __size: usize,
        _compar: __compar_fn_t,
    ) -> *mut ::core::ffi::c_void;
}
unsafe extern "C" {
    pub fn calloc(arg1: usize, arg2: usize) -> *mut ::core::ffi::c_void;
}
unsafe extern "C" {
    pub fn div(__numer: ::core::ffi::c_int, __denom: ::core::ffi::c_int) -> div_t;
}
#[repr(C)]
#[derive(Debug)]
pub struct _rand48 {
    _unused: [u8; 0],
}
unsafe extern "C" {
    pub fn drand48() -> f64;
}
unsafe extern "C" {
    pub fn _drand48_r(arg1: *mut _rand48) -> f64;
}
unsafe extern "C" {
    pub fn erand48(arg1: *mut ::core::ffi::c_ushort) -> f64;
}
unsafe extern "C" {
    pub fn _erand48_r(arg1: *mut _rand48, arg2: *mut ::core::ffi::c_ushort) -> f64;
}
unsafe extern "C" {
    pub fn exit(__status: ::core::ffi::c_int) -> !;
}
unsafe extern "C" {
    pub fn getenv(__string: *const ::core::ffi::c_char) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn secure_getenv(__string: *const ::core::ffi::c_char) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn _findenv(
        arg1: *const ::core::ffi::c_char,
        arg2: *mut ::core::ffi::c_int,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub static mut suboptarg: *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn getsubopt(
        arg1: *mut *mut ::core::ffi::c_char,
        arg2: *const *mut ::core::ffi::c_char,
        arg3: *mut *mut ::core::ffi::c_char,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn grantpt(fd: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn initstate(
        arg1: ::core::ffi::c_uint,
        arg2: *mut ::core::ffi::c_char,
        arg3: usize,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn jrand48(arg1: *mut ::core::ffi::c_ushort) -> ::core::ffi::c_long;
}
unsafe extern "C" {
    pub fn _jrand48_r(arg1: *mut _rand48, arg2: *mut ::core::ffi::c_ushort) -> ::core::ffi::c_long;
}
unsafe extern "C" {
    pub fn labs(arg1: ::core::ffi::c_long) -> ::core::ffi::c_long;
}
unsafe extern "C" {
    pub fn lcong48(arg1: *mut ::core::ffi::c_ushort);
}
unsafe extern "C" {
    pub fn _lcong48_r(arg1: *mut _rand48, arg2: *mut ::core::ffi::c_ushort);
}
unsafe extern "C" {
    pub fn ldiv(__numer: ::core::ffi::c_long, __denom: ::core::ffi::c_long) -> ldiv_t;
}
unsafe extern "C" {
    pub fn llabs(arg1: ::core::ffi::c_longlong) -> ::core::ffi::c_longlong;
}
unsafe extern "C" {
    pub fn lldiv(__numer: ::core::ffi::c_longlong, __denom: ::core::ffi::c_longlong) -> lldiv_t;
}
unsafe extern "C" {
    pub fn lrand48() -> ::core::ffi::c_long;
}
unsafe extern "C" {
    pub fn _lrand48_r(arg1: *mut _rand48) -> ::core::ffi::c_long;
}
unsafe extern "C" {
    pub fn malloc(arg1: usize) -> *mut ::core::ffi::c_void;
}
unsafe extern "C" {
    pub fn mblen(arg1: *const ::core::ffi::c_char, arg2: usize) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn mbstowcs(arg1: *mut wchar_t, arg2: *const ::core::ffi::c_char, arg3: usize) -> usize;
}
unsafe extern "C" {
    pub fn mbtowc(
        arg1: *mut wchar_t,
        arg2: *const ::core::ffi::c_char,
        arg3: usize,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn mkdtemp(arg1: *mut ::core::ffi::c_char) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn mkostemp(arg1: *mut ::core::ffi::c_char, arg2: ::core::ffi::c_int)
        -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn mkostemps(
        arg1: *mut ::core::ffi::c_char,
        arg2: ::core::ffi::c_int,
        arg3: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn mkstemp(arg1: *mut ::core::ffi::c_char) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn mkstemps(arg1: *mut ::core::ffi::c_char, arg2: ::core::ffi::c_int)
        -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn mktemp(arg1: *mut ::core::ffi::c_char) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn mrand48() -> ::core::ffi::c_long;
}
unsafe extern "C" {
    pub fn _mrand48_r(arg1: *mut _rand48) -> ::core::ffi::c_long;
}
unsafe extern "C" {
    pub fn nrand48(arg1: *mut ::core::ffi::c_ushort) -> ::core::ffi::c_long;
}
unsafe extern "C" {
    pub fn _nrand48_r(arg1: *mut _rand48, arg2: *mut ::core::ffi::c_ushort) -> ::core::ffi::c_long;
}
unsafe extern "C" {
    pub fn posix_memalign(
        arg1: *mut *mut ::core::ffi::c_void,
        arg2: usize,
        arg3: usize,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn putenv(__string: *mut ::core::ffi::c_char) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn quick_exit(arg1: ::core::ffi::c_int) -> !;
}
unsafe extern "C" {
    pub fn qsort(
        __base: *mut ::core::ffi::c_void,
        __nmemb: usize,
        __size: usize,
        _compar: __compar_fn_t,
    );
}
unsafe extern "C" {
    pub fn qsort_r(
        __base: *mut ::core::ffi::c_void,
        __nmemb: usize,
        __size: usize,
        _compar: ::core::option::Option<
            unsafe extern "C" fn(
                arg1: *const ::core::ffi::c_void,
                arg2: *const ::core::ffi::c_void,
                arg3: *mut ::core::ffi::c_void,
            ) -> ::core::ffi::c_int,
        >,
        __thunk: *mut ::core::ffi::c_void,
    );
}
unsafe extern "C" {
    pub fn rand() -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn rand_r(__seed: *mut ::core::ffi::c_uint) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn random() -> ::core::ffi::c_long;
}
unsafe extern "C" {
    pub fn realloc(arg1: *mut ::core::ffi::c_void, arg2: usize) -> *mut ::core::ffi::c_void;
}
unsafe extern "C" {
    pub fn reallocarray(
        arg1: *mut ::core::ffi::c_void,
        arg2: usize,
        arg3: usize,
    ) -> *mut ::core::ffi::c_void;
}
unsafe extern "C" {
    pub fn reallocf(arg1: *mut ::core::ffi::c_void, arg2: usize) -> *mut ::core::ffi::c_void;
}
unsafe extern "C" {
    pub fn realpath(
        path: *const ::core::ffi::c_char,
        resolved_path: *mut ::core::ffi::c_char,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn rpmatch(response: *const ::core::ffi::c_char) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn seed48(arg1: *mut ::core::ffi::c_ushort) -> *mut ::core::ffi::c_ushort;
}
unsafe extern "C" {
    pub fn _seed48_r(
        arg1: *mut _rand48,
        arg2: *mut ::core::ffi::c_ushort,
    ) -> *mut ::core::ffi::c_ushort;
}
unsafe extern "C" {
    pub fn setenv(
        __string: *const ::core::ffi::c_char,
        __value: *const ::core::ffi::c_char,
        __overwrite: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn setkey(__key: *const ::core::ffi::c_char);
}
unsafe extern "C" {
    pub fn setstate(arg1: *mut ::core::ffi::c_char) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn srand(__seed: ::core::ffi::c_uint);
}
unsafe extern "C" {
    pub fn srand48(arg1: ::core::ffi::c_long);
}
unsafe extern "C" {
    pub fn _srand48_r(arg1: *mut _rand48, arg2: ::core::ffi::c_long);
}
unsafe extern "C" {
    pub fn srandom(arg1: ::core::ffi::c_uint);
}
unsafe extern "C" {
    pub fn strtod(__n: *const ::core::ffi::c_char, __end_PTR: *mut *mut ::core::ffi::c_char)
        -> f64;
}
unsafe extern "C" {
    pub fn strfromd(
        str_: *mut ::core::ffi::c_char,
        n: usize,
        format: *const ::core::ffi::c_char,
        fp: f64,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn strtof(__n: *const ::core::ffi::c_char, __end_PTR: *mut *mut ::core::ffi::c_char)
        -> f32;
}
unsafe extern "C" {
    pub fn strtold(
        __n: *const ::core::ffi::c_char,
        __end_PTR: *mut *mut ::core::ffi::c_char,
    ) -> f64;
}
unsafe extern "C" {
    pub fn strfromf(
        str_: *mut ::core::ffi::c_char,
        n: usize,
        format: *const ::core::ffi::c_char,
        fp: f32,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn strfroml(
        str_: *mut ::core::ffi::c_char,
        n: usize,
        format: *const ::core::ffi::c_char,
        fp: f64,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn strtol(
        __n: *const ::core::ffi::c_char,
        __end_PTR: *mut *mut ::core::ffi::c_char,
        __base: ::core::ffi::c_int,
    ) -> ::core::ffi::c_long;
}
unsafe extern "C" {
    pub fn strtoll(
        __n: *const ::core::ffi::c_char,
        __end_PTR: *mut *mut ::core::ffi::c_char,
        __base: ::core::ffi::c_int,
    ) -> ::core::ffi::c_longlong;
}
unsafe extern "C" {
    pub fn strtoul(
        __n: *const ::core::ffi::c_char,
        __end_PTR: *mut *mut ::core::ffi::c_char,
        __base: ::core::ffi::c_int,
    ) -> ::core::ffi::c_ulong;
}
unsafe extern "C" {
    pub fn strtoull(
        __n: *const ::core::ffi::c_char,
        __end_PTR: *mut *mut ::core::ffi::c_char,
        __base: ::core::ffi::c_int,
    ) -> ::core::ffi::c_ulonglong;
}
unsafe extern "C" {
    pub fn strtod_l(
        arg1: *const ::core::ffi::c_char,
        arg2: *mut *mut ::core::ffi::c_char,
        arg3: locale_t,
    ) -> f64;
}
unsafe extern "C" {
    pub fn strtof_l(
        arg1: *const ::core::ffi::c_char,
        arg2: *mut *mut ::core::ffi::c_char,
        arg3: locale_t,
    ) -> f32;
}
unsafe extern "C" {
    pub fn strtold_l(
        arg1: *const ::core::ffi::c_char,
        arg2: *mut *mut ::core::ffi::c_char,
        arg3: locale_t,
    ) -> f64;
}
unsafe extern "C" {
    pub fn strtol_l(
        arg1: *const ::core::ffi::c_char,
        arg2: *mut *mut ::core::ffi::c_char,
        arg3: ::core::ffi::c_int,
        arg4: locale_t,
    ) -> ::core::ffi::c_long;
}
unsafe extern "C" {
    pub fn strtoul_l(
        arg1: *const ::core::ffi::c_char,
        arg2: *mut *mut ::core::ffi::c_char,
        arg3: ::core::ffi::c_int,
        __loc: locale_t,
    ) -> ::core::ffi::c_ulong;
}
unsafe extern "C" {
    pub fn strtoll_l(
        arg1: *const ::core::ffi::c_char,
        arg2: *mut *mut ::core::ffi::c_char,
        arg3: ::core::ffi::c_int,
        arg4: locale_t,
    ) -> ::core::ffi::c_longlong;
}
unsafe extern "C" {
    pub fn strtoull_l(
        arg1: *const ::core::ffi::c_char,
        arg2: *mut *mut ::core::ffi::c_char,
        arg3: ::core::ffi::c_int,
        __loc: locale_t,
    ) -> ::core::ffi::c_ulonglong;
}
unsafe extern "C" {
    pub fn system(__string: *const ::core::ffi::c_char) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn unsetenv(__string: *const ::core::ffi::c_char) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn wcstombs(arg1: *mut ::core::ffi::c_char, arg2: *const wchar_t, arg3: usize) -> usize;
}
unsafe extern "C" {
    pub fn wctomb(arg1: *mut ::core::ffi::c_char, arg2: wchar_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn valloc(arg1: usize) -> *mut ::core::ffi::c_void;
}
unsafe extern "C" {
    pub fn l64a(__input: ::core::ffi::c_long) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn on_exit(
        __func: ::core::option::Option<
            unsafe extern "C" fn(arg1: ::core::ffi::c_int, arg2: *mut ::core::ffi::c_void),
        >,
        __arg: *mut ::core::ffi::c_void,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn ecvt(
        arg1: f64,
        arg2: ::core::ffi::c_int,
        arg3: *mut ::core::ffi::c_int,
        arg4: *mut ::core::ffi::c_int,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn ecvt_r(
        arg1: f64,
        arg2: ::core::ffi::c_int,
        arg3: *mut ::core::ffi::c_int,
        arg4: *mut ::core::ffi::c_int,
        arg5: *mut ::core::ffi::c_char,
        arg6: usize,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn ecvtf(
        arg1: f32,
        arg2: ::core::ffi::c_int,
        arg3: *mut ::core::ffi::c_int,
        arg4: *mut ::core::ffi::c_int,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn ecvtf_r(
        arg1: f32,
        arg2: ::core::ffi::c_int,
        arg3: *mut ::core::ffi::c_int,
        arg4: *mut ::core::ffi::c_int,
        arg5: *mut ::core::ffi::c_char,
        arg6: usize,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fcvt(
        arg1: f64,
        arg2: ::core::ffi::c_int,
        arg3: *mut ::core::ffi::c_int,
        arg4: *mut ::core::ffi::c_int,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn fcvt_r(
        arg1: f64,
        arg2: ::core::ffi::c_int,
        arg3: *mut ::core::ffi::c_int,
        arg4: *mut ::core::ffi::c_int,
        arg5: *mut ::core::ffi::c_char,
        arg6: usize,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fcvtf(
        arg1: f32,
        arg2: ::core::ffi::c_int,
        arg3: *mut ::core::ffi::c_int,
        arg4: *mut ::core::ffi::c_int,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn fcvtf_r(
        arg1: f32,
        arg2: ::core::ffi::c_int,
        arg3: *mut ::core::ffi::c_int,
        arg4: *mut ::core::ffi::c_int,
        arg5: *mut ::core::ffi::c_char,
        arg6: usize,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn ecvtl(
        arg1: f64,
        arg2: ::core::ffi::c_int,
        arg3: *mut ::core::ffi::c_int,
        arg4: *mut ::core::ffi::c_int,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn ecvtl_r(
        arg1: f64,
        arg2: ::core::ffi::c_int,
        arg3: *mut ::core::ffi::c_int,
        arg4: *mut ::core::ffi::c_int,
        arg5: *mut ::core::ffi::c_char,
        arg6: usize,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fcvtl(
        arg1: f64,
        arg2: ::core::ffi::c_int,
        arg3: *mut ::core::ffi::c_int,
        arg4: *mut ::core::ffi::c_int,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn fcvtl_r(
        arg1: f64,
        arg2: ::core::ffi::c_int,
        arg3: *mut ::core::ffi::c_int,
        arg4: *mut ::core::ffi::c_int,
        arg5: *mut ::core::ffi::c_char,
        arg6: usize,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn gcvt(
        arg1: f64,
        arg2: ::core::ffi::c_int,
        arg3: *mut ::core::ffi::c_char,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn gcvtf(
        arg1: f32,
        arg2: ::core::ffi::c_int,
        arg3: *mut ::core::ffi::c_char,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn gcvtl(
        arg1: f64,
        arg2: ::core::ffi::c_int,
        arg3: *mut ::core::ffi::c_char,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn __itoa(
        arg1: ::core::ffi::c_int,
        arg2: *mut ::core::ffi::c_char,
        arg3: ::core::ffi::c_int,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn __utoa(
        arg1: ::core::ffi::c_uint,
        arg2: *mut ::core::ffi::c_char,
        arg3: ::core::ffi::c_int,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn itoa(
        arg1: ::core::ffi::c_int,
        arg2: *mut ::core::ffi::c_char,
        arg3: ::core::ffi::c_int,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn utoa(
        arg1: ::core::ffi::c_uint,
        arg2: *mut ::core::ffi::c_char,
        arg3: ::core::ffi::c_int,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn cfree(arg1: *mut ::core::ffi::c_void);
}
unsafe extern "C" {
    pub fn __dtoa(
        arg1: f64,
        arg2: ::core::ffi::c_int,
        arg3: ::core::ffi::c_int,
        arg4: *mut ::core::ffi::c_int,
        arg5: *mut ::core::ffi::c_int,
        arg6: *mut *mut ::core::ffi::c_char,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn __ldtoa(
        arg1: f64,
        arg2: ::core::ffi::c_int,
        arg3: ::core::ffi::c_int,
        arg4: *mut ::core::ffi::c_int,
        arg5: *mut ::core::ffi::c_int,
        arg6: *mut *mut ::core::ffi::c_char,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn __eprintf(
        arg1: *const ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
        arg3: ::core::ffi::c_uint,
        arg4: *const ::core::ffi::c_char,
    );
}
unsafe extern "C" {
    pub fn getprogname() -> *const ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn bcmp(
        arg1: *const ::core::ffi::c_void,
        arg2: *const ::core::ffi::c_void,
        arg3: usize,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn bcopy(arg1: *const ::core::ffi::c_void, arg2: *mut ::core::ffi::c_void, arg3: usize);
}
unsafe extern "C" {
    pub fn bzero(arg1: *mut ::core::ffi::c_void, arg2: usize);
}
unsafe extern "C" {
    pub fn explicit_bzero(arg1: *mut ::core::ffi::c_void, arg2: usize);
}
unsafe extern "C" {
    pub fn ffs(arg1: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn ffsl(arg1: ::core::ffi::c_long) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn ffsll(arg1: ::core::ffi::c_longlong) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fls(arg1: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn flsl(arg1: ::core::ffi::c_long) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn flsll(arg1: ::core::ffi::c_longlong) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn index(
        arg1: *const ::core::ffi::c_char,
        arg2: ::core::ffi::c_int,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn rindex(
        arg1: *const ::core::ffi::c_char,
        arg2: ::core::ffi::c_int,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strcasecmp(
        arg1: *const ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn strncasecmp(
        arg1: *const ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
        arg3: usize,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn strcasecmp_l(
        arg1: *const ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
        arg3: locale_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn strncasecmp_l(
        arg1: *const ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
        arg3: usize,
        arg4: locale_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    #[link_name = "\u{1}__gnu_basename"]
    pub fn basename(arg1: *const ::core::ffi::c_char) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn memccpy(
        arg1: *mut ::core::ffi::c_void,
        arg2: *const ::core::ffi::c_void,
        arg3: ::core::ffi::c_int,
        arg4: usize,
    ) -> *mut ::core::ffi::c_void;
}
unsafe extern "C" {
    pub fn memchr(
        arg1: *const ::core::ffi::c_void,
        arg2: ::core::ffi::c_int,
        arg3: usize,
    ) -> *mut ::core::ffi::c_void;
}
unsafe extern "C" {
    pub fn memcmp(
        arg1: *const ::core::ffi::c_void,
        arg2: *const ::core::ffi::c_void,
        arg3: usize,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn memcpy(
        arg1: *mut ::core::ffi::c_void,
        arg2: *const ::core::ffi::c_void,
        arg3: usize,
    ) -> *mut ::core::ffi::c_void;
}
unsafe extern "C" {
    pub fn memmem(
        arg1: *const ::core::ffi::c_void,
        arg2: usize,
        arg3: *const ::core::ffi::c_void,
        arg4: usize,
    ) -> *mut ::core::ffi::c_void;
}
unsafe extern "C" {
    pub fn memmove(
        arg1: *mut ::core::ffi::c_void,
        arg2: *const ::core::ffi::c_void,
        arg3: usize,
    ) -> *mut ::core::ffi::c_void;
}
unsafe extern "C" {
    pub fn mempcpy(
        arg1: *mut ::core::ffi::c_void,
        arg2: *const ::core::ffi::c_void,
        arg3: usize,
    ) -> *mut ::core::ffi::c_void;
}
unsafe extern "C" {
    pub fn memrchr(
        arg1: *const ::core::ffi::c_void,
        arg2: ::core::ffi::c_int,
        arg3: usize,
    ) -> *mut ::core::ffi::c_void;
}
unsafe extern "C" {
    pub fn memset(
        arg1: *mut ::core::ffi::c_void,
        arg2: ::core::ffi::c_int,
        arg3: usize,
    ) -> *mut ::core::ffi::c_void;
}
unsafe extern "C" {
    pub fn memset_explicit(
        arg1: *mut ::core::ffi::c_void,
        arg2: ::core::ffi::c_int,
        arg3: usize,
    ) -> *mut ::core::ffi::c_void;
}
unsafe extern "C" {
    pub fn rawmemchr(
        arg1: *const ::core::ffi::c_void,
        arg2: ::core::ffi::c_int,
    ) -> *mut ::core::ffi::c_void;
}
unsafe extern "C" {
    pub fn stpcpy(
        arg1: *mut ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn stpncpy(
        arg1: *mut ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
        arg3: usize,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strcasestr(
        arg1: *const ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strcat(
        arg1: *mut ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strchr(
        arg1: *const ::core::ffi::c_char,
        arg2: ::core::ffi::c_int,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strchrnul(
        arg1: *const ::core::ffi::c_char,
        arg2: ::core::ffi::c_int,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strcmp(
        arg1: *const ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn strcoll(
        arg1: *const ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn strcoll_l(
        arg1: *const ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
        arg3: locale_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn strcpy(
        arg1: *mut ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strcspn(arg1: *const ::core::ffi::c_char, arg2: *const ::core::ffi::c_char) -> usize;
}
unsafe extern "C" {
    pub fn strdup(arg1: *const ::core::ffi::c_char) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strerror(arg1: ::core::ffi::c_int) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strerror_l(arg1: ::core::ffi::c_int, arg2: locale_t) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strerror_r(
        arg1: ::core::ffi::c_int,
        arg2: *mut ::core::ffi::c_char,
        arg3: usize,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strerrorname_np(errnum: ::core::ffi::c_int) -> *const ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strerrordesc_np(errnum: ::core::ffi::c_int) -> *const ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strlcat(
        arg1: *mut ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
        arg3: usize,
    ) -> usize;
}
unsafe extern "C" {
    pub fn strlen(arg1: *const ::core::ffi::c_char) -> usize;
}
unsafe extern "C" {
    pub fn strlcpy(
        arg1: *mut ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
        arg3: usize,
    ) -> usize;
}
unsafe extern "C" {
    pub fn strlwr(arg1: *mut ::core::ffi::c_char) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strncat(
        arg1: *mut ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
        arg3: usize,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strncmp(
        arg1: *const ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
        arg3: usize,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn strncpy(
        arg1: *mut ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
        arg3: usize,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strndup(arg1: *const ::core::ffi::c_char, arg2: usize) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strnlen(arg1: *const ::core::ffi::c_char, arg2: usize) -> usize;
}
unsafe extern "C" {
    pub fn strnstr(
        arg1: *const ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
        arg3: usize,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strpbrk(
        arg1: *const ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strrchr(
        arg1: *const ::core::ffi::c_char,
        arg2: ::core::ffi::c_int,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strsep(
        arg1: *mut *mut ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strsignal(__signo: ::core::ffi::c_int) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strspn(arg1: *const ::core::ffi::c_char, arg2: *const ::core::ffi::c_char) -> usize;
}
unsafe extern "C" {
    pub fn strstr(
        arg1: *const ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strtok(
        arg1: *mut ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strtok_r(
        arg1: *mut ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
        arg3: *mut *mut ::core::ffi::c_char,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strupr(arg1: *mut ::core::ffi::c_char) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strverscmp(
        arg1: *const ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn strxfrm(
        arg1: *mut ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
        arg3: usize,
    ) -> usize;
}
unsafe extern "C" {
    pub fn strxfrm_l(
        arg1: *mut ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
        arg3: usize,
        arg4: locale_t,
    ) -> usize;
}
unsafe extern "C" {
    pub fn timingsafe_bcmp(
        arg1: *const ::core::ffi::c_void,
        arg2: *const ::core::ffi::c_void,
        arg3: usize,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn timingsafe_memcmp(
        arg1: *const ::core::ffi::c_void,
        arg2: *const ::core::ffi::c_void,
        arg3: usize,
    ) -> ::core::ffi::c_int;
}
pub type wctype_t = __wctype_t;
pub type mbstate_t = _mbstate_t;
unsafe extern "C" {
    pub fn btowc(arg1: ::core::ffi::c_int) -> wint_t;
}
unsafe extern "C" {
    pub fn fgetwc(arg1: *mut __FILE) -> wint_t;
}
unsafe extern "C" {
    pub fn fgetwc_unlocked(arg1: *mut __FILE) -> wint_t;
}
unsafe extern "C" {
    pub fn fgetws(arg1: *mut wchar_t, arg2: ::core::ffi::c_int, arg3: *mut __FILE) -> *mut wchar_t;
}
unsafe extern "C" {
    pub fn fgetws_unlocked(
        arg1: *mut wchar_t,
        arg2: ::core::ffi::c_int,
        arg3: *mut __FILE,
    ) -> *mut wchar_t;
}
unsafe extern "C" {
    pub fn fputwc(arg1: wchar_t, arg2: *mut __FILE) -> wint_t;
}
unsafe extern "C" {
    pub fn fputwc_unlocked(arg1: wchar_t, arg2: *mut __FILE) -> wint_t;
}
unsafe extern "C" {
    pub fn fputws(arg1: *const wchar_t, arg2: *mut __FILE) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fputws_unlocked(arg1: *const wchar_t, arg2: *mut __FILE) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fwide(arg1: *mut __FILE, arg2: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fwprintf(arg1: *mut __FILE, arg2: *const wchar_t, ...) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fwscanf(arg1: *mut __FILE, arg2: *const wchar_t, ...) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn getwc(arg1: *mut __FILE) -> wint_t;
}
unsafe extern "C" {
    pub fn getwc_unlocked(arg1: *mut __FILE) -> wint_t;
}
unsafe extern "C" {
    pub fn getwchar() -> wint_t;
}
unsafe extern "C" {
    pub fn getwchar_unlocked() -> wint_t;
}
unsafe extern "C" {
    pub fn mbrlen(arg1: *const ::core::ffi::c_char, arg2: usize, arg3: *mut mbstate_t) -> usize;
}
unsafe extern "C" {
    pub fn mbrtowc(
        arg1: *mut wchar_t,
        arg2: *const ::core::ffi::c_char,
        arg3: usize,
        arg4: *mut mbstate_t,
    ) -> usize;
}
unsafe extern "C" {
    pub fn mbsinit(arg1: *const mbstate_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn mbsnrtowcs(
        arg1: *mut wchar_t,
        arg2: *mut *const ::core::ffi::c_char,
        arg3: usize,
        arg4: usize,
        arg5: *mut mbstate_t,
    ) -> usize;
}
unsafe extern "C" {
    pub fn mbsrtowcs(
        arg1: *mut wchar_t,
        arg2: *mut *const ::core::ffi::c_char,
        arg3: usize,
        arg4: *mut mbstate_t,
    ) -> usize;
}
unsafe extern "C" {
    pub fn open_wmemstream(arg1: *mut *mut wchar_t, arg2: *mut usize) -> *mut __FILE;
}
unsafe extern "C" {
    pub fn putwc(arg1: wchar_t, arg2: *mut __FILE) -> wint_t;
}
unsafe extern "C" {
    pub fn putwc_unlocked(arg1: wchar_t, arg2: *mut __FILE) -> wint_t;
}
unsafe extern "C" {
    pub fn putwchar(arg1: wchar_t) -> wint_t;
}
unsafe extern "C" {
    pub fn putwchar_unlocked(arg1: wchar_t) -> wint_t;
}
unsafe extern "C" {
    pub fn swprintf(
        arg1: *mut wchar_t,
        arg2: usize,
        arg3: *const wchar_t,
        ...
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn swscanf(arg1: *const wchar_t, arg2: *const wchar_t, ...) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn ungetwc(wc: wint_t, arg1: *mut __FILE) -> wint_t;
}
unsafe extern "C" {
    pub fn vfwprintf(
        arg1: *mut __FILE,
        arg2: *const wchar_t,
        arg3: __gnuc_va_list,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn vfwscanf(
        arg1: *mut __FILE,
        arg2: *const wchar_t,
        arg3: __gnuc_va_list,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn vswprintf(
        arg1: *mut wchar_t,
        arg2: usize,
        arg3: *const wchar_t,
        arg4: __gnuc_va_list,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn vswscanf(
        arg1: *const wchar_t,
        arg2: *const wchar_t,
        arg3: __gnuc_va_list,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn vwprintf(arg1: *const wchar_t, arg2: __gnuc_va_list) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn vwscanf(arg1: *const wchar_t, arg2: __gnuc_va_list) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn wcpcpy(arg1: *mut wchar_t, arg2: *const wchar_t) -> *mut wchar_t;
}
unsafe extern "C" {
    pub fn wcpncpy(arg1: *mut wchar_t, arg2: *const wchar_t, arg3: usize) -> *mut wchar_t;
}
unsafe extern "C" {
    pub fn wcrtomb(arg1: *mut ::core::ffi::c_char, arg2: wchar_t, arg3: *mut mbstate_t) -> usize;
}
unsafe extern "C" {
    pub fn wcscasecmp(arg1: *const wchar_t, arg2: *const wchar_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn wcscasecmp_l(
        arg1: *const wchar_t,
        arg2: *const wchar_t,
        arg3: locale_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn wcscat(arg1: *mut wchar_t, arg2: *const wchar_t) -> *mut wchar_t;
}
unsafe extern "C" {
    pub fn wcschr(arg1: *const wchar_t, arg2: wchar_t) -> *mut wchar_t;
}
unsafe extern "C" {
    pub fn wcscmp(arg1: *const wchar_t, arg2: *const wchar_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn wcscoll(arg1: *const wchar_t, arg2: *const wchar_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn wcscoll_l(
        arg1: *const wchar_t,
        arg2: *const wchar_t,
        arg3: locale_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn wcscpy(arg1: *mut wchar_t, arg2: *const wchar_t) -> *mut wchar_t;
}
unsafe extern "C" {
    pub fn wcscspn(arg1: *const wchar_t, arg2: *const wchar_t) -> usize;
}
unsafe extern "C" {
    pub fn wcsdup(arg1: *const wchar_t) -> *mut wchar_t;
}
unsafe extern "C" {
    pub fn wcsftime(
        arg1: *mut wchar_t,
        arg2: usize,
        arg3: *const wchar_t,
        arg4: *const tm,
    ) -> usize;
}
unsafe extern "C" {
    pub fn wcsftime_l(
        arg1: *mut wchar_t,
        arg2: usize,
        arg3: *const wchar_t,
        arg4: *const tm,
        arg5: locale_t,
    ) -> usize;
}
unsafe extern "C" {
    pub fn wcslcat(arg1: *mut wchar_t, arg2: *const wchar_t, arg3: usize) -> usize;
}
unsafe extern "C" {
    pub fn wcslcpy(arg1: *mut wchar_t, arg2: *const wchar_t, arg3: usize) -> usize;
}
unsafe extern "C" {
    pub fn wcslen(arg1: *const wchar_t) -> usize;
}
unsafe extern "C" {
    pub fn wcsncasecmp(
        arg1: *const wchar_t,
        arg2: *const wchar_t,
        arg3: usize,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn wcsncasecmp_l(
        arg1: *const wchar_t,
        arg2: *const wchar_t,
        arg3: usize,
        arg4: locale_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn wcsncat(arg1: *mut wchar_t, arg2: *const wchar_t, arg3: usize) -> *mut wchar_t;
}
unsafe extern "C" {
    pub fn wcsncmp(arg1: *const wchar_t, arg2: *const wchar_t, arg3: usize) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn wcsncpy(arg1: *mut wchar_t, arg2: *const wchar_t, arg3: usize) -> *mut wchar_t;
}
unsafe extern "C" {
    pub fn wcsnlen(arg1: *const wchar_t, arg2: usize) -> usize;
}
unsafe extern "C" {
    pub fn wcsnrtombs(
        arg1: *mut ::core::ffi::c_char,
        arg2: *mut *const wchar_t,
        arg3: usize,
        arg4: usize,
        arg5: *mut mbstate_t,
    ) -> usize;
}
unsafe extern "C" {
    pub fn wcspbrk(arg1: *const wchar_t, arg2: *const wchar_t) -> *mut wchar_t;
}
unsafe extern "C" {
    pub fn wcsrchr(arg1: *const wchar_t, arg2: wchar_t) -> *mut wchar_t;
}
unsafe extern "C" {
    pub fn wcsrtombs(
        arg1: *mut ::core::ffi::c_char,
        arg2: *mut *const wchar_t,
        arg3: usize,
        arg4: *mut mbstate_t,
    ) -> usize;
}
unsafe extern "C" {
    pub fn wcsspn(arg1: *const wchar_t, arg2: *const wchar_t) -> usize;
}
unsafe extern "C" {
    pub fn wcsstr(arg1: *const wchar_t, arg2: *const wchar_t) -> *mut wchar_t;
}
unsafe extern "C" {
    pub fn wcstod(arg1: *const wchar_t, arg2: *mut *mut wchar_t) -> f64;
}
unsafe extern "C" {
    pub fn wcstod_l(arg1: *const wchar_t, arg2: *mut *mut wchar_t, arg3: locale_t) -> f64;
}
unsafe extern "C" {
    pub fn wcstof(arg1: *const wchar_t, arg2: *mut *mut wchar_t) -> f32;
}
unsafe extern "C" {
    pub fn wcstof_l(arg1: *const wchar_t, arg2: *mut *mut wchar_t, arg3: locale_t) -> f32;
}
unsafe extern "C" {
    pub fn wcstok(
        arg1: *mut wchar_t,
        arg2: *const wchar_t,
        arg3: *mut *mut wchar_t,
    ) -> *mut wchar_t;
}
unsafe extern "C" {
    pub fn wcstol(
        arg1: *const wchar_t,
        arg2: *mut *mut wchar_t,
        arg3: ::core::ffi::c_int,
    ) -> ::core::ffi::c_long;
}
unsafe extern "C" {
    pub fn wcstol_l(
        arg1: *const wchar_t,
        arg2: *mut *mut wchar_t,
        arg3: ::core::ffi::c_int,
        arg4: locale_t,
    ) -> ::core::ffi::c_long;
}
unsafe extern "C" {
    pub fn wcstold(arg1: *const wchar_t, arg2: *mut *mut wchar_t) -> f64;
}
unsafe extern "C" {
    pub fn wcstold_l(arg1: *const wchar_t, arg2: *mut *mut wchar_t, arg3: locale_t) -> f64;
}
unsafe extern "C" {
    pub fn wcstoll(
        arg1: *const wchar_t,
        arg2: *mut *mut wchar_t,
        arg3: ::core::ffi::c_int,
    ) -> ::core::ffi::c_longlong;
}
unsafe extern "C" {
    pub fn wcstoll_l(
        arg1: *const wchar_t,
        arg2: *mut *mut wchar_t,
        arg3: ::core::ffi::c_int,
        arg4: locale_t,
    ) -> ::core::ffi::c_longlong;
}
unsafe extern "C" {
    pub fn wcstoul(
        arg1: *const wchar_t,
        arg2: *mut *mut wchar_t,
        arg3: ::core::ffi::c_int,
    ) -> ::core::ffi::c_ulong;
}
unsafe extern "C" {
    pub fn wcstoul_l(
        arg1: *const wchar_t,
        arg2: *mut *mut wchar_t,
        arg3: ::core::ffi::c_int,
        arg4: locale_t,
    ) -> ::core::ffi::c_ulong;
}
unsafe extern "C" {
    pub fn wcstoull(
        arg1: *const wchar_t,
        arg2: *mut *mut wchar_t,
        arg3: ::core::ffi::c_int,
    ) -> ::core::ffi::c_ulonglong;
}
unsafe extern "C" {
    pub fn wcstoull_l(
        arg1: *const wchar_t,
        arg2: *mut *mut wchar_t,
        arg3: ::core::ffi::c_int,
        arg4: locale_t,
    ) -> ::core::ffi::c_ulonglong;
}
unsafe extern "C" {
    pub fn wcswidth(arg1: *const wchar_t, arg2: usize) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn wcsxfrm(arg1: *mut wchar_t, arg2: *const wchar_t, arg3: usize) -> usize;
}
unsafe extern "C" {
    pub fn wcsxfrm_l(
        arg1: *mut wchar_t,
        arg2: *const wchar_t,
        arg3: usize,
        arg4: locale_t,
    ) -> usize;
}
unsafe extern "C" {
    pub fn wctob(arg1: wint_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn wcwidth(arg1: wchar_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn wmemchr(arg1: *const wchar_t, arg2: wchar_t, arg3: usize) -> *mut wchar_t;
}
unsafe extern "C" {
    pub fn wmemcmp(arg1: *const wchar_t, arg2: *const wchar_t, arg3: usize) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn wmemcpy(arg1: *mut wchar_t, arg2: *const wchar_t, arg3: usize) -> *mut wchar_t;
}
unsafe extern "C" {
    pub fn wmemmove(arg1: *mut wchar_t, arg2: *const wchar_t, arg3: usize) -> *mut wchar_t;
}
unsafe extern "C" {
    pub fn wmempcpy(arg1: *mut wchar_t, arg2: *const wchar_t, arg3: usize) -> *mut wchar_t;
}
unsafe extern "C" {
    pub fn wmemset(arg1: *mut wchar_t, arg2: wchar_t, arg3: usize) -> *mut wchar_t;
}
unsafe extern "C" {
    pub fn wprintf(arg1: *const wchar_t, ...) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn wscanf(arg1: *const wchar_t, ...) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn _user_strerror(
        errnum: ::core::ffi::c_int,
        internal: ::core::ffi::c_int,
        errptr: *mut ::core::ffi::c_int,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    #[thread_local]
    pub static mut errno: ::core::ffi::c_int;
}
pub type time_t = __time_t;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct timespec {
    pub tv_sec: time_t,
    pub tv_nsec: ::core::ffi::c_long,
}
pub type clock_t = ::core::ffi::c_ulong;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct tm {
    pub tm_sec: ::core::ffi::c_int,
    pub tm_min: ::core::ffi::c_int,
    pub tm_hour: ::core::ffi::c_int,
    pub tm_mday: ::core::ffi::c_int,
    pub tm_mon: ::core::ffi::c_int,
    pub tm_year: ::core::ffi::c_int,
    pub tm_wday: ::core::ffi::c_int,
    pub tm_yday: ::core::ffi::c_int,
    pub tm_isdst: ::core::ffi::c_int,
}
pub type clockid_t = __clockid_t;
pub type timer_t = __timer_t;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct itimerspec {
    pub it_interval: timespec,
    pub it_value: timespec,
}
pub type pid_t = __pid_t;
unsafe extern "C" {
    pub static mut _timezone: ::core::ffi::c_long;
}
unsafe extern "C" {
    pub static mut _daylight: ::core::ffi::c_int;
}
unsafe extern "C" {
    pub static mut tzname: [*mut ::core::ffi::c_char; 2usize];
}
unsafe extern "C" {
    pub fn asctime(_tblock: *const tm) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn asctime_r(arg1: *const tm, arg2: *mut ::core::ffi::c_char) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn clock() -> clock_t;
}
unsafe extern "C" {
    pub fn clock_getres(clock_id: clockid_t, res: *mut timespec) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn clock_gettime(clock_id: clockid_t, tp: *mut timespec) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn clock_nanosleep(
        clock_id: clockid_t,
        flags: ::core::ffi::c_int,
        rqtp: *const timespec,
        rmtp: *mut timespec,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn clock_settime(clock_id: clockid_t, tp: *const timespec) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn ctime(_time: *const time_t) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn ctime_r(arg1: *const time_t, arg2: *mut ::core::ffi::c_char)
        -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn difftime(_time2: time_t, _time1: time_t) -> f64;
}
unsafe extern "C" {
    pub static mut getdate_err: ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn getdate(arg1: *const ::core::ffi::c_char) -> *mut tm;
}
unsafe extern "C" {
    pub fn getdate_r(arg1: *const ::core::ffi::c_char, arg2: *mut tm) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn gmtime(_timer: *const time_t) -> *mut tm;
}
unsafe extern "C" {
    pub fn gmtime_r(arg1: *const time_t, arg2: *mut tm) -> *mut tm;
}
unsafe extern "C" {
    pub fn localtime(_timer: *const time_t) -> *mut tm;
}
unsafe extern "C" {
    pub fn localtime_r(arg1: *const time_t, arg2: *mut tm) -> *mut tm;
}
unsafe extern "C" {
    pub fn mktime(_timeptr: *mut tm) -> time_t;
}
unsafe extern "C" {
    pub fn nanosleep(rqtp: *const timespec, rmtp: *mut timespec) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn strftime(
        _s: *mut ::core::ffi::c_char,
        _maxsize: usize,
        _fmt: *const ::core::ffi::c_char,
        _t: *const tm,
    ) -> usize;
}
unsafe extern "C" {
    pub fn timespec_get(_ts: *mut timespec, _base: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn strftime_l(
        _s: *mut ::core::ffi::c_char,
        _maxsize: usize,
        _fmt: *const ::core::ffi::c_char,
        _t: *const tm,
        _l: locale_t,
    ) -> usize;
}
unsafe extern "C" {
    pub fn strptime(
        arg1: *const ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
        arg3: *mut tm,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn strptime_l(
        arg1: *const ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
        arg3: *mut tm,
        arg4: locale_t,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn time(_timer: *mut time_t) -> time_t;
}
unsafe extern "C" {
    pub fn timegm(_timeptr: *mut tm) -> time_t;
}
#[repr(C)]
#[derive(Debug)]
pub struct sigevent {
    _unused: [u8; 0],
}
unsafe extern "C" {
    pub fn timer_create(
        clock_id: clockid_t,
        evp: *mut sigevent,
        timerid: *mut timer_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn timer_delete(timerid: timer_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn timer_getoverrun(timerid: timer_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn timer_gettime(timerid: timer_t, value: *mut itimerspec) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn timer_settime(
        timerid: timer_t,
        flags: ::core::ffi::c_int,
        value: *const itimerspec,
        ovalue: *mut itimerspec,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn tzset();
}
pub type mode_t = __mode_t;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct flock {
    pub l_type: ::core::ffi::c_short,
    pub l_whence: ::core::ffi::c_short,
    pub l_start: ::core::ffi::c_long,
    pub l_len: ::core::ffi::c_long,
    pub l_pid: ::core::ffi::c_short,
    pub l_xxx: ::core::ffi::c_short,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct eflock {
    pub l_type: ::core::ffi::c_short,
    pub l_whence: ::core::ffi::c_short,
    pub l_start: ::core::ffi::c_long,
    pub l_len: ::core::ffi::c_long,
    pub l_pid: ::core::ffi::c_short,
    pub l_xxx: ::core::ffi::c_short,
    pub l_rpid: ::core::ffi::c_long,
    pub l_rsys: ::core::ffi::c_long,
}
unsafe extern "C" {
    pub fn open(
        arg1: *const ::core::ffi::c_char,
        arg2: ::core::ffi::c_int,
        ...
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn openat(
        arg1: ::core::ffi::c_int,
        arg2: *const ::core::ffi::c_char,
        arg3: ::core::ffi::c_int,
        ...
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn creat(arg1: *const ::core::ffi::c_char, arg2: mode_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fcntl(arg1: ::core::ffi::c_int, arg2: ::core::ffi::c_int, ...) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn flock(arg1: ::core::ffi::c_int, arg2: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
pub type useconds_t = __useconds_t;
pub type uid_t = __uid_t;
pub type gid_t = __gid_t;
unsafe extern "C" {
    pub static mut environ: *mut *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn _exit(__status: ::core::ffi::c_int) -> !;
}
unsafe extern "C" {
    pub fn access(
        __path: *const ::core::ffi::c_char,
        __amode: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn alarm(__secs: ::core::ffi::c_uint) -> ::core::ffi::c_uint;
}
unsafe extern "C" {
    pub fn brk(arg1: *mut ::core::ffi::c_void) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn chdir(__path: *const ::core::ffi::c_char) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn chown(
        __path: *const ::core::ffi::c_char,
        __owner: uid_t,
        __group: gid_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn chroot(__path: *const ::core::ffi::c_char) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn close(__fildes: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_close(
        __fildes: ::core::ffi::c_int,
        __flag: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn confstr(
        __name: ::core::ffi::c_int,
        __buf: *mut ::core::ffi::c_char,
        __len: usize,
    ) -> usize;
}
unsafe extern "C" {
    pub fn crypt(
        __key: *const ::core::ffi::c_char,
        __salt: *const ::core::ffi::c_char,
    ) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn daemon(nochdir: ::core::ffi::c_int, noclose: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn dup(__fildes: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn dup2(__fildes: ::core::ffi::c_int, __fildes2: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn dup3(
        __fildes: ::core::ffi::c_int,
        __fildes2: ::core::ffi::c_int,
        flags: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn eaccess(
        __path: *const ::core::ffi::c_char,
        __mode: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn encrypt(arg1: *mut ::core::ffi::c_char, arg2: ::core::ffi::c_int);
}
unsafe extern "C" {
    pub fn endusershell();
}
unsafe extern "C" {
    pub fn euidaccess(
        __path: *const ::core::ffi::c_char,
        __mode: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn execl(
        __path: *const ::core::ffi::c_char,
        arg1: *const ::core::ffi::c_char,
        ...
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn execle(
        __path: *const ::core::ffi::c_char,
        arg1: *const ::core::ffi::c_char,
        ...
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn execlp(
        __file: *const ::core::ffi::c_char,
        arg1: *const ::core::ffi::c_char,
        ...
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn execlpe(
        __file: *const ::core::ffi::c_char,
        arg1: *const ::core::ffi::c_char,
        ...
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn execv(
        __path: *const ::core::ffi::c_char,
        __argv: *const *mut ::core::ffi::c_char,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn execve(
        __path: *const ::core::ffi::c_char,
        __argv: *const *mut ::core::ffi::c_char,
        __envp: *const *mut ::core::ffi::c_char,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn execvp(
        __file: *const ::core::ffi::c_char,
        __argv: *const *mut ::core::ffi::c_char,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn execvpe(
        __file: *const ::core::ffi::c_char,
        __argv: *const *mut ::core::ffi::c_char,
        __envp: *const *mut ::core::ffi::c_char,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn faccessat(
        __dirfd: ::core::ffi::c_int,
        __path: *const ::core::ffi::c_char,
        __mode: ::core::ffi::c_int,
        __flags: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fchdir(__fildes: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fchown(
        __fildes: ::core::ffi::c_int,
        __owner: uid_t,
        __group: gid_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fchownat(
        __dirfd: ::core::ffi::c_int,
        __path: *const ::core::ffi::c_char,
        __owner: uid_t,
        __group: gid_t,
        __flags: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fexecve(
        __fd: ::core::ffi::c_int,
        __argv: *const *mut ::core::ffi::c_char,
        __envp: *const *mut ::core::ffi::c_char,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fork() -> pid_t;
}
unsafe extern "C" {
    pub fn fpathconf(__fd: ::core::ffi::c_int, __name: ::core::ffi::c_int) -> ::core::ffi::c_long;
}
unsafe extern "C" {
    pub fn fsync(__fd: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fdatasync(__fd: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn get_current_dir_name() -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn getcwd(__buf: *mut ::core::ffi::c_char, __size: usize) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn getdomainname(__name: *mut ::core::ffi::c_char, __len: usize) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn getentropy(arg1: *mut ::core::ffi::c_void, arg2: usize) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn getegid() -> gid_t;
}
unsafe extern "C" {
    pub fn geteuid() -> uid_t;
}
unsafe extern "C" {
    pub fn getgid() -> gid_t;
}
unsafe extern "C" {
    pub fn getgroups(
        __gidsetsize: ::core::ffi::c_int,
        __grouplist: *mut gid_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn getlogin() -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn getpass(__prompt: *const ::core::ffi::c_char) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn getpeereid(
        arg1: ::core::ffi::c_int,
        arg2: *mut uid_t,
        arg3: *mut gid_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn getpgid(arg1: pid_t) -> pid_t;
}
unsafe extern "C" {
    pub fn getpgrp() -> pid_t;
}
unsafe extern "C" {
    pub fn getpid() -> pid_t;
}
unsafe extern "C" {
    pub fn getppid() -> pid_t;
}
unsafe extern "C" {
    pub fn getsid(arg1: pid_t) -> pid_t;
}
unsafe extern "C" {
    pub fn getuid() -> uid_t;
}
unsafe extern "C" {
    pub fn getusershell() -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn getwd(__buf: *mut ::core::ffi::c_char) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn iruserok(
        raddr: ::core::ffi::c_ulong,
        superuser: ::core::ffi::c_int,
        ruser: *const ::core::ffi::c_char,
        luser: *const ::core::ffi::c_char,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn isatty(__fildes: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn issetugid() -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn link(
        __path1: *const ::core::ffi::c_char,
        __path2: *const ::core::ffi::c_char,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn linkat(
        __dirfd1: ::core::ffi::c_int,
        __path1: *const ::core::ffi::c_char,
        __dirfd2: ::core::ffi::c_int,
        __path2: *const ::core::ffi::c_char,
        __flags: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn nice(__nice_value: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn lseek(
        __fildes: ::core::ffi::c_int,
        __offset: off_t,
        __whence: ::core::ffi::c_int,
    ) -> off_t;
}
unsafe extern "C" {
    pub fn lockf(
        __fd: ::core::ffi::c_int,
        __cmd: ::core::ffi::c_int,
        __len: off_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pathconf(
        __path: *const ::core::ffi::c_char,
        __name: ::core::ffi::c_int,
    ) -> ::core::ffi::c_long;
}
unsafe extern "C" {
    pub fn pause() -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_atfork(
        arg1: ::core::option::Option<unsafe extern "C" fn()>,
        arg2: ::core::option::Option<unsafe extern "C" fn()>,
        arg3: ::core::option::Option<unsafe extern "C" fn()>,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pipe(__fildes: *mut ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pipe2(
        __fildes: *mut ::core::ffi::c_int,
        flags: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pread(
        __fd: ::core::ffi::c_int,
        __buf: *mut ::core::ffi::c_void,
        __nbytes: usize,
        __offset: off_t,
    ) -> isize;
}
unsafe extern "C" {
    pub fn pwrite(
        __fd: ::core::ffi::c_int,
        __buf: *const ::core::ffi::c_void,
        __nbytes: usize,
        __offset: off_t,
    ) -> isize;
}
unsafe extern "C" {
    pub fn read(__fd: ::core::ffi::c_int, __buf: *mut ::core::ffi::c_void, __nbyte: usize)
        -> isize;
}
unsafe extern "C" {
    pub fn rresvport(__alport: *mut ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn revoke(__path: *mut ::core::ffi::c_char) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn rmdir(__path: *const ::core::ffi::c_char) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn ruserok(
        rhost: *const ::core::ffi::c_char,
        superuser: ::core::ffi::c_int,
        ruser: *const ::core::ffi::c_char,
        luser: *const ::core::ffi::c_char,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn sbrk(__incr: isize) -> *mut ::core::ffi::c_void;
}
unsafe extern "C" {
    pub fn setegid(__gid: gid_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn seteuid(__uid: uid_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn setgid(__gid: gid_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn setgroups(ngroups: ::core::ffi::c_int, grouplist: *const gid_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn sethostname(arg1: *const ::core::ffi::c_char, arg2: usize) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn setpgid(__pid: pid_t, __pgid: pid_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn setpgrp() -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn setregid(__rgid: gid_t, __egid: gid_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn setreuid(__ruid: uid_t, __euid: uid_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn setsid() -> pid_t;
}
unsafe extern "C" {
    pub fn setuid(__uid: uid_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn setusershell();
}
unsafe extern "C" {
    pub fn sleep(__seconds: ::core::ffi::c_uint) -> ::core::ffi::c_uint;
}
unsafe extern "C" {
    pub fn swab(arg1: *const ::core::ffi::c_void, arg2: *mut ::core::ffi::c_void, arg3: isize);
}
unsafe extern "C" {
    pub fn sysconf(__name: ::core::ffi::c_int) -> ::core::ffi::c_long;
}
unsafe extern "C" {
    pub fn tcgetpgrp(__fildes: ::core::ffi::c_int) -> pid_t;
}
unsafe extern "C" {
    pub fn tcsetpgrp(__fildes: ::core::ffi::c_int, __pgrp_id: pid_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn ttyname(__fildes: ::core::ffi::c_int) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn ttyname_r(
        arg1: ::core::ffi::c_int,
        arg2: *mut ::core::ffi::c_char,
        arg3: usize,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn unlink(__path: *const ::core::ffi::c_char) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn usleep(__useconds: useconds_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn vhangup() -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn write(
        __fd: ::core::ffi::c_int,
        __buf: *const ::core::ffi::c_void,
        __nbyte: usize,
    ) -> isize;
}
unsafe extern "C" {
    pub static mut optarg: *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub static mut optind: ::core::ffi::c_int;
}
unsafe extern "C" {
    pub static mut opterr: ::core::ffi::c_int;
}
unsafe extern "C" {
    pub static mut optopt: ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn getopt(
        arg1: ::core::ffi::c_int,
        arg2: *const *mut ::core::ffi::c_char,
        arg3: *const ::core::ffi::c_char,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub static mut optreset: ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn vfork() -> pid_t;
}
unsafe extern "C" {
    pub fn lseek64(
        __filedes: ::core::ffi::c_int,
        __offset: _off64_t,
        __whence: ::core::ffi::c_int,
    ) -> _off64_t;
}
unsafe extern "C" {
    pub fn ftruncate(__fd: ::core::ffi::c_int, __length: off_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn truncate(arg1: *const ::core::ffi::c_char, __length: off_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn getdtablesize() -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn ualarm(__useconds: useconds_t, __interval: useconds_t) -> useconds_t;
}
unsafe extern "C" {
    pub fn gethostname(__name: *mut ::core::ffi::c_char, __len: usize) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn setdtablesize(arg1: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn sync();
}
unsafe extern "C" {
    pub fn readlink(
        __path: *const ::core::ffi::c_char,
        __buf: *mut ::core::ffi::c_char,
        __buflen: usize,
    ) -> isize;
}
unsafe extern "C" {
    pub fn symlink(
        __name1: *const ::core::ffi::c_char,
        __name2: *const ::core::ffi::c_char,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn readlinkat(
        __dirfd1: ::core::ffi::c_int,
        __path: *const ::core::ffi::c_char,
        __buf: *mut ::core::ffi::c_char,
        __buflen: usize,
    ) -> isize;
}
unsafe extern "C" {
    pub fn symlinkat(
        arg1: *const ::core::ffi::c_char,
        arg2: ::core::ffi::c_int,
        arg3: *const ::core::ffi::c_char,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn unlinkat(
        arg1: ::core::ffi::c_int,
        arg2: *const ::core::ffi::c_char,
        arg3: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
pub type u_int8_t = __uint8_t;
pub type u_int16_t = __uint16_t;
pub type u_int32_t = __uint32_t;
pub type u_int64_t = __uint64_t;
pub type register_t = __intptr_t;
pub type in_addr_t = __uint32_t;
pub type in_port_t = __uint16_t;
pub type u_register_t = __uintptr_t;
pub type u_char = ::core::ffi::c_uchar;
pub type u_short = ::core::ffi::c_ushort;
pub type u_int = ::core::ffi::c_uint;
pub type u_long = ::core::ffi::c_ulong;
pub type ushort = ::core::ffi::c_ushort;
pub type uint = ::core::ffi::c_uint;
pub type ulong = ::core::ffi::c_ulong;
pub type blkcnt_t = __blkcnt_t;
pub type blksize_t = __blksize_t;
pub type daddr_t = __daddr_t;
pub type caddr_t = *mut ::core::ffi::c_char;
pub type fsblkcnt_t = __fsblkcnt_t;
pub type fsfilcnt_t = __fsfilcnt_t;
pub type id_t = __id_t;
pub type ino_t = __ino_t;
pub type dev_t = __dev_t;
pub type key_t = __key_t;
pub type nlink_t = __nlink_t;
pub type suseconds_t = __suseconds_t;
pub type sbintime_t = __int64_t;
pub const LUME_FILES_OPEN: _bindgen_ty_1 = 0;
pub const LUME_FILES_STAT: _bindgen_ty_1 = 1;
pub const LUME_FILES_CHDIR: _bindgen_ty_1 = 2;
pub const LUME_FILES_GETCWD: _bindgen_ty_1 = 3;
pub const LUME_FILES_MKDIR: _bindgen_ty_1 = 4;
pub const LUME_FILES_UNLINK: _bindgen_ty_1 = 5;
pub const LUME_FILES_RMDIR: _bindgen_ty_1 = 6;
pub const LUME_FILES_RENAME: _bindgen_ty_1 = 7;
pub const LUME_FILES_REPLACE: _bindgen_ty_1 = 8;
pub const LUME_FILES_GETDENTS: _bindgen_ty_1 = 9;
pub const LUME_FILES_FD_PATH: _bindgen_ty_1 = 10;
pub const LUME_FILES_CANONICAL: _bindgen_ty_1 = 11;
pub const LUME_FILES_MODE: _bindgen_ty_1 = 12;
pub const LUME_FILES_VOLUME: _bindgen_ty_1 = 13;
pub type _bindgen_ty_1 = ::core::ffi::c_uint;
pub const LUME_FILES_STAT_NOFOLLOW: _bindgen_ty_2 = 1;
pub const LUME_FILES_STAT_EMPTY_PATH: _bindgen_ty_2 = 2;
pub const LUME_FILES_OPEN_DIRECTORY: _bindgen_ty_2 = 65536;
pub const LUME_FILES_OPEN_NOFOLLOW: _bindgen_ty_2 = 131072;
pub type _bindgen_ty_2 = ::core::ffi::c_uint;
pub const LUME_FILES_VOLUME_FD: _bindgen_ty_3 = 1;
pub const LUME_FILES_VOLUME_READ_ONLY: _bindgen_ty_3 = 1;
pub type _bindgen_ty_3 = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_files_request {
    pub version: u32,
    pub bytes: u32,
    pub path: u64,
    pub path2: u64,
    pub buffer: u64,
    pub count: u64,
    pub fd: i64,
    pub fd2: i64,
    pub flags: u64,
    pub mode: u32,
    pub descriptor_flags: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_files_dirent {
    pub inode: u64,
    pub kind: u32,
    pub name_bytes: u32,
    pub name: [::core::ffi::c_char; 256usize],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_files_volume {
    pub version: u32,
    pub bytes: u32,
    pub block_bytes: u64,
    pub fragment_bytes: u64,
    pub total_blocks: u64,
    pub free_blocks: u64,
    pub available_blocks: u64,
    pub filesystem_id: u64,
    pub flags: u32,
    pub name_max: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct dirent {
    pub d_ino: ino_t,
    pub d_reclen: ::core::ffi::c_ushort,
    pub d_type: ::core::ffi::c_uchar,
    pub d_name: [::core::ffi::c_char; 256usize],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct DIR {
    pub fd: ::core::ffi::c_int,
    pub count: ::core::ffi::c_uint,
    pub next: ::core::ffi::c_uint,
    pub eof: ::core::ffi::c_uint,
    pub position: u64,
    pub buffer: [::core::ffi::c_uchar; 1088usize],
    pub entry: dirent,
}
unsafe extern "C" {
    pub fn alphasort(arg1: *mut *const dirent, arg2: *mut *const dirent) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn dirfd(arg1: *mut DIR) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fdclosedir(arg1: *mut DIR) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn opendir(arg1: *const ::core::ffi::c_char) -> *mut DIR;
}
unsafe extern "C" {
    pub fn fdopendir(arg1: ::core::ffi::c_int) -> *mut DIR;
}
unsafe extern "C" {
    pub fn readdir(arg1: *mut DIR) -> *mut dirent;
}
unsafe extern "C" {
    pub fn readdir_r(
        arg1: *mut DIR,
        arg2: *mut dirent,
        arg3: *mut *mut dirent,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn rewinddir(arg1: *mut DIR);
}
unsafe extern "C" {
    pub fn scandir(
        arg1: *const ::core::ffi::c_char,
        arg2: *mut *mut *mut dirent,
        arg3: ::core::option::Option<
            unsafe extern "C" fn(arg1: *const dirent) -> ::core::ffi::c_int,
        >,
        arg4: ::core::option::Option<
            unsafe extern "C" fn(
                arg1: *mut *const dirent,
                arg2: *mut *const dirent,
            ) -> ::core::ffi::c_int,
        >,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn seekdir(arg1: *mut DIR, arg2: ::core::ffi::c_long);
}
unsafe extern "C" {
    pub fn telldir(arg1: *mut DIR) -> ::core::ffi::c_long;
}
unsafe extern "C" {
    pub fn closedir(arg1: *mut DIR) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn scandirat(
        arg1: ::core::ffi::c_int,
        arg2: *const ::core::ffi::c_char,
        arg3: *mut *mut *mut dirent,
        arg4: ::core::option::Option<
            unsafe extern "C" fn(arg1: *const dirent) -> ::core::ffi::c_int,
        >,
        arg5: ::core::option::Option<
            unsafe extern "C" fn(
                arg1: *mut *const dirent,
                arg2: *mut *const dirent,
            ) -> ::core::ffi::c_int,
        >,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn versionsort(arg1: *mut *const dirent, arg2: *mut *const dirent) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_getdents(
        arg1: ::core::ffi::c_int,
        arg2: *mut ::core::ffi::c_void,
        arg3: usize,
        arg4: ::core::ffi::c_int,
    ) -> isize;
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct stack_t {
    pub ss_sp: *mut ::core::ffi::c_void,
    pub ss_flags: ::core::ffi::c_int,
    pub ss_size: usize,
}
pub type sigset_t = u32;
pub type sig_atomic_t = ::core::ffi::c_int;
#[repr(C)]
pub struct sigaction {
    pub sa_handler: sighandler_t,
    pub sa_mask: sigset_t,
    pub sa_flags: ::core::ffi::c_int,
}
unsafe extern "C" {
    pub fn sigaction(
        arg1: ::core::ffi::c_int,
        arg2: *const sigaction,
        arg3: *mut sigaction,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn sigprocmask(
        arg1: ::core::ffi::c_int,
        arg2: *const sigset_t,
        arg3: *mut sigset_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_sigmask(
        arg1: ::core::ffi::c_int,
        arg2: *const sigset_t,
        arg3: *mut sigset_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn sigwait(arg1: *const sigset_t, arg2: *mut ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn sigemptyset(arg1: *mut sigset_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn sigfillset(arg1: *mut sigset_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn sigaddset(arg1: *mut sigset_t, arg2: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn sigdelset(arg1: *mut sigset_t, arg2: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn sigismember(arg1: *const sigset_t, arg2: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn signal(arg1: ::core::ffi::c_int, arg2: sighandler_t) -> sighandler_t;
}
unsafe extern "C" {
    pub fn raise(arg1: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn kill(arg1: pid_t, arg2: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn sigsuspend(arg1: *const sigset_t) -> ::core::ffi::c_int;
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct sched_param {
    pub sched_priority: ::core::ffi::c_int,
}
unsafe extern "C" {
    pub fn sched_getcpu() -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn sched_yield() -> ::core::ffi::c_int;
}
pub type pthread_t = u64;
pub type pthread_key_t = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct pthread_attr_t {
    pub stacksize: usize,
    pub guardsize: usize,
    pub detachstate: ::core::ffi::c_int,
}
#[repr(C)]
#[repr(align(8))]
#[derive(Debug, Copy, Clone)]
pub struct pthread_mutex_t {
    pub word: u32,
    pub type_: u32,
    pub depth: u32,
    pub reserved: u32,
    pub owner: pthread_t,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct pthread_mutexattr_t {
    pub type_: ::core::ffi::c_int,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct pthread_cond_t {
    pub sequence: u32,
    pub waiters: u32,
    pub clock: ::core::ffi::c_int,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct pthread_condattr_t {
    pub clock: ::core::ffi::c_int,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct pthread_rwlock_t {
    pub mutex: pthread_mutex_t,
    pub changed: pthread_cond_t,
    pub readers: ::core::ffi::c_uint,
    pub waiters: ::core::ffi::c_uint,
    pub writer: pthread_t,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct pthread_rwlockattr_t {
    pub shared: ::core::ffi::c_int,
}
pub type pthread_once_t = u32;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct __lume_cleanup_record {
    pub previous: *mut __lume_cleanup_record,
    pub routine: ::core::option::Option<unsafe extern "C" fn(arg1: *mut ::core::ffi::c_void)>,
    pub argument: *mut ::core::ffi::c_void,
}
unsafe extern "C" {
    pub fn pthread_create(
        arg1: *mut pthread_t,
        arg2: *const pthread_attr_t,
        arg3: ::core::option::Option<
            unsafe extern "C" fn(arg1: *mut ::core::ffi::c_void) -> *mut ::core::ffi::c_void,
        >,
        arg4: *mut ::core::ffi::c_void,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_join(arg1: pthread_t, arg2: *mut *mut ::core::ffi::c_void)
        -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_detach(arg1: pthread_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_self() -> pthread_t;
}
unsafe extern "C" {
    pub fn pthread_equal(arg1: pthread_t, arg2: pthread_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn __lume_cleanup_push(
        arg1: *mut __lume_cleanup_record,
        arg2: ::core::option::Option<unsafe extern "C" fn(arg1: *mut ::core::ffi::c_void)>,
        arg3: *mut ::core::ffi::c_void,
    );
}
unsafe extern "C" {
    pub fn __lume_cleanup_pop(arg1: *mut __lume_cleanup_record, arg2: ::core::ffi::c_int);
}
unsafe extern "C" {
    pub fn pthread_exit(arg1: *mut ::core::ffi::c_void);
}
unsafe extern "C" {
    pub fn pthread_attr_init(arg1: *mut pthread_attr_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_attr_destroy(arg1: *mut pthread_attr_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_attr_setstacksize(arg1: *mut pthread_attr_t, arg2: usize) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_attr_getstacksize(
        arg1: *const pthread_attr_t,
        arg2: *mut usize,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_attr_getstack(
        arg1: *const pthread_attr_t,
        arg2: *mut *mut ::core::ffi::c_void,
        arg3: *mut usize,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_attr_setstack(
        arg1: *mut pthread_attr_t,
        arg2: *mut ::core::ffi::c_void,
        arg3: usize,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_getattr_np(arg1: pthread_t, arg2: *mut pthread_attr_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_getname_np(
        arg1: pthread_t,
        arg2: *mut ::core::ffi::c_char,
        arg3: usize,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_setname_np(
        arg1: pthread_t,
        arg2: *const ::core::ffi::c_char,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_getschedparam(
        arg1: pthread_t,
        arg2: *mut ::core::ffi::c_int,
        arg3: *mut sched_param,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_setschedparam(
        arg1: pthread_t,
        arg2: ::core::ffi::c_int,
        arg3: *const sched_param,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_attr_setdetachstate(
        arg1: *mut pthread_attr_t,
        arg2: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_attr_getdetachstate(
        arg1: *const pthread_attr_t,
        arg2: *mut ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_attr_setguardsize(arg1: *mut pthread_attr_t, arg2: usize) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_attr_getguardsize(
        arg1: *const pthread_attr_t,
        arg2: *mut usize,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_mutexattr_init(arg1: *mut pthread_mutexattr_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_mutexattr_destroy(arg1: *mut pthread_mutexattr_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_mutexattr_settype(
        arg1: *mut pthread_mutexattr_t,
        arg2: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_mutexattr_gettype(
        arg1: *const pthread_mutexattr_t,
        arg2: *mut ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_mutex_init(
        arg1: *mut pthread_mutex_t,
        arg2: *const pthread_mutexattr_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_mutex_destroy(arg1: *mut pthread_mutex_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_mutex_lock(arg1: *mut pthread_mutex_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_mutex_trylock(arg1: *mut pthread_mutex_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_mutex_unlock(arg1: *mut pthread_mutex_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_condattr_init(arg1: *mut pthread_condattr_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_condattr_destroy(arg1: *mut pthread_condattr_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_condattr_setclock(
        arg1: *mut pthread_condattr_t,
        arg2: clockid_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_condattr_getclock(
        arg1: *const pthread_condattr_t,
        arg2: *mut clockid_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_cond_init(
        arg1: *mut pthread_cond_t,
        arg2: *const pthread_condattr_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_cond_destroy(arg1: *mut pthread_cond_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_cond_wait(
        arg1: *mut pthread_cond_t,
        arg2: *mut pthread_mutex_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_cond_timedwait(
        arg1: *mut pthread_cond_t,
        arg2: *mut pthread_mutex_t,
        arg3: *const timespec,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_cond_signal(arg1: *mut pthread_cond_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_cond_broadcast(arg1: *mut pthread_cond_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_rwlockattr_init(arg1: *mut pthread_rwlockattr_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_rwlockattr_destroy(arg1: *mut pthread_rwlockattr_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_rwlockattr_getpshared(
        arg1: *const pthread_rwlockattr_t,
        arg2: *mut ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_rwlockattr_setpshared(
        arg1: *mut pthread_rwlockattr_t,
        arg2: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_rwlock_init(
        arg1: *mut pthread_rwlock_t,
        arg2: *const pthread_rwlockattr_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_rwlock_destroy(arg1: *mut pthread_rwlock_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_rwlock_rdlock(arg1: *mut pthread_rwlock_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_rwlock_tryrdlock(arg1: *mut pthread_rwlock_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_rwlock_wrlock(arg1: *mut pthread_rwlock_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_rwlock_trywrlock(arg1: *mut pthread_rwlock_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_rwlock_unlock(arg1: *mut pthread_rwlock_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_once(
        arg1: *mut pthread_once_t,
        arg2: ::core::option::Option<unsafe extern "C" fn()>,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_key_create(
        arg1: *mut pthread_key_t,
        arg2: ::core::option::Option<unsafe extern "C" fn(arg1: *mut ::core::ffi::c_void)>,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_key_delete(arg1: pthread_key_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_setspecific(
        arg1: pthread_key_t,
        arg2: *const ::core::ffi::c_void,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pthread_getspecific(arg1: pthread_key_t) -> *mut ::core::ffi::c_void;
}
#[repr(C)]
#[derive(Debug)]
pub struct __posix_spawnattr {
    _unused: [u8; 0],
}
pub type posix_spawnattr_t = *mut __posix_spawnattr;
#[repr(C)]
#[derive(Debug)]
pub struct __posix_spawn_file_actions {
    _unused: [u8; 0],
}
pub type posix_spawn_file_actions_t = *mut __posix_spawn_file_actions;
unsafe extern "C" {
    pub fn posix_spawn(
        arg1: *mut pid_t,
        arg2: *const ::core::ffi::c_char,
        arg3: *const posix_spawn_file_actions_t,
        arg4: *const posix_spawnattr_t,
        arg5: *const *mut ::core::ffi::c_char,
        arg6: *const *mut ::core::ffi::c_char,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_spawnp(
        arg1: *mut pid_t,
        arg2: *const ::core::ffi::c_char,
        arg3: *const posix_spawn_file_actions_t,
        arg4: *const posix_spawnattr_t,
        arg5: *const *mut ::core::ffi::c_char,
        arg6: *const *mut ::core::ffi::c_char,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_spawn_file_actions_init(
        arg1: *mut posix_spawn_file_actions_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_spawn_file_actions_destroy(
        arg1: *mut posix_spawn_file_actions_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_spawn_file_actions_addopen(
        arg1: *mut posix_spawn_file_actions_t,
        arg2: ::core::ffi::c_int,
        arg3: *const ::core::ffi::c_char,
        arg4: ::core::ffi::c_int,
        arg5: mode_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_spawn_file_actions_adddup2(
        arg1: *mut posix_spawn_file_actions_t,
        arg2: ::core::ffi::c_int,
        arg3: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_spawn_file_actions_addclose(
        arg1: *mut posix_spawn_file_actions_t,
        arg2: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_spawn_file_actions_addchdir(
        arg1: *mut posix_spawn_file_actions_t,
        arg2: *const ::core::ffi::c_char,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_spawn_file_actions_addfchdir(
        arg1: *mut posix_spawn_file_actions_t,
        arg2: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_spawn_file_actions_addchdir_np(
        arg1: *mut posix_spawn_file_actions_t,
        arg2: *const ::core::ffi::c_char,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_spawn_file_actions_addfchdir_np(
        arg1: *mut posix_spawn_file_actions_t,
        arg2: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_spawnattr_init(arg1: *mut posix_spawnattr_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_spawnattr_destroy(arg1: *mut posix_spawnattr_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_spawnattr_getflags(
        arg1: *const posix_spawnattr_t,
        arg2: *mut ::core::ffi::c_short,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_spawnattr_getpgroup(
        arg1: *const posix_spawnattr_t,
        arg2: *mut pid_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_spawnattr_getschedparam(
        arg1: *const posix_spawnattr_t,
        arg2: *mut sched_param,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_spawnattr_getschedpolicy(
        arg1: *const posix_spawnattr_t,
        arg2: *mut ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_spawnattr_getsigdefault(
        arg1: *const posix_spawnattr_t,
        arg2: *mut sigset_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_spawnattr_getsigmask(
        arg1: *const posix_spawnattr_t,
        arg2: *mut sigset_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_spawnattr_setflags(
        arg1: *mut posix_spawnattr_t,
        arg2: ::core::ffi::c_short,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_spawnattr_setpgroup(
        arg1: *mut posix_spawnattr_t,
        arg2: pid_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_spawnattr_setschedparam(
        arg1: *mut posix_spawnattr_t,
        arg2: *const sched_param,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_spawnattr_setschedpolicy(
        arg1: *mut posix_spawnattr_t,
        arg2: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_spawnattr_setsigdefault(
        arg1: *mut posix_spawnattr_t,
        arg2: *const sigset_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_spawnattr_setsigmask(
        arg1: *mut posix_spawnattr_t,
        arg2: *const sigset_t,
    ) -> ::core::ffi::c_int;
}
pub type nfds_t = ::core::ffi::c_ulong;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct pollfd {
    pub fd: ::core::ffi::c_int,
    pub events: ::core::ffi::c_short,
    pub revents: ::core::ffi::c_short,
}
unsafe extern "C" {
    pub fn poll(arg1: *mut pollfd, arg2: nfds_t, arg3: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
#[repr(C)]
#[repr(align(8))]
#[derive(Debug, Copy, Clone)]
pub struct sem_t {
    pub counters: u64,
    pub state: u32,
}
unsafe extern "C" {
    pub fn sem_init(
        arg1: *mut sem_t,
        arg2: ::core::ffi::c_int,
        arg3: ::core::ffi::c_uint,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn sem_destroy(arg1: *mut sem_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn sem_wait(arg1: *mut sem_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn sem_trywait(arg1: *mut sem_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn sem_timedwait(arg1: *mut sem_t, arg2: *const timespec) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn sem_post(arg1: *mut sem_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn sem_getvalue(arg1: *mut sem_t, arg2: *mut ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn sem_open(arg1: *const ::core::ffi::c_char, arg2: ::core::ffi::c_int, ...) -> *mut sem_t;
}
unsafe extern "C" {
    pub fn sem_close(arg1: *mut sem_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn sem_unlink(arg1: *const ::core::ffi::c_char) -> ::core::ffi::c_int;
}
pub type socklen_t = u32;
pub type sa_family_t = u16;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct sockaddr {
    pub sa_family: sa_family_t,
    pub sa_data: [::core::ffi::c_char; 14usize],
}
#[repr(C)]
#[repr(align(4))]
#[derive(Debug, Copy, Clone)]
pub struct sockaddr_storage {
    pub ss_family: sa_family_t,
    pub data: [::core::ffi::c_uchar; 126usize],
}
unsafe extern "C" {
    pub fn socket(
        arg1: ::core::ffi::c_int,
        arg2: ::core::ffi::c_int,
        arg3: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn bind(
        arg1: ::core::ffi::c_int,
        arg2: *const sockaddr,
        arg3: socklen_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn listen(arg1: ::core::ffi::c_int, arg2: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn accept(
        arg1: ::core::ffi::c_int,
        arg2: *mut sockaddr,
        arg3: *mut socklen_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn connect(
        arg1: ::core::ffi::c_int,
        arg2: *const sockaddr,
        arg3: socklen_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn setsockopt(
        arg1: ::core::ffi::c_int,
        arg2: ::core::ffi::c_int,
        arg3: ::core::ffi::c_int,
        arg4: *const ::core::ffi::c_void,
        arg5: socklen_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn getsockopt(
        arg1: ::core::ffi::c_int,
        arg2: ::core::ffi::c_int,
        arg3: ::core::ffi::c_int,
        arg4: *mut ::core::ffi::c_void,
        arg5: *mut socklen_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn getsockname(
        arg1: ::core::ffi::c_int,
        arg2: *mut sockaddr,
        arg3: *mut socklen_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn getpeername(
        arg1: ::core::ffi::c_int,
        arg2: *mut sockaddr,
        arg3: *mut socklen_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn shutdown(arg1: ::core::ffi::c_int, arg2: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn recv(
        arg1: ::core::ffi::c_int,
        arg2: *mut ::core::ffi::c_void,
        arg3: usize,
        arg4: ::core::ffi::c_int,
    ) -> isize;
}
unsafe extern "C" {
    pub fn send(
        arg1: ::core::ffi::c_int,
        arg2: *const ::core::ffi::c_void,
        arg3: usize,
        arg4: ::core::ffi::c_int,
    ) -> isize;
}
unsafe extern "C" {
    pub fn sendto(
        arg1: ::core::ffi::c_int,
        arg2: *const ::core::ffi::c_void,
        arg3: usize,
        arg4: ::core::ffi::c_int,
        arg5: *const sockaddr,
        arg6: socklen_t,
    ) -> isize;
}
unsafe extern "C" {
    pub fn recvfrom(
        arg1: ::core::ffi::c_int,
        arg2: *mut ::core::ffi::c_void,
        arg3: usize,
        arg4: ::core::ffi::c_int,
        arg5: *mut sockaddr,
        arg6: *mut socklen_t,
    ) -> isize;
}
unsafe extern "C" {
    pub fn socketpair(
        arg1: ::core::ffi::c_int,
        arg2: ::core::ffi::c_int,
        arg3: ::core::ffi::c_int,
        arg4: *mut ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct msghdr {
    pub msg_name: *mut ::core::ffi::c_void,
    pub msg_namelen: socklen_t,
    pub msg_iov: *mut iovec,
    pub msg_iovlen: usize,
    pub msg_control: *mut ::core::ffi::c_void,
    pub msg_controllen: usize,
    pub msg_flags: ::core::ffi::c_int,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct cmsghdr {
    pub cmsg_len: usize,
    pub cmsg_level: ::core::ffi::c_int,
    pub cmsg_type: ::core::ffi::c_int,
}
unsafe extern "C" {
    pub fn sendmsg(
        arg1: ::core::ffi::c_int,
        arg2: *const msghdr,
        arg3: ::core::ffi::c_int,
    ) -> isize;
}
unsafe extern "C" {
    pub fn recvmsg(arg1: ::core::ffi::c_int, arg2: *mut msghdr, arg3: ::core::ffi::c_int) -> isize;
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct hostent {
    pub h_name: *mut ::core::ffi::c_char,
    pub h_aliases: *mut *mut ::core::ffi::c_char,
    pub h_addrtype: ::core::ffi::c_int,
    pub h_length: ::core::ffi::c_int,
    pub h_addr_list: *mut *mut ::core::ffi::c_char,
}
unsafe extern "C" {
    pub fn __lume_h_errno() -> *mut ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn gethostbyname(arg1: *const ::core::ffi::c_char) -> *mut hostent;
}
unsafe extern "C" {
    pub fn gethostbyaddr(
        arg1: *const ::core::ffi::c_void,
        arg2: socklen_t,
        arg3: ::core::ffi::c_int,
    ) -> *mut hostent;
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct addrinfo {
    pub ai_flags: ::core::ffi::c_int,
    pub ai_family: ::core::ffi::c_int,
    pub ai_socktype: ::core::ffi::c_int,
    pub ai_protocol: ::core::ffi::c_int,
    pub ai_addrlen: socklen_t,
    pub ai_addr: *mut sockaddr,
    pub ai_canonname: *mut ::core::ffi::c_char,
    pub ai_next: *mut addrinfo,
}
unsafe extern "C" {
    pub fn getaddrinfo(
        arg1: *const ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
        arg3: *const addrinfo,
        arg4: *mut *mut addrinfo,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn freeaddrinfo(arg1: *mut addrinfo);
}
unsafe extern "C" {
    pub fn gai_strerror(arg1: ::core::ffi::c_int) -> *const ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn getnameinfo(
        arg1: *const sockaddr,
        arg2: socklen_t,
        arg3: *mut ::core::ffi::c_char,
        arg4: socklen_t,
        arg5: *mut ::core::ffi::c_char,
        arg6: socklen_t,
        arg7: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct rdatainfo {
    pub rdi_length: ::core::ffi::c_uint,
    pub rdi_data: *mut ::core::ffi::c_uchar,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct rrsetinfo {
    pub rri_flags: ::core::ffi::c_uint,
    pub rri_rdclass: ::core::ffi::c_uint,
    pub rri_rdtype: ::core::ffi::c_uint,
    pub rri_ttl: ::core::ffi::c_uint,
    pub rri_nrdatas: ::core::ffi::c_uint,
    pub rri_nsigs: ::core::ffi::c_uint,
    pub rri_name: *mut ::core::ffi::c_char,
    pub rri_rdatas: *mut rdatainfo,
    pub rri_sigs: *mut rdatainfo,
}
unsafe extern "C" {
    pub fn getrrsetbyname(
        arg1: *const ::core::ffi::c_char,
        arg2: ::core::ffi::c_uint,
        arg3: ::core::ffi::c_uint,
        arg4: ::core::ffi::c_uint,
        arg5: *mut *mut rrsetinfo,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn freerrset(arg1: *mut rrsetinfo);
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct servent {
    pub s_name: *mut ::core::ffi::c_char,
    pub s_aliases: *mut *mut ::core::ffi::c_char,
    pub s_port: ::core::ffi::c_int,
    pub s_proto: *mut ::core::ffi::c_char,
}
unsafe extern "C" {
    pub fn getservbyname(
        arg1: *const ::core::ffi::c_char,
        arg2: *const ::core::ffi::c_char,
    ) -> *mut servent;
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct stat {
    pub st_dev: dev_t,
    pub st_ino: ino_t,
    pub st_mode: mode_t,
    pub st_nlink: nlink_t,
    pub st_uid: uid_t,
    pub st_gid: gid_t,
    pub st_rdev: dev_t,
    pub st_size: off_t,
    pub st_atim: timespec,
    pub st_mtim: timespec,
    pub st_ctim: timespec,
    pub st_blksize: blksize_t,
    pub st_blocks: blkcnt_t,
    pub st_spare4: [::core::ffi::c_long; 2usize],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct stat64 {
    pub st_dev: __dev_t,
    pub __pad1: ::core::ffi::c_uint,
    pub __st_ino: __ino_t,
    pub st_mode: __mode_t,
    pub st_nlink: __nlink_t,
    pub st_uid: __uid_t,
    pub st_gid: __gid_t,
    pub st_rdev: __dev_t,
    pub __pad2: ::core::ffi::c_uint,
    pub st_size: __off64_t,
    pub st_blksize: __blksize_t,
    pub st_blocks: __blkcnt64_t,
    pub st_atim: timespec,
    pub st_mtim: timespec,
    pub st_ctim: timespec,
    pub st_ino: __ino64_t,
}
unsafe extern "C" {
    pub fn chmod(__path: *const ::core::ffi::c_char, __mode: mode_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fchmod(__fd: ::core::ffi::c_int, __mode: mode_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fstat(__fd: ::core::ffi::c_int, __sbuf: *mut stat) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn mkdir(_path: *const ::core::ffi::c_char, __mode: mode_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn mkfifo(__path: *const ::core::ffi::c_char, __mode: mode_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn stat(__path: *const ::core::ffi::c_char, __sbuf: *mut stat) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn umask(__mask: mode_t) -> mode_t;
}
unsafe extern "C" {
    pub fn stat64(__path: *const ::core::ffi::c_char, __sbuf: *mut stat64) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fstat64(__fd: ::core::ffi::c_int, __sbuf: *mut stat64) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn lstat(__path: *const ::core::ffi::c_char, __buf: *mut stat) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn mknod(
        __path: *const ::core::ffi::c_char,
        __mode: mode_t,
        __dev: dev_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fchmodat(
        arg1: ::core::ffi::c_int,
        arg2: *const ::core::ffi::c_char,
        arg3: mode_t,
        arg4: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fstatat(
        arg1: ::core::ffi::c_int,
        arg2: *const ::core::ffi::c_char,
        arg3: *mut stat,
        arg4: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn mkdirat(
        arg1: ::core::ffi::c_int,
        arg2: *const ::core::ffi::c_char,
        arg3: mode_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn mkfifoat(
        arg1: ::core::ffi::c_int,
        arg2: *const ::core::ffi::c_char,
        arg3: mode_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn mknodat(
        arg1: ::core::ffi::c_int,
        arg2: *const ::core::ffi::c_char,
        arg3: mode_t,
        arg4: dev_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn utimensat(
        arg1: ::core::ffi::c_int,
        arg2: *const ::core::ffi::c_char,
        arg3: *const timespec,
        arg4: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn futimens(arg1: ::core::ffi::c_int, arg2: *const timespec) -> ::core::ffi::c_int;
}
pub type __sigset_t = ::core::ffi::c_ulong;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct timeval {
    pub tv_sec: time_t,
    pub tv_usec: suseconds_t,
}
pub type __fd_mask = ::core::ffi::c_ulong;
pub type fd_mask = __fd_mask;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct fd_set {
    pub __fds_bits: [__fd_mask; 8usize],
}
unsafe extern "C" {
    pub fn select(
        __n: ::core::ffi::c_int,
        __readfds: *mut fd_set,
        __writefds: *mut fd_set,
        __exceptfds: *mut fd_set,
        __timeout: *mut timeval,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn pselect(
        __n: ::core::ffi::c_int,
        __readfds: *mut fd_set,
        __writefds: *mut fd_set,
        __exceptfds: *mut fd_set,
        __timeout: *const timespec,
        __set: *const sigset_t,
    ) -> ::core::ffi::c_int;
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct itimerval {
    pub it_interval: timeval,
    pub it_value: timeval,
}
unsafe extern "C" {
    pub fn getitimer(__which: ::core::ffi::c_int, __value: *mut itimerval) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn gettimeofday(__p: *mut timeval, __tz: *mut ::core::ffi::c_void) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn setitimer(
        __which: ::core::ffi::c_int,
        __value: *const itimerval,
        __ovalue: *mut itimerval,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn utimes(arg1: *const ::core::ffi::c_char, arg2: *const timeval) -> ::core::ffi::c_int;
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct timezone {
    pub tz_minuteswest: ::core::ffi::c_int,
    pub tz_dsttime: ::core::ffi::c_int,
}
unsafe extern "C" {
    pub fn adjtime(arg1: *const timeval, arg2: *mut timeval) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn futimes(arg1: ::core::ffi::c_int, arg2: *const timeval) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn lutimes(arg1: *const ::core::ffi::c_char, arg2: *const timeval) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn settimeofday(arg1: *const timeval, arg2: *const timezone) -> ::core::ffi::c_int;
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct bintime {
    pub sec: time_t,
    pub frac: __uint64_t,
}
unsafe extern "C" {
    pub fn futimesat(
        arg1: ::core::ffi::c_int,
        arg2: *const ::core::ffi::c_char,
        arg3: *const timeval,
    ) -> ::core::ffi::c_int;
}
pub type rlim_t = ::core::ffi::c_ulong;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct rlimit {
    pub rlim_cur: rlim_t,
    pub rlim_max: rlim_t,
}
unsafe extern "C" {
    pub fn getrlimit(arg1: ::core::ffi::c_int, arg2: *mut rlimit) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn setrlimit(arg1: ::core::ffi::c_int, arg2: *const rlimit) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn getpriority(arg1: ::core::ffi::c_int, arg2: id_t) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn setpriority(
        arg1: ::core::ffi::c_int,
        arg2: id_t,
        arg3: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn wait(arg1: *mut ::core::ffi::c_int) -> pid_t;
}
unsafe extern "C" {
    pub fn waitpid(arg1: pid_t, arg2: *mut ::core::ffi::c_int, arg3: ::core::ffi::c_int) -> pid_t;
}
unsafe extern "C" {
    pub fn mmap(
        arg1: *mut ::core::ffi::c_void,
        arg2: usize,
        arg3: ::core::ffi::c_int,
        arg4: ::core::ffi::c_int,
        arg5: ::core::ffi::c_int,
        arg6: off_t,
    ) -> *mut ::core::ffi::c_void;
}
unsafe extern "C" {
    pub fn munmap(arg1: *mut ::core::ffi::c_void, arg2: usize) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn mprotect(
        arg1: *mut ::core::ffi::c_void,
        arg2: usize,
        arg3: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn lume_mmap_aligned(
        arg1: *mut ::core::ffi::c_void,
        arg2: usize,
        arg3: ::core::ffi::c_int,
        arg4: ::core::ffi::c_int,
        arg5: ::core::ffi::c_int,
        arg6: off_t,
        arg7: usize,
    ) -> *mut ::core::ffi::c_void;
}
unsafe extern "C" {
    pub fn madvise(
        arg1: *mut ::core::ffi::c_void,
        arg2: usize,
        arg3: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn lume_clear_cache(arg1: *mut ::core::ffi::c_void, arg2: usize) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn lume_jit_write_begin() -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn lume_jit_write_end() -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn posix_madvise(
        arg1: *mut ::core::ffi::c_void,
        arg2: usize,
        arg3: ::core::ffi::c_int,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn memfd_create(
        arg1: *const ::core::ffi::c_char,
        arg2: ::core::ffi::c_uint,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn shm_open(
        arg1: *const ::core::ffi::c_char,
        arg2: ::core::ffi::c_int,
        arg3: mode_t,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn shm_unlink(arg1: *const ::core::ffi::c_char) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn lume_shm_duplicate(
        arg1: ::core::ffi::c_int,
        arg2: ::core::ffi::c_int,
        arg3: ::core::ffi::c_uint,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn mlock(arg1: *const ::core::ffi::c_void, arg2: usize) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn munlock(arg1: *const ::core::ffi::c_void, arg2: usize) -> ::core::ffi::c_int;
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct iovec {
    pub iov_base: *mut ::core::ffi::c_void,
    pub iov_len: usize,
}
unsafe extern "C" {
    pub fn writev(arg1: ::core::ffi::c_int, arg2: *const iovec, arg3: ::core::ffi::c_int) -> isize;
}
unsafe extern "C" {
    pub fn readv(arg1: ::core::ffi::c_int, arg2: *const iovec, arg3: ::core::ffi::c_int) -> isize;
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct utsname {
    pub sysname: [::core::ffi::c_char; 65usize],
    pub nodename: [::core::ffi::c_char; 65usize],
    pub release: [::core::ffi::c_char; 65usize],
    pub version: [::core::ffi::c_char; 65usize],
    pub machine: [::core::ffi::c_char; 65usize],
}
unsafe extern "C" {
    pub fn uname(arg1: *mut utsname) -> ::core::ffi::c_int;
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct statvfs {
    pub f_bsize: ::core::ffi::c_ulong,
    pub f_frsize: ::core::ffi::c_ulong,
    pub f_blocks: fsblkcnt_t,
    pub f_bfree: fsblkcnt_t,
    pub f_bavail: fsblkcnt_t,
    pub f_files: fsfilcnt_t,
    pub f_ffree: fsfilcnt_t,
    pub f_favail: fsfilcnt_t,
    pub f_fsid: ::core::ffi::c_ulong,
    pub f_flag: ::core::ffi::c_ulong,
    pub f_namemax: ::core::ffi::c_ulong,
}
unsafe extern "C" {
    pub fn statvfs(arg1: *const ::core::ffi::c_char, arg2: *mut statvfs) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn fstatvfs(arg1: ::core::ffi::c_int, arg2: *mut statvfs) -> ::core::ffi::c_int;
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct winsize {
    pub ws_row: ::core::ffi::c_ushort,
    pub ws_col: ::core::ffi::c_ushort,
    pub ws_xpixel: ::core::ffi::c_ushort,
    pub ws_ypixel: ::core::ffi::c_ushort,
}
unsafe extern "C" {
    pub fn ioctl(arg1: ::core::ffi::c_int, arg2: ::core::ffi::c_ulong, ...) -> ::core::ffi::c_int;
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct in_addr {
    pub s_addr: in_addr_t,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct sockaddr_in {
    pub sin_family: sa_family_t,
    pub sin_port: in_port_t,
    pub sin_addr: in_addr,
    pub sin_zero: [::core::ffi::c_uchar; 8usize],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct in6_addr {
    pub s6_addr: [::core::ffi::c_uchar; 16usize],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct sockaddr_in6 {
    pub sin6_family: sa_family_t,
    pub sin6_port: in_port_t,
    pub sin6_flowinfo: u32,
    pub sin6_addr: in6_addr,
    pub sin6_scope_id: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ip_mreq {
    pub imr_multiaddr: in_addr,
    pub imr_interface: in_addr,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ipv6_mreq {
    pub ipv6mr_multiaddr: in6_addr,
    pub ipv6mr_interface: ::core::ffi::c_uint,
}
unsafe extern "C" {
    pub static in6addr_any: in6_addr;
}
unsafe extern "C" {
    pub static in6addr_loopback: in6_addr;
}
unsafe extern "C" {
    pub fn inet_pton(
        arg1: ::core::ffi::c_int,
        arg2: *const ::core::ffi::c_char,
        arg3: *mut ::core::ffi::c_void,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn inet_aton(arg1: *const ::core::ffi::c_char, arg2: *mut in_addr) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn inet_ntop(
        arg1: ::core::ffi::c_int,
        arg2: *const ::core::ffi::c_void,
        arg3: *mut ::core::ffi::c_char,
        arg4: socklen_t,
    ) -> *const ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn inet_ntoa(arg1: in_addr) -> *mut ::core::ffi::c_char;
}
unsafe extern "C" {
    pub fn inet_addr(arg1: *const ::core::ffi::c_char) -> in_addr_t;
}
pub const LUME_ACTIVITY_NETWORK: lume_activity_kind = 0;
pub const LUME_ACTIVITY_WIFI: lume_activity_kind = 1;
pub const LUME_ACTIVITY_ETHERNET: lume_activity_kind = 2;
pub const LUME_ACTIVITY_INPUT: lume_activity_kind = 3;
pub const LUME_ACTIVITY_USB: lume_activity_kind = 4;
pub const LUME_ACTIVITY_STORAGE: lume_activity_kind = 5;
pub const LUME_ACTIVITY_IRQ: lume_activity_kind = 6;
pub const LUME_ACTIVITY_WORKER: lume_activity_kind = 7;
pub const LUME_ACTIVITY_COUNT: lume_activity_kind = 8;
pub type lume_activity_kind = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_activity_snapshot {
    pub version: u32,
    pub bytes: u32,
    pub enabled: u32,
    pub core_count: u32,
    pub clock_hz: u64,
    pub counter: u64,
    pub rows: [[lume_activity_snapshot__bindgen_ty_1; 8usize]; 4usize],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_activity_snapshot__bindgen_ty_1 {
    pub ticks: u64,
    pub calls: u64,
}
pub const LUME_GPU_QUERY: _bindgen_ty_4 = 0;
pub const LUME_GPU_BIND: _bindgen_ty_4 = 1;
pub const LUME_GPU_COPY: _bindgen_ty_4 = 2;
pub type _bindgen_ty_4 = ::core::ffi::c_uint;
pub const LUME_GPU_BACKEND_NONE: _bindgen_ty_5 = 0;
pub const LUME_GPU_BACKEND_V3D71: _bindgen_ty_5 = 1;
pub const LUME_GPU_BACKEND_QEMU2D: _bindgen_ty_5 = 2;
pub type _bindgen_ty_5 = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_gpu_rect {
    pub source_x: u32,
    pub source_y: u32,
    pub target_x: u32,
    pub target_y: u32,
    pub width: u32,
    pub height: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_gpu_request {
    pub version: u32,
    pub bytes: u32,
    pub fd: i32,
    pub flags: u32,
    pub pixels: u64,
    pub width: u32,
    pub height: u32,
    pub pitch: u32,
    pub count: u32,
    pub rectangles: u64,
    pub capabilities: u64,
    pub backend: u32,
    pub reserved: u32,
}
pub const LUME_GRAPHICS_QUERY: lume_graphics_action = 0;
pub const LUME_GRAPHICS_ACQUIRE: lume_graphics_action = 1;
pub const LUME_GRAPHICS_PRESENT: lume_graphics_action = 2;
pub type lume_graphics_action = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_graphics_request {
    pub version: u32,
    pub bytes: u32,
    pub fd: i32,
    pub format: u32,
    pub width: u32,
    pub height: u32,
    pub pitch: u32,
    pub x: u32,
    pub y: u32,
    pub reserved: u32,
    pub pixels: u64,
    pub reserved2: u64,
}
pub const LUME_INPUT_KEY: lume_input_type = 1;
pub const LUME_INPUT_POINTER: lume_input_type = 2;
pub const LUME_INPUT_RESET: lume_input_type = 3;
pub type lume_input_type = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_input_event {
    pub type_: u32,
    pub code: u32,
    pub x: i32,
    pub y: i32,
    pub buttons: u32,
    pub milliseconds: u32,
}
pub const LUME_HARDWARE_BOARD: lume_hardware_origin = 1;
pub const LUME_HARDWARE_DT: lume_hardware_origin = 2;
pub const LUME_HARDWARE_ACPI: lume_hardware_origin = 3;
pub const LUME_HARDWARE_PCI: lume_hardware_origin = 4;
pub const LUME_HARDWARE_USB: lume_hardware_origin = 5;
pub const LUME_HARDWARE_VIRTUAL: lume_hardware_origin = 6;
pub type lume_hardware_origin = ::core::ffi::c_uint;
pub const LUME_HARDWARE_DISCOVERED: lume_hardware_state = 0;
pub const LUME_HARDWARE_DISABLED: lume_hardware_state = 1;
pub const LUME_HARDWARE_UNSUPPORTED: lume_hardware_state = 2;
pub const LUME_HARDWARE_READY: lume_hardware_state = 3;
pub const LUME_HARDWARE_ABSENT: lume_hardware_state = 4;
pub const LUME_HARDWARE_DEFERRED: lume_hardware_state = 5;
pub const LUME_HARDWARE_FAILED: lume_hardware_state = 6;
pub type lume_hardware_state = ::core::ffi::c_uint;
pub const LUME_HARDWARE_ID_DT: lume_hardware_identity_kind = 1;
pub const LUME_HARDWARE_ID_ACPI: lume_hardware_identity_kind = 2;
pub const LUME_HARDWARE_ID_PCI: lume_hardware_identity_kind = 3;
pub const LUME_HARDWARE_ID_USB: lume_hardware_identity_kind = 4;
pub const LUME_HARDWARE_ID_BOARD: lume_hardware_identity_kind = 5;
pub type lume_hardware_identity_kind = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_hardware_identity {
    pub kind: u32,
    pub reserved: u32,
    pub value: [::core::ffi::c_char; 96usize],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_hardware_resource_info {
    pub base: u64,
    pub size: u64,
    pub kind: u32,
    pub flags: u32,
    pub domain: u32,
    pub reserved: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_hardware_info {
    pub version: u32,
    pub id: u32,
    pub parent: u32,
    pub origin: u32,
    pub state: u32,
    pub identity_count: u32,
    pub resource_count: u32,
    pub reserved: u32,
    pub error: i32,
    pub references: u32,
    pub name: [::core::ffi::c_char; 64usize],
    pub path: [::core::ffi::c_char; 160usize],
    pub driver: [::core::ffi::c_char; 64usize],
    pub identities: [lume_hardware_identity; 4usize],
    pub resources: [lume_hardware_resource_info; 16usize],
}
pub const LUME_HARDWARE_LIST: lume_hardware_operation = 0;
pub const LUME_HARDWARE_GET: lume_hardware_operation = 1;
pub const LUME_HARDWARE_BIND: lume_hardware_operation = 2;
pub const LUME_HARDWARE_UNBIND: lume_hardware_operation = 3;
pub const LUME_HARDWARE_READ_REGISTER: lume_hardware_operation = 4;
pub const LUME_HARDWARE_DRIVER: lume_hardware_operation = 5;
pub const LUME_HARDWARE_RESCAN: lume_hardware_operation = 6;
pub const LUME_HARDWARE_EJECT: lume_hardware_operation = 7;
pub type lume_hardware_operation = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_hardware_request {
    pub version: u32,
    pub operation: u32,
    pub id: u32,
    pub index: u32,
    pub resource: u32,
    pub width: u32,
    pub reserved: [u32; 2usize],
    pub offset: u64,
    pub value: u64,
    pub driver: [::core::ffi::c_char; 64usize],
    pub info: lume_hardware_info,
}
pub const LUME_MODULE_LIST: _bindgen_ty_6 = 1;
pub const LUME_MODULE_LOAD: _bindgen_ty_6 = 2;
pub const LUME_MODULE_UNLOAD: _bindgen_ty_6 = 3;
pub const LUME_MODULE_KERNEL: _bindgen_ty_6 = 4;
pub const LUME_MODULE_INSPECT: _bindgen_ty_6 = 5;
pub type _bindgen_ty_6 = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_module_info {
    pub id: u32,
    pub references: u32,
    pub drivers: u32,
    pub devices: u32,
    pub bytes: u64,
    pub name: [::core::ffi::c_char; 64usize],
    pub kernel_build: [::core::ffi::c_uchar; 20usize],
    pub reserved: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_module_request {
    pub version: u32,
    pub operation: u32,
    pub index: u32,
    pub id: u32,
    pub file: u64,
    pub bytes: u64,
    pub info: lume_module_info,
}
pub const LUME_SPAWN_CLOSE: _bindgen_ty_7 = 1;
pub const LUME_SPAWN_DUP2: _bindgen_ty_7 = 2;
pub const LUME_SPAWN_OPEN: _bindgen_ty_7 = 3;
pub const LUME_SPAWN_CHDIR: _bindgen_ty_7 = 4;
pub type _bindgen_ty_7 = ::core::ffi::c_uint;
pub const LUME_SPAWN_SIGDEFAULT: _bindgen_ty_8 = 1;
pub const LUME_SPAWN_SIGMASK: _bindgen_ty_8 = 2;
pub const LUME_SPAWN_SETSID: _bindgen_ty_8 = 4;
pub const LUME_SPAWN_CLOEXEC_DEFAULT: _bindgen_ty_8 = 8;
pub type _bindgen_ty_8 = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_spawn_action {
    pub path: u64,
    pub operation: u32,
    pub fd: u32,
    pub target: u32,
    pub flags: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_spawn_actions_request {
    pub path: u64,
    pub argv: u64,
    pub envp: u64,
    pub argc: u32,
    pub envc: u32,
    pub actions: u64,
    pub count: u32,
    pub version: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_spawn_options_request {
    pub base: lume_spawn_actions_request,
    pub flags: u32,
    pub signal_default: u32,
    pub signal_mask: u32,
    pub reserved: u32,
}
pub const LUME_THREAD_CREATE: _bindgen_ty_9 = 0;
pub const LUME_THREAD_EXIT: _bindgen_ty_9 = 1;
pub const LUME_THREAD_JOIN: _bindgen_ty_9 = 2;
pub const LUME_THREAD_DETACH: _bindgen_ty_9 = 3;
pub const LUME_THREAD_SELF: _bindgen_ty_9 = 4;
pub const LUME_THREAD_WAIT: _bindgen_ty_9 = 5;
pub const LUME_THREAD_WAKE: _bindgen_ty_9 = 6;
pub const LUME_THREAD_STACK: _bindgen_ty_9 = 7;
pub const LUME_THREAD_NAME: _bindgen_ty_9 = 8;
pub const LUME_THREAD_WAIT_REALTIME: _bindgen_ty_9 = 9;
pub type _bindgen_ty_9 = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_thread_request {
    pub entry: u64,
    pub argument: u64,
    pub stack_bytes: u64,
    pub tid: u64,
    pub value: u64,
    pub address: u64,
    pub timeout_ms: u64,
}
pub const LUME_TTY_CREATE: lume_tty_action = 0;
pub const LUME_TTY_GET: lume_tty_action = 1;
pub const LUME_TTY_SET: lume_tty_action = 2;
pub const LUME_TTY_SIZE: lume_tty_action = 3;
pub const LUME_TTY_RESIZE: lume_tty_action = 4;
pub const LUME_TTY_CONTROLLING: lume_tty_action = 5;
pub const LUME_TTY_GROUP: lume_tty_action = 6;
pub const LUME_TTY_SET_GROUP: lume_tty_action = 7;
pub const LUME_TTY_FLUSH: lume_tty_action = 8;
pub const LUME_TTY_AVAILABLE: lume_tty_action = 9;
pub const LUME_TTY_NAME: lume_tty_action = 10;
pub const LUME_TTY_OPEN: lume_tty_action = 11;
pub type lume_tty_action = ::core::ffi::c_uint;
pub const LUME_TTY_ISIG: _bindgen_ty_10 = 1;
pub const LUME_TTY_ICANON: _bindgen_ty_10 = 2;
pub const LUME_TTY_ECHO: _bindgen_ty_10 = 8;
pub const LUME_TTY_ECHOE: _bindgen_ty_10 = 16;
pub const LUME_TTY_ECHOK: _bindgen_ty_10 = 32;
pub const LUME_TTY_ECHONL: _bindgen_ty_10 = 64;
pub const LUME_TTY_TOSTOP: _bindgen_ty_10 = 256;
pub const LUME_TTY_ISTRIP: _bindgen_ty_10 = 32;
pub const LUME_TTY_INLCR: _bindgen_ty_10 = 64;
pub const LUME_TTY_IGNCR: _bindgen_ty_10 = 128;
pub const LUME_TTY_ICRNL: _bindgen_ty_10 = 256;
pub const LUME_TTY_IXON: _bindgen_ty_10 = 1024;
pub const LUME_TTY_OPOST: _bindgen_ty_10 = 1;
pub const LUME_TTY_ONLCR: _bindgen_ty_10 = 4;
pub type _bindgen_ty_10 = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_termios {
    pub input: u32,
    pub output: u32,
    pub control: u32,
    pub local: u32,
    pub characters: [::core::ffi::c_uchar; 32usize],
    pub input_speed: u32,
    pub output_speed: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_tty_request {
    pub version: u32,
    pub bytes: u32,
    pub fd: i32,
    pub other: i32,
    pub value: u32,
    pub reserved: u32,
    pub termios: lume_termios,
    pub rows: u32,
    pub columns: u32,
    pub xpixel: u32,
    pub ypixel: u32,
}
pub const LUME_VIDEO_LEGACY_MODES: lume_video_action = 0;
pub const LUME_VIDEO_LEGACY_SELECT: lume_video_action = 1;
pub const LUME_VIDEO_UPLOAD: lume_video_action = 2;
pub const LUME_VIDEO_FRAMES: lume_video_action = 3;
pub const LUME_VIDEO_PIXELS: lume_video_action = 4;
pub const LUME_VIDEO_STATUS: lume_video_action = 5;
pub const LUME_VIDEO_PITCH: lume_video_action = 6;
pub const LUME_VIDEO_DEPTH: lume_video_action = 7;
pub const LUME_VIDEO_MEMORY: lume_video_action = 8;
pub const LUME_VIDEO_MODE_COUNT: lume_video_action = 10;
pub const LUME_VIDEO_MODE_PIXELS: lume_video_action = 11;
pub const LUME_VIDEO_MODE_SELECT: lume_video_action = 13;
pub const LUME_VIDEO_FONT_COUNT: lume_video_action = 14;
pub const LUME_VIDEO_FONT_CELL: lume_video_action = 15;
pub const LUME_VIDEO_FONT_FACE: lume_video_action = 16;
pub const LUME_VIDEO_FONT_SELECT: lume_video_action = 17;
pub const LUME_VIDEO_FONT_CURRENT: lume_video_action = 18;
pub const LUME_VIDEO_CELLS: lume_video_action = 19;
pub type lume_video_action = ::core::ffi::c_uint;
pub const LUME_VIDEO_LATIN: lume_video_face = 0;
pub const LUME_VIDEO_BOLD: lume_video_face = 1;
pub const LUME_VIDEO_COMPACT: lume_video_face = 2;
pub const LUME_VIDEO_CUSTOM: lume_video_face = 3;
pub type lume_video_face = ::core::ffi::c_uint;
pub const LUME_SYS_ABI: lume_syscall = 0;
pub const LUME_SYS_EXIT: lume_syscall = 1;
pub const LUME_SYS_READ: lume_syscall = 2;
pub const LUME_SYS_WRITE: lume_syscall = 3;
pub const LUME_SYS_OPEN: lume_syscall = 4;
pub const LUME_SYS_CLOSE: lume_syscall = 5;
pub const LUME_SYS_SEEK: lume_syscall = 6;
pub const LUME_SYS_STAT: lume_syscall = 7;
pub const LUME_SYS_GETDENTS: lume_syscall = 8;
pub const LUME_SYS_CHDIR: lume_syscall = 9;
pub const LUME_SYS_GETCWD: lume_syscall = 10;
pub const LUME_SYS_GETPID: lume_syscall = 11;
pub const LUME_SYS_RUN: lume_syscall = 12;
pub const LUME_SYS_DUP2: lume_syscall = 13;
pub const LUME_SYS_MKDIR: lume_syscall = 14;
pub const LUME_SYS_UNLINK: lume_syscall = 15;
pub const LUME_SYS_RMDIR: lume_syscall = 16;
pub const LUME_SYS_NET: lume_syscall = 17;
pub const LUME_SYS_NETCTL: lume_syscall = 18;
pub const LUME_SYS_CLOCK: lume_syscall = 19;
pub const LUME_SYS_SLEEP: lume_syscall = 20;
pub const LUME_SYS_SERVICE: lume_syscall = 21;
pub const LUME_SYS_STREAM: lume_syscall = 22;
pub const LUME_SYS_BOOT: lume_syscall = 23;
pub const LUME_SYS_RUN_ENV: lume_syscall = 24;
pub const LUME_SYS_FS: lume_syscall = 25;
pub const LUME_SYS_BRK: lume_syscall = 26;
pub const LUME_SYS_PROCESS: lume_syscall = 27;
pub const LUME_SYS_MEMORY: lume_syscall = 28;
pub const LUME_SYS_TELEMETRY: lume_syscall = 29;
pub const LUME_SYS_TERMINAL: lume_syscall = 30;
pub const LUME_SYS_PROBE: lume_syscall = 31;
pub const LUME_SYS_WAIT: lume_syscall = 32;
pub const LUME_SYS_FD: lume_syscall = 33;
pub const LUME_SYS_RUN_FDS: lume_syscall = 34;
pub const LUME_SYS_FEATURES: lume_syscall = 35;
pub const LUME_SYS_RUN_ACTIONS: lume_syscall = 36;
pub const LUME_SYS_CAP_DROP: lume_syscall = 37;
pub const LUME_SYS_CPU: lume_syscall = 38;
pub const LUME_SYS_THREAD: lume_syscall = 39;
pub const LUME_SYS_HARDWARE: lume_syscall = 40;
pub const LUME_SYS_MODULE: lume_syscall = 41;
pub const LUME_SYS_SUPERVISE: lume_syscall = 42;
pub const LUME_SYS_FORK: lume_syscall = 43;
pub const LUME_SYS_EXEC: lume_syscall = 44;
pub const LUME_SYS_RANDOM: lume_syscall = 45;
pub const LUME_SYS_SOCKET: lume_syscall = 46;
pub const LUME_SYS_CREDENTIAL: lume_syscall = 47;
pub const LUME_SYS_SIGNAL: lume_syscall = 48;
pub const LUME_SYS_FILE_MODE: lume_syscall = 49;
pub const LUME_SYS_TLS: lume_syscall = 50;
pub const LUME_SYS_VIDEO: lume_syscall = 51;
pub const LUME_SYS_GRAPHICS: lume_syscall = 52;
pub const LUME_SYS_TTY: lume_syscall = 53;
pub const LUME_SYS_REALTIME: lume_syscall = 54;
pub const LUME_SYS_GPU: lume_syscall = 55;
pub const LUME_SYS_VM: lume_syscall = 56;
pub const LUME_SYS_EXEPATH: lume_syscall = 57;
pub const LUME_SYS_SANDBOX: lume_syscall = 58;
pub const LUME_SYS_FILE_IO: lume_syscall = 59;
pub const LUME_SYS_FILES: lume_syscall = 60;
pub type lume_syscall = ::core::ffi::c_uint;
pub const LUME_EPERM: _bindgen_ty_11 = 1;
pub const LUME_ENOENT: _bindgen_ty_11 = 2;
pub const LUME_EINTR: _bindgen_ty_11 = 4;
pub const LUME_EIO: _bindgen_ty_11 = 5;
pub const LUME_EBADF: _bindgen_ty_11 = 9;
pub const LUME_EAGAIN: _bindgen_ty_11 = 11;
pub const LUME_ENOMEM: _bindgen_ty_11 = 12;
pub const LUME_EACCES: _bindgen_ty_11 = 13;
pub const LUME_EFAULT: _bindgen_ty_11 = 14;
pub const LUME_EBUSY: _bindgen_ty_11 = 16;
pub const LUME_EEXIST: _bindgen_ty_11 = 17;
pub const LUME_ENOTDIR: _bindgen_ty_11 = 20;
pub const LUME_EISDIR: _bindgen_ty_11 = 21;
pub const LUME_EINVAL: _bindgen_ty_11 = 22;
pub const LUME_EMFILE: _bindgen_ty_11 = 24;
pub const LUME_ETXTBSY: _bindgen_ty_11 = 26;
pub const LUME_ENOSPC: _bindgen_ty_11 = 28;
pub const LUME_ESPIPE: _bindgen_ty_11 = 29;
pub const LUME_EROFS: _bindgen_ty_11 = 30;
pub const LUME_EPIPE: _bindgen_ty_11 = 32;
pub const LUME_ENAMETOOLONG: _bindgen_ty_11 = 36;
pub const LUME_ENOSYS: _bindgen_ty_11 = 38;
pub const LUME_E2BIG: _bindgen_ty_11 = 7;
pub const LUME_ENOEXEC: _bindgen_ty_11 = 8;
pub const LUME_ENOTEMPTY: _bindgen_ty_11 = 39;
pub const LUME_EOVERFLOW: _bindgen_ty_11 = 75;
pub const LUME_ENOTSOCK: _bindgen_ty_11 = 88;
pub const LUME_EDESTADDRREQ: _bindgen_ty_11 = 89;
pub const LUME_EMSGSIZE: _bindgen_ty_11 = 90;
pub const LUME_EPROTOTYPE: _bindgen_ty_11 = 91;
pub const LUME_ENOPROTOOPT: _bindgen_ty_11 = 92;
pub const LUME_EPROTONOSUPPORT: _bindgen_ty_11 = 93;
pub const LUME_ESOCKTNOSUPPORT: _bindgen_ty_11 = 94;
pub const LUME_EOPNOTSUPP: _bindgen_ty_11 = 95;
pub const LUME_EPFNOSUPPORT: _bindgen_ty_11 = 96;
pub const LUME_EAFNOSUPPORT: _bindgen_ty_11 = 97;
pub const LUME_EADDRINUSE: _bindgen_ty_11 = 98;
pub const LUME_EADDRNOTAVAIL: _bindgen_ty_11 = 99;
pub const LUME_ENETDOWN: _bindgen_ty_11 = 100;
pub const LUME_ENETUNREACH: _bindgen_ty_11 = 101;
pub const LUME_ENETRESET: _bindgen_ty_11 = 102;
pub const LUME_ECONNABORTED: _bindgen_ty_11 = 103;
pub const LUME_ECONNRESET: _bindgen_ty_11 = 104;
pub const LUME_ENOBUFS: _bindgen_ty_11 = 105;
pub const LUME_EISCONN: _bindgen_ty_11 = 106;
pub const LUME_ENOTCONN: _bindgen_ty_11 = 107;
pub const LUME_ESHUTDOWN: _bindgen_ty_11 = 108;
pub const LUME_ETOOMANYREFS: _bindgen_ty_11 = 109;
pub const LUME_ETIMEDOUT: _bindgen_ty_11 = 110;
pub const LUME_ECONNREFUSED: _bindgen_ty_11 = 111;
pub const LUME_EHOSTDOWN: _bindgen_ty_11 = 112;
pub const LUME_EHOSTUNREACH: _bindgen_ty_11 = 113;
pub const LUME_EALREADY: _bindgen_ty_11 = 114;
pub const LUME_EINPROGRESS: _bindgen_ty_11 = 115;
pub type _bindgen_ty_11 = ::core::ffi::c_uint;
pub const LUME_O_RDONLY: _bindgen_ty_12 = 0;
pub const LUME_O_WRONLY: _bindgen_ty_12 = 1;
pub const LUME_O_RDWR: _bindgen_ty_12 = 2;
pub const LUME_O_CREAT: _bindgen_ty_12 = 256;
pub const LUME_O_TRUNC: _bindgen_ty_12 = 512;
pub const LUME_O_APPEND: _bindgen_ty_12 = 1024;
pub const LUME_O_EXCL: _bindgen_ty_12 = 2048;
pub type _bindgen_ty_12 = ::core::ffi::c_uint;
pub const LUME_SEEK_SET: _bindgen_ty_13 = 0;
pub const LUME_SEEK_CUR: _bindgen_ty_13 = 1;
pub const LUME_SEEK_END: _bindgen_ty_13 = 2;
pub type _bindgen_ty_13 = ::core::ffi::c_uint;
pub const LUME_NODE_FILE: _bindgen_ty_14 = 1;
pub const LUME_NODE_DIR: _bindgen_ty_14 = 2;
pub const LUME_NODE_CHAR: _bindgen_ty_14 = 3;
pub const LUME_NODE_PIPE: _bindgen_ty_14 = 4;
pub const LUME_NODE_BLOCK: _bindgen_ty_14 = 5;
pub const LUME_NODE_SOCKET: _bindgen_ty_14 = 6;
pub type _bindgen_ty_14 = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_stat {
    pub inode: u64,
    pub size: u64,
    pub kind: u32,
    pub reserved: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_dirent {
    pub inode: u64,
    pub kind: u32,
    pub reserved: u32,
    pub name: [::core::ffi::c_char; 48usize],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_net_request {
    pub argument: [::core::ffi::c_char; 128usize],
    pub output: [::core::ffi::c_char; 3072usize],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_net_route {
    pub ifindex: u32,
    pub flags: u32,
    pub destination: [u8; 4usize],
    pub netmask: [u8; 4usize],
    pub gateway: [u8; 4usize],
    pub metric: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_net_neighbor {
    pub ifindex: u32,
    pub flags: u32,
    pub address: [u8; 4usize],
    pub mac: [u8; 6usize],
    pub reserved: [u8; 2usize],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_net_connection {
    pub protocol: u32,
    pub state: u32,
    pub owner: u32,
    pub handle: u32,
    pub local: [u8; 4usize],
    pub remote: [u8; 4usize],
    pub local_port: u32,
    pub remote_port: u32,
    pub receive_bytes: u64,
    pub send_bytes: u64,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_net_ipv4 {
    pub fields: u32,
    pub address: [u8; 4usize],
    pub netmask: [u8; 4usize],
    pub gateway: [u8; 4usize],
}
pub const LUME_IF_ETHERNET: _bindgen_ty_15 = 1;
pub const LUME_IF_WIFI: _bindgen_ty_15 = 2;
pub type _bindgen_ty_15 = ::core::ffi::c_uint;
pub const LUME_IF_UP: _bindgen_ty_16 = 1;
pub const LUME_IF_LINK: _bindgen_ty_16 = 2;
pub const LUME_IF_DHCP: _bindgen_ty_16 = 4;
pub type _bindgen_ty_16 = ::core::ffi::c_uint;
pub const LUME_WIFI_STATION: _bindgen_ty_17 = 1;
pub const LUME_WIFI_SCAN: _bindgen_ty_17 = 2;
pub const LUME_WIFI_WPA2_CCMP: _bindgen_ty_17 = 4;
pub type _bindgen_ty_17 = ::core::ffi::c_uint;
pub const LUME_WIFI_BAND_2GHZ: _bindgen_ty_18 = 1;
pub const LUME_WIFI_BAND_5GHZ: _bindgen_ty_18 = 2;
pub type _bindgen_ty_18 = ::core::ffi::c_uint;
pub const LUME_BSS_OPEN: _bindgen_ty_19 = 0;
pub const LUME_BSS_PRIVACY: _bindgen_ty_19 = 1;
pub const LUME_BSS_RSN: _bindgen_ty_19 = 2;
pub const LUME_BSS_WPA: _bindgen_ty_19 = 4;
pub type _bindgen_ty_19 = ::core::ffi::c_uint;
pub const LUME_WIFI_DOWN: _bindgen_ty_20 = 0;
pub const LUME_WIFI_DISCONNECTED: _bindgen_ty_20 = 1;
pub const LUME_WIFI_SCANNING: _bindgen_ty_20 = 2;
pub const LUME_WIFI_CONNECTING: _bindgen_ty_20 = 3;
pub const LUME_WIFI_CONNECTED: _bindgen_ty_20 = 4;
pub const LUME_WIFI_FAILED: _bindgen_ty_20 = 5;
pub type _bindgen_ty_20 = ::core::ffi::c_uint;
pub const LUME_EVENT_REGISTER: _bindgen_ty_21 = 1;
pub const LUME_EVENT_ADMIN: _bindgen_ty_21 = 2;
pub const LUME_EVENT_LINK: _bindgen_ty_21 = 3;
pub const LUME_EVENT_SCAN: _bindgen_ty_21 = 4;
pub const LUME_EVENT_CONNECTION: _bindgen_ty_21 = 5;
pub const LUME_EVENT_IPV4: _bindgen_ty_21 = 6;
pub const LUME_EVENT_UNREGISTER: _bindgen_ty_21 = 7;
pub type _bindgen_ty_21 = ::core::ffi::c_uint;
pub const LUME_NET_ENUM: lume_net_control_op = 0;
pub const LUME_NET_GET: lume_net_control_op = 1;
pub const LUME_NET_ADMIN: lume_net_control_op = 2;
pub const LUME_NET_WIFI_CAPS: lume_net_control_op = 3;
pub const LUME_NET_WIFI_STATE: lume_net_control_op = 4;
pub const LUME_NET_WIFI_SCAN: lume_net_control_op = 5;
pub const LUME_NET_WIFI_CONNECT: lume_net_control_op = 6;
pub const LUME_NET_WIFI_DISCONNECT: lume_net_control_op = 7;
pub const LUME_NET_EVENTS: lume_net_control_op = 8;
pub const LUME_NET_DHCP_START: lume_net_control_op = 9;
pub const LUME_NET_DHCP_BEGIN: lume_net_control_op = 10;
pub const LUME_NET_WIFI_CONNECT_BEGIN: lume_net_control_op = 11;
pub const LUME_NET_STATS: lume_net_control_op = 12;
pub const LUME_NET_ROUTES: lume_net_control_op = 13;
pub const LUME_NET_NEIGHBORS: lume_net_control_op = 14;
pub const LUME_NET_CONNECTIONS: lume_net_control_op = 15;
pub const LUME_NET_IPV4: lume_net_control_op = 16;
pub const LUME_NET_ROUTE_ADD: lume_net_control_op = 17;
pub const LUME_NET_ROUTE_DELETE: lume_net_control_op = 18;
pub type lume_net_control_op = ::core::ffi::c_uint;
pub const LUME_NET_TX_DEPTH: lume_net_account_event = 0;
pub const LUME_NET_TX_BUSY: lume_net_account_event = 1;
pub const LUME_NET_TX_POLL: lume_net_account_event = 2;
pub const LUME_NET_TX_COMPLETE: lume_net_account_event = 3;
pub const LUME_NET_TX_DMA_ERROR: lume_net_account_event = 4;
pub const LUME_NET_TX_TIMEOUT: lume_net_account_event = 5;
pub const LUME_NET_TX_SUBMIT_ERROR: lume_net_account_event = 6;
pub const LUME_NET_TX_CANCELLED: lume_net_account_event = 7;
pub const LUME_NET_RX_BUDGET: lume_net_account_event = 8;
pub const LUME_NET_TX_RESTART: lume_net_account_event = 9;
pub const LUME_NET_WIFI_CREDIT: lume_net_account_event = 10;
pub const LUME_NET_WIFI_CREDIT_STALL: lume_net_account_event = 11;
pub const LUME_NET_WIFI_RX_DEPTH: lume_net_account_event = 12;
pub const LUME_NET_WIFI_RX_DROP: lume_net_account_event = 13;
pub const LUME_NET_WIFI_PACKETS: lume_net_account_event = 14;
pub const LUME_NET_WIFI_BUS_ERROR: lume_net_account_event = 15;
pub const LUME_NET_WIFI_POLL_MS: lume_net_account_event = 16;
pub const LUME_NET_WIFI_SEND_MS: lume_net_account_event = 17;
pub type lume_net_account_event = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_net_statistics {
    pub version: u32,
    pub bytes: u32,
    pub polls: u64,
    pub rx_bytes: u64,
    pub tx_bytes: u64,
    pub tx_busy: u64,
    pub tx_polls: u64,
    pub tx_completed: u64,
    pub tx_dma_errors: u64,
    pub tx_timeouts: u64,
    pub tx_submit_errors: u64,
    pub tx_cancelled: u64,
    pub tx_depth: u64,
    pub tx_high_water: u64,
    pub tx_completion_ms: u64,
    pub tx_max_completion_ms: u64,
    pub rx_budget_exhausted: u64,
    pub tx_restarts: u64,
    pub wifi_credit_available: u64,
    pub wifi_credit_stalls: u64,
    pub wifi_rx_high_water: u64,
    pub wifi_rx_drops: u64,
    pub wifi_packets: u64,
    pub wifi_bus_errors: u64,
    pub wifi_poll_ms: u64,
    pub wifi_send_ms: u64,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_netif_info {
    pub ifindex: u32,
    pub kind: u32,
    pub flags: u32,
    pub mtu: u32,
    pub name: [::core::ffi::c_char; 16usize],
    pub mac: [u8; 6usize],
    pub reserved: [u8; 2usize],
    pub rx_packets: u64,
    pub tx_packets: u64,
    pub rx_errors: u64,
    pub tx_errors: u64,
    pub address: [u8; 4usize],
    pub netmask: [u8; 4usize],
    pub gateway: [u8; 4usize],
    pub dns: [u8; 4usize],
    pub address_family: u32,
    pub reserved2: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_wifi_caps {
    pub features: u32,
    pub bands: u32,
    pub max_scan_results: u32,
    pub max_ssid_bytes: u32,
    pub country: [::core::ffi::c_char; 2usize],
    pub reserved: [u8; 14usize],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_wifi_bss_info {
    pub bssid: [u8; 6usize],
    pub channel: u16,
    pub rssi: i16,
    pub security: u16,
    pub ssid_bytes: u32,
    pub ssid: [u8; 32usize],
    pub reserved: [u8; 16usize],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_wifi_state {
    pub state: u32,
    pub reason: u32,
    pub bssid: [u8; 6usize],
    pub ssid_bytes: u8,
    pub reserved: u8,
    pub ssid: [u8; 32usize],
    pub reserved2: [u8; 16usize],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_wifi_connect {
    pub ssid_bytes: u32,
    pub key_bytes: u32,
    pub ssid: [u8; 32usize],
    pub key: [u8; 64usize],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_net_event {
    pub sequence: u64,
    pub ifindex: u32,
    pub type_: u32,
    pub state: u32,
    pub reason: u32,
    pub value: u64,
    pub reserved: [u8; 32usize],
}
#[repr(C)]
#[derive(Copy, Clone)]
pub struct lume_net_control {
    pub version: u32,
    pub ifindex: u32,
    pub cursor: u64,
    pub count: u32,
    pub lost: u32,
    pub reserved: u64,
    pub data: lume_net_control__bindgen_ty_1,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union lume_net_control__bindgen_ty_1 {
    pub interfaces: [lume_netif_info; 4usize],
    pub capabilities: lume_wifi_caps,
    pub wireless: lume_wifi_state,
    pub scan: [lume_wifi_bss_info; 32usize],
    pub connect: lume_wifi_connect,
    pub events: [lume_net_event; 16usize],
    pub statistics: lume_net_statistics,
    pub routes: [lume_net_route; 32usize],
    pub neighbors: [lume_net_neighbor; 32usize],
    pub connections: [lume_net_connection; 32usize],
    pub ipv4: lume_net_ipv4,
    pub admin_up: u32,
}
pub const LUME_CREDENTIAL_GET: _bindgen_ty_22 = 0;
pub const LUME_CREDENTIAL_UID: _bindgen_ty_22 = 1;
pub const LUME_CREDENTIAL_GID: _bindgen_ty_22 = 2;
pub const LUME_CREDENTIAL_GROUPS: _bindgen_ty_22 = 3;
pub const LUME_CREDENTIAL_CHROOT: _bindgen_ty_22 = 4;
pub const LUME_CREDENTIAL_LIMIT_GET: _bindgen_ty_22 = 5;
pub const LUME_CREDENTIAL_LIMIT_SET: _bindgen_ty_22 = 6;
pub const LUME_CREDENTIAL_UMASK: _bindgen_ty_22 = 7;
pub type _bindgen_ty_22 = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_credential_request {
    pub version: u32,
    pub bytes: u32,
    pub uid: u32,
    pub euid: u32,
    pub suid: u32,
    pub gid: u32,
    pub egid: u32,
    pub sgid: u32,
    pub groups_count: u32,
    pub reserved: u32,
    pub groups: [u32; 8usize],
    pub path: u64,
}
pub const LUME_LIMIT_FILES: _bindgen_ty_23 = 0;
pub const LUME_LIMIT_PROCESSES: _bindgen_ty_23 = 1;
pub const LUME_LIMIT_FILE_SIZE: _bindgen_ty_23 = 2;
pub const LUME_LIMIT_CORE_SIZE: _bindgen_ty_23 = 3;
pub const LUME_LIMIT_COUNT: _bindgen_ty_23 = 4;
pub type _bindgen_ty_23 = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_limit_request {
    pub version: u32,
    pub bytes: u32,
    pub resource: u32,
    pub reserved: u32,
    pub soft: u64,
    pub hard: u64,
}
pub const LUME_FILE_IO_PREAD: _bindgen_ty_24 = 0;
pub const LUME_FILE_IO_PWRITE: _bindgen_ty_24 = 1;
pub const LUME_FILE_IO_SEEK: _bindgen_ty_24 = 2;
pub const LUME_FILE_IO_TRUNCATE: _bindgen_ty_24 = 3;
pub const LUME_FILE_IO_OFD_GETLK: _bindgen_ty_24 = 4;
pub const LUME_FILE_IO_OFD_SETLK: _bindgen_ty_24 = 5;
pub const LUME_FILE_IO_POSIX_GETLK: _bindgen_ty_24 = 6;
pub const LUME_FILE_IO_POSIX_SETLK: _bindgen_ty_24 = 7;
pub const LUME_FILE_IO_FLOCK_NB: _bindgen_ty_24 = 8;
pub const LUME_FILE_IO_READV: _bindgen_ty_24 = 9;
pub const LUME_FILE_IO_WRITEV: _bindgen_ty_24 = 10;
pub type _bindgen_ty_24 = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_io_vector {
    pub buffer: u64,
    pub bytes: u64,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_file_io_request {
    pub version: u32,
    pub bytes: u32,
    pub fd: i32,
    pub whence: u32,
    pub buffer: u64,
    pub count: u64,
    pub offset: i64,
}
pub const LUME_FILE_LOCK_READ: _bindgen_ty_25 = 1;
pub const LUME_FILE_LOCK_WRITE: _bindgen_ty_25 = 2;
pub const LUME_FILE_LOCK_UNLOCK: _bindgen_ty_25 = 3;
pub type _bindgen_ty_25 = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_file_lock_request {
    pub version: u32,
    pub bytes: u32,
    pub fd: i32,
    pub type_: u32,
    pub whence: u32,
    pub reserved: u32,
    pub start: i64,
    pub length: i64,
    pub pid: i32,
    pub reserved2: u32,
}
pub const LUME_FILE_MODE_PATH: _bindgen_ty_26 = 0;
pub const LUME_FILE_MODE_FD: _bindgen_ty_26 = 1;
pub const LUME_FILE_MODE_OPEN: _bindgen_ty_26 = 2;
pub const LUME_FILE_MODE_MKDIR: _bindgen_ty_26 = 3;
pub type _bindgen_ty_26 = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_file_mode_request {
    pub version: u32,
    pub bytes: u32,
    pub path: u64,
    pub fd: i32,
    pub mode: u32,
    pub flags: u32,
    pub descriptor_flags: u32,
    pub uid: u32,
    pub gid: u32,
}
pub const LUME_STREAM_CONNECT: _bindgen_ty_27 = 0;
pub const LUME_STREAM_SEND: _bindgen_ty_27 = 1;
pub const LUME_STREAM_RECEIVE: _bindgen_ty_27 = 2;
pub const LUME_STREAM_CLOSE: _bindgen_ty_27 = 3;
pub const LUME_STREAM_CONNECT_ASYNC: _bindgen_ty_27 = 4;
pub const LUME_STREAM_POLL: _bindgen_ty_27 = 5;
pub const LUME_STREAM_SHUTDOWN_WRITE: _bindgen_ty_27 = 6;
pub const LUME_STREAM_LISTEN: _bindgen_ty_27 = 7;
pub const LUME_STREAM_ACCEPT: _bindgen_ty_27 = 8;
pub const LUME_DATAGRAM_CONNECT: _bindgen_ty_27 = 9;
pub const LUME_DATAGRAM_LISTEN: _bindgen_ty_27 = 10;
pub const LUME_ICMP_ECHO_START: _bindgen_ty_27 = 11;
pub const LUME_ICMP_ECHO_POLL: _bindgen_ty_27 = 12;
pub const LUME_ICMP_ECHO_CLOSE: _bindgen_ty_27 = 13;
pub type _bindgen_ty_27 = ::core::ffi::c_uint;
pub const LUME_STREAM_READABLE: _bindgen_ty_28 = 1;
pub const LUME_STREAM_WRITABLE: _bindgen_ty_28 = 2;
pub const LUME_STREAM_ERROR: _bindgen_ty_28 = 4;
pub const LUME_STREAM_HANGUP: _bindgen_ty_28 = 8;
pub type _bindgen_ty_28 = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_stream_request {
    pub version: u32,
    pub handle: u32,
    pub count: u32,
    pub port: u32,
    pub address: [::core::ffi::c_uchar; 4usize],
    pub reserved: u32,
    pub data: [::core::ffi::c_uchar; 4096usize],
}
pub const LUME_BOOT_INFO: _bindgen_ty_29 = 0;
pub const LUME_BOOT_BEGIN: _bindgen_ty_29 = 1;
pub const LUME_BOOT_WRITE: _bindgen_ty_29 = 2;
pub const LUME_BOOT_VERIFY: _bindgen_ty_29 = 3;
pub const LUME_BOOT_ACTIVATE: _bindgen_ty_29 = 4;
pub const LUME_BOOT_ABORT: _bindgen_ty_29 = 5;
pub const LUME_BOOT_CAPABILITIES: _bindgen_ty_29 = 6;
pub const LUME_BOOT_CONFIRM: _bindgen_ty_29 = 7;
pub const LUME_BOOT_SELECT_PARTITION: _bindgen_ty_29 = 8;
pub type _bindgen_ty_29 = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_boot_request {
    pub version: u32,
    pub count: u32,
    pub total: u32,
    pub offset: u32,
    pub crc: u32,
    pub active: u32,
    pub data: [::core::ffi::c_uchar; 4096usize],
}
pub const LUME_VM_MAP: lume_vm_operation = 0;
pub const LUME_VM_UNMAP: lume_vm_operation = 1;
pub const LUME_VM_PROTECT: lume_vm_operation = 2;
pub const LUME_VM_ADVISE: lume_vm_operation = 3;
pub const LUME_VM_CREATE: lume_vm_operation = 4;
pub const LUME_VM_RESIZE: lume_vm_operation = 5;
pub const LUME_VM_OPEN: lume_vm_operation = 6;
pub const LUME_VM_UNLINK: lume_vm_operation = 7;
pub const LUME_VM_DUPLICATE: lume_vm_operation = 8;
pub const LUME_VM_ADD_SEALS: lume_vm_operation = 9;
pub const LUME_VM_GET_SEALS: lume_vm_operation = 10;
pub const LUME_VM_INFO: lume_vm_operation = 11;
pub const LUME_VM_SYNC: lume_vm_operation = 12;
pub const LUME_VM_JIT_BEGIN: lume_vm_operation = 13;
pub const LUME_VM_JIT_END: lume_vm_operation = 14;
pub type lume_vm_operation = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_vm_request {
    pub version: u32,
    pub flags: u32,
    pub address: u64,
    pub length: u64,
    pub offset: u64,
    pub alignment: u64,
    pub fd: i32,
    pub protection: u32,
    pub advice: u32,
    pub reserved: u32,
    pub name: [::core::ffi::c_char; 128usize],
}
pub const LUME_NET_STATUS: _bindgen_ty_30 = 0;
pub const LUME_NET_SCAN: _bindgen_ty_30 = 1;
pub const LUME_NET_JOIN: _bindgen_ty_30 = 2;
pub const LUME_NET_LEAVE: _bindgen_ty_30 = 3;
pub const LUME_NET_DHCP: _bindgen_ty_30 = 4;
pub const LUME_NET_PING: _bindgen_ty_30 = 5;
pub const LUME_NET_DNS: _bindgen_ty_30 = 6;
pub const LUME_NET_TCP: _bindgen_ty_30 = 7;
pub type _bindgen_ty_30 = ::core::ffi::c_uint;
pub const LUME_CAP_NET_ADMIN: _bindgen_ty_31 = 1;
pub const LUME_CAP_BOOT_UPDATE: _bindgen_ty_31 = 2;
pub const LUME_CAP_SERVICE: _bindgen_ty_31 = 4;
pub const LUME_CAP_DRIVER_ADMIN: _bindgen_ty_31 = 8;
pub const LUME_CAP_HW_DIAGNOSTIC: _bindgen_ty_31 = 16;
pub const LUME_CAP_SUPERVISOR: _bindgen_ty_31 = 32;
pub const LUME_CAP_SERVICE_CONTROL: _bindgen_ty_31 = 64;
pub const LUME_CAP_CLOCK_ADMIN: _bindgen_ty_31 = 128;
pub type _bindgen_ty_31 = ::core::ffi::c_uint;
pub const LUME_FEATURE_FDS: _bindgen_ty_32 = 1;
pub const LUME_FEATURE_SPAWN_ACTIONS: _bindgen_ty_32 = 2;
pub const LUME_FEATURE_CAPABILITIES: _bindgen_ty_32 = 4;
pub const LUME_FEATURE_THREADS: _bindgen_ty_32 = 8;
pub const LUME_FEATURE_SMP: _bindgen_ty_32 = 16;
pub const LUME_FEATURE_HARDWARE: _bindgen_ty_32 = 32;
pub const LUME_FEATURE_MODULES: _bindgen_ty_32 = 64;
pub const LUME_FEATURE_SUPERVISION: _bindgen_ty_32 = 128;
pub const LUME_FEATURE_FORK: _bindgen_ty_32 = 256;
pub const LUME_FEATURE_EXEC: _bindgen_ty_32 = 512;
pub const LUME_FEATURE_RANDOM: _bindgen_ty_32 = 1024;
pub const LUME_FEATURE_SOCKETS: _bindgen_ty_32 = 2048;
pub const LUME_FEATURE_CREDENTIALS: _bindgen_ty_32 = 4096;
pub const LUME_FEATURE_SIGNALS: _bindgen_ty_32 = 8192;
pub const LUME_FEATURE_FILE_MODES: _bindgen_ty_32 = 16384;
pub const LUME_FEATURE_EVENT_WAIT: _bindgen_ty_32 = 32768;
pub const LUME_FEATURE_SIGNAL_WAIT: _bindgen_ty_32 = 65536;
pub const LUME_FEATURE_SIMD_CONTEXT: _bindgen_ty_32 = 131072;
pub type _bindgen_ty_32 = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_system_features {
    pub abi: u64,
    pub features: u64,
    pub pointer_bits: u32,
    pub compat_pointer_bits: u32,
    pub fd_limit: u32,
    pub task_limit: u32,
    pub ddk_version: u32,
    pub boot_profile: u32,
    pub capabilities: u64,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_run_request {
    pub path: u64,
    pub argv: u64,
    pub envp: u64,
    pub argc: u32,
    pub envc: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_task_info {
    pub pid: u32,
    pub parent: u32,
    pub uid: u32,
    pub state: u32,
    pub runs: u32,
    pub cores: u32,
    pub name: [::core::ffi::c_char; 24usize],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_memory_snapshot {
    pub total_pages: u64,
    pub free_pages: u64,
    pub process_pages: u64,
    pub heap_base: u64,
    pub heap_end: u64,
    pub heap_limit: u64,
}
pub const LUME_WAIT_BLOCK: _bindgen_ty_33 = 1;
pub const LUME_WAIT_STOPPED: _bindgen_ty_33 = 2;
pub const LUME_WAIT_CONTINUED: _bindgen_ty_33 = 4;
pub const LUME_WAIT_ORPHANED: _bindgen_ty_33 = 8;
pub type _bindgen_ty_33 = ::core::ffi::c_uint;
pub const LUME_FS_SYNC: _bindgen_ty_34 = 0;
pub const LUME_FS_TRUNCATE: _bindgen_ty_34 = 1;
pub const LUME_FS_RENAME: _bindgen_ty_34 = 2;
pub const LUME_FS_STAT_PATH: _bindgen_ty_34 = 3;
pub const LUME_FS_STAT_FD: _bindgen_ty_34 = 4;
pub const LUME_FS_COMPAT_OPEN: _bindgen_ty_34 = 5;
pub const LUME_FS_COMPAT_REMOVE: _bindgen_ty_34 = 6;
pub const LUME_FS_STAT_INODE: _bindgen_ty_34 = 7;
pub const LUME_FS_REPLACE: _bindgen_ty_34 = 8;
pub type _bindgen_ty_34 = ::core::ffi::c_uint;
pub const LUME_REALTIME_GET: _bindgen_ty_35 = 0;
pub const LUME_REALTIME_SET: _bindgen_ty_35 = 1;
pub type _bindgen_ty_35 = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_realtime_request {
    pub version: u32,
    pub bytes: u32,
    pub seconds: u64,
    pub nanoseconds: u32,
    pub flags: u32,
    pub reserved: u64,
}
pub const LUME_SANDBOX_QUERY: lume_sandbox_operation = 0;
pub const LUME_SANDBOX_ENTER: lume_sandbox_operation = 1;
pub const LUME_SANDBOX_DIRECTORY_MINT: lume_sandbox_operation = 16;
pub const LUME_SANDBOX_DIRECTORY_ACTIVATE: lume_sandbox_operation = 17;
pub const LUME_SANDBOX_DIRECTORY_RELEASE: lume_sandbox_operation = 18;
pub const LUME_SANDBOX_X11_AUTHORITY: lume_sandbox_operation = 32;
pub type lume_sandbox_operation = ::core::ffi::c_uint;
pub const LUME_SANDBOX_NONE: lume_sandbox_profile = 0;
pub const LUME_SANDBOX_RENDERER: lume_sandbox_profile = 1;
pub const LUME_SANDBOX_UTILITY: lume_sandbox_profile = 2;
pub const LUME_SANDBOX_NETWORK: lume_sandbox_profile = 3;
pub const LUME_SANDBOX_GPU_SOFTWARE: lume_sandbox_profile = 4;
pub type lume_sandbox_profile = ::core::ffi::c_uint;
pub const LUME_SERVICE_GET: _bindgen_ty_36 = 0;
pub const LUME_SERVICE_SEND: _bindgen_ty_36 = 1;
pub const LUME_SERVICE_RECEIVE: _bindgen_ty_36 = 2;
pub const LUME_SERVICE_PUBLISH: _bindgen_ty_36 = 3;
pub const LUME_SERVICE_SUBMIT: _bindgen_ty_36 = 4;
pub const LUME_SERVICE_RESULT: _bindgen_ty_36 = 5;
pub const LUME_SERVICE_WAIT: _bindgen_ty_36 = 6;
pub const LUME_SERVICE_CANCEL: _bindgen_ty_36 = 7;
pub type _bindgen_ty_36 = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_service_result {
    pub version: u32,
    pub bytes: u32,
    pub sequence: u64,
    pub result: i32,
    pub reserved: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_service_wait {
    pub version: u32,
    pub bytes: u32,
    pub sequence: u64,
    pub timeout_ms: u64,
    pub signal_epoch: u64,
    pub flags: u32,
    pub reserved: u32,
}
pub const NETD_IMPORT: _bindgen_ty_37 = 1;
pub const NETD_FORGET: _bindgen_ty_37 = 2;
pub const NETD_PREFER: _bindgen_ty_37 = 3;
pub const NETD_AUTO_ON: _bindgen_ty_37 = 4;
pub const NETD_AUTO_OFF: _bindgen_ty_37 = 5;
pub const NETD_CONNECT: _bindgen_ty_37 = 6;
pub const NETD_PAUSE: _bindgen_ty_37 = 7;
pub const NETD_RECONNECT: _bindgen_ty_37 = 8;
pub const NETD_RELOAD: _bindgen_ty_37 = 9;
pub const NETD_IMPORT_FILE: _bindgen_ty_37 = 10;
pub type _bindgen_ty_37 = ::core::ffi::c_uint;
pub const NETD_STARTING: _bindgen_ty_38 = 0;
pub const NETD_OFF: _bindgen_ty_38 = 1;
pub const NETD_WAITING: _bindgen_ty_38 = 2;
pub const NETD_CONNECTING: _bindgen_ty_38 = 3;
pub const NETD_ONLINE: _bindgen_ty_38 = 4;
pub const NETD_ERROR: _bindgen_ty_38 = 5;
pub type _bindgen_ty_38 = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_service_message {
    pub sequence: u64,
    pub version: u32,
    pub command: u32,
    pub name: [::core::ffi::c_char; 16usize],
    pub reserved: [u8; 32usize],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_net_service_status {
    pub version: u32,
    pub pid: u32,
    pub state: u32,
    pub ifindex: u32,
    pub generation: u64,
    pub acknowledged: u64,
    pub retry_at: u64,
    pub result: i32,
    pub attempts: u32,
    pub automatic: u32,
    pub profiles: u32,
    pub active: [::core::ffi::c_char; 16usize],
    pub profile: [lume_net_service_status__bindgen_ty_1; 4usize],
    pub reserved: [u8; 344usize],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_net_service_status__bindgen_ty_1 {
    pub name: [::core::ffi::c_char; 16usize],
    pub priority: u32,
    pub reserved: u32,
}
pub const LUME_SIGNAL_ACTION: _bindgen_ty_39 = 0;
pub const LUME_SIGNAL_MASK: _bindgen_ty_39 = 1;
pub const LUME_SIGNAL_SEND: _bindgen_ty_39 = 2;
pub const LUME_SIGNAL_TIMER: _bindgen_ty_39 = 3;
pub const LUME_SIGNAL_RETURN: _bindgen_ty_39 = 4;
pub const LUME_SIGNAL_PARENT: _bindgen_ty_39 = 5;
pub const LUME_SIGNAL_SESSION: _bindgen_ty_39 = 6;
pub const LUME_SIGNAL_GROUP: _bindgen_ty_39 = 7;
pub const LUME_SIGNAL_SET_SESSION: _bindgen_ty_39 = 8;
pub const LUME_SIGNAL_SET_GROUP: _bindgen_ty_39 = 9;
pub const LUME_SIGNAL_TRY_WAIT: _bindgen_ty_39 = 10;
pub const LUME_SIGNAL_WAIT: _bindgen_ty_39 = 11;
pub type _bindgen_ty_39 = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_signal_request {
    pub version: u32,
    pub bytes: u32,
    pub number: u32,
    pub flags: u32,
    pub handler: u64,
    pub restorer: u64,
    pub delay_ms: u64,
    pub interval_ms: u64,
    pub mask: u32,
    pub operation: u32,
    pub pid: i32,
    pub group: i32,
}
pub const LUME_SOCKET_CREATE: _bindgen_ty_40 = 0;
pub const LUME_SOCKET_PAIR: _bindgen_ty_40 = 1;
pub const LUME_SOCKET_BIND: _bindgen_ty_40 = 2;
pub const LUME_SOCKET_LISTEN: _bindgen_ty_40 = 3;
pub const LUME_SOCKET_ACCEPT: _bindgen_ty_40 = 4;
pub const LUME_SOCKET_CONNECT: _bindgen_ty_40 = 5;
pub const LUME_SOCKET_LOCAL: _bindgen_ty_40 = 6;
pub const LUME_SOCKET_PEER: _bindgen_ty_40 = 7;
pub const LUME_SOCKET_SHUTDOWN: _bindgen_ty_40 = 8;
pub const LUME_SOCKET_SEND_FD: _bindgen_ty_40 = 9;
pub const LUME_SOCKET_RECEIVE_FD: _bindgen_ty_40 = 10;
pub const LUME_SOCKET_OPTION: _bindgen_ty_40 = 11;
pub const LUME_SOCKET_PENDING: _bindgen_ty_40 = 12;
pub const LUME_SOCKET_CREDENTIALS: _bindgen_ty_40 = 13;
pub const LUME_SOCKET_SEND_MESSAGE: _bindgen_ty_40 = 14;
pub const LUME_SOCKET_RECEIVE_MESSAGE: _bindgen_ty_40 = 15;
pub const LUME_SOCKET_ERROR: _bindgen_ty_40 = 16;
pub const LUME_SOCKET_SENDTO: _bindgen_ty_40 = 17;
pub const LUME_SOCKET_RECVFROM: _bindgen_ty_40 = 18;
pub type _bindgen_ty_40 = ::core::ffi::c_uint;
pub const LUME_SOCKET_OPTION_TYPE: _bindgen_ty_41 = 0;
pub const LUME_SOCKET_OPTION_RECEIVE_BUFFER: _bindgen_ty_41 = 8;
pub const LUME_SOCKET_OPTION_SEND_BUFFER: _bindgen_ty_41 = 16;
pub const LUME_SOCKET_OPTION_RECEIVE_TOS: _bindgen_ty_41 = 32;
pub const LUME_SOCKET_OPTION_TOS: _bindgen_ty_41 = 64;
pub type _bindgen_ty_41 = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_socket_credentials {
    pub version: u32,
    pub bytes: u32,
    pub uid: u32,
    pub gid: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_socket_request {
    pub version: u32,
    pub bytes: u32,
    pub fd: i32,
    pub other: i32,
    pub family: u32,
    pub type_: u32,
    pub flags: u32,
    pub port: u32,
    pub address: [::core::ffi::c_uchar; 4usize],
    pub reserved: u32,
    pub buffer: u64,
    pub count: u32,
    pub reserved2: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_socket_message {
    pub version: u32,
    pub bytes: u32,
    pub buffer: u64,
    pub count: u32,
    pub flags: u32,
    pub rights_count: u32,
    pub received_flags: u32,
    pub rights: [i32; 128usize],
}
pub const LUME_SV_REGISTER: _bindgen_ty_42 = 0;
pub const LUME_SV_SUBMIT: _bindgen_ty_42 = 1;
pub const LUME_SV_RECEIVE: _bindgen_ty_42 = 2;
pub const LUME_SV_REPLY: _bindgen_ty_42 = 3;
pub const LUME_SV_RESULT: _bindgen_ty_42 = 4;
pub const LUME_SV_CANCEL: _bindgen_ty_42 = 5;
pub const LUME_SV_STOP_CHILD: _bindgen_ty_42 = 6;
pub const LUME_SV_WAIT: _bindgen_ty_42 = 7;
pub type _bindgen_ty_42 = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_supervision_wait {
    pub version: u32,
    pub bytes: u32,
    pub sequence: u64,
    pub timeout_ms: u64,
    pub signal_epoch: u64,
    pub flags: u32,
    pub reserved: u32,
}
pub const LUME_SV_LIST: _bindgen_ty_43 = 0;
pub const LUME_SV_STATUS: _bindgen_ty_43 = 1;
pub const LUME_SV_START: _bindgen_ty_43 = 2;
pub const LUME_SV_STOP: _bindgen_ty_43 = 3;
pub const LUME_SV_RESTART: _bindgen_ty_43 = 4;
pub const LUME_SV_ENABLE: _bindgen_ty_43 = 5;
pub const LUME_SV_DISABLE: _bindgen_ty_43 = 6;
pub type _bindgen_ty_43 = ::core::ffi::c_uint;
pub const LUME_SV_STOPPED: _bindgen_ty_44 = 0;
pub const LUME_SV_RUNNING: _bindgen_ty_44 = 1;
pub const LUME_SV_STOPPING: _bindgen_ty_44 = 2;
pub const LUME_SV_BACKOFF: _bindgen_ty_44 = 3;
pub const LUME_SV_FAILED: _bindgen_ty_44 = 4;
pub type _bindgen_ty_44 = ::core::ffi::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_service_status {
    pub name: [::core::ffi::c_char; 24usize],
    pub pid: u32,
    pub state: u32,
    pub enabled: u32,
    pub restarts: u32,
    pub exit_code: i32,
    pub error: i32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lume_supervision_request {
    pub version: u32,
    pub command: u32,
    pub sequence: u64,
    pub name: [::core::ffi::c_char; 24usize],
    pub result: i32,
    pub count: u32,
    pub service: [lume_service_status; 8usize],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct sockaddr_un {
    pub sun_family: sa_family_t,
    pub sun_path: [::core::ffi::c_char; 108usize],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct passwd {
    pub pw_name: *mut ::core::ffi::c_char,
    pub pw_passwd: *mut ::core::ffi::c_char,
    pub pw_uid: uid_t,
    pub pw_gid: gid_t,
    pub pw_comment: *mut ::core::ffi::c_char,
    pub pw_gecos: *mut ::core::ffi::c_char,
    pub pw_dir: *mut ::core::ffi::c_char,
    pub pw_shell: *mut ::core::ffi::c_char,
}
unsafe extern "C" {
    pub fn getpwuid(arg1: uid_t) -> *mut passwd;
}
unsafe extern "C" {
    pub fn getpwnam(arg1: *const ::core::ffi::c_char) -> *mut passwd;
}
unsafe extern "C" {
    pub fn getpwnam_r(
        arg1: *const ::core::ffi::c_char,
        arg2: *mut passwd,
        arg3: *mut ::core::ffi::c_char,
        arg4: usize,
        arg5: *mut *mut passwd,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn getpwuid_r(
        arg1: uid_t,
        arg2: *mut passwd,
        arg3: *mut ::core::ffi::c_char,
        arg4: usize,
        arg5: *mut *mut passwd,
    ) -> ::core::ffi::c_int;
}
unsafe extern "C" {
    pub fn getpwent() -> *mut passwd;
}
unsafe extern "C" {
    pub fn setpwent();
}
unsafe extern "C" {
    pub fn endpwent();
}
unsafe extern "C" {
    pub fn setpassent(arg1: ::core::ffi::c_int) -> ::core::ffi::c_int;
}
pub type __builtin_va_list = *mut ::core::ffi::c_char;
