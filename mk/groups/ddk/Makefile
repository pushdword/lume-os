ARCH ?= aarch64
.DEFAULT_GOAL := all
OS_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../..)
include $(OS_ROOT)/mk/kernel-path.mk
include $(OS_ROOT)/arch/$(ARCH)/config.mk
KERNEL ?= $(OS_ROOT)/../lume-kernel
SDK ?= /data/sdk
OUT ?= build
CC = clang
AR = llvm-ar
BATCH ?= 1
SOURCES := $(addprefix $(KERNEL)/,src/device/device.c src/device/irq.c src/dma/dma.c src/storage/block.c src/core/work.c) $(KERNEL)/arch/$(ARCH)/dma.c
OBJECTS := $(OUT)/device.o $(OUT)/irq.o $(OUT)/dma.o $(OUT)/block.o $(OUT)/work.o $(OUT)/arch-dma.o
FLAGS := $(ARCH_CFLAGS) -O2 -std=c11 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -nostdinc -isystem $(SDK)/bare -I$(KERNEL)/include/ddk -I$(KERNEL)/include/uapi -I$(KERNEL)/include/abi/$(UAPI_ABI) -I$(KERNEL)/include/kernel -I$(KERNEL)/arch/$(ARCH)/include/kernel
.PHONY: all
all: $(OUT)/liblume-ddk.a
ifeq ($(BATCH),1)
$(filter-out $(OUT)/arch-dma.o,$(OBJECTS)) &: $(filter-out $(KERNEL)/arch/$(ARCH)/dma.c,$(SOURCES))
	mkdir -p $(OUT)
	$(CC) $(FLAGS) -working-directory $(abspath $(OUT)) -c $^
else
$(OUT)/device.o: $(KERNEL)/src/device/device.c
	mkdir -p $(OUT)
	$(CC) $(FLAGS) -c $< -o $@
$(OUT)/irq.o: $(KERNEL)/src/device/irq.c
	mkdir -p $(OUT)
	$(CC) $(FLAGS) -c $< -o $@
$(OUT)/dma.o: $(KERNEL)/src/dma/dma.c
	mkdir -p $(OUT)
	$(CC) $(FLAGS) -c $< -o $@
$(OUT)/block.o: $(KERNEL)/src/storage/block.c
	mkdir -p $(OUT)
	$(CC) $(FLAGS) -c $< -o $@
$(OUT)/work.o: $(KERNEL)/src/core/work.c
	mkdir -p $(OUT)
	$(CC) $(FLAGS) -c $< -o $@

endif
$(OUT)/arch-dma.o: $(KERNEL)/arch/$(ARCH)/dma.c
	mkdir -p $(OUT)
	$(CC) $(FLAGS) -c $< -o $@
$(OUT)/liblume-ddk.a: $(OBJECTS)
	$(AR) rcs $@ $^
