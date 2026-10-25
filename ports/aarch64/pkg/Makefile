.DEFAULT_GOAL := all
OS := ../../..
KERNEL ?= $(OS)/../lume-kernel
SDK ?= /data/sdk
SYSROOT ?= $(SDK)
OUT ?= build
CC = clang
LD = ld.lld
CPPFLAGS += -DLUME_ARCH_AARCH64=1 -I$(OS)/apps/common -I$(OS)/apps/aarch64 -I$(OS)/libc/common -I$(OS)/include/common/posix -I$(OS)/include/common -I$(KERNEL)/include/uapi/aarch64 -I$(KERNEL)/include/uapi/common -isystem $(SYSROOT)/include
CFLAGS += --target=aarch64-unknown-none-elf -mcpu=cortex-a76 -mstrict-align -O2 -std=gnu11 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -ffunction-sections -fdata-sections
OBJECTS := $(OUT)/pkg.o $(OUT)/pkg_platform.o $(OUT)/sha256.o
.PHONY: all clean
all: $(OUT)/pkg
$(OUT)/pkg: $(OBJECTS) $(SDK)/lib/crt0.o $(SDK)/lib/liblume.a $(SDK)/lib/libc.a $(SDK)/native.ld
	$(LD) --gc-sections --build-id=none -z max-page-size=4096 -T $(SDK)/native.ld $(SDK)/lib/crt0.o $(OBJECTS) --start-group $(SDK)/lib/liblume.a $(SDK)/lib/libc.a --end-group -o $@
$(OUT)/pkg.o: $(OS)/apps/common/pkg.c $(OS)/apps/common/pkg_format.h
	mkdir -p $(OUT)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(OUT)/pkg_platform.o: $(OS)/apps/aarch64/pkg_platform.c $(OS)/apps/common/pkg_format.h $(OS)/apps/aarch64/network_tool.h
	mkdir -p $(OUT)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(OUT)/sha256.o: $(OS)/libc/common/sha256.c $(OS)/libc/common/sha256.h
	mkdir -p $(OUT)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
clean:
	rm -f $(OBJECTS) $(OUT)/pkg
