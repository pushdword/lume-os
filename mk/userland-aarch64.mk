CC := "$(TOOLBIN)clang$(HOSTEXE)" --target=aarch64-unknown-none-elf
LD := "$(TOOLBIN)ld.lld$(HOSTEXE)"
OBJCOPY := "$(TOOLBIN)llvm-objcopy$(HOSTEXE)"
SYSROOT ?= $(SDK)/sysroot
CMAKE ?= cmake
PICOLIBC ?=
.PHONY: libc
libc:
	@test -n "$(PICOLIBC)" || { echo 'PICOLIBC must name a Picolibc source tree'; exit 1; }
	$(CMAKE) -S "$(PICOLIBC)" -B "$(OUT)/picolibc" -DCMAKE_TOOLCHAIN_FILE="$(CURDIR)/libc/aarch64/picolibc.cmake" -DCMAKE_C_COMPILER="$(TOOLBIN)clang$(HOSTEXE)" -DCMAKE_AR="$(TOOLBIN)llvm-ar$(HOSTEXE)" -DCMAKE_RANLIB="$(TOOLBIN)llvm-ranlib$(HOSTEXE)" -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_INSTALL_PREFIX="$(SYSROOT)"
	$(CMAKE) --build "$(OUT)/picolibc" --parallel
	$(CMAKE) --install "$(OUT)/picolibc"
CFLAGS := -O2 -std=gnu11 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -fno-unwind-tables -fno-asynchronous-unwind-tables -mcpu=cortex-a76 -mstrict-align -ffunction-sections -fdata-sections
CPPFLAGS := -DLUME_ARCH_AARCH64=1 -Iinclude/common -I"$(KERNEL)/$(KERNEL_OUT)/include" -isystem "$(SYSROOT)/include"
ARM_LINK := -nostdlib -static -Wl,--gc-sections,--build-id=none,--strip-debug,-z,max-page-size=4096,-T,apps/aarch64/user.ld
ARM_LIBS := -Wl,--start-group "$(SYSROOT)/lib/libc.a" -Wl,--end-group
ARM_LD_FLAGS := --gc-sections --build-id=none --strip-debug -z max-page-size=4096 -T apps/aarch64/user.ld
ARM_LD_LIBS := --start-group "$(SYSROOT)/lib/libc.a" --end-group
ifeq ($(DEBUG),1)
CFLAGS += -g -fno-omit-frame-pointer
ARM_LD_FLAGS := $(filter-out --strip-debug,$(ARM_LD_FLAGS))
endif
ARM_COMMON_SOURCES := arch/aarch64/start.S libc/common/api.c libc/common/runtime.c libc/aarch64/api.c libc/common/posix/sbrk.c libc/common/posix/errno_map.c
ARM_COMMON_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(ARM_COMMON_SOURCES)))
include mk/busybox.mk
include mk/htop.mk
ARM_APPS := shell ls dir less top worker probe fstest libctest video smpdemo iostat iobench cp touch tar base64
ARM_APPLETS := basename cat dirname echo false head printenv pwd true wc sort cut cmp cksum crc32 expand unexpand mkdir rmdir
ARM_APP_TARGETS := $(addprefix $(OUT)/,$(addsuffix .elf,$(ARM_APPS)))
VI_SOURCES := ports/common/busybox/compat.c ports/common/vi/compat.c ports/common/vi/upstream/vi.c
HTOP_SOURCES += $(addprefix ports/common/htop/,LumeBackend.c curses.c posix.c entry.c runtime.c) $(addprefix libc/common/posix/,sbrk.c time.c posix_runtime.c errno_map.c)
BB_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(BB_SOURCES)))
VI_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(VI_SOURCES)))
HTOP_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(HTOP_SOURCES)))
$(BB_OBJECTS): CPPFLAGS += -Iports/common/busybox
$(VI_OBJECTS): CPPFLAGS += -Iports/common/vi -Iports/common/busybox
$(HTOP_OBJECTS): CPPFLAGS += -Iports/common/htop -Ivendor/htop -Ivendor/htop/unsupported -Iinclude/common/posix -DPATH_MAX=128 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -D_POSIX_MONOTONIC_CLOCK=200809L -DHAVE_CONFIG_H -DNDEBUG
$(OUT)/obj/apps/common/libctest.c.o: CPPFLAGS += -DLUME_TEST_HEAP_EXCESS=1073741824
all: headers $(ARM_APP_TARGETS) $(OUT)/busybox.elf $(OUT)/vi.elf $(OUT)/htop.elf $(OUT)/net.elf $(OUT)/netd.elf $(OUT)/update.elf $(OUT)/fetch.elf $(OUT)/ping.elf $(OUT)/nc.elf kernel
$(ARM_COMMON_OBJECTS) $(BB_OBJECTS) $(VI_OBJECTS) $(HTOP_OBJECTS) $(addprefix $(OUT)/obj/apps/common/,$(addsuffix .c.o,$(ARM_APPS))): | headers
$(OUT)/obj/%.c.o: %.c $(OUT)/configuration Makefile mk/userland-aarch64.mk
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(OUT)/obj/%.S.o: %.S $(OUT)/configuration Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -c "$<" -o "$@"
$(OUT)/%.elf: $(OUT)/obj/apps/common/%.c.o $(ARM_COMMON_OBJECTS)
	$(LD) $(ARM_LD_FLAGS) $^ $(ARM_LD_LIBS) -o "$@"
