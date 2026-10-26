.DEFAULT_GOAL := all
CSTD := gnu23
PORT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
SOURCE = $(OS_ROOT)/vendor/patch
include $(PORT)/../standalone.mk
CPPFLAGS += -DHAVE_CONFIG_H -D_GNU_SOURCE '-I$(OS_ROOT)/ports/patch/generated/lib' '-I$(OS_ROOT)/ports/patch/generated' '-I$(OS_ROOT)/vendor/patch/lib' '-I$(OS_ROOT)/ports/patch' '-I$(OS_ROOT)/ports/standalone/include' '-I$(KERNEL)/include/abi/native' '-I$(OS_ROOT)/vendor/patch/src' '-I$(OS_ROOT)/vendor/patch' -DENABLE_MERGE
SOURCES := lib/access.c lib/argmatch.c lib/at-func2.c lib/backup-find.c lib/backupfile.c lib/basename-lgpl.c lib/bitrotate.c lib/c-ctype.c lib/c-strcasecmp.c lib/c-strncasecmp.c lib/c32isprint.c lib/calloc.c lib/canonicalize-lgpl.c lib/chdir-long.c lib/chmod.c lib/chown.c lib/cloexec.c lib/close-stream.c lib/close.c lib/closedir.c lib/closeout.c lib/dirfd.c lib/dirname-lgpl.c lib/dup.c lib/dup2.c lib/error.c lib/euidaccess.c lib/exitfail.c lib/faccessat.c lib/fchdir.c lib/fchmodat.c lib/fchownat.c lib/fcntl.c lib/fd-hook.c lib/fdopendir.c lib/filenamecat-lgpl.c lib/fpending.c lib/free.c lib/fstat.c lib/fstatat.c lib/futimens.c lib/getcwd-lgpl.c lib/getdtablesize.c lib/getgroups.c lib/getopt.c lib/getopt1.c lib/getprogname.c lib/getrandom.c lib/gettime.c lib/group-member.c lib/hard-locale.c lib/hash.c lib/ialloc.c lib/lchmod.c lib/lchown.c lib/localcharset.c lib/lstat.c lib/malloc.c lib/malloc/scratch_buffer_grow.c lib/malloc/scratch_buffer_grow_preserve.c lib/malloc/scratch_buffer_set_array_size.c lib/malloca.c lib/mbrtoc32.c lib/mbrtowc.c lib/mbszero.c lib/memchr.c lib/mkdir.c lib/mkdirat.c lib/mktime.c lib/open.c lib/openat-die.c lib/openat-proc.c lib/openat.c lib/opendir.c lib/opendirat.c lib/parse-datetime.c lib/progname.c lib/quotearg.c lib/readlink.c lib/readlinkat.c lib/realloc.c lib/reallocarray.c lib/rename.c lib/renameat.c lib/renameatu.c lib/rmdir.c lib/same-inode.c lib/save-cwd.c lib/setenv.c lib/setlocale-lock.c lib/setlocale_null-unlocked.c lib/setlocale_null.c lib/stat-time.c lib/stat.c lib/stdlib.c lib/stdopen.c lib/strerror-override.c lib/strerror.c lib/stripslash.c lib/symlink.c lib/symlinkat.c lib/tempname.c lib/time_rz.c lib/timegm.c lib/timespec.c lib/unictype/ctype_print.c lib/unistd.c lib/unlink.c lib/unlinkat.c lib/unsetenv.c lib/utime.c lib/utimens.c lib/utimensat.c lib/wctype-h.c lib/xalloc-die.c lib/xmalloc.c lib/xstdopen.c shared/signals.c shared/file_mode.c shared/resource.c shared/optional.c shared/sleep.c shared/realpath.c shared/limits.c shared/process.c shared/stream_pipe.c
OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(basename $(SOURCES))))
.PHONY: all
all: $(OUT)/libpatch.a $(OUT)/patch
$(OUT)/libpatch.a: $(OBJECTS)
	rm -f $@
	$(AR) rcs $@ $^
$(OUT)/patch: $(OUT)/obj/src/inp.o $(OUT)/obj/src/patch.o $(OUT)/obj/src/pch.o $(OUT)/obj/src/safe.o $(OUT)/obj/src/util.o $(OUT)/obj/src/version.o $(OUT)/obj/src/merge.o $(OUT)/libpatch.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
