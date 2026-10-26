ARCH ?= aarch64
.DEFAULT_GOAL := all
OS_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../..)
include $(OS_ROOT)/mk/kernel-path.mk
include $(OS_ROOT)/arch/$(ARCH)/config.mk
KERNEL ?= $(OS_ROOT)/../lume-kernel
SDK ?= /data/sdk
SYSROOT ?= $(SDK)
OUT ?= build
CC = clang
LD = ld.lld
CPPFLAGS += -I$(OS_ROOT)/arch/$(ARCH)/include -I$(OS_ROOT)/include/internal -I$(OS_ROOT)/include/posix  -I$(OS_ROOT)/apps/utils -I$(OS_ROOT)/apps/diagnostics -I$(OS_ROOT)/services/svcd -I$(OS_ROOT)/apps/pkg -I$(OS_ROOT)/include/posix -I$(OS_ROOT)/include/lume -I$(KERNEL)/include/abi/$(UAPI_ABI) -I$(KERNEL)/include/uapi -isystem $(SYSROOT)/include
CFLAGS += $(ARCH_CFLAGS) -O2 -std=gnu11 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -ffunction-sections -fdata-sections
OBJECTS := $(OUT)/service.o $(OUT)/service_platform.o
.PHONY: all clean
all: $(OUT)/service $(OUT)/svcd
$(OUT)/service: $(OBJECTS) $(SDK)/lib/crt0.o $(SDK)/lib/liblume.a $(SDK)/lib/libc.a $(SDK)/native.ld
	$(LD) --gc-sections --build-id=none -z max-page-size=4096 -T $(SDK)/native.ld $(SDK)/lib/crt0.o $(OBJECTS) --start-group $(SDK)/lib/liblume.a $(SDK)/lib/libc.a --end-group -o $@
$(OUT)/svcd: $(OUT)/service
	cp $< $@
$(OUT)/service.o: $(OS_ROOT)/services/svcd/service.c $(OS_ROOT)/services/svcd/service_policy.h $(OS_ROOT)/services/svcd/service_config.h $(OS_ROOT)/services/svcd/service_platform.h
	mkdir -p $(OUT)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(OUT)/service_platform.o: $(OS_ROOT)/services/svcd/platform.c $(OS_ROOT)/services/svcd/service_platform.h
	mkdir -p $(OUT)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
clean:
	rm -f $(OBJECTS) $(OUT)/service $(OUT)/svcd
