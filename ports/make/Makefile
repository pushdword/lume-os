ARCH ?= aarch64
PORT_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
OS_ROOT ?= $(PORT_ROOT)/../..
OS_ROOT := $(abspath $(OS_ROOT))
include $(OS_ROOT)/mk/port.mk
KERNEL_ROOT ?= $(OS_ROOT)/../lume-kernel
VENDOR := $(OS_ROOT)/vendor/make
BUILD ?= build
CC = clang
SDK ?= /data/sdk
SYSROOT ?= $(SDK)
BATCH ?= 1
PREFIX_FLAGS := -ffile-prefix-map=$(OS_ROOT)=lume-os -ffile-prefix-map=$(KERNEL_ROOT)=lume-kernel
CPPFLAGS += -isystem $(SYSROOT)/include  -I$(OS_ROOT)/arch/$(ARCH)/include -I$(OS_ROOT)/include/internal -I$(OS_ROOT)/include/posix  -I$(PORT_ROOT) -I$(VENDOR)/src -I$(VENDOR)/lib \
    -I$(OS_ROOT)/include/lume -I$(OS_ROOT)/include/posix \
    -I$(KERNEL_ROOT)/include/uapi -I$(KERNEL_ROOT)/include/abi/$(UAPI_ABI) \
    -DHAVE_CONFIG_H -D__lume__=1 -D_GNU_SOURCE -D_POSIX_MONOTONIC_CLOCK=200809L \
    -DLIBDIR=\"/data/sdk/lib\" -DLOCALEDIR=\"/data/share/locale\"
CFLAGS += -std=gnu17 -Oz -fno-pic -fno-stack-protector \
    -ffunction-sections -fdata-sections
SDK_BUILTINS := $(or $(wildcard $(SDK)/lib/libcompiler-rt.a),$(SDK)/lib/libcrt.a)
LDFLAGS ?= $(ARCH_CFLAGS) -nostdlib -static -fuse-ld=lld \
    -Wl,--gc-sections,--build-id=none,-z,max-page-size=4096 -Wl,-T,$(SDK)/native.ld $(SDK)/lib/crt0.o
LDLIBS ?= -Wl,--start-group $(SDK)/lib/liblume.a $(SDK)/lib/libc.a $(SDK_BUILTINS) -Wl,--end-group
NAMES := ar arscan commands default dir expand file function getopt getopt1 \
    guile hash implicit job load loadapi main misc output read remake rule \
    shuffle signame strcache variable version vpath posixos remote-stub
LIB_NAMES := concat-filename findprog-in fnmatch
OBJECTS := $(addprefix $(BUILD)/,$(addsuffix .o,$(NAMES) $(LIB_NAMES)))
OBJECTS += $(BUILD)/compat.o

all: $(BUILD)/make

$(BUILD):
	mkdir -p $@

ifeq ($(BATCH),1)
$(addprefix $(BUILD)/,$(addsuffix .o,$(NAMES))) &: $(addprefix $(VENDOR)/src/,$(addsuffix .c,$(NAMES))) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(PREFIX_FLAGS) -working-directory $(abspath $(BUILD)) -c $^

$(addprefix $(BUILD)/,$(addsuffix .o,$(LIB_NAMES))) &: $(addprefix $(VENDOR)/lib/,$(addsuffix .c,$(LIB_NAMES))) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(PREFIX_FLAGS) -working-directory $(abspath $(BUILD)) -c $^
else
$(addprefix $(BUILD)/,$(addsuffix .o,$(NAMES))): $(BUILD)/%.o: $(VENDOR)/src/%.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(PREFIX_FLAGS) -c $< -o $@

$(addprefix $(BUILD)/,$(addsuffix .o,$(LIB_NAMES))): $(BUILD)/%.o: $(VENDOR)/lib/%.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(PREFIX_FLAGS) -c $< -o $@
endif

$(BUILD)/compat.o: $(PORT_ROOT)/compat.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(PREFIX_FLAGS) -c $< -o $@

$(BUILD)/make: $(OBJECTS) $(PORT_ROOT)/Makefile $(SDK)/lib/liblume.a $(SDK)/lib/libc.a $(SDK_BUILTINS) $(SDK)/lib/crt0.o $(SDK)/native.ld
	$(CC) $(LDFLAGS) -Wl,--wrap=fileno $(OBJECTS) $(LDLIBS) -o $@

.PHONY: all
