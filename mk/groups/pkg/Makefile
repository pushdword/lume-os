ARCH ?= aarch64
.DEFAULT_GOAL := all
OS := ../../..
OS_ROOT := $(abspath $(OS))
include $(OS_ROOT)/mk/kernel-path.mk
include $(OS)/arch/$(ARCH)/config.mk
KERNEL ?= $(OS)/../lume-kernel
SDK ?= /data/sdk
SYSROOT ?= $(SDK)
OUT ?= build
CC = clang
LD = ld.lld
CPPFLAGS += -I$(OS)/arch/$(ARCH)/include -I$(OS)/include/internal -I$(OS)/include/posix  $(ARCH_CPPFLAGS) -I$(OS)/apps/pkg -I$(OS)/apps/net -I$(OS)/libc/crypto -I$(OS)/include/posix -I$(OS)/include/lume -I$(KERNEL)/include/abi/$(UAPI_ABI) -I$(KERNEL)/include/uapi -isystem $(SYSROOT)/include
CFLAGS += $(ARCH_CFLAGS) -O2 -std=gnu11 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -ffunction-sections -fdata-sections
OBJECTS := $(OUT)/pkg.o $(OUT)/pkg_platform.o $(OUT)/sha256.o
.PHONY: all clean
all: $(OUT)/pkg
$(OUT)/pkg: $(OBJECTS) $(SDK)/lib/crt0.o $(SDK)/lib/liblume.a $(SDK)/lib/libc.a $(SDK)/native.ld
	$(LD) --gc-sections --build-id=none -z max-page-size=4096 -T $(SDK)/native.ld $(SDK)/lib/crt0.o $(OBJECTS) --start-group $(SDK)/lib/liblume.a $(SDK)/lib/libc.a --end-group -o $@
$(OUT)/pkg.o: $(OS)/apps/pkg/pkg.c $(OS)/apps/pkg/pkg_format.h
	mkdir -p $(OUT)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(OUT)/pkg_platform.o: $(OS)/apps/pkg/platform.c $(OS)/apps/pkg/pkg_format.h $(OS)/apps/net/network_tool.h
	mkdir -p $(OUT)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(OUT)/sha256.o: $(OS)/libc/crypto/sha256.c $(OS)/libc/crypto/sha256.h
	mkdir -p $(OUT)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
clean:
	rm -f $(OBJECTS) $(OUT)/pkg
