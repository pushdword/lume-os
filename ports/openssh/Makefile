ARCH ?= aarch64
.DEFAULT_GOAL := all
PORT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
OS_ROOT := $(abspath $(PORT)/../..)
include $(OS_ROOT)/mk/port.mk
SOURCE := $(OS_ROOT)/vendor/openssh
SDK ?= /data/sdk
SYSROOT ?= $(SDK)
OUT ?= build
CC = clang
LD = ld.lld
AR = llvm-ar
include $(PORT)/objects.mk
CPPFLAGS += -I$(OS_ROOT)/arch/$(ARCH)/include -I$(OS_ROOT)/include/internal -I$(OS_ROOT)/include/posix  -DHAVE_CONFIG_H -D_GNU_SOURCE -I$(PORT) -I$(PORT)/lume -I$(PORT)/lume/include -I$(SOURCE) -I$(SOURCE)/openbsd-compat -I$(OS_ROOT)/include/posix -I$(OS_ROOT)/include/lume -I$(KERNEL)/include/abi/$(UAPI_ABI) -I$(KERNEL)/include/uapi -isystem $(SYSROOT)/include
CFLAGS += -O2 -std=gnu11 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -ffunction-sections -fdata-sections -ffile-prefix-map=$(OS_ROOT)=lume-os -ffile-prefix-map=$(KERNEL)=lume-kernel
NATIVE := $(addprefix $(OUT)/obj/native/,entry.o network.o platform.o process.o)
CORE := $(addprefix $(OUT)/obj/,$(LIBSSH_OBJS))
COMPAT_OBJS := $(addprefix $(OUT)/obj/compat/,$(OPENBSD) $(COMPAT) $(PORTS))
LIBRARIES := $(OUT)/libssh.a $(OUT)/libcompat.a $(SDK)/lib/liblume.a $(SDK)/lib/libc.a
LINK_INPUTS := $(SDK)/lib/crt0.o $(SDK)/native.ld
LINK = $(LD) --wrap=main --wrap=poll --gc-sections --build-id=none -z max-page-size=4096 -T $(SDK)/native.ld $(SDK)/lib/crt0.o
HEADERS := $(wildcard $(OS_ROOT)/arch/$(ARCH)/include/lume/*.h) $(wildcard $(PORT)/*.h $(PORT)/lume/*.h $(PORT)/lume/include/*.h $(PORT)/lume/include/sys/*.h $(PORT)/lume/include/netinet/*.h $(PORT)/lume/include/arpa/*.h $(SOURCE)/*.h $(SOURCE)/openbsd-compat/*.h)
.PHONY: all
all: $(addprefix $(OUT)/,sshd sshd-session sshd-auth ssh-keygen sftp-server sshd-setup)
$(OUT)/sshd: $(addprefix $(OUT)/obj/,$(SSHDOBJS)) $(NATIVE) $(LIBRARIES) $(LINK_INPUTS)
	$(LINK) $(addprefix $(OUT)/obj/,$(SSHDOBJS)) $(NATIVE) --start-group $(LIBRARIES) --end-group -o $@
$(OUT)/sshd-session: $(addprefix $(OUT)/obj/,$(SSHD_SESSION_OBJS)) $(NATIVE) $(LIBRARIES) $(LINK_INPUTS)
	$(LINK) $(addprefix $(OUT)/obj/,$(SSHD_SESSION_OBJS)) $(NATIVE) --start-group $(LIBRARIES) --end-group -o $@
$(OUT)/sshd-auth: $(addprefix $(OUT)/obj/,$(SSHD_AUTH_OBJS)) $(NATIVE) $(LIBRARIES) $(LINK_INPUTS)
	$(LINK) $(addprefix $(OUT)/obj/,$(SSHD_AUTH_OBJS)) $(NATIVE) --start-group $(LIBRARIES) --end-group -o $@
$(OUT)/ssh-keygen: $(addprefix $(OUT)/obj/,ssh-keygen.o sshsig.o $(SKOBJS) $(P11OBJS)) $(NATIVE) $(LIBRARIES) $(LINK_INPUTS)
	$(LINK) $(addprefix $(OUT)/obj/,ssh-keygen.o sshsig.o $(SKOBJS) $(P11OBJS)) $(NATIVE) --start-group $(LIBRARIES) --end-group -o $@
$(OUT)/sftp-server: $(addprefix $(OUT)/obj/,sftp-server-main.o sftp-server.o sftp-common.o) $(NATIVE) $(LIBRARIES) $(LINK_INPUTS)
	$(LINK) $(addprefix $(OUT)/obj/,sftp-server-main.o sftp-server.o sftp-common.o) $(NATIVE) --start-group $(LIBRARIES) --end-group -o $@
$(OUT)/sshd-setup: $(OUT)/obj/tools/setup.o $(NATIVE) $(LIBRARIES) $(LINK_INPUTS)
	$(LINK) $(OUT)/obj/tools/setup.o $(NATIVE) --start-group $(LIBRARIES) --end-group -o $@
$(OUT)/libssh.a: $(CORE)
	$(AR) rcs $@ $^
$(OUT)/libcompat.a: $(COMPAT_OBJS)
	$(AR) rcs $@ $^
$(OUT)/obj/compat/%.o: $(SOURCE)/openbsd-compat/%.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(OUT)/obj/native/%.o: $(PORT)/lume/%.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(OUT)/obj/tools/%.o: $(PORT)/tools/%.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(OUT)/obj/%.o: $(SOURCE)/%.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
HOSTCC ?= cc
HOSTCFLAGS ?= -O2 -std=c11
HOSTEXE := $(if $(filter Windows_NT,$(OS)),.exe,)
.PHONY: check-poll
check-poll: $(OUT)/tests/poll$(HOSTEXE)
	env "$<"
$(OUT)/tests/poll$(HOSTEXE): $(OS_ROOT)/tests/unit/openssh_poll_test.c $(PORT)/lume/poll_wait.h $(KERNEL)/include/uapi/fd.h $(KERNEL)/include/abi/native/lume/signal.h
	mkdir -p "$(@D)"
	$(HOSTCC) $(HOSTCFLAGS) -UNDEBUG $(ARCH_CPPFLAGS) -I$(KERNEL)/include/uapi -I$(KERNEL)/include/abi/native "$<" -o "$@"
