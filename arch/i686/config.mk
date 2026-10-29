TARGET ?= i686-unknown-none-elf
ARCH_CFLAGS := --target=$(TARGET) -m32 -march=i686 -mno-sse -mno-sse2 -mno-mmx -ftls-model=local-exec
ARCH_CPPFLAGS := -DLUME_ARCH_I686=1 -DLUME_NATIVE_ABI=1
UAPI_ABI := native
