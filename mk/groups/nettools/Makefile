ARCH ?= aarch64
SHELL := /bin/sh
OS_ROOT ?= ../../..
OS_ROOT := $(abspath $(OS_ROOT))
include $(OS_ROOT)/mk/kernel-path.mk
include $(OS_ROOT)/arch/$(ARCH)/config.mk
KERNEL ?= $(OS_ROOT)/../lume-kernel
SDK ?= /data
OUT ?= $(OS_ROOT)/build/native-nettools
CC := $(SDK)/bin/clang
CPPFLAGS := -I$(OS_ROOT)/arch/$(ARCH)/include -I$(OS_ROOT)/include/internal -I$(OS_ROOT)/include/posix  -nostdinc -isystem $(SDK)/sdk/bare -I$(KERNEL)/include/abi/$(UAPI_ABI) -I$(KERNEL)/include/uapi
CFLAGS := -O2 -std=c11 $(filter-out -mstrict-align,$(ARCH_CFLAGS)) -fno-builtin -fno-stack-protector -ffreestanding -ffunction-sections -fdata-sections
SOURCES := $(OS_ROOT)/arch/$(ARCH)/start_bare.S $(OS_ROOT)/libc/string/memory.c
LINK := -nostdlib -Wl,--threads=1,--gc-sections,-z,max-page-size=4096,-T,$(OS_ROOT)/arch/$(ARCH)/user.ld
.PHONY: all install
all: $(OUT)/ping $(OUT)/nc
$(OUT)/ping $(OUT)/nc: $(OUT)/%: $(OS_ROOT)/apps/net/%.c $(OS_ROOT)/apps/net/network_tool.h $(OS_ROOT)/include/internal/lume/app_support.h $(SOURCES) $(KERNEL)/include/abi/$(UAPI_ABI)/lume/io.h
	mkdir -p "$(OUT)"
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LINK) "$<" $(SOURCES) -o "$@"
install: all
	cp "$(OUT)/ping" "$(SDK)/bin/ping"
	cp "$(OUT)/nc" "$(SDK)/bin/nc"
	cp "$(OUT)/nc" "$(SDK)/bin/netcat"
