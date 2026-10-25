.DEFAULT_GOAL := all
KERNEL := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../../../lume-kernel)
SDK ?= /data/sdk
OUT ?= build
CC = clang
AR = llvm-ar
BATCH ?= 1
SOURCES := $(addprefix $(KERNEL)/src/aarch64/,device.c irq.c dma.c block.c work.c) $(KERNEL)/arch/aarch64/dma.c
OBJECTS := $(OUT)/device.o $(OUT)/irq.o $(OUT)/dma.o $(OUT)/block.o $(OUT)/work.o $(OUT)/arch-dma.o
FLAGS := --target=aarch64-unknown-none-elf -mcpu=cortex-a76 -mstrict-align -O2 -std=c11 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -nostdinc -isystem $(SDK)/bare -I$(KERNEL)/include/ddk -I$(KERNEL)/include/uapi/common -I$(KERNEL)/include/uapi/aarch64 -I$(KERNEL)/include/internal/aarch64
.PHONY: all
all: $(OUT)/liblume-ddk.a
ifeq ($(BATCH),1)
$(filter-out $(OUT)/arch-dma.o,$(OBJECTS)) &: $(filter-out $(KERNEL)/arch/aarch64/dma.c,$(SOURCES))
	mkdir -p $(OUT)
	$(CC) $(FLAGS) -working-directory $(abspath $(OUT)) -c $^
else
$(OUT)/%.o: $(KERNEL)/src/aarch64/%.c
	mkdir -p $(OUT)
	$(CC) $(FLAGS) -c $< -o $@
endif
$(OUT)/arch-dma.o: $(KERNEL)/arch/aarch64/dma.c
	mkdir -p $(OUT)
	$(CC) $(FLAGS) -c $< -o $@
$(OUT)/liblume-ddk.a: $(OBJECTS)
	$(AR) rcs $@ $^
