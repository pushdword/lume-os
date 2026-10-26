CC := "$(TOOLBIN)clang$(HOSTEXE)" --target=$(TARGET)
LD := "$(TOOLBIN)ld.lld$(HOSTEXE)"
OBJCOPY := "$(TOOLBIN)llvm-objcopy$(HOSTEXE)"
SYSROOT ?= $(SDK)/sysroot
NATIVE_KERNEL_IMAGE := $(if $(filter amd64,$(ARCH)),lume-amd64.bin,lume-pi5.img)
CMAKE ?= cmake
PICOLIBC ?=
.PHONY: libc
libc:
	@test -n "$(PICOLIBC)" || { echo 'PICOLIBC must name a Picolibc source tree'; exit 1; }
	$(CMAKE) -S "$(PICOLIBC)" -B "$(OUT)/picolibc" -DCMAKE_TOOLCHAIN_FILE="$(CURDIR)/arch/$(ARCH)/picolibc.cmake" -DCMAKE_C_COMPILER="$(TOOLBIN)clang$(HOSTEXE)" -DCMAKE_AR="$(TOOLBIN)llvm-ar$(HOSTEXE)" -DCMAKE_RANLIB="$(TOOLBIN)llvm-ranlib$(HOSTEXE)" -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_INSTALL_PREFIX="$(SYSROOT)"
	$(CMAKE) --build "$(OUT)/picolibc" --parallel
	$(CMAKE) --install "$(OUT)/picolibc"
CFLAGS := -O2 -std=gnu11 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -fno-unwind-tables -fno-asynchronous-unwind-tables $(filter-out --target=%,$(ARCH_CFLAGS)) -ffunction-sections -fdata-sections
CPPFLAGS := $(ARCH_CPPFLAGS) -Iinclude/lume -Iinclude/posix -Iarch/$(ARCH)/include -Iinclude/internal -Iapps/net -Iservices/netd -Iservices/svcd -Iapps/pkg -Ilibc/posix -Ilibc/crypto -I"$(KERNEL)/$(KERNEL_OUT)/include" -isystem "$(SYSROOT)/include"
ARM_LINK := -nostdlib -static -Wl,--gc-sections,--build-id=none,--strip-debug,-z,max-page-size=4096,-T,arch/$(ARCH)/user.ld
ARM_LIBS := -Wl,--start-group "$(SYSROOT)/lib/libc.a" -Wl,--end-group
ARM_LD_FLAGS := --gc-sections --build-id=none --strip-debug -z max-page-size=4096 -T arch/$(ARCH)/user.ld
ARM_LD_LIBS := --start-group "$(SYSROOT)/lib/libc.a" --end-group
ifeq ($(DEBUG),1)
CFLAGS += -g -fno-omit-frame-pointer
ARM_LD_FLAGS := $(filter-out --strip-debug,$(ARM_LD_FLAGS))
endif
ARM_COMMON_SOURCES := arch/$(ARCH)/crt0.S libc/lume/api.c libc/runtime/runtime.c libc/lume/native.c libc/posix/sbrk.c libc/posix/errno_map.c libc/lume/thread.c arch/$(ARCH)/tls.c libc/runtime/locks.c libc/posix/pthread.c
ARM_COMMON_SOURCES += libc/posix/time.c
$(OUT)/obj/libc/posix/pthread.c.o $(OUT)/obj/libc/lume/thread.c.o $(OUT)/obj/arch/$(ARCH)/tls.c.o $(OUT)/obj/libc/runtime/locks.c.o: CPPFLAGS += -Iinclude/posix -D_POSIX_C_SOURCE=200809L -D_POSIX_MONOTONIC_CLOCK=200809L
ARM_COMMON_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(ARM_COMMON_SOURCES)))
include mk/busybox.mk
include mk/htop.mk
ARM_APPS := shell ls dir less top worker probe fstest libctest video smpdemo iostat iobench cp touch tar base64
ARM_APPLETS := basename cat dirname echo false head printenv pwd true wc sort cut cmp cksum crc32 expand unexpand mkdir rmdir
ARM_APP_TARGETS := $(addprefix $(OUT)/,$(addsuffix .elf,$(ARM_APPS)))
VI_SOURCES := ports/busybox/compat.c ports/vi/compat.c vendor/vi/vi.c
HTOP_SOURCES += $(addprefix ports/htop/,LumeBackend.c curses.c posix.c entry.c runtime.c) $(addprefix libc/posix/,sbrk.c posix_runtime.c errno_map.c)
BB_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(BB_SOURCES)))
VI_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(VI_SOURCES)))
HTOP_OBJECTS := $(addprefix $(OUT)/obj/,$(addsuffix .o,$(HTOP_SOURCES)))
$(BB_OBJECTS): CPPFLAGS += -Iports/busybox
$(VI_OBJECTS): CPPFLAGS += -Iports/vi -Iports/busybox
$(HTOP_OBJECTS): CPPFLAGS += -Iports/htop -Ivendor/htop -Ivendor/htop/unsupported -Iinclude/posix -DPATH_MAX=128 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -D_POSIX_MONOTONIC_CLOCK=200809L -DHAVE_CONFIG_H -DNDEBUG
$(OUT)/obj/apps/diagnostics/libctest.c.o: CPPFLAGS += -DLUME_TEST_HEAP_EXCESS=1073741824
all: headers $(ARM_APP_TARGETS) $(OUT)/busybox.elf $(OUT)/vi.elf $(OUT)/htop.elf $(OUT)/net.elf $(OUT)/netd.elf $(OUT)/update.elf $(OUT)/fetch.elf $(OUT)/ping.elf $(OUT)/nc.elf $(OUT)/device.elf $(OUT)/module.elf kernel
ifeq ($(ARCH),amd64)
all: $(OUT)/amd64-runtime.elf
$(OUT)/amd64-runtime.elf: $(OUT)/obj/tests/integration/amd64_runtime.c.o $(ARM_COMMON_OBJECTS)
	$(LD) $(ARM_LD_FLAGS) $^ $(ARM_LD_LIBS) -o "$@"
