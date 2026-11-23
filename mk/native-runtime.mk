ifndef LUME_NATIVE_RUNTIME_INCLUDED
LUME_NATIVE_RUNTIME_INCLUDED := 1
ifeq ($(filter aarch64 amd64 i686,$(ARCH)),)
$(error Native runtime requires ARCH=aarch64, ARCH=amd64 or ARCH=i686)
endif

NATIVE_CORE_SOURCES := libc/lume/api.c libc/runtime/runtime.c libc/lume/native.c \
    libc/posix/sbrk.c libc/posix/errno_map.c libc/lume/thread.c libc/posix/mman.c \
    arch/$(ARCH)/tls.c libc/runtime/locks.c libc/posix/pthread.c libc/runtime/init.c \
    libc/posix/atfork.c libc/posix/sigwait.c libc/posix/getrandom.c libc/posix/time.c \
    libc/posix/positioned_io.c libc/posix/semaphore.c libc/posix/file_lock.c \
    libc/posix/statvfs.c libc/posix/tty.c libc/posix/eventfd.c \
    ports/picolibc/native-stdio/flush.c ports/picolibc/native-stdio/fdopen.c \
    ports/picolibc/native-stdio/funopen.c ports/picolibc/native-stdio/fmemopen.c \
    ports/picolibc/native-stdio/fdevopen.c
ifneq ($(filter aarch64 i686,$(ARCH)),)
NATIVE_CORE_SOURCES += libc/string/transfer.c
endif
ifeq ($(ARCH),aarch64)
NATIVE_CORE_SOURCES += libc/posix/ucontext.c libc/runtime/ucontext.S
endif
ifeq ($(ARCH),i686)
NATIVE_CORE_SOURCES += arch/i686/builtins.c
endif

NATIVE_POSIX_SOURCES := libc/posix/fd.c libc/posix/process.c libc/posix/process_native.c \
    libc/posix/directory.c libc/posix/filesystem.c libc/posix/socket.c libc/posix/signals.c \
    libc/posix/main.c libc/posix/system_native.c libc/posix/uio.c libc/posix/path_native.c \
    libc/posix/accounts.c libc/posix/words.c vendor/posix-glob/glob.c \
    ports/standalone/file_mode.c ports/standalone/stream_pipe.c ports/standalone/limits.c \
    libc/posix/ppoll.c libc/posix/memstream.c libc/posix/dlfcn.c libc/posix/resource.c \
    libc/posix/syslog.c

NATIVE_MATH_SOURCES := ports/picolibc/platform/libm/common/math_denorm.c \
    ports/picolibc/platform/libm/common/math_denormf.c \
    ports/picolibc/platform/libm/common/math_denorml.c \
    ports/picolibc/platform/libm/ld/s_finitel.c vendor/picolibc/libm/ld/s_fpclassifyl.c
NATIVE_CXX_SOURCES := ports/llvm/cxxabi.cpp vendor/libcxxabi/src/private_typeinfo.cpp \
    vendor/libcxxabi/src/abort_message.cpp vendor/libcxxabi/src/cxa_demangle.cpp
NATIVE_LIBCXXABI_SOURCES := vendor/libcxxabi/src/cxa_aux_runtime.cpp \
    vendor/libcxxabi/src/cxa_default_handlers.cpp \
    vendor/libcxxabi/src/cxa_exception_storage.cpp \
    vendor/libcxxabi/src/cxa_handlers.cpp vendor/libcxxabi/src/cxa_vector.cpp \
    vendor/libcxxabi/src/fallback_malloc.cpp \
    vendor/libcxxabi/src/stdlib_exception.cpp \
    vendor/libcxxabi/src/stdlib_stdexcept.cpp \
    vendor/libcxxabi/src/stdlib_typeinfo.cpp \
    vendor/libcxxabi/src/cxa_exception.cpp \
    vendor/libcxxabi/src/cxa_personality.cpp
NATIVE_LIBUNWIND_CXX_SOURCES := vendor/libunwind/src/libunwind.cpp \
    vendor/libunwind/src/Unwind-EHABI.cpp vendor/libunwind/src/Unwind-seh.cpp
NATIVE_LIBUNWIND_C_SOURCES := vendor/libunwind/src/UnwindLevel1.c \
    vendor/libunwind/src/UnwindLevel1-gcc-ext.c \
    vendor/libunwind/src/Unwind-sjlj.c vendor/libunwind/src/Unwind-wasm.c
NATIVE_LIBUNWIND_ASM_SOURCES := vendor/libunwind/src/UnwindRegistersRestore.S \
    vendor/libunwind/src/UnwindRegistersSave.S
NATIVE_DESKTOP_C_SOURCES := $(NATIVE_CORE_SOURCES) $(NATIVE_POSIX_SOURCES) \
    ports/xorg-server/sha1.c vendor/openssh/openbsd-compat/sha1.c \
    ports/picolibc/native-allocator.c libc/posix/directory_sort.c \
    libc/posix/pause.c libc/posix/pthread_timed.c $(NATIVE_MATH_SOURCES)
NATIVE_DESKTOP_SOURCES := $(NATIVE_DESKTOP_C_SOURCES) arch/$(ARCH)/crt0.S \
    $(NATIVE_CXX_SOURCES) vendor/libcxxabi/src/cxa_aux_runtime.cpp

.PHONY: native-runtime-sources
native-runtime-sources:
	@printf '%s\n' $(NATIVE_DESKTOP_SOURCES)
endif
