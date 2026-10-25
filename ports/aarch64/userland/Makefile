.DEFAULT_GOAL := all
OS_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../..)
KERNEL_ROOT ?= $(OS_ROOT)/../lume-kernel
SDK ?= /data/sdk
SYSROOT ?= $(SDK)
OUT ?= build
CC = clang
LD = ld.lld
AR = llvm-ar
BATCH ?= 1
CPPFLAGS := -I$(OS_ROOT)/apps/aarch64 -I$(OS_ROOT)/libc/common -DLUME_ARCH_AARCH64=1 -I$(OS_ROOT)/include/common -I$(KERNEL_ROOT)/include/uapi/common -I$(KERNEL_ROOT)/include/uapi/aarch64 -isystem $(SYSROOT)/include
CFLAGS := --target=aarch64-unknown-none-elf -O2 -std=gnu11 -mcpu=cortex-a76 -mstrict-align -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -ffunction-sections -fdata-sections
LDFLAGS := --gc-sections --build-id=none -z max-page-size=4096 -T $(OS_ROOT)/apps/aarch64/user.ld
LIBS := --start-group $(SDK)/lib/libc.a $(SDK)/lib/libcrt.a --end-group
COMMON := arch/aarch64/start.S libc/common/api.c libc/common/runtime.c libc/aarch64/api.c libc/common/posix/sbrk.c libc/common/posix/errno_map.c libc/aarch64/thread_backend.c libc/common/posix/pthread.c libc/common/posix/time.c
COMMON_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(COMMON)))
APPS := ls dir less top worker probe fstest libctest video smpdemo iostat iobench cp touch tar base64
include $(OS_ROOT)/mk/busybox.mk
include $(OS_ROOT)/mk/htop.mk
VI_SOURCES := ports/common/busybox/compat.c ports/common/vi/compat.c ports/common/vi/upstream/vi.c
HTOP_SOURCES += $(addprefix ports/common/htop/,LumeBackend.c curses.c posix.c entry.c runtime.c) $(addprefix libc/common/posix/,sbrk.c posix_runtime.c errno_map.c)
BB_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(BB_SOURCES)))
VI_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(VI_SOURCES)))
HTOP_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(HTOP_SOURCES)))
ifeq ($(BATCH),1)
BATCH_APPS := $(filter-out libctest,$(APPS))
BATCH_BB := $(filter-out ports/common/busybox/compat.c,$(BB_SOURCES))
BATCH_HTOP := $(filter-out libc/common/posix/%,$(HTOP_SOURCES))
BB_OBJECTS := $(addprefix $(OUT)/batch/busybox/,$(notdir $(BATCH_BB:.c=.o))) $(OUT)/obj/ports/common/busybox/compat.c.o
HTOP_OBJECTS := $(addprefix $(OUT)/batch/htop/,$(notdir $(BATCH_HTOP:.c=.o))) $(addprefix $(OUT)/obj/,$(addsuffix .o,$(filter libc/common/posix/%,$(HTOP_SOURCES))))
define compile_batch
$(1) &: $(addprefix $(OS_ROOT)/,$(2))
	mkdir -p $(3)
	$$(CC) $$(CPPFLAGS) $$(CFLAGS) -working-directory $(abspath $(3)) -c $$^