$(OUT)/obj/tests/integration/amd64_runtime.c.o: | headers
endif
$(OUT)/tests/pthread-probe.elf: tests/integration/pthread_probe.c $(ARM_COMMON_OBJECTS) | headers
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) -Iinclude/posix $(CFLAGS) $(ARM_LINK) $^ $(ARM_LIBS) -o "$@"
$(ARM_COMMON_OBJECTS) $(BB_OBJECTS) $(VI_OBJECTS) $(HTOP_OBJECTS) $(ARM_APP_OBJECTS): | headers
$(OUT)/obj/%.c.o: %.c $(OUT)/configuration Makefile mk/userland-native.mk
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(OUT)/obj/%.S.o: %.S $(OUT)/configuration Makefile
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -c "$<" -o "$@"
define native_application
$(OUT)/$(1).elf: $(OUT)/obj/$(APP_SOURCE_$(1)).o $(ARM_COMMON_OBJECTS)
	$(LD) $(ARM_LD_FLAGS) $$^ $(ARM_LD_LIBS) -o "$$@"
endef
$(foreach app,$(ARM_APPS),$(eval $(call native_application,$(app))))
$(OUT)/busybox.elf: $(BB_OBJECTS) $(ARM_COMMON_OBJECTS)
	$(LD) $(ARM_LD_FLAGS) $^ --defsym=__d_vfprintf=__l_vfprintf --defsym=vfprintf=__l_vfprintf $(ARM_LD_LIBS) -o "$@"
$(OUT)/vi.elf: $(VI_OBJECTS) $(ARM_COMMON_OBJECTS)
	$(LD) $(ARM_LD_FLAGS) $^ --defsym=__d_vfprintf=__l_vfprintf --defsym=vfprintf=__l_vfprintf $(ARM_LD_LIBS) -o "$@"
$(OUT)/htop.elf: $(HTOP_OBJECTS) $(filter-out $(OUT)/obj/libc/runtime/runtime.c.o $(OUT)/obj/libc/posix/sbrk.c.o $(OUT)/obj/libc/posix/errno_map.c.o,$(ARM_COMMON_OBJECTS))
	$(LD) $(ARM_LD_FLAGS) $^ $(ARM_LD_LIBS) -o "$@"
