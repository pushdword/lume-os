TARGET ?= aarch64-unknown-none-elf
CPU ?= cortex-a76
ARCH_CFLAGS := --target=$(TARGET) -mcpu=$(CPU) -mstrict-align
ARCH_CPPFLAGS := -DLUME_ARCH_AARCH64=1
UAPI_ABI := native
