SHELL := sh
.DEFAULT_GOAL := all
ARCH ?= x32
BOARD ?= $(if $(filter aarch64,$(ARCH)),raspberrypi5,tang25k)
SDK ?=
KERNEL ?= kernel
KERNEL_OUT ?= build/$(ARCH)
OUT ?= build/$(ARCH)
ifeq ($(filter build/%,$(OUT)),)
$(error OUT must be a directory below build/)
endif
ifneq ($(findstring ..,$(OUT)),)
$(error OUT cannot contain parent-directory components)
endif
HOSTCC ?= cc
HOSTCXX ?= c++
HOSTCFLAGS ?= -O2 -std=c11
HOSTCXXFLAGS ?= -O2 -std=c++17
HOSTEXE := $(if $(filter Windows_NT,$(OS)),.exe,)
TOOLBIN = $(if $(strip $(SDK)),$(SDK)/bin/,)
.PHONY: all image check clean kernel headers
ifeq ($(ARCH),aarch64)
include mk/userland-aarch64.mk
else
ifeq ($(ARCH),r32)
CC := "$(TOOLBIN)r32-clang$(HOSTEXE)"
LD := "$(TOOLBIN)r32-ld.lld$(HOSTEXE)"
CPPFLAGS := -DLUME_ARCH_R32=1
LINK_FLAGS := -m r32elf
BUILTINS := builtins-r32
KERNEL_FILE := lume.r32k
else ifeq ($(ARCH),x32)
CC := "$(TOOLBIN)clang$(HOSTEXE)" --target=x32-unknown-none-elf
LD := "$(TOOLBIN)ld.lld$(HOSTEXE)"
LINK_FLAGS := -m x32elf
BUILTINS := builtins-x32
KERNEL_FILE := lume.bin
else
$(error Unsupported ARCH $(ARCH))
endif
CFLAGS := -Oz -std=gnu11 -ffreestanding -fno-builtin -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections
CPPFLAGS += -Iinclude/common -I"$(KERNEL)/$(KERNEL_OUT)/include" --sysroot="$(SDK)/sysroot"
LIBS := --start-group "$(SDK)/sysroot/lib/libc.a" "$(SDK)/sysroot/lib/libclang_rt.$(BUILTINS).a" --end-group
COMMON_SOURCES := arch/$(ARCH)/start.s libc/common/api.c libc/common/runtime.c arch/$(ARCH)/memory.s
ifeq ($(ARCH),r32)
COMMON_SOURCES += libc/r32/strings.c
endif
COMMON_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(COMMON_SOURCES)))
APP_NAMES := init shell worker probe ls dir less fstest libctest video smpdemo top net iostat iobench netd
APP_TARGETS := $(addprefix $(OUT)/bin/,$(addsuffix .$(ARCH),$(APP_NAMES) busybox vi htop dropbear sshshell))
include mk/htop.mk
include mk/dropbear.mk
include mk/busybox.mk
VI_SOURCES := ports/common/busybox/compat.c ports/common/vi/compat.c ports/common/vi/upstream/vi.c
HTOP_SOURCES += $(addprefix ports/common/htop/,LumeBackend.c curses.c posix.c entry.c runtime.c) $(addprefix libc/common/posix/,sbrk.c time.c posix_runtime.c errno_map.c)
DROPBEAR_SOURCES += $(wildcard ports/common/dropbear/*.c) libc/common/random.c libc/common/posix/errno_map.c libc/common/posix/sbrk.c
ifeq ($(ARCH),r32)
SSH_CRYPTO ?= hardware
ifeq ($(SSH_CRYPTO),software)
DROPBEAR_SOURCES := $(filter-out vendor/dropbear/src/curve25519.c ports/common/dropbear/accelerator.c,$(DROPBEAR_SOURCES))
else ifeq ($(SSH_CRYPTO),hardware)
DROPBEAR_SOURCES := $(filter-out ports/common/dropbear/curve25519.c ports/common/dropbear/accelerator_soft.c,$(DROPBEAR_SOURCES))
else
$(error Unsupported SSH_CRYPTO $(SSH_CRYPTO))
endif
else
DROPBEAR_SOURCES := $(filter-out ports/common/dropbear/curve25519.c ports/common/dropbear/accelerator_soft.c,$(DROPBEAR_SOURCES))
endif
BB_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(BB_SOURCES)))
VI_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(VI_SOURCES)))
HTOP_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(HTOP_SOURCES)))
SSH_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(DROPBEAR_SOURCES)))
BASE_OBJECTS := $(filter-out $(OUT)/obj/libc/common/runtime.c.o,$(COMMON_OBJECTS))
$(BB_OBJECTS): CPPFLAGS += -Iports/common/busybox
$(VI_OBJECTS): CPPFLAGS += -Iports/common/vi -Iports/common/busybox
$(HTOP_OBJECTS): CPPFLAGS += -Iports/common/htop -Ivendor/htop -Ivendor/htop/unsupported -Iinclude/common/posix -DPATH_MAX=128 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -D_POSIX_MONOTONIC_CLOCK=200809L -DHAVE_CONFIG_H -DNDEBUG
SSH_CPPFLAGS := -Ilibc/common -Iports/common/dropbear -Iports/common/dropbear/include -Ivendor/dropbear/src -Ivendor/dropbear/libtomcrypt/src/headers -Ivendor/dropbear/libtommath -Iinclude/common/posix -DDROPBEAR_SERVER=1 -DLOCALOPTIONS_H_EXISTS -DBUNDLED_LIBTOM -DLTC_NO_ASM -DMP_32BIT
$(SSH_OBJECTS): CPPFLAGS += $(SSH_CPPFLAGS)
$(filter %/sha256.c.o %/sha512.c.o,$(SSH_OBJECTS)): CPPFLAGS += -DLTC_SMALL_CODE
$(filter $(OUT)/obj/vendor/dropbear/libtomcrypt/%,$(SSH_OBJECTS)): CPPFLAGS += -DLTC_SOURCE
$(OUT)/obj/libc/common/dhcp.c.o: CPPFLAGS += -Iinclude/common/platform
$(OUT)/obj/apps/common/netd.c.o: CPPFLAGS += -Iinclude/common/platform
all: headers $(APP_TARGETS) kernel
$(COMMON_OBJECTS) $(BB_OBJECTS) $(VI_OBJECTS) $(HTOP_OBJECTS) $(SSH_OBJECTS) $(addprefix $(OUT)/obj/apps/common/,$(addsuffix .c.o,$(APP_NAMES) sshshell)) $(OUT)/obj/libc/common/dhcp.c.o: | headers
$(OUT)/obj/%.c.o: %.c $(OUT)/configuration Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(OUT)/obj/%.s.o: %.s $(OUT)/configuration Makefile
	mkdir -p "$(@D)"
	$(CC) -c "$<" -o "$@"
$(OUT)/bin/%.$(ARCH): $(OUT)/obj/apps/common/%.c.o $(COMMON_OBJECTS) arch/$(ARCH)/user.ld
	mkdir -p "$(@D)"
	$(LD) $(LINK_FLAGS) --gc-sections --build-id=none -T arch/$(ARCH)/user.ld $(COMMON_OBJECTS) "$<" $(LIBS) -o "$@"
$(OUT)/bin/netd.$(ARCH): $(OUT)/obj/apps/common/netd.c.o $(OUT)/obj/libc/common/dhcp.c.o $(COMMON_OBJECTS)
	mkdir -p "$(@D)"
	$(LD) $(LINK_FLAGS) --gc-sections --build-id=none -T arch/$(ARCH)/user.ld $^ $(LIBS) -o "$@"
$(OUT)/bin/busybox.$(ARCH): $(BB_OBJECTS) $(COMMON_OBJECTS)
	mkdir -p "$(@D)"
	$(LD) $(LINK_FLAGS) --gc-sections --build-id=none -T arch/$(ARCH)/utility.ld $^ --defsym=__d_vfprintf=__l_vfprintf --defsym=vfprintf=__l_vfprintf $(LIBS) -o "$@"
$(OUT)/bin/vi.$(ARCH): $(VI_OBJECTS) $(COMMON_OBJECTS)
	mkdir -p "$(@D)"
	$(LD) $(LINK_FLAGS) --gc-sections --build-id=none -T arch/$(ARCH)/utility.ld $^ --defsym=__d_vfprintf=__l_vfprintf --defsym=vfprintf=__l_vfprintf $(LIBS) -o "$@"
$(OUT)/bin/htop.$(ARCH): $(HTOP_OBJECTS) $(BASE_OBJECTS)
	mkdir -p "$(@D)"
	$(file >$@.rsp,$(LINK_FLAGS) --gc-sections --build-id=none -T libc/$(ARCH)/posix/native.ld $(foreach obj,$^,"$(obj)") $(LIBS) -o "$@")
	$(LD) @"$@.rsp"
$(OUT)/bin/dropbear.$(ARCH): $(SSH_OBJECTS) $(BASE_OBJECTS)
	mkdir -p "$(@D)"
	$(file >$@.rsp,$(LINK_FLAGS) --gc-sections --build-id=none -T libc/$(ARCH)/posix/native.ld $(foreach obj,$^,"$(obj)") $(LIBS) -o "$@")
	$(LD) @"$@.rsp"
$(OUT)/obj/apps/common/sshshell.c.o: apps/common/shell.c $(OUT)/configuration Makefile | headers
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -DLUME_SHELL_FDS=1 -c "$<" -o "$@"
$(OUT)/bin/sshshell.$(ARCH): $(OUT)/obj/apps/common/sshshell.c.o $(COMMON_OBJECTS)
	mkdir -p "$(@D)"
	$(file >$@.rsp,$(LINK_FLAGS) --gc-sections --build-id=none -T libc/$(ARCH)/posix/native.ld $(foreach obj,$^,"$(obj)") $(LIBS) -o "$@")
	$(LD) @"$@.rsp"
$(OUT)/bin/htop.$(ARCH) $(OUT)/bin/dropbear.$(ARCH) $(OUT)/bin/sshshell.$(ARCH): | $(OUT)/bin
$(OUT)/bin:
	mkdir -p "$@"
ifeq ($(ARCH),r32)
R32_CRYPTO_TEST := $(OUT)/obj/tests/r32/ssh_crypto.c.o
R32_CRYPTO_OBJECTS := $(filter $(OUT)/obj/vendor/dropbear/libtomcrypt/% %/libc/common/random.c.o %/ports/common/dropbear/accelerator.c.o %/ports/common/dropbear/accelerator_soft.c.o %/ports/common/dropbear/profile.c.o %/ports/common/dropbear/curve25519.c.o %/vendor/dropbear/src/curve25519.c.o %/dbhelpers.c.o,$(SSH_OBJECTS))
$(R32_CRYPTO_TEST): CPPFLAGS += $(SSH_CPPFLAGS)
$(R32_CRYPTO_TEST): | headers
.PHONY: r32-crypto-test
r32-crypto-test: $(OUT)/bin/cryptot.r32
$(OUT)/bin/cryptot.r32: $(R32_CRYPTO_TEST) $(COMMON_OBJECTS) $(R32_CRYPTO_OBJECTS) | $(OUT)/bin
	mkdir -p "$(@D)"
	$(file >$@.rsp,$(LINK_FLAGS) --gc-sections --build-id=none -T arch/r32/utility.ld $(foreach obj,$^,"$(obj)") $(LIBS) -o "$@")
	$(LD) @"$@.rsp"
endif
kernel:
	$(MAKE) -C "$(KERNEL)" ARCH=$(ARCH) BOARD=$(BOARD) SDK="$(SDK)" OUT="$(KERNEL_OUT)" HOSTCXX='$(HOSTCXX)' HOSTCXXFLAGS='$(HOSTCXXFLAGS)'
BOOTLOADER ?=
R32_NATIVE_ROOT ?=
IMAGE_MIB ?= $(if $(and $(filter r32,$(ARCH)),$(strip $(R32_NATIVE_ROOT))),127,34)
ifneq ($(filter image,$(MAKECMDGOALS)),)
ifeq ($(strip $(BOOTLOADER)),)
$(error BOOTLOADER must name the target U-Boot image)
endif
endif
image: all $(OUT)/image-tool$(HOSTEXE)
	rm -rf -- "$(OUT)/root"
	mkdir -p "$(OUT)/root/boot" "$(OUT)/root/bin" "$(OUT)/root/sbin" "$(OUT)/root/usr/bin" "$(OUT)/root/usr/libexec" "$(OUT)/root/usr/share/fonts" "$(OUT)/root/etc" "$(OUT)/root/root"
	cp "$(BOOTLOADER)" "$(OUT)/root/boot/uboot.$(ARCH)"
	cp "$(KERNEL)/$(KERNEL_OUT)/$(KERNEL_FILE)" "$(OUT)/root/boot/lume.$(if $(filter r32,$(ARCH)),r32,bin)"
	cp "$(OUT)/bin/init.$(ARCH)" "$(OUT)/root/sbin/init.$(ARCH)"
	cp "$(OUT)/bin/shell.$(ARCH)" "$(OUT)/root/bin/sh.$(ARCH)"
	cp "$(OUT)/bin/ls.$(ARCH)" "$(OUT)/bin/dir.$(ARCH)" "$(OUT)/bin/busybox.$(ARCH)" "$(OUT)/root/bin/"
	cp "$(OUT)/bin/less.$(ARCH)" "$(OUT)/root/usr/bin/"
	cp "$(OUT)/bin/video.$(ARCH)" "$(OUT)/root/usr/bin/"
	cp "$(OUT)/bin/smpdemo.$(ARCH)" "$(OUT)/root/usr/bin/"
	cp "$(OUT)/bin/top.$(ARCH)" "$(OUT)/root/usr/bin/"
	cp "$(OUT)/bin/net.$(ARCH)" "$(OUT)/root/usr/bin/"
	cp "$(OUT)/bin/iostat.$(ARCH)" "$(OUT)/root/usr/bin/"
	cp "$(OUT)/bin/iobench.$(ARCH)" "$(OUT)/root/usr/bin/"
	cp "$(OUT)/bin/netd.$(ARCH)" "$(OUT)/root/usr/bin/"
	cp "$(OUT)/bin/vi.$(ARCH)" "$(OUT)/root/usr/bin/"
	cp "$(OUT)/bin/htop.$(ARCH)" "$(OUT)/root/usr/bin/"
	cp "$(OUT)/bin/dropbear.$(ARCH)" "$(OUT)/root/usr/bin/"
	cp "$(OUT)/bin/sshshell.$(ARCH)" "$(OUT)/root/usr/bin/"
	cp "$(OUT)/bin/worker.$(ARCH)" "$(OUT)/root/usr/libexec/"
	cp "$(OUT)/bin/probe.$(ARCH)" "$(OUT)/root/usr/libexec/"
	cp "$(OUT)/bin/fstest.$(ARCH)" "$(OUT)/root/usr/libexec/"
	cp "$(OUT)/bin/libctest.$(ARCH)" "$(OUT)/root/usr/libexec/"
	cp assets/$(ARCH)/lume8x16.fnt "$(OUT)/root/usr/share/fonts/"
	cp root/$(ARCH)/etc/* "$(OUT)/root/etc/"
ifeq ($(ARCH),r32)
ifneq ($(strip $(R32_NATIVE_ROOT)),)
	test -f "$(R32_NATIVE_ROOT)/usr/bin/clang.r32"
	test -f "$(R32_NATIVE_ROOT)/usr/libexec/rcc1.r32"
	test -f "$(R32_NATIVE_ROOT)/usr/libexec/rld.r32"
	cp -Rp "$(R32_NATIVE_ROOT)/usr/." "$(OUT)/root/usr/"
endif
endif
	MSYS2_ARG_CONV_EXCL='*' "$(OUT)/image-tool$(HOSTEXE)" alias /bin/busybox.$(ARCH) "$(OUT)/root/bin/basename.$(ARCH)"
	MSYS2_ARG_CONV_EXCL='*' "$(OUT)/image-tool$(HOSTEXE)" alias /bin/busybox.$(ARCH) "$(OUT)/root/bin/cat.$(ARCH)"
	MSYS2_ARG_CONV_EXCL='*' "$(OUT)/image-tool$(HOSTEXE)" alias /bin/busybox.$(ARCH) "$(OUT)/root/bin/dirname.$(ARCH)"
	MSYS2_ARG_CONV_EXCL='*' "$(OUT)/image-tool$(HOSTEXE)" alias /bin/busybox.$(ARCH) "$(OUT)/root/bin/echo.$(ARCH)"
	MSYS2_ARG_CONV_EXCL='*' "$(OUT)/image-tool$(HOSTEXE)" alias /bin/busybox.$(ARCH) "$(OUT)/root/bin/false.$(ARCH)"
	MSYS2_ARG_CONV_EXCL='*' "$(OUT)/image-tool$(HOSTEXE)" alias /bin/busybox.$(ARCH) "$(OUT)/root/bin/head.$(ARCH)"
	MSYS2_ARG_CONV_EXCL='*' "$(OUT)/image-tool$(HOSTEXE)" alias /bin/busybox.$(ARCH) "$(OUT)/root/bin/printenv.$(ARCH)"
	MSYS2_ARG_CONV_EXCL='*' "$(OUT)/image-tool$(HOSTEXE)" alias /bin/busybox.$(ARCH) "$(OUT)/root/bin/pwd.$(ARCH)"
	MSYS2_ARG_CONV_EXCL='*' "$(OUT)/image-tool$(HOSTEXE)" alias /bin/busybox.$(ARCH) "$(OUT)/root/bin/true.$(ARCH)"
	MSYS2_ARG_CONV_EXCL='*' "$(OUT)/image-tool$(HOSTEXE)" alias /bin/busybox.$(ARCH) "$(OUT)/root/bin/wc.$(ARCH)"
	MSYS2_ARG_CONV_EXCL='*' "$(OUT)/image-tool$(HOSTEXE)" alias /bin/busybox.$(ARCH) "$(OUT)/root/bin/sort.$(ARCH)"
	MSYS2_ARG_CONV_EXCL='*' "$(OUT)/image-tool$(HOSTEXE)" alias /bin/busybox.$(ARCH) "$(OUT)/root/bin/cut.$(ARCH)"
	MSYS2_ARG_CONV_EXCL='*' "$(OUT)/image-tool$(HOSTEXE)" alias /bin/busybox.$(ARCH) "$(OUT)/root/bin/cmp.$(ARCH)"
	MSYS2_ARG_CONV_EXCL='*' "$(OUT)/image-tool$(HOSTEXE)" alias /bin/busybox.$(ARCH) "$(OUT)/root/bin/cksum.$(ARCH)"
	MSYS2_ARG_CONV_EXCL='*' "$(OUT)/image-tool$(HOSTEXE)" alias /bin/busybox.$(ARCH) "$(OUT)/root/bin/crc32.$(ARCH)"
	MSYS2_ARG_CONV_EXCL='*' "$(OUT)/image-tool$(HOSTEXE)" alias /bin/busybox.$(ARCH) "$(OUT)/root/bin/expand.$(ARCH)"
	MSYS2_ARG_CONV_EXCL='*' "$(OUT)/image-tool$(HOSTEXE)" alias /bin/busybox.$(ARCH) "$(OUT)/root/bin/unexpand.$(ARCH)"
	MSYS2_ARG_CONV_EXCL='*' "$(OUT)/image-tool$(HOSTEXE)" alias /bin/busybox.$(ARCH) "$(OUT)/root/bin/mkdir.$(ARCH)"
	MSYS2_ARG_CONV_EXCL='*' "$(OUT)/image-tool$(HOSTEXE)" alias /bin/busybox.$(ARCH) "$(OUT)/root/bin/rmdir.$(ARCH)"

	"$(OUT)/image-tool$(HOSTEXE)" disk "$(OUT)/root" "$(OUT)/lume.img" "$(IMAGE_MIB)"
endif
headers:
	$(MAKE) -C "$(KERNEL)" ARCH=$(ARCH) OUT="$(KERNEL_OUT)" headers
.PHONY: check-shell
check: check-shell
check-shell: $(OUT)/tests/shell-console$(HOSTEXE) $(OUT)/tests/shell-ssh$(HOSTEXE)
	env "$(OUT)/tests/shell-console$(HOSTEXE)"
	env "$(OUT)/tests/shell-ssh$(HOSTEXE)"
$(OUT)/tests/shell-ssh$(HOSTEXE): SHELL_TEST_FLAGS := -DLUME_SHELL_FDS=1
$(OUT)/tests/shell-console$(HOSTEXE) $(OUT)/tests/shell-ssh$(HOSTEXE): tests/common/shell.c apps/common/shell.c apps/common/shell_io.h include/common/api.h include/common/fd_client.h $(KERNEL)/include/uapi/common/lume/app.h | headers
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) $(SHELL_TEST_FLAGS) $(if $(filter aarch64,$(ARCH)),-DLUME_ARCH_AARCH64=1,$(if $(filter r32,$(ARCH)),-DLUME_ARCH_R32=1,)) -Iinclude/common -I"$(KERNEL)/$(KERNEL_OUT)/include" "$<" -o "$@"
$(OUT)/image-tool$(HOSTEXE): tools/image.cpp
	mkdir -p "$(@D)"
	$(HOSTCXX) $(HOSTCXXFLAGS) "$<" -o "$@"
check: $(OUT)/tests/net-client$(HOSTEXE) $(OUT)/tests/net-policy$(HOSTEXE) $(OUT)/tests/netd$(HOSTEXE) $(OUT)/tests/image$(HOSTEXE)
	env "$(OUT)/tests/net-client$(HOSTEXE)"
	env "$(OUT)/tests/net-policy$(HOSTEXE)"
	env "$(OUT)/tests/netd$(HOSTEXE)"
	env "$(OUT)/tests/image$(HOSTEXE)" "$(OUT)/tests/image-files"
$(OUT)/tests/image$(HOSTEXE): tests/image.cpp tools/image.cpp
	mkdir -p "$(@D)"
	$(HOSTCXX) $(HOSTCXXFLAGS) "$<" -o "$@"
$(OUT)/tests/net-client$(HOSTEXE): tests/aarch64/net_client_test.c apps/aarch64/net_client.h apps/aarch64/net_service_client.h apps/aarch64/lib.h
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -fno-builtin -Iapps/aarch64 -I"$(KERNEL)/include/uapi/aarch64" "$<" -o "$@"
$(OUT)/tests/net-policy$(HOSTEXE): tests/aarch64/net_policy_test.c apps/aarch64/net_policy.c
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -Iapps/aarch64 $^ -o "$@"
$(OUT)/tests/netd$(HOSTEXE): tests/aarch64/netd_test.c apps/aarch64/net_policy.c apps/aarch64/netd.c apps/aarch64/net_policy.h
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -Iapps/aarch64 -I"$(KERNEL)/include/uapi/aarch64" $(filter %netd_test.c %net_policy.c,$^) -o "$@"
clean:
	rm -rf -- "$(OUT)"
-include $(addsuffix .d,$(COMMON_OBJECTS) $(BB_OBJECTS) $(VI_OBJECTS) $(HTOP_OBJECTS) $(SSH_OBJECTS) $(addprefix $(OUT)/obj/apps/common/,$(addsuffix .c.o,$(APP_NAMES) sshshell)))

.PHONY: FORCE
FORCE:
$(OUT):
	mkdir -p "$@"
$(OUT)/configuration: FORCE | $(OUT)
	$(file >$@.tmp,$(CC) | $(CPPFLAGS) | $(CFLAGS) | $(SDK) | $(SHELL_ELF) | $(SSH_CRYPTO))
	cmp -s "$@.tmp" "$@" || cp "$@.tmp" "$@"