$(OUT)/netd.elf: arch/$(ARCH)/start_bare.S services/netd/netd.c services/netd/net_policy.c libc/string/memory.c | headers
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) -Iapps/net -Iservices/netd -Iservices/svcd -Iapps/pkg -Iarch/$(ARCH)/include -Iinclude/internal $(CFLAGS) $(ARM_LINK) $(filter %.c %.S,$^) -o "$@"
$(OUT)/net.elf: arch/$(ARCH)/start_bare.S apps/net/net.c apps/net/net_client.h apps/net/net_service_client.h libc/string/memory.c | headers
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) -Iapps/net -Iservices/netd -Iservices/svcd -Iapps/pkg -Iarch/$(ARCH)/include -Iinclude/internal $(CFLAGS) $(ARM_LINK) $(filter %.c %.S,$^) -o "$@"
$(OUT)/ping.elf $(OUT)/nc.elf: $(OUT)/%.elf: apps/net/%.c apps/net/network_tool.h include/internal/lume/app_support.h arch/$(ARCH)/start_bare.S libc/string/memory.c | headers
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) -Iapps/net -Iservices/netd -Iservices/svcd -Iapps/pkg -Iarch/$(ARCH)/include -Iinclude/internal $(CFLAGS) $(ARM_LINK) $(filter %.c %.S,$^) -o "$@"
$(OUT)/update.elf: arch/$(ARCH)/start_bare.S apps/update/update.c libc/crypto/sha256.c libc/crypto/sha256.h libc/string/memory.c | headers
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) -Iapps/net -Iservices/netd -Iservices/svcd -Iapps/pkg -Iarch/$(ARCH)/include -Iinclude/internal -Ilibc/lume -Ilibc/crypto -Ilibc/posix $(CFLAGS) $(ARM_LINK) $(filter %.c %.S,$^) -o "$@"
$(OUT)/fetch.elf: arch/$(ARCH)/start_bare.S apps/update/fetch.c libc/crypto/sha256.c libc/crypto/sha256.h libc/string/memory.c | headers
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) -Iapps/net -Iservices/netd -Iservices/svcd -Iapps/pkg -Iarch/$(ARCH)/include -Iinclude/internal -Ilibc/lume -Ilibc/crypto -Ilibc/posix $(CFLAGS) $(ARM_LINK) $(filter %.c %.S,$^) -o "$@"
NATIVE_SDK ?=
.PHONY: pkg
ifneq ($(strip $(NATIVE_SDK)),)
PKG_OBJECTS := $(OUT)/obj/apps/pkg/pkg.c.o $(OUT)/obj/apps/pkg/platform.c.o $(OUT)/obj/libc/crypto/sha256.c.o
$(PKG_OBJECTS): CPPFLAGS += -Iapps/net -Iservices/netd -Iservices/svcd -Iapps/pkg -Iarch/$(ARCH)/include -Iinclude/internal -Iapps/utils -Iapps/shell -Iapps/diagnostics -Iservices/svcd -Iapps/pkg -Ilibc/lume -Ilibc/crypto -Ilibc/posix -Iinclude/posix
$(PKG_OBJECTS): | headers
SERVICE_OBJECTS := $(OUT)/obj/services/svcd/service.c.o $(OUT)/obj/services/svcd/platform.c.o
$(SERVICE_OBJECTS): CPPFLAGS += -Iapps/net -Iservices/netd -Iservices/svcd -Iapps/pkg -Iarch/$(ARCH)/include -Iinclude/internal -Iapps/utils -Iapps/shell -Iapps/diagnostics -Iservices/svcd -Iapps/pkg -Iinclude/posix
$(SERVICE_OBJECTS): | headers
all pkg: $(OUT)/pkg.elf
all: $(OUT)/service.elf $(OUT)/svcd.elf
$(OUT)/service.elf: $(SERVICE_OBJECTS) $(NATIVE_SDK)/lib/crt0.o $(NATIVE_SDK)/lib/liblume.a $(NATIVE_SDK)/lib/libc.a $(NATIVE_SDK)/native.ld
	$(LD) --gc-sections --build-id=none -z max-page-size=4096 -T "$(NATIVE_SDK)/native.ld" "$(NATIVE_SDK)/lib/crt0.o" $(SERVICE_OBJECTS) --start-group "$(NATIVE_SDK)/lib/liblume.a" "$(NATIVE_SDK)/lib/libc.a" --end-group -o "$@"
$(OUT)/svcd.elf: $(OUT)/service.elf
	cp "$<" "$@"
$(OUT)/pkg.elf: $(PKG_OBJECTS) $(NATIVE_SDK)/lib/crt0.o $(NATIVE_SDK)/lib/liblume.a $(NATIVE_SDK)/lib/libc.a $(NATIVE_SDK)/native.ld
	$(LD) --gc-sections --build-id=none -z max-page-size=4096 -T "$(NATIVE_SDK)/native.ld" "$(NATIVE_SDK)/lib/crt0.o" $(PKG_OBJECTS) --start-group "$(NATIVE_SDK)/lib/liblume.a" "$(NATIVE_SDK)/lib/libc.a" --end-group -o "$@"
else
pkg:
	@echo 'NATIVE_SDK must name the native TLS runtime SDK'
	@exit 1
