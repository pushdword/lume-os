ARCH ?= aarch64
.DEFAULT_GOAL := all
OS_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../..)
include $(OS_ROOT)/mk/kernel-path.mk
include $(OS_ROOT)/arch/$(ARCH)/config.mk
KERNEL_ROOT ?= $(OS_ROOT)/../lume-kernel
SDK ?= /data/sdk
SYSROOT ?= $(SDK)
OUT ?= build
CC = clang
LD = ld.lld
AR = llvm-ar
BATCH ?= 1
CPPFLAGS := -I$(OS_ROOT)/arch/$(ARCH)/include -I$(OS_ROOT)/include/posix  -I$(OS_ROOT)/apps/net -I$(OS_ROOT)/services/netd -I$(OS_ROOT)/services/svcd -I$(OS_ROOT)/apps/pkg -I$(OS_ROOT)/include/internal -I$(OS_ROOT)/libc/lume -I$(OS_ROOT)/libc/posix -I$(OS_ROOT)/libc/crypto $(ARCH_CPPFLAGS) -I$(OS_ROOT)/include/lume -I$(KERNEL_ROOT)/include/uapi -I$(KERNEL_ROOT)/include/abi/$(UAPI_ABI) -isystem $(SYSROOT)/include
CFLAGS := $(ARCH_CFLAGS) -O2 -std=gnu11 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -ffunction-sections -fdata-sections
LDFLAGS := --gc-sections --build-id=none -z max-page-size=4096 -T $(OS_ROOT)/arch/$(ARCH)/user.ld
LIBS := --start-group $(SDK)/lib/libc.a $(SDK)/lib/libcrt.a --end-group
COMMON := arch/$(ARCH)/crt0.S libc/lume/api.c libc/runtime/runtime.c libc/lume/native.c libc/posix/sbrk.c libc/posix/errno_map.c libc/lume/thread.c arch/$(ARCH)/tls.c libc/runtime/locks.c libc/posix/pthread.c libc/posix/time.c
COMMON_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(COMMON)))
include $(OS_ROOT)/mk/applications.mk
APPS := ls dir less top worker probe fstest libctest video smpdemo iostat iobench cp touch tar base64
include $(OS_ROOT)/mk/busybox.mk
include $(OS_ROOT)/mk/htop.mk
VI_SOURCES := ports/busybox/compat.c ports/vi/compat.c vendor/vi/vi.c
HTOP_SOURCES += $(addprefix ports/htop/,LumeBackend.c curses.c posix.c entry.c runtime.c) $(addprefix libc/posix/,sbrk.c posix_runtime.c errno_map.c)
BB_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(BB_SOURCES)))
VI_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(VI_SOURCES)))
HTOP_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(HTOP_SOURCES)))
ifeq ($(BATCH),1)
BATCH_APPS := $(filter-out libctest,$(APPS))
BATCH_BB := $(filter-out ports/busybox/compat.c,$(BB_SOURCES))
BATCH_HTOP := $(filter-out libc/posix/%,$(HTOP_SOURCES))
BB_OBJECTS := $(addprefix $(OUT)/batch/busybox/,$(notdir $(BATCH_BB:.c=.o))) $(OUT)/obj/ports/busybox/compat.c.o
HTOP_OBJECTS := $(addprefix $(OUT)/batch/htop/,$(notdir $(BATCH_HTOP:.c=.o))) $(addprefix $(OUT)/obj/,$(addsuffix .o,$(filter libc/posix/%,$(HTOP_SOURCES))))
define compile_batch
$(1) &: $(addprefix $(OS_ROOT)/,$(2))
	mkdir -p $(3)
	$$(CC) $$(CPPFLAGS) $$(CFLAGS) -working-directory $(abspath $(3)) -c $$^
