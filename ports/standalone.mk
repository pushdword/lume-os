OS_ROOT := $(abspath $(PORT)/../..)
include $(OS_ROOT)/mk/port.mk
SDK ?= /data/sdk
SYSROOT ?= $(SDK)
OUT ?= build
CC = clang
AR = llvm-ar
LD = ld.lld
TARGET ?= aarch64-unknown-none-elf
BASE_CPPFLAGS := -D__lume__=1 $(ARCH_CPPFLAGS) -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -D_POSIX_MONOTONIC_CLOCK=200809L -I$(OS_ROOT)/arch/$(ARCH)/include -I$(OS_ROOT)/include/internal -I$(OS_ROOT)/include/lume -I$(OS_ROOT)/include/posix -I$(KERNEL)/include/abi/$(UAPI_ABI) -I$(KERNEL)/include/uapi -isystem $(SYSROOT)/include
CPPFLAGS += $(BASE_CPPFLAGS)
CFLAGS += -O2 -std=$(CSTD) -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -ffunction-sections -fdata-sections -ffile-prefix-map=$(OS_ROOT)=lume-os -ffile-prefix-map=$(KERNEL)=lume-kernel
CFLAGS += -ffile-prefix-map=$(SOURCE)=lume-os/vendor/$(notdir $(PORT))
CXXFLAGS += $(ARCH_CFLAGS) -O2 -std=c++17 -fhosted -fno-builtin -fno-stack-protector -fno-pic -fno-pie -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -nostdinc++ -isystem $(SYSROOT)/include/c++/v1 -isystem $(SYSROOT)/include
CXXFLAGS += -ffile-prefix-map=$(OS_ROOT)=lume-os -ffile-prefix-map=$(KERNEL)=lume-kernel -ffile-prefix-map=$(SOURCE)=lume-os/vendor/$(notdir $(PORT))
LDFLAGS += --gc-sections --build-id=none -z max-page-size=4096 -T $(SDK)/native.ld
RUNTIME := $(SDK)/lib/crt0.o
LIBS := --start-group $(SDK)/lib/liblume.a $(SDK)/lib/libc.a $(SDK)/lib/libcrt.a --end-group
$(OUT)/obj/%.o: $(SOURCE)/%.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(OUT)/obj/%.o: $(SOURCE)/%.cc
	mkdir -p $(@D)
	$(CC) $(subst -isystem $(SYSROOT)/include,,$(CPPFLAGS)) $(CXXFLAGS) -c $< -o $@
$(OUT)/obj/%.o: $(SOURCE)/%.cxx
	mkdir -p $(@D)
	$(CC) $(subst -isystem $(SYSROOT)/include,,$(CPPFLAGS)) $(CXXFLAGS) -c $< -o $@
$(OUT)/obj/port/%.o: $(PORT)/%.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(OUT)/obj/shared/%.o: $(PORT)/../standalone/%.c
	mkdir -p $(@D)
	$(CC) $(BASE_CPPFLAGS) -I$(OS_ROOT)/ports/standalone/include $(CFLAGS) -c $< -o $@
$(OUT)/obj/os/%.o: $(OS_ROOT)/%.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