endif
kernel: $(OUT)/shell.elf
	$(MAKE) -C "$(KERNEL)" ARCH=$(ARCH) BOARD=$(BOARD) SDK="$(SDK)" OUT="$(KERNEL_OUT)" SHELL_ELF="$(CURDIR)/$(OUT)/shell.elf" HOSTCXX='$(HOSTCXX)' HOSTCXXFLAGS='$(HOSTCXXFLAGS)'
ifneq ($(wildcard platform/$(BOARD)/config.mk),)
include platform/$(BOARD)/config.mk
endif
ifneq ($(wildcard platform/$(BOARD)/recovery.mk),)
include platform/$(BOARD)/recovery.mk
endif
$(RECOVERY_OBJECTS): CFLAGS += -mgeneral-regs-only
image: all $(OUT)/image-tool$(HOSTEXE)
	rm -rf -- "$(OUT)/root"
	mkdir -p "$(OUT)/root/bin" "$(OUT)/root/usr/bin" "$(OUT)/root/usr/libexec" "$(OUT)/root/lib/firmware" "$(OUT)/root/lib/modules" "$(OUT)/root/boot" "$(OUT)/root/etc"
	cp "$(KERNEL)/$(KERNEL_OUT)/$(NATIVE_KERNEL_IMAGE)" "$(OUT)/root/boot/kernel.img"
