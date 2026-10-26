.DEFAULT_GOAL := all
CSTD := gnu11
PORT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
SOURCE = $(OS_ROOT)/vendor/busybox
include $(PORT)/../standalone.mk
CPPFLAGS += -D_GNU_SOURCE -DLUME_BUSYBOX_EXTENDED=1 -DLUME_PORT_ARG_BYTES=16384 -DLUME_PORT_ARG_COUNT=256 '-I$(OS_ROOT)/ports/busybox' '-I$(OS_ROOT)/include/lume' '-I$(KERNEL)/include/uapi' '-I$(KERNEL)/include/abi/native'
SOURCES := os/vendor/busybox/coreutils/basename.c os/vendor/busybox/coreutils/cat.c os/vendor/busybox/coreutils/dirname.c os/vendor/busybox/coreutils/echo.c os/vendor/busybox/coreutils/false.c os/vendor/busybox/coreutils/head.c os/vendor/busybox/coreutils/printenv.c os/vendor/busybox/coreutils/pwd.c os/vendor/busybox/coreutils/true.c os/vendor/busybox/coreutils/wc.c os/vendor/busybox/libbb/bb_cat.c os/vendor/busybox/libbb/full_write.c os/vendor/busybox/libbb/get_last_path_component.c os/vendor/busybox/libbb/process_escape_sequence.c os/vendor/busybox/libbb/single_argv.c os/vendor/busybox/coreutils/sort.c os/vendor/busybox/coreutils/cut.c os/vendor/busybox/editors/cmp.c os/vendor/busybox/coreutils/cksum.c os/vendor/busybox/coreutils/expand.c os/vendor/busybox/coreutils/mkdir.c os/vendor/busybox/coreutils/rmdir.c os/vendor/busybox/libbb/crc32.c os/ports/busybox/main.c os/ports/busybox/compat.c os/vendor/busybox/findutils/grep.c os/vendor/busybox/findutils/find.c os/vendor/busybox/findutils/xargs.c os/vendor/busybox/editors/sed.c os/vendor/busybox/libbb/xregcomp.c os/vendor/busybox/libbb/recursive_action.c os/vendor/busybox/libbb/concat_path_file.c os/vendor/busybox/libbb/concat_subpath_file.c os/vendor/busybox/libbb/skip_whitespace.c os/vendor/busybox/libbb/safe_strncpy.c port/extra.c shared/file_mode.c
OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(basename $(SOURCES))))
.PHONY: all
all: $(OUT)/libbusybox.a $(OUT)/busybox
$(OUT)/libbusybox.a: $(OBJECTS)
	rm -f $@
	$(AR) rcs $@ $^
$(OUT)/busybox: $(OUT)/obj/port/entry.o $(OUT)/libbusybox.a
	$(LD) $(LDFLAGS) $(RUNTIME) $^ $(LIBS) -o $@
