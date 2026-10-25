.DEFAULT_GOAL := all
OS_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../..)
KERNEL ?= $(OS_ROOT)/../lume-kernel
SDK ?= /data/sdk
SYSROOT ?= $(SDK)
OUT ?= build
CC = clang
LD = ld.lld
CPPFLAGS += -I$(OS_ROOT)/apps/common -I$(OS_ROOT)/include/common/posix -I$(OS_ROOT)/include/common -I$(KERNEL)/include/uapi/aarch64 -I$(KERNEL)/include/uapi/common -isystem $(SYSROOT)/include
CFLAGS += --target=aarch64-unknown-none-elf -mcpu=cortex-a76 -mstrict-align -O2 -std=gnu11 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -ffunction-sections -fdata-sections
OBJECTS := $(OUT)/service.o $(OUT)/service_platform.o
.PHONY: all clean
all: $(OUT)/service $(OUT)/svcd
$(OUT)/service: $(OBJECTS) $(SDK)/lib/crt0.o $(SDK)/lib/liblume.a $(SDK)/lib/libc.a $(SDK)/native.ld
	$(LD) --gc-sections --build-id=none -z max-page-size=4096 -T $(SDK)/native.ld $(SDK)/lib/crt0.o $(OBJECTS) --start-group $(SDK)/lib/liblume.a $(SDK)/lib/libc.a --end-group -o $@
$(OUT)/svcd: $(OUT)/service
	cp $< $@
$(OUT)/service.o: $(OS_ROOT)/apps/common/service.c $(OS_ROOT)/apps/common/service_policy.h $(OS_ROOT)/apps/common/service_config.h $(OS_ROOT)/apps/common/service_platform.h
	mkdir -p $(OUT)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(OUT)/service_platform.o: $(OS_ROOT)/apps/aarch64/service_platform.c $(OS_ROOT)/apps/common/service_platform.h
	mkdir -p $(OUT)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
clean:
	rm -f $(OBJECTS) $(OUT)/service $(OUT)/svcd