ifneq ($(ARCH),amd64)
	cp assets/firmware/cyw43455/* "$(OUT)/root/lib/firmware/"
	$(foreach name,gic timer watchdog dma uart sd usb display entropy wifi gem,cp "$(KERNEL)/$(KERNEL_OUT)/modules/$(name).lkm" "$(OUT)/root/lib/modules/";)
	cp root/profiles/native/etc/* "$(OUT)/root/etc/"
else
	$(foreach name,vblk vrng vga ps2 xhci $(if $(filter recovery,$(PROFILE)),,vnet),cp "$(KERNEL)/$(KERNEL_OUT)/modules/$(name).lkm" "$(OUT)/root/lib/modules/";)
	cp root/profiles/q35/etc/* "$(OUT)/root/etc/"
ifeq ($(PROFILE),recovery)
	cp root/profiles/q35-recovery/etc/modules "$(OUT)/root/etc/modules"
endif
endif
	printf 'standalone\n' > "$(OUT)/root/etc/bootmode"
	$(foreach app,busybox ls dir net netd update fetch cp touch tar base64 ping nc device module,cp "$(OUT)/$(app).elf" "$(OUT)/root/bin/$(app)";)
	$(if $(strip $(NATIVE_SDK)),cp "$(OUT)/pkg.elf" "$(OUT)/root/bin/pkg",true)
	$(if $(strip $(NATIVE_SDK)),cp "$(OUT)/service.elf" "$(OUT)/root/bin/service"; cp "$(OUT)/svcd.elf" "$(OUT)/root/bin/svcd",true)
	cp "$(OUT)/nc.elf" "$(OUT)/root/bin/netcat"
	cp "$(OUT)/device.elf" "$(OUT)/root/bin/hwreg"
	$(foreach app,less top vi htop video smpdemo iostat iobench,cp "$(OUT)/$(app).elf" "$(OUT)/root/usr/bin/$(app)";)
	$(foreach app,worker probe fstest libctest,cp "$(OUT)/$(app).elf" "$(OUT)/root/usr/libexec/$(app)";)
ifeq ($(ARCH),amd64)
	cp "$(OUT)/amd64-runtime.elf" "$(OUT)/root/usr/libexec/amdtest"
endif
	$(foreach app,$(ARM_APPLETS),MSYS2_ARG_CONV_EXCL='*' "$(OUT)/image-tool$(HOSTEXE)" alias /bin/busybox "$(OUT)/root/bin/$(app)";)
	"$(OUT)/image-tool$(HOSTEXE)" fat "$(OUT)/root" "$(OUT)/lume-root.fat"
.PHONY: check-update check-network-tools check-services
check-services: $(OUT)/tests/service$(HOSTEXE) $(OUT)/tests/supervision$(HOSTEXE)
	env "$(OUT)/tests/service$(HOSTEXE)"
	env "$(OUT)/tests/supervision$(HOSTEXE)"
$(OUT)/tests/service$(HOSTEXE): tests/unit/service_test.c services/svcd/service_config.h services/svcd/service_policy.h
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -UNDEBUG -Iapps/utils -Iapps/shell -Iapps/diagnostics -Iservices/svcd -Iapps/pkg -I"$(KERNEL)/include/abi/native" "$<" -o "$@"
$(OUT)/tests/supervision$(HOSTEXE): $(KERNEL)/tests/unit/supervision_test.c $(KERNEL)/src/core/supervision_bus.h
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -UNDEBUG -I"$(KERNEL)/src/core" -I"$(KERNEL)/include/abi/native" -I"$(KERNEL)/include/uapi" "$<" -o "$@"
check-network-tools: $(OUT)/tests/network-tool$(HOSTEXE) $(OUT)/tests/ping$(HOSTEXE)
	env "$(OUT)/tests/network-tool$(HOSTEXE)"
	env "$(OUT)/tests/ping$(HOSTEXE)"
$(OUT)/tests/ping$(HOSTEXE): tests/unit/ping_test.c apps/net/ping.c apps/net/network_tool.h include/internal/lume/app_support.h
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -fno-builtin -Iapps/net -Iservices/netd -Iservices/svcd -Iapps/pkg -Iarch/$(ARCH)/include -Iinclude/internal -I"$(KERNEL)/include/uapi" -I"$(KERNEL)/include/abi/native" "$<" -o "$@"
$(OUT)/tests/network-tool$(HOSTEXE): tests/unit/network_tool_test.c apps/net/network_tool.h include/internal/lume/app_support.h
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -fno-builtin -I"$(KERNEL)/include/abi/native" "$<" -o "$@"
check-update: $(OUT)/tests/sha256$(HOSTEXE) $(OUT)/tests/update$(HOSTEXE) $(OUT)/tests/abi$(HOSTEXE)
	env "$(OUT)/tests/sha256$(HOSTEXE)"
	env "$(OUT)/tests/update$(HOSTEXE)"
	env "$(OUT)/tests/abi$(HOSTEXE)"
$(OUT)/tests/abi$(HOSTEXE): tests/unit/abi_test.c libc/lume/native.c
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -DLUME_USER_HOST_TEST=1 $(ARCH_CPPFLAGS) -Iinclude/lume -Iinclude/posix -Iarch/$(ARCH)/include -Iinclude/internal -Iapps/net -Iservices/netd -Iservices/svcd -Iapps/pkg -Ilibc/posix -Ilibc/crypto -I"$(KERNEL)/include/uapi" -I"$(KERNEL)/include/abi/native" $^ -o "$@"
$(OUT)/tests/sha256$(HOSTEXE): tests/unit/sha256_test.c libc/crypto/sha256.c libc/crypto/sha256.h
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -Ilibc/lume -Ilibc/crypto -Ilibc/posix $(filter %.c,$^) -o "$@"
$(OUT)/tests/update$(HOSTEXE): tests/unit/update_test.c apps/update/update.c libc/crypto/sha256.c libc/crypto/sha256.h
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -Iapps/net -Iservices/netd -Iservices/svcd -Iapps/pkg -Iarch/$(ARCH)/include -Iinclude/internal -Ilibc/lume -Ilibc/crypto -Ilibc/posix -I"$(KERNEL)/include/abi/native" tests/unit/update_test.c libc/crypto/sha256.c -o "$@"
-include $(addsuffix .d,$(ARM_COMMON_OBJECTS) $(BB_OBJECTS) $(VI_OBJECTS) $(HTOP_OBJECTS) $(ARM_APP_OBJECTS))

$(OUT)/device.elf: arch/$(ARCH)/start_bare.S apps/device/device.c libc/string/memory.c | headers
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) -Iapps/net -Iservices/netd -Iservices/svcd -Iapps/pkg -Iarch/$(ARCH)/include -Iinclude/internal $(CFLAGS) $(ARM_LINK) $(filter %.c %.S,$^) -o "$@"
$(OUT)/module.elf: arch/$(ARCH)/start_bare.S apps/module/module.c libc/string/memory.c | headers
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) -Iapps/net -Iservices/netd -Iservices/svcd -Iapps/pkg -Iarch/$(ARCH)/include -Iinclude/internal $(CFLAGS) $(ARM_LINK) $(filter %.c %.S,$^) -o "$@"
$(OUT)/tests/module-probe.elf: arch/$(ARCH)/start_bare.S tests/integration/module_probe.c libc/string/memory.c | headers
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) -Iapps/net -Iservices/netd -Iservices/svcd -Iapps/pkg -Iarch/$(ARCH)/include -Iinclude/internal $(CFLAGS) $(ARM_LINK) $(filter %.c %.S,$^) -o "$@"
$(OUT)/tests/module-net-probe.elf: arch/$(ARCH)/start_bare.S tests/integration/module_net_probe.c libc/string/memory.c | headers
	mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) -Iapps/net -Iservices/netd -Iservices/svcd -Iapps/pkg -Iarch/$(ARCH)/include -Iinclude/internal $(CFLAGS) $(ARM_LINK) $(filter %.c %.S,$^) -o "$@"
