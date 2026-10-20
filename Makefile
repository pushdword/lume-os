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
CC := "$(TOOLBIN)clang$(HOSTEXE)" --target=aarch64-unknown-none-elf
LD := "$(TOOLBIN)ld.lld$(HOSTEXE)"
CFLAGS := -O2 -std=c11 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -fno-unwind-tables -fno-asynchronous-unwind-tables -mcpu=cortex-a76 -mgeneral-regs-only -mstrict-align -ffunction-sections -fdata-sections
CPPFLAGS := -Iapps/aarch64 -I"$(KERNEL)/$(KERNEL_OUT)/include"
BB := apps/aarch64/busybox
APPLET_NAMES := cat echo false true pwd printenv basename dirname head wc mkdir rmdir
BB_SOURCES := $(BB)/main.c $(BB)/lume_tools.c $(BB)/compat.c $(addprefix $(BB)/upstream/coreutils/,$(addsuffix .c,$(APPLET_NAMES))) $(addprefix $(BB)/upstream/libbb/,$(addsuffix .c,single_argv process_escape_sequence get_last_path_component full_write bb_cat))
ARM_LINK := -nostdlib -static -Wl,--gc-sections,--build-id=none,-z,max-page-size=4096,-T,apps/aarch64/user.ld
all: headers $(OUT)/shell.elf $(OUT)/busybox.elf $(OUT)/progtest.elf $(OUT)/net.elf kernel
$(OUT)/shell.elf $(OUT)/busybox.elf $(OUT)/progtest.elf $(OUT)/net.elf: | headers
$(OUT)/shell.elf: apps/aarch64/start.S apps/aarch64/shell.c $(wildcard apps/aarch64/*.h)
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) $(ARM_LINK) apps/aarch64/start.S apps/aarch64/shell.c -o "$@"
$(OUT)/busybox.elf: $(BB_SOURCES) apps/aarch64/libc.c apps/aarch64/apps_start.S
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) -I$(BB) $(CFLAGS) $(ARM_LINK) apps/aarch64/apps_start.S apps/aarch64/libc.c $(BB_SOURCES) -o "$@"
$(OUT)/progtest.elf: apps/aarch64/progtest.c apps/aarch64/libc.c apps/aarch64/apps_start.S
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) $(ARM_LINK) $(filter %.c %.S,$^) -o "$@"
$(OUT)/net.elf: apps/aarch64/start.S apps/aarch64/netd.c apps/aarch64/net_policy.c libc/aarch64/memory.c
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) $(ARM_LINK) $(filter %.c %.S,$^) -o "$@"
kernel: $(OUT)/shell.elf
	$(MAKE) -C "$(KERNEL)" ARCH=$(ARCH) BOARD=$(BOARD) SDK="$(SDK)" OUT="$(KERNEL_OUT)" SHELL_ELF="$(CURDIR)/$(OUT)/shell.elf" HOSTCXX='$(HOSTCXX)' HOSTCXXFLAGS='$(HOSTCXXFLAGS)'
image: all $(OUT)/image-tool$(HOSTEXE)
	rm -rf -- "$(OUT)/root"
	mkdir -p "$(OUT)/root/bin" "$(OUT)/root/etc" "$(OUT)/root/lib/firmware"
	cp "$(OUT)/shell.elf" "$(OUT)/root/bin/sh"
	cp "$(OUT)/busybox.elf" "$(OUT)/root/bin/busybox"
	cp "$(OUT)/progtest.elf" "$(OUT)/root/bin/progtest"
	cp "$(OUT)/net.elf" "$(OUT)/root/bin/net"
	cp assets/pi5/wifi.bin assets/pi5/wifi.clm assets/pi5/wifi.txt assets/pi5/license "$(OUT)/root/lib/firmware/"
	cp "$(OUT)/busybox.elf" "$(OUT)/root/bin/cat"
	cp "$(OUT)/busybox.elf" "$(OUT)/root/bin/echo"
	cp "$(OUT)/busybox.elf" "$(OUT)/root/bin/false"
	cp "$(OUT)/busybox.elf" "$(OUT)/root/bin/true"
	cp "$(OUT)/busybox.elf" "$(OUT)/root/bin/pwd"
	cp "$(OUT)/busybox.elf" "$(OUT)/root/bin/printenv"
	cp "$(OUT)/busybox.elf" "$(OUT)/root/bin/basename"
	cp "$(OUT)/busybox.elf" "$(OUT)/root/bin/dirname"
	cp "$(OUT)/busybox.elf" "$(OUT)/root/bin/head"
	cp "$(OUT)/busybox.elf" "$(OUT)/root/bin/wc"
	cp "$(OUT)/busybox.elf" "$(OUT)/root/bin/mkdir"
	cp "$(OUT)/busybox.elf" "$(OUT)/root/bin/rmdir"
	cp "$(OUT)/busybox.elf" "$(OUT)/root/bin/ls"
	cp "$(OUT)/busybox.elf" "$(OUT)/root/bin/cp"
	cp "$(OUT)/busybox.elf" "$(OUT)/root/bin/rm"
	cp "$(OUT)/busybox.elf" "$(OUT)/root/bin/touch"
	cp root/aarch64/etc/programs "$(OUT)/root/etc/programs"
	"$(OUT)/image-tool$(HOSTEXE)" fat "$(OUT)/root" "$(OUT)/lume-root.fat"
$(OUT)/shell.elf $(OUT)/busybox.elf $(OUT)/progtest.elf $(OUT)/net.elf: Makefile $(wildcard apps/aarch64/*.h) $(wildcard $(KERNEL)/include/uapi/aarch64/lume/*.h)
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
CPPFLAGS += -Iinclude/$(ARCH) -I"$(KERNEL)/$(KERNEL_OUT)/include" --sysroot="$(SDK)/sysroot"
LIBS := --start-group "$(SDK)/sysroot/lib/libc.a" "$(SDK)/sysroot/lib/libclang_rt.$(BUILTINS).a" --end-group
COMMON_SOURCES := arch/$(ARCH)/start.s libc/$(ARCH)/api.c libc/$(ARCH)/runtime.c arch/$(ARCH)/memory.s
COMMON_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(COMMON_SOURCES)))
APP_NAMES := init shell worker probe ls dir less fstest libctest video smpdemo top net iostat iobench netd
APP_TARGETS := $(addprefix $(OUT)/bin/,$(addsuffix .$(ARCH),$(APP_NAMES) busybox vi htop dropbear sshshell))
include mk/htop.mk
include mk/dropbear.mk
include mk/busybox-$(ARCH).mk
VI_SOURCES := ports/$(ARCH)/busybox/compat.c ports/$(ARCH)/vi/compat.c ports/$(ARCH)/vi/upstream/vi.c
HTOP_SOURCES += $(addprefix ports/$(ARCH)/htop/,LumeBackend.c curses.c posix.c entry.c runtime.c) $(addprefix libc/$(ARCH)/posix/,sbrk.c time.c posix_runtime.c errno_map.c)
DROPBEAR_SOURCES += $(wildcard ports/$(ARCH)/dropbear/*.c) libc/$(ARCH)/random.c libc/$(ARCH)/posix/errno_map.c libc/$(ARCH)/posix/sbrk.c
BB_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(BB_SOURCES)))
VI_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(VI_SOURCES)))
HTOP_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(HTOP_SOURCES)))
SSH_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(DROPBEAR_SOURCES)))
BASE_OBJECTS := $(filter-out $(OUT)/obj/libc/$(ARCH)/runtime.c.o,$(COMMON_OBJECTS))
$(BB_OBJECTS): CPPFLAGS += -Iports/$(ARCH)/busybox
$(VI_OBJECTS): CPPFLAGS += -Iports/$(ARCH)/vi -Iports/$(ARCH)/busybox
$(HTOP_OBJECTS): CPPFLAGS += -Iports/$(ARCH)/htop -Ivendor/htop -Ivendor/htop/unsupported -Iinclude/$(ARCH)/posix -DPATH_MAX=128 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -D_POSIX_MONOTONIC_CLOCK=200809L -DHAVE_CONFIG_H -DNDEBUG
$(SSH_OBJECTS): CPPFLAGS += -Ilibc/$(ARCH) -Iports/$(ARCH)/dropbear -Iports/$(ARCH)/dropbear/include -Ivendor/dropbear/src -Ivendor/dropbear/libtomcrypt/src/headers -Ivendor/dropbear/libtommath -Iinclude/$(ARCH)/posix -DDROPBEAR_SERVER=1 -DLOCALOPTIONS_H_EXISTS -DBUNDLED_LIBTOM -DLTC_NO_ASM -DMP_32BIT
$(filter %/sha256.c.o %/sha512.c.o,$(SSH_OBJECTS)): CPPFLAGS += -DLTC_SMALL_CODE
$(filter $(OUT)/obj/vendor/dropbear/libtomcrypt/%,$(SSH_OBJECTS)): CPPFLAGS += -DLTC_SOURCE
$(OUT)/obj/libc/$(ARCH)/dhcp.c.o: CPPFLAGS += -Iinclude/$(ARCH)/platform
$(OUT)/obj/apps/$(ARCH)/netd.c.o: CPPFLAGS += -Iinclude/$(ARCH)/platform
all: headers $(APP_TARGETS) kernel
$(COMMON_OBJECTS) $(BB_OBJECTS) $(VI_OBJECTS) $(HTOP_OBJECTS) $(SSH_OBJECTS) $(addprefix $(OUT)/obj/apps/$(ARCH)/,$(addsuffix .c.o,$(APP_NAMES) sshshell)) $(OUT)/obj/libc/$(ARCH)/dhcp.c.o: | headers
$(OUT)/obj/%.c.o: %.c $(OUT)/configuration Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(OUT)/obj/%.s.o: %.s $(OUT)/configuration Makefile
	mkdir -p "$(@D)"
	$(CC) -c "$<" -o "$@"
$(OUT)/bin/%.$(ARCH): $(OUT)/obj/apps/$(ARCH)/%.c.o $(COMMON_OBJECTS) arch/$(ARCH)/user.ld
	mkdir -p "$(@D)"
	$(LD) $(LINK_FLAGS) --gc-sections --build-id=none -T arch/$(ARCH)/user.ld $(COMMON_OBJECTS) "$<" $(LIBS) -o "$@"
$(OUT)/bin/netd.$(ARCH): $(OUT)/obj/apps/$(ARCH)/netd.c.o $(OUT)/obj/libc/$(ARCH)/dhcp.c.o $(COMMON_OBJECTS)
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
$(OUT)/bin/sshshell.$(ARCH): $(OUT)/obj/apps/$(ARCH)/sshshell.c.o $(COMMON_OBJECTS)
	mkdir -p "$(@D)"
	$(file >$@.rsp,$(LINK_FLAGS) --gc-sections --build-id=none -T libc/$(ARCH)/posix/native.ld $(foreach obj,$^,"$(obj)") $(LIBS) -o "$@")
	$(LD) @"$@.rsp"
kernel:
	$(MAKE) -C "$(KERNEL)" ARCH=$(ARCH) BOARD=$(BOARD) SDK="$(SDK)" OUT="$(KERNEL_OUT)" HOSTCXX='$(HOSTCXX)' HOSTCXXFLAGS='$(HOSTCXXFLAGS)'
BOOTLOADER ?=
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

	"$(OUT)/image-tool$(HOSTEXE)" disk "$(OUT)/root" "$(OUT)/lume.img"
endif
headers:
	$(MAKE) -C "$(KERNEL)" ARCH=$(ARCH) OUT="$(KERNEL_OUT)" headers
$(OUT)/image-tool$(HOSTEXE): tools/image.cpp
	mkdir -p "$(@D)"
	$(HOSTCXX) $(HOSTCXXFLAGS) "$<" -o "$@"
check: $(OUT)/tests/net-policy$(HOSTEXE) $(OUT)/tests/netd$(HOSTEXE)
	env "$(OUT)/tests/net-policy$(HOSTEXE)"
	env "$(OUT)/tests/netd$(HOSTEXE)"
$(OUT)/tests/net-policy$(HOSTEXE): tests/aarch64/net_policy_test.c apps/aarch64/net_policy.c
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -Iapps/aarch64 $^ -o "$@"
$(OUT)/tests/netd$(HOSTEXE): tests/aarch64/netd_test.c apps/aarch64/net_policy.c
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -Iapps/aarch64 -I"$(KERNEL)/include/uapi/aarch64" $^ -o "$@"
clean:
	rm -rf -- "$(OUT)"
-include $(addsuffix .d,$(COMMON_OBJECTS) $(BB_OBJECTS) $(VI_OBJECTS) $(HTOP_OBJECTS) $(SSH_OBJECTS) $(addprefix $(OUT)/obj/apps/$(ARCH)/,$(addsuffix .c.o,$(APP_NAMES) sshshell)))

.PHONY: FORCE
FORCE:
$(OUT):
	mkdir -p "$@"
$(OUT)/configuration: FORCE | $(OUT)
	$(file >$@.tmp,$(CC) | $(CPPFLAGS) | $(CFLAGS) | $(SDK) | $(SHELL_ELF))
	cmp -s "$@.tmp" "$@" || cp "$@.tmp" "$@"
