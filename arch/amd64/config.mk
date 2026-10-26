TARGET ?= x86_64-unknown-none-elf
ARCH_CFLAGS := --target=$(TARGET) -m64 -march=x86-64 -mno-red-zone -ftls-model=local-exec
ARCH_CPPFLAGS := -DLUME_ARCH_AMD64=1 -DLUME_NATIVE_ABI=1
UAPI_ABI := native