endef
$(eval $(call compile_batch,$(addprefix $(OUT)/batch/apps/,$(addsuffix .o,$(BATCH_APPS))),$(foreach app,$(BATCH_APPS),$(APP_SOURCE_$(app))),$(OUT)/batch/apps))
$(eval $(call compile_batch,$(filter-out $(OUT)/obj/ports/busybox/compat.c.o,$(BB_OBJECTS)),$(BATCH_BB),$(OUT)/batch/busybox))
$(eval $(call compile_batch,$(filter-out $(OUT)/obj/%,$(HTOP_OBJECTS)),$(BATCH_HTOP),$(OUT)/batch/htop))
endif
$(OUT)/obj/libc/posix/pthread.c.o $(OUT)/obj/libc/lume/thread.c.o $(OUT)/obj/arch/$(ARCH)/tls.c.o $(OUT)/obj/libc/runtime/locks.c.o: CPPFLAGS += -I$(OS_ROOT)/arch/$(ARCH)/include -I$(OS_ROOT)/include/internal -I$(OS_ROOT)/include/posix  -I$(OS_ROOT)/include/posix -D_POSIX_C_SOURCE=200809L -D_POSIX_MONOTONIC_CLOCK=200809L
$(BB_OBJECTS): CPPFLAGS += -I$(OS_ROOT)/ports/busybox
$(VI_OBJECTS): CPPFLAGS += -I$(OS_ROOT)/ports/vi -I$(OS_ROOT)/ports/busybox
$(HTOP_OBJECTS): CPPFLAGS += -I$(OS_ROOT)/ports/htop -I$(OS_ROOT)/vendor/htop -I$(OS_ROOT)/vendor/htop/unsupported -I$(OS_ROOT)/include/posix -DPATH_MAX=128 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -D_POSIX_MONOTONIC_CLOCK=200809L -DHAVE_CONFIG_H -DNDEBUG
$(OUT)/obj/apps/diagnostics/libctest.c.o: CPPFLAGS += -DLUME_TEST_HEAP_EXCESS=1073741824
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
$(OUT)/libctest: $(OUT)/obj/apps/diagnostics/libctest.c.o $(OUT)/common.a
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@
else
define native_utility
$(OUT)/$(1): $(OUT)/obj/$(APP_SOURCE_$(1)).o $(OUT)/common.a
	$(LD) $(LDFLAGS) $$^ $(LIBS) -o $$@
endef
$(foreach app,$(APPS),$(eval $(call native_utility,$(app))))
endif
$(OUT)/busybox: $(BB_OBJECTS) $(OUT)/common.a
	$(LD) $(LDFLAGS) $^ --defsym=__d_vfprintf=__l_vfprintf --defsym=vfprintf=__l_vfprintf $(LIBS) -o $@
$(OUT)/vi: $(VI_OBJECTS) $(OUT)/common.a
	$(LD) $(LDFLAGS) $^ --defsym=__d_vfprintf=__l_vfprintf --defsym=vfprintf=__l_vfprintf $(LIBS) -o $@
$(OUT)/htop.a: $(HTOP_OBJECTS)
	$(file >$@.rsp,$^)
	$(AR) rcs $@ @$@.rsp
$(OUT)/htop: $(OUT)/htop.a $(filter-out $(OUT)/obj/libc/runtime/runtime.c.o $(OUT)/obj/libc/posix/sbrk.c.o $(OUT)/obj/libc/posix/errno_map.c.o,$(COMMON_OBJECTS))
	$(LD) $(LDFLAGS) --whole-archive $< --no-whole-archive $(filter-out $<,$^) $(LIBS) -o $@
nettools: $(OUT)/ping $(OUT)/nc
$(OUT)/ping $(OUT)/nc: $(OUT)/%: $(OUT)/obj/apps/net/%.c.o $(OUT)/obj/arch/$(ARCH)/start_bare.S.o $(OUT)/obj/libc/string/memory.c.o
	$(LD) $(LDFLAGS) $^ -o $@

.PHONY: base
base: $(OUT)/net $(OUT)/netd $(OUT)/update $(OUT)/fetch
$(OUT)/net: $(OUT)/obj/apps/net/net.c.o $(OUT)/obj/arch/$(ARCH)/start_bare.S.o $(OUT)/obj/libc/string/memory.c.o
	$(LD) $(LDFLAGS) $^ -o $@
$(OUT)/netd: $(OUT)/obj/services/netd/netd.c.o $(OUT)/obj/services/netd/net_policy.c.o $(OUT)/obj/arch/$(ARCH)/start_bare.S.o $(OUT)/obj/libc/string/memory.c.o
	$(LD) $(LDFLAGS) $^ -o $@
$(OUT)/update $(OUT)/fetch: $(OUT)/%: $(OUT)/obj/apps/update/%.c.o $(OUT)/obj/libc/crypto/sha256.c.o $(OUT)/obj/arch/$(ARCH)/start_bare.S.o $(OUT)/obj/libc/string/memory.c.o
	$(LD) $(LDFLAGS) $^ -o $@
