OS_ROOT ?= ../../..
KERNEL_ROOT ?= $(OS_ROOT)/../lume-kernel
VENDOR := $(OS_ROOT)/vendor/make
BUILD ?= build
CC ?= clang
SDK ?= /data/sdk
CPPFLAGS += -I. -I$(VENDOR)/src -I$(VENDOR)/lib \
    -I$(OS_ROOT)/include/common -I$(OS_ROOT)/include/common/posix \
    -I$(KERNEL_ROOT)/include/uapi/common -I$(KERNEL_ROOT)/include/uapi/aarch64 \
    -DHAVE_CONFIG_H -DLUME_ARCH_AARCH64=1 -D__lume__=1 -D_GNU_SOURCE -D_POSIX_MONOTONIC_CLOCK=200809L \
    -DLIBDIR=\"/data/sdk/lib\" -DLOCALEDIR=\"/data/share/locale\"
CFLAGS += -std=gnu17 -Oz -mcpu=cortex-a76 -fno-pic -fno-stack-protector \
    -ffunction-sections -fdata-sections
NAMES := ar arscan commands default dir expand file function getopt getopt1 \
    guile hash implicit job load loadapi main misc output read remake rule \
    shuffle signame strcache variable version vpath posixos remote-stub
LIB_NAMES := concat-filename findprog-in fnmatch glob getloadavg
OBJECTS := $(addprefix $(BUILD)/,$(addsuffix .o,$(NAMES) $(LIB_NAMES)))
POSIX_NAMES := directory process filesystem
OBJECTS += $(addprefix $(BUILD)/posix-,$(addsuffix .o,$(POSIX_NAMES)))
OBJECTS += $(BUILD)/compat.o

all: $(BUILD)/make

$(BUILD):
	mkdir -p $@

$(BUILD)/%.o: $(VENDOR)/src/%.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(VENDOR)/lib/%.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/posix-%.o: $(OS_ROOT)/libc/common/posix/%.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/compat.o: compat.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/make: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.PHONY: all
