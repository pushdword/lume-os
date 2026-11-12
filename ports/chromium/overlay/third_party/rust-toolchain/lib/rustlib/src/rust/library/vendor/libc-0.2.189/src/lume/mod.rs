use crate::prelude::*;

#[cfg(not(any(target_arch = "aarch64", target_arch = "x86_64", target_arch = "x86")))]
core::compile_error!("Lume libc bindings require AArch64, x86_64 or x86");

#[cfg(target_arch = "aarch64")]
#[allow(missing_copy_implementations, missing_debug_implementations)]
mod bindings;
#[cfg(target_arch = "x86_64")]
#[path = "bindings_x86_64.rs"]
#[allow(missing_copy_implementations, missing_debug_implementations)]
mod bindings;
#[cfg(target_arch = "x86")]
#[path = "bindings_i686.rs"]
#[allow(missing_copy_implementations, missing_debug_implementations)]
mod bindings;
pub use self::bindings::*;
pub use self::bindings::{fstat64 as fstat, stat64 as stat};

pub type ino_t = bindings::__ino64_t;
pub type blkcnt_t = bindings::__blkcnt64_t;

pub type size_t = usize;
pub type ssize_t = isize;
pub type ptrdiff_t = isize;
pub type intptr_t = isize;
pub type uintptr_t = usize;

pub const PTHREAD_STACK_MIN: usize = 32768;
pub const PTHREAD_ONCE_INIT: pthread_once_t = 0;
pub const PTHREAD_MUTEX_INITIALIZER: pthread_mutex_t = pthread_mutex_t {
    word: 0,
    type_: 0,
    depth: 0,
    reserved: 0,
    owner: 0,
};
pub const PTHREAD_COND_INITIALIZER: pthread_cond_t = pthread_cond_t {
    sequence: 0,
    waiters: 0,
    clock: CLOCK_REALTIME as i32,
};
pub const PTHREAD_RWLOCK_INITIALIZER: pthread_rwlock_t = pthread_rwlock_t {
    mutex: PTHREAD_MUTEX_INITIALIZER,
    changed: PTHREAD_COND_INITIALIZER,
    readers: 0,
    waiters: 0,
    writer: 0,
};
pub const MAP_FAILED: *mut core::ffi::c_void = usize::MAX as *mut core::ffi::c_void;
pub const SIG_DFL: sighandler_t = 0;
pub const SIG_IGN: sighandler_t = 1;
pub const SIG_ERR: sighandler_t = usize::MAX;
pub const O_ACCMODE: core::ffi::c_int = O_RDONLY | O_WRONLY | O_RDWR;

impl Default for timespec {
    fn default() -> Self {
        Self {
            tv_sec: 0,
            tv_nsec: 0,
        }
    }
}

unsafe extern "C" {
    #[link_name = "lstat64"]
    pub fn lstat(path: *const core::ffi::c_char, metadata: *mut stat) -> core::ffi::c_int;

    #[link_name = "fstatat64"]
    pub fn fstatat(
        directory: core::ffi::c_int,
        path: *const core::ffi::c_char,
        metadata: *mut stat,
        flags: core::ffi::c_int,
    ) -> core::ffi::c_int;

    pub fn renameat(
        source_directory: core::ffi::c_int,
        source: *const core::ffi::c_char,
        target_directory: core::ffi::c_int,
        target: *const core::ffi::c_char,
    ) -> core::ffi::c_int;

    pub fn pthread_create(
        thread: *mut pthread_t,
        attributes: *const pthread_attr_t,
        start: unsafe extern "C" fn(*mut core::ffi::c_void) -> *mut core::ffi::c_void,
        argument: *mut core::ffi::c_void,
    ) -> core::ffi::c_int;
}

#[inline]
pub fn errno_location() -> *mut core::ffi::c_int {
    &raw mut errno
}

pub const fn WIFEXITED(status: core::ffi::c_int) -> bool {
    status & 0xff == 0
}
pub const fn WIFSIGNALED(status: core::ffi::c_int) -> bool {
    status & 0x7f > 0 && status & 0x7f < 0x7f
}
pub const fn WIFSTOPPED(status: core::ffi::c_int) -> bool {
    status & 0xff == 0x7f
}
pub const fn WIFCONTINUED(status: core::ffi::c_int) -> bool {
    status == 0xffff
}
pub const fn WEXITSTATUS(status: core::ffi::c_int) -> core::ffi::c_int {
    (status >> 8) & 0xff
}
pub const fn WTERMSIG(status: core::ffi::c_int) -> core::ffi::c_int {
    status & 0x7f
}
pub const fn WSTOPSIG(status: core::ffi::c_int) -> core::ffi::c_int {
    WEXITSTATUS(status)
}
pub const fn WCOREDUMP(status: core::ffi::c_int) -> bool {
    status & 0x80 != 0
}

const fn cmsg_align(bytes: usize) -> usize {
    bytes.wrapping_add(core::mem::size_of::<size_t>() - 1) & !(core::mem::size_of::<size_t>() - 1)
}

