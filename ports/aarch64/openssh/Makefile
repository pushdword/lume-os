.DEFAULT_GOAL := all
PORT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
OS_ROOT := $(abspath $(PORT)/../../..)
KERNEL ?= $(OS_ROOT)/../lume-kernel
SOURCE := $(OS_ROOT)/vendor/openssh
SDK ?= /data/sdk
SYSROOT ?= $(SDK)
OUT ?= build
CC = clang
LD = ld.lld
AR = llvm-ar
include $(PORT)/objects.mk
CPPFLAGS += -DHAVE_CONFIG_H -D_GNU_SOURCE -I$(PORT) -I$(PORT)/include -I$(SOURCE) -I$(SOURCE)/openbsd-compat -I$(OS_ROOT)/include/common/posix -I$(OS_ROOT)/include/common -I$(KERNEL)/include/uapi/aarch64 -I$(KERNEL)/include/uapi/common -isystem $(SYSROOT)/include
CFLAGS += --target=aarch64-unknown-none-elf -mcpu=cortex-a76 -mstrict-align -O2 -std=gnu11 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -ffunction-sections -fdata-sections -ffile-prefix-map=$(OS_ROOT)=lume-os -ffile-prefix-map=$(KERNEL)=lume-kernel
NATIVE := $(addprefix $(OUT)/obj/native/,entry.o network.o platform.o process.o)
CORE := $(addprefix $(OUT)/obj/,$(LIBSSH_OBJS))
COMPAT_OBJS := $(addprefix $(OUT)/obj/compat/,$(OPENBSD) $(COMPAT) $(PORTS))
RUNTIME := $(OUT)/libc_aarch64_api.c.obj $(OUT)/libc_common_posix_posix_runtime.c.obj $(OUT)/libc_common_posix_process.c.obj
LIBRARIES := $(OUT)/libssh.a $(OUT)/libcompat.a $(OUT)/liblume.a $(SDK)/lib/libc.a
LINK = $(LD) --wrap=main --gc-sections --build-id=none -z max-page-size=4096 -T $(SDK)/native.ld $(SDK)/lib/crt0.o
HEADERS := $(wildcard $(PORT)/*.h $(PORT)/include/*.h $(PORT)/include/sys/*.h $(PORT)/include/netinet/*.h $(PORT)/include/arpa/*.h $(SOURCE)/*.h $(SOURCE)/openbsd-compat/*.h)
.PHONY: all
all: $(addprefix $(OUT)/,sshd sshd-session sshd-auth ssh-keygen sftp-server sshd-setup)
$(OUT)/sshd: $(addprefix $(OUT)/obj/,$(SSHDOBJS)) $(NATIVE) $(LIBRARIES)
	$(LINK) $(addprefix $(OUT)/obj/,$(SSHDOBJS)) $(NATIVE) --start-group $(LIBRARIES) --end-group -o $@
$(OUT)/sshd-session: $(addprefix $(OUT)/obj/,$(SSHD_SESSION_OBJS)) $(NATIVE) $(LIBRARIES)
	$(LINK) $(addprefix $(OUT)/obj/,$(SSHD_SESSION_OBJS)) $(NATIVE) --start-group $(LIBRARIES) --end-group -o $@
$(OUT)/sshd-auth: $(addprefix $(OUT)/obj/,$(SSHD_AUTH_OBJS)) $(NATIVE) $(LIBRARIES)
	$(LINK) $(addprefix $(OUT)/obj/,$(SSHD_AUTH_OBJS)) $(NATIVE) --start-group $(LIBRARIES) --end-group -o $@
$(OUT)/ssh-keygen: $(addprefix $(OUT)/obj/,ssh-keygen.o sshsig.o $(SKOBJS) $(P11OBJS)) $(NATIVE) $(LIBRARIES)
	$(LINK) $(addprefix $(OUT)/obj/,ssh-keygen.o sshsig.o $(SKOBJS) $(P11OBJS)) $(NATIVE) --start-group $(LIBRARIES) --end-group -o $@
$(OUT)/sftp-server: $(addprefix $(OUT)/obj/,sftp-server-main.o sftp-server.o sftp-common.o) $(NATIVE) $(LIBRARIES)
	$(LINK) $(addprefix $(OUT)/obj/,sftp-server-main.o sftp-server.o sftp-common.o) $(NATIVE) --start-group $(LIBRARIES) --end-group -o $@
$(OUT)/sshd-setup: $(OUT)/obj/tools/setup.o $(NATIVE) $(LIBRARIES)
	$(LINK) $(OUT)/obj/tools/setup.o $(NATIVE) --start-group $(LIBRARIES) --end-group -o $@
$(OUT)/libssh.a: $(CORE)
	$(AR) rcs $@ $^
$(OUT)/libcompat.a: $(COMPAT_OBJS)
	$(AR) rcs $@ $^
$(OUT)/liblume.a: $(RUNTIME) $(SDK)/lib/liblume.a
	cp $(SDK)/lib/liblume.a $@
	$(AR) r $@ $(RUNTIME)
$(OUT)/obj/compat/%.o: $(SOURCE)/openbsd-compat/%.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(OUT)/obj/native/%.o: $(PORT)/%.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(OUT)/obj/tools/%.o: $(PORT)/tools/%.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(OUT)/obj/%.o: $(SOURCE)/%.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(OUT)/libc_aarch64_api.c.obj: $(OS_ROOT)/libc/aarch64/api.c
	mkdir -p $(@D)
	$(CC) $(filter-out -DHAVE_CONFIG_H -D_GNU_SOURCE -I$(PORT) -I$(PORT)/include -I$(SOURCE) -I$(SOURCE)/openbsd-compat,$(CPPFLAGS)) $(CFLAGS) -DLUME_ARCH_AARCH64=1 -c $< -o $@
$(OUT)/libc_common_posix_posix_runtime.c.obj: $(OS_ROOT)/libc/common/posix/posix_runtime.c $(OS_ROOT)/libc/common/runtime.c
	mkdir -p $(@D)
	$(CC) $(filter-out -DHAVE_CONFIG_H -D_GNU_SOURCE -I$(PORT) -I$(PORT)/include -I$(SOURCE) -I$(SOURCE)/openbsd-compat,$(CPPFLAGS)) $(CFLAGS) -DLUME_ARCH_AARCH64=1 -c $< -o $@
$(OUT)/libc_common_posix_process.c.obj: $(OS_ROOT)/libc/common/posix/process.c
	mkdir -p $(@D)
	$(CC) $(filter-out -DHAVE_CONFIG_H -D_GNU_SOURCE -I$(PORT) -I$(PORT)/include -I$(SOURCE) -I$(SOURCE)/openbsd-compat,$(CPPFLAGS)) $(CFLAGS) -DLUME_ARCH_AARCH64=1 -c $< -o $@
