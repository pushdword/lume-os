.DEFAULT_GOAL := all
CSTD := gnu23
PORT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
SOURCE = $(OS_ROOT)/vendor/gzip
include $(PORT)/../standalone.mk
CPPFLAGS += -DHAVE_CONFIG_H -D_GNU_SOURCE '-I$(OS_ROOT)/ports/gzip/generated/lib' '-I$(OS_ROOT)/ports/gzip/generated' '-I$(OS_ROOT)/vendor/gzip/lib' '-I$(OS_ROOT)/ports/gzip' '-I$(OS_ROOT)/ports/standalone/include' '-I$(KERNEL)/include/abi/native' '-I$(OS_ROOT)/vendor/gzip/src' '-I$(OS_ROOT)/vendor/gzip'
SOURCES := lib/asnprintf.c lib/basename-lgpl.c lib/calloc.c lib/chdir-long.c lib/cloexec.c lib/close.c lib/closedir.c lib/crc.c lib/creat-safer.c lib/dirfd.c lib/dirname-lgpl.c lib/dup-safer.c lib/dup.c lib/dup2.c lib/endian.c lib/error.c lib/exitfail.c lib/fchdir.c lib/fclose.c lib/fcntl.c lib/fd-hook.c lib/fd-safer.c lib/fdatasync.c lib/fdopendir.c lib/fflush.c lib/filenamecat-lgpl.c lib/fopen.c lib/fprintf.c lib/fpurge.c lib/freading.c lib/free.c lib/fseek.c lib/fseeko.c lib/fseterr.c lib/fstat.c lib/getcwd-lgpl.c lib/getdtablesize.c lib/getopt.c lib/getopt1.c lib/getprogname.c lib/gettime.c lib/ialloc.c lib/lstat.c lib/malloc.c lib/malloca.c lib/math.c lib/mbszero.c lib/memchr.c lib/open-safer.c lib/open.c lib/openat-die.c lib/openat-proc.c lib/openat-safer.c lib/openat.c lib/opendir.c lib/pipe-safer.c lib/printf-args.c lib/printf-frexp.c lib/printf-frexpl.c lib/printf-parse.c lib/printf.c lib/realloc.c lib/reallocarray.c lib/rmdir.c lib/save-cwd.c lib/savedir.c lib/sig-handler.c lib/signbitd.c lib/signbitf.c lib/signbitl.c lib/stat-time.c lib/stat.c lib/stdlib.c lib/strerror-override.c lib/strerror.c lib/stripslash.c lib/timespec.c lib/unistd.c lib/unlink.c lib/unlinkat.c lib/utime.c lib/utimens.c lib/vasnprintf.c lib/vfprintf.c lib/vfzprintf.c lib/xalloc-die.c lib/xmalloc.c lib/xsize.c lib/yesno.c port/version.c shared/signals.c shared/file_mode.c shared/resource.c shared/optional.c shared/sleep.c shared/realpath.c shared/limits.c shared/process.c shared/stream_pipe.c
OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(basename $(SOURCES))))
.PHONY: all
all: $(OUT)/libgzip.a $(OUT)/gzip
$(OUT)/libgzip.a: $(OBJECTS)
	rm -f $@
	$(AR) rcs $@ $^
$(OUT)/gzip: $(OUT)/obj/bits.o $(OUT)/obj/deflate.o $(OUT)/obj/gzip.o $(OUT)/obj/inflate.o $(OUT)/obj/trees.o $(OUT)/obj/unlzh.o $(OUT)/obj/unlzw.o $(OUT)/obj/unpack.o $(OUT)/obj/unzip.o $(OUT)/obj/util.o $(OUT)/obj/zip.o $(OUT)/libgzip.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