endef
$(eval $(call compile_batch,$(addprefix $(OUT)/batch/apps/,$(addsuffix .o,$(BATCH_APPS))),$(addprefix apps/common/,$(addsuffix .c,$(BATCH_APPS))),$(OUT)/batch/apps))
$(eval $(call compile_batch,$(filter-out $(OUT)/obj/ports/common/busybox/compat.c.o,$(BB_OBJECTS)),$(BATCH_BB),$(OUT)/batch/busybox))
$(eval $(call compile_batch,$(filter-out $(OUT)/obj/%,$(HTOP_OBJECTS)),$(BATCH_HTOP),$(OUT)/batch/htop))
endif
$(OUT)/obj/libc/common/posix/pthread.c.o $(OUT)/obj/libc/aarch64/thread_backend.c.o: CPPFLAGS += -I$(OS_ROOT)/include/common/posix -D_POSIX_C_SOURCE=200809L -D_POSIX_MONOTONIC_CLOCK=200809L
$(BB_OBJECTS): CPPFLAGS += -I$(OS_ROOT)/ports/common/busybox
$(VI_OBJECTS): CPPFLAGS += -I$(OS_ROOT)/ports/common/vi -I$(OS_ROOT)/ports/common/busybox
$(HTOP_OBJECTS): CPPFLAGS += -I$(OS_ROOT)/ports/common/htop -I$(OS_ROOT)/vendor/htop -I$(OS_ROOT)/vendor/htop/unsupported -I$(OS_ROOT)/include/common/posix -DPATH_MAX=128 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -D_POSIX_MONOTONIC_CLOCK=200809L -DHAVE_CONFIG_H -DNDEBUG
$(OUT)/obj/apps/common/libctest.c.o: CPPFLAGS += -DLUME_TEST_HEAP_EXCESS=1073741824
.PHONY: all lume-utils busybox vi htop nettools
all: lume-utils busybox vi htop nettools
lume-utils: $(addprefix $(OUT)/,$(APPS))
busybox: $(OUT)/busybox
vi: $(OUT)/vi
htop: $(OUT)/htop
$(OUT)/obj/%.c.o: $(OS_ROOT)/%.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(OUT)/obj/%.S.o: $(OS_ROOT)/%.S
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(OUT)/common.a: $(COMMON_OBJECTS)
	$(AR) rcs $@ $^
ifeq ($(BATCH),1)
$(addprefix $(OUT)/,$(BATCH_APPS)): $(OUT)/%: $(OUT)/batch/apps/%.o $(OUT)/common.a
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@
$(OUT)/libctest: $(OUT)/obj/apps/common/libctest.c.o $(OUT)/common.a
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@
else
$(addprefix $(OUT)/,$(APPS)): $(OUT)/%: $(OUT)/obj/apps/common/%.c.o $(OUT)/common.a
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@
endif
$(OUT)/busybox: $(BB_OBJECTS) $(OUT)/common.a
	$(LD) $(LDFLAGS) $^ --defsym=__d_vfprintf=__l_vfprintf --defsym=vfprintf=__l_vfprintf $(LIBS) -o $@
$(OUT)/vi: $(VI_OBJECTS) $(OUT)/common.a
	$(LD) $(LDFLAGS) $^ --defsym=__d_vfprintf=__l_vfprintf --defsym=vfprintf=__l_vfprintf $(LIBS) -o $@
$(OUT)/htop.a: $(HTOP_OBJECTS)
	$(file >$@.rsp,$^)
	$(AR) rcs $@ @$@.rsp
$(OUT)/htop: $(OUT)/htop.a $(filter-out $(OUT)/obj/libc/common/runtime.c.o $(OUT)/obj/libc/common/posix/sbrk.c.o $(OUT)/obj/libc/common/posix/errno_map.c.o,$(COMMON_OBJECTS))
	$(LD) $(LDFLAGS) --whole-archive $< --no-whole-archive $(filter-out $<,$^) $(LIBS) -o $@
nettools: $(OUT)/ping $(OUT)/nc
$(OUT)/ping $(OUT)/nc: $(OUT)/%: $(OUT)/obj/apps/aarch64/%.c.o $(OUT)/obj/apps/aarch64/start.S.o $(OUT)/obj/libc/aarch64/memory.c.o
	$(LD) $(LDFLAGS) $^ -o $@

.PHONY: base
base: $(OUT)/net $(OUT)/netd $(OUT)/update $(OUT)/fetch
$(OUT)/net: $(OUT)/obj/apps/aarch64/net.c.o $(OUT)/obj/apps/aarch64/start.S.o $(OUT)/obj/libc/aarch64/memory.c.o
	$(LD) $(LDFLAGS) $^ -o $@
$(OUT)/netd: $(OUT)/obj/apps/aarch64/netd.c.o $(OUT)/obj/apps/aarch64/net_policy.c.o $(OUT)/obj/apps/aarch64/start.S.o $(OUT)/obj/libc/aarch64/memory.c.o
	$(LD) $(LDFLAGS) $^ -o $@
$(OUT)/update $(OUT)/fetch: $(OUT)/%: $(OUT)/obj/apps/common/%.c.o $(OUT)/obj/libc/common/sha256.c.o $(OUT)/obj/apps/aarch64/start.S.o $(OUT)/obj/libc/aarch64/memory.c.o
	$(LD) $(LDFLAGS) $^ -o $@