$(OUT)/busybox.elf: $(BB_OBJECTS) $(ARM_COMMON_OBJECTS)
	$(LD) $(ARM_LD_FLAGS) $^ --defsym=__d_vfprintf=__l_vfprintf --defsym=vfprintf=__l_vfprintf $(ARM_LD_LIBS) -o "$@"
$(OUT)/vi.elf: $(VI_OBJECTS) $(ARM_COMMON_OBJECTS)
	$(LD) $(ARM_LD_FLAGS) $^ --defsym=__d_vfprintf=__l_vfprintf --defsym=vfprintf=__l_vfprintf $(ARM_LD_LIBS) -o "$@"
$(OUT)/htop.elf: $(HTOP_OBJECTS) $(filter-out $(OUT)/obj/libc/common/runtime.c.o $(OUT)/obj/libc/common/posix/sbrk.c.o $(OUT)/obj/libc/common/posix/errno_map.c.o,$(ARM_COMMON_OBJECTS))
	$(LD) $(ARM_LD_FLAGS) $^ $(ARM_LD_LIBS) -o "$@"
$(OUT)/netd.elf: apps/aarch64/start.S apps/aarch64/netd.c apps/aarch64/net_policy.c libc/aarch64/memory.c | headers
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) -Iapps/aarch64 $(CFLAGS) $(ARM_LINK) $(filter %.c %.S,$^) -o "$@"
$(OUT)/net.elf: apps/aarch64/start.S apps/aarch64/net.c apps/aarch64/net_client.h apps/aarch64/net_service_client.h libc/aarch64/memory.c | headers
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) -Iapps/aarch64 $(CFLAGS) $(ARM_LINK) $(filter %.c %.S,$^) -o "$@"
$(OUT)/ping.elf $(OUT)/nc.elf: $(OUT)/%.elf: apps/aarch64/%.c apps/aarch64/network_tool.h apps/aarch64/lib.h apps/aarch64/start.S libc/aarch64/memory.c | headers
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) -Iapps/aarch64 $(CFLAGS) $(ARM_LINK) $(filter %.c %.S,$^) -o "$@"
$(OUT)/update.elf: apps/aarch64/start.S apps/common/update.c libc/common/sha256.c libc/common/sha256.h libc/aarch64/memory.c | headers
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) -Iapps/aarch64 -Ilibc/common $(CFLAGS) $(ARM_LINK) $(filter %.c %.S,$^) -o "$@"
$(OUT)/fetch.elf: apps/aarch64/start.S apps/common/fetch.c libc/common/sha256.c libc/common/sha256.h libc/aarch64/memory.c | headers
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) -Iapps/aarch64 -Ilibc/common $(CFLAGS) $(ARM_LINK) $(filter %.c %.S,$^) -o "$@"
kernel: $(OUT)/shell.elf
	$(MAKE) -C "$(KERNEL)" ARCH=$(ARCH) BOARD=$(BOARD) SDK="$(SDK)" OUT="$(KERNEL_OUT)" SHELL_ELF="$(CURDIR)/$(OUT)/shell.elf" HOSTCXX='$(HOSTCXX)' HOSTCXXFLAGS='$(HOSTCXXFLAGS)'
