LUME_SDK_ROOT := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))
include $(LUME_SDK_ROOT)/target.mk
LUME_SDK_CC ?= clang
LUME_SDK_LD ?= ld.lld
LUME_SDK_CPPFLAGS := -I$(LUME_SDK_ROOT)/include/posix -I$(LUME_SDK_ROOT)/include
LUME_SDK_CFLAGS := --target=$(LUME_SDK_TARGET) --sysroot=$(LUME_SDK_ROOT) \
    -O2 -ffunction-sections -fdata-sections -fno-pic -fno-pie -fno-stack-protector \
    -ftls-model=local-exec
ifeq ($(LUME_SDK_ARCH),aarch64)
LUME_SDK_CFLAGS += -mcpu=cortex-a76 -mstrict-align
else ifeq ($(LUME_SDK_ARCH),amd64)
LUME_SDK_CFLAGS += -m64 -march=x86-64 -mcmodel=large -mno-red-zone
else ifeq ($(LUME_SDK_ARCH),i686)
LUME_SDK_CFLAGS += -m32 -march=i686 -DLUME_NATIVE_LFS64=1
else
$(error Unsupported SDK architecture)
endif
LUME_SDK_CRT := $(LUME_SDK_ROOT)/lib/crt0.o
LUME_SDK_CXX_CRT := $(LUME_SDK_ROOT)/lib/crt0-cxx.o
LUME_SDK_LDFLAGS := --gc-sections -z max-page-size=4096 -T $(LUME_SDK_ROOT)/native.ld
LUME_SDK_LIBS := --start-group $(LUME_SDK_ROOT)/lib/liblume.a \
    $(LUME_SDK_ROOT)/lib/libc.a $(wildcard $(LUME_SDK_ROOT)/lib/libcompiler-rt.a) --end-group
