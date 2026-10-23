SHELL := /bin/sh
OS_ROOT ?= ../../..
OS_ROOT := $(abspath $(OS_ROOT))
KERNEL ?= $(OS_ROOT)/../lume-kernel
SDK ?= /data
OUT ?= $(OS_ROOT)/build/native-nettools
CC := $(SDK)/bin/clang
CPPFLAGS := -nostdinc -isystem $(SDK)/sdk/bare -I$(KERNEL)/include/uapi/aarch64
CFLAGS := -O2 -std=c11 -mcpu=cortex-a76 -fno-builtin -fno-stack-protector -ffreestanding -ffunction-sections -fdata-sections
SOURCES := $(OS_ROOT)/apps/aarch64/start.S $(OS_ROOT)/libc/aarch64/memory.c
LINK := -nostdlib -Wl,--threads=1,--gc-sections,-z,max-page-size=4096,-T,$(OS_ROOT)/apps/aarch64/user.ld
.PHONY: all install
all: $(OUT)/ping $(OUT)/nc
$(OUT)/ping $(OUT)/nc: $(OUT)/%: $(OS_ROOT)/apps/aarch64/%.c $(OS_ROOT)/apps/aarch64/network_tool.h $(OS_ROOT)/apps/aarch64/lib.h $(SOURCES) $(KERNEL)/include/uapi/aarch64/lume/io.h
	mkdir -p "$(OUT)"
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LINK) "$<" $(SOURCES) -o "$@"
install: all
	cp "$(OUT)/ping" "$(SDK)/bin/ping"
	cp "$(OUT)/nc" "$(SDK)/bin/nc"
	cp "$(OUT)/nc" "$(SDK)/bin/netcat"