include mk/recovery-aarch64.mk
$(RECOVERY_OBJECTS): CFLAGS += -mgeneral-regs-only
image: all $(OUT)/image-tool$(HOSTEXE)
	rm -rf -- "$(OUT)/root"
	mkdir -p "$(OUT)/root/bin" "$(OUT)/root/usr/bin" "$(OUT)/root/usr/libexec" "$(OUT)/root/lib/firmware" "$(OUT)/root/boot" "$(OUT)/root/etc"
	cp "$(KERNEL)/$(KERNEL_OUT)/lume-pi5.img" "$(OUT)/root/boot/kernel.img"
	cp assets/pi5/* "$(OUT)/root/lib/firmware/"
	cp root/aarch64/etc/* "$(OUT)/root/etc/"
	printf 'standalone\n' > "$(OUT)/root/etc/bootmode"
	$(foreach app,busybox ls dir net netd update fetch cp touch tar base64 ping nc,cp "$(OUT)/$(app).elf" "$(OUT)/root/bin/$(app)";)
	cp "$(OUT)/nc.elf" "$(OUT)/root/bin/netcat"
	$(foreach app,less top vi htop video smpdemo iostat iobench,cp "$(OUT)/$(app).elf" "$(OUT)/root/usr/bin/$(app)";)
	$(foreach app,worker probe fstest libctest,cp "$(OUT)/$(app).elf" "$(OUT)/root/usr/libexec/$(app)";)
	$(foreach app,$(ARM_APPLETS),MSYS2_ARG_CONV_EXCL='*' "$(OUT)/image-tool$(HOSTEXE)" alias /bin/busybox "$(OUT)/root/bin/$(app)";)
	"$(OUT)/image-tool$(HOSTEXE)" fat "$(OUT)/root" "$(OUT)/lume-root.fat"
.PHONY: check-update check-network-tools
check-network-tools: $(OUT)/tests/network-tool$(HOSTEXE)
	env "$(OUT)/tests/network-tool$(HOSTEXE)"
$(OUT)/tests/network-tool$(HOSTEXE): tests/aarch64/network_tool_test.c apps/aarch64/network_tool.h apps/aarch64/lib.h
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -fno-builtin -I"$(KERNEL)/include/uapi/aarch64" "$<" -o "$@"
check-update: $(OUT)/tests/sha256$(HOSTEXE) $(OUT)/tests/update$(HOSTEXE) $(OUT)/tests/abi$(HOSTEXE)
	env "$(OUT)/tests/sha256$(HOSTEXE)"
	env "$(OUT)/tests/update$(HOSTEXE)"
	env "$(OUT)/tests/abi$(HOSTEXE)"
$(OUT)/tests/abi$(HOSTEXE): tests/aarch64/abi_test.c libc/aarch64/api.c
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -DLUME_USER_HOST_TEST=1 -DLUME_ARCH_AARCH64=1 -Iinclude/common -I"$(KERNEL)/include/uapi/common" -I"$(KERNEL)/include/uapi/aarch64" $^ -o "$@"
$(OUT)/tests/sha256$(HOSTEXE): tests/aarch64/sha256_test.c libc/common/sha256.c libc/common/sha256.h
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -Ilibc/common $(filter %.c,$^) -o "$@"
$(OUT)/tests/update$(HOSTEXE): tests/aarch64/update_test.c apps/common/update.c libc/common/sha256.c libc/common/sha256.h
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -Iapps/aarch64 -Ilibc/common -I"$(KERNEL)/include/uapi/aarch64" tests/aarch64/update_test.c libc/common/sha256.c -o "$@"
-include $(addsuffix .d,$(ARM_COMMON_OBJECTS) $(BB_OBJECTS) $(VI_OBJECTS) $(HTOP_OBJECTS) $(addprefix $(OUT)/obj/apps/common/,$(addsuffix .c.o,$(ARM_APPS))))