pub const unsafe fn CMSG_SPACE(bytes: core::ffi::c_uint) -> core::ffi::c_uint {
    (cmsg_align(core::mem::size_of::<cmsghdr>()) + cmsg_align(bytes as usize)) as core::ffi::c_uint
}

pub const unsafe fn CMSG_LEN(bytes: core::ffi::c_uint) -> core::ffi::c_uint {
    (cmsg_align(core::mem::size_of::<cmsghdr>()) + bytes as usize) as core::ffi::c_uint
}

pub unsafe fn CMSG_DATA(header: *const cmsghdr) -> *mut core::ffi::c_uchar {
    unsafe {
        header
            .cast::<core::ffi::c_uchar>()
            .cast_mut()
            .add(cmsg_align(core::mem::size_of::<cmsghdr>()))
    }
}

pub unsafe fn CMSG_FIRSTHDR(message: *const msghdr) -> *mut cmsghdr {
    if unsafe { (*message).msg_controllen } < core::mem::size_of::<cmsghdr>() {
        core::ptr::null_mut()
    } else {
        unsafe { (*message).msg_control.cast() }
    }
}

pub unsafe fn CMSG_NXTHDR(message: *const msghdr, header: *const cmsghdr) -> *mut cmsghdr {
    let base = unsafe { (*message).msg_control as usize };
    let bytes = unsafe { (*message).msg_controllen };
    let current = header as usize;
    let length = unsafe { (*header).cmsg_len };
    if current < base
        || length < core::mem::size_of::<cmsghdr>()
        || length > usize::MAX - (core::mem::size_of::<size_t>() - 1)
    {
        return core::ptr::null_mut();
    }
    let core::option::Option::Some(next) = current.checked_add(cmsg_align(length)) else {
        return core::ptr::null_mut();
    };
    let core::option::Option::Some(end) = base.checked_add(bytes) else {
        return core::ptr::null_mut();
    };
    if next <= end && end - next >= core::mem::size_of::<cmsghdr>() {
        next as *mut cmsghdr
    } else {
        core::ptr::null_mut()
    }
}

pub unsafe fn FD_ZERO(set: *mut fd_set) {
    unsafe { (*set).__fds_bits.fill(0) };
}

pub unsafe fn FD_SET(fd: core::ffi::c_int, set: *mut fd_set) {
    let fd = fd as usize;
    unsafe {
        (*set).__fds_bits[fd / (8 * core::mem::size_of::<__fd_mask>())] |=
            (1 as __fd_mask) << (fd % (8 * core::mem::size_of::<__fd_mask>()))
    };
}

pub unsafe fn FD_CLR(fd: core::ffi::c_int, set: *mut fd_set) {
    let fd = fd as usize;
    unsafe {
        (*set).__fds_bits[fd / (8 * core::mem::size_of::<__fd_mask>())] &=
            !((1 as __fd_mask) << (fd % (8 * core::mem::size_of::<__fd_mask>())))
    };
}

pub unsafe fn FD_ISSET(fd: core::ffi::c_int, set: *const fd_set) -> bool {
    let fd = fd as usize;
    unsafe {
        (*set).__fds_bits[fd / (8 * core::mem::size_of::<__fd_mask>())]
            & ((1 as __fd_mask) << (fd % (8 * core::mem::size_of::<__fd_mask>())))
            != 0
    }
}

pub unsafe fn killpg(group: pid_t, signal: core::ffi::c_int) -> core::ffi::c_int {
    if group < 0 {
        unsafe { errno = EINVAL };
        return -1;
    }
    unsafe { kill(-group, signal) }
}

#[inline]
pub unsafe fn lume_exepath(buffer: *mut core::ffi::c_char, bytes: usize) -> isize {
    let result: i64;
    #[cfg(target_arch = "aarch64")]
    unsafe {
        core::arch::asm!(
            "svc {svc}",
            svc = const LUME_ABI_SVC,
            in("x8") LUME_SYS_EXEPATH as u64,
            inlateout("x0") 0u64 => result,
            in("x1") buffer,
            in("x2") bytes,
            options(nostack),
        );
    }
    #[cfg(target_arch = "x86_64")]
    unsafe {
        core::arch::asm!(
            "int 0x80",
            in("r8") LUME_SYS_EXEPATH as u64,
            inlateout("rdi") 0u64 => result,
            in("rsi") buffer,
            in("rdx") bytes,
            options(nostack),
        );
    }
    #[cfg(target_arch = "x86")]
    unsafe {
        let request = [
            LUME_SYS_EXEPATH as u64,
            0,
            buffer as usize as u64,
            bytes as u64,
        ];
        let low: u32;
        let high: u32;
        core::arch::asm!(
            "int 0x80",
            inlateout("eax") request.as_ptr() as usize => low,
            lateout("edx") high,
            options(nostack),
        );
        result = (((high as u64) << 32) | low as u64) as i64;
    }
    if result < 0 {
        unsafe extern "C" {
            fn lume_errno_from_syscall(error: core::ffi::c_uint) -> core::ffi::c_int;
        }
        unsafe { errno = lume_errno_from_syscall((-result) as core::ffi::c_uint) };
        -1
    } else {
        result as isize
    }
}
